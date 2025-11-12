/**
 * @file main.cpp
 * @brief Preact Counter示例主程序
 */

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/dom_bindings.h"
#include "core/quickjs/preact_bindings.h"
#include "core/quickjs/preact_renderer.h"
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

// 打印DOM树
void PrintDOMTree(std::shared_ptr<dom::Node> node, int indent = 0) {
    std::string indent_str(indent * 2, ' ');
    
    if (node->GetNodeType() == dom::Node::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<dom::Element>(node);
        std::cout << indent_str << "<" << element->GetTagName() << ">" << std::endl;
        
        auto children = element->GetChildNodes();
        for (auto& child : children) {
            PrintDOMTree(child, indent + 1);
        }
        
        std::cout << indent_str << "</" << element->GetTagName() << ">" << std::endl;
    } else if (node->GetNodeType() == dom::Node::TEXT_NODE) {
        auto text = std::static_pointer_cast<dom::Text>(node);
        std::string content = text->GetData();
        // 只打印非空白文本
        if (!content.empty() && content.find_first_not_of(" \t\n\r") != std::string::npos) {
            std::cout << indent_str << "\"" << content << "\"" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    JSContext* ctx = nullptr;
    
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink + Preact Counter Example" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 1. 创建QuickJS运行时
        std::cout << "[1/5] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        ctx = runtime->GetContext();
        
        // 2. 创建Document
        std::cout << "[2/5] Creating document..." << std::endl;
        auto document = std::make_shared<dom::Document>();
        
        // 3. 创建PreactRenderer
        std::cout << "[3/5] Creating Preact renderer..." << std::endl;
        auto renderer = std::make_shared<PreactRenderer>(ctx, document);
        
        // 4. 初始化绑定
        std::cout << "[4/5] Initializing bindings..." << std::endl;
        DOMBindings::RegisterDOMModule(ctx, document);
        PreactBindings::RegisterPreactModule(ctx, renderer);
        
        // 5. 加载Preact库
        std::cout << "[5/5] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  - preact.js loaded" << std::endl;
        
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  - hooks.js loaded" << std::endl;
        
        // 6. 加载并运行应用
        std::cout << "[6/6] Loading and running app..." << std::endl;
        std::string app_code = ReadFile("examples/preact_counter/app.js");
        runtime->Eval(app_code, "app.js");
        
        // 打印DOM树
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Application Rendered Successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "DOM Tree:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        PrintDOMTree(document->GetBody());
        std::cout << "----------------------------------------" << std::endl;
        std::cout << std::endl;
        
        // 打印统计信息
        auto body = document->GetBody();
        auto children = body->GetChildNodes();
        std::cout << "Statistics:" << std::endl;
        std::cout << "  - Body children: " << children.size() << std::endl;
        
        // 计算总元素数
        int total_elements = 0;
        std::function<void(std::shared_ptr<dom::Node>)> count_elements;
        count_elements = [&](std::shared_ptr<dom::Node> node) {
            if (node->GetNodeType() == dom::Node::ELEMENT_NODE) {
                total_elements++;
                auto element = std::static_pointer_cast<dom::Element>(node);
                auto children = element->GetChildNodes();
                for (auto& child : children) {
                    count_elements(child);
                }
            }
        };
        count_elements(body);
        std::cout << "  - Total elements: " << total_elements << std::endl;
        std::cout << std::endl;
        
        std::cout << "✅ Example completed successfully!" << std::endl;
        
        // 清理Preact绑定
        if (ctx) {
            PreactBindings::Cleanup(ctx);
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        
        // 清理Preact绑定
        if (ctx) {
            PreactBindings::Cleanup(ctx);
        }
        
        return 1;
    }
}

