/**
 * @file main.cpp
 * @brief Preact Counter示例 - 使用原生 Preact JavaScript
 */

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
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
void PrintDOMTree(std::shared_ptr<Node> node, int indent = 0) {
    std::string indent_str(indent * 2, ' ');

    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        std::cout << indent_str << "<" << element->GetTagName() << ">" << std::endl;

        auto children = element->GetChildNodes();
        for (auto& child : children) {
            PrintDOMTree(child, indent + 1);
        }

        std::cout << indent_str << "</" << element->GetTagName() << ">" << std::endl;
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        std::string content = text->GetData();
        // 只打印非空白文本
        if (!content.empty() && content.find_first_not_of(" \t\n\r") != std::string::npos) {
            std::cout << indent_str << "\"" << content << "\"" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink + Preact Counter Example" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建QuickJS运行时
        std::cout << "[1/4] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();

        // 2. 创建Document并初始化DOM绑定
        std::cout << "[2/4] Initializing DOM bindings..." << std::endl;
        auto document = std::make_shared<Document>();
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);

        // 3. 创建body元素
        std::cout << "[3/4] Creating body element..." << std::endl;
        auto body = document->CreateElement("body");
        document->SetBody(body);

        // 4. 加载并执行Preact代码
        std::cout << "[4/4] Loading Preact..." << std::endl;

        // 加载 Preact 核心 (相对于 build/bin/Release/)
        std::string preact_code = ReadFile("../../examples/demo_html/js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");

        // 加载 Preact Hooks
        std::string hooks_code = ReadFile("../../examples/demo_html/js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");

        // 加载应用代码
        std::string app_code = ReadFile("../../examples/demo_html/preact_counter/app.js");
        runtime->Eval(app_code, "app.js");

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Rendering Complete!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 打印DOM树
        std::cout << "DOM Tree:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        PrintDOMTree(body);
        std::cout << "----------------------------------------" << std::endl;
        std::cout << std::endl;

        // 计算总元素数
        int total_elements = 0;
        std::function<void(std::shared_ptr<Node>)> count_elements;
        count_elements = [&](std::shared_ptr<Node> node) {
            if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                total_elements++;
                auto element = std::static_pointer_cast<Element>(node);
                auto children = element->GetChildNodes();
                for (auto& child : children) {
                    count_elements(child);
                }
            }
        };
        count_elements(body);

        std::cout << "Statistics:" << std::endl;
        std::cout << "  Total elements: " << total_elements << std::endl;
        std::cout << std::endl;

        std::cout << "✓ Example completed successfully!" << std::endl;

        // 清理（在 runtime 析构前）
        DOMBindings::Cleanup(ctx);
        document.reset();
        body.reset();

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

