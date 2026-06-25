/**
 * @file mblink.cpp
 * @brief MBlink C API v2 实现
 *
 * 一个 create() 调用完成全部初始化：
 * Window → Document → TaskScheduler → QuickJSRuntime → DOMBindings →
 * WindowBindings → EventLoop → FetchBindings → StateManager → HostBridge
 */

#include "mblink.h"
#include "resource_package.h"
#include "core/bridge/state_manager.h"
#include "core/bridge/host_bridge.h"
#include "core/bridge/main_thread_queue.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/window/app_tray.h"
#include "core/dom/document.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/event/loop/event_loop.h"
#include "core/event/loop/task_scheduler.h"
#include "core/network/fetch_bindings.h"
#include "core/render/image/image_loader.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "core/quickjs/dom_binding_map.h"
#include "tools/esm_loader/embedded_js.h"
#include "core/devtools/devtools_bridge.h"

#include "core/utils/encoding_utils.h"
#include "core/utils/async_resource_context.h"
#include "core/utils/background_task_runner.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

#include <algorithm>
#include <atomic>
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>

namespace {

namespace fs = std::filesystem;

#define SAFE_CLEANUP(label, ...) \
    do { \
        try { \
            __VA_ARGS__; \
        } catch (const std::exception& e) { \
            (void)(label); \
            (void)e; \
        } catch (...) { \
            (void)(label); \
        } \
    } while (0)


bool g_initialized = false;
std::string g_lastError;
std::mutex g_errorMutex;
#ifdef _WIN32
std::mutex g_dbghelpMutex;
#endif

constexpr unsigned int kDevToolsAttachVersion = mblink::kDevToolsBridgeVersion;

void ClearElementListenersRecursive(const std::shared_ptr<mblink::Node>& node) {
    if (!node) {
        return;
    }

    if (auto element = std::dynamic_pointer_cast<mblink::Element>(node)) {
        element->ClearAllEventListeners();
    }

    for (const auto& child : node->GetChildNodes()) {
        ClearElementListenersRecursive(child);
    }
}

void ClearDocumentElementListeners(const std::shared_ptr<mblink::Document>& document) {
    if (!document) {
        return;
    }

    if (auto document_element = document->GetDocumentElement()) {
        ClearElementListenersRecursive(document_element);
        return;
    }

    if (auto body = document->GetBody()) {
        ClearElementListenersRecursive(body);
    }
}


// ========== WindowContext ==========

struct WindowContext;
class StructuredJsonBuffer;
mblink::DevToolsHostContext makeDevToolsHostContext(WindowContext* ctx);

struct MountedResourceState {
    mutable std::mutex mutex;
    std::string package;
    std::string key;
    std::string mountPoint = "/";
    std::atomic<bool> alive{true};
};

// ========== SharedObject 结构体 ==========
// Python/JS 共享的 C 对象，内含 QuickJS JSValue
struct SharedObjectData {
    JSContext* ctx = nullptr;
    JSValue js_obj = JS_UNDEFINED;       // 实际的 JS 对象（globalThis.<name>）
    JSValue updater_func = JS_UNDEFINED; // 内部 shared update dispatcher 缓存
    std::string name;                    // globalThis 上的名字
    int batch_depth = 0;                 // 批量层级（支持嵌套）
    bool pending_notify_ = false;        // 是否有待处理的非批量通知（延迟刷新用）
    std::unordered_set<std::string> pending_keys_; // 待通知的顶层 key 集合

    // ===== 线程安全数据存储 =====
    mutable std::shared_mutex dataMutex_;
    nlohmann::json data_ = nlohmann::json::object();
    mblink::MainThreadQueue* mainQueue_ = nullptr;
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
    WindowContext* owner_ = nullptr;

    void refreshUpdater() {
        if (!ctx) return;
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue updater = JS_GetPropertyStr(ctx, global, "__mblinkSharedUpdateDispatcher");
        JS_FreeValue(ctx, global);

        if (!JS_IsUndefined(updater_func)) {
            JS_FreeValue(ctx, updater_func);
        }

        if (JS_IsFunction(ctx, updater)) {
            updater_func = updater;
        } else {
            JS_FreeValue(ctx, updater);
            updater_func = JS_UNDEFINED;
        }
    }

    void lazyRefreshUpdater() {
        if (JS_IsUndefined(updater_func)) {
            refreshUpdater();
        }
    }

    void beginBatch() {
        batch_depth++;
    }

    void endBatch() {
        if (batch_depth <= 0) return;
        batch_depth--;
        if (batch_depth == 0) {
            flushBatch();
        }
    }

    // notifyUpdate：记录待通知，不立即调用 JS。
    // 真正的通知由 flushPendingNotify() 在事件循环帧中统一触发，
    // 避免在 Python 回调的 C 调用链中嵌套执行 QuickJS JS 代码（QuickJS 重入）。
    void notifyUpdate(const char* key) {
        if (key && *key) {
            pending_keys_.insert(std::string(key));
        }
        pending_notify_ = true;
    }

    void flushPendingNotify() {
        if (!pending_notify_) return;
        pending_notify_ = false;
        if (pending_keys_.empty()) return;

        std::vector<std::string> changedKeys;
        changedKeys.reserve(pending_keys_.size());
        for (const auto& key : pending_keys_) {
            changedKeys.push_back(key);
        }
        pending_keys_.clear();

        refreshUpdater();
        if (!JS_IsUndefined(updater_func) && !JS_IsNull(updater_func)) {
            JSValue arg = JS_NewArray(ctx);
            for (uint32_t i = 0; i < changedKeys.size(); ++i) {
                JS_SetPropertyUint32(ctx, arg, i,
                    JS_NewString(ctx, changedKeys[i].c_str()));
            }
            JSValue ret = JS_Call(ctx, updater_func, JS_UNDEFINED, 1, &arg);
            JS_FreeValue(ctx, arg);
            if (JS_IsException(ret)) {
                JSValue exc = JS_GetException(ctx);
                JS_FreeValue(ctx, exc);
            }
            JS_FreeValue(ctx, ret);
        }

    }

    void flushBatch() {
        if (!pending_keys_.empty()) {
            pending_notify_ = true;
        }
    }

    void applyPropertyToJS(const char* key, const nlohmann::json& value) {
        if (value.is_null()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NULL);
        } else if (value.is_boolean()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NewBool(ctx, value.get<bool>()));
        } else if (value.is_number_integer()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NewInt64(ctx, value.get<int64_t>()));
        } else if (value.is_number_unsigned()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NewInt64(ctx, static_cast<int64_t>(value.get<uint64_t>())));
        } else if (value.is_number_float()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NewFloat64(ctx, value.get<double>()));
        } else if (value.is_string()) {
            JS_SetPropertyStr(ctx, js_obj, key, JS_NewString(ctx, value.get_ref<const std::string&>().c_str()));
        } else {
            const std::string jsonStr = value.dump();
            JSValue parsed = JS_ParseJSON(ctx, jsonStr.c_str(), jsonStr.size(), "<shared_json>");
            if (!JS_IsException(parsed)) {
                JS_SetPropertyStr(ctx, js_obj, key, parsed);
            } else {
                JSValue exc = JS_GetException(ctx);
                JS_FreeValue(ctx, exc);
            }
        }

    }

    bool safeSetProperty(const char* key, const nlohmann::json& value) {
        bool changed = false;
        {
            std::unique_lock<std::shared_mutex> lock(dataMutex_);
            auto it = data_.find(key);
            if (it != data_.end() && it.value() == value) {
                return false;
            }
            data_[key] = value;
            changed = true;
        }

        if (!changed) {
            return false;
        }

        const std::string depId = name + ":" + key;

        if (mainQueue_ && mainQueue_->isMainThread()) {
            applyPropertyToJS(key, value);
            lazyRefreshUpdater();
            notifyUpdate(depId.c_str());
        } else if (mainQueue_) {
            std::string keyCopy(key);
            std::string depIdCopy(depId);
            nlohmann::json valueCopy = value;
            auto queueAlive = mainQueue_->aliveFlag();
            auto objAlive = alive_;
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, depIdCopy, valueCopy, queueAlive, objAlive]() {
                if (!queueAlive->load() || !objAlive->load()) return;
                self->applyPropertyToJS(keyCopy.c_str(), valueCopy);
                self->lazyRefreshUpdater();
                self->notifyUpdate(depIdCopy.c_str());
            });
        }

        return true;
    }

    bool safeSetPropertyFromJS(const char* key, const nlohmann::json& value) {
        std::unique_lock<std::shared_mutex> lock(dataMutex_);
        auto it = data_.find(key);
        if (it != data_.end() && it.value() == value) {
            return false;
        }
        data_[key] = value;
        return true;
    }

    bool safeDeleteProperty(const char* key) {
        bool existed = false;
        {
            std::unique_lock<std::shared_mutex> lock(dataMutex_);
            existed = data_.erase(key) > 0;
        }

        if (!existed) {
            return false;
        }

        const std::string depId = name + ":" + key;

        if (mainQueue_ && mainQueue_->isMainThread()) {
            JSAtom atom = JS_NewAtom(ctx, key);
            JS_DeleteProperty(ctx, js_obj, atom, 0);
            JS_FreeAtom(ctx, atom);
            notifyUpdate(depId.c_str());
        } else if (mainQueue_) {
            std::string keyCopy(key);
            std::string depIdCopy(depId);
            auto queueAlive = mainQueue_->aliveFlag();
            auto objAlive = alive_;
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, depIdCopy, queueAlive, objAlive]() {
                if (!queueAlive->load() || !objAlive->load()) return;
                JSAtom atom = JS_NewAtom(self->ctx, keyCopy.c_str());
                JS_DeleteProperty(self->ctx, self->js_obj, atom, 0);
                JS_FreeAtom(self->ctx, atom);
                self->notifyUpdate(depIdCopy.c_str());
            });
        }

        return true;
    }

    bool safeDeletePropertyFromJS(const char* key) {
        std::unique_lock<std::shared_mutex> lock(dataMutex_);
        return data_.erase(key) > 0;
    }

    nlohmann::json safeGetProperty(const char* key) const {
        std::shared_lock<std::shared_mutex> lock(dataMutex_);
        auto it = data_.find(key);
        if (it == data_.end()) {
            return nullptr;
        }
        return *it;
    }

    bool safeHasProperty(const char* key) const {
        std::shared_lock<std::shared_mutex> lock(dataMutex_);
        return data_.contains(key);
    }

    int safeGetType(const char* key) const {
        auto val = safeGetProperty(key);
        if (val.is_null() || val.is_discarded()) return MBLINK_TYPE_NULL;
        if (val.is_boolean()) return MBLINK_TYPE_BOOL;
        if (val.is_number_integer() || val.is_number_unsigned()) return MBLINK_TYPE_INT;
        if (val.is_number_float()) return MBLINK_TYPE_DOUBLE;
        if (val.is_string()) return MBLINK_TYPE_STRING;
        if (val.is_array()) return MBLINK_TYPE_ARRAY;
        if (val.is_object()) return MBLINK_TYPE_OBJECT;
        return MBLINK_TYPE_NULL;
    }
};

struct WindowContext {
    // 核心组件（完整初始化链）
    std::shared_ptr<mblink::Window> window;
    std::shared_ptr<mblink::Document> document;
    std::shared_ptr<mblink::TaskScheduler> taskScheduler;
    std::unique_ptr<mblink::QuickJSRuntime> runtime;
    std::unique_ptr<mblink::WindowBindings> windowBindings;
    std::unique_ptr<mblink::EventLoop> eventLoop;
    std::unique_ptr<mblink::HostBridge> hostBridge;
    std::unique_ptr<mblink::FetchBindings> fetchBindings;
    std::unique_ptr<mblink::StateManager> stateManager;
    std::shared_ptr<mblink::BackgroundTaskRunner> backgroundTaskRunner;
    std::shared_ptr<mblink::AsyncResourceContext> asyncResourceContext;
    std::shared_ptr<MountedResourceState> mountedResourceState;
    uint64_t asyncOwnerToken = 0;
    mblink::MainThreadQueue mainThreadQueue;
    std::string mountedResourcePackage;
    std::string mountedResourceKey;
    std::string mountedResourceMountPoint = "/";
    std::string runtimeEpoch;
    bool embeddedRuntimeLoaded = false;
    bool officialPreactLoaded = false;
    std::string lifecycleReason = "created";
    MBlinkLifecycleState lifecycleState = MBLINK_LIFECYCLE_CREATED;
    std::unique_ptr<StructuredJsonBuffer> consoleBuffer;
    std::unique_ptr<StructuredJsonBuffer> errorBuffer;
    MBlinkObserveCallback observeCallback = nullptr;
    void* observeUserData = nullptr;
    std::shared_ptr<std::atomic<bool>> shutdownRequested =
        std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<bool>> devtoolsSyncCancelled =
        std::make_shared<std::atomic<bool>>(false);

    // 共享对象存储
    std::unordered_map<std::string, SharedObjectData*> sharedObjects;

    // 回调存储
    std::unordered_map<int, std::pair<MBlinkStateCallback, void*>> watchCallbacks;
    std::unordered_map<std::string, std::pair<MBlinkCallback, void*>> boundFunctions;
    std::unordered_map<std::string, std::pair<MBlinkAsyncCallback, void*>> boundAsyncFunctions;

    // 事件回调
    MBlinkResizeCallback onResizeCallback = nullptr;
    void* onResizeUserData = nullptr;
    MBlinkBoolCallback onCloseRequestCallback = nullptr;
    void* onCloseRequestUserData = nullptr;
    MBlinkVoidCallback onCloseCallback = nullptr;
    void* onCloseUserData = nullptr;
    MBlinkVoidCallback onFocusCallback = nullptr;
    void* onFocusUserData = nullptr;
    MBlinkVoidCallback onBlurCallback = nullptr;
    void* onBlurUserData = nullptr;
    MBlinkUpdateCallback onUpdateCallback = nullptr;
    void* onUpdateUserData = nullptr;
    MBlinkVoidCallback onTrayLeftClickCallback = nullptr;
    void* onTrayLeftClickUserData = nullptr;
    MBlinkCallback onTrayMenuCallback = nullptr;
    void* onTrayMenuUserData = nullptr;

    bool running = false;
    bool initialFrameWarmupDone = false;
    std::unique_ptr<mblink::AppTray> tray;
    std::string trayTooltip;
    std::vector<mblink::AppTrayMenuItem> trayMenuItems;
};

int RunDevToolsOnMainThreadSync(const mblink::DevToolsHostContext* host,
                                mblink::DevToolsMainThreadTask task,
                                void* user_data) {
    if (!host || !host->host_user_data || !task) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    auto* ctx = static_cast<WindowContext*>(host->host_user_data);
    if (!ctx || !ctx->shutdownRequested || ctx->shutdownRequested->load()) {
        return MBLINK_ERROR_INVALID_HANDLE;
    }
    if (ctx->mainThreadQueue.isMainThread()) {
        task(user_data);
        return MBLINK_OK;
    }

    struct PendingTask {
        mblink::DevToolsMainThreadTask task = nullptr;
        void* user_data = nullptr;
        std::atomic<bool> cancel{false};
    };
    auto pending = std::make_shared<PendingTask>();
    pending->task = task;
    pending->user_data = user_data;
    auto done = std::make_shared<std::atomic<bool>>(false);
    auto alive = ctx->mainThreadQueue.aliveFlag();
    auto global_cancelled = ctx->devtoolsSyncCancelled;
    const auto cancelled = [global_cancelled]() {
        return global_cancelled && global_cancelled->load(std::memory_order_acquire);
    };
    ctx->mainThreadQueue.post([pending, done, alive, global_cancelled]() {
        const bool should_cancel =
            pending->cancel.load(std::memory_order_acquire) ||
            (global_cancelled && global_cancelled->load(std::memory_order_acquire));
        if (!should_cancel && alive && alive->load() && pending->task) {
            pending->task(pending->user_data);
        }
        done->store(true, std::memory_order_release);
    });

    while (!done->load(std::memory_order_acquire)) {
        if (cancelled() ||
            (ctx->shutdownRequested && ctx->shutdownRequested->load(std::memory_order_acquire))) {
            pending->cancel.store(true, std::memory_order_release);
            return MBLINK_ERROR_INVALID_HANDLE;
        }
        if (!alive || !alive->load()) {
            pending->cancel.store(true, std::memory_order_release);
            return MBLINK_ERROR_INVALID_HANDLE;
        }
#ifdef _WIN32
        Sleep(1);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
    }
    return MBLINK_OK;
}

int RunWithCurrentDevToolsContextSync(void* host_user_data,
                                      mblink::DevToolsHostContextTask task,
                                      void* user_data) {
    if (!host_user_data || !task) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    auto* ctx = static_cast<WindowContext*>(host_user_data);
    if (!ctx || !ctx->shutdownRequested || ctx->shutdownRequested->load()) {
        return MBLINK_ERROR_INVALID_HANDLE;
    }

    struct TaskState {
        WindowContext* ctx = nullptr;
        mblink::DevToolsHostContextTask task = nullptr;
        void* user_data = nullptr;
    } state{ctx, task, user_data};

    auto initial_host = makeDevToolsHostContext(ctx);
    return RunDevToolsOnMainThreadSync(
        &initial_host,
        [](void* raw_state) {
            auto* state = static_cast<TaskState*>(raw_state);
            if (!state || !state->ctx || !state->task) {
                return;
            }
            auto host = makeDevToolsHostContext(state->ctx);
            state->task(&host, state->user_data);
        },
        &state);
}

struct LogViewHandleData {
    std::shared_ptr<mblink::HTMLLogViewElement> element;
    mblink::MainThreadQueue* mainQueue = nullptr;
};

struct TerminalHandleData {
    std::shared_ptr<mblink::HTMLTerminalElement> element;
    mblink::MainThreadQueue* mainQueue = nullptr;
};

class StructuredJsonBuffer {
public:
    StructuredJsonBuffer(std::string field_name, size_t max_entries)
        : field_name_(std::move(field_name)), max_entries_(max_entries) {}

    void Push(nlohmann::json entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        entry["timestamp"] = CurrentTimestampIso8601();
        entry["line"] = ++next_line_;
        entries_.push_back(std::move(entry));
        while (entries_.size() > max_entries_) entries_.pop_front();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        next_line_ = 0;
    }

    nlohmann::json ToJson() const {
        std::lock_guard<std::mutex> lock(mutex_);
        nlohmann::json payload;
        payload[field_name_] = nlohmann::json::array();
        for (const auto& entry : entries_) payload[field_name_].push_back(entry);
        return payload;
    }

private:
    static std::string CurrentTimestampIso8601() {
        using namespace std::chrono;
        const auto now = system_clock::now();
        const auto t = system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        char buf[64] = {0};
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec);
        return std::string(buf);
    }

    std::string field_name_;
    size_t max_entries_ = 0;
    mutable std::mutex mutex_;
    std::deque<nlohmann::json> entries_;
    int next_line_ = 0;
};

// ========== 辅助函数 ==========

void setLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastError = error;
}

template <typename T>
std::shared_ptr<T> getElementByIdAs(WindowContext* ctx, const char* element_id) {
    if (!ctx || !ctx->document || !element_id || !*element_id) {
        return nullptr;
    }
    auto element = ctx->document->GetElementById(element_id);
    if (!element) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<T>(element);
}

std::vector<std::string> collectSharedObjectNames(WindowContext* ctx) {
    std::vector<std::string> names;
    if (!ctx) return names;
    names.reserve(ctx->sharedObjects.size());
    for (const auto& kv : ctx->sharedObjects) {
        names.push_back(kv.first);
    }
    return names;
}

template <typename Fn>
void forEachSharedObjectSnapshot(WindowContext* ctx, Fn&& fn) {
    for (const auto& name : collectSharedObjectNames(ctx)) {
        auto it = ctx->sharedObjects.find(name);
        if (it != ctx->sharedObjects.end() && it->second) {
            fn(it->second);
        }
    }
}

void destroySharedObjectInternal(SharedObjectData* shared, bool removeFromOwner) {
    if (!shared) return;

    *(shared->alive_) = false;

    if (removeFromOwner && shared->owner_) {
        shared->owner_->sharedObjects.erase(shared->name);
    }

    if (shared->ctx) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        JSAtom atom = JS_NewAtom(shared->ctx, shared->name.c_str());
        JS_DeleteProperty(shared->ctx, global, atom, 0);
        JS_FreeAtom(shared->ctx, atom);
        JS_FreeValue(shared->ctx, global);

        JS_FreeValue(shared->ctx, shared->js_obj);
        if (!JS_IsUndefined(shared->updater_func)) {
            JS_FreeValue(shared->ctx, shared->updater_func);
        }
    }

    delete shared;
}


void reportNativeError(const std::string& error) {
    setLastError(error);
    std::fprintf(stderr, "[MBlink Native Error] %s\n", error.c_str());
#ifdef _WIN32
    std::string out = "[MBlink Native Error] " + error + "\n";
    ::OutputDebugStringA(out.c_str());
#endif
    std::ofstream log("mblink_native_error.log", std::ios::app);
    if (log.is_open()) {
        log << error << std::endl;
    }
}

void processRuntimeQueuesForWarmup(WindowContext* ctx) {
    if (!ctx) return;

    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    if (ctx->runtime) {
        ctx->runtime->RunEventLoop(1);
        ctx->runtime->ProcessMicrotasks();
    }
    ctx->mainThreadQueue.flush();
    forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
        shared->flushPendingNotify();
    });
    if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    }
}

void renderWarmupFrame(WindowContext* ctx, bool forceFullRepaint) {
    if (!ctx || !ctx->window) return;

    if (forceFullRepaint) {
        ctx->window->InvalidateRenderTree();
        ctx->window->SetForceFullRepaint(true);
        ctx->window->SetNeedsRepaintFor(mblink::RepaintReason::API);
    }

    if (ctx->window->NeedsRepaint()) {
        ctx->window->Render();
        ctx->window->SwapBuffers();
    }
}

void ensureInitialFrameWarmup(WindowContext* ctx) {
    if (!ctx || ctx->initialFrameWarmupDone) return;
    ctx->initialFrameWarmupDone = true;

    try {
        renderWarmupFrame(ctx, true);
        processRuntimeQueuesForWarmup(ctx);
        renderWarmupFrame(ctx, false);
    } catch (const std::exception& e) {
        reportNativeError(std::string("Initial frame warmup failed: ") + e.what());
    } catch (...) {
        reportNativeError("Initial frame warmup failed: unknown error");
    }
}

#ifdef _WIN32
std::string formatHexValue(uint64_t value, size_t width = 0) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    if (width > 0) {
        oss << std::setw(static_cast<int>(width));
    }
    oss << value;
    return oss.str();
}

std::string formatPointer(const void* ptr) {
    return "0x" + formatHexValue(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr)), sizeof(uintptr_t) * 2);
}

void ensureDbgHelpInitialized() {
    static std::once_flag once;
    std::call_once(once, []() {
        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(process, nullptr, TRUE);
    });
}

std::string captureCurrentStackTrace(USHORT framesToSkip) {
    ensureDbgHelpInitialized();

    void* frames[62] = {};
    const USHORT captured = CaptureStackBackTrace(static_cast<DWORD>(framesToSkip + 1), 62, frames, nullptr);
    if (captured == 0) {
        return "  <no stack trace>\n";
    }

    HANDLE process = GetCurrentProcess();
    std::lock_guard<std::mutex> lock(g_dbghelpMutex);
    std::ostringstream oss;

    for (USHORT i = 0; i < captured; ++i) {
        const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;

        const bool hasSymbol = SymFromAddr(process, address, &displacement, symbol) == TRUE;
        const bool hasLine = SymGetLineFromAddr64(process, address, &lineDisplacement, &line) == TRUE;

        oss << "  #" << i << " ";
        if (hasSymbol) {
            oss << symbol->Name;
            if (displacement > 0) {
                oss << " +0x" << formatHexValue(displacement);
            }
        } else {
            oss << "<unknown>";
        }
        if (hasLine && line.FileName) {
            oss << " (" << line.FileName << ":" << line.LineNumber << ")";
        }
        oss << " [" << formatPointer(reinterpret_cast<void*>(address)) << "]\n";
    }

    return oss.str();
}

std::string formatSehException(const char* scope, EXCEPTION_POINTERS* exceptionInfo, USHORT framesToSkip = 0) {
    const auto* record = exceptionInfo ? exceptionInfo->ExceptionRecord : nullptr;
    const unsigned int code = record ? static_cast<unsigned int>(record->ExceptionCode) : 0;
    const void* address = record ? record->ExceptionAddress : nullptr;

    std::ostringstream oss;
    oss << "SEH exception";
    if (scope && *scope) {
        oss << " in " << scope;
    }
    oss << ", code=0x" << formatHexValue(code, 8)
        << ", address=" << formatPointer(address)
        << ", thread=" << GetCurrentThreadId();

    if (record && code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
        const auto action = static_cast<unsigned long long>(record->ExceptionInformation[0]);
        const void* target = reinterpret_cast<void*>(record->ExceptionInformation[1]);
        const char* op = action == 0 ? "read" : (action == 1 ? "write" : (action == 8 ? "execute" : "access"));
        oss << ", access=" << op << " " << formatPointer(target);
    }

    oss << "\nStack trace:\n" << captureCurrentStackTrace(static_cast<USHORT>(framesToSkip + 1));
    return oss.str();
}

int captureSehException(const char* scope,
                        unsigned int* sehCode,
                        std::string* sehMessage,
                        EXCEPTION_POINTERS* exceptionInfo,
                        bool continueSearch = false) {
    if (sehCode) {
        *sehCode = exceptionInfo && exceptionInfo->ExceptionRecord
            ? static_cast<unsigned int>(exceptionInfo->ExceptionRecord->ExceptionCode)
            : 0;
    }
    if (sehMessage) {
        *sehMessage = formatSehException(scope, exceptionInfo, 2);
    }
    return continueSearch ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

char* invokeCallbackWithSEH(MBlinkCallback cb,
                            const char* args,
                            void* user_data,
                            unsigned int* sehCode,
                            std::string* sehMessage);
bool runEventLoopWithSEH(mblink::EventLoop* eventLoop,
                         unsigned int* sehCode,
                         std::string* sehMessage);
#endif

std::string invokeBoundJsonCallback(WindowContext* ctx,
                                    const std::string& funcName,
                                    const std::string& args,
                                    MBlinkCallback cb,
                                    void* ud) {
    auto finishSharedBatch = [ctx]() {
        forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
            shared->endBatch();
        });
    };

    forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
        shared->beginBatch();
    });

    char* result = nullptr;

#ifdef _WIN32
    unsigned int sehCode = 0;
    std::string sehMessage;
    result = invokeCallbackWithSEH(cb, args.c_str(), ud, &sehCode, &sehMessage);
    finishSharedBatch();
    if (!result && sehCode != 0) {
        reportNativeError(sehMessage.empty()
            ? ("SEH exception in bound callback '" + funcName + "', code=0x" + formatHexValue(sehCode, 8))
            : sehMessage);
        return R"({"error":"Native SEH exception in callback"})";
    }
#else
    try {
        result = cb(args.c_str(), ud);
    } catch (const std::exception& e) {
        finishSharedBatch();
        reportNativeError("C++ exception in bound callback '" + funcName +
                          "': " + e.what());
        return std::string("{\"error\":\"Native callback exception: ") + e.what() + "\"}";
    } catch (...) {
        finishSharedBatch();
        reportNativeError("Unknown C++ exception in bound callback '" + funcName + "'");
        return R"({"error":"Native callback unknown exception"})";
    }
    finishSharedBatch();
#endif

    if (!result) {
        return "null";
    }

    std::string ret(result);
    mblink_free(result);
    return ret;
}

bool loadEmbeddedRuntimeScripts(mblink::QuickJSRuntime* runtime) {
    if (!runtime || !mblink::embedded::HasEmbeddedJS()) {
        return true;
    }

    auto evalScript = [&](std::string_view code, const char* filename) {
        if (code.empty()) {
            return;
        }
        runtime->Eval(std::string(code), filename ? filename : "<embedded>");
    };

    evalScript(mblink::embedded::GetDomPolyfillsJS(), "dom.js");
    evalScript(mblink::embedded::GetBootstrapJS(), "bootstrap.js");
    return true;
}

fs::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return fs::path(mblink::utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string FsPathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return mblink::utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizeFsPath(const fs::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

fs::path FindOfficialPreactRoot() {
    static const fs::path kOfficialPreactRelativeRoot =
        Utf8PathToFsPath("third_party") / Utf8PathToFsPath("preact");
    fs::path current = fs::absolute(Utf8PathToFsPath(__FILE__)).parent_path();
    while (!current.empty()) {
        const fs::path candidate = current / kOfficialPreactRelativeRoot / "package.json";
        if (fs::exists(candidate)) {
            return current / kOfficialPreactRelativeRoot;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate official Preact sources under third_party/preact");
}

std::string BuildOfficialPreactModule(const fs::path& entry_path) {
    return "export * from '" + NormalizeFsPath(fs::absolute(entry_path)) + "';";
}

std::string EmbeddedOfficialPreactModule(const char* path) {
    auto source = mblink::embedded::GetEmbeddedJS(path);
    if (source.empty()) {
        throw std::runtime_error(std::string("Missing embedded official Preact module: ") + path);
    }
    return std::string(source);
}

std::string StripJsExtension(std::string path) {
    if (path.size() > 3 && path.substr(path.size() - 3) == ".js") {
        path.resize(path.size() - 3);
    }
    return path;
}

std::string OfficialPreactModuleId(const char* path) {
    std::string id(path ? path : "");
    static const std::string prefix = "third_party/preact/";
    if (id.rfind(prefix, 0) == 0) {
        id.replace(0, prefix.size(), "__mblink_official_preact/");
    }
    return id;
}

std::string BuildEmbeddedOfficialPreactModule(const char* path) {
    return "export * from '" + OfficialPreactModuleId(path) + "';";
}

void RegisterOfficialPreactSource(mblink::QuickJSRuntime* runtime, const char* path) {
    const auto source = EmbeddedOfficialPreactModule(path);
    const auto module_id = OfficialPreactModuleId(path);
    runtime->RegisterModule(module_id, source);
    runtime->RegisterModule(StripJsExtension(module_id), source);
}

std::string NormalizeResourcePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.empty()) return "/";
    const bool is_relative =
        path.rfind("./", 0) == 0 ||
        path.rfind("../", 0) == 0 ||
        (!path.empty() && path.front() != '/');

    fs::path normalized_fs = Utf8PathToFsPath(path).lexically_normal();
    path = FsPathToUtf8String(normalized_fs);
    std::replace(path.begin(), path.end(), '\\', '/');

    if (path == ".") {
        path = "/";
    } else if (is_relative && (path.empty() || path.front() != '/')) {
        path.insert(path.begin(), '/');
    }
    if (path.front() != '/') path.insert(path.begin(), '/');
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    return path;
}

std::string JoinMountedResourcePath(const std::string& mountPoint, const std::string& requestPath) {
    std::string normalizedMount = NormalizeResourcePath(mountPoint.empty() ? "/" : mountPoint);
    std::string normalizedRequest = NormalizeResourcePath(requestPath);
    if (normalizedMount != "/") {
        if (normalizedRequest == normalizedMount) return "";
        if (normalizedRequest.rfind(normalizedMount + "/", 0) != 0) return "";
        normalizedRequest.erase(0, normalizedMount.size());
    }
    while (!normalizedRequest.empty() && normalizedRequest.front() == '/') {
        normalizedRequest.erase(normalizedRequest.begin());
    }
    return normalizedRequest;
}

bool LoadMountedResourceAsset(const WindowContext* ctx,
                              const std::string& requestPath,
                              std::vector<uint8_t>& out) {
    if (!ctx) {
        return false;
    }

    if (ctx->mountedResourceState) {
        auto state = ctx->mountedResourceState;
        if (!state->alive.load()) {
            return false;
        }

        std::string package;
        std::string key;
        std::string mountPoint;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            package = state->package;
            key = state->key;
            mountPoint = state->mountPoint;
        }

        if (package.empty()) {
            return false;
        }

        const std::string resourcePath = JoinMountedResourcePath(
            mountPoint.empty() ? "/" : mountPoint,
            requestPath);
        if (resourcePath.empty()) {
            return false;
        }

        std::string error;
        return mblink::resourcepkg::LoadResourceFile(
            package.c_str(),
            resourcePath.c_str(),
            key.c_str(),
            out,
            nullptr,
            error);
    }

    if (ctx->mountedResourcePackage.empty()) {
        return false;
    }

    const std::string resourcePath = JoinMountedResourcePath(
        ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
        requestPath);
    if (resourcePath.empty()) {
        return false;
    }

    std::string error;
    return mblink::resourcepkg::LoadResourceFile(
        ctx->mountedResourcePackage.c_str(),
        resourcePath.c_str(),
        ctx->mountedResourceKey.c_str(),
        out,
        nullptr,
        error);
}

bool LoadMountedResourceAsset(const std::shared_ptr<MountedResourceState>& state,
                              const std::string& requestPath,
                              std::vector<uint8_t>& out) {
    if (!state || !state->alive.load()) {
        return false;
    }

    std::string package;
    std::string key;
    std::string mountPoint;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        package = state->package;
        key = state->key;
        mountPoint = state->mountPoint;
    }

    if (package.empty()) {
        return false;
    }

    const std::string resourcePath = JoinMountedResourcePath(
        mountPoint.empty() ? "/" : mountPoint,
        requestPath);
    if (resourcePath.empty()) {
        return false;
    }

    std::string error;
    return mblink::resourcepkg::LoadResourceFile(
        package.c_str(),
        resourcePath.c_str(),
        key.c_str(),
        out,
        nullptr,
        error);
}

bool MountedResourcePackageEmpty(const std::shared_ptr<MountedResourceState>& state) {
    if (!state || !state->alive.load()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    return state->package.empty();
}

void SetMountedResourceState(WindowContext* ctx,
                             std::string package,
                             std::string key,
                             std::string mountPoint) {
    if (!ctx) {
        return;
    }

    ctx->mountedResourcePackage = package;
    ctx->mountedResourceKey = key;
    ctx->mountedResourceMountPoint = mountPoint;

    if (!ctx->mountedResourceState) {
        ctx->mountedResourceState = std::make_shared<MountedResourceState>();
    }

    std::lock_guard<std::mutex> lock(ctx->mountedResourceState->mutex);
    ctx->mountedResourceState->package = std::move(package);
    ctx->mountedResourceState->key = std::move(key);
    ctx->mountedResourceState->mountPoint = std::move(mountPoint);
}

void SetRuntimeBasePath(WindowContext* ctx, const std::string& basePath) {
    if (!ctx) {
        return;
    }
    if (ctx->document) {
        ctx->document->SetBasePath(basePath);
    }
    if (ctx->asyncResourceContext) {
        ctx->asyncResourceContext->SetBasePath(basePath);
    }
    mblink::FetchBindings::SetBasePath(basePath);
    mblink::ImageLoader::SetBasePath(basePath);
}

void registerPreactModules(mblink::QuickJSRuntime* runtime) {
    if (!runtime) {
        return;
    }

    static constexpr const char* kOfficialPreactSources[] = {
        "third_party/preact/src/index.js",
        "third_party/preact/src/render.js",
        "third_party/preact/src/create-element.js",
        "third_party/preact/src/component.js",
        "third_party/preact/src/options.js",
        "third_party/preact/src/util.js",
        "third_party/preact/src/constants.js",
        "third_party/preact/src/clone-element.js",
        "third_party/preact/src/create-context.js",
        "third_party/preact/src/diff/index.js",
        "third_party/preact/src/diff/children.js",
        "third_party/preact/src/diff/props.js",
        "third_party/preact/src/diff/catch-error.js",
        "third_party/preact/hooks/src/index.js",
        "third_party/preact/jsx-runtime/src/index.js",
        "third_party/preact/jsx-runtime/src/utils.js",
    };
    for (const auto* path : kOfficialPreactSources) {
        RegisterOfficialPreactSource(runtime, path);
    }
    runtime->RegisterModule("preact", BuildEmbeddedOfficialPreactModule("third_party/preact/src/index.js"));
    runtime->RegisterModule("preact/hooks", BuildEmbeddedOfficialPreactModule("third_party/preact/hooks/src/index.js"));
    runtime->RegisterModule("preact/jsx-runtime", BuildEmbeddedOfficialPreactModule("third_party/preact/jsx-runtime/src/index.js"));
    runtime->RegisterModule("preact/jsx-dev-runtime", BuildEmbeddedOfficialPreactModule("third_party/preact/jsx-runtime/src/index.js"));
}

#ifdef _WIN32
LONG WINAPI mblinkUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    reportNativeError(formatSehException("unhandled exception filter", exceptionInfo, 1));
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif


int toErrorCode(mblink::MBlinkError err) {
    switch (err) {
        case mblink::MBlinkError::Ok: return MBLINK_OK;
        case mblink::MBlinkError::InvalidHandle: return MBLINK_ERROR_INVALID_HANDLE;
        case mblink::MBlinkError::NotFound: return MBLINK_ERROR_NOT_FOUND;
        case mblink::MBlinkError::TypeMismatch: return MBLINK_ERROR_TYPE_MISMATCH;
        case mblink::MBlinkError::IndexOutOfRange: return MBLINK_ERROR_OUT_OF_RANGE;
        case mblink::MBlinkError::InvalidJson: return MBLINK_ERROR_INVALID_PARAM;
        case mblink::MBlinkError::AlreadyExists: return MBLINK_ERROR_INVALID_PARAM;
        case mblink::MBlinkError::InvalidName: return MBLINK_ERROR_INVALID_PARAM;
        default: return MBLINK_ERROR_UNKNOWN;
    }
}

MBlinkType toMBlinkType(mblink::MBlinkType type) {
    switch (type) {
        case mblink::MBlinkType::Null: return MBLINK_TYPE_NULL;
        case mblink::MBlinkType::Bool: return MBLINK_TYPE_BOOL;
        case mblink::MBlinkType::Int: return MBLINK_TYPE_INT;
        case mblink::MBlinkType::Double: return MBLINK_TYPE_DOUBLE;
        case mblink::MBlinkType::String: return MBLINK_TYPE_STRING;
        case mblink::MBlinkType::Array: return MBLINK_TYPE_ARRAY;
        case mblink::MBlinkType::Object: return MBLINK_TYPE_OBJECT;
        default: return MBLINK_TYPE_NULL;
    }
}

WindowContext* getContext(MBlinkHandle handle) {
    return reinterpret_cast<WindowContext*>(handle);
}

char* duplicateString(const std::string& str) {
    char* result = static_cast<char*>(malloc(str.size() + 1));
    if (result) {
        memcpy(result, str.c_str(), str.size() + 1);
    }
    return result;
}

char* duplicateString(const char* str) {
    if (!str) return nullptr;
    size_t len = strlen(str);
    char* result = static_cast<char*>(malloc(len + 1));
    if (result) {
        memcpy(result, str, len + 1);
    }
    return result;
}

std::string CurrentTimestampIso8601() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64] = {0};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

std::string readFileContents(const char* filepath);

void setLifecycle(WindowContext* ctx, MBlinkLifecycleState state, std::string reason) {
    if (!ctx) return;
    ctx->lifecycleState = state;
    ctx->lifecycleReason = std::move(reason);
}

std::string newRuntimeEpoch() {
    const auto tick = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return "rt-" + std::to_string(tick);
}

bool isHtmlFilePath(const std::string& path) {
    fs::path p = Utf8PathToFsPath(path);
    auto ext = FsPathToUtf8String(p.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".html" || ext == ".htm";
}

const char* lifecycleStateName(MBlinkLifecycleState state) {
    switch (state) {
        case MBLINK_LIFECYCLE_CREATED: return "created";
        case MBLINK_LIFECYCLE_LOADED: return "loaded";
        case MBLINK_LIFECYCLE_RUNNING: return "running";
        case MBLINK_LIFECYCLE_CLOSE_REQUESTED: return "close_requested";
        case MBLINK_LIFECYCLE_STOPPED: return "stopped";
        case MBLINK_LIFECYCLE_DESTROYED: return "destroyed";
    }
    return "unknown";
}

void flushRuntimeWork(WindowContext* ctx) {
    if (!ctx) return;
    if (ctx->stateManager) ctx->stateManager->processQueue();
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    if (ctx->runtime) ctx->runtime->ProcessMicrotasks();
    ctx->mainThreadQueue.flush();
    forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
        shared->flushPendingNotify();
    });
    if (ctx->runtime) ctx->runtime->ProcessMicrotasks();
}

void forceRenderFrame(WindowContext* ctx, int passes) {
    if (!ctx || !ctx->window) return;
    if (passes < 1) passes = 1;
    for (int i = 0; i < passes; ++i) {
        if (ctx->runtime) {
            ctx->runtime->RunEventLoop(1);
            ctx->runtime->ProcessMicrotasks();
        }
        if (auto* pipeline = ctx->window->GetRenderPipeline()) {
            pipeline->ForceRasterize();
        }
        ctx->window->SetNeedsRepaint();
        ctx->window->Render();
        ctx->window->SwapBuffers();
        if (ctx->runtime) {
            ctx->runtime->RunEventLoop(1);
            ctx->runtime->ProcessMicrotasks();
        }
    }
}

int loadHtmlContent(WindowContext* ctx,
                    const std::string& content,
                    const std::string& base_path,
                    bool execute_scripts) {
    if (!ctx || !ctx->document) return MBLINK_ERROR_INVALID_HANDLE;

    ctx->document->SetBasePath(base_path);
    mblink::FetchBindings::SetBasePath(base_path);
    mblink::ImageLoader::SetBasePath(base_path);
    if (!ctx->document->LoadHTML(content)) {
        setLastError("Failed to parse HTML");
        return MBLINK_ERROR_INVALID_PARAM;
    }

    ctx->document->ConsumeLoadErrors();
    std::string phase_error;
    try {
        ctx->document->LoadExternalStylesheets();
        if (execute_scripts) {
            ctx->document->ExecuteScripts();
        }
    } catch (const std::exception& e) {
        phase_error = std::string("HTML parsed but resource execution failed: ") + e.what();
    } catch (...) {
        phase_error = "HTML parsed but resource execution failed: unknown error";
    }

    std::string load_errors = ctx->document->ConsumeLoadErrors();
    if (!phase_error.empty() && !load_errors.empty()) {
        setLastError(phase_error + "\n" + load_errors);
    } else if (!phase_error.empty()) {
        setLastError(phase_error);
    } else if (!load_errors.empty()) {
        setLastError(load_errors);
    }

    if (ctx->window) {
        ctx->window->SetNeedsRepaintFor(mblink::RepaintReason::API);
    }
    setLifecycle(ctx, MBLINK_LIFECYCLE_LOADED, "loaded");
    return MBLINK_OK;
}

int loadHtmlString(WindowContext* ctx, const char* html, bool execute_scripts) {
    if (!html) return MBLINK_ERROR_INVALID_PARAM;
    return loadHtmlContent(ctx, html, "", execute_scripts);
}

int loadHtmlFile(WindowContext* ctx, const char* filepath, bool execute_scripts) {
    if (!ctx) return MBLINK_ERROR_INVALID_HANDLE;
    if (!filepath) return MBLINK_ERROR_INVALID_PARAM;

    std::string content;
    std::string base_path;

    const std::string resourcePath = JoinMountedResourcePath(
        ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
        filepath);

    if (!ctx->mountedResourcePackage.empty() && !resourcePath.empty()) {
        std::vector<uint8_t> data;
        std::string error;
        if (mblink::resourcepkg::LoadResourceFile(
                ctx->mountedResourcePackage.c_str(),
                resourcePath.c_str(),
                ctx->mountedResourceKey.c_str(),
                data,
                nullptr,
                error)) {
            content.assign(reinterpret_cast<const char*>(data.data()), data.size());
            fs::path html_dir = fs::path(NormalizeResourcePath(filepath)).parent_path();
            base_path = NormalizeResourcePath(FsPathToUtf8String(html_dir));
        }
    }

    if (content.empty()) {
        content = readFileContents(filepath);
        fs::path html_dir = fs::absolute(Utf8PathToFsPath(filepath)).parent_path();
        base_path = NormalizeFsPath(html_dir);
    }

    return loadHtmlContent(ctx, content, base_path, execute_scripts);
}

#ifdef _WIN32
HWND getOwnerHwnd(WindowContext* ctx) {
    if (!ctx || !ctx->window) return nullptr;
    auto* sdl_window = ctx->window->GetSDLWindow();
    if (!sdl_window) return nullptr;
    return static_cast<HWND>(SDL_GetPointerProperty(
        SDL_GetWindowProperties(sdl_window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr));
}
#endif

mblink::AppTrayMenuItem parseTrayMenuItem(const nlohmann::json& item) {
    mblink::AppTrayMenuItem menuItem;
    const std::string type = item.value("type", "action");
    if (type == "separator") {
        menuItem.type = mblink::AppTrayMenuItemType::Separator;
    } else if (type == "submenu") {
        menuItem.type = mblink::AppTrayMenuItemType::Submenu;
    } else {
        menuItem.type = mblink::AppTrayMenuItemType::Action;
    }

    menuItem.id = item.value("id", "");
    menuItem.label = item.value("label", "");
    menuItem.enabled = item.value("enabled", true);
    menuItem.checked = item.value("checked", false);

    if (item.contains("children") && item["children"].is_array()) {
        for (const auto& child : item["children"]) {
            menuItem.children.push_back(parseTrayMenuItem(child));
        }
    }
    return menuItem;
}

void wireTrayCallbacks(WindowContext* ctx) {
    if (!ctx || !ctx->tray) return;

    ctx->tray->SetLeftClickHandler([ctx]() {
        if (ctx->onTrayLeftClickCallback) {
            ctx->onTrayLeftClickCallback(ctx->onTrayLeftClickUserData);
        }
    });

    ctx->tray->SetMenuItemHandler([ctx](const std::string& id) {
        if (!ctx->onTrayMenuCallback) return;
        const auto payload = nlohmann::json{{"id", id}}.dump();
        char* result = ctx->onTrayMenuCallback(payload.c_str(), ctx->onTrayMenuUserData);
        if (result) {
            mblink_free(result);
        }
    });
}

void syncTrayState(WindowContext* ctx) {
    if (!ctx || !ctx->tray) return;
    ctx->tray->SetTooltip(ctx->trayTooltip);
    ctx->tray->SetMenuItems(ctx->trayMenuItems);
    wireTrayCallbacks(ctx);
}

mblink::DevToolsHostContext makeDevToolsHostContext(WindowContext* ctx) {
    mblink::DevToolsHostContext host;
    host.version = kDevToolsAttachVersion;
    if (!ctx) {
        return host;
    }
    host.window = ctx->window.get();
    host.document = ctx->document.get();
    host.runtime = ctx->runtime.get();
    host.runtime_epoch = ctx->runtimeEpoch.empty() ? nullptr : ctx->runtimeEpoch.c_str();
    host.shutdown_requested = ctx->shutdownRequested && ctx->shutdownRequested->load();
    host.host_user_data = ctx;
    return host;
}

void fillDevToolsHostServices(mblink::DevToolsHostServices* out_services) {
    if (!out_services) {
        return;
    }
    *out_services = {};
    out_services->version = kDevToolsAttachVersion;
    out_services->copy_string = mblink_copy_string;
    out_services->free_string = mblink_free;
    out_services->is_main_thread = [](const mblink::DevToolsHostContext* host) -> bool {
        if (!host || !host->host_user_data) {
            return false;
        }
        auto* ctx = static_cast<WindowContext*>(host->host_user_data);
        return ctx && ctx->mainThreadQueue.isMainThread();
    };
    out_services->run_on_main_thread_sync = RunDevToolsOnMainThreadSync;
    out_services->wake_event_loop = [](const mblink::DevToolsHostContext* host) {
        if (!host || !host->host_user_data) {
            return;
        }
        auto* ctx = static_cast<WindowContext*>(host->host_user_data);
        if (ctx && ctx->eventLoop) {
            ctx->eventLoop->Wake();
        }
    };
    out_services->set_main_thread_sync_cancelled = [](void* host_user_data, bool cancelled) {
        auto* ctx = static_cast<WindowContext*>(host_user_data);
        if (!ctx) {
            return;
        }
        if (ctx->devtoolsSyncCancelled) {
            ctx->devtoolsSyncCancelled->store(cancelled, std::memory_order_release);
        }
        if (cancelled && ctx->eventLoop) {
            ctx->eventLoop->Wake();
        }
    };
    out_services->with_current_context_sync = RunWithCurrentDevToolsContextSync;
    out_services->shutdown_requested = [](void* host_user_data) -> std::atomic<bool>* {
        auto* ctx = static_cast<WindowContext*>(host_user_data);
        return ctx && ctx->shutdownRequested ? ctx->shutdownRequested.get() : nullptr;
    };
    out_services->flush_for_snapshot = [](const mblink::DevToolsHostContext* host, int passes) -> int {
        if (!host || !host->host_user_data) {
            return MBLINK_ERROR_INVALID_PARAM;
        }
        auto* ctx = static_cast<WindowContext*>(host->host_user_data);
        if (!ctx || !ctx->shutdownRequested || ctx->shutdownRequested->load()) {
            return MBLINK_ERROR_INVALID_HANDLE;
        }
        flushRuntimeWork(ctx);
        forceRenderFrame(ctx, passes < 1 ? 1 : passes);
        flushRuntimeWork(ctx);
        return MBLINK_OK;
    };
}

#ifdef _WIN32
char* invokeCallbackWithSEH(MBlinkCallback cb,
                            const char* args,
                            void* user_data,
                            unsigned int* sehCode,
                            std::string* sehMessage) {
    if (sehCode) {
        *sehCode = 0;
    }
    if (sehMessage) {
        sehMessage->clear();
    }

    char* result = nullptr;
    __try {
        result = cb(args, user_data);
    } __except (captureSehException("bound callback", sehCode, sehMessage, GetExceptionInformation())) {
        result = nullptr;
    }
    return result;
}

bool runEventLoopWithSEH(mblink::EventLoop* eventLoop,
                         unsigned int* sehCode,
                         std::string* sehMessage) {
    if (sehCode) {
        *sehCode = 0;
    }
    if (sehMessage) {
        sehMessage->clear();
    }
    if (!eventLoop) {
        return false;
    }

    __try {
        eventLoop->Run();
        return true;
    } __except (captureSehException("mblink_run", sehCode, sehMessage, GetExceptionInformation())) {
        return false;
    }
}
#endif



std::string readFileContents(const char* filepath) {
    const fs::path fsPath = Utf8PathToFsPath(filepath ? filepath : "");
    std::ifstream file(fsPath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + (filepath ? filepath : ""));
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// 从 MBlinkConfig 构建 WindowConfig
mblink::WindowConfig buildWindowConfig(const MBlinkConfig* config) {
    mblink::WindowConfig wc;
    wc.title = config->title ? config->title : "MBlink";
    wc.width = config->width > 0 ? config->width : 800;
    wc.height = config->height > 0 ? config->height : 600;
    wc.headless = config->headless;
    wc.borderless = config->borderless;
    wc.transparent = config->transparent;
    wc.always_on_top = config->always_on_top;
    wc.resizable = config->resizable;
    wc.gpu = config->gpu;
    wc.fullscreen = config->fullscreen;
    wc.resize_border_width = config->resize_border_width > 0 ? config->resize_border_width : 8;
    wc.min_width = config->min_width;
    wc.min_height = config->min_height;
    wc.max_width = config->max_width;
    wc.max_height = config->max_height;
    return wc;
}

// 完整初始化 WindowContext
WindowContext* createWindowContext(const mblink::WindowConfig& wc) {
    auto ctx = new WindowContext();

    // 1. 创建 Window
    ctx->window = std::make_shared<mblink::Window>(wc);

    // 2. 创建 Document → 初始化基础 DOM 结构 → 设置到 Window
    ctx->document = std::make_shared<mblink::Document>();
    ctx->document->Initialize();
    ctx->window->SetDocument(ctx->document);

    // 3. 创建 TaskScheduler
    ctx->taskScheduler = std::make_shared<mblink::TaskScheduler>();
    ctx->backgroundTaskRunner = std::make_shared<mblink::BackgroundTaskRunner>();
    ctx->asyncResourceContext = std::make_shared<mblink::AsyncResourceContext>();
    ctx->mountedResourceState = std::make_shared<MountedResourceState>();
    ctx->asyncOwnerToken = reinterpret_cast<uintptr_t>(ctx);

    // 4. 注册到 WindowManager
    mblink::WindowManager::Instance().RegisterWindow(ctx->window);

    ctx->trayTooltip = wc.title;

    // 5. 创建 QuickJS Runtime
    ctx->runtime = std::make_unique<mblink::QuickJSRuntime>();
    ctx->consoleBuffer = std::make_unique<StructuredJsonBuffer>("entries", 500);
    ctx->errorBuffer = std::make_unique<StructuredJsonBuffer>("errors", 200);

    // 6. 设置 JS Runtime 到 Document
    ctx->document->SetJSRuntime(ctx->runtime.get());
    SetRuntimeBasePath(ctx, "");
    SetMountedResourceState(ctx, "", "", "/");

    auto resourceState = ctx->mountedResourceState;
    auto mountedAssetProvider = [resourceState](const std::string& path, std::vector<uint8_t>& out) {
        return LoadMountedResourceAsset(resourceState, path, out);
    };
    ctx->runtime->SetFileLoader([resourceState](const std::string& path, std::string& out, std::string* error) {
        std::vector<uint8_t> data;
        if (!LoadMountedResourceAsset(resourceState, path, data)) {
            if (error) *error = MountedResourcePackageEmpty(resourceState)
                                   ? "resource package not mounted"
                                   : "resource path outside mount point or not found";
            return false;
        }

        out.assign(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    });
    mblink::Document::SetAssetProvider(mountedAssetProvider);
    mblink::FetchBindings::SetAssetProvider(mountedAssetProvider);
    mblink::ImageLoader::SetAssetProvider(mountedAssetProvider);
    mblink::LexborStyleSheet::SetAssetProvider(mountedAssetProvider);
    ctx->asyncResourceContext->SetAssetProvider(mountedAssetProvider);

    // 7. 创建 WindowBindings + 初始化（setTimeout/setInterval/RAF/DOM/Canvas...）
    // DOM 主路径初始化统一由 WindowBindings::InitBindings() 负责，
    // 其中 BindDocumentAPIs() 会注入全局 document/window.document。
    auto jsCtx = ctx->runtime->GetContext();
    ctx->windowBindings = std::make_unique<mblink::WindowBindings>(
        ctx->runtime.get(), ctx->window, ctx->taskScheduler);
    ctx->windowBindings->InitBindings();

    // 9. 创建 EventLoop（使用同一 TaskScheduler）
    ctx->eventLoop = std::make_unique<mblink::EventLoop>(ctx->taskScheduler);
    ctx->eventLoop->SetQuickJSRuntime(ctx->runtime.get());
    ctx->mainThreadQueue.setWakeCallback([event_loop = ctx->eventLoop.get()]() {
        if (event_loop) {
            event_loop->Wake();
        }
    });
    ctx->window->SetUiTaskWakeCallback([event_loop = ctx->eventLoop.get()]() {
        if (event_loop) {
            event_loop->Wake();
        }
    });
    mblink::WindowBindings::SetActiveEventLoop(ctx->eventLoop.get());

    // 10. 创建 FetchBindings
    ctx->fetchBindings = std::make_unique<mblink::FetchBindings>(
        jsCtx, ctx->taskScheduler, ctx->backgroundTaskRunner, ctx->asyncResourceContext);
    ctx->fetchBindings->InitBindings();

    // 11. 创建 StateManager + HostBridge
#ifdef _WIN32
    SetUnhandledExceptionFilter(mblinkUnhandledExceptionFilter);
#endif

    ctx->stateManager = std::make_unique<mblink::StateManager>();
    ctx->document->SetStateManager(ctx->stateManager.get());
    ctx->hostBridge = std::make_unique<mblink::HostBridge>(
        jsCtx, ctx->stateManager.get(), ctx->backgroundTaskRunner);
    ctx->hostBridge->registerGlobal();
    mblink::ImageLoader::RegisterAsyncContext(
        ctx->asyncOwnerToken, ctx->backgroundTaskRunner, ctx->asyncResourceContext);

    ctx->runtime->RegisterFunction("__mblinkSharedNativeSet", [ctx](const nlohmann::json& args) -> nlohmann::json {
        if (!args.is_array() || args.size() < 3 || !args[0].is_string() || !args[1].is_string()) {
            return false;
        }

        auto it = ctx->sharedObjects.find(args[0].get<std::string>());
        if (it == ctx->sharedObjects.end() || !it->second) {
            return false;
        }

        return it->second->safeSetPropertyFromJS(args[1].get<std::string>().c_str(), args[2]);
    });

    ctx->runtime->RegisterFunction("__mblinkSharedNativeDelete", [ctx](const nlohmann::json& args) -> nlohmann::json {
        if (!args.is_array() || args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
            return false;
        }

        auto it = ctx->sharedObjects.find(args[0].get<std::string>());
        if (it == ctx->sharedObjects.end() || !it->second) {
            return false;
        }

        return it->second->safeDeletePropertyFromJS(args[1].get<std::string>().c_str());
    });


    ctx->runtimeEpoch = newRuntimeEpoch();
    ctx->runtime->SetConsoleCallback([ctx](const nlohmann::json& entry) {
        if (ctx->consoleBuffer) ctx->consoleBuffer->Push(entry);
        if (ctx->observeCallback) {
            const auto payload = entry.dump();
            ctx->observeCallback(MBLINK_OBSERVE_CONSOLE, payload.c_str(), ctx->observeUserData);
        }
    });
    ctx->runtime->SetErrorCallback([ctx](const nlohmann::json& entry) {
        if (ctx->errorBuffer) ctx->errorBuffer->Push(entry);
        if (ctx->observeCallback) {
            const auto payload = entry.dump();
            ctx->observeCallback(MBLINK_OBSERVE_ERROR, payload.c_str(), ctx->observeUserData);
        }
    });
    ctx->window->SetOnCloseCallback([ctx]() {
        ctx->shutdownRequested->store(true);
        setLifecycle(ctx, MBLINK_LIFECYCLE_STOPPED, "user_closed");
        if (ctx->onCloseCallback) {
            ctx->onCloseCallback(ctx->onCloseUserData);
        }
        if (ctx->eventLoop) {
            ctx->eventLoop->Stop();
        }
    });

    return ctx;
}

} // anonymous namespace

// ========== 生命周期 ==========

int mblink_init(void) {
    if (g_initialized) return MBLINK_OK;
    g_initialized = true;
    return MBLINK_OK;
}

int mblink_devtools_host_attach(unsigned int version,
                               const mblink::DevToolsBridgeApi* api) {
    if (version != kDevToolsAttachVersion || !api ||
        api->version != kDevToolsAttachVersion) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    return mblink::RegisterDevToolsBridge(api) ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

void mblink_devtools_host_detach(unsigned int version,
                                const mblink::DevToolsBridgeApi* api) {
    if (version == kDevToolsAttachVersion) {
        mblink::UnregisterDevToolsBridge(api);
    }
}

int mblink_devtools_host_get_services(unsigned int version,
                                     mblink::DevToolsHostServices* out_services) {
    if (version != kDevToolsAttachVersion || !out_services) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    fillDevToolsHostServices(out_services);
    return MBLINK_OK;
}

void mblink_cleanup(void) {
    g_initialized = false;
}

const char* mblink_version(void) {
    return "0.1.0";
}

// ========== 窗口管理 ==========

MBlinkHandle mblink_create(const char* title, int width, int height) {
    if (!g_initialized) {
        setLastError("MBlink not initialized");
        return nullptr;
    }

    try {
        mblink::WindowConfig wc;
        wc.title = title ? title : "MBlink";
        wc.width = width > 0 ? width : 800;
        wc.height = height > 0 ? height : 600;
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<MBlinkHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

MBlinkHandle mblink_create_ex(const MBlinkConfig* config) {
    if (!g_initialized) {
        setLastError("MBlink not initialized");
        return nullptr;
    }
    if (!config) {
        setLastError("config is null");
        return nullptr;
    }

    try {
        auto wc = buildWindowConfig(config);
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<MBlinkHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

MBlinkConfig mblink_default_config(void) {
    MBlinkConfig config = {};
    config.title = "MBlink";
    config.width = 800;
    config.height = 600;
    config.headless = false;
    config.borderless = false;
    config.transparent = false;
    config.always_on_top = false;
    config.resizable = true;
    config.gpu = true;
    config.fullscreen = false;
    config.resize_border_width = 8;
    config.min_width = 0;
    config.min_height = 0;
    config.max_width = 0;
    config.max_height = 0;
    return config;
}

void mblink_destroy(MBlinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    setLifecycle(ctx, MBLINK_LIFECYCLE_DESTROYED, "destroyed");

    SAFE_CLEANUP("devtools_shutdown", {
        auto host = makeDevToolsHostContext(ctx);
        mblink::DevToolsShutdown(host);
    });

    if (ctx->shutdownRequested) {
        ctx->shutdownRequested->store(true, std::memory_order_release);
    }
    if (ctx->devtoolsSyncCancelled) {
        ctx->devtoolsSyncCancelled->store(true, std::memory_order_release);
    }

    // 0. 先销毁 tray，避免后续窗口销毁时残留托盘图标
    SAFE_CLEANUP("destroy_tray", if (ctx->tray) { ctx->tray->Destroy(); ctx->tray.reset(); });

    // 1. 先隐藏窗口，避免后续重清理阶段造成用户可见卡顿
    SAFE_CLEANUP("hide_window", if (ctx->window) { ctx->window->Hide(); });

    // 2. 停止事件循环
    SAFE_CLEANUP("stop_event_loop", if (ctx->eventLoop && ctx->running) {
        ctx->eventLoop->Stop();
        ctx->running = false;
    });


    // 2. 先解绑 backend/py 上已注册的宿主函数，避免函数对象在关闭时仍被全局对象持有
    SAFE_CLEANUP("unbind_host_functions", if (ctx->hostBridge) {
        std::vector<std::string> boundNames;
        boundNames.reserve(ctx->boundFunctions.size() + ctx->boundAsyncFunctions.size());
        for (const auto& [name, _] : ctx->boundFunctions) {
            boundNames.push_back(name);
        }
        for (const auto& [name, _] : ctx->boundAsyncFunctions) {
            if (std::find(boundNames.begin(), boundNames.end(), name) == boundNames.end()) {
                boundNames.push_back(name);
            }
        }
        for (const auto& name : boundNames) {
            ctx->hostBridge->unbind(name);
        }
    });
    ctx->boundFunctions.clear();
    ctx->boundAsyncFunctions.clear();

    // 3. JS framework cleanup：必须在 scheduler 仍存活时执行
    SAFE_CLEANUP("cancel_pending_promises", if (ctx->hostBridge) {
        ctx->hostBridge->cancelPendingPromises("Window destroyed");
    });

    SAFE_CLEANUP("pre_shutdown_microtasks", if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    });

    SAFE_CLEANUP("js_shutdown", if (ctx->runtime) {
        auto jsCtx = ctx->runtime->GetContext();
        const char* shutdownScript =
            "(function(){"
            "if(typeof __mblinkShutdown==='function'){__mblinkShutdown();}"
            "if(typeof __fetchCleanup==='function'){__fetchCleanup();}"
            "})();";
        JSValue res = JS_Eval(jsCtx, shutdownScript, strlen(shutdownScript),
                              "<shutdown>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(res)) {
            JSValue exc = JS_GetException(jsCtx);
            JS_FreeValue(jsCtx, exc);
        }
        JS_FreeValue(jsCtx, res);
    });

    SAFE_CLEANUP("post_shutdown_microtasks", if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    });


    // 不要在销毁路径手动 flush mainThreadQueue。
    // 队列中的残留任务会在 WindowContext 析构后因 alive flag 失效，
    // 避免 shutdown 过程中再执行额外的 JS/DOM 操作导致 QuickJS 残留对象。

    // 4. 停止任务源
    SAFE_CLEANUP("fetch_begin_shutdown", if (ctx->fetchBindings) {
        ctx->fetchBindings->BeginShutdown();
    });

    SAFE_CLEANUP("mark_async_resources_dead", {
        if (ctx->asyncResourceContext) {
            ctx->asyncResourceContext->MarkDead();
        }
        if (ctx->mountedResourceState) {
            ctx->mountedResourceState->alive = false;
        }
    });

    SAFE_CLEANUP("unregister_image_async_context", {
        if (ctx->asyncOwnerToken != 0) {
            mblink::ImageLoader::UnregisterAsyncContext(ctx->asyncOwnerToken);
        }
    });

    SAFE_CLEANUP("shutdown_background_runner", if (ctx->backgroundTaskRunner) {
        ctx->backgroundTaskRunner->Shutdown();
    });

    SAFE_CLEANUP("shutdown_task_scheduler", if (ctx->taskScheduler) {
        ctx->taskScheduler->Shutdown();
        ctx->taskScheduler->ClearAllTasks();
    });

    SAFE_CLEANUP("final_microtasks", if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    });

    // 5. 释放 SharedObject
    for (const auto& name : collectSharedObjectNames(ctx)) {
        auto it = ctx->sharedObjects.find(name);
        if (it != ctx->sharedObjects.end() && it->second) {
            SAFE_CLEANUP("destroy_shared_object", destroySharedObjectInternal(it->second, true));
        }
    }

    // 6. 清理 DOM 绑定
    SAFE_CLEANUP("clear_document_element_listeners", if (ctx->document) {
        ClearDocumentElementListeners(ctx->document);
    });

    SAFE_CLEANUP("window_bindings_cleanup", if (ctx->windowBindings) {
        ctx->windowBindings->Cleanup();
    });

    SAFE_CLEANUP("dom_bindings_legacy_cleanup", {
        mblink::DOMBindings::Cleanup(nullptr);
    });

    SAFE_CLEANUP("detach_window_document", if (ctx->window) {
        ctx->window->SetDocument(nullptr);
    });
    ctx->document.reset();

    // 7. 释放 HostBridge 和 StateManager
    SAFE_CLEANUP("clear_watchers", if (ctx->stateManager) {
        ctx->stateManager->clearWatchers();
    });

    ctx->hostBridge.reset();
    ctx->stateManager.reset();


    // 8. GC + Runtime
    SAFE_CLEANUP("run_gc", if (ctx->runtime) {
        ctx->runtime->RunGC();
    });

    ctx->runtime.reset();

    // 9. 释放 native 资源
    ctx->mainThreadQueue.setWakeCallback({});
    if (ctx->window) {
        ctx->window->SetUiTaskWakeCallback({});
    }
    ctx->eventLoop.reset();
    ctx->windowBindings.reset();
    ctx->fetchBindings.reset();
    ctx->taskScheduler.reset();

    SAFE_CLEANUP("unregister_window", if (ctx->window) {
        mblink::WindowManager::Instance().UnregisterWindow(ctx->window);
    });

    ctx->watchCallbacks.clear();
    ctx->boundFunctions.clear();
    ctx->boundAsyncFunctions.clear();

    ctx->window.reset();

    delete ctx;
}

void mblink_run(MBlinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return;
    if (ctx->running) return;

    ctx->running = true;
    setLifecycle(ctx, MBLINK_LIFECYCLE_RUNNING, "running");

    // 设置 update callback：处理 StateManager 队列 + HostBridge 事件 + MainThreadQueue + SharedObject 延迟通知 + 用户回调
    ctx->eventLoop->SetUpdateCallback([ctx](float dt) {
        // 处理状态变更队列
        if (ctx->stateManager) {
            ctx->stateManager->processQueue();
        }
        // 刷新 HostBridge 事件队列到 JS 端
        if (ctx->hostBridge) {
            ctx->hostBridge->flushEvents();
            ctx->hostBridge->flushAsyncResults();
        }
        if (ctx->runtime) {
            ctx->runtime->ProcessMicrotasks();
        }
        // 刷新主线程任务队列，确保所有跨线程 JS/DOM 操作都在主线程执行
        ctx->mainThreadQueue.flush();
        // 刷新所有 SharedObject 的延迟通知（由 notifyUpdate/flushBatch 标记的 pending_notify_）
        forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
            shared->flushPendingNotify();
        });
        if (ctx->runtime) {
            ctx->runtime->ProcessMicrotasks();
        }
        // 调用用户的 update 回调
        if (ctx->onUpdateCallback) {
            ctx->onUpdateCallback(dt, ctx->onUpdateUserData);
        }
    });

    // 阻塞运行事件循环
    ensureInitialFrameWarmup(ctx);

#ifdef _WIN32
    unsigned int sehCode = 0;
    std::string sehMessage;
    if (!runEventLoopWithSEH(ctx->eventLoop.get(), &sehCode, &sehMessage) && sehCode != 0) {
        reportNativeError(sehMessage.empty()
            ? ("SEH exception in mblink_run, code=0x" + formatHexValue(sehCode, 8))
            : sehMessage);
    }
#else
    ctx->eventLoop->Run();
#endif
    ctx->running = false;
    if (ctx->lifecycleState == MBLINK_LIFECYCLE_RUNNING) {
        setLifecycle(ctx, MBLINK_LIFECYCLE_STOPPED, "event_loop_stopped");
    }
}

void mblink_stop(MBlinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->shutdownRequested->store(true);
    ctx->running = false;
    setLifecycle(ctx, MBLINK_LIFECYCLE_STOPPED, "stop_requested");
    if (ctx->eventLoop) {
        ctx->eventLoop->Stop();
    }
}

bool mblink_poll_events(MBlinkHandle handle) {
    if (!handle) return false;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return false;
    if (!ctx->running && ctx->lifecycleState != MBLINK_LIFECYCLE_STOPPED) {
        ctx->running = true;
        setLifecycle(ctx, MBLINK_LIFECYCLE_RUNNING, "running");
    }

    ensureInitialFrameWarmup(ctx);
    flushRuntimeWork(ctx);

    ctx->eventLoop->RunOnceNonBlocking();

    const bool alive = !ctx->eventLoop->ShouldQuit();
    if (!alive && ctx->lifecycleState == MBLINK_LIFECYCLE_RUNNING) {
        ctx->running = false;
        setLifecycle(ctx, MBLINK_LIFECYCLE_STOPPED, "event_loop_stopped");
    }
    return alive;
}

// ========== 窗口属性 ==========

bool mblink_wait_events(MBlinkHandle handle) {
    if (!handle) return false;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return false;
    if (!ctx->running && ctx->lifecycleState != MBLINK_LIFECYCLE_STOPPED) {
        ctx->running = true;
        setLifecycle(ctx, MBLINK_LIFECYCLE_RUNNING, "running");
    }

    flushRuntimeWork(ctx);

    ctx->eventLoop->RunOnce();

    const bool alive = !ctx->eventLoop->ShouldQuit();
    if (!alive && ctx->lifecycleState == MBLINK_LIFECYCLE_RUNNING) {
        ctx->running = false;
        setLifecycle(ctx, MBLINK_LIFECYCLE_STOPPED, "event_loop_stopped");
    }
    return alive;
}

MBlinkRuntimeOptions mblink_default_runtime_options(void) {
    MBlinkRuntimeOptions options = {};
    options.runtime_epoch = nullptr;
    options.load_embedded_runtime = true;
    options.load_official_preact = true;
    return options;
}

int mblink_configure_runtime(MBlinkHandle handle, const MBlinkRuntimeOptions* options) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!options) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    ctx->runtimeEpoch = options->runtime_epoch && *options->runtime_epoch
                            ? options->runtime_epoch
                            : newRuntimeEpoch();
    if (options->load_embedded_runtime) {
        return mblink_load_embedded_runtime(handle, options->load_official_preact);
    }
    return MBLINK_OK;
}

int mblink_load_embedded_runtime(MBlinkHandle handle, bool include_official_preact) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBLINK_ERROR_INVALID_HANDLE;
    try {
        if (!ctx->embeddedRuntimeLoaded) {
            loadEmbeddedRuntimeScripts(ctx->runtime.get());
            ctx->embeddedRuntimeLoaded = true;
        }
        if (include_official_preact && !ctx->officialPreactLoaded) {
            registerPreactModules(ctx->runtime.get());
            ctx->officialPreactLoaded = true;
        }
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_JS_ERROR;
    }
}

int mblink_load_module_file(MBlinkHandle handle, const char* entry_path) {
    return mblink_load_js_file(handle, entry_path);
}

int mblink_load_entry_file(MBlinkHandle handle, const char* entry_path,
                          bool execute_html_scripts) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!entry_path) return MBLINK_ERROR_INVALID_PARAM;
    const int rc = isHtmlFilePath(entry_path)
                       ? loadHtmlFile(getContext(handle), entry_path, execute_html_scripts)
                       : mblink_load_js_file(handle, entry_path);
    if (rc == MBLINK_OK) {
        setLifecycle(getContext(handle), MBLINK_LIFECYCLE_LOADED, "loaded");
    }
    return rc;
}

int mblink_render_frame(MBlinkHandle handle, int passes) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    forceRenderFrame(getContext(handle), passes);
    return MBLINK_OK;
}

int mblink_runtime_epoch(MBlinkHandle handle, char** out_epoch) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!out_epoch) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    *out_epoch = duplicateString(ctx->runtimeEpoch);
    return *out_epoch ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

MBlinkLifecycleState mblink_lifecycle_state(MBlinkHandle handle) {
    if (!handle) return MBLINK_LIFECYCLE_DESTROYED;
    return getContext(handle)->lifecycleState;
}

int mblink_lifecycle_reason(MBlinkHandle handle, char** out_reason) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!out_reason) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    *out_reason = duplicateString(ctx->lifecycleReason);
    return *out_reason ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

int mblink_set_title(MBlinkHandle handle, const char* title) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (title && ctx->window) {
        ctx->window->SetTitle(title);
    }
    return MBLINK_OK;
}

int mblink_tray_create(MBlinkHandle handle, const char* tooltip) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (!ctx->window) return MBLINK_ERROR_INVALID_HANDLE;

#ifndef _WIN32
    (void)tooltip;
    return MBLINK_ERROR_NOT_FOUND;
#else
    if (tooltip) {
        ctx->trayTooltip = tooltip;
    }

    mblink::AppTrayConfig trayConfig;
    trayConfig.tooltip = ctx->trayTooltip;
    trayConfig.owner_native_window = getOwnerHwnd(ctx);

    if (!ctx->tray) {
        ctx->tray = mblink::AppTray::CreateForPlatform(trayConfig);
        if (!ctx->tray) {
            return MBLINK_ERROR_NOT_FOUND;
        }
    }

    syncTrayState(ctx);
    return ctx->tray->Create() ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
#endif
}

int mblink_tray_destroy(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->tray) {
        ctx->tray->Destroy();
        ctx->tray.reset();
    }
    return MBLINK_OK;
}

int mblink_tray_set_tooltip(MBlinkHandle handle, const char* tooltip) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!tooltip) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    ctx->trayTooltip = tooltip;
    if (ctx->tray) {
        ctx->tray->SetTooltip(ctx->trayTooltip);
    }
    return MBLINK_OK;
}

int mblink_tray_set_menu(MBlinkHandle handle, const char* menu_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!menu_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);

    try {
        auto json = nlohmann::json::parse(menu_json);
        if (!json.is_array()) {
            return MBLINK_ERROR_INVALID_PARAM;
        }

        std::vector<mblink::AppTrayMenuItem> items;
        items.reserve(json.size());
        for (const auto& item : json) {
            if (!item.is_object()) {
                return MBLINK_ERROR_INVALID_PARAM;
            }
            items.push_back(parseTrayMenuItem(item));
        }

        ctx->trayMenuItems = std::move(items);
        if (ctx->tray) {
            ctx->tray->SetMenuItems(ctx->trayMenuItems);
        }
        return MBLINK_OK;
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_tray_set_left_click_callback(MBlinkHandle handle,
                                       MBlinkVoidCallback callback,
                                       void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onTrayLeftClickCallback = callback;
    ctx->onTrayLeftClickUserData = user_data;
    if (ctx->tray) {
        wireTrayCallbacks(ctx);
    }
    return MBLINK_OK;
}

int mblink_tray_set_menu_callback(MBlinkHandle handle,
                                 MBlinkCallback callback,
                                 void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onTrayMenuCallback = callback;
    ctx->onTrayMenuUserData = user_data;
    if (ctx->tray) {
        wireTrayCallbacks(ctx);
    }
    return MBLINK_OK;
}

int mblink_set_size(MBlinkHandle handle, int width, int height) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetSize(width, height);
    }
    return MBLINK_OK;
}

int mblink_get_size(MBlinkHandle handle, int* width, int* height) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int w = 0, h = 0;
        ctx->window->GetSize(&w, &h);
        if (width) *width = w;
        if (height) *height = h;
    }
    return MBLINK_OK;
}

int mblink_set_position(MBlinkHandle handle, int x, int y) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetPosition(x, y);
    }
    return MBLINK_OK;
}

int mblink_get_position(MBlinkHandle handle, int* x, int* y) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int px = 0, py = 0;
        ctx->window->GetPosition(&px, &py);
        if (x) *x = px;
        if (y) *y = py;
    }
    return MBLINK_OK;
}

int mblink_set_min_size(MBlinkHandle handle, int width, int height) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMinSize(width, height);
    }
    return MBLINK_OK;
}

int mblink_set_max_size(MBlinkHandle handle, int width, int height) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMaxSize(width, height);
    }
    return MBLINK_OK;
}

int mblink_minimize(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Minimize();
    return MBLINK_OK;
}

int mblink_maximize(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Maximize();
    return MBLINK_OK;
}

int mblink_restore(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Restore();
    return MBLINK_OK;
}

int mblink_show(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Show();
    return MBLINK_OK;
}

int mblink_hide(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Hide();
    return MBLINK_OK;
}

int mblink_set_fullscreen(MBlinkHandle handle, bool fullscreen) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetFullscreen(fullscreen);
    return MBLINK_OK;
}

int mblink_set_resizable(MBlinkHandle handle, bool resizable) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetResizable(resizable);
    return MBLINK_OK;
}

int mblink_set_borderless(MBlinkHandle handle, bool borderless) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetBorderless(borderless);
    return MBLINK_OK;
}

int mblink_set_always_on_top(MBlinkHandle handle, bool on_top) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetAlwaysOnTop(on_top);
    return MBLINK_OK;
}

// ========== UI 加载 ==========

int mblink_load_html(MBlinkHandle handle, const char* html) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!html) return MBLINK_ERROR_INVALID_PARAM;

    try {
        return loadHtmlString(getContext(handle), html, true);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_load_html_file(MBlinkHandle handle, const char* filepath) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!filepath) return MBLINK_ERROR_INVALID_PARAM;
    try {
        return loadHtmlFile(getContext(handle), filepath, true);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_eval_js(MBlinkHandle handle, const char* js_code) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!js_code) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBLINK_ERROR_INVALID_HANDLE;

    try {
        ctx->runtime->Eval(js_code);
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_JS_ERROR;
    }
}

int mblink_eval_module(MBlinkHandle handle, const char* code, const char* filename) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!code) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBLINK_ERROR_INVALID_HANDLE;

    try {
        const char* fname = filename ? filename : "<module>";
        ctx->runtime->EvalModule(code, fname);
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_JS_ERROR;
    }
}

int mblink_load_js_file(MBlinkHandle handle, const char* filepath) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!filepath) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBLINK_ERROR_INVALID_HANDLE;

    try {
        std::string path = filepath;
        std::string next_base_path;
        if (!ctx->mountedResourcePackage.empty()) {
            const std::string resourcePath = JoinMountedResourcePath(
                ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
                filepath);
            if (!resourcePath.empty()) {
                std::vector<uint8_t> data;
                uint32_t flags = 0;
                std::string error;
                if (mblink::resourcepkg::LoadResourceFile(
                        ctx->mountedResourcePackage.c_str(),
                        resourcePath.c_str(),
                        ctx->mountedResourceKey.c_str(),
                        data,
                        &flags,
                        error)) {
                    const fs::path module_dir = Utf8PathToFsPath(NormalizeResourcePath(filepath)).parent_path();
                    next_base_path = NormalizeResourcePath(FsPathToUtf8String(module_dir));
                    if ((flags & mblink::resourcepkg::kResourceFlagBytecode) != 0) {
                        auto jsCtx = ctx->runtime->GetContext();
                        SetRuntimeBasePath(ctx, next_base_path);
                        if (!mblink::resourcepkg::EvalMaybeMergedBytecode(jsCtx, data.data(), data.size(), error)) {
                            setLastError(error.empty() ? "Failed to eval resource bytecode" : error);
                            return MBLINK_ERROR_JS_ERROR;
                        }
                        if (ctx->window) {
                            ctx->window->SetNeedsRepaintFor(mblink::RepaintReason::API);
                        }
                        setLifecycle(ctx, MBLINK_LIFECYCLE_LOADED, "loaded");
                        return MBLINK_OK;
                    }
                    path = NormalizeResourcePath(filepath);
                }
            }
        }
        if (next_base_path.empty()) {
            fs::path module_dir;
            if (!path.empty() && path.front() == '/') {
                module_dir = Utf8PathToFsPath(NormalizeResourcePath(path)).parent_path();
                next_base_path = NormalizeResourcePath(FsPathToUtf8String(module_dir));
            } else {
                module_dir = fs::absolute(Utf8PathToFsPath(path)).parent_path();
                next_base_path = NormalizeFsPath(module_dir);
            }
        }
        SetRuntimeBasePath(ctx, next_base_path);
        ctx->runtime->LoadModuleFile(path);
        if (ctx->window) {
            ctx->window->SetNeedsRepaintFor(mblink::RepaintReason::API);
        }
        setLifecycle(ctx, MBLINK_LIFECYCLE_LOADED, "loaded");
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_JS_ERROR;
    }
}

int mblink_load_bytecode(MBlinkHandle handle, const void* data, size_t size) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!data || size == 0) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBLINK_ERROR_INVALID_HANDLE;

    try {
        auto jsCtx = ctx->runtime->GetContext();
        std::string error;
        if (!mblink::resourcepkg::EvalMaybeMergedBytecode(jsCtx, data, size, error)) {
            setLastError(error.empty() ? "Failed to eval bytecode" : error);
            return MBLINK_ERROR_JS_ERROR;
        }
        setLifecycle(ctx, MBLINK_LIFECYCLE_LOADED, "loaded");
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_JS_ERROR;
    }
}

// ========== 函数绑定 ==========

int mblink_bind(MBlinkHandle handle, const char* name,
                 MBlinkCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !callback) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    ctx->boundFunctions[name] = {callback, user_data};

    // 通过 HostBridge 注册，这样 JS 端可以通过 backend.name() 调用
    if (ctx->hostBridge) {
        MBlinkCallback cb = callback;
        void* ud = user_data;
        std::string funcName = name;
        ctx->hostBridge->bind(name, [ctx, cb, ud, funcName](const std::string& args) -> std::string {
            return invokeBoundJsonCallback(ctx, funcName, args, cb, ud);
        });
    }
    return MBLINK_OK;
}

int mblink_bind_async(MBlinkHandle handle, const char* name,
                     MBlinkAsyncCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !callback) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    ctx->boundAsyncFunctions[name] = {callback, user_data};

    if (ctx->hostBridge) {
        MBlinkAsyncCallback cb = callback;
        void* ud = user_data;
        std::string funcName = name;
        ctx->hostBridge->bindAsync(name, [ctx, cb, ud, funcName](const std::string& args) -> std::string {
            return invokeBoundJsonCallback(ctx, funcName, args, cb, ud);
        });
    }
    return MBLINK_OK;
}

void mblink_unbind(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->boundFunctions.erase(name);
    ctx->boundAsyncFunctions.erase(name);
    if (ctx->hostBridge) {
        ctx->hostBridge->unbind(name);
    }
}

// ========== 事件回调 ==========

int mblink_on_resize(MBlinkHandle handle, MBlinkResizeCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onResizeCallback = callback;
    ctx->onResizeUserData = user_data;
    if (ctx->window) {
        auto cb = callback;
        auto ud = user_data;
        ctx->window->SetOnResizeCallback([cb, ud](int w, int h) {
            if (cb) cb(w, h, ud);
        });
    }
    return MBLINK_OK;
}

int mblink_on_close(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onCloseCallback = callback;
    ctx->onCloseUserData = user_data;
    if (ctx->window) {
        auto cb = callback;
        auto ud = user_data;
        ctx->window->SetOnCloseCallback([cb, ud]() {
            if (cb) cb(ud);
        });
    }
    return MBLINK_OK;
}

int mblink_on_close_request(MBlinkHandle handle, MBlinkBoolCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onCloseRequestCallback = callback;
    ctx->onCloseRequestUserData = user_data;
    if (ctx->window) {
        auto cb = callback;
        auto ud = user_data;
        ctx->window->SetOnCloseRequestHandler([cb, ud]() -> bool {
            return cb ? cb(ud) : false;
        });
    }
    return MBLINK_OK;
}

int mblink_on_focus(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onFocusCallback = callback;
    ctx->onFocusUserData = user_data;
    if (ctx->window) {
        auto cb = callback;
        auto ud = user_data;
        ctx->window->SetOnFocusCallback([cb, ud]() {
            if (cb) cb(ud);
        });
    }
    return MBLINK_OK;
}

int mblink_on_blur(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onBlurCallback = callback;
    ctx->onBlurUserData = user_data;
    if (ctx->window) {
        auto cb = callback;
        auto ud = user_data;
        ctx->window->SetOnBlurCallback([cb, ud]() {
            if (cb) cb(ud);
        });
    }
    return MBLINK_OK;
}

int mblink_on_update(MBlinkHandle handle, MBlinkUpdateCallback callback, void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onUpdateCallback = callback;
    ctx->onUpdateUserData = user_data;
    return MBLINK_OK;
}

// ========== 事件发送 ==========

int mblink_emit(MBlinkHandle handle, const char* event_name, const char* data_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!event_name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (ctx->hostBridge) {
        ctx->hostBridge->emit(event_name, data_json ? data_json : "null");
    }
    return MBLINK_OK;
}

int mblink_observe_set_callback(MBlinkHandle handle,
                               MBlinkObserveCallback callback,
                               void* user_data) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->observeCallback = callback;
    ctx->observeUserData = user_data;
    return MBLINK_OK;
}

int mblink_observe_console_json(MBlinkHandle handle, char** out_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!out_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    auto payload = (ctx->consoleBuffer ? ctx->consoleBuffer->ToJson()
                                       : nlohmann::json{{"entries", nlohmann::json::array()}}).dump();
    *out_json = duplicateString(payload);
    return *out_json ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

int mblink_observe_errors_json(MBlinkHandle handle, char** out_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!out_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    auto payload = (ctx->errorBuffer ? ctx->errorBuffer->ToJson()
                                     : nlohmann::json{{"errors", nlohmann::json::array()}}).dump();
    *out_json = duplicateString(payload);
    return *out_json ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

int mblink_observe_lifecycle_json(MBlinkHandle handle, char** out_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!out_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    nlohmann::json payload{{"ok", true},
                           {"status", lifecycleStateName(ctx->lifecycleState)},
                           {"reason", ctx->lifecycleReason},
                           {"runtime_epoch", ctx->runtimeEpoch},
                           {"timestamp", CurrentTimestampIso8601()}};
    *out_json = duplicateString(payload.dump());
    return *out_json ? MBLINK_OK : MBLINK_ERROR_UNKNOWN;
}

int mblink_observe_clear(MBlinkHandle handle, MBlinkObserveKind kind) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (kind == MBLINK_OBSERVE_CONSOLE) {
        if (ctx->consoleBuffer) ctx->consoleBuffer->Clear();
    } else if (kind == MBLINK_OBSERVE_ERROR) {
        if (ctx->errorBuffer) ctx->errorBuffer->Clear();
    } else {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    return MBLINK_OK;
}

int mblink_state_create_null(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createNull(name));
}

int mblink_state_create_bool(MBlinkHandle handle, const char* name, bool value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createBool(name, value));
}

int mblink_state_create_int(MBlinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createInt(name, value));
}

int mblink_state_create_double(MBlinkHandle handle, const char* name, double value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createDouble(name, value));
}

int mblink_state_create_string(MBlinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createString(name, value ? value : ""));
}

int mblink_state_create_array(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createArray(name));
}

int mblink_state_create_object(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createObject(name));
}

int mblink_state_create_json(MBlinkHandle handle, const char* name, const char* json_str) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(json_str);
        return toErrorCode(ctx->stateManager->createJson(name, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}


// ========== 状态查询 ==========

bool mblink_state_exists(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->exists(name);
}

MBlinkType mblink_state_type(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return MBLINK_TYPE_NULL;
    auto ctx = getContext(handle);
    return toMBlinkType(ctx->stateManager->type(name));
}

void mblink_state_delete(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->stateManager->remove(name);
    ctx->stateManager->processQueue();
}

// ========== 状态读取（直接类型） ==========

bool mblink_state_get_bool(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->getBool(name);
}

int64_t mblink_state_get_int(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getInt(name);
}

double mblink_state_get_double(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return 0.0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getDouble(name);
}

const char* mblink_state_get_string(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return "";
    auto ctx = getContext(handle);
    return ctx->stateManager->getString(name).c_str();
}

int mblink_state_get_length(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->getLength(name));
}

// ========== 状态读取（JSON） ==========

char* mblink_state_get_json(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getJson(name);
    return duplicateString(j.dump());
}

char* mblink_state_get_at(MBlinkHandle handle, const char* name, int index) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getAt(name, index);
    return duplicateString(j.dump());
}

char* mblink_state_get_key(MBlinkHandle handle, const char* name, const char* key) {
    if (!handle || !name || !key) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getKey(name, key);
    return duplicateString(j.dump());
}

// ========== 状态写入 ==========

int mblink_state_set_null(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setNull(name));
}

int mblink_state_set_bool(MBlinkHandle handle, const char* name, bool value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setBool(name, value));
}

int mblink_state_set_int(MBlinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setInt(name, value));
}

int mblink_state_set_double(MBlinkHandle handle, const char* name, double value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setDouble(name, value));
}

int mblink_state_set_string(MBlinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setString(name, value ? value : ""));
}

int mblink_state_set_json(MBlinkHandle handle, const char* name, const char* json_str) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(json_str);
        return toErrorCode(ctx->stateManager->setJson(name, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}


// ========== 数组操作 ==========

int mblink_state_array_push(MBlinkHandle handle, const char* name, const char* item_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayPush(name, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_state_array_push_int(MBlinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mblink_state_array_push_double(MBlinkHandle handle, const char* name, double value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mblink_state_array_push_string(MBlinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value ? value : ""));
}

int mblink_state_array_push_bool(MBlinkHandle handle, const char* name, bool value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mblink_state_array_pop(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPop(name));
}

int mblink_state_array_shift(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayShift(name));
}

int mblink_state_array_unshift(MBlinkHandle handle, const char* name, const char* item_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayUnshift(name, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_state_array_remove(MBlinkHandle handle, const char* name, int index) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayRemove(name, index));
}

int mblink_state_array_clear(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayClear(name));
}

int mblink_state_array_set(MBlinkHandle handle, const char* name, int index, const char* item_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arraySet(name, index, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_state_array_set_int(MBlinkHandle handle, const char* name, int index, int64_t value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int mblink_state_array_set_double(MBlinkHandle handle, const char* name, int index, double value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int mblink_state_array_set_string(MBlinkHandle handle, const char* name, int index, const char* value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value ? value : ""));
}


// ========== 对象操作 ==========

int mblink_state_object_set(MBlinkHandle handle, const char* name, const char* key, const char* value_json) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key || !value_json) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mblink::json::parse(value_json);
        return toErrorCode(ctx->stateManager->objectSet(name, key, j));
    } catch (...) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
}

int mblink_state_object_set_int(MBlinkHandle handle, const char* name, const char* key, int64_t value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mblink_state_object_set_double(MBlinkHandle handle, const char* name, const char* key, double value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mblink_state_object_set_string(MBlinkHandle handle, const char* name, const char* key, const char* value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value ? value : ""));
}

int mblink_state_object_set_bool(MBlinkHandle handle, const char* name, const char* key, bool value) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mblink_state_object_remove(MBlinkHandle handle, const char* name, const char* key) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectRemove(name, key));
}

int mblink_state_object_clear(MBlinkHandle handle, const char* name) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectClear(name));
}

// ========== 数值操作 ==========

int mblink_state_increment(MBlinkHandle handle, const char* name, double delta) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->increment(name, delta));
}

int mblink_state_multiply(MBlinkHandle handle, const char* name, double factor) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->multiply(name, factor));
}

// ========== 字符串操作 ==========

int mblink_state_string_append(MBlinkHandle handle, const char* name, const char* suffix) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !suffix) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringAppend(name, suffix));
}

int mblink_state_string_prepend(MBlinkHandle handle, const char* name, const char* prefix) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!name || !prefix) return MBLINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringPrepend(name, prefix));
}

// ========== 监听 ==========

int mblink_state_watch(MBlinkHandle handle, const char* name,
                        MBlinkStateCallback callback, void* user_data) {
    if (!handle) return -1;
    if (!name || !callback) return -1;

    auto ctx = getContext(handle);
    int watchId = ctx->stateManager->watch(name,
        [callback, user_data](const std::string& n, const mblink::json& v) {
            std::string jsonStr = v.dump();
            callback(n.c_str(), jsonStr.c_str(), user_data);
        });

    ctx->watchCallbacks[watchId] = {callback, user_data};
    return watchId;
}

void mblink_state_unwatch(MBlinkHandle handle, int watch_id) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->unwatch(watch_id);
    ctx->watchCallbacks.erase(watch_id);
}

// ========== 批量操作 ==========

void mblink_state_batch_begin(MBlinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchBegin();
}

void mblink_state_batch_end(MBlinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchEnd();
}

// ========== 队列控制 ==========

void mblink_state_set_merge_mode(MBlinkHandle handle, bool enable) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->setMergeMode(enable);
}

int mblink_process_queue(MBlinkHandle handle) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->stateManager->processQueue();
    return MBLINK_OK;
}

int mblink_queue_size(MBlinkHandle handle) {
    if (!handle) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->queueSize());
}

// ========== 共享 C 对象 (SharedObject) ==========

MBlinkSharedHandle mblink_shared_create(MBlinkHandle handle, const char* name) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return nullptr;

    auto jsCtx = ctx->runtime->GetContext();

    auto* shared = new SharedObjectData();
    shared->ctx = jsCtx;
    shared->name = name;
    shared->js_obj = JS_NewObject(jsCtx);
    shared->mainQueue_ = &ctx->mainThreadQueue;
    shared->owner_ = ctx;

    JSValue global = JS_GetGlobalObject(jsCtx);
    JSValue exposed = JS_DupValue(jsCtx, shared->js_obj);
    JSValue wrapFn = JS_GetPropertyStr(jsCtx, global, "__mblinkWrapSharedObject");
    if (JS_IsFunction(jsCtx, wrapFn)) {
        JSValue args[2] = {
            JS_NewString(jsCtx, name),
            JS_DupValue(jsCtx, shared->js_obj)
        };
        JSValue wrapped = JS_Call(jsCtx, wrapFn, JS_UNDEFINED, 2, args);
        JS_FreeValue(jsCtx, args[0]);
        JS_FreeValue(jsCtx, args[1]);
        if (!JS_IsException(wrapped) && !JS_IsUndefined(wrapped) && !JS_IsNull(wrapped)) {
            JS_FreeValue(jsCtx, exposed);
            exposed = wrapped;
        } else {
            JS_FreeValue(jsCtx, wrapped);
        }
    }
    JS_FreeValue(jsCtx, wrapFn);
    JS_SetPropertyStr(jsCtx, global, name, exposed);

    shared->refreshUpdater();
    JS_FreeValue(jsCtx, global);

    ctx->sharedObjects[name] = shared;

    return reinterpret_cast<MBlinkSharedHandle>(shared);
}

void mblink_shared_destroy(MBlinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    destroySharedObjectInternal(shared, true);
}

// ---- Setter 实现 ----

int mblink_shared_set_int(MBlinkSharedHandle shared_handle,
                            const char* key, int64_t value) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBLINK_OK;
}

int mblink_shared_set_double(MBlinkSharedHandle shared_handle,
                               const char* key, double value) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBLINK_OK;
}

int mblink_shared_set_string(MBlinkSharedHandle shared_handle,
                               const char* key, const char* value) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (value) {
        shared->safeSetProperty(key, nlohmann::json(std::string(value)));
    } else {
        shared->safeSetProperty(key, nlohmann::json(nullptr));
    }
    return MBLINK_OK;
}

int mblink_shared_set_bool(MBlinkSharedHandle shared_handle,
                             const char* key, bool value) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBLINK_OK;
}

int mblink_shared_set_null(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(nullptr));
    return MBLINK_OK;
}

int mblink_shared_set_json(MBlinkSharedHandle shared_handle,
                             const char* key, const char* json_str) {
    if (!shared_handle || !key || !json_str) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto parsed = nlohmann::json::parse(json_str, nullptr, false);
    if (parsed.is_discarded()) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    shared->safeSetProperty(key, parsed);
    return MBLINK_OK;
}

// ---- Getter 实现 ----

int64_t mblink_shared_get_int(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number_integer()) return val.get<int64_t>();
    if (val.is_number_unsigned()) return static_cast<int64_t>(val.get<uint64_t>());
    if (val.is_number_float()) return static_cast<int64_t>(val.get<double>());
    return 0;
}

double mblink_shared_get_double(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0.0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number()) return val.get<double>();
    return 0.0;
}

const char* mblink_shared_get_string(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_string()) {
        return duplicateString(val.get<std::string>().c_str());
    }
    return nullptr;
}

bool mblink_shared_get_bool(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_boolean()) return val.get<bool>();
    return false;
}

const char* mblink_shared_get_json(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_null()) return nullptr;
    auto s = val.dump();
    return duplicateString(s.c_str());
}

// ---- 属性查询 ----

int mblink_shared_get_type(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBLINK_TYPE_NULL;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeGetType(key);
}

int mblink_shared_delete(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBLINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeDeleteProperty(key);
    return MBLINK_OK;
}

bool mblink_shared_has(MBlinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeHasProperty(key);
}

// ---- 批量更新 ----

void mblink_shared_batch_begin(MBlinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (shared->mainQueue_ && shared->mainQueue_->isMainThread()) {
        shared->beginBatch();
    } else if (shared->mainQueue_) {
        auto queueAlive = shared->mainQueue_->aliveFlag();
        auto objAlive = shared->alive_;
        SharedObjectData* self = shared;
        shared->mainQueue_->post([self, queueAlive, objAlive]() {
            if (!queueAlive->load() || !objAlive->load()) return;
            self->beginBatch();
        });
    }
}

void mblink_shared_batch_end(MBlinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (shared->mainQueue_ && shared->mainQueue_->isMainThread()) {
        shared->endBatch();
    } else if (shared->mainQueue_) {
        auto queueAlive = shared->mainQueue_->aliveFlag();
        auto objAlive = shared->alive_;
        SharedObjectData* self = shared;
        shared->mainQueue_->post([self, queueAlive, objAlive]() {
            if (!queueAlive->load() || !objAlive->load()) return;
            self->endBatch();
        });
    }
}

MBlinkLogViewHandle mblink_logview_get(MBlinkHandle handle, const char* element_id) {
    if (!handle || !element_id) return nullptr;
    auto ctx = getContext(handle);
    auto element = getElementByIdAs<mblink::HTMLLogViewElement>(ctx, element_id);
    if (!element) {
        setLastError(std::string("logview element not found: ") + element_id);
        return nullptr;
    }
    auto* data = new LogViewHandleData();
    data->element = std::move(element);
    data->mainQueue = &ctx->mainThreadQueue;
    return reinterpret_cast<MBlinkLogViewHandle>(data);
}

void mblink_logview_destroy(MBlinkLogViewHandle logview_handle) {
    if (!logview_handle) return;
    delete reinterpret_cast<LogViewHandleData*>(logview_handle);
}

int mblink_logview_append(MBlinkLogViewHandle logview_handle,
                         const char* level,
                         const char* source,
                         const char* message) {
    if (!logview_handle || !level || !source || !message) {
        return MBLINK_ERROR_INVALID_PARAM;
    }
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Append(level, source, message);
    } else if (data->mainQueue) {
        std::string l(level), s(source), m(message);
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, l, s, m, alive]() {
            if (!alive->load()) return;
            elem->Append(l.c_str(), s.c_str(), m.c_str());
        });
    }
    return MBLINK_OK;
}

void mblink_logview_clear(MBlinkLogViewHandle logview_handle) {
    if (!logview_handle) return;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Clear();
    } else if (data->mainQueue) {
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, alive]() {
            if (!alive->load()) return;
            elem->Clear();
        });
    }
}

const char* mblink_logview_export(MBlinkLogViewHandle logview_handle,
                                 const char* format) {
    if (!logview_handle) return nullptr;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && !data->mainQueue->isMainThread()) {
        setLastError("mblink_logview_export must be called on main thread");
        return nullptr;
    }
    auto content = data->element->Export(format ? format : "text");
    return duplicateString(content.c_str());
}

MBlinkTerminalHandle mblink_terminal_get(MBlinkHandle handle, const char* element_id) {
    if (!handle || !element_id) return nullptr;
    auto ctx = getContext(handle);
    auto element = getElementByIdAs<mblink::HTMLTerminalElement>(ctx, element_id);
    if (!element) {
        setLastError(std::string("terminal element not found: ") + element_id);
        return nullptr;
    }
    auto* data = new TerminalHandleData();
    data->element = std::move(element);
    data->mainQueue = &ctx->mainThreadQueue;
    return reinterpret_cast<MBlinkTerminalHandle>(data);
}

void mblink_terminal_destroy(MBlinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    delete reinterpret_cast<TerminalHandleData*>(terminal_handle);
}

int mblink_terminal_write(MBlinkTerminalHandle terminal_handle, const char* data_str) {
    if (!terminal_handle || !data_str) return MBLINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Write(data_str);
    } else if (data->mainQueue) {
        std::string text(data_str);
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, text, alive]() {
            if (!alive->load()) return;
            elem->Write(text.c_str());
        });
    }
    return MBLINK_OK;
}

void mblink_terminal_clear(MBlinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Clear();
    } else if (data->mainQueue) {
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, alive]() {
            if (!alive->load()) return;
            elem->Clear();
        });
    }
}

int mblink_terminal_execute(MBlinkTerminalHandle terminal_handle, const char* command) {
    if (!terminal_handle || !command) return MBLINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Execute(command);
    } else if (data->mainQueue) {
        std::string cmd(command);
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, cmd, alive]() {
            if (!alive->load()) return;
            elem->Execute(cmd.c_str());
        });
    }
    return MBLINK_OK;
}

int mblink_terminal_start_shell(MBlinkTerminalHandle terminal_handle, const char* shell) {
    if (!terminal_handle) return MBLINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->StartShell(shell ? shell : "");
    } else if (data->mainQueue) {
        std::string shellCopy = shell ? shell : "";
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, shellCopy, alive]() {
            if (!alive->load()) return;
            elem->StartShell(shellCopy.c_str());
        });
    }
    return MBLINK_OK;
}

int mblink_terminal_send_input(MBlinkTerminalHandle terminal_handle, const char* input) {
    if (!terminal_handle || !input) return MBLINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->SendInput(input);
    } else if (data->mainQueue) {
        std::string text(input);
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, text, alive]() {
            if (!alive->load()) return;
            elem->SendInput(text.c_str());
        });
    }
    return MBLINK_OK;
}

void mblink_terminal_resize(MBlinkTerminalHandle terminal_handle, int rows, int cols) {
    if (!terminal_handle) return;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Resize(rows, cols);
    } else if (data->mainQueue) {
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, rows, cols, alive]() {
            if (!alive->load()) return;
            elem->Resize(rows, cols);
        });
    }
}

const char* mblink_terminal_serialize(MBlinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return nullptr;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && !data->mainQueue->isMainThread()) {
        setLastError("mblink_terminal_serialize must be called on main thread");
        return nullptr;
    }
    auto content = data->element->Serialize();
    return duplicateString(content.c_str());
}

int mblink_compile_resources(const char* input_path,
                            const char* output_file,
                            const char* encryption_key) {
    try {
        std::string error;
        if (!mblink::resourcepkg::CompileResources(input_path, output_file, encryption_key, error)) {
            setLastError(error.empty() ? "Failed to compile resources" : error);
            return MBLINK_ERROR_UNKNOWN;
        }
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_UNKNOWN;
    }
}

int mblink_load_resource_file(const char* package_file,
                             const char* resource_path,
                             const char* encryption_key,
                             void** out_data,
                             size_t* out_size,
                             uint32_t* out_flags) {
    if (!out_data || !out_size) return MBLINK_ERROR_INVALID_PARAM;
    *out_data = nullptr;
    *out_size = 0;
    if (out_flags) *out_flags = 0;

    try {
        std::vector<uint8_t> data;
        std::string error;
        if (!mblink::resourcepkg::LoadResourceFile(package_file,
                                                  resource_path,
                                                  encryption_key,
                                                  data,
                                                  out_flags,
                                                  error)) {
            setLastError(error.empty() ? "Failed to load resource file" : error);
            return error == "resource not found" ? MBLINK_ERROR_NOT_FOUND : MBLINK_ERROR_UNKNOWN;
        }

        void* buffer = std::malloc(data.empty() ? 1 : data.size());
        if (!buffer) {
            setLastError("Out of memory");
            return MBLINK_ERROR_UNKNOWN;
        }
        if (!data.empty()) std::memcpy(buffer, data.data(), data.size());

        *out_data = buffer;
        *out_size = data.size();
        return MBLINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBLINK_ERROR_UNKNOWN;
    }
}

int mblink_mount_resource_package(MBlinkHandle handle,
                                 const char* package_file,
                                 const char* encryption_key,
                                 const char* mount_point) {
    if (!handle) return MBLINK_ERROR_INVALID_HANDLE;
    if (!package_file) return MBLINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    if (!ctx || !ctx->runtime || !ctx->document) return MBLINK_ERROR_INVALID_HANDLE;

    const char* normalizedKey = encryption_key ? encryption_key : "";
    const std::string normalizedMount = NormalizeResourcePath(mount_point ? mount_point : "/");

    SetMountedResourceState(ctx, package_file, normalizedKey, normalizedMount);
    SetRuntimeBasePath(ctx, normalizedMount);
    ctx->runtime->SetBaseModulePath(normalizedMount == "/" ? "/index.js" : normalizedMount + "/index.js");
    return MBLINK_OK;
}


// ========== 工具函数 ==========

void mblink_free(void* ptr) {
    free(ptr);
}

char* mblink_copy_string(const char* str) {
    return duplicateString(str);
}

const char* mblink_last_error(void) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return g_lastError.c_str();
}
