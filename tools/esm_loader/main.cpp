/**
 * @file main.cpp
 * @brief MBink ESM Loader - 支持 ES 模块的应用加载器
 *
 * 用法: esm_loader.exe <entry.js> [选项]
 *
 * 选项:
 *   --width <宽度>      窗口宽度 (默认: 800)
 *   --height <高度>     窗口高度 (默认: 600)
 *   --title <标题>      窗口标题 (默认: MBink App)
 *   --devtools          启动时打开开发者工具
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
#include "embedded_js.h"

extern "C" {
#include "quickjs/quickjs.h"
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <filesystem>

using namespace lightui;
namespace fs = std::filesystem;

std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void PrintUsage(const char* program_name) {
    std::cout << "MBink ESM Loader - ES 模块加载器" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << program_name << " <entry.js> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --width <宽度>      窗口宽度 (默认: 800)" << std::endl;
    std::cout << "  --height <高度>     窗口高度 (默认: 600)" << std::endl;
    std::cout << "  --title <标题>      窗口标题 (默认: MBink App)" << std::endl;
    std::cout << "  --devtools          启动时打开开发者工具" << std::endl;
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " app.js" << std::endl;
    std::cout << "  " << program_name << " app.js --width 1024 --height 768" << std::endl;
}

// 加载嵌入的 JS 库
bool LoadEmbeddedLibraries(QuickJSRuntime* runtime) {
    using namespace lightui::embedded;

    if (!HasEmbeddedJS()) {
        return false;
    }

    try {
        // 加载 DOM polyfills
        auto polyfills = GetDomPolyfillsJS();
        if (!polyfills.empty()) {
            std::string polyfills_str(polyfills);
            runtime->Eval(polyfills_str, "dom.js");
            std::cout << "  ✓ DOM polyfills loaded" << std::endl;
        }

        // 加载 Preact（作为全局对象）
        auto preact = GetPreactJS();
        if (!preact.empty()) {
            std::string preact_str(preact);
            runtime->Eval(preact_str, "preact.js");
            std::cout << "  ✓ Preact loaded (global)" << std::endl;
        }

        // 加载 Hooks
        auto hooks = GetHooksJS();
        if (!hooks.empty()) {
            std::string hooks_str(hooks);
            runtime->Eval(hooks_str, "hooks.js");
            std::cout << "  ✓ Hooks loaded (global)" << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Error loading embedded JS: " << e.what() << std::endl;
        return false;
    }
}

// 注册 Preact 为 ES 模块
void RegisterPreactModules(QuickJSRuntime* runtime) {
    // 注册 preact 模块（从全局对象导出）
    runtime->RegisterModule("preact", R"(
        export const h = globalThis.Preact.h;
        export const render = globalThis.Preact.render;
        export const Component = globalThis.Preact.Component;
        export const Fragment = globalThis.Preact.Fragment;
        export const createRef = globalThis.Preact.createRef;
        export const createElement = globalThis.Preact.createElement;
        export default globalThis.Preact;
    )");

    // 注册 preact/hooks 模块
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

    std::cout << "  ✓ Preact ES modules registered" << std::endl;
}

int main(int argc, char** argv) {
    std::string entry_path;
    int width = 1200;
    int height = 800;
    std::string title = "MBink App";
    bool open_devtools = false;

    // 解析命令行参数
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
            entry_path = arg;
        }
    }

    if (entry_path.empty()) {
        std::cerr << "错误: 未指定入口文件" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }

    if (!fs::exists(entry_path)) {
        std::cerr << "错误: 文件不存在: " << entry_path << std::endl;
        return 1;
    }

    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink ESM Loader" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Entry: " << entry_path << std::endl;
        std::cout << "  Size: " << width << "x" << height << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/5] Creating window..." << std::endl;
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;

        auto window = std::make_shared<Window>(config);
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window created" << std::endl;

        // 2. 创建文档
        std::cout << "[2/5] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        window->SetDocument(document);
        std::cout << "  ✓ Document initialized" << std::endl;

        // 3. 创建 QuickJS 运行时
        std::cout << "[3/5] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        auto task_scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 4. 初始化绑定和库
        std::cout << "[4/5] Initializing bindings..." << std::endl;
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        std::cout << "  ✓ Window bindings initialized" << std::endl;

        // 加载嵌入的库
        if (lightui::embedded::HasEmbeddedJS()) {
            LoadEmbeddedLibraries(runtime.get());
            RegisterPreactModules(runtime.get());
        } else {
            std::cerr << "  ⚠ No embedded JS libraries" << std::endl;
        }

        // 5. 加载入口模块
        std::cout << "[5/5] Loading entry module..." << std::endl;

        // 设置模块基础路径
        fs::path abs_path = fs::absolute(entry_path);
        runtime->SetBaseModulePath(abs_path.string());

        // 读取入口文件
        std::string entry_code = ReadFile(entry_path);
        if (entry_code.empty()) {
            std::cerr << "  ✗ Failed to read: " << entry_path << std::endl;
            return 1;
        }

        // 使用 EvalModule 执行 ES 模块
        try {
            runtime->EvalModule(entry_code, abs_path.string());
            std::cout << "  ✓ Entry module loaded" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  ✗ Module error: " << e.what() << std::endl;
            return 1;
        }

        // 显示窗口
        window->Show();

        // 初始化 DevTools
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Initialize(document.get(), window.get());
        if (open_devtools) {
            devtools.Open();
        }

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Press F12 to toggle DevTools" << std::endl;
        std::cout << std::endl;

        // 事件循环
        EventLoop event_loop(task_scheduler);
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });
        event_loop.Run();

        // 清理
        std::cout << std::endl;
        std::cout << "Shutting down..." << std::endl;
        devtools.Shutdown();
        runtime.reset();
        document.reset();
        FontManager::GetInstance().ClearCache();
        window_manager.UnregisterWindow(window);
        window.reset();

        std::quick_exit(0);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
