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
#include "core/dom/dom_bindings.h"
#include "core/dom/canvas_bindings.h"
#include <iostream>

namespace lightui {

// ========== WindowBindings 实现 ==========

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
    
    // 然后绑定 Document API（会向 window 对象添加 getSelection 等方法）
    BindDocumentAPIs(runtime_->GetContext(), window_.get());
}

void WindowBindings::BindWindowObject() {
    // 绑定 window.innerWidth
    runtime_->RegisterFunction("__getInnerWidth", [this](const json& args) -> json {
        int width, height;
        window_->GetSize(&width, &height);
        return width;
    });

    // 绑定 window.innerHeight
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
    )";
    
    runtime_->Eval(window_code, "<window_bindings>");
}

void WindowBindings::BindDocumentObject() {
    // 绑定 document.body
    runtime_->RegisterFunction("__getDocumentBody", [this](const json& args) -> json {
        auto doc = window_->GetDocument();
        if (!doc) {
            return nullptr;
        }
        
        auto body = doc->GetBody();
        if (!body) {
            return nullptr;
        }
        
        // 返回一个简单的对象表示
        json result;
        result["tagName"] = body->GetTagName();
        result["id"] = body->GetAttribute("id");
        return result;
    });
    
    // 绑定 document.documentElement
    runtime_->RegisterFunction("__getDocumentElement", [this](const json& args) -> json {
        auto doc = window_->GetDocument();
        if (!doc) {
            return nullptr;
        }
        
        auto root = doc->GetDocumentElement();
        if (!root) {
            return nullptr;
        }
        
        json result;
        result["tagName"] = root->GetTagName();
        result["id"] = root->GetAttribute("id");
        return result;
    });
    
    // 绑定 document.getElementById
    runtime_->RegisterFunction("__getElementById", [this](const json& args) -> json {
        auto doc = window_->GetDocument();
        // args 是数组，第一个元素是 id 字符串
        if (!doc || !args.is_array() || args.empty() || !args[0].is_string()) {
            return nullptr;
        }

        auto element = doc->GetElementById(args[0].get<std::string>());
        if (!element) {
            return nullptr;
        }

        json result;
        result["tagName"] = element->GetTagName();
        result["id"] = element->GetAttribute("id");
        result["textContent"] = element->GetTextContent();
        return result;
    });

    // 绑定 element.textContent setter
    runtime_->RegisterFunction("__setTextContent", [this](const json& args) -> json {
        auto doc = window_->GetDocument();
        // args[0] = element id, args[1] = new text content
        if (!doc || !args.is_array() || args.size() < 2 ||
            !args[0].is_string() || !args[1].is_string()) {
            return false;
        }

        std::string id = args[0].get<std::string>();
        std::string content = args[1].get<std::string>();

        auto element = doc->GetElementById(id);
        if (!element) {
            return false;
        }

        element->SetTextContent(content);
        return true;
    });

    // 绑定 document.createElement
    runtime_->RegisterFunction("__createElement", [this](const json& args) -> json {
        auto doc = window_->GetDocument();
        // args 是数组，第一个元素是标签名字符串
        if (!doc || !args.is_array() || args.empty() || !args[0].is_string()) {
            return nullptr;
        }

        auto element = doc->CreateElement(args[0].get<std::string>());
        if (!element) {
            return nullptr;
        }

        json result;
        result["tagName"] = element->GetTagName();
        result["id"] = element->GetAttribute("id");
        return result;
    });
    
    // 创建 document 对象
    std::string document_code = R"(
        globalThis.document = {
            get body() { return __getDocumentBody(); },
            get documentElement() { return __getDocumentElement(); },
            getElementById: function(id) {
                const elem = __getElementById(id);
                if (!elem) return null;

                // 创建一个代理对象，支持 textContent setter
                return {
                    tagName: elem.tagName,
                    id: elem.id,
                    get textContent() {
                        return elem.textContent;
                    },
                    set textContent(value) {
                        __setTextContent(elem.id, value);
                    }
                };
            },
            createElement: function(tagName) { return __createElement(tagName); }
        };
    )";

    runtime_->Eval(document_code, "<document_bindings>");
}

void WindowBindings::BindTimers() {
    if (!task_scheduler_) {
        std::cerr << "Warning: TaskScheduler is null, timers will not work" << std::endl;
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
                std::cerr << "Error in setTimeout callback: " << e.what() << std::endl;
            }
        }, delay_ms);

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
                std::cerr << "Error in setInterval callback: " << e.what() << std::endl;
            }
        }, interval_ms);
        
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
                std::cerr << "Error in requestAnimationFrame callback: " << e.what() << std::endl;
            }
        });

        return task_id;
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
    )";
    
    runtime_->Eval(timer_code, "<timer_bindings>");
}

void WindowBindings::BindEventListeners() {
    // 绑定 window.addEventListener
    runtime_->RegisterFunction("__windowAddEventListener", [this](const json& args) -> json {
        if (!args.is_array() || args.size() < 2) {
            return false;
        }
        
        std::string event_type = args[0].get<std::string>();
        std::string callback_name = args[1].get<std::string>();
        
        // TODO: 实现事件监听器注册
        std::cout << "addEventListener: " << event_type << std::endl;
        
        return true;
    });
    
    // 创建事件监听器函数
    std::string event_code = R"(
        globalThis.window.addEventListener = function(type, listener) {
            const callbackName = '__event_' + Math.random().toString(36).substr(2, 9);
            globalThis[callbackName] = listener;
            return __windowAddEventListener([type, callbackName]);
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

} // namespace lightui

