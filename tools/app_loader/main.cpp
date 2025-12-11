/**
 * @file main.cpp
 * @brief MBink App Loader - 通用的 app.js 加载器
 * 
 * 用法: app_loader.exe <app.js路径> [选项]
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

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <filesystem>

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
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " my_app.js" << std::endl;
    std::cout << "  " << program_name << " my_app.js --width 1024 --height 768" << std::endl;
    std::cout << "  " << program_name << " my_app.js --title \"My Application\"" << std::endl;
}

// 查找 Preact 库路径
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

int main(int argc, char** argv) {
    // 解析命令行参数
    std::string app_path;
    int width = 800;
    int height = 600;
    std::string title = "MBink App";
    
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
    
    // 检查文件是否存在
    if (!fs::exists(app_path)) {
        std::cerr << "错误: 文件不存在: " << app_path << std::endl;
        return 1;
    }
    
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink App Loader" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  App: " << app_path << std::endl;
        std::cout << "  Size: " << width << "x" << height << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/6] Creating window..." << std::endl;
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created" << std::endl;

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);

        // 2. 创建文档
        std::cout << "[2/6] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;

        // 创建body元素
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        
        // **重要：先将文档关联到窗口**
        window->SetDocument(document);

        // 3. 创建QuickJS运行时和任务调度器
        std::cout << "[3/6] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        auto task_scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 4. 初始化 WindowBindings（新的 DOM 绑定系统）
        // WindowBindings 会自动从 window 获取 document 并绑定到全局
        std::cout << "[4/6] Initializing Window bindings..." << std::endl;
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        std::cout << "  ✓ Window bindings initialized" << std::endl;

        // 5. 加载Preact库
        std::cout << "[5/6] Loading Preact library..." << std::endl;
        std::string preact_path = FindPreactPath(app_path);
        
        if (preact_path.empty()) {
            std::cerr << "  ✗ Could not find Preact library" << std::endl;
            std::cerr << "  Please ensure js/preact/preact.js exists" << std::endl;
            return 1;
        }
        
        std::string preact_code = ReadFile(preact_path + "/preact.js");
        if (preact_code.empty()) {
            std::cerr << "  ✗ Failed to load preact.js" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ Preact loaded from: " << preact_path << std::endl;

        // 加载Hooks库
        std::string hooks_code = ReadFile(preact_path + "/hooks.js");
        if (!hooks_code.empty()) {
            runtime->Eval(hooks_code, "hooks.js");
            std::cout << "  ✓ Hooks library loaded" << std::endl;
        }

        // 6. 加载并运行应用
        std::cout << "[6/6] Loading application..." << std::endl;
        std::string app_code = ReadFile(app_path);
        if (app_code.empty()) {
            std::cerr << "  ✗ Failed to load: " << app_path << std::endl;
            return 1;
        }
        
        fs::path app_filename = fs::path(app_path).filename();
        runtime->Eval(app_code, app_filename.string());
        std::cout << "  ✓ Application loaded" << std::endl;

        // 显示窗口
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Close window to exit" << std::endl;
        std::cout << std::endl;

        // 创建事件循环
        EventLoop event_loop;

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });

        // 运行事件循环
        event_loop.Run();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  👋 Application Closed" << std::endl;
        std::cout << "========================================" << std::endl;

        // 清理（WindowBindings 会自动清理）

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

