/**
 * @file main.cpp
 * @brief Layout Compare Test - 对比浏览器和 MBink 的布局渲染结果
 *
 * 测试内容:
 * - Block 布局 (width, height, margin, padding, border)
 * - Flexbox 布局 (flex-direction, justify-content, align-items, gap)
 * - 尺寸约束 (min-width, max-width, min-height, max-height)
 * - 百分比布局
 * - 嵌套布局
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/render/render_object.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>

using namespace lightui;

// 打印布局信息的辅助函数
void PrintLayoutTree(const std::shared_ptr<RenderObject>& render_obj, int depth = 0) {
    if (!render_obj) return;

    auto node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    
    std::string indent(depth * 2, ' ');
    std::string tag = element ? element->GetTagName() : "#text";
    
    const auto& layout = render_obj->GetLayoutInfo();
    const auto& style = render_obj->GetComputedStyle();
    
    // 只打印有意义的布局节点
    if (layout.width > 0 || layout.height > 0) {
        std::cout << indent << "[" << tag << "] "
                  << "x=" << std::fixed << std::setprecision(1) << layout.x 
                  << " y=" << layout.y
                  << " w=" << layout.width 
                  << " h=" << layout.height;
        
        // 打印 display 类型 - 从 ComputedStyle 获取准确的 display 类型
        auto display_type = style.display;
        std::string display_str;
        switch (display_type) {
            case RenderObjectType::BLOCK: display_str = "block"; break;
            case RenderObjectType::INLINE: display_str = "inline"; break;
            case RenderObjectType::INLINE_BLOCK: display_str = "inline-block"; break;
            case RenderObjectType::FLEX: display_str = "flex"; break;
            case RenderObjectType::GRID: display_str = "grid"; break;
            case RenderObjectType::TEXT: display_str = "text"; break;
            case RenderObjectType::NONE: display_str = "none"; break;
            default: display_str = "other"; break;
        }
        std::cout << " (" << display_str << ")";

        // 如果是 flex 容器，打印 justify-content 和 align-items
        if (display_type == RenderObjectType::FLEX) {
            std::cout << " jc:" << style.justify_content << " ai:" << style.align_items;
        }
        
        // 打印 margin/padding 如果非零
        if (style.margin.top.value != 0 || style.margin.right.value != 0 ||
            style.margin.bottom.value != 0 || style.margin.left.value != 0) {
            std::cout << " m:[" << style.margin.top.value << " " 
                      << style.margin.right.value << " "
                      << style.margin.bottom.value << " " 
                      << style.margin.left.value << "]";
        }
        
        if (style.padding.top.value != 0 || style.padding.right.value != 0 ||
            style.padding.bottom.value != 0 || style.padding.left.value != 0) {
            std::cout << " p:[" << style.padding.top.value << " " 
                      << style.padding.right.value << " "
                      << style.padding.bottom.value << " " 
                      << style.padding.left.value << "]";
        }
        
        std::cout << std::endl;
    }

    // 递归遍历子节点 (限制深度避免输出过多)
    if (depth < 8) {
        for (const auto& child : render_obj->GetChildren()) {
            PrintLayoutTree(child, depth + 1);
        }
    }
}

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

// 尝试多个路径加载文件
std::string LoadFileFromPaths(const std::vector<std::string>& paths, const std::string& filename) {
    for (const auto& path : paths) {
        std::string code = ReadFile(path);
        if (!code.empty()) {
            std::cout << "  Found " << filename << " at: " << path << std::endl;
            return code;
        }
    }
    return "";
}

int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink Layout Compare Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink Layout Compare Test";
        config.width = 900;
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
        std::vector<std::string> preact_paths = {
            "js/preact/preact.js",
            "examples/demo_html/js/preact/preact.js",
            "../examples/demo_html/js/preact/preact.js",
            "../../examples/demo_html/js/preact/preact.js",
            "../../../examples/demo_html/js/preact/preact.js"
        };
        std::string preact_code = LoadFileFromPaths(preact_paths, "preact.js");
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js from any path" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;

        // 加载Hooks库
        std::vector<std::string> hooks_paths = {
            "js/preact/hooks.js",
            "examples/demo_html/js/preact/hooks.js",
            "../examples/demo_html/js/preact/hooks.js",
            "../../examples/demo_html/js/preact/hooks.js",
            "../../../examples/demo_html/js/preact/hooks.js"
        };
        std::string hooks_code = LoadFileFromPaths(hooks_paths, "hooks.js");
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js from any path" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Hooks library loaded" << std::endl;

        // 7. 加载并运行应用
        std::cout << "[7/7] Loading Layout Compare Test application..." << std::endl;
        std::vector<std::string> app_paths = {
            "layout_compare_test/app.js",
            "examples/demo_html/layout_compare_test/app.js",
            "../examples/demo_html/layout_compare_test/app.js",
            "../../examples/demo_html/layout_compare_test/app.js",
            "../../../examples/demo_html/layout_compare_test/app.js"
        };
        std::string app_code = LoadFileFromPaths(app_paths, "app.js");
        if (app_code.empty()) {
            std::cerr << "Failed to load app.js from any path" << std::endl;
            return 1;
        }
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ Application loaded and rendered" << std::endl;

        // 将文档关联到窗口并显示
        window->SetDocument(document);
        window->Show();

        // 执行一次渲染以构建渲染树
        window->RenderDocument();
        window->SwapBuffers();

        // 打印布局树信息
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  📊 Layout Tree (MBink)" << std::endl;
        std::cout << "========================================" << std::endl;

        auto render_tree = window->GetCachedRenderTree();
        if (render_tree) {
            PrintLayoutTree(render_tree);
        } else {
            std::cout << "  (No render tree available)" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Layout Compare Test Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  打开 browser.html 在浏览器中对比渲染结果" << std::endl;
        std::cout << "  examples/demo_html/layout_compare_test/browser.html" << std::endl;
        std::cout << std::endl;
        std::cout << "  Close window to exit." << std::endl;
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
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

