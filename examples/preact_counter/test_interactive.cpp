/**
 * @file test_interactive.cpp
 * @brief 测试 Preact 交互功能（自动触发点击事件）
 */

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/mouse_event.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <thread>
#include <chrono>

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
    std::string indentStr(indent * 2, ' ');

    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        std::cout << indentStr << "<" << element->GetTagName() << ">" << std::endl;

        for (const auto& child : node->GetChildNodes()) {
            PrintDOMTree(child, indent + 1);
        }

        std::cout << indentStr << "</" << element->GetTagName() << ">" << std::endl;
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        std::string content = text->GetData();
        if (!content.empty() && content.find_first_not_of(" \t\n\r") != std::string::npos) {
            std::cout << indentStr << "\"" << content << "\"" << std::endl;
        }
    }
}

// 查找所有按钮
std::vector<std::shared_ptr<Element>> FindButtons(std::shared_ptr<Node> node) {
    std::vector<std::shared_ptr<Element>> buttons;
    
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() == "button") {
            buttons.push_back(element);
        }
        
        for (const auto& child : node->GetChildNodes()) {
            auto child_buttons = FindButtons(child);
            buttons.insert(buttons.end(), child_buttons.begin(), child_buttons.end());
        }
    }
    
    return buttons;
}

int main(int argc, char** argv) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  Preact Interactive Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 1. 创建QuickJS运行时
        std::cout << "[1/5] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();
        std::cout << "  ✓ QuickJS runtime created" << std::endl;
        
        // 2. 创建Document并初始化DOM绑定
        std::cout << "[2/5] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        std::cout << "  ✓ Document initialized" << std::endl;
        
        // 3. 加载Preact代码
        std::cout << "[3/5] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");
        
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;
        
        // 4. 加载并运行应用
        std::cout << "[4/5] Loading and running app..." << std::endl;
        std::string app_code = ReadFile("examples/preact_counter/app.js");
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ App loaded and rendered" << std::endl;
        
        // 5. 测试交互
        std::cout << "[5/5] Testing interactivity..." << std::endl;
        std::cout << std::endl;
        
        // 打印初始DOM树
        std::cout << "========================================" << std::endl;
        std::cout << "  Initial DOM Tree:" << std::endl;
        std::cout << "========================================" << std::endl;
        PrintDOMTree(document->GetBody());
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 查找按钮
        auto buttons = FindButtons(document->GetBody());
        std::cout << "Found " << buttons.size() << " buttons" << std::endl;
        std::cout << std::endl;
        
        if (buttons.size() >= 3) {
            auto incrementBtn = buttons[0];
            auto decrementBtn = buttons[1];
            auto resetBtn = buttons[2];
            
            // 测试 1: 点击 Increment
            std::cout << "[Test 1] Clicking Increment button..." << std::endl;
            auto click_event1 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            incrementBtn->DispatchEvent(click_event1);
            std::cout << "  ✓ Event dispatched" << std::endl;
            std::cout << std::endl;
            
            // 打印更新后的DOM
            std::cout << "DOM after first click:" << std::endl;
            PrintDOMTree(document->GetBody());
            std::cout << std::endl;
            
            // 测试 2: 再次点击 Increment
            std::cout << "[Test 2] Clicking Increment button again..." << std::endl;
            auto click_event2 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            incrementBtn->DispatchEvent(click_event2);
            std::cout << "  ✓ Event dispatched" << std::endl;
            std::cout << std::endl;
            
            // 打印更新后的DOM
            std::cout << "DOM after second click:" << std::endl;
            PrintDOMTree(document->GetBody());
            std::cout << std::endl;
            
            // 测试 3: 点击 Decrement
            std::cout << "[Test 3] Clicking Decrement button..." << std::endl;
            auto click_event3 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            decrementBtn->DispatchEvent(click_event3);
            std::cout << "  ✓ Event dispatched" << std::endl;
            std::cout << std::endl;
            
            // 打印更新后的DOM
            std::cout << "DOM after decrement:" << std::endl;
            PrintDOMTree(document->GetBody());
            std::cout << std::endl;
            
            // 测试 4: 点击 Reset
            std::cout << "[Test 4] Clicking Reset button..." << std::endl;
            auto click_event4 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            resetBtn->DispatchEvent(click_event4);
            std::cout << "  ✓ Event dispatched" << std::endl;
            std::cout << std::endl;
            
            // 打印最终DOM
            std::cout << "Final DOM after reset:" << std::endl;
            PrintDOMTree(document->GetBody());
            std::cout << std::endl;
        }
        
        std::cout << "========================================" << std::endl;
        std::cout << "  ✓ All tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 清理
        DOMBindings::Cleanup(ctx);
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

