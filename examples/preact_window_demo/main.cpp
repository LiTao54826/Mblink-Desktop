/**
 * @file main.cpp
 * @brief Preact Window Demo - 带GUI窗口的Preact应用
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
std::string ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 打印DOM树（用于调试）
void PrintDOMTree(std::shared_ptr<Node> node, int indent = 0) {
    std::string indentStr(indent * 2, ' ');

    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        std::cout << indentStr << "<" << element->GetTagName();

        // 打印属性
        auto style = element->GetAttribute("style");
        if (!style.empty()) {
            std::cout << " style=\"" << style << "\"";
        }

        std::cout << ">" << std::endl;

        // 打印子节点
        auto children = element->GetChildNodes();
        for (const auto& child : children) {
            PrintDOMTree(child, indent + 1);
        }

        std::cout << indentStr << "</" << element->GetTagName() << ">" << std::endl;
    }
    else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        std::string content = text->GetData();
        // 去除空白文本节点
        if (!content.empty() && content.find_first_not_of(" \t\n\r") != std::string::npos) {
            std::cout << indentStr << "\"" << content << "\"" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    JSContext* ctx = nullptr;
    
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink + Preact Window Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 1. 创建窗口
        std::cout << "[1/8] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink + Preact Demo";
        config.width = 600;
        config.height = 400;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created: " << config.width << "x" << config.height << std::endl;
        
        // 注册窗口到 WindowManager
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window registered" << std::endl;
        
        // 2. 创建Document
        std::cout << "[2/6] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;

        // 3. 创建QuickJS运行时
        std::cout << "[3/6] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        ctx = runtime->GetContext();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;

        // 4. 初始化DOM绑定
        std::cout << "[4/6] Initializing DOM bindings..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        std::cout << "  ✓ DOM bindings initialized" << std::endl;

        // 5. 加载Preact库
        std::cout << "[5/6] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  ✓ preact.js loaded" << std::endl;

        std::string hooks_code = ReadFile("js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ hooks.js loaded" << std::endl;

        // 6. 加载并运行应用
        std::cout << "[6/6] Loading and running app..." << std::endl;
        std::string app_code = ReadFile("examples/preact_window_demo/app.js");
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ App loaded and rendered" << std::endl;

        // 将文档关联到窗口
        window->SetDocument(document);
        std::cout << "  ✓ Document attached to window" << std::endl;

        // 打印DOM树（调试）
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  DOM Tree:" << std::endl;
        std::cout << "========================================" << std::endl;
        PrintDOMTree(document->GetDocumentElement());
        std::cout << "========================================" << std::endl;

        // 打印body的子节点数量
        auto body = document->GetBody();
        if (body) {
            auto children = body->GetChildNodes();
            std::cout << "Body has " << children.size() << " child nodes" << std::endl;
            for (size_t i = 0; i < children.size(); ++i) {
                auto child = children[i];
                if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto elem = std::static_pointer_cast<Element>(child);
                    std::cout << "  Child " << i << ": <" << elem->GetTagName() << ">" << std::endl;
                } else if (child->GetNodeType() == NodeType::TEXT_NODE) {
                    auto text = std::static_pointer_cast<Text>(child);
                    std::cout << "  Child " << i << ": TEXT \"" << text->GetData() << "\"" << std::endl;
                }
            }
        }
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 显示窗口
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

        // 清理DOM绑定
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

