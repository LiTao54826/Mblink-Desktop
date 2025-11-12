/**
 * @file main.cpp
 * @brief Preact Hello World示例 - 主程序
 */

#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/preact_renderer.h"
#include "core/quickjs/preact_bindings.h"
#include "core/dom/dom_bindings.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"
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
        std::cout << indentStr << "<" << element->GetTagName() << ">" << std::endl;

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
        std::cout << "  MBink + Preact Hello World Example" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建QuickJS运行时
        std::cout << "[1/6] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        ctx = runtime->GetContext();
        
        // 2. 创建Document
        std::cout << "[2/6] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        
        // 3. 创建Preact渲染器
        std::cout << "[3/6] Creating Preact renderer..." << std::endl;
        auto renderer = std::make_shared<PreactRenderer>(runtime.get(), document);
        
        // 4. 初始化绑定
        std::cout << "[4/6] Initializing bindings..." << std::endl;
        DOMBindings::Init(runtime->GetContext());
        PreactBindings::Init(runtime->GetContext(), renderer);
        
        // 暴露document到JavaScript
        JSValue global = JS_GetGlobalObject(runtime->GetContext());
        JSValue doc_obj = DOMBindings::WrapDocument(runtime->GetContext(), document);
        JS_SetPropertyStr(runtime->GetContext(), global, "document", doc_obj);
        JS_FreeValue(runtime->GetContext(), global);
        
        // 5. 加载Preact库
        std::cout << "[5/6] Loading Preact library..." << std::endl;
        
        // 加载preact.js
        std::string preact_code = ReadFile("js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  - preact.js loaded" << std::endl;
        
        // 加载hooks.js
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  - hooks.js loaded" << std::endl;
        
        // 测试：直接在C++中创建DOM
        std::cout << "[6/6] Testing direct DOM creation..." << std::endl;
        {
            auto body = document->GetBody();
            auto div = document->CreateElement("div");
            auto h1 = document->CreateElement("h1");
            auto text = document->CreateTextNode("Hello from C++!");
            h1->AppendChild(text);
            div->AppendChild(h1);
            body->AppendChild(div);
        }
        std::cout << "Direct DOM creation successful!" << std::endl;

        // 6. 加载并运行应用
        std::cout << "[6/6] Loading and running app..." << std::endl;
        std::string app_code = ReadFile("examples/preact_hello_world/app.js");
        runtime->Eval(app_code, "app.js");
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Application Rendered Successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 打印DOM树
        std::cout << "DOM Tree:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        auto body = document->GetBody();
        if (body) {
            PrintDOMTree(body);
        }
        std::cout << "----------------------------------------" << std::endl;
        std::cout << std::endl;
        
        // 打印统计信息
        std::cout << "Statistics:" << std::endl;
        std::cout << "  - Body children: " << (body ? body->GetChildNodes().size() : 0) << std::endl;
        
        // 计算总元素数
        int totalElements = 0;
        std::function<void(std::shared_ptr<Node>)> countElements = [&](std::shared_ptr<Node> node) {
            if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                totalElements++;
                auto element = std::static_pointer_cast<Element>(node);
                for (const auto& child : element->GetChildNodes()) {
                    countElements(child);
                }
            }
        };
        if (body) {
            countElements(body);
        }
        std::cout << "  - Total elements: " << totalElements << std::endl;
        
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

