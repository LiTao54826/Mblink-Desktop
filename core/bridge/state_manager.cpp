/**
 * @file state_manager.cpp
 * @brief StateManager 实现
 */

#include "state_manager.h"
#include <algorithm>
#include <cctype>

namespace mbink {

// 静态成员初始化
const std::string StateManager::emptyString_;

StateManager::StateManager() = default;
StateManager::~StateManager() = default;

// ========== 辅助函数 ==========

bool StateManager::isValidName(const std::string& name) const {
    if (name.empty()) return false;
    // 检查是否全是空白字符
    return std::any_of(name.begin(), name.end(),
        [](unsigned char c) { return !std::isspace(c); });
}

MBinkType StateManager::jsonTypeToMBinkType(const json& j) {
    if (j.is_null()) return MBinkType::Null;
    if (j.is_boolean()) return MBinkType::Bool;
    if (j.is_number_integer()) return MBinkType::Int;
    if (j.is_number_float()) return MBinkType::Double;
    if (j.is_string()) return MBinkType::String;
    if (j.is_array()) return MBinkType::Array;
    if (j.is_object()) return MBinkType::Object;
    return MBinkType::Null;
}

// ========== 创建 ==========

MBinkError StateManager::createNull(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = nullptr;
    return MBinkError::Ok;
}

MBinkError StateManager::createBool(const std::string& name, bool value) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = value;
    return MBinkError::Ok;
}

MBinkError StateManager::createInt(const std::string& name, int64_t value) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = value;
    return MBinkError::Ok;
}

MBinkError StateManager::createDouble(const std::string& name, double value) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = value;
    return MBinkError::Ok;
}

MBinkError StateManager::createString(const std::string& name, const std::string& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = value;
    return MBinkError::Ok;
}

MBinkError StateManager::createArray(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = json::array();
    return MBinkError::Ok;
}

MBinkError StateManager::createObject(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = json::object();
    return MBinkError::Ok;
}

MBinkError StateManager::createJson(const std::string& name, const json& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;

    std::unique_lock lock(statesMutex_);
    if (states_.find(name) != states_.end()) {
        return MBinkError::AlreadyExists;
    }
    states_[name] = value;
    return MBinkError::Ok;
}


// ========== 读取 ==========

bool StateManager::exists(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    return states_.find(name) != states_.end();
}

MBinkType StateManager::type(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end()) {
        return MBinkType::Null;
    }
    return jsonTypeToMBinkType(it->second);
}

bool StateManager::getBool(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end() || !it->second.is_boolean()) {
        return false;
    }
    return it->second.get<bool>();
}

int64_t StateManager::getInt(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end()) return 0;

    if (it->second.is_number_integer()) {
        return it->second.get<int64_t>();
    }
    if (it->second.is_number_float()) {
        return static_cast<int64_t>(it->second.get<double>());
    }
    return 0;
}

double StateManager::getDouble(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end()) return 0.0;

    if (it->second.is_number()) {
        return it->second.get<double>();
    }
    return 0.0;
}

const std::string& StateManager::getString(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end() || !it->second.is_string()) {
        return emptyString_;
    }
    // 更新缓存并返回引用
    stringCache_[name] = it->second.get<std::string>();
    return stringCache_[name];
}

json StateManager::getJson(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end()) {
        return nullptr;
    }
    return it->second;  // 返回深拷贝
}

json StateManager::getAt(const std::string& name, int index) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end() || !it->second.is_array()) {
        return nullptr;
    }

    const auto& arr = it->second;
    if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
        return nullptr;
    }
    return arr[index];
}

json StateManager::getKey(const std::string& name, const std::string& key) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end() || !it->second.is_object()) {
        return nullptr;
    }

    const auto& obj = it->second;
    if (!obj.contains(key)) {
        return nullptr;
    }
    return obj[key];
}

size_t StateManager::getLength(const std::string& name) const {
    std::shared_lock lock(statesMutex_);
    auto it = states_.find(name);
    if (it == states_.end()) return 0;

    if (it->second.is_array()) {
        return it->second.size();
    }
    if (it->second.is_string()) {
        return it->second.get<std::string>().size();
    }
    return 0;
}


// ========== 写入（入队） ==========

MBinkError StateManager::setNull(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, nullptr, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::setBool(const std::string& name, bool value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, value, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::setInt(const std::string& name, int64_t value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, value, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::setDouble(const std::string& name, double value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, value, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::setString(const std::string& name, const std::string& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, value, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::setJson(const std::string& name, const json& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Set, name, value, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::remove(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Delete, name, nullptr, "", -1});
    return MBinkError::Ok;
}

// ========== 数组操作 ==========

MBinkError StateManager::arrayPush(const std::string& name, const json& item) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayPush, name, item, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::arrayPop(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayPop, name, nullptr, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::arrayShift(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayShift, name, nullptr, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::arrayUnshift(const std::string& name, const json& item) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayUnshift, name, item, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::arrayRemove(const std::string& name, int index) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayRemove, name, nullptr, "", index});
    return MBinkError::Ok;
}

MBinkError StateManager::arrayClear(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArrayClear, name, nullptr, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::arraySet(const std::string& name, int index, const json& item) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ArraySet, name, item, "", index});
    return MBinkError::Ok;
}

// ========== 对象操作 ==========

MBinkError StateManager::objectSet(const std::string& name, const std::string& key, const json& value) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ObjectSet, name, value, key, -1});
    return MBinkError::Ok;
}

MBinkError StateManager::objectRemove(const std::string& name, const std::string& key) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ObjectRemove, name, nullptr, key, -1});
    return MBinkError::Ok;
}

MBinkError StateManager::objectClear(const std::string& name) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::ObjectClear, name, nullptr, "", -1});
    return MBinkError::Ok;
}

// ========== 数值操作 ==========

MBinkError StateManager::increment(const std::string& name, double delta) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Increment, name, delta, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::multiply(const std::string& name, double factor) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::Multiply, name, factor, "", -1});
    return MBinkError::Ok;
}

// ========== 字符串操作 ==========

MBinkError StateManager::stringAppend(const std::string& name, const std::string& suffix) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::StringAppend, name, suffix, "", -1});
    return MBinkError::Ok;
}

MBinkError StateManager::stringPrepend(const std::string& name, const std::string& prefix) {
    if (!isValidName(name)) return MBinkError::InvalidName;
    enqueue({StateOp::StringPrepend, name, prefix, "", -1});
    return MBinkError::Ok;
}


// ========== 监听 ==========

int StateManager::watch(const std::string& name, StateCallback callback) {
    std::lock_guard lock(watchersMutex_);
    int id = nextWatcherId_++;
    watchers_.push_back({id, name, std::move(callback)});
    return id;
}

void StateManager::unwatch(int watchId) {
    std::lock_guard lock(watchersMutex_);
    watchers_.erase(
        std::remove_if(watchers_.begin(), watchers_.end(),
            [watchId](const Watcher& w) { return w.id == watchId; }),
        watchers_.end()
    );
}


void StateManager::clearWatchers() {
    std::lock_guard lock(watchersMutex_);
    watchers_.clear();
}

// ========== 批量/队列 ==========

void StateManager::batchBegin() {
    batchMode_ = true;
    batchChanges_.clear();
}

void StateManager::batchEnd() {
    if (!batchMode_) return;

    batchMode_ = false;

    // 通知所有累积的变更
    for (const auto& name : batchChanges_) {
        notify(name);
    }
    batchChanges_.clear();
}

void StateManager::setMergeMode(bool enable) {
    mergeMode_ = enable;
}

void StateManager::processQueue() {
    // 写操作已同步执行，这里只处理待通知
    std::set<std::string> changed;
    {
        std::lock_guard lock(notifyMutex_);
        std::swap(changed, pendingNotifications_);
    }

    if (changed.empty()) return;

    if (!batchMode_) {
        for (const auto& name : changed) {
            notify(name);
        }
    } else {
        batchChanges_.insert(changed.begin(), changed.end());
    }
}

size_t StateManager::queueSize() const {
    std::lock_guard lock(notifyMutex_);
    return pendingNotifications_.size();
}

// ========== 私有方法 ==========

void StateManager::enqueue(StateOperation op) {
    // 立即执行操作（同步写入 states_）
    std::string name = op.name;
    applyOp(op);
    
    // 记录待通知的状态名
    std::lock_guard lock(notifyMutex_);
    pendingNotifications_.insert(std::move(name));
}

void StateManager::applyOp(const StateOperation& op) {
    std::unique_lock lock(statesMutex_);

    switch (op.op) {
        case StateOp::Set:
            states_[op.name] = op.value;
            break;

        case StateOp::Delete:
            states_.erase(op.name);
            stringCache_.erase(op.name);
            break;

        case StateOp::Increment:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_number()) {
                states_[op.name] = states_[op.name].get<double>() + op.value.get<double>();
            }
            break;

        case StateOp::Multiply:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_number()) {
                states_[op.name] = states_[op.name].get<double>() * op.value.get<double>();
            }
            break;

        case StateOp::ArrayPush:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()) {
                states_[op.name].push_back(op.value);
            }
            break;

        case StateOp::ArrayPop:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()
                && !states_[op.name].empty()) {
                states_[op.name].erase(states_[op.name].end() - 1);
            }
            break;

        case StateOp::ArrayShift:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()
                && !states_[op.name].empty()) {
                states_[op.name].erase(states_[op.name].begin());
            }
            break;

        case StateOp::ArrayUnshift:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()) {
                states_[op.name].insert(states_[op.name].begin(), op.value);
            }
            break;

        case StateOp::ArrayRemove:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()) {
                auto& arr = states_[op.name];
                if (op.index >= 0 && static_cast<size_t>(op.index) < arr.size()) {
                    arr.erase(arr.begin() + op.index);
                }
            }
            break;

        case StateOp::ArrayClear:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()) {
                states_[op.name].clear();
            }
            break;

        case StateOp::ArraySet:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_array()) {
                auto& arr = states_[op.name];
                if (op.index >= 0 && static_cast<size_t>(op.index) < arr.size()) {
                    arr[op.index] = op.value;
                }
            }
            break;

        case StateOp::ObjectSet:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_object()) {
                states_[op.name][op.key] = op.value;
            }
            break;

        case StateOp::ObjectRemove:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_object()) {
                states_[op.name].erase(op.key);
            }
            break;

        case StateOp::ObjectClear:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_object()) {
                states_[op.name].clear();
            }
            break;

        case StateOp::StringAppend:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_string()) {
                states_[op.name] = states_[op.name].get<std::string>() + op.value.get<std::string>();
            }
            break;

        case StateOp::StringPrepend:
            if (states_.find(op.name) != states_.end() && states_[op.name].is_string()) {
                states_[op.name] = op.value.get<std::string>() + states_[op.name].get<std::string>();
            }
            break;
    }
}

void StateManager::notify(const std::string& name) {
    json value;
    {
        std::shared_lock lock(statesMutex_);
        auto it = states_.find(name);
        if (it != states_.end()) {
            value = it->second;
        }
    }

    std::lock_guard lock(watchersMutex_);
    for (const auto& watcher : watchers_) {
        if (watcher.name == name) {
            watcher.callback(name, value);
        }
    }
}

} // namespace mbink
