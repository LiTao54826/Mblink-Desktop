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
#include "core/compositor/compositor_layer.h"
#include "core/render/image/image_cache.h"
#include <iostream>
#include <SDL3/SDL.h>
#ifdef _WIN32
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

namespace mbink {

namespace {
json CollectProcessMemoryStats() {
    json result = json::object();
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
        result["WorkingSetSize"] = static_cast<uint64_t>(counters.WorkingSetSize);
        result["PrivateUsage"] = static_cast<uint64_t>(counters.PrivateUsage);
        result["PagefileUsage"] = static_cast<uint64_t>(counters.PagefileUsage);
        result["PeakWorkingSetSize"] = static_cast<uint64_t>(counters.PeakWorkingSetSize);
    }
#endif
    return result;
}

int64_t JsonPathInt64(const json& value, std::initializer_list<const char*> path) {
    const json* current = &value;
    for (const char* key : path) {
        if (!current->is_object()) {
            return 0;
        }
        auto it = current->find(key);
        if (it == current->end()) {
            return 0;
        }
        current = &(*it);
    }
    if (current->is_number_unsigned()) {
        return static_cast<int64_t>(current->get<uint64_t>());
    }
    if (current->is_number_integer()) {
        return current->get<int64_t>();
    }
    if (current->is_number_float()) {
        return static_cast<int64_t>(current->get<double>());
    }
    return 0;
}

json BuildNativeLeakDelta(const json& current, const json& previous) {
    if (!previous.is_object()) {
        return json::object();
    }

    return {
        {"CompositorLayer.bitmap_count", JsonPathInt64(current, {"CompositorLayer.bitmap_count"}) - JsonPathInt64(previous, {"CompositorLayer.bitmap_count"})},
        {"CompositorLayer.bitmap_bytes", JsonPathInt64(current, {"CompositorLayer.bitmap_bytes"}) - JsonPathInt64(previous, {"CompositorLayer.bitmap_bytes"})},
        {"CompositorLayer.texture_count", JsonPathInt64(current, {"CompositorLayer.texture_count"}) - JsonPathInt64(previous, {"CompositorLayer.texture_count"})},
        {"CompositorLayer.texture_bytes", JsonPathInt64(current, {"CompositorLayer.texture_bytes"}) - JsonPathInt64(previous, {"CompositorLayer.texture_bytes"})},
        {"ImageCache.count", JsonPathInt64(current, {"ImageCache.count"}) - JsonPathInt64(previous, {"ImageCache.count"})},
        {"ImageCache.bytes", JsonPathInt64(current, {"ImageCache.bytes"}) - JsonPathInt64(previous, {"ImageCache.bytes"})},
        {"Window.surface_estimated_bytes", JsonPathInt64(current, {"Window.surface_estimated_bytes"}) - JsonPathInt64(previous, {"Window.surface_estimated_bytes"})},
        {"Window.fbo_estimated_texture_bytes", JsonPathInt64(current, {"Window.fbo_estimated_texture_bytes"}) - JsonPathInt64(previous, {"Window.fbo_estimated_texture_bytes"})},
        {"Window.fbo_estimated_depth_stencil_bytes", JsonPathInt64(current, {"Window.fbo_estimated_depth_stencil_bytes"}) - JsonPathInt64(previous, {"Window.fbo_estimated_depth_stencil_bytes"})},
        {"Window.fbo_estimated_total_bytes", JsonPathInt64(current, {"Window.fbo_estimated_total_bytes"}) - JsonPathInt64(previous, {"Window.fbo_estimated_total_bytes"})},
        {"Skia.resource_cache_count", JsonPathInt64(current, {"Skia.resource_cache_count"}) - JsonPathInt64(previous, {"Skia.resource_cache_count"})},
        {"Skia.resource_cache_bytes", JsonPathInt64(current, {"Skia.resource_cache_bytes"}) - JsonPathInt64(previous, {"Skia.resource_cache_bytes"})},
        {"Skia.resource_cache_limit", JsonPathInt64(current, {"Skia.resource_cache_limit"}) - JsonPathInt64(previous, {"Skia.resource_cache_limit"})},
        {"ProcessMemory.WorkingSetSize", JsonPathInt64(current, {"ProcessMemory", "WorkingSetSize"}) - JsonPathInt64(previous, {"ProcessMemory", "WorkingSetSize"})},
        {"ProcessMemory.PrivateUsage", JsonPathInt64(current, {"ProcessMemory", "PrivateUsage"}) - JsonPathInt64(previous, {"ProcessMemory", "PrivateUsage"})},
        {"ProcessMemory.PagefileUsage", JsonPathInt64(current, {"ProcessMemory", "PagefileUsage"}) - JsonPathInt64(previous, {"ProcessMemory", "PagefileUsage"})}
    };
}

json g_last_native_leak_payload;
std::string g_last_native_leak_tag;
}


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

    runtime_->RegisterFunction("__mbinkDumpNativeLeakStats", [this](const json& args) -> json {
        std::string tag = "native";
        bool run_gc_first = true;
        bool should_purge_skia = false;
        if (args.is_array() && !args.empty()) {
            if (args[0].is_string()) {
                tag = args[0].get<std::string>();
            }
            if (args.size() > 1 && args[1].is_boolean()) {
                run_gc_first = args[1].get<bool>();
            }
            if (args.size() > 2 && args[2].is_boolean()) {
                should_purge_skia = args[2].get<bool>();
            } else if (tag.find(".settled") != std::string::npos) {
                should_purge_skia = true;
            }
        } else if (tag.find(".settled") != std::string::npos) {
            should_purge_skia = true;
        }

        (void)run_gc_first;

        auto& image_cache = ImageCache::GetInstance();
        int logical_width = 0;
        int logical_height = 0;
        int physical_width = 0;
        int physical_height = 0;
        if (window_) {
            window_->GetSize(&logical_width, &logical_height);
            window_->GetPhysicalSize(&physical_width, &physical_height);
        }

        const auto backend = window_ ? window_->GetActualBackend() : RenderBackend::AUTO;
        const char* backend_name = "AUTO";
        switch (backend) {
            case RenderBackend::OPENGL: backend_name = "OPENGL"; break;
            case RenderBackend::CPU: backend_name = "CPU"; break;
            case RenderBackend::SOFTWARE: backend_name = "SOFTWARE"; break;
            case RenderBackend::AUTO:
            default: backend_name = "AUTO"; break;
        }

        json payload = {
            {"tag", tag},
            {"CompositorLayer.bitmap_count", CompositorLayer::GetLiveBitmapCount()},
            {"CompositorLayer.bitmap_bytes", CompositorLayer::GetLiveBitmapBytes()},
            {"CompositorLayer.texture_count", CompositorLayer::GetLiveTextureCount()},
            {"CompositorLayer.texture_bytes", CompositorLayer::GetLiveTextureBytes()},
            {"ImageCache.count", image_cache.GetCacheCount()},
            {"ImageCache.bytes", image_cache.GetCurrentCacheSize()},
            {"Window.backend", backend_name},
            {"Window.surface_present", window_ ? window_->HasSurface() : false},
            {"Window.gr_context_present", window_ ? window_->HasGrContext() : false},
            {"Window.fbo_present", window_ ? window_->HasFBOManager() : false},
            {"Window.logical_width", logical_width},
            {"Window.logical_height", logical_height},
            {"Window.physical_width", physical_width},
            {"Window.physical_height", physical_height},
            {"Window.dpi_scale", window_ ? window_->GetDisplayScale() : 1.0f},
            {"Window.surface_estimated_bytes", window_ ? window_->GetEstimatedSurfaceBytes() : 0},
            {"Window.fbo_estimated_texture_bytes", window_ ? window_->GetEstimatedFBOTextureBytes() : 0},
            {"Window.fbo_estimated_depth_stencil_bytes", window_ ? window_->GetEstimatedFBODepthStencilBytes() : 0},
            {"Window.fbo_estimated_total_bytes", window_ ? window_->GetEstimatedFBOTotalBytes() : 0},
            {"Skia.resource_cache_count", window_ ? window_->GetSkiaResourceCacheCount() : 0},
            {"Skia.resource_cache_bytes", window_ ? window_->GetSkiaResourceCacheBytes() : 0},
            {"Skia.resource_cache_limit", window_ ? window_->GetSkiaResourceCacheLimit() : 0},
            {"Skia.purge_executed", false},
            {"ProcessMemory", CollectProcessMemoryStats()}
        };

        if (window_ && should_purge_skia) {
            window_->PurgeSkiaResourceCache();
            payload["Skia.purge_executed"] = true;
            payload["Skia.resource_cache_count_after_purge"] = window_->GetSkiaResourceCacheCount();
            payload["Skia.resource_cache_bytes_after_purge"] = window_->GetSkiaResourceCacheBytes();
            payload["ProcessMemoryAfterPurge"] = CollectProcessMemoryStats();
        }

        payload["prev_tag"] = g_last_native_leak_tag;
        payload["delta_from_prev"] = BuildNativeLeakDelta(payload, g_last_native_leak_payload);
        g_last_native_leak_tag = tag;
        g_last_native_leak_payload = payload;

        std::cout << "[NativeLeakStats][" << tag << "] " << payload.dump() << std::endl;
        return payload;
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

