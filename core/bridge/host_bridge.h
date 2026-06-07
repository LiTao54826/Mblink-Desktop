/**
 * @file host_bridge.h
 * @brief JS Host Bridge - 连接 JavaScript 和宿主语言
 * 
 * 提供 JS 端的 host 对象：
 * - host.call(name, args) - 调用宿主函数
 * - host.state.get(name) - 读取共享状态
 * - host.state.set(name, value) - 写入共享状态
 * - host.state.watch(name, callback) - 监听状态变化
 */

#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

extern "C" {
#include "quickjs/quickjs.h"
}

namespace mbink {

class StateManager;
class BackgroundTaskRunner;

/**
 * @brief 宿主函数回调类型
 * @param args JSON 格式的参数
 * @return JSON 格式的返回值
 */
using HostCallback = std::function<std::string(const std::string& args)>;
using HostAsyncCallback = std::function<std::string(const std::string& args)>;

/**
 * @brief 宿主函数信息
 */
struct HostFunction {
    HostCallback callback;
    void* userData = nullptr;
};

struct HostAsyncFunction {
    HostAsyncCallback callback;
    void* userData = nullptr;
};

/**
 * @brief 宿主事件监听器
 */
struct HostEventListener {
    int id;                 ///< 唯一 ListenerId
    std::string eventName;  ///< 事件名
    JSValue callback;       ///< JS 回调函数（引用计数管理）
};

/**
 * @brief 待处理事件
 */
struct PendingEvent {
    std::string eventName;  ///< 事件名称
    std::string dataJson;   ///< JSON 序列化的事件数据
};

/**
 * @brief JS Host Bridge
 * 
 * 将 StateManager 暴露给 JavaScript，并支持宿主函数绑定。
 * 同时提供事件分发系统，支持宿主层主动向 JS 端推送事件。
 */
class HostBridge {
public:
    /**
     * @brief 构造函数
     * @param ctx QuickJS 上下文
     * @param stateManager 状态管理器
     */
    HostBridge(JSContext* ctx,
               StateManager* stateManager,
               std::shared_ptr<BackgroundTaskRunner> background_runner = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~HostBridge();
    
    // 禁止拷贝
    HostBridge(const HostBridge&) = delete;
    HostBridge& operator=(const HostBridge&) = delete;
    
    /**
     * @brief 注册 host 对象到 JS 全局
     */
    void registerGlobal();
    
    /**
     * @brief 绑定宿主函数
     * @param name 函数名
     * @param callback 回调函数
     * @param userData 用户数据
     */
    void bind(const std::string& name, HostCallback callback, void* userData = nullptr);
    void bindAsync(const std::string& name, HostAsyncCallback callback, void* userData = nullptr);
    
    /**
     * @brief 解绑宿主函数
     * @param name 函数名
     */
    void unbind(const std::string& name);
    
    /**
     * @brief 调用宿主函数
     * @param name 函数名
     * @param args JSON 参数
     * @return JSON 返回值
     */
    std::string call(const std::string& name, const std::string& args);
    bool hasAsyncFunction(const std::string& name) const;
    
    /**
     * @brief 获取状态管理器
     */
    StateManager* getStateManager() const { return stateManager_; }
    
    /**
     * @brief 获取 JS 上下文
     */
    JSContext* getContext() const { return ctx_; }

    // ========== 事件系统 ==========

    /**
     * @brief 注册事件监听器
     * @param eventName 事件名
     * @param callback JS 回调函数
     * @return 唯一的 ListenerId
     */
    int on(const std::string& eventName, JSValue callback);

    /**
     * @brief 移除事件监听器
     * @param listenerId 监听器 ID
     */
    void off(int listenerId);

    /**
     * @brief 发送事件到队列（线程安全）
     * @param eventName 事件名
     * @param dataJson JSON 序列化的事件数据
     */
    void emit(const std::string& eventName, const std::string& dataJson);

    /**
     * @brief 刷新事件队列，执行 JS 回调（主线程调用）
     */
    void flushEvents();
    void flushAsyncResults();
    void cancelPendingPromises(const std::string& reason);

private:
    struct PendingPromise {
        uint64_t id;
        JSValue promise;
        JSValue resolve;
        JSValue reject;
    };

    struct AsyncCompletion {
        uint64_t promiseId;
        bool success;
        std::string payloadJson;
    };

    struct AsyncQueueState {
        std::vector<AsyncCompletion> completions;
        std::mutex mutex;
        std::atomic<bool> alive{true};
    };

    JSContext* ctx_;
    StateManager* stateManager_;
    std::shared_ptr<BackgroundTaskRunner> background_runner_;
    std::unordered_map<std::string, HostFunction> functions_;
    std::unordered_map<std::string, HostAsyncFunction> asyncFunctions_;
    
    // 事件监听器
    std::vector<HostEventListener> listeners_;
    int nextListenerId_ = 0;

    // 事件队列（线程安全）
    std::vector<PendingEvent> eventQueue_;
    std::mutex eventQueueMutex_;

    std::unordered_map<uint64_t, PendingPromise> pendingPromises_;
    std::mutex pendingPromisesMutex_;
    std::shared_ptr<AsyncQueueState> asyncQueueState_;
    std::atomic<uint64_t> nextPromiseId_{1};

    // 状态自动事件的 watcher 管理
    struct AutoWatcher {
        int watcherId;
        int listenerCount;
    };
    std::unordered_map<std::string, AutoWatcher> autoWatchers_;
    
    // JS 回调实现
    static JSValue jsCall(JSContext* ctx, JSValueConst thisVal, 
                          int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsBackendCall(JSContext* ctx, JSValueConst thisVal,
                                 int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateGet(JSContext* ctx, JSValueConst thisVal,
                              int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateSet(JSContext* ctx, JSValueConst thisVal,
                              int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateWatch(JSContext* ctx, JSValueConst thisVal,
                                int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateUnwatch(JSContext* ctx, JSValueConst thisVal,
                                  int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateExists(JSContext* ctx, JSValueConst thisVal,
                                 int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsStateType(JSContext* ctx, JSValueConst thisVal,
                               int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsHostOn(JSContext* ctx, JSValueConst thisVal,
                            int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsHostOff(JSContext* ctx, JSValueConst thisVal,
                             int argc, JSValueConst* argv, int magic, JSValue* func_data);
    JSValue createBackendFunction(const std::string& name);
    void installBoundFunction(const std::string& name);
    void removeBoundFunction(const std::string& name);
    static bool shouldRejectPayload(const std::string& payloadJson);
    static JSValue buildErrorValue(JSContext* ctx, const std::string& payloadJson);
    
    // 辅助函数
    static std::string jsValueToJson(JSContext* ctx, JSValueConst val);
    static JSValue jsonToJsValue(JSContext* ctx, const std::string& json);
};

} // namespace mbink
