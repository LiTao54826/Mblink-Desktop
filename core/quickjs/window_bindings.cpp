/**
 * @file window_bindings.cpp
 * @brief Window 对象的 JavaScript 绑定实现
 */

#include "window_bindings.h"
#include "bindings/js_node.h"
#include "bindings/js_element.h"
#include "bindings/js_style_declaration.h"
#include "bindings/js_event.h"
#include "bindings/js_data_transfer.h"
#include "bindings/js_range.h"
#include "bindings/js_selection.h"
#include "bindings/js_mutation_observer.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/dom/bindings/canvas_bindings.h"
#include <iostream>
#include <SDL3/SDL.h>

namespace mbink {

// ========== WindowBindings 实现 ==========

void WindowBindings::TrackTimerCallback(int task_id, const std::string& callback_name) {
    if (task_id >= 0) {
        timer_callbacks_[task_id] = callback_name;
    }
}

void WindowBindings::ReleaseTimerCallback(int task_id) {
    auto it = timer_callbacks_.find(task_id);
    if (it == timer_callbacks_.end()) {
        return;
    }

    ReleaseTimerCallbackByName(it->second);
    timer_callbacks_.erase(it);
}

void WindowBindings::ReleaseTimerCallbackByName(const std::string& callback_name) {
    JSContext* ctx = runtime_ ? runtime_->GetContext() : nullptr;
    if (!ctx || callback_name.empty()) {
        return;
    }

    for (auto it = timer_callbacks_.begin(); it != timer_callbacks_.end(); ) {
        if (it->second == callback_name) {
            it = timer_callbacks_.erase(it);
        } else {
            ++it;
        }
    }

    JSValue global = JS_GetGlobalObject(ctx);
    JSAtom callback_atom = JS_NewAtom(ctx, callback_name.c_str());
    JS_DeleteProperty(ctx, global, callback_atom, 0);
    JS_FreeAtom(ctx, callback_atom);
    JS_FreeValue(ctx, global);
}

WindowBindings::WindowBindings(QuickJSRuntime* runtime, 
                               std::shared_ptr<Window> window,
                               std::shared_ptr<TaskScheduler> task_scheduler)
    : runtime_(runtime)
    , window_(window)
    , task_scheduler_(task_scheduler) {
}

void WindowBindings::InitBindings() {
    // 初始化新的模块化 DOM 绑定系统（有 exotic 支持）
    bindings::InitNodeBinding(runtime_->GetContext());
    bindings::InitElementBinding(runtime_->GetContext());
    bindings::InitStyleDeclarationBinding(runtime_->GetContext());  // 支持 element.style.xxx = '...'
    bindings::InitEventBinding(runtime_->GetContext());
    bindings::InitDataTransferBinding(runtime_->GetContext());  // 支持 DragEvent.dataTransfer
    bindings::InitRangeBinding(runtime_->GetContext());  // 支持 Range API
    bindings::InitSelectionBinding(runtime_->GetContext());  // 支持 Selection API
    bindings::InitMutationObserverBinding(runtime_->GetContext());  // 支持 MutationObserver API
    
    // 初始化 Canvas 绑定（独立模块）
    CanvasBindings::Init(runtime_->GetContext());
    
    // 初始化 Image 构造函数（支持 new Image()）
    InitImageConstructor(runtime_->GetContext());
    
    // 设置全局 TaskScheduler（定时器需要）
    if (task_scheduler_) {
        DOMBindings::SetGlobalTaskScheduler(runtime_->GetContext(), task_scheduler_);
    }
    
    // 先绑定 window 对象（创建空的 window 对象）
    BindWindowObject();
    BindTimers();
    BindEventListeners();
    
    // 然后绑定 Document API（会创建完整的 document 对象）
    BindDocumentAPIs(runtime_->GetContext(), window_.get());
}

void WindowBindings::BindWindowObject() {
    // 绑定 window.innerWidth - 返回布局宽度（与 getBoundingClientRect 坐标系一致）
    runtime_->RegisterFunction("__getInnerWidth", [this](const json& args) -> json {
        int width, height;
        window_->GetSize(&width, &height);
        // 注意：直接返回物理像素宽度，与布局引擎 getBoundingClientRect 坐标系保持一致
        // Chrome headless viewport 也以相同像素数设置，保证对比基准一致
        return width;
    });

    // 绑定 window.innerHeight - 返回布局高度（与 getBoundingClientRect 坐标系一致）
    runtime_->RegisterFunction("__getInnerHeight", [this](const json& args) -> json {
        int width, height;
        window_->GetSize(&width, &height);
        return height;
    });

    // 绑定 window.devicePixelRatio
    runtime_->RegisterFunction("__getDevicePixelRatio", [this](const json& args) -> json {
        return static_cast<double>(window_->GetDisplayScale());
    });

    // 绑定 window.title
    runtime_->RegisterFunction("__getTitle", [this](const json& args) -> json {
        return window_->GetTitle();
    });

    runtime_->RegisterFunction("__setTitle", [this](const json& args) -> json {
        // args 是一个数组，第一个元素是标题字符串
        if (args.is_array() && !args.empty() && args[0].is_string()) {
            std::string title = args[0].get<std::string>();
            window_->SetTitle(title);
            return title;
        }
        return nullptr;
    });

    // 窗口控制函数：最小化、最大化、还原、关闭
    runtime_->RegisterFunction("__windowMinimize", [this](const json& args) -> json {
        window_->Minimize();
        return true;
    });

    runtime_->RegisterFunction("__windowMaximize", [this](const json& args) -> json {
        window_->Maximize();
        return true;
    });

    runtime_->RegisterFunction("__windowRestore", [this](const json& args) -> json {
        window_->Restore();
        return true;
    });

    runtime_->RegisterFunction("__windowClose", [this](const json& args) -> json {
        // 推送 SDL_QUIT 事件，让事件循环正常退出
        SDL_Event quit_event;
        quit_event.type = SDL_EVENT_QUIT;
        quit_event.quit.timestamp = SDL_GetTicksNS();
        SDL_PushEvent(&quit_event);
        return true;
    });

    // 创建 window 对象（如果不存在则创建，否则扩展现有对象）
    std::string window_code = R"(
        if (!globalThis.window) {
            globalThis.window = {};
        }
        Object.defineProperty(globalThis.window, 'innerWidth', {
            get: function() { return __getInnerWidth(); },
            configurable: true
        });
        Object.defineProperty(globalThis.window, 'innerHeight', {
            get: function() { return __getInnerHeight(); },
            configurable: true
        });
        Object.defineProperty(globalThis.window, 'devicePixelRatio', {
            get: function() { return __getDevicePixelRatio(); },
            configurable: true
        });
        Object.defineProperty(globalThis.window, 'title', {
            get: function() { return __getTitle(); },
            set: function(value) { __setTitle(value); },
            configurable: true
        });

        // 窗口控制方法
        globalThis.window.minimize = function() { return __windowMinimize(); };
        globalThis.window.maximize = function() { return __windowMaximize(); };
        globalThis.window.restore = function() { return __windowRestore(); };
        globalThis.window.close = function() { return __windowClose(); };

        // 创建 navigator 对象（用于平台/浏览器检测）
        if (!globalThis.navigator) {
            globalThis.navigator = {
                platform: 'Win32',
                userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
                vendor: 'Google Inc.',
                language: 'zh-CN',
                languages: ['zh-CN', 'en'],
                onLine: true,
                cookieEnabled: false,
                clipboard: undefined,
                maxTouchPoints: 0
            };
        }
        // 也设置到 window 上
        globalThis.window.navigator = globalThis.navigator;
    )";
    
    runtime_->Eval(window_code, "<window_bindings>");
}

void WindowBindings::BindTimers() {
    if (!task_scheduler_) {
        return;
    }
    
    // 绑定 setTimeout
    runtime_->RegisterFunction("__setTimeout", [this](const json& args) -> json {
        if (!args.is_array() || args.size() < 2) {
            return -1;
        }

        // args[0] 是回调函数名（字符串）
        // args[1] 是延迟时间（毫秒）
        std::string callback_name = args[0].get<std::string>();
        int delay_ms = args[1].get<int>();

        // 创建任务
        auto task_id = task_scheduler_->SetTimeout([this, callback_name]() {
            try {
                runtime_->CallFunction(callback_name, json::array());
            } catch (const std::exception& e) {
            }

            ReleaseTimerCallbackByName(callback_name);
        }, delay_ms);

        TrackTimerCallback(task_id, callback_name);

        return task_id;
    });
    
    // 绑定 setInterval
    runtime_->RegisterFunction("__setInterval", [this](const json& args) -> json {
        if (!args.is_array() || args.size() < 2) {
            return -1;
        }
        
        std::string callback_name = args[0].get<std::string>();
        int interval_ms = args[1].get<int>();
        
        auto task_id = task_scheduler_->SetInterval([this, callback_name]() {
            try {
                runtime_->CallFunction(callback_name, json::array());
            } catch (const std::exception& e) {
            }
        }, interval_ms);

        TrackTimerCallback(task_id, callback_name);
        
        return task_id;
    });
    
    // 绑定 clearTimeout
    runtime_->RegisterFunction("__clearTimeout", [this](const json& args) -> json {
        // args 是数组，第一个元素是 task_id
        if (!args.is_array() || args.empty() || !args[0].is_number_integer()) {
            return false;
        }

        int task_id = args[0].get<int>();
        task_scheduler_->ClearTimeout(task_id);
        ReleaseTimerCallback(task_id);
        return true;
    });

    // 绑定 clearInterval
    runtime_->RegisterFunction("__clearInterval", [this](const json& args) -> json {
        // args 是数组，第一个元素是 task_id
        if (!args.is_array() || args.empty() || !args[0].is_number_integer()) {
            return false;
        }

        int task_id = args[0].get<int>();
        task_scheduler_->ClearInterval(task_id);
        ReleaseTimerCallback(task_id);
        return true;
    });
    
    // 绑定 requestAnimationFrame
    runtime_->RegisterFunction("__requestAnimationFrame", [this](const json& args) -> json {
        // args 是数组，第一个元素是回调函数名
        if (!args.is_array() || args.empty() || !args[0].is_string()) {
            return -1;
        }

        std::string callback_name = args[0].get<std::string>();

        auto task_id = task_scheduler_->RequestAnimationFrame([this, callback_name](double timestamp) {
            try {
                json callback_args = json::array();
                callback_args.push_back(timestamp);
                runtime_->CallFunction(callback_name, callback_args);
            } catch (const std::exception& e) {
            }

            ReleaseTimerCallbackByName(callback_name);
        });

        TrackTimerCallback(task_id, callback_name);

        return task_id;
    });

    runtime_->RegisterFunction("__cancelAnimationFrame", [this](const json& args) -> json {
        if (!args.is_array() || args.empty() || !args[0].is_number_integer()) {
            return false;
        }

        int task_id = args[0].get<int>();
        task_scheduler_->CancelAnimationFrame(task_id);
        ReleaseTimerCallback(task_id);
        return true;
    });
    
    // 创建定时器函数
    std::string timer_code = R"(
        globalThis.setTimeout = function(callback, delay) {
            // 将回调函数存储到全局对象
            const callbackName = '__callback_' + Math.random().toString(36).substr(2, 9);
            globalThis[callbackName] = callback;
            return __setTimeout(callbackName, delay || 0);
        };

        globalThis.setInterval = function(callback, interval) {
            const callbackName = '__callback_' + Math.random().toString(36).substr(2, 9);
            globalThis[callbackName] = callback;
            return __setInterval(callbackName, interval || 0);
        };

        globalThis.clearTimeout = function(id) {
            return __clearTimeout(id);
        };

        globalThis.clearInterval = function(id) {
            return __clearInterval(id);
        };

        globalThis.requestAnimationFrame = function(callback) {
            const callbackName = '__callback_' + Math.random().toString(36).substr(2, 9);
            globalThis[callbackName] = callback;
            return __requestAnimationFrame(callbackName);
        };

        globalThis.cancelAnimationFrame = function(id) {
            return __cancelAnimationFrame(id);
        };
    )";
    
    runtime_->Eval(timer_code, "<timer_bindings>");
}

void WindowBindings::BindEventListeners() {
    // 绑定 window.addEventListener
    runtime_->RegisterFunction("__windowAddEventListener", [this](const json& args) -> json {
        if (!args.is_array() || args.empty() || !args[0].is_string()) {
            return false;
        }

        // TODO: 实现事件监听器注册

        return true;
    });

    runtime_->RegisterFunction("__windowRemoveEventListener", [this](const json& args) -> json {
        if (!args.is_array() || args.empty() || !args[0].is_string()) {
            return false;
        }

        // TODO: 实现事件监听器移除
        return true;
    });
    
    // 创建事件监听器函数
    std::string event_code = R"(
        globalThis.window.addEventListener = function(type, listener) {
            if (typeof listener !== 'function') {
                return false;
            }
            return __windowAddEventListener([type]);
        };

        globalThis.window.removeEventListener = function(type, listener) {
            if (typeof listener !== 'function') {
                return false;
            }
            return __windowRemoveEventListener([type]);
        };
    )";
    
    runtime_->Eval(event_code, "<event_bindings>");
}

// ========== DocumentBindings 实现 ==========

DocumentBindings::DocumentBindings(QuickJSRuntime* runtime, std::shared_ptr<Document> document)
    : runtime_(runtime)
    , document_(document) {
}

void DocumentBindings::InitBindings() {
    BindQueryMethods();
    BindCreateMethods();
    BindProperties();
}

void DocumentBindings::BindQueryMethods() {
    // 已在 WindowBindings::BindDocumentObject 中实现
}

void DocumentBindings::BindCreateMethods() {
    // 已在 WindowBindings::BindDocumentObject 中实现
}

void DocumentBindings::BindProperties() {
    // 已在 WindowBindings::BindDocumentObject 中实现
}

} // namespace mbink

