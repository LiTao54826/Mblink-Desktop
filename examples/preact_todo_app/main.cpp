/**
 * @file main.cpp
 * @brief Preact Todo App - Interactive todo list example
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"

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
        std::cout << "  MBink + Preact Todo App" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink Todo App - Interactive Example";
        config.width = 900;
        config.height = 700;
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window registered" << std::endl;

        // 2. 创建文档
        std::cout << "[2/7] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;

        // 3. 创建QuickJS运行时
        std::cout << "[3/7] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 4. 初始化DOM绑定
        std::cout << "[4/7] Initializing DOM bindings..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);

        // 重要：在 JS 执行前就将 Document 关联到 Window，以便注册 DOM Observer
        // 这样 JS 中的 DOM 变化才能触发重绘
        window->SetDocument(document);
        std::cout << "  ✓ DOM bindings initialized" << std::endl;

        // 5. 创建body元素
        std::cout << "[5/7] Creating body element..." << std::endl;
        auto body = document->CreateElement("body");
        // 设置 body 的 overflow: auto 以支持页面级滚动
        // 不设置固定宽高，让 body 自动适应视口大小
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        std::cout << "  ✓ Body element created with overflow: auto" << std::endl;

        // 6. 加载Preact库
        std::cout << "[6/7] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("../../examples/demo_html/js/preact/preact.js");
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;

        // 加载Hooks库
        std::string hooks_code = ReadFile("../../examples/demo_html/js/preact/hooks.js");
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Hooks library loaded" << std::endl;

        // 7. 加载并运行应用
        std::cout << "[7/7] Loading application..." << std::endl;
        std::string app_code = ReadFile("../../examples/demo_html/preact_todo/app.js");
        if (app_code.empty()) {
            std::cerr << "Failed to load app.js" << std::endl;
            return 1;
        }
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ Application loaded and rendered" << std::endl;

        // 显示窗口（Document 已在前面关联）
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Try these interactions:" << std::endl;
        std::cout << "  - Type in the input box and click 'Add'" << std::endl;
        std::cout << "  - Click 'Done' to mark todos as complete" << std::endl;
        std::cout << "  - Click 'Undo' to mark todos as pending" << std::endl;
        std::cout << "  - Click 'Delete' to remove todos" << std::endl;
        std::cout << "  - Close window to exit" << std::endl;
        std::cout << std::endl;

        // 创建事件循环
        EventLoop event_loop;

        // 重要：将 TaskScheduler 绑定到 JS，以支持 setTimeout/setInterval/requestAnimationFrame
        auto task_scheduler = event_loop.GetTaskSchedulerPtr();
        DOMBindings::SetGlobalTaskScheduler(ctx, task_scheduler);

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

        // 清理
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

