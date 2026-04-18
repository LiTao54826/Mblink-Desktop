/**
 * @file state_manager.cpp
 * @brief StateManager 实现
 */

#include "state_manager.h"

#include <algorithm>
#include <cctype>

namespace mbink {

const std::string StateManager::emptyString_;

StateManager::StateManager()
    : graph_(std::make_unique<NativeStateGraph>()) {
}

StateManager::~StateManager() = default;

MBinkError StateManager::createNull(const std::string& name) {
    return createValue(name, nullptr);
}

MBinkError StateManager::createBool(const std::string& name, bool value) {
    return createValue(name, value);
}

MBinkError StateManager::createInt(const std::string& name, int64_t value) {
    return createValue(name, value);
}

MBinkError StateManager::createDouble(const std::string& name, double value) {
    return createValue(name, value);
}

MBinkError StateManager::createString(const std::string& name, const std::string& value) {
    return createValue(name, value);
}

MBinkError StateManager::createArray(const std::string& name) {
    return createValue(name, json::array());
}

MBinkError StateManager::createObject(const std::string& name) {
    return createValue(name, json::object());
}

MBinkError StateManager::createJson(const std::string& name, const json& value) {
    return createValue(name, value);
}

bool StateManager::exists(const std::string& name) const {
    return graph_->exists(name);
}

MBinkType StateManager::type(const std::string& name) const {
    return jsonTypeToMBinkType(graph_->get(name));
}

bool StateManager::getBool(const std::string& name) const {
    const json value = graph_->get(name);
    return value.is_boolean() ? value.get<bool>() : false;
}

int64_t StateManager::getInt(const std::string& name) const {
    const json value = graph_->get(name);
    if (value.is_number_integer()) return value.get<int64_t>();
    if (value.is_number_float()) return static_cast<int64_t>(value.get<double>());
    return 0;
}

double StateManager::getDouble(const std::string& name) const {
    const json value = graph_->get(name);
    return value.is_number() ? value.get<double>() : 0.0;
}

const std::string& StateManager::getString(const std::string& name) const {
    const json value = graph_->get(name);
    if (!value.is_string()) {
        return emptyString_;
    }
    std::lock_guard<std::mutex> lock(stringCacheMutex_);
    stringCache_[name] = value.get<std::string>();
    return stringCache_[name];
}

json StateManager::getJson(const std::string& name) const {
    return graph_->get(name);
}

json StateManager::getAt(const std::string& name, int index) const {
    const json value = graph_->get(name);
    if (!value.is_array() || index < 0 || static_cast<size_t>(index) >= value.size()) {
        return nullptr;
    }
    return value[index];
}

json StateManager::getKey(const std::string& name, const std::string& key) const {
    const json value = graph_->get(name);
    if (!value.is_object() || !value.contains(key)) {
        return nullptr;
    }
    return value[key];
}

size_t StateManager::getLength(const std::string& name) const {
    const json value = graph_->get(name);
    if (value.is_array()) return value.size();
    if (value.is_string()) return value.get<std::string>().size();
    return 0;
}

MBinkError StateManager::setNull(const std::string& name) { return set(name, nullptr) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::setBool(const std::string& name, bool value) { return set(name, value) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::setInt(const std::string& name, int64_t value) { return set(name, value) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::setDouble(const std::string& name, double value) { return set(name, value) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::setString(const std::string& name, const std::string& value) { return set(name, value) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::setJson(const std::string& name, const json& value) { return set(name, value) ? MBinkError::Ok : MBinkError::InvalidName; }
MBinkError StateManager::remove(const std::string& name) { return deletePath(name) ? MBinkError::Ok : MBinkError::InvalidName; }

MBinkError StateManager::arrayPush(const std::string& name, const json& item) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array()) return false;
        value.push_back(item);
        return true;
    });
}

MBinkError StateManager::arrayPop(const std::string& name) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array() || value.empty()) return false;
        value.erase(value.end() - 1);
        return true;
    });
}

MBinkError StateManager::arrayShift(const std::string& name) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array() || value.empty()) return false;
        value.erase(value.begin());
        return true;
    });
}

MBinkError StateManager::arrayUnshift(const std::string& name, const json& item) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array()) return false;
        value.insert(value.begin(), item);
        return true;
    });
}

MBinkError StateManager::arrayRemove(const std::string& name, int index) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array() || index < 0 || static_cast<size_t>(index) >= value.size()) return false;
        value.erase(value.begin() + index);
        return true;
    });
}

MBinkError StateManager::arrayClear(const std::string& name) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array()) return false;
        value.clear();
        return true;
    });
}

MBinkError StateManager::arraySet(const std::string& name, int index, const json& item) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_array() || index < 0 || static_cast<size_t>(index) >= value.size()) return false;
        value[index] = item;
        return true;
    });
}

MBinkError StateManager::objectSet(const std::string& name, const std::string& key, const json& value) {
    return mutateTopLevel(name, [&](json& object) {
        if (!object.is_object()) return false;
        object[key] = value;
        return true;
    });
}

MBinkError StateManager::objectRemove(const std::string& name, const std::string& key) {
    return mutateTopLevel(name, [&](json& object) {
        if (!object.is_object() || !object.contains(key)) return false;
        object.erase(key);
        return true;
    });
}

MBinkError StateManager::objectClear(const std::string& name) {
    return mutateTopLevel(name, [&](json& object) {
        if (!object.is_object()) return false;
        object.clear();
        return true;
    });
}

MBinkError StateManager::increment(const std::string& name, double delta) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_number()) return false;
        value = value.get<double>() + delta;
        return true;
    });
}

MBinkError StateManager::multiply(const std::string& name, double factor) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_number()) return false;
        value = value.get<double>() * factor;
        return true;
    });
}

MBinkError StateManager::stringAppend(const std::string& name, const std::string& suffix) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_string()) return false;
        value = value.get<std::string>() + suffix;
        return true;
    });
}

MBinkError StateManager::stringPrepend(const std::string& name, const std::string& prefix) {
    return mutateTopLevel(name, [&](json& value) {
        if (!value.is_string()) return false;
        value = prefix + value.get<std::string>();
        return true;
    });
}

int StateManager::watch(const std::string& name, StateCallback callback) {
    std::lock_guard<std::mutex> lock(watchersMutex_);
    const int id = nextWatcherId_++;
    watchers_.push_back({id, name, std::move(callback)});
    return id;
}

void StateManager::unwatch(int watchId) {
    std::lock_guard<std::mutex> lock(watchersMutex_);
    watchers_.erase(std::remove_if(watchers_.begin(), watchers_.end(), [watchId](const Watcher& watcher) {
        return watcher.id == watchId;
    }), watchers_.end());
}

void StateManager::clearWatchers() {
    std::lock_guard<std::mutex> lock(watchersMutex_);
    watchers_.clear();
}

void StateManager::batchBegin() { beginBatch(); }
void StateManager::batchEnd() { endBatch(); }
void StateManager::setMergeMode(bool enable) { mergeMode_ = enable; }

void StateManager::processQueue() {
    flush();
}

size_t StateManager::queueSize() const {
    return graph_->pendingCount();
}

json StateManager::get(const std::string& path) const {
    return graph_->get(path);
}

bool StateManager::set(const std::string& path, const json& value) {
    return isValidName(path) && graph_->set(path, value);
}

bool StateManager::deletePath(const std::string& path) {
    return isValidName(path) && graph_->deletePath(path);
}

uint64_t StateManager::subscribe(const std::string& path,
                                 SubscriptionMode mode,
                                 uint64_t bindingId,
                                 SubscriptionCallback callback) {
    return graph_->subscribe(path, mode, bindingId, std::move(callback));
}

bool StateManager::unsubscribe(uint64_t subscriptionId) {
    return graph_->unsubscribe(subscriptionId);
}

uint32_t StateManager::beginBatch() {
    return graph_->beginBatch();
}

uint32_t StateManager::endBatch() {
    const uint32_t depth = graph_->endBatch();
    for (const auto& path : graph_->consumeFlushedPaths()) {
        notifyWatcher(path);
    }
    return depth;
}

uint32_t StateManager::flush() {
    const uint32_t delivered = graph_->flush();
    for (const auto& path : graph_->consumeFlushedPaths()) {
        notifyWatcher(path);
    }
    return delivered;
}

uint64_t StateManager::revisionOf(const std::string& path) const {
    return graph_->revisionOf(path);
}

bool StateManager::isValidName(const std::string& name) const {
    if (name.empty()) return false;
    return std::any_of(name.begin(), name.end(), [](unsigned char c) {
        return !std::isspace(c);
    });
}

MBinkType StateManager::jsonTypeToMBinkType(const json& j) const {
    if (j.is_boolean()) return MBinkType::Bool;
    if (j.is_number_integer()) return MBinkType::Int;
    if (j.is_number_float()) return MBinkType::Double;
    if (j.is_string()) return MBinkType::String;
    if (j.is_array()) return MBinkType::Array;
    if (j.is_object()) return MBinkType::Object;
    return MBinkType::Null;
}

MBinkError StateManager::createValue(const std::string& name, const json& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    return graph_->initialize(name, value) ? MBinkError::Ok : MBinkError::AlreadyExists;
}

MBinkError StateManager::mutateTopLevel(const std::string& name, const std::function<bool(json&)>& mutator) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    json value = graph_->get(name);
    if (!mutator(value)) {
        return MBinkError::TypeMismatch;
    }
    return graph_->set(name, value) ? MBinkError::Ok : MBinkError::TypeMismatch;
}

void StateManager::notifyWatcher(const std::string& name) {
    const json value = graph_->get(name);
    std::lock_guard<std::mutex> lock(watchersMutex_);
    for (const auto& watcher : watchers_) {
        if (watcher.name == name) {
            watcher.callback(name, value);
        }
    }
}

} // namespace mbink