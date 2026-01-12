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
#include <memory>

extern "C" {
#include "quickjs/quickjs.h"
}

namespace lightui {

class StateManager;

/**
 * @brief 宿主函数回调类型
 * @param args JSON 格式的参数
 * @return JSON 格式的返回值
 */
using HostCallback = std::function<std::string(const std::string& args)>;

/**
 * @brief 宿主函数信息
 */
struct HostFunction {
    HostCallback callback;
    void* userData = nullptr;
};

/**
 * @brief JS Host Bridge
 * 
 * 将 StateManager 暴露给 JavaScript，并支持宿主函数绑定。
 */
class HostBridge {
public:
    /**
     * @brief 构造函数
     * @param ctx QuickJS 上下文
     * @param stateManager 状态管理器
     */
    HostBridge(JSContext* ctx, StateManager* stateManager);
    
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
    
    /**
     * @brief 获取状态管理器
     */
    StateManager* getStateManager() const { return stateManager_; }
    
    /**
     * @brief 获取 JS 上下文
     */
    JSContext* getContext() const { return ctx_; }

private:
    JSContext* ctx_;
    StateManager* stateManager_;
    std::unordered_map<std::string, HostFunction> functions_;
    
    // JS 回调实现
    static JSValue jsCall(JSContext* ctx, JSValueConst thisVal, 
                          int argc, JSValueConst* argv, int magic, JSValue* func_data);
    static JSValue jsPyCall(JSContext* ctx, JSValueConst thisVal,
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
    
    // 辅助函数
    static std::string jsValueToJson(JSContext* ctx, JSValueConst val);
    static JSValue jsonToJsValue(JSContext* ctx, const std::string& json);
};

} // namespace lightui
