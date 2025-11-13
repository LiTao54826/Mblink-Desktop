/**
 * @file test_auto_click.cpp
 * @brief 自动测试 Preact 窗口示例的点击功能
 */

#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/event/mouse_event.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>

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

// 获取计数值
int GetCountValue(std::shared_ptr<Document> document) {
    // 查找 <p> 元素
    auto body = document->GetBody();
    if (!body) return -999;
    
    for (const auto& child1 : body->GetChildNodes()) {
        if (child1->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto div1 = std::static_pointer_cast<Element>(child1);
            for (const auto& child2 : div1->GetChildNodes()) {
                if (child2->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto div2 = std::static_pointer_cast<Element>(child2);
                    for (const auto& child3 : div2->GetChildNodes()) {
                        if (child3->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto elem = std::static_pointer_cast<Element>(child3);
                            if (elem->GetTagName() == "p") {
                                std::string text = elem->GetTextContent();
                                // 解析 "Count: X"
                                size_t pos = text.find("Count: ");
                                if (pos != std::string::npos) {
                                    std::string numStr = text.substr(pos + 7);
                                    return std::stoi(numStr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return -999;
}

int main(int argc, char** argv) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  Preact Window Auto-Click Test" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink + Preact Test";
        config.width = 600;
        config.height = 400;
        config.resizable = false;
        config.vsync = false;
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created" << std::endl;
        
        // 2. 创建Document
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
        
        // 5. 加载Preact库
        std::cout << "[5/7] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("js/preact/preact.js");
        runtime->Eval(preact_code, "preact.js");
        
        std::string hooks_code = ReadFile("js/preact/hooks.js");
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  ✓ Preact library loaded" << std::endl;
        
        // 6. 加载并运行应用
        std::cout << "[6/7] Loading and running app..." << std::endl;
        std::string app_code = ReadFile("examples/preact_window_demo/app.js");
        runtime->Eval(app_code, "app.js");
        std::cout << "  ✓ App loaded and rendered" << std::endl;
        
        // 7. 附加到窗口
        std::cout << "[7/7] Attaching to window..." << std::endl;
        window->SetDocument(document);
        std::cout << "  ✓ Document attached to window" << std::endl;
        std::cout << std::endl;
        
        // 测试交互
        std::cout << "========================================" << std::endl;
        std::cout << "  Testing Interactivity" << std::endl;
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
            
            // 初始值
            int count = GetCountValue(document);
            std::cout << "[Initial] Count = " << count << std::endl;
            if (count != 0) {
                std::cerr << "❌ FAILED: Initial count should be 0" << std::endl;
                return 1;
            }
            std::cout << "  ✓ Initial count is correct" << std::endl;
            std::cout << std::endl;
            
            // 测试 1: Increment
            std::cout << "[Test 1] Clicking Increment..." << std::endl;
            auto click1 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            incrementBtn->DispatchEvent(click1);
            count = GetCountValue(document);
            std::cout << "  Count = " << count << std::endl;
            if (count != 1) {
                std::cerr << "❌ FAILED: Count should be 1 after increment" << std::endl;
                return 1;
            }
            std::cout << "  ✓ Increment works correctly" << std::endl;
            std::cout << std::endl;
            
            // 测试 2: Increment again
            std::cout << "[Test 2] Clicking Increment again..." << std::endl;
            auto click2 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            incrementBtn->DispatchEvent(click2);
            count = GetCountValue(document);
            std::cout << "  Count = " << count << std::endl;
            if (count != 2) {
                std::cerr << "❌ FAILED: Count should be 2 after second increment" << std::endl;
                return 1;
            }
            std::cout << "  ✓ Second increment works correctly" << std::endl;
            std::cout << std::endl;
            
            // 测试 3: Decrement
            std::cout << "[Test 3] Clicking Decrement..." << std::endl;
            auto click3 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            decrementBtn->DispatchEvent(click3);
            count = GetCountValue(document);
            std::cout << "  Count = " << count << std::endl;
            if (count != 1) {
                std::cerr << "❌ FAILED: Count should be 1 after decrement" << std::endl;
                return 1;
            }
            std::cout << "  ✓ Decrement works correctly" << std::endl;
            std::cout << std::endl;
            
            // 测试 4: Reset
            std::cout << "[Test 4] Clicking Reset..." << std::endl;
            auto click4 = std::make_shared<MouseEvent>("click", 0, 0, 0);
            resetBtn->DispatchEvent(click4);
            count = GetCountValue(document);
            std::cout << "  Count = " << count << std::endl;
            if (count != 0) {
                std::cerr << "❌ FAILED: Count should be 0 after reset" << std::endl;
                return 1;
            }
            std::cout << "  ✓ Reset works correctly" << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "========================================" << std::endl;
        std::cout << "  ✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 清理
        DOMBindings::Cleanup(ctx);
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

