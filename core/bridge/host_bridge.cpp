/**
 * @file host_bridge.cpp
 * @brief JS Host Bridge 实现
 */

#include "host_bridge.h"
#include "state_manager.h"
#include "quickjs/js_value_wrapper.h"

extern "C" {
#include "quickjs.h"
}

#include <algorithm>
#include <cstdio>

namespace lightui {

HostBridge::HostBridge(JSContext* ctx, StateManager* stateManager)
    : ctx_(ctx), stateManager_(stateManager) {
}

HostBridge::~HostBridge() {
    // 释放所有 Listener 的 JS 回调引用
    for (auto& listener : listeners_) {
        JS_FreeValue(ctx_, listener.callback);
    }
    listeners_.clear();

    // 清理 auto-watchers
    if (stateManager_) {
        for (auto& [name, aw] : autoWatchers_) {
            stateManager_->unwatch(aw.watcherId);
        }
    }
    autoWatchers_.clear();
}

void HostBridge::registerGlobal() {
    if (!ctx_) return;
    
    JSValue global = JS_GetGlobalObject(ctx_);
    
    // 创建 host 对象
    JSValue host = JS_NewObject(ctx_);
    
    // 存储 this 指针到 func_data
    JSValue bridgePtr = JS_NewInt64(ctx_, reinterpret_cast<int64_t>(this));
    
    // host.call
    JS_SetPropertyStr(ctx_, host, "call",
        JS_NewCFunctionData(ctx_, jsCall, 2, 0, 1, &bridgePtr));
    
    // 创建 host.state 对象
    JSValue state = JS_NewObject(ctx_);
    
    // host.state.get
    JS_SetPropertyStr(ctx_, state, "get",
        JS_NewCFunctionData(ctx_, jsStateGet, 1, 0, 1, &bridgePtr));
    
    // host.state.set
    JS_SetPropertyStr(ctx_, state, "set",
        JS_NewCFunctionData(ctx_, jsStateSet, 2, 0, 1, &bridgePtr));
    
    // host.state.watch
    JS_SetPropertyStr(ctx_, state, "watch",
        JS_NewCFunctionData(ctx_, jsStateWatch, 2, 0, 1, &bridgePtr));
    
    // host.state.unwatch
    JS_SetPropertyStr(ctx_, state, "unwatch",
        JS_NewCFunctionData(ctx_, jsStateUnwatch, 1, 0, 1, &bridgePtr));
    
    // host.state.exists
    JS_SetPropertyStr(ctx_, state, "exists",
        JS_NewCFunctionData(ctx_, jsStateExists, 1, 0, 1, &bridgePtr));
    
    // host.state.type
    JS_SetPropertyStr(ctx_, state, "type",
        JS_NewCFunctionData(ctx_, jsStateType, 1, 0, 1, &bridgePtr));
    
    JS_SetPropertyStr(ctx_, host, "state", state);
    
    // host.on / host.off（事件系统）
    JS_SetPropertyStr(ctx_, host, "on",
        JS_NewCFunctionData(ctx_, jsHostOn, 2, 0, 1, &bridgePtr));
    JS_SetPropertyStr(ctx_, host, "off",
        JS_NewCFunctionData(ctx_, jsHostOff, 1, 0, 1, &bridgePtr));
    
    JS_SetPropertyStr(ctx_, global, "host", host);
    
    // 创建 py 命名空间对象（用于直接调用 Python 函数）
    // py.funcName(args) 等价于 host.call('funcName', args)
    JSValue py = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, global, "py", py);
    
    JS_FreeValue(ctx_, bridgePtr);
    JS_FreeValue(ctx_, global);
}

void HostBridge::bind(const std::string& name, HostCallback callback, void* userData) {
    functions_[name] = {std::move(callback), userData};
    
    // 同时注册到 py 命名空间，支持 py.funcName(args) 调用
    if (ctx_) {
        JSValue global = JS_GetGlobalObject(ctx_);
        JSValue py = JS_GetPropertyStr(ctx_, global, "py");
        
        if (!JS_IsUndefined(py)) {
            // 创建包含函数名和 bridge 指针的数据
            JSValue funcData[2];
            funcData[0] = JS_NewInt64(ctx_, reinterpret_cast<int64_t>(this));
            funcData[1] = JS_NewString(ctx_, name.c_str());
            
            // 创建 JS 函数
            JSValue func = JS_NewCFunctionData(ctx_, jsPyCall, 1, 0, 2, funcData);
            JS_SetPropertyStr(ctx_, py, name.c_str(), func);
            
            JS_FreeValue(ctx_, funcData[0]);
            JS_FreeValue(ctx_, funcData[1]);
        }
        
        JS_FreeValue(ctx_, py);
        JS_FreeValue(ctx_, global);
    }
}

void HostBridge::unbind(const std::string& name) {
    functions_.erase(name);
    
    // 从 py 命名空间移除
    if (ctx_) {
        JSValue global = JS_GetGlobalObject(ctx_);
        JSValue py = JS_GetPropertyStr(ctx_, global, "py");
        
        if (!JS_IsUndefined(py)) {
            JS_DeleteProperty(ctx_, py, JS_NewAtom(ctx_, name.c_str()), 0);
        }
        
        JS_FreeValue(ctx_, py);
        JS_FreeValue(ctx_, global);
    }
}

std::string HostBridge::call(const std::string& name, const std::string& args) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return R"({"error": "Function not found"})";
    }
    return it->second.callback(args);
}

// ========== JS 回调实现 ==========

JSValue HostBridge::jsCall(JSContext* ctx, JSValueConst thisVal,
                           int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;

    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);

    if (!bridge || argc < 1) {
        return JS_UNDEFINED;
    }

    // 获取函数名
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_UNDEFINED;

    // 获取参数（如果有）
    std::string args = "null";
    if (argc > 1) {
        args = jsValueToJson(ctx, argv[1]);
    }

    std::string result;
    try {
        // 调用宿主函数
        result = bridge->call(name, args);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[HostBridge::jsCall] exception in host callback '%s': %s\n", name, e.what());
        result = std::string("{\"error\":\"Host callback exception: ") + e.what() + "\"}";
    } catch (...) {
        std::fprintf(stderr, "[HostBridge::jsCall] unknown exception in host callback '%s'\n", name);
        result = R"({"error":"Host callback unknown exception"})";
    }
    JS_FreeCString(ctx, name);

    // 注意：不在这里调用 processQueue() / flushEvents()！同 jsPyCall 的原因。
    // 这两个函数已由 EventLoop update 回调定期处理，在 JS 调用栈中调用会造成重入。

    // 返回结果
    return jsonToJsValue(ctx, result);
}

JSValue HostBridge::jsPyCall(JSContext* ctx, JSValueConst thisVal,
                             int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;

    // func_data[0] = bridge 指针, func_data[1] = 函数名
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);

    const char* name = JS_ToCString(ctx, func_data[1]);
    if (!bridge || !name) {
        if (name) JS_FreeCString(ctx, name);
        return JS_UNDEFINED;
    }

    // 获取参数（如果有）
    std::string args = "null";
    if (argc > 0) {
        args = jsValueToJson(ctx, argv[0]);
    }

    std::string result;
    try {
        // 调用宿主函数
        result = bridge->call(name, args);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[HostBridge::jsPyCall] exception in host callback '%s': %s\n", name, e.what());
        result = std::string("{\"error\":\"Host callback exception: ") + e.what() + "\"}";
    } catch (...) {
        std::fprintf(stderr, "[HostBridge::jsPyCall] unknown exception in host callback '%s'\n", name);
        result = R"({"error":"Host callback unknown exception"})";
    }
    JS_FreeCString(ctx, name);

    // 注意：不在这里调用 processQueue() / flushEvents()！
    // 原因：jsPyCall 本身是在 QuickJS JS 调用栈中执行的（由 JS onClick 事件触发），
    // 在 JS 执行栈中再次调用 JS 回调（processQueue/flushEvents 可能触发 JS watcher/listener 回调）
    // 会造成 QuickJS 重入，可能导致迭代器失效和 use-after-free 崩溃。
    // processQueue() 和 flushEvents() 已由 EventLoop 的 update 回调定期处理，此处无需手动调用。

    // 返回结果
    return jsonToJsValue(ctx, result);
}

JSValue HostBridge::jsStateGet(JSContext* ctx, JSValueConst thisVal,
                               int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;
    
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);
    
    if (!bridge || argc < 1) {
        return JS_UNDEFINED;
    }
    
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_UNDEFINED;
    
    json value = bridge->stateManager_->getJson(name);
    JS_FreeCString(ctx, name);
    
    return jsonToJsValue(ctx, value.dump());
}

JSValue HostBridge::jsStateSet(JSContext* ctx, JSValueConst thisVal,
                               int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;
    
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);
    
    if (!bridge || argc < 2) {
        return JS_UNDEFINED;
    }
    
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_UNDEFINED;
    
    std::string jsonStr = jsValueToJson(ctx, argv[1]);
    
    try {
        json value = json::parse(jsonStr);
        bridge->stateManager_->setJson(name, value);
        // 立即处理队列以应用更改
        bridge->stateManager_->processQueue();
        // flush 事件队列（状态 watcher 可能产生了事件）
        bridge->flushEvents();
    } catch (...) {
        // 解析失败，忽略
    }
    
    JS_FreeCString(ctx, name);
    return JS_UNDEFINED;
}

JSValue HostBridge::jsStateWatch(JSContext* ctx, JSValueConst thisVal,
                                 int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;

    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);

    if (!bridge || argc < 2 || !bridge->stateManager_) {
        return JS_NewInt32(ctx, -1);
    }

    if (!JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "host.state.watch requires a function as callback");
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NewInt32(ctx, -1);

    std::string stateName(name);
    JS_FreeCString(ctx, name);

    // 使用 RAII 包装 JS 回调，确保在 unwatch/clearWatchers 时自动释放
    auto callback_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[1]);

    // 注册监听器
    int watchId = bridge->stateManager_->watch(stateName,
        [ctx, callback_wrapper](const std::string& /*name*/, const json& v) {
            JSValue arg = jsonToJsValue(ctx, v.dump());
            JSValue callback = callback_wrapper->Get();
            JSValue result = JS_Call(ctx, callback, JS_UNDEFINED, 1, &arg);

            JS_FreeValue(ctx, arg);
            JS_FreeValue(ctx, result);
        });

    return JS_NewInt32(ctx, watchId);
}

JSValue HostBridge::jsStateExists(JSContext* ctx, JSValueConst thisVal,
                                  int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;
    
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);
    
    if (!bridge || argc < 1) {
        return JS_FALSE;
    }
    
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_FALSE;
    
    bool exists = bridge->stateManager_->exists(name);
    JS_FreeCString(ctx, name);
    
    return exists ? JS_TRUE : JS_FALSE;
}

JSValue HostBridge::jsStateUnwatch(JSContext* ctx, JSValueConst thisVal,
                                   int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;
    
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);
    
    if (!bridge || argc < 1) {
        return JS_UNDEFINED;
    }
    
    int32_t watchId;
    if (JS_ToInt32(ctx, &watchId, argv[0]) < 0) {
        return JS_UNDEFINED;
    }
    
    bridge->stateManager_->unwatch(watchId);
    return JS_UNDEFINED;
}

JSValue HostBridge::jsStateType(JSContext* ctx, JSValueConst thisVal,
                                int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;
    
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);
    
    if (!bridge || argc < 1) {
        return JS_NewString(ctx, "null");
    }
    
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NewString(ctx, "null");
    
    LightUIType type = bridge->stateManager_->type(name);
    JS_FreeCString(ctx, name);
    
    const char* typeStr = "null";
    switch (type) {
        case LightUIType::Bool: typeStr = "boolean"; break;
        case LightUIType::Int:
        case LightUIType::Double: typeStr = "number"; break;
        case LightUIType::String: typeStr = "string"; break;
        case LightUIType::Array: typeStr = "array"; break;
        case LightUIType::Object: typeStr = "object"; break;
        default: break;
    }
    
    return JS_NewString(ctx, typeStr);
}

// ========== 事件系统实现 ==========

int HostBridge::on(const std::string& eventName, JSValue callback) {
    int id = nextListenerId_++;
    JS_DupValue(ctx_, callback);
    listeners_.push_back({id, eventName, callback});

    // 检查是否需要创建 auto-watcher
    if (stateManager_ && stateManager_->exists(eventName)) {
        auto it = autoWatchers_.find(eventName);
        if (it == autoWatchers_.end()) {
            // 首次监听该状态名，注册 StateManager watcher
            std::string name = eventName;
            int watcherId = stateManager_->watch(eventName,
                [this, name](const std::string& /*n*/, const json& value) {
                    emit(name, value.dump());
                });
            autoWatchers_[eventName] = {watcherId, 1};
        } else {
            it->second.listenerCount++;
        }
    }

    return id;
}

void HostBridge::off(int listenerId) {
    auto it = std::find_if(listeners_.begin(), listeners_.end(),
        [listenerId](const HostEventListener& l) { return l.id == listenerId; });
    
    if (it == listeners_.end()) return;

    std::string eventName = it->eventName;
    JS_FreeValue(ctx_, it->callback);
    listeners_.erase(it);

    // 检查是否需要移除 auto-watcher
    auto awIt = autoWatchers_.find(eventName);
    if (awIt != autoWatchers_.end()) {
        awIt->second.listenerCount--;
        if (awIt->second.listenerCount <= 0) {
            if (stateManager_) {
                stateManager_->unwatch(awIt->second.watcherId);
            }
            autoWatchers_.erase(awIt);
        }
    }
}

void HostBridge::emit(const std::string& eventName, const std::string& dataJson) {
    std::lock_guard<std::mutex> lock(eventQueueMutex_);
    eventQueue_.push_back({eventName, dataJson});
}

void HostBridge::flushEvents() {
    // 取出队列中所有事件
    std::vector<PendingEvent> events;
    {
        std::lock_guard<std::mutex> lock(eventQueueMutex_);
        std::swap(events, eventQueue_);
    }

    if (events.empty() || !ctx_) return;

    // 按入队顺序分发事件
    for (const auto& event : events) {
        JSValue data = jsonToJsValue(ctx_, event.dataJson);

        for (const auto& listener : listeners_) {
            if (listener.eventName == event.eventName) {
                JSValue result = JS_Call(ctx_, listener.callback, JS_UNDEFINED, 1, &data);
                if (JS_IsException(result)) {
                    // 捕获异常，输出日志，继续执行后续 Listener
                    JSValue exception = JS_GetException(ctx_);
                    const char* msg = JS_ToCString(ctx_, exception);
                    if (msg) {
                        fprintf(stderr, "[HostBridge] Event callback error (%s): %s\n",
                                event.eventName.c_str(), msg);
                        JS_FreeCString(ctx_, msg);
                    }
                    JS_FreeValue(ctx_, exception);
                }
                JS_FreeValue(ctx_, result);
            }
        }

        JS_FreeValue(ctx_, data);
    }
}

JSValue HostBridge::jsHostOn(JSContext* ctx, JSValueConst thisVal,
                             int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;

    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);

    if (!bridge || argc < 2) {
        return JS_NewInt32(ctx, -1);
    }

    // 参数校验：eventName 必须是字符串
    if (!JS_IsString(argv[0])) {
        return JS_NewInt32(ctx, -1);
    }

    // 参数校验：callback 必须是函数
    if (!JS_IsFunction(ctx, argv[1])) {
        return JS_NewInt32(ctx, -1);
    }

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_NewInt32(ctx, -1);

    int id = bridge->on(name, argv[1]);
    JS_FreeCString(ctx, name);

    return JS_NewInt32(ctx, id);
}

JSValue HostBridge::jsHostOff(JSContext* ctx, JSValueConst thisVal,
                              int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    (void)thisVal;
    (void)magic;

    int64_t ptr;
    JS_ToInt64(ctx, &ptr, func_data[0]);
    auto* bridge = reinterpret_cast<HostBridge*>(ptr);

    if (!bridge || argc < 1) {
        return JS_UNDEFINED;
    }

    int32_t listenerId;
    if (JS_ToInt32(ctx, &listenerId, argv[0]) < 0) {
        return JS_UNDEFINED;
    }

    bridge->off(listenerId);
    return JS_UNDEFINED;
}

// ========== 辅助函数 ==========

std::string HostBridge::jsValueToJson(JSContext* ctx, JSValueConst val) {
    JSValue jsonStr = JS_JSONStringify(ctx, val, JS_UNDEFINED, JS_UNDEFINED);
    if (JS_IsException(jsonStr)) {
        return "null";
    }
    
    const char* str = JS_ToCString(ctx, jsonStr);
    std::string result = str ? str : "null";
    
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, jsonStr);
    
    return result;
}

JSValue HostBridge::jsonToJsValue(JSContext* ctx, const std::string& jsonStr) {
    return JS_ParseJSON(ctx, jsonStr.c_str(), jsonStr.size(), "<json>");
}

} // namespace lightui
