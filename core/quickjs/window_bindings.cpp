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
#include "bindings/js_file_list.h"
#include "bindings/js_range.h"
#include "bindings/js_selection.h"
#include "bindings/js_mutation_observer.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/dom/bindings/canvas_bindings.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/compositor/compositor_layer.h"
#include "core/compositor/layer_tree_manager.h"
#include "core/compositor/scroll_layer_manager.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/image/image_cache.h"
#include "core/event/loop/event_loop.h"
#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#ifdef _WIN32
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

namespace mblink {

namespace {
EventLoop* g_active_event_loop = nullptr;

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

json BuildScrollInvalidationStatsPayload(const ScrollInvalidationStats& stats) {
    return {
        {"scrolls_handled", stats.scrolls_handled},
        {"full_dirty_scrolls", stats.full_dirty_scrolls},
        {"clip_layer_full_dirty_scrolls", stats.clip_layer_full_dirty_scrolls},
        {"ancestor_layer_full_dirty_scrolls", stats.ancestor_layer_full_dirty_scrolls},
        {"missing_layer_target_scrolls", stats.missing_layer_target_scrolls},
        {"incremental_eligible_fallbacks", IncrementalEligibleScrollFallbacks(stats)},
        {"conservative_fallbacks", ConservativeScrollFallbacks(stats)}
    };
}

json BuildScrollContainerMemoryStatsPayload(const std::vector<ScrollContainerMemoryStats>& stats) {
    json containers = json::array();
    uint64_t total_bitmap_bytes = 0;
    uint64_t total_texture_bytes = 0;
    uint64_t total_clip_bitmap_bytes = 0;
    uint64_t total_content_bitmap_bytes = 0;
    uint64_t total_descendant_bitmap_bytes = 0;

    for (const auto& entry : stats) {
        total_bitmap_bytes += entry.total_bitmap_bytes;
        total_texture_bytes += entry.total_texture_bytes;
        total_clip_bitmap_bytes += entry.clip_layer_bitmap_bytes;
        total_content_bitmap_bytes += entry.content_layer_bitmap_bytes;
        total_descendant_bitmap_bytes += entry.descendant_bitmap_bytes;

        containers.push_back({
            {"container_id", entry.container_id},
            {"clip_layer_id", entry.clip_layer_id},
            {"content_layer_id", entry.content_layer_id},
            {"content_width", entry.content_width},
            {"content_height", entry.content_height},
            {"viewport_width", entry.viewport_width},
            {"viewport_height", entry.viewport_height},
            {"clip_layer_bitmap_bytes", entry.clip_layer_bitmap_bytes},
            {"content_layer_bitmap_bytes", entry.content_layer_bitmap_bytes},
            {"descendant_bitmap_bytes", entry.descendant_bitmap_bytes},
            {"total_bitmap_bytes", entry.total_bitmap_bytes},
            {"clip_layer_texture_bytes", entry.clip_layer_texture_bytes},
            {"content_layer_texture_bytes", entry.content_layer_texture_bytes},
            {"descendant_texture_bytes", entry.descendant_texture_bytes},
            {"total_texture_bytes", entry.total_texture_bytes},
            {"content_layer_allows_bitmap_backing", entry.content_layer_allows_bitmap_backing}
        });
    }

    return {
        {"container_count", stats.size()},
        {"total_bitmap_bytes", total_bitmap_bytes},
        {"total_texture_bytes", total_texture_bytes},
        {"total_clip_layer_bitmap_bytes", total_clip_bitmap_bytes},
        {"total_content_layer_bitmap_bytes", total_content_bitmap_bytes},
        {"total_descendant_bitmap_bytes", total_descendant_bitmap_bytes},
        {"containers", containers}
    };
}

json g_last_native_leak_payload;
std::string g_last_native_leak_tag;

void ClearKnownDOMWrapperBackrefs(JSContext* ctx, JSValueConst value) {
    if (!ctx || JS_IsUndefined(value) || JS_IsNull(value)) {
        return;
    }

    JS_SetPropertyStr(ctx, value, "_children", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, value, "_listeners", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, value, "__preactRoot", JS_UNDEFINED);
}
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

    const std::string callback_name = it->second;
    ReleaseTimerCallbackByName(callback_name);
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

void WindowBindings::SetActiveEventLoop(EventLoop* event_loop) {
    g_active_event_loop = event_loop;
}

EventLoop* WindowBindings::GetActiveEventLoop() {
    return g_active_event_loop;
}

void WindowBindings::InitBindings() {
    // 初始化新的模块化 DOM 绑定系统（有 exotic 支持）
    bindings::InitFileListBinding(runtime_->GetContext());
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

    // 先绑定 window 对象（创建空的 window 对象）
    BindWindowObject();
    BindTimers();
    BindEventListeners();

    // 然后绑定 Document API（会创建完整的 document 对象）
    BindDocumentAPIs(runtime_->GetContext(), window_.get());
}

void WindowBindings::Cleanup() {
    JSContext* ctx = runtime_ ? runtime_->GetContext() : nullptr;

    if (task_scheduler_) {
        task_scheduler_->ClearAllTasks();
    }

    if (!ctx) {
        if (g_active_event_loop) {
            g_active_event_loop = nullptr;
        }
        return;
    }

    std::vector<std::string> callback_names;
    callback_names.reserve(timer_callbacks_.size());
    for (const auto& [_, callback_name] : timer_callbacks_) {
        callback_names.push_back(callback_name);
    }
    for (const auto& callback_name : callback_names) {
        ReleaseTimerCallbackByName(callback_name);
    }
    timer_callbacks_.clear();

    auto& dom_binding_map = DOMBindingMap::GetInstance();
    dom_binding_map.ForEach([ctx](Node* node, JSContext* entry_ctx, JSValueConst value) {
        auto* element = dynamic_cast<Element*>(node);
        if (!element) {
            return;
        }

        if (entry_ctx && !JS_IsUndefined(value) && !JS_IsNull(value) &&
            JS_GetOpaque(value, bindings::GetElementClassID())) {
            bindings::ClearElementListenerBindings(entry_ctx, value);
        }
        ClearKnownDOMWrapperBackrefs(entry_ctx, value);

        element->ClearAllEventListeners();
    });

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "addEventListener", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "removeEventListener", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "dispatchEvent", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "setTimeout", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "setInterval", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "requestAnimationFrame", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "clearTimeout", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "clearInterval", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "cancelAnimationFrame", JS_UNDEFINED);

    JS_SetPropertyStr(ctx, global, "document", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, global, "__mblink_window_ptr", JS_UNDEFINED);

    JSValue window_obj = JS_GetPropertyStr(ctx, global, "window");
    if (!JS_IsUndefined(window_obj) && !JS_IsNull(window_obj)) {
        JS_SetPropertyStr(ctx, window_obj, "document", JS_UNDEFINED);
        JS_SetPropertyStr(ctx, window_obj, "addEventListener", JS_UNDEFINED);
        JS_SetPropertyStr(ctx, window_obj, "removeEventListener", JS_UNDEFINED);
        JS_SetPropertyStr(ctx, window_obj, "dispatchEvent", JS_UNDEFINED);
        JS_FreeValue(ctx, window_obj);
    }

    JS_FreeValue(ctx, global);

    dom_binding_map.Clear();
    g_active_event_loop = nullptr;
    JS_RunGC(JS_GetRuntime(ctx));
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

    runtime_->RegisterFunction("__mblinkDumpNativeLeakStats", [this](const json& args) -> json {
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

        json scroll_memory = {
            {"container_count", 0},
            {"total_bitmap_bytes", 0},
            {"total_texture_bytes", 0},
            {"total_clip_layer_bitmap_bytes", 0},
            {"total_content_layer_bitmap_bytes", 0},
            {"total_descendant_bitmap_bytes", 0},
            {"containers", json::array()}
        };
        json scroll_invalidation = json::object();
        if (window_) {
            if (auto* pipeline = window_->GetRenderPipeline()) {
                if (auto* scroll_manager = pipeline->GetScrollManager()) {
                    scroll_memory = BuildScrollContainerMemoryStatsPayload(
                        scroll_manager->CollectMemoryStats());
                    scroll_invalidation["scroll_manager"] =
                        BuildScrollInvalidationStatsPayload(scroll_manager->GetInvalidationStats());
                }
                if (auto* layer_tree_manager = pipeline->GetLayerTreeManager()) {
                    scroll_invalidation["layer_tree_manager"] =
                        BuildScrollInvalidationStatsPayload(layer_tree_manager->GetInvalidationStats());
                }
                const auto& frame_stats = pipeline->GetLastFrameStats();
                scroll_invalidation["last_frame"] = {
                    {"scrolls_handled", frame_stats.scrolls_handled},
                    {"full_dirty_fallbacks", frame_stats.scroll_full_dirty_fallbacks},
                    {"clip_layer_full_dirty_fallbacks", frame_stats.scroll_clip_layer_full_dirty_fallbacks},
                    {"ancestor_layer_full_dirty_fallbacks", frame_stats.scroll_ancestor_layer_full_dirty_fallbacks},
                    {"missing_layer_target_fallbacks", frame_stats.scroll_missing_layer_target_fallbacks},
                    {"incremental_eligible_fallbacks", frame_stats.scroll_incremental_eligible_fallbacks},
                    {"conservative_fallbacks", frame_stats.scroll_conservative_fallbacks},
                    {"retained_present_blocking_fallbacks", frame_stats.scroll_retained_present_blocking_fallbacks}
                };
            }
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
            {"ScrollContainers", scroll_memory},
            {"ScrollInvalidation", scroll_invalidation},
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


    // 创建 window 对象（与浏览器行为保持一致：window/self 指向 globalThis）
    std::string window_code = R"(
        if (typeof globalThis.globalThis === 'undefined') {
            globalThis.globalThis = globalThis;
        }
        const __mblinkExistingWindow = globalThis.window;
        if (!__mblinkExistingWindow || __mblinkExistingWindow !== globalThis) {
            if (__mblinkExistingWindow && typeof __mblinkExistingWindow === 'object') {
                try {
                    Object.assign(globalThis, __mblinkExistingWindow);
                } catch (_) {}
            }
            globalThis.window = globalThis;
        }
        if (typeof globalThis.self === 'undefined' || globalThis.self !== globalThis) {
            globalThis.self = globalThis;
        }

        Object.defineProperty(globalThis, 'innerWidth', {
            get: function() { return __getInnerWidth(); },
            configurable: true
        });
        Object.defineProperty(globalThis, 'innerHeight', {
            get: function() { return __getInnerHeight(); },
            configurable: true
        });
        Object.defineProperty(globalThis, 'devicePixelRatio', {
            get: function() { return __getDevicePixelRatio(); },
            configurable: true
        });
        Object.defineProperty(globalThis, 'title', {
            get: function() { return __getTitle(); },
            set: function(value) { __setTitle(value); },
            configurable: true
        });

        // 窗口控制方法
        globalThis.minimize = function() { return __windowMinimize(); };
        globalThis.maximize = function() { return __windowMaximize(); };
        globalThis.restore = function() { return __windowRestore(); };
        globalThis.close = function() { return __windowClose(); };

        (function(global) {
            const DEFAULT_ORIGIN = 'http://mblink.local';

            const navigationState = global.__mblinkNavigationState || {
                stack: [],
                index: 0
            };
            global.__mblinkNavigationState = navigationState;

            function stringify(value) {
                return value == null ? '' : String(value);
            }

            function normalizeHref(input) {
                let href = stringify(input).trim();
                if (!href) {
                    return DEFAULT_ORIGIN + '/';
                }

                if (/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(href)) {
                    return href;
                }

                const hasBase = Array.isArray(navigationState.stack) && navigationState.stack.length > 0;
                const base = hasBase ? (navigationState.stack[navigationState.index] || navigationState.stack[0]) : {
                    origin: DEFAULT_ORIGIN,
                    pathname: '/',
                    search: '',
                    hash: ''
                };

                if (href.charAt(0) === '#') {
                    return base.origin + base.pathname + base.search + href;
                }
                if (href.charAt(0) === '?') {
                    return base.origin + base.pathname + href + base.hash;
                }
                if (href.charAt(0) === '/') {
                    return DEFAULT_ORIGIN + href;
                }

                href = href.replace(/^\.\//, '');
                if (!href.startsWith('/')) {
                    href = '/' + href;
                }
                return DEFAULT_ORIGIN + href;
            }

            function parseHref(input) {
                const normalized = normalizeHref(input);
                const match = /^(?:([a-zA-Z][a-zA-Z0-9+.-]*):\/\/([^/?#]*))?([^?#]*)(\?[^#]*)?(#.*)?$/.exec(normalized) || [];
                const protocol = match[1] ? match[1] + ':' : 'http:';
                const host = match[2] || 'mblink.local';
                let pathname = match[3] || '/';
                if (!pathname.startsWith('/')) {
                    pathname = '/' + pathname;
                }
                pathname = pathname.replace(/\/\/+/g, '/');
                const search = match[4] || '';
                const hash = match[5] || '';
                const origin = protocol + '//' + host;
                const hostname = host.indexOf(':') >= 0 ? host.slice(0, host.indexOf(':')) : host;
                const port = host.indexOf(':') >= 0 ? host.slice(host.indexOf(':') + 1) : '';
                return {
                    href: origin + pathname + search + hash,
                    origin,
                    protocol,
                    host,
                    hostname,
                    port,
                    pathname,
                    search,
                    hash,
                    state: null
                };
            }

            function cloneState(state) {
                return state === undefined ? null : state;
            }

            function createEntry(url, state) {
                const parsed = parseHref(url);
                parsed.state = cloneState(state);
                return parsed;
            }

            if (!Array.isArray(navigationState.stack) || navigationState.stack.length === 0) {
                navigationState.stack = [createEntry('/', null)];
                navigationState.index = 0;
            }

            function currentEntry() {
                return navigationState.stack[navigationState.index] || navigationState.stack[0];
            }

            const locationObject = typeof global.location === 'object' && global.location !== null ? global.location : {};
            const historyObject = typeof global.history === 'object' && global.history !== null ? global.history : {};

            function defineValue(target, key, getter, setter) {
                Object.defineProperty(target, key, {
                    get: getter,
                    set: setter,
                    enumerable: true,
                    configurable: true
                });
            }

            function updateObjects(entry) {
                defineValue(locationObject, 'href', function() { return currentEntry().href; }, function(value) {
                    navigate(value, { mode: 'push' });
                });
                defineValue(locationObject, 'origin', function() { return currentEntry().origin; });
                defineValue(locationObject, 'protocol', function() { return currentEntry().protocol; });
                defineValue(locationObject, 'host', function() { return currentEntry().host; });
                defineValue(locationObject, 'hostname', function() { return currentEntry().hostname; });
                defineValue(locationObject, 'port', function() { return currentEntry().port; });
                defineValue(locationObject, 'pathname', function() { return currentEntry().pathname; }, function(value) {
                    const next = currentEntry();
                    let pathname = stringify(value) || '/';
                    if (!pathname.startsWith('/')) pathname = '/' + pathname;
                    navigate(pathname + next.search + next.hash, { mode: 'push' });
                });
                defineValue(locationObject, 'search', function() { return currentEntry().search; }, function(value) {
                    let search = stringify(value);
                    if (search && !search.startsWith('?')) search = '?' + search;
                    const next = currentEntry();
                    navigate(next.pathname + search + next.hash, { mode: 'push' });
                });
                defineValue(locationObject, 'hash', function() { return currentEntry().hash; }, function(value) {
                    let hash = stringify(value);
                    if (hash && !hash.startsWith('#')) hash = '#' + hash;
                    const next = currentEntry();
                    navigate(next.pathname + next.search + hash, { mode: 'push' });
                });
                defineValue(historyObject, 'length', function() { return navigationState.stack.length; });
                defineValue(historyObject, 'state', function() { return currentEntry().state; });
            }

            function dispatchNavigationEvents(previous, next, options) {
                const prevEntry = previous || next;
                if (options && options.popstate) {
                    const popstateEvent = new CustomEvent('popstate', {
                        bubbles: false,
                        cancelable: false,
                        detail: next.state
                    });
                    try {
                        Object.defineProperty(popstateEvent, 'state', {
                            value: next.state,
                            enumerable: true,
                            configurable: true
                        });
                    } catch (_) {
                        popstateEvent.state = next.state;
                    }
                    global.dispatchEvent(popstateEvent);
                }

                if (prevEntry.hash !== next.hash) {
                    const hashchangeEvent = new CustomEvent('hashchange', {
                        bubbles: false,
                        cancelable: false,
                        detail: {
                            oldURL: prevEntry.href,
                            newURL: next.href
                        }
                    });
                    try {
                        Object.defineProperty(hashchangeEvent, 'oldURL', {
                            value: prevEntry.href,
                            enumerable: true,
                            configurable: true
                        });
                        Object.defineProperty(hashchangeEvent, 'newURL', {
                            value: next.href,
                            enumerable: true,
                            configurable: true
                        });
                    } catch (_) {
                        hashchangeEvent.oldURL = prevEntry.href;
                        hashchangeEvent.newURL = next.href;
                    }
                    global.dispatchEvent(hashchangeEvent);
                }
            }

            function navigate(url, options) {
                options = options || {};
                const previous = currentEntry();
                const next = createEntry(url == null ? previous.href : url, options.state);

                if (options.mode === 'replace') {
                    navigationState.stack[navigationState.index] = next;
                } else if (options.mode === 'pop') {
                    navigationState.index = options.index;
                } else {
                    navigationState.stack = navigationState.stack.slice(0, navigationState.index + 1);
                    navigationState.stack.push(next);
                    navigationState.index = navigationState.stack.length - 1;
                }

                updateObjects(next);
                dispatchNavigationEvents(previous, next, options);
                return next.href;
            }

            locationObject.assign = function(url) {
                navigate(url, { mode: 'push' });
            };
            locationObject.replace = function(url) {
                navigate(url, { mode: 'replace' });
            };
            locationObject.reload = function() {};
            locationObject.toString = function() {
                return currentEntry().href;
            };

            historyObject.pushState = function(state, title, url) {
                navigate(url == null ? currentEntry().href : url, {
                    mode: 'push',
                    state: state
                });
            };
            historyObject.replaceState = function(state, title, url) {
                navigate(url == null ? currentEntry().href : url, {
                    mode: 'replace',
                    state: state
                });
            };
            historyObject.go = function(delta) {
                const offset = Number(delta || 0);
                if (!Number.isFinite(offset)) return;
                const nextIndex = Math.max(0, Math.min(navigationState.stack.length - 1, navigationState.index + offset));
                if (nextIndex === navigationState.index) return;
                const previous = currentEntry();
                navigationState.index = nextIndex;
                const next = currentEntry();
                updateObjects(next);
                dispatchNavigationEvents(previous, next, { popstate: true });
            };
            historyObject.back = function() {
                historyObject.go(-1);
            };
            historyObject.forward = function() {
                historyObject.go(1);
            };

            updateObjects(currentEntry());
            Object.defineProperty(global, 'location', {
                get: function() { return locationObject; },
                set: function(value) { navigate(value, { mode: 'push' }); },
                enumerable: true,
                configurable: true
            });
            Object.defineProperty(global, 'history', {
                get: function() { return historyObject; },
                enumerable: true,
                configurable: true
            });
        })(globalThis);

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

        (function(global) {
            function hasElementShape(value) {
                return !!value && typeof value === 'object' && typeof value.tagName === 'string';
            }

            function hasTagName(value, tagName) {
                return hasElementShape(value) && value.tagName.toLowerCase() === tagName;
            }

            function installInterface(name, baseName, predicate) {
                if (typeof global[name] === 'function') {
                    return;
                }

                const ctor = function() {
                    throw new TypeError('Illegal constructor');
                };
                const base = baseName && global[baseName] && global[baseName].prototype
                    ? global[baseName].prototype
                    : Object.prototype;
                ctor.prototype = Object.create(base);
                Object.defineProperty(ctor.prototype, 'constructor', {
                    value: ctor,
                    writable: true,
                    configurable: true
                });
                if (typeof Symbol === 'function' && Symbol.hasInstance) {
                    Object.defineProperty(ctor, Symbol.hasInstance, {
                        value: predicate,
                        configurable: true
                    });
                }
                Object.defineProperty(global, name, {
                    value: ctor,
                    writable: true,
                    configurable: true
                });
            }

            installInterface('Element', null, hasElementShape);
            installInterface('HTMLElement', 'Element', hasElementShape);
            installInterface('HTMLCanvasElement', 'HTMLElement', function(value) {
                return hasTagName(value, 'canvas');
            });
            installInterface('HTMLImageElement', 'HTMLElement', function(value) {
                return hasTagName(value, 'img');
            });

            function rectFor(target) {
                if (target && typeof target.getBoundingClientRect === 'function') {
                    try {
                        const rect = target.getBoundingClientRect();
                        const x = Number(rect.x || rect.left || 0);
                        const y = Number(rect.y || rect.top || 0);
                        const width = Number(rect.width || 0);
                        const height = Number(rect.height || 0);
                        return {
                            x,
                            y,
                            width,
                            height,
                            top: Number(rect.top || y),
                            left: Number(rect.left || x),
                            right: Number(rect.right || (x + width)),
                            bottom: Number(rect.bottom || (y + height))
                        };
                    } catch (_) {}
                }

                const width = Number((target && (target.clientWidth || target.width)) || 0);
                const height = Number((target && (target.clientHeight || target.height)) || 0);
                return {
                    x: 0,
                    y: 0,
                    width,
                    height,
                    top: 0,
                    left: 0,
                    right: width,
                    bottom: height
                };
            }

            if (typeof global.ResizeObserver !== 'function') {
                global.ResizeObserver = class ResizeObserver {
                    constructor(callback) {
                        if (typeof callback !== 'function') {
                            throw new TypeError('ResizeObserver callback must be a function');
                        }
                        this._callback = callback;
                        this._targets = [];
                        this._scheduled = false;
                    }

                    observe(target) {
                        if (!target) {
                            throw new TypeError('ResizeObserver.observe requires a target');
                        }
                        if (this._targets.indexOf(target) < 0) {
                            this._targets.push(target);
                        }
                        this._schedule();
                    }

                    unobserve(target) {
                        this._targets = this._targets.filter(function(item) {
                            return item !== target;
                        });
                    }

                    disconnect() {
                        this._targets = [];
                        this._scheduled = false;
                    }

                    _schedule() {
                        if (this._scheduled) {
                            return;
                        }
                        this._scheduled = true;
                        const run = () => {
                            this._scheduled = false;
                            if (!this._targets.length) {
                                return;
                            }
                            const entries = this._targets.map(function(target) {
                                const rect = rectFor(target);
                                const size = { inlineSize: rect.width, blockSize: rect.height };
                                return {
                                    target,
                                    contentRect: rect,
                                    borderBoxSize: [size],
                                    contentBoxSize: [size],
                                    devicePixelContentBoxSize: [size]
                                };
                            });
                            this._callback(entries, this);
                        };
                        if (typeof global.queueMicrotask === 'function') {
                            global.queueMicrotask(run);
                        } else if (typeof global.setTimeout === 'function') {
                            global.setTimeout(run, 0);
                        } else if (typeof global.requestAnimationFrame === 'function') {
                            global.requestAnimationFrame(run);
                        } else {
                            Promise.resolve().then(run);
                        }
                    }
                };
            }
        })(globalThis);
    )";

    runtime_->Eval(window_code, "<window_bindings>");

    runtime_->RegisterFunction("__performanceNow", [](const json& args) -> json {
        return static_cast<double>(SDL_GetTicksNS()) / 1000000.0;
    });

    std::string perf_microtask_code = R"(
        if (typeof globalThis.queueMicrotask !== 'function') {
            globalThis.queueMicrotask = function(cb) {
                return Promise.resolve().then(cb);
            };
        }

        if (typeof globalThis.performance !== 'object' || globalThis.performance === null) {
            globalThis.performance = {};
        }

        if (typeof globalThis.performance.now !== 'function') {
            globalThis.performance.now = function() {
                return __performanceNow();
            };
        }
    )";

    runtime_->Eval(perf_microtask_code, "<performance_microtask_bindings>");
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
    std::string event_code = R"(
        globalThis.window.addEventListener = function(type, listener, options) {
            return document.addEventListener(type, listener, options);
        };

        globalThis.window.removeEventListener = function(type, listener, options) {
            return document.removeEventListener(type, listener, options);
        };

        globalThis.window.dispatchEvent = function(event) {
            return document.dispatchEvent(event);
        };

        globalThis.addEventListener = globalThis.window.addEventListener;
        globalThis.removeEventListener = globalThis.window.removeEventListener;
        globalThis.dispatchEvent = globalThis.window.dispatchEvent;
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

} // namespace mblink

