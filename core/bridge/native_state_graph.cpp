/**
 * @file native_state_graph.cpp
 * @brief Stage 1 MVP StateGraph 实现
 */

#include "native_state_graph.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace mbink {

namespace {

std::string trimPath(const std::string& input) {
    size_t begin = 0;
    size_t end = input.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }
    return input.substr(begin, end - begin);
}

} // namespace

NativeStateGraph::NativeStateGraph() {
    std::lock_guard<std::mutex> lock(mutex_);
    rebuildNodesLocked();
}

bool NativeStateGraph::initialize(const std::string& path, const json& value) {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (existsLocked(normalized)) {
        return false;
    }
    return setLocked(normalized, value, false);
}

bool NativeStateGraph::exists(const std::string& path) const {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return existsLocked(normalized);
}

json NativeStateGraph::get(const std::string& path) const {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return getLocked(normalized);
}

bool NativeStateGraph::set(const std::string& path, const json& value) {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return setLocked(normalized, value, true);
}

bool NativeStateGraph::deletePath(const std::string& path) {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return deleteLocked(normalized, true);
}

uint64_t NativeStateGraph::subscribe(const std::string& path,
                                     SubscriptionMode mode,
                                     uint64_t bindingId,
                                     SubscriptionCallback callback) {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t subscriptionId = nextSubscriptionId_++;
    subscriptions_[subscriptionId] = SubscriptionRecord{
        subscriptionId,
        bindingId,
        normalized,
        mode,
        true,
        0,
        std::move(callback),
    };
    ensureNodeChainLocked(normalized);
    attachSubscriptionsLocked();
    return subscriptionId;
}

bool NativeStateGraph::unsubscribe(uint64_t subscriptionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(subscriptionId);
    if (it == subscriptions_.end() || !it->second.active) {
        return false;
    }
    it->second.active = false;
    attachSubscriptionsLocked();
    return true;
}

uint32_t NativeStateGraph::beginBatch() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++batchDepth_;
    return batchDepth_;
}

uint32_t NativeStateGraph::endBatch() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (batchDepth_ == 0) {
            return 0;
        }
        --batchDepth_;
        if (batchDepth_ != 0) {
            return batchDepth_;
        }
    }
    flush();
    std::lock_guard<std::mutex> lock(mutex_);
    return batchDepth_;
}

uint32_t NativeStateGraph::flush() {
    uint32_t deliveredCount = 0;
    uint32_t rounds = 0;
    std::vector<std::pair<std::string, uint64_t>> currentBatch;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastFlushedPaths_.clear();
    }

    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (batchDepth_ > 0 || isFlushing_) {
                return deliveredCount;
            }
            if (pendingMutationPaths_.empty()) {
                return deliveredCount;
            }

            isFlushing_ = true;
            reentrantMutationCount_ = 0;
            currentBatch.clear();

            for (const auto& path : pendingMutationPaths_) {
                const auto it = revisions_.find(path);
                currentBatch.emplace_back(path, it == revisions_.end() ? 0 : it->second);
                lastFlushedPaths_.push_back(path);
            }
            pendingMutationPaths_.clear();
        }

        for (const auto& [path, revision] : currentBatch) {
            std::vector<SubscriptionRecord> subscriptions;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                subscriptions = collectSubscriptionsLocked(path, revision);
            }

            for (const auto& subscription : subscriptions) {
                if (!subscription.callback) {
                    continue;
                }
                subscription.callback(path, revision);
                ++deliveredCount;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            isFlushing_ = false;
            if (deferredMutationPaths_.empty()) {
                return deliveredCount;
            }
            pendingMutationPaths_.insert(deferredMutationPaths_.begin(), deferredMutationPaths_.end());
            deferredMutationPaths_.clear();
            ++rounds;
            if (rounds > kMaxReentrantFlushRounds ||
                reentrantMutationCount_ > kMaxReentrantFlushRounds) {
                pendingMutationPaths_.clear();
                throw std::runtime_error("NativeStateGraph re-entrant mutation limit exceeded");
            }
        }
    }
}

size_t NativeStateGraph::pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingMutationPaths_.size() + deferredMutationPaths_.size();
}

uint64_t NativeStateGraph::revisionOf(const std::string& path) const {
    const std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = revisions_.find(normalized);
    return it == revisions_.end() ? 0 : it->second;
}

std::vector<std::string> NativeStateGraph::consumeFlushedPaths() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto paths = lastFlushedPaths_;
    lastFlushedPaths_.clear();
    return paths;
}

std::string NativeStateGraph::normalizePath(const std::string& path) {
    const std::string normalized = trimPath(path);
    if (normalized.empty()) {
        return {};
    }
    if (normalized.front() == '.' || normalized.back() == '.') {
        return {};
    }
    if (normalized.find("..") != std::string::npos) {
        return {};
    }
    return normalized;
}

std::vector<std::string> NativeStateGraph::splitPath(const std::string& path) {
    std::vector<std::string> segments;
    size_t start = 0;
    while (start < path.size()) {
        const size_t pos = path.find('.', start);
        const std::string segment = path.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        if (segment.empty()) {
            return {};
        }
        segments.push_back(segment);
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    return segments;
}

std::string NativeStateGraph::joinPath(const std::vector<std::string>& segments, size_t endExclusive) {
    std::string path;
    for (size_t i = 0; i < endExclusive; ++i) {
        if (!path.empty()) {
            path.push_back('.');
        }
        path += segments[i];
    }
    return path;
}

StateValueKind NativeStateGraph::classifyValue(const json& value) {
    if (value.is_null()) return StateValueKind::Null;
    if (value.is_boolean()) return StateValueKind::Bool;
    if (value.is_number()) return StateValueKind::Number;
    if (value.is_string()) return StateValueKind::String;
    if (value.is_object()) return StateValueKind::Object;
    if (value.is_array()) return StateValueKind::Array;
    return StateValueKind::Undefined;
}

bool NativeStateGraph::isSubtreeHit(const std::string& mutationPath, const std::string& subscriptionPath) {
    return mutationPath == subscriptionPath ||
           (mutationPath.size() > subscriptionPath.size() &&
            mutationPath.compare(0, subscriptionPath.size(), subscriptionPath) == 0 &&
            mutationPath[subscriptionPath.size()] == '.');
}

bool NativeStateGraph::existsLocked(const std::string& path) const {
    const auto segments = splitPath(path);
    if (segments.empty()) {
        return false;
    }

    const json* current = &root_;
    for (const auto& segment : segments) {
        if (!current->is_object()) {
            return false;
        }
        auto it = current->find(segment);
        if (it == current->end()) {
            return false;
        }
        current = &(*it);
    }
    return true;
}

json NativeStateGraph::getLocked(const std::string& path) const {
    const auto segments = splitPath(path);
    if (segments.empty()) {
        return nullptr;
    }

    const json* current = &root_;
    for (const auto& segment : segments) {
        if (!current->is_object()) {
            return nullptr;
        }
        auto it = current->find(segment);
        if (it == current->end()) {
            return nullptr;
        }
        current = &(*it);
    }
    return *current;
}

bool NativeStateGraph::setLocked(const std::string& path, const json& value, bool queueMutation) {
    const auto segments = splitPath(path);
    if (segments.empty()) {
        return false;
    }

    json* current = &root_;
    for (size_t i = 0; i + 1 < segments.size(); ++i) {
        json& child = (*current)[segments[i]];
        if (!child.is_object()) {
            child = json::object();
        }
        current = &child;
    }

    const std::string& leaf = segments.back();
    auto it = current->find(leaf);
    if (it != current->end() && *it == value) {
        return false;
    }

    (*current)[leaf] = value;
    revisions_[path] = ++globalRevision_;
    rebuildNodesLocked();
    if (queueMutation) {
        queueMutationLocked(path);
    }
    return true;
}

bool NativeStateGraph::deleteLocked(const std::string& path, bool queueMutation) {
    const auto segments = splitPath(path);
    if (segments.empty()) {
        return false;
    }

    json* current = &root_;
    for (size_t i = 0; i + 1 < segments.size(); ++i) {
        auto it = current->find(segments[i]);
        if (it == current->end() || !it->is_object()) {
            return false;
        }
        current = &(*it);
    }

    if (!current->is_object()) {
        return false;
    }

    if (current->erase(segments.back()) == 0) {
        return false;
    }

    revisions_[path] = ++globalRevision_;
    rebuildNodesLocked();
    if (queueMutation) {
        queueMutationLocked(path);
    }
    return true;
}

void NativeStateGraph::queueMutationLocked(const std::string& path) {
    if (isFlushing_) {
        deferredMutationPaths_.insert(path);
        ++reentrantMutationCount_;
        return;
    }
    pendingMutationPaths_.insert(path);
}

void NativeStateGraph::rebuildNodesLocked() {
    nodes_.clear();
    if (root_.is_object()) {
        for (auto it = root_.begin(); it != root_.end(); ++it) {
            buildNodeTreeLocked(it.key(), it.key(), std::string(), it.value());
        }
    }
    attachSubscriptionsLocked();
}

void NativeStateGraph::buildNodeTreeLocked(const std::string& path,
                                           const std::string& key,
                                           const std::string& parentPath,
                                           const json& value) {
    auto& node = nodes_[path];
    node.path = path;
    node.key = key;
    node.parentPath = parentPath;
    node.exists = true;
    node.value = value;
    node.valueKind = classifyValue(value);
    node.revision = revisions_.count(path) == 0 ? 0 : revisions_[path];
    node.children.clear();
    node.exactSubscriptions.clear();
    node.subtreeSubscriptions.clear();

    if (!parentPath.empty()) {
        nodes_[parentPath].children[key] = path;
    }

    if (!value.is_object()) {
        return;
    }

    for (auto it = value.begin(); it != value.end(); ++it) {
        const std::string childPath = path + "." + it.key();
        buildNodeTreeLocked(childPath, it.key(), path, it.value());
    }
}

void NativeStateGraph::ensureNodeChainLocked(const std::string& path) {
    const auto segments = splitPath(path);
    if (segments.empty()) {
        return;
    }

    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string currentPath = joinPath(segments, i + 1);
        auto& node = nodes_[currentPath];
        node.path = currentPath;
        node.key = segments[i];
        node.parentPath = i == 0 ? std::string() : joinPath(segments, i);
        if (!node.exists) {
            node.value = nullptr;
            node.valueKind = StateValueKind::Undefined;
            node.revision = revisions_.count(currentPath) == 0 ? 0 : revisions_[currentPath];
        }
        if (i > 0) {
            nodes_[node.parentPath].children[segments[i]] = currentPath;
        }
    }
}

void NativeStateGraph::attachSubscriptionsLocked() {
    for (auto& [_, node] : nodes_) {
        node.exactSubscriptions.clear();
        node.subtreeSubscriptions.clear();
    }

    for (const auto& [id, subscription] : subscriptions_) {
        if (!subscription.active) {
            continue;
        }
        ensureNodeChainLocked(subscription.path);
        auto& node = nodes_[subscription.path];
        if (subscription.mode == SubscriptionMode::Exact) {
            node.exactSubscriptions.insert(id);
        } else {
            node.subtreeSubscriptions.insert(id);
        }
    }
}

std::vector<SubscriptionRecord> NativeStateGraph::collectSubscriptionsLocked(const std::string& mutationPath,
                                                                             uint64_t revision) {
    std::vector<SubscriptionRecord> matches;
    std::set<uint64_t> seen;

    auto exactNodeIt = nodes_.find(mutationPath);
    if (exactNodeIt != nodes_.end()) {
        for (uint64_t id : exactNodeIt->second.exactSubscriptions) {
            auto recordIt = subscriptions_.find(id);
            if (recordIt == subscriptions_.end() || !recordIt->second.active) {
                continue;
            }
            recordIt->second.lastDeliveredRevision = revision;
            if (seen.insert(id).second) {
                matches.push_back(recordIt->second);
            }
        }
    }

    std::string currentPath = mutationPath;
    while (!currentPath.empty()) {
        auto nodeIt = nodes_.find(currentPath);
        if (nodeIt != nodes_.end()) {
            for (uint64_t id : nodeIt->second.subtreeSubscriptions) {
                auto recordIt = subscriptions_.find(id);
                if (recordIt == subscriptions_.end() || !recordIt->second.active) {
                    continue;
                }
                if (!isSubtreeHit(mutationPath, recordIt->second.path)) {
                    continue;
                }
                recordIt->second.lastDeliveredRevision = revision;
                if (seen.insert(id).second) {
                    matches.push_back(recordIt->second);
                }
            }
        }

        const size_t dot = currentPath.rfind('.');
        if (dot == std::string::npos) {
            break;
        }
        currentPath = currentPath.substr(0, dot);
    }

    std::sort(matches.begin(), matches.end(), [](const SubscriptionRecord& a, const SubscriptionRecord& b) {
        return a.subscriptionId < b.subscriptionId;
    });
    return matches;
}

} // namespace mbink