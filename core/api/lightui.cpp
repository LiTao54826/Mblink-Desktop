/**
 * @file lightui.cpp
 * @brief LightUI C API v2 实现
 *
 * 一个 create() 调用完成全部初始化：
 * Window → Document → TaskScheduler → QuickJSRuntime → DOMBindings →
 * WindowBindings → EventLoop → FetchBindings → StateManager → HostBridge
 */

#include "lightui.h"
#include "core/bridge/state_manager.h"
#include "core/bridge/host_bridge.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/event/loop/event_loop.h"
#include "core/event/loop/task_scheduler.h"
#include "core/network/fetch_bindings.h"

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

namespace {

// ========== 全局状态 ==========

bool g_initialized = false;
std::string g_lastError;
std::mutex g_errorMutex;

// ========== WindowContext ==========

struct WindowContext {
    // 核心组件（完整初始化链）
    std::shared_ptr<lightui::Window> window;
    std::shared_ptr<lightui::Document> document;
    std::shared_ptr<lightui::TaskScheduler> taskScheduler;
    std::unique_ptr<lightui::QuickJSRuntime> runtime;
    std::unique_ptr<lightui::WindowBindings> windowBindings;
    std::unique_ptr<lightui::EventLoop> eventLoop;
    std::unique_ptr<lightui::HostBridge> hostBridge;
    std::unique_ptr<lightui::FetchBindings> fetchBindings;
    std::unique_ptr<lightui::StateManager> stateManager;

    // 回调存储
    std::unordered_map<int, std::pair<LightUIStateCallback, void*>> watchCallbacks;
    std::unordered_map<std::string, std::pair<LightUICallback, void*>> boundFunctions;

    // 事件回调
    LightUIResizeCallback onResizeCallback = nullptr;
    void* onResizeUserData = nullptr;
    LightUIVoidCallback onCloseCallback = nullptr;
    void* onCloseUserData = nullptr;
    LightUIVoidCallback onFocusCallback = nullptr;
    void* onFocusUserData = nullptr;
    LightUIVoidCallback onBlurCallback = nullptr;
    void* onBlurUserData = nullptr;
    LightUIUpdateCallback onUpdateCallback = nullptr;
    void* onUpdateUserData = nullptr;

    bool running = false;
};

// ========== 辅助函数 ==========

void setLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastError = error;
}

int toErrorCode(lightui::LightUIError err) {
    switch (err) {
        case lightui::LightUIError::Ok: return LIGHTUI_OK;
        case lightui::LightUIError::InvalidHandle: return LIGHTUI_ERROR_INVALID_HANDLE;
        case lightui::LightUIError::NotFound: return LIGHTUI_ERROR_NOT_FOUND;
        case lightui::LightUIError::TypeMismatch: return LIGHTUI_ERROR_TYPE_MISMATCH;
        case lightui::LightUIError::IndexOutOfRange: return LIGHTUI_ERROR_OUT_OF_RANGE;
        case lightui::LightUIError::InvalidJson: return LIGHTUI_ERROR_INVALID_PARAM;
        case lightui::LightUIError::AlreadyExists: return LIGHTUI_ERROR_INVALID_PARAM;
        case lightui::LightUIError::InvalidName: return LIGHTUI_ERROR_INVALID_PARAM;
        default: return LIGHTUI_ERROR_UNKNOWN;
    }
}

LightUIType toLightUIType(lightui::LightUIType type) {
    switch (type) {
        case lightui::LightUIType::Null: return LIGHTUI_TYPE_NULL;
        case lightui::LightUIType::Bool: return LIGHTUI_TYPE_BOOL;
        case lightui::LightUIType::Int: return LIGHTUI_TYPE_INT;
        case lightui::LightUIType::Double: return LIGHTUI_TYPE_DOUBLE;
        case lightui::LightUIType::String: return LIGHTUI_TYPE_STRING;
        case lightui::LightUIType::Array: return LIGHTUI_TYPE_ARRAY;
        case lightui::LightUIType::Object: return LIGHTUI_TYPE_OBJECT;
        default: return LIGHTUI_TYPE_NULL;
    }
}

WindowContext* getContext(LightUIHandle handle) {
    return reinterpret_cast<WindowContext*>(handle);
}

char* duplicateString(const std::string& str) {
    char* result = static_cast<char*>(malloc(str.size() + 1));
    if (result) {
        memcpy(result, str.c_str(), str.size() + 1);
    }
    return result;
}

std::string readFileContents(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + filepath);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// 从 LightUIConfig 构建 WindowConfig
lightui::WindowConfig buildWindowConfig(const LightUIConfig* config) {
    lightui::WindowConfig wc;
    wc.title = config->title ? config->title : "LightUI";
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
WindowContext* createWindowContext(const lightui::WindowConfig& wc) {
    auto ctx = new WindowContext();

    // 1. 创建 Window
    ctx->window = std::make_shared<lightui::Window>(wc);

    // 2. 创建 Document → 设置到 Window
    ctx->document = std::make_shared<lightui::Document>();
    ctx->window->SetDocument(ctx->document);

    // 3. 创建 TaskScheduler
    ctx->taskScheduler = std::make_shared<lightui::TaskScheduler>();

    // 4. 注册到 WindowManager
    lightui::WindowManager::Instance().RegisterWindow(ctx->window);

    // 5. 创建 QuickJS Runtime
    ctx->runtime = std::make_unique<lightui::QuickJSRuntime>();

    // 6. 设置 JS Runtime 到 Document
    ctx->document->SetJSRuntime(ctx->runtime.get());

    // 7. 初始化 DOM 绑定
    auto jsCtx = ctx->runtime->GetContext();
    lightui::DOMBindings::Init(jsCtx);
    lightui::DOMBindings::SetGlobalDocument(jsCtx, ctx->document);

    // 8. 创建 WindowBindings + 初始化（setTimeout/setInterval/RAF/DOM/Canvas...）
    ctx->windowBindings = std::make_unique<lightui::WindowBindings>(
        ctx->runtime.get(), ctx->window, ctx->taskScheduler);
    ctx->windowBindings->InitBindings();

    // 9. 创建 EventLoop（使用同一 TaskScheduler）
    ctx->eventLoop = std::make_unique<lightui::EventLoop>(ctx->taskScheduler);
    ctx->eventLoop->SetQuickJSRuntime(ctx->runtime.get());
    lightui::DOMBindings::SetGlobalEventLoop(jsCtx, ctx->eventLoop.get());

    // 10. 创建 FetchBindings
    ctx->fetchBindings = std::make_unique<lightui::FetchBindings>(jsCtx, ctx->taskScheduler);
    ctx->fetchBindings->InitBindings();

    // 11. 创建 StateManager + HostBridge
    ctx->stateManager = std::make_unique<lightui::StateManager>();
    ctx->hostBridge = std::make_unique<lightui::HostBridge>(jsCtx, ctx->stateManager.get());
    ctx->hostBridge->registerGlobal();

    return ctx;
}

} // anonymous namespace

// ========== 生命周期 ==========

int lightui_init(void) {
    if (g_initialized) return LIGHTUI_OK;
    g_initialized = true;
    return LIGHTUI_OK;
}

void lightui_cleanup(void) {
    g_initialized = false;
}

const char* lightui_version(void) {
    return "0.1.0";
}

// ========== 窗口管理 ==========

LightUIHandle lightui_create(const char* title, int width, int height) {
    if (!g_initialized) {
        setLastError("LightUI not initialized");
        return nullptr;
    }

    try {
        lightui::WindowConfig wc;
        wc.title = title ? title : "LightUI";
        wc.width = width > 0 ? width : 800;
        wc.height = height > 0 ? height : 600;
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<LightUIHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

LightUIHandle lightui_create_ex(const LightUIConfig* config) {
    if (!g_initialized) {
        setLastError("LightUI not initialized");
        return nullptr;
    }
    if (!config) {
        setLastError("config is null");
        return nullptr;
    }

    try {
        auto wc = buildWindowConfig(config);
        auto ctx = createWindowContext(wc);
        return reinterpret_cast<LightUIHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

LightUIConfig lightui_default_config(void) {
    LightUIConfig config = {};
    config.title = "LightUI";
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

void lightui_destroy(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);

    // 停止事件循环
    if (ctx->eventLoop && ctx->running) {
        ctx->eventLoop->Stop();
        ctx->running = false;
    }

    // 清理 DOM 绑定
    if (ctx->runtime) {
        lightui::DOMBindings::Cleanup(ctx->runtime->GetContext());
    }

    // 从 WindowManager 注销
    if (ctx->window) {
        lightui::WindowManager::Instance().UnregisterWindow(ctx->window);
    }

    delete ctx;
}

void lightui_run(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return;
    if (ctx->running) return;

    ctx->running = true;

    // 设置 update callback：处理 StateManager 队列 + HostBridge 事件 + 用户回调
    ctx->eventLoop->SetUpdateCallback([ctx](float dt) {
        // 处理状态变更队列
        if (ctx->stateManager) {
            ctx->stateManager->processQueue();
        }
        // 刷新 HostBridge 事件队列到 JS 端
        if (ctx->hostBridge) {
            ctx->hostBridge->flushEvents();
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

void lightui_stop(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    if (ctx->eventLoop) {
        ctx->eventLoop->Stop();
    }
}

bool lightui_poll_events(LightUIHandle handle) {
    if (!handle) return false;
    auto ctx = getContext(handle);
    if (!ctx->eventLoop) return false;

    // 处理状态队列
    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
    }

    // 单次事件循环迭代
    ctx->eventLoop->RunOnce();
    return !ctx->eventLoop->ShouldQuit();
}

// ========== 窗口属性 ==========

int lightui_set_title(LightUIHandle handle, const char* title) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (title && ctx->window) {
        ctx->window->SetTitle(title);
    }
    return LIGHTUI_OK;
}

int lightui_set_size(LightUIHandle handle, int width, int height) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetSize(width, height);
    }
    return LIGHTUI_OK;
}

int lightui_get_size(LightUIHandle handle, int* width, int* height) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int w = 0, h = 0;
        ctx->window->GetSize(&w, &h);
        if (width) *width = w;
        if (height) *height = h;
    }
    return LIGHTUI_OK;
}

int lightui_set_position(LightUIHandle handle, int x, int y) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetPosition(x, y);
    }
    return LIGHTUI_OK;
}

int lightui_get_position(LightUIHandle handle, int* x, int* y) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        int px = 0, py = 0;
        ctx->window->GetPosition(&px, &py);
        if (x) *x = px;
        if (y) *y = py;
    }
    return LIGHTUI_OK;
}

int lightui_set_min_size(LightUIHandle handle, int width, int height) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMinSize(width, height);
    }
    return LIGHTUI_OK;
}

int lightui_set_max_size(LightUIHandle handle, int width, int height) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) {
        ctx->window->SetMaxSize(width, height);
    }
    return LIGHTUI_OK;
}

int lightui_minimize(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Minimize();
    return LIGHTUI_OK;
}

int lightui_maximize(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Maximize();
    return LIGHTUI_OK;
}

int lightui_restore(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Restore();
    return LIGHTUI_OK;
}

int lightui_show(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Show();
    return LIGHTUI_OK;
}

int lightui_hide(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->Hide();
    return LIGHTUI_OK;
}

int lightui_set_fullscreen(LightUIHandle handle, bool fullscreen) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetFullscreen(fullscreen);
    return LIGHTUI_OK;
}

int lightui_set_resizable(LightUIHandle handle, bool resizable) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetResizable(resizable);
    return LIGHTUI_OK;
}

int lightui_set_borderless(LightUIHandle handle, bool borderless) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetBorderless(borderless);
    return LIGHTUI_OK;
}

int lightui_set_always_on_top(LightUIHandle handle, bool on_top) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (ctx->window) ctx->window->SetAlwaysOnTop(on_top);
    return LIGHTUI_OK;
}

// ========== UI 加载 ==========

int lightui_load_html(LightUIHandle handle, const char* html) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!html) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (ctx->document) {
        ctx->document->LoadHTML(html);
        // 执行 HTML 中嵌入的 <script> 标签
        ctx->document->ExecuteScripts();
    }
    return LIGHTUI_OK;
}

int lightui_load_html_file(LightUIHandle handle, const char* filepath) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!filepath) return LIGHTUI_ERROR_INVALID_PARAM;
    try {
        std::string content = readFileContents(filepath);
        auto ctx = getContext(handle);
        if (ctx->document) {
            ctx->document->LoadHTML(content);
            // 执行 HTML 中嵌入的 <script> 标签
            ctx->document->ExecuteScripts();
        }
        return LIGHTUI_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_eval_js(LightUIHandle handle, const char* js_code) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!js_code) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return LIGHTUI_ERROR_INVALID_HANDLE;

    try {
        ctx->runtime->Eval(js_code);
        return LIGHTUI_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return LIGHTUI_ERROR_JS_ERROR;
    }
}

int lightui_eval_module(LightUIHandle handle, const char* code, const char* filename) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!code) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return LIGHTUI_ERROR_INVALID_HANDLE;

    try {
        auto jsCtx = ctx->runtime->GetContext();
        const char* fname = filename ? filename : "<module>";
        JSValue result = JS_Eval(jsCtx, code, strlen(code), fname, JS_EVAL_TYPE_MODULE);
        if (JS_IsException(result)) {
            JSValue exc = JS_GetException(jsCtx);
            const char* err = JS_ToCString(jsCtx, exc);
            if (err) {
                setLastError(err);
                JS_FreeCString(jsCtx, err);
            }
            JS_FreeValue(jsCtx, exc);
            JS_FreeValue(jsCtx, result);
            return LIGHTUI_ERROR_JS_ERROR;
        }
        JS_FreeValue(jsCtx, result);
        return LIGHTUI_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return LIGHTUI_ERROR_JS_ERROR;
    }
}

int lightui_load_js_file(LightUIHandle handle, const char* filepath) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!filepath) return LIGHTUI_ERROR_INVALID_PARAM;
    try {
        std::string code = readFileContents(filepath);
        return lightui_eval_js(handle, code.c_str());
    } catch (const std::exception& e) {
        setLastError(e.what());
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_load_bytecode(LightUIHandle handle, const void* data, size_t size) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!data || size == 0) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (!ctx->runtime) return LIGHTUI_ERROR_INVALID_HANDLE;

    try {
        auto jsCtx = ctx->runtime->GetContext();
        JSValue obj = JS_ReadObject(jsCtx, static_cast<const uint8_t*>(data), size, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(obj)) {
            setLastError("Failed to read bytecode");
            return LIGHTUI_ERROR_JS_ERROR;
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
            return LIGHTUI_ERROR_JS_ERROR;
        }
        JS_FreeValue(jsCtx, result);
        return LIGHTUI_OK;
    } catch (const std::exception& e) {
        setLastError(e.what());
        return LIGHTUI_ERROR_JS_ERROR;
    }
}

// ========== 函数绑定 ==========

int lightui_bind(LightUIHandle handle, const char* name,
                 LightUICallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !callback) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    ctx->boundFunctions[name] = {callback, user_data};

    // 通过 HostBridge 注册，这样 JS 端可以通过 py.name() 调用
    if (ctx->hostBridge) {
        LightUICallback cb = callback;
        void* ud = user_data;
        ctx->hostBridge->bind(name, [cb, ud](const std::string& args) -> std::string {
            char* result = cb(args.c_str(), ud);
            if (result) {
                std::string ret(result);
                // 注意：不调用 free(result)
                // 返回值的内存由回调方自行管理（Python ctypes 自动维护引用，
                // C 回调可使用 static buffer，其他语言各自处理）
                // std::string 已经拷贝了数据，后续使用 ret 即可
                return ret;
            }
            return "null";
        });
    }
    return LIGHTUI_OK;
}

void lightui_unbind(LightUIHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->boundFunctions.erase(name);
    if (ctx->hostBridge) {
        ctx->hostBridge->unbind(name);
    }
}

// ========== 事件回调 ==========

int lightui_on_resize(LightUIHandle handle, LightUIResizeCallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
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
    return LIGHTUI_OK;
}

int lightui_on_close(LightUIHandle handle, LightUIVoidCallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
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
    return LIGHTUI_OK;
}

int lightui_on_focus(LightUIHandle handle, LightUIVoidCallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
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
    return LIGHTUI_OK;
}

int lightui_on_blur(LightUIHandle handle, LightUIVoidCallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
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
    return LIGHTUI_OK;
}

int lightui_on_update(LightUIHandle handle, LightUIUpdateCallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->onUpdateCallback = callback;
    ctx->onUpdateUserData = user_data;
    return LIGHTUI_OK;
}

// ========== 事件发送 ==========

int lightui_emit(LightUIHandle handle, const char* event_name, const char* data_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!event_name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    if (ctx->hostBridge) {
        ctx->hostBridge->emit(event_name, data_json ? data_json : "null");
    }
    return LIGHTUI_OK;
}

// ========== DevTools ==========

int lightui_devtools_open(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    // DevTools 功能暂不实现，预留接口
    return LIGHTUI_OK;
}

int lightui_devtools_close(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    // DevTools 功能暂不实现，预留接口
    return LIGHTUI_OK;
}

// ========== 状态创建 ==========

int lightui_state_create_null(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createNull(name));
}

int lightui_state_create_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createBool(name, value));
}

int lightui_state_create_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createInt(name, value));
}

int lightui_state_create_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createDouble(name, value));
}

int lightui_state_create_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createString(name, value ? value : ""));
}

int lightui_state_create_array(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createArray(name));
}

int lightui_state_create_object(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createObject(name));
}

int lightui_state_create_json(LightUIHandle handle, const char* name, const char* json_str) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return LIGHTUI_ERROR_INVALID_PARAM;

    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(json_str);
        return toErrorCode(ctx->stateManager->createJson(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}


// ========== 状态查询 ==========

bool lightui_state_exists(LightUIHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->exists(name);
}

LightUIType lightui_state_type(LightUIHandle handle, const char* name) {
    if (!handle || !name) return LIGHTUI_TYPE_NULL;
    auto ctx = getContext(handle);
    return toLightUIType(ctx->stateManager->type(name));
}

void lightui_state_delete(LightUIHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->stateManager->remove(name);
    ctx->stateManager->processQueue();
}

// ========== 状态读取（直接类型） ==========

bool lightui_state_get_bool(LightUIHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->getBool(name);
}

int64_t lightui_state_get_int(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getInt(name);
}

double lightui_state_get_double(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0.0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getDouble(name);
}

const char* lightui_state_get_string(LightUIHandle handle, const char* name) {
    if (!handle || !name) return "";
    auto ctx = getContext(handle);
    return ctx->stateManager->getString(name).c_str();
}

int lightui_state_get_length(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->getLength(name));
}

// ========== 状态读取（JSON） ==========

char* lightui_state_get_json(LightUIHandle handle, const char* name) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getJson(name);
    return duplicateString(j.dump());
}

char* lightui_state_get_at(LightUIHandle handle, const char* name, int index) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getAt(name, index);
    return duplicateString(j.dump());
}

char* lightui_state_get_key(LightUIHandle handle, const char* name, const char* key) {
    if (!handle || !name || !key) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getKey(name, key);
    return duplicateString(j.dump());
}

// ========== 状态写入 ==========

int lightui_state_set_null(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setNull(name));
}

int lightui_state_set_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setBool(name, value));
}

int lightui_state_set_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setInt(name, value));
}

int lightui_state_set_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setDouble(name, value));
}

int lightui_state_set_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setString(name, value ? value : ""));
}

int lightui_state_set_json(LightUIHandle handle, const char* name, const char* json_str) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(json_str);
        return toErrorCode(ctx->stateManager->setJson(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}


// ========== 数组操作 ==========

int lightui_state_array_push(LightUIHandle handle, const char* name, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayPush(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_push_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_push_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_push_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value ? value : ""));
}

int lightui_state_array_push_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_pop(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPop(name));
}

int lightui_state_array_shift(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayShift(name));
}

int lightui_state_array_unshift(LightUIHandle handle, const char* name, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayUnshift(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_remove(LightUIHandle handle, const char* name, int index) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayRemove(name, index));
}

int lightui_state_array_clear(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayClear(name));
}

int lightui_state_array_set(LightUIHandle handle, const char* name, int index, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arraySet(name, index, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_set_int(LightUIHandle handle, const char* name, int index, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int lightui_state_array_set_double(LightUIHandle handle, const char* name, int index, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int lightui_state_array_set_string(LightUIHandle handle, const char* name, int index, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value ? value : ""));
}


// ========== 对象操作 ==========

int lightui_state_object_set(LightUIHandle handle, const char* name, const char* key, const char* value_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key || !value_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(value_json);
        return toErrorCode(ctx->stateManager->objectSet(name, key, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_object_set_int(LightUIHandle handle, const char* name, const char* key, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_set_double(LightUIHandle handle, const char* name, const char* key, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_set_string(LightUIHandle handle, const char* name, const char* key, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value ? value : ""));
}

int lightui_state_object_set_bool(LightUIHandle handle, const char* name, const char* key, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_remove(LightUIHandle handle, const char* name, const char* key) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectRemove(name, key));
}

int lightui_state_object_clear(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectClear(name));
}

// ========== 数值操作 ==========

int lightui_state_increment(LightUIHandle handle, const char* name, double delta) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->increment(name, delta));
}

int lightui_state_multiply(LightUIHandle handle, const char* name, double factor) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->multiply(name, factor));
}

// ========== 字符串操作 ==========

int lightui_state_string_append(LightUIHandle handle, const char* name, const char* suffix) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !suffix) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringAppend(name, suffix));
}

int lightui_state_string_prepend(LightUIHandle handle, const char* name, const char* prefix) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !prefix) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringPrepend(name, prefix));
}

// ========== 监听 ==========

int lightui_state_watch(LightUIHandle handle, const char* name,
                        LightUIStateCallback callback, void* user_data) {
    if (!handle) return -1;
    if (!name || !callback) return -1;

    auto ctx = getContext(handle);
    int watchId = ctx->stateManager->watch(name,
        [callback, user_data](const std::string& n, const lightui::json& v) {
            std::string jsonStr = v.dump();
            callback(n.c_str(), jsonStr.c_str(), user_data);
        });

    ctx->watchCallbacks[watchId] = {callback, user_data};
    return watchId;
}

void lightui_state_unwatch(LightUIHandle handle, int watch_id) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->unwatch(watch_id);
    ctx->watchCallbacks.erase(watch_id);
}

// ========== 批量操作 ==========

void lightui_state_batch_begin(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchBegin();
}

void lightui_state_batch_end(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchEnd();
}

// ========== 队列控制 ==========

void lightui_state_set_merge_mode(LightUIHandle handle, bool enable) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->setMergeMode(enable);
}

int lightui_process_queue(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->stateManager->processQueue();
    return LIGHTUI_OK;
}

int lightui_queue_size(LightUIHandle handle) {
    if (!handle) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->queueSize());
}

// ========== 工具函数 ==========

void lightui_free(void* ptr) {
    free(ptr);
}

const char* lightui_last_error(void) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return g_lastError.c_str();
}
