/**
 * @file main.cpp
 * @brief Preact Todo App - 更复杂的示例
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/lexbor/lexbor_document.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/preact_bindings.h"
#include "core/quickjs/preact_renderer.h"
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
        std::cout << "[1/8] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink Todo App";
        config.width = 900;
        config.height = 700;
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window registered" << std::endl;

        // 2. 创建文档
        std::cout << "[2/8] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;

        // 3. 创建QuickJS运行时
        std::cout << "[3/8] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 4. 创建Preact渲染器
        std::cout << "[4/8] Creating Preact renderer..." << std::endl;
        auto renderer = std::make_shared<PreactRenderer>(runtime.get(), document);
        std::cout << "  ✓ Preact renderer created" << std::endl;

        // 5. 初始化绑定
        std::cout << "[5/8] Initializing bindings..." << std::endl;
        DOMBindings::Init(ctx);
        PreactBindings::Init(ctx, renderer);
        std::cout << "  ✓ DOM and Preact bindings initialized" << std::endl;

        // 暴露document到JavaScript
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue doc_obj = DOMBindings::WrapDocument(ctx, document);
        JS_SetPropertyStr(ctx, global, "document", doc_obj);
        JS_FreeValue(ctx, global);
        std::cout << "  ✓ Document exposed to JavaScript" << std::endl;

        // 6. 加载Preact库
        std::cout << "[6/8] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("js/preact/preact.js");
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;

        // 加载Hooks库
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Hooks library loaded" << std::endl;

        // 7. 加载并运行应用
        std::cout << "[7/8] Loading application..." << std::endl;
        std::string app_code = ReadFile("examples/preact_todo_app/app.js");
        if (app_code.empty()) {
            std::cerr << "Failed to load app.js" << std::endl;
            return 1;
        }
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ Application loaded and rendered" << std::endl;

        // 8. 将文档关联到窗口并显示
        std::cout << "[8/8] Showing window..." << std::endl;
        window->SetDocument(document);
        window->Show();
        std::cout << "  ✓ Window shown" << std::endl;

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Close window to exit" << std::endl;
        std::cout << std::endl;

        // 创建事件循环
        EventLoop event_loop;

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->RenderDocument();
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
        PreactBindings::Cleanup(ctx);
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

