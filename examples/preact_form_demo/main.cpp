/**
 * @file main.cpp
 * @brief Preact Form Demo - 表单元素和验证示例
 *
 * 支持两种运行模式:
 * 1. MBink 运行: 加载 HTML 文件，使用内置 Preact
 * 2. 浏览器运行: 直接打开 index.html，使用 CDN Preact
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/event/task_scheduler.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

using namespace lightui;

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink + Preact Form Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink Form Demo - Input Validation Example";
        config.width = 700;
        config.height = 800;
        auto window = std::make_shared<Window>(config);
        std::cout << "  Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  Window registered" << std::endl;

        // 2. 创建文档
        std::cout << "[2/7] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  Document initialized" << std::endl;

        // 3. 创建QuickJS运行时
        std::cout << "[3/7] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();
        std::cout << "  QuickJS runtime created" << std::endl;

        // 4. 创建 TaskScheduler
        std::cout << "[4/8] Creating task scheduler..." << std::endl;
        auto scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  Task scheduler created" << std::endl;

        // 5. 初始化DOM绑定
        std::cout << "[5/8] Initializing DOM bindings..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        DOMBindings::SetGlobalTaskScheduler(ctx, scheduler);
        std::cout << "  DOM bindings initialized" << std::endl;

        // 6. 创建body元素
        std::cout << "[6/8] Creating body element..." << std::endl;
        auto body = document->CreateElement("body");
        document->SetBody(body);
        std::cout << "  Body element created" << std::endl;

        // 7. 加载Preact库 (相对于 build/bin/Release/)
        std::cout << "[7/8] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("../../examples/demo_html/js/preact/preact.js");
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  Preact library loaded" << std::endl;

        // 加载Hooks库
        std::string hooks_code = ReadFile("../../examples/demo_html/js/preact/hooks.js");
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  Hooks library loaded" << std::endl;

        // 8. 加载并运行应用
        std::cout << "[8/8] Loading application..." << std::endl;
        std::string app_code = ReadFile("../../examples/demo_html/preact_form/app.js");
        if (app_code.empty()) {
            std::cerr << "Failed to load app.js" << std::endl;
            return 1;
        }
        runtime->Eval(app_code, "app.js");
        std::cout << "  Application loaded and rendered" << std::endl;

        std::cout << std::endl;
        std::cout << "  Note: Open examples/demo_html/preact_form/index.html in browser to compare" << std::endl;

        // 将文档关联到窗口并显示
        window->SetDocument(document);
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Form features:" << std::endl;
        std::cout << "  - Text, email, password, number inputs" << std::endl;
        std::cout << "  - Radio buttons and checkboxes" << std::endl;
        std::cout << "  - Real-time form preview" << std::endl;
        std::cout << "  - Input validation on submit" << std::endl;
        std::cout << "  - Reset functionality" << std::endl;
        std::cout << std::endl;
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
        std::cout << "  Application Closed" << std::endl;
        std::cout << "========================================" << std::endl;

        // 清理
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

