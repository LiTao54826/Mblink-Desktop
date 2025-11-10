/**
 * @file counter_app.cpp
 * @brief LightUI 计数器应用示例
 * 
 * 展示如何：
 * 1. 使用 JavaScript 绑定
 * 2. 处理用户交互
 * 3. 动态更新 DOM
 * 4. 使用定时器
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/event/event_loop.h"
#include "core/event/task_scheduler.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include <iostream>
#include <memory>

using namespace lightui;

int main(int argc, char* argv[]) {
    std::cout << "=== LightUI Counter App ===" << std::endl;
    
    try {
        // 1. 创建窗口
        WindowConfig config;
        config.title = "Counter App";
        config.width = 400;
        config.height = 300;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        std::cout << "✓ Window created" << std::endl;

        // 注册窗口到 WindowManager
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "✓ Window registered" << std::endl;

        // 2. 创建 DOM 文档
        auto document = std::make_shared<Document>();
        document->Initialize();
        
        // 3. 创建任务调度器
        auto scheduler = std::make_shared<TaskScheduler>();

        // 4. 创建 JavaScript 运行时
        QuickJSRuntime runtime;

        // 5. 创建 JavaScript 绑定
        WindowBindings bindings(&runtime, window, scheduler);

        std::cout << "✓ JavaScript runtime initialized" << std::endl;
        
        // 6. 构建 DOM 结构
        auto body = document->GetBody();
        
        // 容器
        auto container = document->CreateElement("div");
        body->AppendChild(container);
        container->SetAttribute("id", "container");
        
        // 标题
        auto title = document->CreateElement("h1");
        title->SetTextContent("Counter App");
        container->AppendChild(title);
        
        // 计数显示
        auto counter_display = document->CreateElement("div");
        counter_display->SetTextContent("Count: 0");
        container->AppendChild(counter_display);
        counter_display->SetAttribute("id", "counter");
        
        // 按钮容器
        auto button_container = document->CreateElement("div");
        container->AppendChild(button_container);
        button_container->SetAttribute("id", "buttons");
        
        // 增加按钮
        auto increment_btn = document->CreateElement("button");
        increment_btn->SetTextContent("Increment (+1)");
        button_container->AppendChild(increment_btn);
        increment_btn->SetAttribute("id", "increment");
        
        // 减少按钮
        auto decrement_btn = document->CreateElement("button");
        decrement_btn->SetTextContent("Decrement (-1)");
        button_container->AppendChild(decrement_btn);
        decrement_btn->SetAttribute("id", "decrement");
        
        // 重置按钮
        auto reset_btn = document->CreateElement("button");
        reset_btn->SetTextContent("Reset");
        button_container->AppendChild(reset_btn);
        reset_btn->SetAttribute("id", "reset");
        
        // 自动计数按钮
        auto auto_btn = document->CreateElement("button");
        auto_btn->SetTextContent("Auto Count");
        button_container->AppendChild(auto_btn);
        auto_btn->SetAttribute("id", "auto");
        
        std::cout << "✓ DOM structure created" << std::endl;
        
        // 7. 将文档关联到窗口
        window->SetDocument(document);
        
        // 8. 初始化 JavaScript 应用逻辑
        runtime.Eval(R"(
            // 初始化计数器
            globalThis.count = 0;
            globalThis.autoIntervalId = null;
            
            // 更新显示
            function updateDisplay() {
                const counter = document.getElementById('counter');
                if (counter) {
                    counter.textContent = 'Count: ' + globalThis.count;
                }
            }
            
            // 增加
            function increment() {
                globalThis.count++;
                updateDisplay();
            }
            
            // 减少
            function decrement() {
                globalThis.count--;
                updateDisplay();
            }
            
            // 重置
            function reset() {
                globalThis.count = 0;
                updateDisplay();
                
                // 停止自动计数
                if (globalThis.autoIntervalId !== null) {
                    clearInterval(globalThis.autoIntervalId);
                    globalThis.autoIntervalId = null;
                }
            }
            
            // 自动计数
            function toggleAutoCount() {
                if (globalThis.autoIntervalId === null) {
                    // 开始自动计数
                    globalThis.autoIntervalId = setInterval(function() {
                        globalThis.count++;
                        updateDisplay();
                    }, 1000);
                    console.log('Auto count started');
                } else {
                    // 停止自动计数
                    clearInterval(globalThis.autoIntervalId);
                    globalThis.autoIntervalId = null;
                    console.log('Auto count stopped');
                }
            }
            
            console.log('Counter app initialized');
        )", "counter_app.js");
        
        std::cout << "✓ JavaScript initialized" << std::endl;
        
        // 9. 显示窗口
        window->Show();
        std::cout << "✓ Window shown" << std::endl;
        
        // 10. 创建事件循环
        EventLoop event_loop;
        
        // 设置更新回调（处理定时器）
        event_loop.SetUpdateCallback([scheduler, &runtime](float delta_time) {
            scheduler->ProcessTasks();
            runtime.ProcessMicrotasks();
        });
        
        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->RenderDocument();
                window->SwapBuffers();  // 显示到屏幕
            }
        });
        
        std::cout << "✓ Event loop configured" << std::endl;
        std::cout << "\n🚀 Counter App running..." << std::endl;
        std::cout << "   - Click buttons to interact" << std::endl;
        std::cout << "   - Use JavaScript console for debugging" << std::endl;
        std::cout << "   - Close window to exit\n" << std::endl;
        
        // 11. 运行事件循环
        event_loop.Run();
        
        std::cout << "\n✓ Application exited normally" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

