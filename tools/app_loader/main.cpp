/**
 * @file main.cpp
 * @brief MBink App Loader - 通用的 app.js 加载器
 * 
 * 支持两种模式：
 * 1. 命令行模式: app_loader.exe <app.js路径> [选项]
 * 2. 嵌入模式: 当 exe 末尾包含有效的 payload 时，自动加载嵌入的字节码
 * 
 * 选项:
 *   --width <宽度>      窗口宽度 (默认: 800)
 *   --height <高度>     窗口高度 (默认: 600)
 *   --title <标题>      窗口标题 (默认: MBink App)
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/event/task_scheduler.h"
#include "core/event/event_loop.h"
#include "core/devtools/devtools_manager.h"
#include "core/render/text/font_manager.h"
#include "core/render/image/image_loader.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "embedded_js.h"
#include "asset_manager.h"
#include "payload.h"
#include "bytecode_compiler.h"
#include <SDL3/SDL.h>

extern "C" {
#include "quickjs/quickjs.h"
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace lightui;
namespace fs = std::filesystem;

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 打印帮助信息
void PrintUsage(const char* program_name) {
    std::cout << "MBink App Loader - 通用 app.js 加载器" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << program_name << " <app.js路径> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --width <宽度>      窗口宽度 (默认: 800)" << std::endl;
    std::cout << "  --height <高度>     窗口高度 (默认: 600)" << std::endl;
    std::cout << "  --title <标题>      窗口标题 (默认: MBink App)" << std::endl;
    std::cout << "  --devtools          启动时打开开发者工具" << std::endl;
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "快捷键:" << std::endl;
    std::cout << "  F12                 切换开发者工具" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " my_app.js" << std::endl;
    std::cout << "  " << program_name << " my_app.js --width 1024 --height 768" << std::endl;
    std::cout << "  " << program_name << " my_app.js --devtools" << std::endl;
}

// 查找 Preact 库路径（仅在不使用嵌入资源时需要）
std::string FindPreactPath(const std::string& app_path) {
    fs::path app_dir = fs::path(app_path).parent_path();
    
    // 尝试多个可能的路径
    std::vector<std::string> search_paths = {
        (app_dir / "js" / "preact").string(),
        (app_dir / "preact").string(),
        (app_dir.parent_path() / "js" / "preact").string(),
        "js/preact",
        "../js/preact",
        "../../js/preact",
        "examples/demo_html/js/preact",
        "../examples/demo_html/js/preact",
        "../../examples/demo_html/js/preact",
    };
    
    for (const auto& path : search_paths) {
        if (fs::exists(path + "/preact.js")) {
            return path;
        }
    }
    
    return "";
}

// 使用嵌入的 JS 资源加载库
bool LoadEmbeddedLibraries(QuickJSRuntime* runtime, bool verbose = true) {
    using namespace lightui::embedded;
    
    if (!HasEmbeddedJS()) {
        return false;
    }
    
    try {
        // 1. 加载 DOM polyfills
        auto polyfills = GetDomPolyfillsJS();
        if (!polyfills.empty()) {
            std::string polyfills_str(polyfills);
            runtime->Eval(polyfills_str, "dom.js");
            if (verbose) std::cout << "  ✓ DOM polyfills loaded (embedded, " << polyfills.size() << " bytes)" << std::endl;
        }
        
        // 2. 加载 Preact
        auto preact = GetPreactJS();
        if (!preact.empty()) {
            std::string preact_str(preact);
            runtime->Eval(preact_str, "preact.js");
            if (verbose) std::cout << "  ✓ Preact loaded (embedded, " << preact.size() << " bytes)" << std::endl;
        }
        
        // 3. 加载 Hooks
        auto hooks = GetHooksJS();
        if (!hooks.empty()) {
            std::string hooks_str(hooks);
            runtime->Eval(hooks_str, "hooks.js");
            if (verbose) std::cout << "  ✓ Hooks loaded (embedded, " << hooks.size() << " bytes)" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Error loading embedded JS: " << e.what() << std::endl;
        return false;
    }
}

// JS API: loadAsset(path) - 返回 ArrayBuffer
static JSValue js_loadAsset(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "loadAsset requires a path argument");
    }
    
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) {
        return JS_ThrowTypeError(ctx, "loadAsset path must be a string");
    }
    
    std::vector<uint8_t> data;
    bool found = AssetManager::Instance().GetAsset(path, data);
    JS_FreeCString(ctx, path);
    
    if (!found) {
        return JS_NULL;
    }
    
    // 创建 ArrayBuffer
    JSValue array_buffer = JS_NewArrayBufferCopy(ctx, data.data(), data.size());
    return array_buffer;
}

// JS API: getAssetUrl(path) - 返回 data:// URL
static JSValue js_getAssetUrl(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "getAssetUrl requires a path argument");
    }
    
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) {
        return JS_ThrowTypeError(ctx, "getAssetUrl path must be a string");
    }
    
    std::string url = AssetManager::Instance().GetAssetDataUrl(path);
    JS_FreeCString(ctx, path);
    
    if (url.empty()) {
        return JS_NULL;
    }
    
    return JS_NewString(ctx, url.c_str());
}

// JS API: hasAsset(path) - 检查资源是否存在
static JSValue js_hasAsset(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_FALSE;
    }
    
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) {
        return JS_FALSE;
    }
    
    bool exists = AssetManager::Instance().HasAsset(path);
    JS_FreeCString(ctx, path);
    
    return exists ? JS_TRUE : JS_FALSE;
}

// JS API: listAssets() - 列出所有资源
static JSValue js_listAssets(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto paths = AssetManager::Instance().GetAssetPaths();
    
    JSValue array = JS_NewArray(ctx);
    for (size_t i = 0; i < paths.size(); i++) {
        JS_SetPropertyUint32(ctx, array, i, JS_NewString(ctx, paths[i].c_str()));
    }
    
    return array;
}

// 注册资源 API 到全局对象
void RegisterAssetAPI(QuickJSRuntime* runtime) {
    JSContext* ctx = runtime->GetContext();
    JSValue global = JS_GetGlobalObject(ctx);
    
    // 注册全局函数
    JS_SetPropertyStr(ctx, global, "loadAsset",
        JS_NewCFunction(ctx, js_loadAsset, "loadAsset", 1));
    JS_SetPropertyStr(ctx, global, "getAssetUrl",
        JS_NewCFunction(ctx, js_getAssetUrl, "getAssetUrl", 1));
    JS_SetPropertyStr(ctx, global, "hasAsset",
        JS_NewCFunction(ctx, js_hasAsset, "hasAsset", 1));
    JS_SetPropertyStr(ctx, global, "listAssets",
        JS_NewCFunction(ctx, js_listAssets, "listAssets", 0));
    
    JS_FreeValue(ctx, global);
}

// 检测并加载嵌入的 payload
bool TryLoadEmbeddedPayload(const std::string& exe_path, mbink::PayloadData& payload_data) {
    return mbink::PayloadBuilder::ParseFromFile(exe_path, payload_data);
}

// 执行嵌入的字节码模块
bool ExecuteEmbeddedBytecode(QuickJSRuntime* runtime, const mbink::PayloadData& payload_data, bool verbose = true) {
    // 解析合并的字节码
    auto modules = mbink::BytecodeCompiler::ParseMergedBytecode(payload_data.bytecode);
    
    if (modules.empty()) {
        std::cerr << "  ✗ No modules found in payload" << std::endl;
        return false;
    }
    
    if (verbose) std::cout << "  Found " << modules.size() << " modules in payload" << std::endl;
    
    // 获取 QuickJS 上下文
    JSContext* ctx = runtime->GetContext();
    if (!ctx) {
        std::cerr << "  ✗ Failed to get QuickJS context" << std::endl;
        return false;
    }
    
    // 按顺序加载并执行每个模块
    for (const auto& module : modules) {
        if (verbose) {
            std::cout << "  Loading module: " << module.id;
            if (module.is_entry) std::cout << " (entry)";
            std::cout << std::endl;
        }
        
        // 从字节码读取对象
        JSValue obj = JS_ReadObject(ctx, module.bytecode.data(), module.bytecode.size(),
                                    JS_READ_OBJ_BYTECODE);
        
        if (JS_IsException(obj)) {
            JSValue exception = JS_GetException(ctx);
            const char* msg = JS_ToCString(ctx, exception);
            std::cerr << "  ✗ Failed to load bytecode: " << (msg ? msg : "unknown error") << std::endl;
            if (msg) JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, exception);
            return false;
        }
        
        // 执行字节码
        JSValue result = JS_EvalFunction(ctx, obj);
        
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx);
            const char* msg = JS_ToCString(ctx, exception);
            std::cerr << "  ✗ Execution error: " << (msg ? msg : "unknown error") << std::endl;
            if (msg) JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, exception);
            return false;
        }
        
        JS_FreeValue(ctx, result);
    }
    
    return true;
}

// 运行应用的主逻辑
int RunApp(int width, int height, const std::string& title, 
           bool open_devtools, const std::string& app_path,
           mbink::PayloadData* embedded_payload, bool verbose = true) {
    
    // 用于条件输出的宏
    #define LOG(x) if (verbose) { std::cout << x << std::endl; }
    #define LOG_NOENDL(x) if (verbose) { std::cout << x; }
    
    try {
        LOG("========================================");
        LOG("  MBink App Loader");
        LOG("========================================");
        if (embedded_payload) {
            LOG("  Mode: Embedded Bytecode");
        } else {
            LOG("  App: " << app_path);
        }
        LOG("  Size: " << width << "x" << height);
        LOG("  Title: " << title);
        LOG("========================================");
        LOG("");

        // 1. 创建窗口
        LOG("[1/6] Creating window...");
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        LOG("  ✓ Window created");

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);

        // 2. 创建文档
        LOG("[2/6] Creating document...");
        auto document = std::make_shared<Document>();
        document->Initialize();
        LOG("  ✓ Document initialized");

        // 创建body元素
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        
        // **重要：先将文档关联到窗口**
        window->SetDocument(document);

        // 3. 创建QuickJS运行时和任务调度器
        LOG("[3/6] Creating QuickJS runtime...");
        auto runtime = std::make_unique<QuickJSRuntime>();
        auto task_scheduler = std::make_shared<TaskScheduler>();
        LOG("  ✓ QuickJS runtime created");

        // 4. 初始化 WindowBindings
        LOG("[4/6] Initializing Window bindings...");
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        LOG("  ✓ Window bindings initialized");

        // 5. 加载 JS 库或执行嵌入的字节码
        if (embedded_payload) {
            // 嵌入模式：先加载基础库，再执行字节码
            LOG("[5/6] Loading libraries and embedded bytecode...");
            
            // 初始化资源管理器
            if (!embedded_payload->assets_index.empty()) {
                AssetManager::Instance().Initialize(
                    embedded_payload->assets_data,
                    embedded_payload->assets_index
                );
                
                // 注册 ImageLoader 的资源提供者，实现透明加载
                ImageLoader::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return AssetManager::Instance().GetAsset(path, data);
                });
                
                // 注册 CSS 的资源提供者
                LexborStyleSheet::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return AssetManager::Instance().GetAsset(path, data);
                });
                
                // 注册 Document 的资源提供者（用于 link 元素加载 CSS）
                Document::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return AssetManager::Instance().GetAsset(path, data);
                });
                
                if (verbose) {
                    std::cout << "  ✓ Loaded " << embedded_payload->assets_index.size() 
                              << " embedded assets" << std::endl;
                }
            }
            
            // 注册资源 API
            RegisterAssetAPI(runtime.get());
            
            // 先加载嵌入的 Preact 等库（用户代码可能依赖全局 Preact 对象）
            if (lightui::embedded::HasEmbeddedJS()) {
                LoadEmbeddedLibraries(runtime.get(), verbose);
            }
            
            // 然后执行嵌入的字节码
            LOG("  Executing embedded bytecode...");
            if (!ExecuteEmbeddedBytecode(runtime.get(), *embedded_payload, verbose)) {
                return 1;
            }
            LOG("  ✓ Bytecode executed");
        } else {
            // 命令行模式：加载 JS 库和应用
            LOG("[5/7] Loading JavaScript libraries...");
            
            if (lightui::embedded::HasEmbeddedJS()) {
                LoadEmbeddedLibraries(runtime.get(), verbose);
            } else {
                LOG("  ⚠ No embedded JS, loading from filesystem...");
                
                std::string preact_path = FindPreactPath(app_path);
                if (preact_path.empty()) {
                    std::cerr << "  ✗ Could not find Preact library" << std::endl;
                    return 1;
                }
                
                // 加载 DOM polyfills
                std::string polyfills_path = preact_path + "/../polyfills/dom.js";
                if (fs::exists(polyfills_path)) {
                    std::string polyfills_code = ReadFile(polyfills_path);
                    if (!polyfills_code.empty()) {
                        runtime->Eval(polyfills_code, "dom.js");
                        LOG("  ✓ DOM polyfills loaded");
                    }
                }
                
                // 加载 Preact
                std::string preact_code = ReadFile(preact_path + "/preact.js");
                if (preact_code.empty()) {
                    std::cerr << "  ✗ Failed to load preact.js" << std::endl;
                    return 1;
                }
                runtime->Eval(preact_code, "preact.js");
                LOG("  ✓ Preact loaded from: " << preact_path);
                
                // 加载 Hooks
                std::string hooks_code = ReadFile(preact_path + "/hooks.js");
                if (!hooks_code.empty()) {
                    runtime->Eval(hooks_code, "hooks.js");
                    LOG("  ✓ Hooks library loaded");
                }
            }

            // 加载 Chart.js（可选）
            LOG("[6/7] Loading optional libraries...");
            std::vector<std::string> chartjs_search_paths = {
                "js/chart.dev.js", "js/chart.js",
                "../js/chart.dev.js", "../js/chart.js",
                "../../js/chart.dev.js", "../../js/chart.js"
            };
            bool chartjs_loaded = false;
            for (const auto& path : chartjs_search_paths) {
                if (fs::exists(path)) {
                    std::string chartjs_code = ReadFile(path);
                    if (!chartjs_code.empty()) {
                        runtime->Eval(chartjs_code, "chart.js");
                        LOG("  ✓ Chart.js loaded from: " << path);
                        chartjs_loaded = true;
                        break;
                    }
                }
            }
            if (!chartjs_loaded) {
                LOG("  - Chart.js not found (optional)");
            }

            // 加载用户应用
            LOG("[7/7] Loading application...");
            std::string app_code = ReadFile(app_path);
            if (app_code.empty()) {
                std::cerr << "  ✗ Failed to load: " << app_path << std::endl;
                return 1;
            }
            
            fs::path app_filename = fs::path(app_path).filename();
            runtime->Eval(app_code, app_filename.string());
            LOG("  ✓ Application loaded");
        }

        // 显示窗口
        LOG("[6/6] Starting application...");
        window->Show();

        // 初始化 DevTools
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Initialize(document.get(), window.get());
        
        if (open_devtools) {
            devtools.Open();
            LOG("  ✓ DevTools opened");
        }

        LOG("");
        LOG("========================================");
        LOG("  🚀 Application Started!");
        LOG("========================================");
        LOG("  Close window to exit");
        LOG("  Press F12 to toggle DevTools");
        LOG("");

        // 创建事件循环
        EventLoop event_loop(task_scheduler);

        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });

        event_loop.Run();

        LOG("");
        LOG("========================================");
        LOG("  👋 Application Closed");
        LOG("========================================");

        // 清理
        LOG("Shutting down DevTools...");
        devtools.Shutdown();

        LOG("Cleaning up resources...");
        runtime.reset();
        document.reset();
        FontManager::GetInstance().ClearCache();
        window_manager.UnregisterWindow(window);
        window.reset();

        LOG("All resources cleaned up, exiting...");
        if (verbose) std::cout.flush();
        
        std::quick_exit(0);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    #undef LOG
    #undef LOG_NOENDL
}

int main(int argc, char** argv) {
    // 首先检查是否有嵌入的 payload
    mbink::PayloadData embedded_payload;
    bool has_embedded = TryLoadEmbeddedPayload(argv[0], embedded_payload);
    
    // Windows: 在嵌入模式下，如果没有 --verbose 参数，释放控制台
#ifdef _WIN32
    if (has_embedded && embedded_payload.valid) {
        bool want_console = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--verbose" || arg == "-v") {
                want_console = true;
                break;
            }
        }
        if (!want_console) {
            FreeConsole();
        }
    }
#endif
    
    if (has_embedded && embedded_payload.valid) {
        // 嵌入模式：使用 payload 中的配置
        int width = embedded_payload.config.width;
        int height = embedded_payload.config.height;
        std::string title = embedded_payload.config.title;
        bool open_devtools = false;
        bool verbose = false;
        
        // 解析命令行参数
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--width" && i + 1 < argc) {
                width = std::stoi(argv[++i]);
            } else if (arg == "--height" && i + 1 < argc) {
                height = std::stoi(argv[++i]);
            } else if (arg == "--title" && i + 1 < argc) {
                title = argv[++i];
            } else if (arg == "--devtools") {
                open_devtools = true;
            } else if (arg == "--verbose" || arg == "-v") {
                verbose = true;
            }
        }
        
        // 嵌入模式默认静默运行（除非指定 --verbose）
        return RunApp(width, height, title, open_devtools, "", &embedded_payload, verbose);
    }
    
    // 命令行模式
    std::string app_path;
    int width = 800;
    int height = 600;
    std::string title = "MBink App";
    bool open_devtools = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if (arg == "--title" && i + 1 < argc) {
            title = argv[++i];
        } else if (arg == "--devtools") {
            open_devtools = true;
        } else if (arg[0] != '-') {
            app_path = arg;
        }
    }
    
    if (app_path.empty()) {
        std::cerr << "错误: 未指定 app.js 文件路径" << std::endl;
        std::cerr << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    
    if (!fs::exists(app_path)) {
        std::cerr << "错误: 文件不存在: " << app_path << std::endl;
        return 1;
    }
    
    // 命令行模式默认显示日志
    return RunApp(width, height, title, open_devtools, app_path, nullptr, true);
}
