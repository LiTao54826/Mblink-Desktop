/**
 * @file mbink.cpp
 * @brief MBink C API v2 实现
 *
 * 一个 create() 调用完成全部初始化：
 * Window → Document → TaskScheduler → QuickJSRuntime → DOMBindings →
 * WindowBindings → EventLoop → FetchBindings → StateManager → HostBridge
 */

#include "mbink.h"
#include "resource_package.h"
#include "core/bridge/state_manager.h"
#include "core/bridge/host_bridge.h"
#include "core/bridge/main_thread_queue.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
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
#include "core/lexbor/lexbor_stylesheet.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/quickjs/bindings/js_element.h"
#include "tools/esm_loader/embedded_js.h"
#include "core/utils/encoding_utils.h"

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <filesystem>
#include <chrono>

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

// ========== WindowContext ==========

struct WindowContext;
bool isSharedDiagEnabled(WindowContext* ctx);
nlohmann::json getSharedDiagMemoryStats(WindowContext* ctx, bool gc);

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
    mbink::MainThreadQueue* mainQueue_ = nullptr;
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
    WindowContext* owner_ = nullptr;

    void refreshUpdater() {
        if (!ctx) return;
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue updater = JS_GetPropertyStr(ctx, global, "__mbinkSharedUpdateDispatcher");
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
        if (val.is_null() || val.is_discarded()) return MBINK_TYPE_NULL;
        if (val.is_boolean()) return MBINK_TYPE_BOOL;
        if (val.is_number_integer() || val.is_number_unsigned()) return MBINK_TYPE_INT;
        if (val.is_number_float()) return MBINK_TYPE_DOUBLE;
        if (val.is_string()) return MBINK_TYPE_STRING;
        if (val.is_array()) return MBINK_TYPE_ARRAY;
        if (val.is_object()) return MBINK_TYPE_OBJECT;
        return MBINK_TYPE_NULL;
    }
};

struct WindowContext {
    // 核心组件（完整初始化链）
    std::shared_ptr<mbink::Window> window;
    std::shared_ptr<mbink::Document> document;
    std::shared_ptr<mbink::TaskScheduler> taskScheduler;
    std::unique_ptr<mbink::QuickJSRuntime> runtime;
    std::unique_ptr<mbink::WindowBindings> windowBindings;
    std::unique_ptr<mbink::EventLoop> eventLoop;
    std::unique_ptr<mbink::HostBridge> hostBridge;
    std::unique_ptr<mbink::FetchBindings> fetchBindings;
    std::unique_ptr<mbink::StateManager> stateManager;
    mbink::MainThreadQueue mainThreadQueue;
    std::string mountedResourcePackage;
    std::string mountedResourceKey;
    std::string mountedResourceMountPoint = "/";

    // 共享对象存储
    std::unordered_map<std::string, SharedObjectData*> sharedObjects;

    // 回调存储
    std::unordered_map<int, std::pair<MBinkStateCallback, void*>> watchCallbacks;
    std::unordered_map<std::string, std::pair<MBinkCallback, void*>> boundFunctions;
    std::unordered_map<std::string, std::pair<MBinkAsyncCallback, void*>> boundAsyncFunctions;

    // 事件回调
    MBinkResizeCallback onResizeCallback = nullptr;
    void* onResizeUserData = nullptr;
    MBinkVoidCallback onCloseCallback = nullptr;
    void* onCloseUserData = nullptr;
    MBinkVoidCallback onFocusCallback = nullptr;
    void* onFocusUserData = nullptr;
    MBinkVoidCallback onBlurCallback = nullptr;
    void* onBlurUserData = nullptr;
    MBinkUpdateCallback onUpdateCallback = nullptr;
    void* onUpdateUserData = nullptr;

    bool running = false;
};

struct LogViewHandleData {
    std::shared_ptr<mbink::HTMLLogViewElement> element;
    mbink::MainThreadQueue* mainQueue = nullptr;
};

struct TerminalHandleData {
    std::shared_ptr<mbink::HTMLTerminalElement> element;
    mbink::MainThreadQueue* mainQueue = nullptr;
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
    std::fprintf(stderr, "[MBink Native Error] %s\n", error.c_str());
#ifdef _WIN32
    std::string out = "[MBink Native Error] " + error + "\n";
    ::OutputDebugStringA(out.c_str());
#endif
    std::ofstream log("mbink_native_error.log", std::ios::app);
    if (log.is_open()) {
        log << error << std::endl;
    }
}

#ifdef _WIN32
char* invokeCallbackWithSEH(MBinkCallback cb, const char* args, void* user_data, unsigned int* sehCode);
#endif

std::string invokeBoundJsonCallback(WindowContext* ctx,
                                    const std::string& funcName,
                                    const std::string& args,
                                    MBinkCallback cb,
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
    result = invokeCallbackWithSEH(cb, args.c_str(), ud, &sehCode);
    finishSharedBatch();
    if (!result && sehCode != 0) {
        reportNativeError("SEH exception in bound callback '" + funcName +
                          "', code=0x" + std::to_string(sehCode));
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
    mbink_free(result);
    return ret;
}

bool loadEmbeddedRuntimeScripts(mbink::QuickJSRuntime* runtime) {
    if (!runtime || !mbink::embedded::HasEmbeddedJS()) {
        return true;
    }

    auto evalScript = [&](std::string_view code, const char* filename) {
        if (code.empty()) {
            return;
        }
        runtime->Eval(std::string(code), filename ? filename : "<embedded>");
    };

    evalScript(mbink::embedded::GetDomPolyfillsJS(), "dom.js");
    evalScript(mbink::embedded::GetBootstrapJS(), "bootstrap.js");
    evalScript(mbink::embedded::GetPreactJS(), "preact.js");
    evalScript(mbink::embedded::GetHooksJS(), "hooks.js");
    return true;
}

fs::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return fs::path(mbink::utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string FsPathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return mbink::utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizeFsPath(const fs::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
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
    if (!ctx || ctx->mountedResourcePackage.empty()) {
        return false;
    }

    const std::string resourcePath = JoinMountedResourcePath(
        ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
        requestPath);
    if (resourcePath.empty()) {
        return false;
    }

    std::string error;
    return mbink::resourcepkg::LoadResourceFile(
        ctx->mountedResourcePackage.c_str(),
        resourcePath.c_str(),
        ctx->mountedResourceKey.c_str(),
        out,
        nullptr,
        error);
}

void registerPreactModules(mbink::QuickJSRuntime* runtime) {
    if (!runtime) {
        return;
    }

    runtime->RegisterModule("preact", R"(
        export const h = globalThis.Preact.h;
        export const render = globalThis.Preact.render;
        export const Component = globalThis.Preact.Component;
        export const Fragment = globalThis.Preact.Fragment;
        export const createRef = globalThis.Preact.createRef;
        export const createElement = globalThis.Preact.createElement;
        export const createContext = globalThis.Preact.createContext;
        export const cloneElement = globalThis.Preact.cloneElement;
        export const isValidElement = globalThis.Preact.isValidElement;
        export default globalThis.Preact;
    )");

    runtime->RegisterModule("preact/hooks", R"(
        export const useState = globalThis.PreactHooks.useState;
        export const useEffect = globalThis.PreactHooks.useEffect;
        export const useRef = globalThis.PreactHooks.useRef;
        export const useMemo = globalThis.PreactHooks.useMemo;
        export const useCallback = globalThis.PreactHooks.useCallback;
        export const useContext = globalThis.PreactHooks.useContext;
        export const useReducer = globalThis.PreactHooks.useReducer;
        export const useLayoutEffect = globalThis.PreactHooks.useLayoutEffect;
        export const useImperativeHandle = globalThis.PreactHooks.useImperativeHandle;
        export const useDebugValue = globalThis.PreactHooks.useDebugValue;
        export default globalThis.PreactHooks;
    )");
}

#ifdef _WIN32
LONG WINAPI mbinkUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    unsigned int code = exceptionInfo ? exceptionInfo->ExceptionRecord->ExceptionCode : 0;
    void* address = (exceptionInfo && exceptionInfo->ExceptionRecord)
                        ? exceptionInfo->ExceptionRecord->ExceptionAddress
                        : nullptr;
    std::string error = "Unhandled SEH exception, code=0x" + std::to_string(code) +
                        ", address=" + std::to_string(reinterpret_cast<uintptr_t>(address));
    reportNativeError(error);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif


int toErrorCode(mbink::MBinkError err) {
    switch (err) {
        case mbink::MBinkError::Ok: return MBINK_OK;
        case mbink::MBinkError::InvalidHandle: return MBINK_ERROR_INVALID_HANDLE;
        case mbink::MBinkError::NotFound: return MBINK_ERROR_NOT_FOUND;
        case mbink::MBinkError::TypeMismatch: return MBINK_ERROR_TYPE_MISMATCH;
        case mbink::MBinkError::IndexOutOfRange: return MBINK_ERROR_OUT_OF_RANGE;
        case mbink::MBinkError::InvalidJson: return MBINK_ERROR_INVALID_PARAM;
        case mbink::MBinkError::AlreadyExists: return MBINK_ERROR_INVALID_PARAM;
        case mbink::MBinkError::InvalidName: return MBINK_ERROR_INVALID_PARAM;
        default: return MBINK_ERROR_UNKNOWN;
    }
}

MBinkType toMBinkType(mbink::MBinkType type) {
    switch (type) {
        case mbink::MBinkType::Null: return MBINK_TYPE_NULL;
        case mbink::MBinkType::Bool: return MBINK_TYPE_BOOL;
        case mbink::MBinkType::Int: return MBINK_TYPE_INT;
        case mbink::MBinkType::Double: return MBINK_TYPE_DOUBLE;
        case mbink::MBinkType::String: return MBINK_TYPE_STRING;
        case mbink::MBinkType::Array: return MBINK_TYPE_ARRAY;
        case mbink::MBinkType::Object: return MBINK_TYPE_OBJECT;
        default: return MBINK_TYPE_NULL;
    }
}

WindowContext* getContext(MBinkHandle handle) {
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

#ifdef _WIN32
char* invokeCallbackWithSEH(MBinkCallback cb, const char* args, void* user_data, unsigned int* sehCode) {
    if (sehCode) {
        *sehCode = 0;
    }

    char* result = nullptr;
    __try {
        result = cb(args, user_data);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (sehCode) {
            *sehCode = static_cast<unsigned int>(GetExceptionCode());
        }
        result = nullptr;
    }
    return result;
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

// 从 MBinkConfig 构建 WindowConfig
mbink::WindowConfig buildWindowConfig(const MBinkConfig* config) {
    mbink::WindowConfig wc;
    wc.title = config->title ? config->title : "MBink";
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
WindowContext* createWindowContext(const mbink::WindowConfig& wc) {
    auto ctx = new WindowContext();

    // 1. 创建 Window
    ctx->window = std::make_shared<mbink::Window>(wc);

    // 2. 创建 Document → 设置到 Window
    ctx->document = std::make_shared<mbink::Document>();
    ctx->window->SetDocument(ctx->document);

    // 3. 创建 TaskScheduler
    ctx->taskScheduler = std::make_shared<mbink::TaskScheduler>();

    // 4. 注册到 WindowManager
    mbink::WindowManager::Instance().RegisterWindow(ctx->window);

    // 5. 创建 QuickJS Runtime
    ctx->runtime = std::make_unique<mbink::QuickJSRuntime>();

    // 6. 设置 JS Runtime 到 Document
    ctx->document->SetJSRuntime(ctx->runtime.get());
    ctx->document->SetBasePath("");
    mbink::FetchBindings::SetBasePath("");
    mbink::ImageLoader::SetBasePath("");

    auto mountedAssetProvider = [ctx](const std::string& path, std::vector<uint8_t>& out) {
        return LoadMountedResourceAsset(ctx, path, out);
    };
    ctx->runtime->SetFileLoader([ctx](const std::string& path, std::string& out, std::string* error) {
        std::vector<uint8_t> data;
        if (!LoadMountedResourceAsset(ctx, path, data)) {
            if (error) *error = ctx->mountedResourcePackage.empty()
                                   ? "resource package not mounted"
                                   : "resource path outside mount point or not found";
            return false;
        }

        out.assign(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    });
    mbink::Document::SetAssetProvider(mountedAssetProvider);
    mbink::FetchBindings::SetAssetProvider(mountedAssetProvider);
    mbink::ImageLoader::SetAssetProvider(mountedAssetProvider);
    mbink::LexborStyleSheet::SetAssetProvider(mountedAssetProvider);

    // 7. 初始化 DOM 绑定
    auto jsCtx = ctx->runtime->GetContext();
    mbink::DOMBindings::Init(jsCtx);
    mbink::DOMBindings::SetGlobalDocument(jsCtx, ctx->document);

    // 8. 创建 WindowBindings + 初始化（setTimeout/setInterval/RAF/DOM/Canvas...）
    ctx->windowBindings = std::make_unique<mbink::WindowBindings>(
        ctx->runtime.get(), ctx->window, ctx->taskScheduler);
    ctx->windowBindings->InitBindings();

    // 9. 创建 EventLoop（使用同一 TaskScheduler）
    ctx->eventLoop = std::make_unique<mbink::EventLoop>(ctx->taskScheduler);
    ctx->eventLoop->SetQuickJSRuntime(ctx->runtime.get());
    mbink::DOMBindings::SetGlobalEventLoop(jsCtx, ctx->eventLoop.get());

    // 10. 创建 FetchBindings
    ctx->fetchBindings = std::make_unique<mbink::FetchBindings>(jsCtx, ctx->taskScheduler);
    ctx->fetchBindings->InitBindings();

    // 11. 创建 StateManager + HostBridge
#ifdef _WIN32
    SetUnhandledExceptionFilter(mbinkUnhandledExceptionFilter);
#endif

    ctx->stateManager = std::make_unique<mbink::StateManager>();
    ctx->hostBridge = std::make_unique<mbink::HostBridge>(jsCtx, ctx->stateManager.get());
    ctx->hostBridge->registerGlobal();

    loadEmbeddedRuntimeScripts(ctx->runtime.get());
    registerPreactModules(ctx->runtime.get());

    return ctx;
}

} // anonymous namespace

// ========== 生命周期 ==========

int mbink_init(void) {
    if (g_initialized) return MBINK_OK;
    g_initialized = true;
    return MBINK_OK;
}

void mbink_cleanup(void) {
    g_initialized = false;
}

const char* mbink_version(void) {
    return "0.1.0";
}

// ========== 窗口管理 ==========

MBinkHandle mbink_create(const char* title, int width, int height) {
    if (!g_initialized) {
        setLastError("MBink not initialized");
        return nullptr;
    }

    try {
        mbink::WindowConfig wc;
        wc.title = title ? title : "MBink";
        wc.width = width > 0 ? width : 800;
        wc.height = height > 0 ? height : 600;
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<MBinkHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

MBinkHandle mbink_create_ex(const MBinkConfig* config) {
    if (!g_initialized) {
        setLastError("MBink not initialized");
        return nullptr;
    }
    if (!config) {
        setLastError("config is null");
        return nullptr;
    }

    try {
        auto wc = buildWindowConfig(config);
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<MBinkHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

MBinkConfig mbink_default_config(void) {
    MBinkConfig config = {};
    config.title = "MBink";
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

void mbink_destroy(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);

    // 0. 先隐藏窗口，避免后续重清理阶段造成用户可见卡顿
    SAFE_CLEANUP("hide_window", if (ctx->window) { ctx->window->Hide(); });

    // 1. 停止事件循环
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
            "(function(){if(typeof __mbinkShutdown==='function'){__mbinkShutdown();}})();";
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
    SAFE_CLEANUP("flush_main_thread_queue", ctx->mainThreadQueue.flush());

    // 4. 停止任务源
    SAFE_CLEANUP("shutdown_task_scheduler", if (ctx->taskScheduler) {
        ctx->taskScheduler->Shutdown();
        ctx->taskScheduler->ClearAllTasks();
    });
    SAFE_CLEANUP("flush_main_thread_queue_after_shutdown", ctx->mainThreadQueue.flush());
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
    SAFE_CLEANUP("dom_bindings_cleanup", if (ctx->runtime) {
        mbink::DOMBindings::Cleanup(ctx->runtime->GetContext());
    });
    ctx->document.reset();
    mbink::DOMBindingMap::GetInstance().Clear();

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
    ctx->eventLoop.reset();
    ctx->windowBindings.reset();
    ctx->fetchBindings.reset();
    ctx->taskScheduler.reset();

    SAFE_CLEANUP("unregister_window", if (ctx->window) {
        mbink::WindowManager::Instance().UnregisterWindow(ctx->window);
    });

    ctx->watchCallbacks.clear();
    ctx->boundFunctions.clear();
    ctx->boundAsyncFunctions.clear();

    std::exit(0);
}

void mbink_run(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return;
    if (ctx->running) return;

    ctx->running = true;

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
    ctx->eventLoop->Run();
    ctx->running = false;
}

void mbink_stop(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (ctx->eventLoop) {
        ctx->eventLoop->Stop();
    }
}

bool mbink_poll_events(MBinkHandle handle) {
    if (!handle) return false;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return false;

    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    }
    ctx->mainThreadQueue.flush();
    forEachSharedObjectSnapshot(ctx, [](SharedObjectData* shared) {
        shared->flushPendingNotify();
    });
    if (ctx->runtime) {
        ctx->runtime->ProcessMicrotasks();
    }

    ctx->eventLoop->RunOnce();

    return !ctx->eventLoop->ShouldQuit();
}

// ========== 窗口属性 ==========

int mbink_set_title(MBinkHandle handle, const char* title) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (title && ctx->window) {
        ctx->window->SetTitle(title);
    }
    return MBINK_OK;
}

int mbink_set_size(MBinkHandle handle, int width, int height) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetSize(width, height);
    }
    return MBINK_OK;
}

int mbink_get_size(MBinkHandle handle, int* width, int* height) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int w = 0, h = 0;
        ctx->window->GetSize(&w, &h);
        if (width) *width = w;
        if (height) *height = h;
    }
    return MBINK_OK;
}

int mbink_set_position(MBinkHandle handle, int x, int y) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetPosition(x, y);
    }
    return MBINK_OK;
}

int mbink_get_position(MBinkHandle handle, int* x, int* y) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int px = 0, py = 0;
        ctx->window->GetPosition(&px, &py);
        if (x) *x = px;
        if (y) *y = py;
    }
    return MBINK_OK;
}

int mbink_set_min_size(MBinkHandle handle, int width, int height) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMinSize(width, height);
    }
    return MBINK_OK;
}

int mbink_set_max_size(MBinkHandle handle, int width, int height) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMaxSize(width, height);
    }
    return MBINK_OK;
}

int mbink_minimize(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Minimize();
    return MBINK_OK;
}

int mbink_maximize(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Maximize();
    return MBINK_OK;
}

int mbink_restore(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Restore();
    return MBINK_OK;
}

int mbink_show(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Show();
    return MBINK_OK;
}

int mbink_hide(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Hide();
    return MBINK_OK;
}

int mbink_set_fullscreen(MBinkHandle handle, bool fullscreen) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetFullscreen(fullscreen);
    return MBINK_OK;
}

int mbink_set_resizable(MBinkHandle handle, bool resizable) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetResizable(resizable);
    return MBINK_OK;
}

int mbink_set_borderless(MBinkHandle handle, bool borderless) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetBorderless(borderless);
    return MBINK_OK;
}

int mbink_set_always_on_top(MBinkHandle handle, bool on_top) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetAlwaysOnTop(on_top);
    return MBINK_OK;
}

// ========== UI 加载 ==========

int mbink_load_html(MBinkHandle handle, const char* html) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!html) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (ctx->document) {
        ctx->document->SetBasePath("");
        mbink::FetchBindings::SetBasePath("");
        mbink::ImageLoader::SetBasePath("");
        if (!ctx->document->LoadHTML(html)) {
            setLastError("Failed to parse HTML");
            return MBINK_ERROR_INVALID_PARAM;
        }
        ctx->document->LoadExternalStylesheets();
        ctx->document->ExecuteScripts();
    }
    return MBINK_OK;
}

int mbink_load_html_file(MBinkHandle handle, const char* filepath) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!filepath) return MBINK_ERROR_INVALID_PARAM;
    try {
        auto ctx = getContext(handle);
        if (ctx->document) {
            std::string content;
            std::string base_path;

            const std::string resourcePath = JoinMountedResourcePath(
                ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
                filepath);

            if (!ctx->mountedResourcePackage.empty() && !resourcePath.empty()) {
                std::vector<uint8_t> data;
                std::string error;
                if (mbink::resourcepkg::LoadResourceFile(
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

            ctx->document->SetBasePath(base_path);
            mbink::FetchBindings::SetBasePath(base_path);
            mbink::ImageLoader::SetBasePath(base_path);
            if (!ctx->document->LoadHTML(content)) {
                setLastError("Failed to parse HTML");
                return MBINK_ERROR_INVALID_PARAM;
            }
            ctx->document->LoadExternalStylesheets();
            ctx->document->ExecuteScripts();
        }
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_INVALID_PARAM;
    }
}

int mbink_eval_js(MBinkHandle handle, const char* js_code) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!js_code) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBINK_ERROR_INVALID_HANDLE;

    try {
        ctx->runtime->Eval(js_code);
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_JS_ERROR;
    }
}

int mbink_eval_module(MBinkHandle handle, const char* code, const char* filename) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!code) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBINK_ERROR_INVALID_HANDLE;

    try {
        const char* fname = filename ? filename : "<module>";
        ctx->runtime->EvalModule(code, fname);
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_JS_ERROR;
    }
}

int mbink_load_js_file(MBinkHandle handle, const char* filepath) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!filepath) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBINK_ERROR_INVALID_HANDLE;

    try {
        std::string path = filepath;
        if (!ctx->mountedResourcePackage.empty()) {
            const std::string resourcePath = JoinMountedResourcePath(
                ctx->mountedResourceMountPoint.empty() ? "/" : ctx->mountedResourceMountPoint,
                filepath);
            if (!resourcePath.empty()) {
                std::vector<uint8_t> data;
                uint32_t flags = 0;
                std::string error;
                if (mbink::resourcepkg::LoadResourceFile(
                        ctx->mountedResourcePackage.c_str(),
                        resourcePath.c_str(),
                        ctx->mountedResourceKey.c_str(),
                        data,
                        &flags,
                        error)) {
                    if ((flags & mbink::resourcepkg::kResourceFlagBytecode) != 0) {
                        auto jsCtx = ctx->runtime->GetContext();
                        if (!mbink::resourcepkg::EvalMaybeMergedBytecode(jsCtx, data.data(), data.size(), error)) {
                            setLastError(error.empty() ? "Failed to eval resource bytecode" : error);
                            return MBINK_ERROR_JS_ERROR;
                        }
                        return MBINK_OK;
                    }
                    path = NormalizeResourcePath(filepath);
                }
            }
        }
        ctx->runtime->LoadModuleFile(path);
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_JS_ERROR;
    }
}

int mbink_load_bytecode(MBinkHandle handle, const void* data, size_t size) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!data || size == 0) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return MBINK_ERROR_INVALID_HANDLE;

    try {
        auto jsCtx = ctx->runtime->GetContext();
        std::string error;
        if (!mbink::resourcepkg::EvalMaybeMergedBytecode(jsCtx, data, size, error)) {
            setLastError(error.empty() ? "Failed to eval bytecode" : error);
            return MBINK_ERROR_JS_ERROR;
        }
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_JS_ERROR;
    }
}

// ========== 函数绑定 ==========

int mbink_bind(MBinkHandle handle, const char* name,
                 MBinkCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !callback) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    ctx->boundFunctions[name] = {callback, user_data};

    // 通过 HostBridge 注册，这样 JS 端可以通过 backend.name() 调用
    if (ctx->hostBridge) {
        MBinkCallback cb = callback;
        void* ud = user_data;
        std::string funcName = name;
        ctx->hostBridge->bind(name, [ctx, cb, ud, funcName](const std::string& args) -> std::string {
            return invokeBoundJsonCallback(ctx, funcName, args, cb, ud);
        });
    }
    return MBINK_OK;
}

int mbink_bind_async(MBinkHandle handle, const char* name,
                     MBinkAsyncCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !callback) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    ctx->boundAsyncFunctions[name] = {callback, user_data};

    if (ctx->hostBridge) {
        MBinkAsyncCallback cb = callback;
        void* ud = user_data;
        std::string funcName = name;
        ctx->hostBridge->bindAsync(name, [ctx, cb, ud, funcName](const std::string& args) -> std::string {
            return invokeBoundJsonCallback(ctx, funcName, args, cb, ud);
        });
    }
    return MBINK_OK;
}

void mbink_unbind(MBinkHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->boundFunctions.erase(name);
    ctx->boundAsyncFunctions.erase(name);
    if (ctx->hostBridge) {
        ctx->hostBridge->unbind(name);
    }
}

// ========== 事件回调 ==========

int mbink_on_resize(MBinkHandle handle, MBinkResizeCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
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
    return MBINK_OK;
}

int mbink_on_close(MBinkHandle handle, MBinkVoidCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
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
    return MBINK_OK;
}

int mbink_on_focus(MBinkHandle handle, MBinkVoidCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
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
    return MBINK_OK;
}

int mbink_on_blur(MBinkHandle handle, MBinkVoidCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
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
    return MBINK_OK;
}

int mbink_on_update(MBinkHandle handle, MBinkUpdateCallback callback, void* user_data) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onUpdateCallback = callback;
    ctx->onUpdateUserData = user_data;
    return MBINK_OK;
}

// ========== 事件发送 ==========

int mbink_emit(MBinkHandle handle, const char* event_name, const char* data_json) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!event_name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (ctx->hostBridge) {
        ctx->hostBridge->emit(event_name, data_json ? data_json : "null");
    }
    return MBINK_OK;
}

// ========== DevTools ==========

int mbink_devtools_open(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    // DevTools 功能暂不实现，预留接口
    return MBINK_OK;
}

int mbink_devtools_close(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    // DevTools 功能暂不实现，预留接口
    return MBINK_OK;
}

// ========== 状态创建 ==========

int mbink_state_create_null(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createNull(name));
}

int mbink_state_create_bool(MBinkHandle handle, const char* name, bool value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createBool(name, value));
}

int mbink_state_create_int(MBinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createInt(name, value));
}

int mbink_state_create_double(MBinkHandle handle, const char* name, double value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createDouble(name, value));
}

int mbink_state_create_string(MBinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createString(name, value ? value : ""));
}

int mbink_state_create_array(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createArray(name));
}

int mbink_state_create_object(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createObject(name));
}

int mbink_state_create_json(MBinkHandle handle, const char* name, const char* json_str) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(json_str);
        return toErrorCode(ctx->stateManager->createJson(name, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}


// ========== 状态查询 ==========

bool mbink_state_exists(MBinkHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->exists(name);
}

MBinkType mbink_state_type(MBinkHandle handle, const char* name) {
    if (!handle || !name) return MBINK_TYPE_NULL;
    auto ctx = getContext(handle);
    return toMBinkType(ctx->stateManager->type(name));
}

void mbink_state_delete(MBinkHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->stateManager->remove(name);
    ctx->stateManager->processQueue();
}

// ========== 状态读取（直接类型） ==========

bool mbink_state_get_bool(MBinkHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->getBool(name);
}

int64_t mbink_state_get_int(MBinkHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getInt(name);
}

double mbink_state_get_double(MBinkHandle handle, const char* name) {
    if (!handle || !name) return 0.0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getDouble(name);
}

const char* mbink_state_get_string(MBinkHandle handle, const char* name) {
    if (!handle || !name) return "";
    auto ctx = getContext(handle);
    return ctx->stateManager->getString(name).c_str();
}

int mbink_state_get_length(MBinkHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->getLength(name));
}

// ========== 状态读取（JSON） ==========

char* mbink_state_get_json(MBinkHandle handle, const char* name) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getJson(name);
    return duplicateString(j.dump());
}

char* mbink_state_get_at(MBinkHandle handle, const char* name, int index) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getAt(name, index);
    return duplicateString(j.dump());
}

char* mbink_state_get_key(MBinkHandle handle, const char* name, const char* key) {
    if (!handle || !name || !key) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getKey(name, key);
    return duplicateString(j.dump());
}

// ========== 状态写入 ==========

int mbink_state_set_null(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setNull(name));
}

int mbink_state_set_bool(MBinkHandle handle, const char* name, bool value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setBool(name, value));
}

int mbink_state_set_int(MBinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setInt(name, value));
}

int mbink_state_set_double(MBinkHandle handle, const char* name, double value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setDouble(name, value));
}

int mbink_state_set_string(MBinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setString(name, value ? value : ""));
}

int mbink_state_set_json(MBinkHandle handle, const char* name, const char* json_str) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(json_str);
        return toErrorCode(ctx->stateManager->setJson(name, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}


// ========== 数组操作 ==========

int mbink_state_array_push(MBinkHandle handle, const char* name, const char* item_json) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayPush(name, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}

int mbink_state_array_push_int(MBinkHandle handle, const char* name, int64_t value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mbink_state_array_push_double(MBinkHandle handle, const char* name, double value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mbink_state_array_push_string(MBinkHandle handle, const char* name, const char* value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value ? value : ""));
}

int mbink_state_array_push_bool(MBinkHandle handle, const char* name, bool value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int mbink_state_array_pop(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPop(name));
}

int mbink_state_array_shift(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayShift(name));
}

int mbink_state_array_unshift(MBinkHandle handle, const char* name, const char* item_json) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayUnshift(name, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}

int mbink_state_array_remove(MBinkHandle handle, const char* name, int index) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayRemove(name, index));
}

int mbink_state_array_clear(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayClear(name));
}

int mbink_state_array_set(MBinkHandle handle, const char* name, int index, const char* item_json) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arraySet(name, index, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}

int mbink_state_array_set_int(MBinkHandle handle, const char* name, int index, int64_t value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int mbink_state_array_set_double(MBinkHandle handle, const char* name, int index, double value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int mbink_state_array_set_string(MBinkHandle handle, const char* name, int index, const char* value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value ? value : ""));
}


// ========== 对象操作 ==========

int mbink_state_object_set(MBinkHandle handle, const char* name, const char* key, const char* value_json) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key || !value_json) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = mbink::json::parse(value_json);
        return toErrorCode(ctx->stateManager->objectSet(name, key, j));
    } catch (...) {
        return MBINK_ERROR_INVALID_PARAM;
    }
}

int mbink_state_object_set_int(MBinkHandle handle, const char* name, const char* key, int64_t value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mbink_state_object_set_double(MBinkHandle handle, const char* name, const char* key, double value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mbink_state_object_set_string(MBinkHandle handle, const char* name, const char* key, const char* value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value ? value : ""));
}

int mbink_state_object_set_bool(MBinkHandle handle, const char* name, const char* key, bool value) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int mbink_state_object_remove(MBinkHandle handle, const char* name, const char* key) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !key) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectRemove(name, key));
}

int mbink_state_object_clear(MBinkHandle handle, const char* name) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectClear(name));
}

// ========== 数值操作 ==========

int mbink_state_increment(MBinkHandle handle, const char* name, double delta) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->increment(name, delta));
}

int mbink_state_multiply(MBinkHandle handle, const char* name, double factor) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->multiply(name, factor));
}

// ========== 字符串操作 ==========

int mbink_state_string_append(MBinkHandle handle, const char* name, const char* suffix) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !suffix) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringAppend(name, suffix));
}

int mbink_state_string_prepend(MBinkHandle handle, const char* name, const char* prefix) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!name || !prefix) return MBINK_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringPrepend(name, prefix));
}

// ========== 监听 ==========

int mbink_state_watch(MBinkHandle handle, const char* name,
                        MBinkStateCallback callback, void* user_data) {
    if (!handle) return -1;
    if (!name || !callback) return -1;

    auto ctx = getContext(handle);
    int watchId = ctx->stateManager->watch(name,
        [callback, user_data](const std::string& n, const mbink::json& v) {
            std::string jsonStr = v.dump();
            callback(n.c_str(), jsonStr.c_str(), user_data);
        });

    ctx->watchCallbacks[watchId] = {callback, user_data};
    return watchId;
}

void mbink_state_unwatch(MBinkHandle handle, int watch_id) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->unwatch(watch_id);
    ctx->watchCallbacks.erase(watch_id);
}

// ========== 批量操作 ==========

void mbink_state_batch_begin(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchBegin();
}

void mbink_state_batch_end(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchEnd();
}

// ========== 队列控制 ==========

void mbink_state_set_merge_mode(MBinkHandle handle, bool enable) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->setMergeMode(enable);
}

int mbink_process_queue(MBinkHandle handle) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->stateManager->processQueue();
    return MBINK_OK;
}

int mbink_queue_size(MBinkHandle handle) {
    if (!handle) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->queueSize());
}

// ========== 共享 C 对象 (SharedObject) ==========

MBinkSharedHandle mbink_shared_create(MBinkHandle handle, const char* name) {
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
    JSValue wrapFn = JS_GetPropertyStr(jsCtx, global, "__mbinkWrapSharedObject");
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

    return reinterpret_cast<MBinkSharedHandle>(shared);
}

void mbink_shared_destroy(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    destroySharedObjectInternal(shared, true);
}

// ---- Setter 实现 ----

int mbink_shared_set_int(MBinkSharedHandle shared_handle,
                            const char* key, int64_t value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}

int mbink_shared_set_double(MBinkSharedHandle shared_handle,
                               const char* key, double value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}

int mbink_shared_set_string(MBinkSharedHandle shared_handle,
                               const char* key, const char* value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (value) {
        shared->safeSetProperty(key, nlohmann::json(std::string(value)));
    } else {
        shared->safeSetProperty(key, nlohmann::json(nullptr));
    }
    return MBINK_OK;
}

int mbink_shared_set_bool(MBinkSharedHandle shared_handle,
                             const char* key, bool value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}

int mbink_shared_set_null(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(nullptr));
    return MBINK_OK;
}

int mbink_shared_set_json(MBinkSharedHandle shared_handle,
                             const char* key, const char* json_str) {
    if (!shared_handle || !key || !json_str) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto parsed = nlohmann::json::parse(json_str, nullptr, false);
    if (parsed.is_discarded()) {
        return MBINK_ERROR_INVALID_PARAM;
    }
    shared->safeSetProperty(key, parsed);
    return MBINK_OK;
}

// ---- Getter 实现 ----

int64_t mbink_shared_get_int(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number_integer()) return val.get<int64_t>();
    if (val.is_number_unsigned()) return static_cast<int64_t>(val.get<uint64_t>());
    if (val.is_number_float()) return static_cast<int64_t>(val.get<double>());
    return 0;
}

double mbink_shared_get_double(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0.0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number()) return val.get<double>();
    return 0.0;
}

const char* mbink_shared_get_string(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_string()) {
        return duplicateString(val.get<std::string>().c_str());
    }
    return nullptr;
}

bool mbink_shared_get_bool(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_boolean()) return val.get<bool>();
    return false;
}

const char* mbink_shared_get_json(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_null()) return nullptr;
    auto s = val.dump();
    return duplicateString(s.c_str());
}

// ---- 属性查询 ----

int mbink_shared_get_type(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_TYPE_NULL;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeGetType(key);
}

int mbink_shared_delete(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeDeleteProperty(key);
    return MBINK_OK;
}

bool mbink_shared_has(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeHasProperty(key);
}

// ---- 批量更新 ----

void mbink_shared_batch_begin(MBinkSharedHandle shared_handle) {
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

void mbink_shared_batch_end(MBinkSharedHandle shared_handle) {
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

MBinkLogViewHandle mbink_logview_get(MBinkHandle handle, const char* element_id) {
    if (!handle || !element_id) return nullptr;
    auto ctx = getContext(handle);
    auto element = getElementByIdAs<mbink::HTMLLogViewElement>(ctx, element_id);
    if (!element) {
        setLastError(std::string("logview element not found: ") + element_id);
        return nullptr;
    }
    auto* data = new LogViewHandleData();
    data->element = std::move(element);
    data->mainQueue = &ctx->mainThreadQueue;
    return reinterpret_cast<MBinkLogViewHandle>(data);
}

void mbink_logview_destroy(MBinkLogViewHandle logview_handle) {
    if (!logview_handle) return;
    delete reinterpret_cast<LogViewHandleData*>(logview_handle);
}

int mbink_logview_append(MBinkLogViewHandle logview_handle,
                         const char* level,
                         const char* source,
                         const char* message) {
    if (!logview_handle || !level || !source || !message) {
        return MBINK_ERROR_INVALID_PARAM;
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
    return MBINK_OK;
}

void mbink_logview_clear(MBinkLogViewHandle logview_handle) {
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

const char* mbink_logview_export(MBinkLogViewHandle logview_handle,
                                 const char* format) {
    if (!logview_handle) return nullptr;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && !data->mainQueue->isMainThread()) {
        setLastError("mbink_logview_export must be called on main thread");
        return nullptr;
    }
    auto content = data->element->Export(format ? format : "text");
    return duplicateString(content.c_str());
}

MBinkTerminalHandle mbink_terminal_get(MBinkHandle handle, const char* element_id) {
    if (!handle || !element_id) return nullptr;
    auto ctx = getContext(handle);
    auto element = getElementByIdAs<mbink::HTMLTerminalElement>(ctx, element_id);
    if (!element) {
        setLastError(std::string("terminal element not found: ") + element_id);
        return nullptr;
    }
    auto* data = new TerminalHandleData();
    data->element = std::move(element);
    data->mainQueue = &ctx->mainThreadQueue;
    return reinterpret_cast<MBinkTerminalHandle>(data);
}

void mbink_terminal_destroy(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    delete reinterpret_cast<TerminalHandleData*>(terminal_handle);
}

int mbink_terminal_write(MBinkTerminalHandle terminal_handle, const char* data_str) {
    if (!terminal_handle || !data_str) return MBINK_ERROR_INVALID_PARAM;
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
    return MBINK_OK;
}

void mbink_terminal_clear(MBinkTerminalHandle terminal_handle) {
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

int mbink_terminal_execute(MBinkTerminalHandle terminal_handle, const char* command) {
    if (!terminal_handle || !command) return MBINK_ERROR_INVALID_PARAM;
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
    return MBINK_OK;
}

int mbink_terminal_start_shell(MBinkTerminalHandle terminal_handle, const char* shell) {
    if (!terminal_handle) return MBINK_ERROR_INVALID_PARAM;
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
    return MBINK_OK;
}

int mbink_terminal_send_input(MBinkTerminalHandle terminal_handle, const char* input) {
    if (!terminal_handle || !input) return MBINK_ERROR_INVALID_PARAM;
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
    return MBINK_OK;
}

void mbink_terminal_resize(MBinkTerminalHandle terminal_handle, int rows, int cols) {
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

const char* mbink_terminal_serialize(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return nullptr;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && !data->mainQueue->isMainThread()) {
        setLastError("mbink_terminal_serialize must be called on main thread");
        return nullptr;
    }
    auto content = data->element->Serialize();
    return duplicateString(content.c_str());
}

int mbink_compile_resources(const char* input_path,
                            const char* output_file,
                            const char* encryption_key) {
    try {
        std::string error;
        if (!mbink::resourcepkg::CompileResources(input_path, output_file, encryption_key, error)) {
            setLastError(error.empty() ? "Failed to compile resources" : error);
            return MBINK_ERROR_UNKNOWN;
        }
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_UNKNOWN;
    }
}

int mbink_load_resource_file(const char* package_file,
                             const char* resource_path,
                             const char* encryption_key,
                             void** out_data,
                             size_t* out_size,
                             uint32_t* out_flags) {
    if (!out_data || !out_size) return MBINK_ERROR_INVALID_PARAM;
    *out_data = nullptr;
    *out_size = 0;
    if (out_flags) *out_flags = 0;

    try {
        std::vector<uint8_t> data;
        std::string error;
        if (!mbink::resourcepkg::LoadResourceFile(package_file,
                                                  resource_path,
                                                  encryption_key,
                                                  data,
                                                  out_flags,
                                                  error)) {
            setLastError(error.empty() ? "Failed to load resource file" : error);
            return error == "resource not found" ? MBINK_ERROR_NOT_FOUND : MBINK_ERROR_UNKNOWN;
        }

        void* buffer = std::malloc(data.empty() ? 1 : data.size());
        if (!buffer) {
            setLastError("Out of memory");
            return MBINK_ERROR_UNKNOWN;
        }
        if (!data.empty()) std::memcpy(buffer, data.data(), data.size());

        *out_data = buffer;
        *out_size = data.size();
        return MBINK_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return MBINK_ERROR_UNKNOWN;
    }
}

int mbink_mount_resource_package(MBinkHandle handle,
                                 const char* package_file,
                                 const char* encryption_key,
                                 const char* mount_point) {
    if (!handle) return MBINK_ERROR_INVALID_HANDLE;
    if (!package_file) return MBINK_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    if (!ctx || !ctx->runtime || !ctx->document) return MBINK_ERROR_INVALID_HANDLE;

    const char* normalizedKey = encryption_key ? encryption_key : "";
    const std::string normalizedMount = NormalizeResourcePath(mount_point ? mount_point : "/");

    ctx->mountedResourcePackage = package_file;
    ctx->mountedResourceKey = normalizedKey;
    ctx->mountedResourceMountPoint = normalizedMount;
    ctx->document->SetBasePath(normalizedMount);
    mbink::FetchBindings::SetBasePath(normalizedMount);
    mbink::ImageLoader::SetBasePath(normalizedMount);
    ctx->runtime->SetBaseModulePath(normalizedMount == "/" ? "/index.js" : normalizedMount + "/index.js");
    return MBINK_OK;
}


// ========== 工具函数 ==========

void mbink_free(void* ptr) {
    free(ptr);
}

char* mbink_copy_string(const char* str) {
    return duplicateString(str);
}

const char* mbink_last_error(void) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return g_lastError.c_str();
}
