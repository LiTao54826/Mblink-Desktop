/**
 * @file mbink.cpp
 * @brief MBink C API v2 实现
 *
 * 一个 create() 调用完成全部初始化：
 * Window → Document → TaskScheduler → QuickJSRuntime → DOMBindings →
 * WindowBindings → EventLoop → FetchBindings → StateManager → HostBridge
 */

#include "mbink.h"
#include "core/bridge/state_manager.h"
#include "core/bridge/host_bridge.h"
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
#include "core/quickjs/dom_binding_map.h"
#include "tools/esm_loader/embedded_js.h"
#include "core/utils/encoding_utils.h"

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <filesystem>

namespace {

namespace fs = std::filesystem;

// ========== 全局状态 ==========

bool g_initialized = false;
std::string g_lastError;
std::mutex g_errorMutex;

// ========== WindowContext ==========

// ========== SharedObject 结构体 ==========
// Python/JS 共享的 C 对象，内含 QuickJS JSValue
struct SharedObjectData {
    JSContext* ctx = nullptr;
    JSValue js_obj = JS_UNDEFINED;     // 实际的 JS 对象（globalThis.<name>）
    JSValue updater_func = JS_UNDEFINED; // __onSharedUpdate 函数缓存
    std::string name;                   // globalThis 上的名字
    int batch_depth = 0;                // 批量层级（支持嵌套）
    int pending_updates = 0;            // 批量模式中的待处理更新数
    bool pending_notify_ = false;       // 是否有待处理的非批量通知（延迟刷新用）

    void refreshUpdater() {
        if (!ctx) return;
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue updater = JS_GetPropertyStr(ctx, global, "__onSharedUpdate");
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

    void beginBatch() {
        batch_depth++;
        if (batch_depth == 1) {
            pending_updates = 0;
        }
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
    void notifyUpdate(const char* /*key*/) {
        if (batch_depth > 0) {
            pending_updates++;
            return;
        }
        // 只设置标志，不立即调用 JS_Call
        pending_notify_ = true;
    }

    // flushPendingNotify：由事件循环在安全时机调用，真正触发 JS 通知。
    // 此时 JS 调用栈已清空，调用 JS_Call 是安全的。
    void flushPendingNotify() {
        if (!pending_notify_) return;
        pending_notify_ = false;
        refreshUpdater();
        if (!JS_IsUndefined(updater_func) && !JS_IsNull(updater_func)) {
            JSValue arg = JS_NewString(ctx, "*");
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
        if (pending_updates > 0) {
            pending_updates = 0;
            // 批量结束时同样延迟通知，保持一致性
            pending_notify_ = true;
        }
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
};

struct TerminalHandleData {
    std::shared_ptr<mbink::HTMLTerminalElement> element;
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
        for (auto& kv : ctx->sharedObjects) {
            if (kv.second) {
                kv.second->endBatch();
            }
        }
    };

    for (auto& kv : ctx->sharedObjects) {
        if (kv.second) {
            kv.second->beginBatch();
        }
    }

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
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + filepath);
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

    // 1. 停止事件循环
    if (ctx->eventLoop && ctx->running) {
        ctx->eventLoop->Stop();
        ctx->running = false;
    }

    // 2. JS 清理：unmount Preact + 清空全局引用（必须在 DOMBindings::Cleanup 之前）
    if (ctx->hostBridge) {
        ctx->hostBridge->cancelPendingPromises("Window destroyed");
    }

    if (ctx->runtime) {
        auto jsCtx = ctx->runtime->GetContext();
        const char* cleanupScript =
            "(function(){"
            "  if(typeof __preactCleanup==='function'){try{__preactCleanup();}catch(e){}}"
            "  if(typeof __preactHooksCleanup==='function'){try{__preactHooksCleanup();}catch(e){}}"
            "  var keys=['Preact','PreactHooks','preact','preactHooks',"
            "            '__preactCleanup','__preactHooksCleanup',"
            "            '__onSharedUpdate','data','backend','py'];"
            "  for(var i=0;i<keys.length;i++){"
            "    try{globalThis[keys[i]]=undefined;}catch(e){}"
            "  }"
            "})();";
        JSValue res = JS_Eval(jsCtx, cleanupScript, strlen(cleanupScript),
                              "<cleanup>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(res)) {
            JSValue exc = JS_GetException(jsCtx);
            JS_FreeValue(jsCtx, exc);
        }
        JS_FreeValue(jsCtx, res);
    }

    // 3. 清理 DOM 绑定
    if (ctx->runtime) {
        mbink::DOMBindings::Cleanup(ctx->runtime->GetContext());
    }

    // 4. 释放 document
    ctx->document.reset();

    // 5. 清理 DOMBindingMap（Node* -> JSValue 映射）
    mbink::DOMBindingMap::GetInstance().Clear();

    // 6. 释放 HostBridge 和 StateManager
    if (ctx->stateManager) {
        ctx->stateManager->clearWatchers();
    }
    ctx->hostBridge.reset();
    ctx->stateManager.reset();

    // 7. GC
    if (ctx->runtime) {
        ctx->runtime->RunGC();
    }

    // 8. 释放 runtime
    ctx->runtime.reset();

    // 9. 释放 eventLoop（SDL_DestroyCursor 必须在 SDL_Quit 之前）
    ctx->eventLoop.reset();

    // 10. 释放 windowBindings/fetchBindings/taskScheduler
    //     windowBindings 持有 shared_ptr<Window>，必须在 window 析构之前 reset
    ctx->windowBindings.reset();
    ctx->fetchBindings.reset();
    ctx->taskScheduler.reset();

    // 11. 从 WindowManager 注销（不调用 window.reset()，避免 Window::~Window 卡在 SDL/Skia 清理）
    if (ctx->window) {
        mbink::WindowManager::Instance().UnregisterWindow(ctx->window);
    }

    ctx->sharedObjects.clear();
    ctx->watchCallbacks.clear();
    ctx->boundFunctions.clear();
    ctx->boundAsyncFunctions.clear();

    // SDL 有后台线程无法正常退出，参考 esm_loader 使用强制退出
#ifdef _WIN32
    ::TerminateProcess(::GetCurrentProcess(), 0);
#else
    std::quick_exit(0);
#endif
}

void mbink_run(MBinkHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return;
    if (ctx->running) return;

    ctx->running = true;

    // 设置 update callback：处理 StateManager 队列 + HostBridge 事件 + SharedObject 延迟通知 + 用户回调
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
        // 刷新所有 SharedObject 的延迟通知（由 notifyUpdate/flushBatch 标记的 pending_notify_）
        // 在事件循环帧中调用，此时 JS 调用栈已清空，调用 JS_Call 是安全的
        for (auto& kv : ctx->sharedObjects) {
            kv.second->flushPendingNotify();
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

    // 处理状态队列
    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    // 刷新 SharedObject 的延迟通知
    for (auto& kv : ctx->sharedObjects) {
        kv.second->flushPendingNotify();
    }

    // 单次事件循环迭代
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
        std::string content = readFileContents(filepath);
        auto ctx = getContext(handle);
        if (ctx->document) {
            fs::path html_dir = fs::absolute(Utf8PathToFsPath(filepath)).parent_path();
            std::string base_path = NormalizeFsPath(html_dir);
            ctx->document->SetBasePath(base_path);
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
        ctx->runtime->LoadModuleFile(filepath);
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
        JSValue obj = JS_ReadObject(jsCtx, static_cast<const uint8_t*>(data), size, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(obj)) {
            setLastError("Failed to read bytecode");
            return MBINK_ERROR_JS_ERROR;
        }
        JSValue result = JS_EvalFunction(jsCtx, obj);
        if (JS_IsException(result)) {
            JSValue exc = JS_GetException(jsCtx);
            const char* err = JS_ToCString(jsCtx, exc);
            if (err) {
                setLastError(err);
                JS_FreeCString(jsCtx, err);
            }
            JS_FreeValue(jsCtx, exc);
            JS_FreeValue(jsCtx, result);
            return MBINK_ERROR_JS_ERROR;
        }
        JS_FreeValue(jsCtx, result);
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

    // 创建 SharedObjectData
    auto* shared = new SharedObjectData();
    shared->ctx = jsCtx;
    shared->name = name;
    shared->js_obj = JS_NewObject(jsCtx);

    // 注册为 globalThis.<name>
    JSValue global = JS_GetGlobalObject(jsCtx);
    JS_DupValue(jsCtx, shared->js_obj);
    JS_SetPropertyStr(jsCtx, global, name, shared->js_obj);

    // 预取 __onSharedUpdate，后续 flush 时会再次刷新，兼容延后注入/用户覆盖
    shared->refreshUpdater();
    JS_FreeValue(jsCtx, global);

    // 存储到 WindowContext
    ctx->sharedObjects[name] = shared;

    return reinterpret_cast<MBinkSharedHandle>(shared);
}

void mbink_shared_destroy(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    if (shared->ctx) {
        // 从 globalThis 移除（必须先 FreeAtom 避免 atom leak）
        JSValue global = JS_GetGlobalObject(shared->ctx);
        JSAtom atom = JS_NewAtom(shared->ctx, shared->name.c_str());
        JS_DeleteProperty(shared->ctx, global, atom, 0);
        JS_FreeAtom(shared->ctx, atom);
        JS_FreeValue(shared->ctx, global);

        // 释放 JS 值
        JS_FreeValue(shared->ctx, shared->js_obj);
        if (!JS_IsUndefined(shared->updater_func)) {
            JS_FreeValue(shared->ctx, shared->updater_func);
        }
    }
    delete shared;
}

// ---- Setter 实现 ----

int mbink_shared_set_int(MBinkSharedHandle shared_handle,
                            const char* key, int64_t value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, JS_NewInt64(shared->ctx, value));
    // 每次 set 后刷新 updater_func 缓存（用户可能在 set 之后才定义 __onSharedUpdate）
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

int mbink_shared_set_double(MBinkSharedHandle shared_handle,
                               const char* key, double value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, JS_NewFloat64(shared->ctx, value));
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

int mbink_shared_set_string(MBinkSharedHandle shared_handle,
                               const char* key, const char* value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JS_SetPropertyStr(shared->ctx, shared->js_obj, key,
        value ? JS_NewString(shared->ctx, value) : JS_NULL);
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

int mbink_shared_set_bool(MBinkSharedHandle shared_handle,
                             const char* key, bool value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, JS_NewBool(shared->ctx, value));
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

int mbink_shared_set_null(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, JS_NULL);
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

int mbink_shared_set_json(MBinkSharedHandle shared_handle,
                             const char* key, const char* json_str) {
    if (!shared_handle || !key || !json_str) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JSValue val = JS_ParseJSON(shared->ctx, json_str, strlen(json_str), "<json>");
    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(shared->ctx);
        JS_FreeValue(shared->ctx, exc);
        return MBINK_ERROR_INVALID_PARAM;
    }
    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, val);
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}

// ---- Getter 实现 ----

int64_t mbink_shared_get_int(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    int64_t result = 0;
    JS_ToInt64(shared->ctx, &result, val);
    JS_FreeValue(shared->ctx, val);
    return result;
}

double mbink_shared_get_double(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0.0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    double result = 0.0;
    JS_ToFloat64(shared->ctx, &result, val);
    JS_FreeValue(shared->ctx, val);
    return result;
}

const char* mbink_shared_get_string(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    const char* str = JS_ToCString(shared->ctx, val);
    JS_FreeValue(shared->ctx, val);
    if (!str) return nullptr;
    char* result = duplicateString(str);
    JS_FreeCString(shared->ctx, str);
    return result;
}

bool mbink_shared_get_bool(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    int result = JS_ToBool(shared->ctx, val);
    JS_FreeValue(shared->ctx, val);
    return result != 0;
}

const char* mbink_shared_get_json(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    JSValue json_val = JS_JSONStringify(shared->ctx, val, JS_UNDEFINED, JS_UNDEFINED);
    JS_FreeValue(shared->ctx, val);
    if (JS_IsException(json_val)) {
        JSValue exc = JS_GetException(shared->ctx);
        JS_FreeValue(shared->ctx, exc);
        return nullptr;
    }
    const char* str = JS_ToCString(shared->ctx, json_val);
    JS_FreeValue(shared->ctx, json_val);
    if (!str) return nullptr;
    char* result = duplicateString(str);
    JS_FreeCString(shared->ctx, str);
    return result;
}

// ---- 属性查询 ----

int mbink_shared_get_type(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_TYPE_NULL;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSValue val = JS_GetPropertyStr(shared->ctx, shared->js_obj, key);
    int tag = JS_VALUE_GET_TAG(val);
    int result;
    if (JS_IsNull(val) || JS_IsUndefined(val)) result = MBINK_TYPE_NULL;
    else if (JS_IsBool(val)) result = MBINK_TYPE_BOOL;
    else if (tag == JS_TAG_INT) result = MBINK_TYPE_INT;
    else if (JS_TAG_IS_FLOAT64(tag)) result = MBINK_TYPE_DOUBLE;
    else if (JS_IsString(val)) result = MBINK_TYPE_STRING;
    else if (JS_IsArray(val)) result = MBINK_TYPE_ARRAY;
    else if (JS_IsObject(val)) result = MBINK_TYPE_OBJECT;
    else result = MBINK_TYPE_NULL;
    JS_FreeValue(shared->ctx, val);
    return result;
}

int mbink_shared_delete(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSAtom atom = JS_NewAtom(shared->ctx, key);
    JS_DeleteProperty(shared->ctx, shared->js_obj, atom, 0);
    JS_FreeAtom(shared->ctx, atom);
    shared->notifyUpdate(key);
    return MBINK_OK;
}

bool mbink_shared_has(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    JSAtom atom = JS_NewAtom(shared->ctx, key);
    int has = JS_HasProperty(shared->ctx, shared->js_obj, atom);
    JS_FreeAtom(shared->ctx, atom);
    return has > 0;
}

// ---- 批量更新 ----

void mbink_shared_batch_begin(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->beginBatch();
}

void mbink_shared_batch_end(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->endBatch();
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
    data->element->Append(level, source, message);
    return MBINK_OK;
}

void mbink_logview_clear(MBinkLogViewHandle logview_handle) {
    if (!logview_handle) return;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    data->element->Clear();
}

const char* mbink_logview_export(MBinkLogViewHandle logview_handle,
                                 const char* format) {
    if (!logview_handle) return nullptr;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
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
    return reinterpret_cast<MBinkTerminalHandle>(data);
}

void mbink_terminal_destroy(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    delete reinterpret_cast<TerminalHandleData*>(terminal_handle);
}

int mbink_terminal_write(MBinkTerminalHandle terminal_handle, const char* data_str) {
    if (!terminal_handle || !data_str) return MBINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->Write(data_str);
    return MBINK_OK;
}

void mbink_terminal_clear(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->Clear();
}

int mbink_terminal_execute(MBinkTerminalHandle terminal_handle, const char* command) {
    if (!terminal_handle || !command) return MBINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->Execute(command);
    return MBINK_OK;
}

int mbink_terminal_start_shell(MBinkTerminalHandle terminal_handle, const char* shell) {
    if (!terminal_handle) return MBINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->StartShell(shell ? shell : "");
    return MBINK_OK;
}

int mbink_terminal_send_input(MBinkTerminalHandle terminal_handle, const char* input) {
    if (!terminal_handle || !input) return MBINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->SendInput(input);
    return MBINK_OK;
}

void mbink_terminal_resize(MBinkTerminalHandle terminal_handle, int rows, int cols) {
    if (!terminal_handle) return;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    data->element->Resize(rows, cols);
}

const char* mbink_terminal_serialize(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return nullptr;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    auto content = data->element->Serialize();
    return duplicateString(content.c_str());
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
