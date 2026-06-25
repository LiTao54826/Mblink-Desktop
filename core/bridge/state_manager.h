/**
 * @file state_manager.h
 * @brief 基于 path 的宿主状态管理器
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"
#include "native_state_graph.h"

namespace mblink {

using json = nlohmann::json;

enum class MBlinkType {
    Null = 0,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object
};

enum class MBlinkError {
    Ok = 0,
    InvalidHandle = -1,
    NotFound = -2,
    TypeMismatch = -3,
    IndexOutOfRange = -4,
    InvalidJson = -5,
    AlreadyExists = -6,
    InvalidName = -7,
    QueueFull = -8,
    Unknown = -99
};

using StateCallback = std::function<void(const std::string& name, const json& value)>;

struct Watcher {
    int id;
    std::string name;
    StateCallback callback;
};

class StateManager {
public:
    StateManager();
    ~StateManager();

    MBlinkError createNull(const std::string& name);
    MBlinkError createBool(const std::string& name, bool value);
    MBlinkError createInt(const std::string& name, int64_t value);
    MBlinkError createDouble(const std::string& name, double value);
    MBlinkError createString(const std::string& name, const std::string& value);
    MBlinkError createArray(const std::string& name);
    MBlinkError createObject(const std::string& name);
    MBlinkError createJson(const std::string& name, const json& value);

    bool exists(const std::string& name) const;
    MBlinkType type(const std::string& name) const;
    bool getBool(const std::string& name) const;
    int64_t getInt(const std::string& name) const;
    double getDouble(const std::string& name) const;
    const std::string& getString(const std::string& name) const;
    json getJson(const std::string& name) const;
    json getAt(const std::string& name, int index) const;
    json getKey(const std::string& name, const std::string& key) const;
    size_t getLength(const std::string& name) const;

    MBlinkError setNull(const std::string& name);
    MBlinkError setBool(const std::string& name, bool value);
    MBlinkError setInt(const std::string& name, int64_t value);
    MBlinkError setDouble(const std::string& name, double value);
    MBlinkError setString(const std::string& name, const std::string& value);
    MBlinkError setJson(const std::string& name, const json& value);
    MBlinkError remove(const std::string& name);

    MBlinkError arrayPush(const std::string& name, const json& item);
    MBlinkError arrayPop(const std::string& name);
    MBlinkError arrayShift(const std::string& name);
    MBlinkError arrayUnshift(const std::string& name, const json& item);
    MBlinkError arrayRemove(const std::string& name, int index);
    MBlinkError arrayClear(const std::string& name);
    MBlinkError arraySet(const std::string& name, int index, const json& item);

    MBlinkError objectSet(const std::string& name, const std::string& key, const json& value);
    MBlinkError objectRemove(const std::string& name, const std::string& key);
    MBlinkError objectClear(const std::string& name);

    MBlinkError increment(const std::string& name, double delta);
    MBlinkError multiply(const std::string& name, double factor);
    MBlinkError stringAppend(const std::string& name, const std::string& suffix);
    MBlinkError stringPrepend(const std::string& name, const std::string& prefix);

    int watch(const std::string& name, StateCallback callback);
    void unwatch(int watchId);
    void clearWatchers();

    void batchBegin();
    void batchEnd();
    void setMergeMode(bool enable);
    void processQueue();
    size_t queueSize() const;

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
    uint64_t revisionOf(const std::string& path) const;

private:
    bool isValidName(const std::string& name) const;
    MBlinkType jsonTypeToMBlinkType(const json& j) const;
    MBlinkError createValue(const std::string& name, const json& value);
    MBlinkError mutateTopLevel(const std::string& name, const std::function<bool(json&)>& mutator);
    void notifyWatcher(const std::string& name);

    std::unique_ptr<NativeStateGraph> graph_;
    mutable std::unordered_map<std::string, std::string> stringCache_;
    mutable std::mutex stringCacheMutex_;
    std::vector<Watcher> watchers_;
    int nextWatcherId_ = 0;
    mutable std::mutex watchersMutex_;
    bool mergeMode_ = true;
    static const std::string emptyString_;
};

} // namespace mblink