/**
 * @file main.cpp
 * @brief MBink HTML Loader (Clean Version) - 纯净版 HTML 加载器
 * 
 * 不在 C++ 中查找和加载任何 JS 库，完全依赖 HTML 中的 <script src="..."> 加载
 * 
 * 用法: html_loader_clean.exe <html文件路径> [选项]
 * 
 * 选项:
 *   --width <宽度>      窗口宽度 (默认: 800)
 *   --height <高度>     窗口高度 (默认: 600)
 *   --title <标题>      窗口标题 (默认: 从HTML title标签获取)
 *   --no-scripts        不执行脚本
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/event/loop/task_scheduler.h"
#include "core/event/loop/event_loop.h"
#include "core/network/fetch_bindings.h"
#include "core/devtools/devtools_manager.h"
#include "core/render/image/image_loader.h"

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
    std::cout << "MBink HTML Loader (Clean Version)" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << program_name << " <html文件路径> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --width <宽度>      窗口宽度 (默认: 800)" << std::endl;
    std::cout << "  --height <高度>     窗口高度 (默认: 600)" << std::endl;
    std::cout << "  --title <标题>      窗口标题 (默认: 从HTML title标签获取)" << std::endl;
    std::cout << "  --no-scripts        不执行脚本" << std::endl;
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " index.html" << std::endl;
    std::cout << "  " << program_name << " app.html --width 1024 --height 768" << std::endl;
}

// 从文档中获取 title
std::string GetDocumentTitle(std::shared_ptr<Document> doc) {
    auto titles = doc->GetElementsByTagName("title");
    if (!titles.empty()) {
        return titles[0]->GetTextContent();
    }
    return "MBink App";
}

int main(int argc, char** argv) {
    // 解析命令行参数
    std::string html_path;
    int width = 1600;
    int height = 1000;
    std::string title;
    bool execute_scripts = true;
    
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
        } else if (arg == "--no-scripts") {
            execute_scripts = false;
        } else if (arg[0] != '-') {
            html_path = arg;
        }
    }
    
    if (html_path.empty()) {
        std::cerr << "错误: 未指定 HTML 文件路径" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    
    // 检查文件是否存在
    if (!fs::exists(html_path)) {
        std::cerr << "错误: 文件不存在: " << html_path << std::endl;
        return 1;
    }
    
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink HTML Loader (Clean)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  HTML: " << html_path << std::endl;
        std::cout << "  Size: " << width << "x" << height << std::endl;
        std::cout << "  Scripts: " << (execute_scripts ? "enabled" : "disabled") << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建 QuickJS 运行时
        std::cout << "[1/4] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        auto task_scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 2. 创建文档并加载 HTML
        std::cout << "[2/4] Loading HTML document..." << std::endl;
        auto document = std::make_shared<Document>();
        
        // 关联 JS 运行时
        if (execute_scripts) {
            document->SetJSRuntime(runtime.get());
        }
        
        // 在加载 HTML 之前设置基础路径（用于解析相对路径的外部资源）
        fs::path html_dir = fs::path(html_path).parent_path();
        if (html_dir.empty()) {
            html_dir = fs::current_path();
        }
        std::string base_path = fs::absolute(html_dir).string();
        document->SetBasePath(base_path);
        ImageLoader::SetBasePath(base_path);
        std::cout << "  ✓ Base path: " << base_path << std::endl;
        
        // 读取并解析 HTML
        std::string html_content = ReadFile(html_path);
        if (html_content.empty()) {
            std::cerr << "  ✗ Failed to read HTML file" << std::endl;
            return 1;
        }
        
        if (!document->LoadHTML(html_content)) {
            std::cerr << "  ✗ Failed to parse HTML" << std::endl;
            return 1;
        }
        std::cout << "  ✓ HTML document loaded" << std::endl;

        // 加载外部样式表
        document->LoadExternalStylesheets();

        // 统计标签
        auto styles = document->GetElementsByTagName("style");
        auto links = document->GetElementsByTagName("link");
        auto scripts = document->GetElementsByTagName("script");
        std::cout << "  ✓ Found " << styles.size() << " <style>, "
                  << links.size() << " <link>, "
                  << scripts.size() << " <script>" << std::endl;

        // 获取或设置窗口标题
        if (title.empty()) {
            title = GetDocumentTitle(document);
        }
        std::cout << "  ✓ Title: " << title << std::endl;

        // 3. 创建窗口并初始化绑定
        std::cout << "[3/4] Creating window and bindings..." << std::endl;
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        
        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);

        // 关联文档到窗口
        window->SetDocument(document);

        // 初始化 WindowBindings
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        std::cout << "  ✓ Window bindings initialized" << std::endl;

        // 初始化 FetchBindings
        FetchBindings fetch_bindings(runtime->GetContext(), task_scheduler);
        fetch_bindings.InitBindings();
        std::cout << "  ✓ Fetch API initialized" << std::endl;

        // 初始化 DevTools
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Initialize(document.get(), window.get());
        std::cout << "  ✓ DevTools initialized (F12 to toggle)" << std::endl;

        // 4. 执行脚本（完全由 HTML 中的 <script> 标签控制）
        if (execute_scripts) {
            std::cout << "[4/4] Executing scripts..." << std::endl;
            document->ExecuteScripts();
            std::cout << "  ✓ Scripts executed" << std::endl;
        } else {
            std::cout << "[4/4] Skipping script execution (--no-scripts)" << std::endl;
        }

        // 显示窗口
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "  Press F12 or Ctrl+Shift+I for DevTools" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 创建事件循环
        EventLoop event_loop(task_scheduler);
        event_loop.SetQuickJSRuntime(runtime.get());

        // 设置全局 EventLoop 以便 execCommand 等 API 可以访问
        DOMBindings::SetGlobalEventLoop(runtime->GetContext(), &event_loop);

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });

        // 运行事件循环
        event_loop.Run();

        // 关闭 DevTools
        devtools.Shutdown();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  👋 Application Closed" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

