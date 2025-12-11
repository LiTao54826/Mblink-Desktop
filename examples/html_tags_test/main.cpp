/**
 * @file main.cpp
 * @brief HTML Tags Comprehensive Test - 测试所有已实现的 HTML 标签
 * 
 * 测试覆盖:
 * - Phase 1: 语义化标签 (30+个)
 * - Phase 2: 列表标签 (ol, li)
 * - Phase 3: 表格标签 (10个)
 * - Phase 4: SVG 支持 (10个)
 * - Phase 5: 表单增强标签 (8个)
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
#include <vector>

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
        std::cout << "  MBink HTML Tags Comprehensive Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink HTML Tags Test - Phase 1~5";
        config.width = 1200;
        config.height = 900;
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
        std::cout << "  ✓ DOM bindings initialized" << std::endl;

        // 5. 创建body元素
        std::cout << "[5/7] Creating body element..." << std::endl;
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        std::cout << "  ✓ Body element created with overflow: auto" << std::endl;

        // 6. 加载Preact库
        std::cout << "[6/7] Loading Preact library..." << std::endl;
        // 尝试多个可能的路径
        std::vector<std::string> preact_paths = {
            "../../examples/demo_html/js/preact/preact.js",
            "../examples/demo_html/js/preact/preact.js",
            "examples/demo_html/js/preact/preact.js",
            "../../../examples/demo_html/js/preact/preact.js"
        };
        std::string preact_code;
        for (const auto& path : preact_paths) {
            preact_code = ReadFile(path);
            if (!preact_code.empty()) {
                std::cout << "  Found preact.js at: " << path << std::endl;
                break;
            }
        }
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js from any path" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;

        // 加载Hooks库
        std::vector<std::string> hooks_paths = {
            "../../examples/demo_html/js/preact/hooks.js",
            "../examples/demo_html/js/preact/hooks.js",
            "examples/demo_html/js/preact/hooks.js",
            "../../../examples/demo_html/js/preact/hooks.js"
        };
        std::string hooks_code;
        for (const auto& path : hooks_paths) {
            hooks_code = ReadFile(path);
            if (!hooks_code.empty()) {
                std::cout << "  Found hooks.js at: " << path << std::endl;
                break;
            }
        }
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js from any path" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Hooks library loaded" << std::endl;

        // 7. 加载并运行应用
        std::cout << "[7/7] Loading HTML Tags Test application..." << std::endl;
        std::vector<std::string> app_paths = {
            "../../examples/demo_html/html_tags_test/app.js",
            "../examples/demo_html/html_tags_test/app.js",
            "examples/demo_html/html_tags_test/app.js",
            "../../../examples/demo_html/html_tags_test/app.js"
        };
        std::string app_code;
        for (const auto& path : app_paths) {
            app_code = ReadFile(path);
            if (!app_code.empty()) {
                std::cout << "  Found app.js at: " << path << std::endl;
                break;
            }
        }
            if (app_code.empty()) {
                std::cerr << "Failed to load app.js from any path" << std::endl;
                return 1;
            }
            runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ Application loaded and rendered" << std::endl;
        
        // 将文档关联到窗口并显示
        window->SetDocument(document);
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 HTML Tags Test Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Testing:" << std::endl;
        std::cout << "  ✓ Phase 1: Semantic tags (30+)" << std::endl;
        std::cout << "  ✓ Phase 2: List tags (ol, li)" << std::endl;
        std::cout << "  ✓ Phase 3: Table tags (10)" << std::endl;
        std::cout << "  ✓ Phase 4: SVG support (10)" << std::endl;
        std::cout << "  ✓ Phase 5: Form enhanced tags (8)" << std::endl;
        std::cout << std::endl;
        std::cout << "  Scroll down to see all tests!" << std::endl;
        std::cout << "  Close window to exit." << std::endl;
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

        // 清理
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

