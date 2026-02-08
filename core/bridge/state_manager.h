/**
 * @file state_manager.h
 * @brief 跨语言状态管理器
 * 
 * StateManager 是 LightUI 跨语言绑定的核心组件，提供：
 * - 线程安全的状态存储
 * - 操作队列机制（写操作入队，主线程处理）
 * - 状态变化监听
 * - 批量操作支持
 */

#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <set>
#include <cstdint>

#include "nlohmann/json.hpp"

namespace lightui {

using json = nlohmann::json;

/**
 * @brief 状态值类型枚举
 */
enum class LightUIType {
    Null = 0,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object
};

/**
 * @brief 错误码枚举
 */
enum class LightUIError {
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

/**
 * @brief 状态操作类型
 */
enum class StateOp {
    // 通用
    Set,
    Delete,
    
    // 数值
    Increment,
    Multiply,
    
    // 数组
    ArrayPush,
    ArrayPop,
    ArrayShift,
    ArrayUnshift,
    ArrayRemove,
    ArrayClear,
    ArraySet,
    
    // 对象
    ObjectSet,
    ObjectRemove,
    ObjectClear,
    
    // 字符串
    StringAppend,
    StringPrepend
};


/**
 * @brief 状态操作结构
 */
struct StateOperation {
    StateOp op;
    std::string name;
    json value;
    std::string key;    // 用于对象操作
    int index = -1;     // 用于数组操作
};

/**
 * @brief 状态变化回调类型
 */
using StateCallback = std::function<void(const std::string& name, const json& value)>;

/**
 * @brief 监听器结构
 */
struct Watcher {
    int id;
    std::string name;
    StateCallback callback;
};

/**
 * @brief 线程安全的状态管理器
 * 
 * 设计原则：
 * - 读操作直接访问（共享锁）
 * - 写操作入队（互斥锁保护队列）
 * - 主线程调用 processQueue() 应用变更
 * - 变更后通知所有监听器
 */
class StateManager {
public:
    StateManager();
    ~StateManager();
    
    // ========== 创建 ==========
    
    /**
     * @brief 创建 null 类型状态
     * @param name 状态名称
     * @return 错误码
     */
    LightUIError createNull(const std::string& name);
    
    /**
     * @brief 创建 bool 类型状态
     */
    LightUIError createBool(const std::string& name, bool value);
    
    /**
     * @brief 创建 int64 类型状态
     */
    LightUIError createInt(const std::string& name, int64_t value);
    
    /**
     * @brief 创建 double 类型状态
     */
    LightUIError createDouble(const std::string& name, double value);
    
    /**
     * @brief 创建 string 类型状态
     */
    LightUIError createString(const std::string& name, const std::string& value);
    
    /**
     * @brief 创建空数组状态
     */
    LightUIError createArray(const std::string& name);
    
    /**
     * @brief 创建空对象状态
     */
    LightUIError createObject(const std::string& name);
    
    /**
     * @brief 从 JSON 创建状态
     */
    LightUIError createJson(const std::string& name, const json& value);
    
    // ========== 读取（线程安全） ==========
    
    /**
     * @brief 检查状态是否存在
     */
    bool exists(const std::string& name) const;
    
    /**
     * @brief 获取状态类型
     * @return 状态类型，不存在返回 Null
     */
    LightUIType type(const std::string& name) const;
    
    /**
     * @brief 获取 bool 值
     * @return 类型不匹配返回 false
     */
    bool getBool(const std::string& name) const;
    
    /**
     * @brief 获取 int64 值
     * @return 类型不匹配返回 0
     */
    int64_t getInt(const std::string& name) const;
    
    /**
     * @brief 获取 double 值
     * @return 类型不匹配返回 0.0
     */
    double getDouble(const std::string& name) const;
    
    /**
     * @brief 获取 string 值（返回内部缓存引用）
     * @return 类型不匹配返回空字符串
     */
    const std::string& getString(const std::string& name) const;
    
    /**
     * @brief 获取 JSON 值（深拷贝）
     */
    json getJson(const std::string& name) const;
    
    /**
     * @brief 获取数组元素
     * @return 越界或类型不匹配返回 null
     */
    json getAt(const std::string& name, int index) const;
    
    /**
     * @brief 获取对象属性
     * @return 不存在或类型不匹配返回 null
     */
    json getKey(const std::string& name, const std::string& key) const;
    
    /**
     * @brief 获取数组/字符串长度
     * @return 类型不匹配返回 0
     */
    size_t getLength(const std::string& name) const;

    
    // ========== 写入（入队） ==========
    
    LightUIError setNull(const std::string& name);
    LightUIError setBool(const std::string& name, bool value);
    LightUIError setInt(const std::string& name, int64_t value);
    LightUIError setDouble(const std::string& name, double value);
    LightUIError setString(const std::string& name, const std::string& value);
    LightUIError setJson(const std::string& name, const json& value);
    LightUIError remove(const std::string& name);
    
    // ========== 数组操作（入队） ==========
    
    LightUIError arrayPush(const std::string& name, const json& item);
    LightUIError arrayPop(const std::string& name);
    LightUIError arrayShift(const std::string& name);
    LightUIError arrayUnshift(const std::string& name, const json& item);
    LightUIError arrayRemove(const std::string& name, int index);
    LightUIError arrayClear(const std::string& name);
    LightUIError arraySet(const std::string& name, int index, const json& item);
    
    // ========== 对象操作（入队） ==========
    
    LightUIError objectSet(const std::string& name, const std::string& key, const json& value);
    LightUIError objectRemove(const std::string& name, const std::string& key);
    LightUIError objectClear(const std::string& name);
    
    // ========== 数值操作（入队） ==========
    
    LightUIError increment(const std::string& name, double delta);
    LightUIError multiply(const std::string& name, double factor);
    
    // ========== 字符串操作（入队） ==========
    
    LightUIError stringAppend(const std::string& name, const std::string& suffix);
    LightUIError stringPrepend(const std::string& name, const std::string& prefix);
    
    // ========== 监听 ==========
    
    /**
     * @brief 监听状态变化
     * @return 唯一的 watch_id
     */
    int watch(const std::string& name, StateCallback callback);
    
    /**
     * @brief 取消监听
     */
    void unwatch(int watchId);

    /**
     * @brief 清理所有监听器（释放其捕获资源）
     */
    void clearWatchers();

    // ========== 批量/队列 ==========

    /**
     * @brief 进入批量模式
     */
    void batchBegin();
    
    /**
     * @brief 退出批量模式并触发通知
     */
    void batchEnd();
    
    /**
     * @brief 设置合并模式
     * @param enable 启用后，同名 SET 操作只保留最后一次
     */
    void setMergeMode(bool enable);
    
    /**
     * @brief 处理操作队列（主线程调用）
     */
    void processQueue();
    
    /**
     * @brief 获取队列大小
     */
    size_t queueSize() const;

private:
    // 验证状态名称
    bool isValidName(const std::string& name) const;
    
    // 入队操作
    void enqueue(StateOperation op);
    
    // 应用单个操作
    void applyOp(const StateOperation& op);
    
    // 通知监听器
    void notify(const std::string& name);
    
    // 从 json 类型转换为 LightUIType
    static LightUIType jsonTypeToLightUIType(const json& j);
    
    // 状态存储
    std::unordered_map<std::string, json> states_;
    mutable std::shared_mutex statesMutex_;
    
    // 字符串缓存（用于 getString 返回引用）
    mutable std::unordered_map<std::string, std::string> stringCache_;
    static const std::string emptyString_;
    
    // 操作队列
    std::deque<StateOperation> opQueue_;
    mutable std::mutex queueMutex_;
    bool mergeMode_ = true;
    
    // 监听器
    std::vector<Watcher> watchers_;
    int nextWatcherId_ = 0;
    std::mutex watchersMutex_;
    
    // 批量模式
    bool batchMode_ = false;
    std::set<std::string> batchChanges_;
};

} // namespace lightui
