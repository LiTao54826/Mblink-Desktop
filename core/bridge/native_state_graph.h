/**
 * @file native_state_graph.h
 * @brief Stage 1 MVP StateGraph 实现
 */

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nlohmann/json.hpp"

namespace mblink {

using json = nlohmann::json;

enum class SubscriptionMode {
    Exact = 0,
    Subtree = 1,
};

using SubscriptionCallback = std::function<void(const std::string& path, uint64_t revision)>;

enum class StateValueKind {
    Null,
    Bool,
    Number,
    String,
    Object,
    Array,
    Undefined,
};

struct StateGraphNode {
    std::string path;
    std::string key;
    std::string parentPath;
    bool exists = false;
    json value = nullptr;
    StateValueKind valueKind = StateValueKind::Undefined;
    uint64_t revision = 0;
    std::unordered_map<std::string, std::string> children;
    std::unordered_set<uint64_t> exactSubscriptions;
    std::unordered_set<uint64_t> subtreeSubscriptions;
};

struct SubscriptionRecord {
    uint64_t subscriptionId = 0;
    uint64_t bindingId = 0;
    std::string path;
    SubscriptionMode mode = SubscriptionMode::Exact;
    bool active = false;
    uint64_t lastDeliveredRevision = 0;
    SubscriptionCallback callback;
};

class NativeStateGraph {
public:
    NativeStateGraph();
    ~NativeStateGraph() = default;

    bool initialize(const std::string& path, const json& value);
    bool exists(const std::string& path) const;
    json get(const std::string& path) const;
    bool set(const std::string& path, const json& value);
    bool deletePath(const std::string& path);

    uint64_t subscribe(const std::string& path,
                       SubscriptionMode mode,
                       uint64_t bindingId,
                       SubscriptionCallback callback);
    bool unsubscribe(uint64_t subscriptionId);

    uint32_t beginBatch();
    uint32_t endBatch();
    uint32_t flush();
    size_t pendingCount() const;
    uint64_t revisionOf(const std::string& path) const;
    std::vector<std::string> consumeFlushedPaths();

private:
    static constexpr uint32_t kMaxReentrantFlushRounds = 32;

    static std::string normalizePath(const std::string& path);
    static std::vector<std::string> splitPath(const std::string& path);
    static std::string joinPath(const std::vector<std::string>& segments, size_t endExclusive);
    static StateValueKind classifyValue(const json& value);
    static bool isSubtreeHit(const std::string& mutationPath, const std::string& subscriptionPath);

    bool existsLocked(const std::string& path) const;
    json getLocked(const std::string& path) const;
    bool setLocked(const std::string& path, const json& value, bool queueMutation);
    bool deleteLocked(const std::string& path, bool queueMutation);
    void queueMutationLocked(const std::string& path);
    void rebuildNodesLocked();
    void buildNodeTreeLocked(const std::string& path,
                             const std::string& key,
                             const std::string& parentPath,
                             const json& value);
    void ensureNodeChainLocked(const std::string& path);
    void attachSubscriptionsLocked();
    std::vector<SubscriptionRecord> collectSubscriptionsLocked(const std::string& mutationPath,
                                                               uint64_t revision);

    mutable std::mutex mutex_;
    json root_ = json::object();
    std::unordered_map<std::string, StateGraphNode> nodes_;
    std::unordered_map<std::string, uint64_t> revisions_;
    std::unordered_map<uint64_t, SubscriptionRecord> subscriptions_;
    std::set<std::string> pendingMutationPaths_;
    std::set<std::string> deferredMutationPaths_;
    std::vector<std::string> lastFlushedPaths_;
    uint64_t nextSubscriptionId_ = 1;
    uint64_t globalRevision_ = 0;
    uint32_t batchDepth_ = 0;
    bool isFlushing_ = false;
    uint32_t reentrantMutationCount_ = 0;
};

} // namespace mblink