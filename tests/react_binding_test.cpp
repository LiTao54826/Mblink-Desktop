/**
 * @file react_binding_test.cpp
 * @brief React 绑定功能测试程序
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/event/task_scheduler.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"

using namespace lightui;

std::string ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "  React (Preact) Binding Test\n";
    std::cout << "=================================================\n\n";

    try {
        // 创建窗口配置
        WindowConfig config;
        config.title = "React Binding Test";
        config.width = 800;
        config.height = 600;

        // 创建窗口
        std::cout << "Creating window...\n";
        auto window = std::make_shared<Window>(config);
        
        // 创建 DOM 文档
        auto document = std::make_shared<Document>();
        document->Initialize();
        window->SetDocument(document);
        
        // 创建 QuickJS 运行时
        auto js_runtime = std::make_shared<QuickJSRuntime>();
        
        // 创建任务调度器
        auto task_scheduler = std::make_shared<TaskScheduler>();
        
        // 创建并初始化 JavaScript 绑定
        WindowBindings window_bindings(js_runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        
        std::cout << "Window and bindings created successfully\n\n";

        // ========== 测试 1: DOM 绑定 ==========
        std::cout <<"\n";
        std::cout << "=================================================\n";
        std::cout << "  Running DOM Binding Tests\n";
        std::cout << "=================================================\n";
        
        try {
            std::string dom_test = ReadFile("tests/react_dom_test.js");
            js_runtime->Eval(dom_test, "react_dom_test.js");
            std::cout << "\nDOM tests completed\n";
        } catch (const std::exception& e) {
            std::cerr << "DOM Test Error: " << e.what() << "\n";
        }

        // ========== 测试 2: 事件绑定 ==========
        std::cout << "\n";
        std::cout << "=================================================\n";
        std::cout << "  Running Event Binding Tests\n";
        std::cout << "=================================================\n";
        
        try {
            std::string event_test = ReadFile("tests/react_event_test.js");
            js_runtime->Eval(event_test, "react_event_test.js");
            std::cout << "\nEvent tests completed\n";
        } catch (const std::exception& e) {
            std::cerr << "Event Test Error: " << e.what() << "\n";
        }

        std::cout << "\n";
        std::cout << "=================================================\n";
        std::cout << "  All Tests Complete\n";
        std::cout << "=================================================\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
