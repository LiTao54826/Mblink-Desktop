/**
 * @file javascript_integration_example.cpp
 * @brief JavaScript 集成示例
 * 
 * 演示：
 * - Window + DOM + JavaScript 集成
 * - JavaScript 访问 window 和 document 对象
 * - JavaScript 定时器功能
 * - JavaScript 操作 DOM
 */

#include "core/window/window_manager.h"
#include "core/window/window.h"
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
    std::cout << "=== JavaScript Integration Example ===" << std::endl;
    
    // 1. 创建窗口管理器
    auto& window_manager = WindowManager::Instance();

    // 2. 创建窗口配置
    WindowConfig config;
    config.title = "JavaScript Integration Example";
    config.width = 800;
    config.height = 600;
    config.resizable = true;
    config.vsync = true;

    // 3. 创建窗口
    auto window = std::make_shared<Window>(config);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }

    // 注册窗口到管理器
    window_manager.RegisterWindow(window);

    int width, height;
    window->GetSize(&width, &height);
    std::cout << "Window created: " << width << "x" << height << std::endl;
    
    // 4. 创建 DOM 文档
    auto document = std::make_shared<Document>();
    document->Initialize();  // 初始化 DOM 结构（会自动创建 html 和 body）

    // 获取自动创建的 body 元素
    auto body = document->GetBody();
    body->SetAttribute("id", "main-body");

    auto title = document->CreateElement("h1");
    title->SetAttribute("id", "title");
    title->SetTextContent("JavaScript Integration Example");
    title->SetStyle("color", "blue");
    title->SetStyle("font-size", "32px");

    auto description = document->CreateElement("p");
    description->SetAttribute("id", "description");
    description->SetTextContent("This example demonstrates JavaScript integration with LightUI");
    description->SetStyle("color", "gray");
    description->SetStyle("font-size", "16px");

    auto counter = document->CreateElement("div");
    counter->SetAttribute("id", "counter");
    counter->SetTextContent("Counter: 0");
    counter->SetStyle("color", "green");
    counter->SetStyle("font-size", "24px");
    counter->SetStyle("margin-top", "20px");
    
    body->AppendChild(title);
    body->AppendChild(description);
    body->AppendChild(counter);

    // 5. 设置文档到窗口
    window->SetDocument(document);
    
    std::cout << "Document created and attached to window" << std::endl;
    
    // 6. 创建 QuickJS 运行时
    auto js_runtime = std::make_shared<QuickJSRuntime>();
    
    std::cout << "QuickJS runtime created" << std::endl;
    
    // 7. 创建任务调度器
    auto task_scheduler = std::make_shared<TaskScheduler>();
    
    // 8. 创建 JavaScript 绑定
    WindowBindings window_bindings(js_runtime.get(), window, task_scheduler);
    window_bindings.InitBindings();
    
    std::cout << "JavaScript bindings initialized" << std::endl;
    
    // 9. 执行 JavaScript 代码
    std::string js_code = R"(
        console.log('JavaScript is running!');
        console.log('Window size: ' + window.innerWidth + 'x' + window.innerHeight);
        console.log('Device pixel ratio: ' + window.devicePixelRatio);
        console.log('Window title: ' + window.title);
        
        // 访问 document
        console.log('Document body:', document.body);
        console.log('Document element:', document.documentElement);
        
        // 查找元素
        var titleElement = document.getElementById('title');
        console.log('Title element:', titleElement);
        
        var counterElement = document.getElementById('counter');
        console.log('Counter element:', counterElement);
        
        // 定义计数器更新函数
        var count = 0;
        globalThis.updateCounter = function() {
            count++;
            console.log('Counter updated: ' + count);
            // 注意：这里只是演示，实际更新 DOM 需要更完整的绑定
        };
        
        // 使用 setTimeout
        setTimeout(function() {
            console.log('setTimeout: 1 second passed');
            updateCounter();
        }, 1000);
        
        // 使用 setInterval
        var intervalId = setInterval(function() {
            console.log('setInterval: tick');
            updateCounter();
            
            // 5 秒后停止
            if (count >= 5) {
                clearInterval(intervalId);
                console.log('Interval cleared');
            }
        }, 1000);
        
        // 使用 requestAnimationFrame
        var frameCount = 0;
        globalThis.animationLoop = function(timestamp) {
            frameCount++;
            if (frameCount % 60 === 0) {
                console.log('requestAnimationFrame: frame ' + frameCount + ', timestamp: ' + timestamp);
            }
            
            // 继续动画循环（前 300 帧）
            if (frameCount < 300) {
                requestAnimationFrame(animationLoop);
            } else {
                console.log('Animation loop finished');
            }
        };
        
        requestAnimationFrame(animationLoop);
        
        console.log('JavaScript code executed successfully');
    )";
    
    try {
        js_runtime->Eval(js_code, "<main>");
        std::cout << "JavaScript code executed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "JavaScript error: " << e.what() << std::endl;
    }
    
    // 10. 创建事件循环
    EventLoop event_loop;
    
    // 设置更新回调（EventLoop 内部已经处理 ProcessTasks 和 ProcessAnimationFrames）
    // 这里只需要处理 QuickJS microtasks
    event_loop.SetUpdateCallback([js_runtime](float delta_time) {
        js_runtime->ProcessMicrotasks();
    });
    
    // 设置渲染回调
    event_loop.SetRenderCallback([window]() {
        if (window->NeedsRepaint()) {
            window->Render();
            window->SwapBuffers();
        }
    });
    
    std::cout << "Event loop created" << std::endl;
    std::cout << "Starting main loop..." << std::endl;
    std::cout << "Press ESC or close window to exit" << std::endl;
    
    // 11. 运行事件循环
    bool running = true;
    while (running) {
        // 处理窗口事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = false;
            }
        }
        
        // 运行事件循环一帧
        event_loop.RunOnce();
        
        // 处理 JavaScript 微任务
        js_runtime->ProcessMicrotasks();
        
        // 限制帧率
        SDL_Delay(16); // ~60 FPS
    }
    
    std::cout << "Exiting..." << std::endl;

    // 12. 清理
    window_manager.UnregisterWindow(window);
    window.reset();

    std::cout << "Example finished" << std::endl;
    
    return 0;
}

