/**
 * @file integration_example.cpp
 * @brief 模块集成示例 - 演示 Window + DOM + Renderer 集成
 * 
 * 功能：
 * - 创建窗口
 * - 创建 DOM 文档
 * - 渲染 DOM 到窗口
 * - 事件循环
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/event/event_loop.h"
#include <iostream>
#include <memory>

using namespace lightui;

int main(int argc, char* argv[]) {
    std::cout << "=== LightUI Integration Example ===" << std::endl;
    
    try {
        // 1. 创建窗口
        std::cout << "Creating window..." << std::endl;
        WindowConfig config;
        config.title = "LightUI Integration Example";
        config.width = 800;
        config.height = 600;
        config.backend = RenderBackend::AUTO;
        
        auto window = std::make_shared<Window>(config);
        window->Show();

        // 注册到 WindowManager
        WindowManager::Instance().RegisterWindow(window);
        
        // 2. 创建 DOM 文档
        std::cout << "Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        
        // 创建 HTML 结构
        auto html = document->CreateElement("html");
        auto body = document->CreateElement("body");
        body->SetAttribute("id", "main-body");
        body->SetStyle("background-color", "#f0f0f0");
        body->SetStyle("padding", "20px");
        
        // 创建标题
        auto title = document->CreateElement("h1");
        title->SetStyle("color", "#333333");
        title->SetStyle("font-size", "32px");
        auto title_text = document->CreateTextNode("Hello, LightUI!");
        title->AppendChild(title_text);
        body->AppendChild(title);
        
        // 创建段落
        auto paragraph = document->CreateElement("p");
        paragraph->SetStyle("color", "#666666");
        paragraph->SetStyle("font-size", "16px");
        auto para_text = document->CreateTextNode("This is a demonstration of LightUI's integrated rendering pipeline.");
        paragraph->AppendChild(para_text);
        body->AppendChild(paragraph);
        
        // 创建按钮
        auto button = document->CreateElement("button");
        button->SetAttribute("id", "test-button");
        button->SetStyle("background-color", "#3498db");
        button->SetStyle("color", "#ffffff");
        button->SetStyle("padding", "10px 20px");
        button->SetStyle("border-radius", "5px");
        auto button_text = document->CreateTextNode("Click Me");
        button->AppendChild(button_text);
        body->AppendChild(button);
        
        html->AppendChild(body);
        document->AppendChild(html);
        document->SetBody(body);
        
        // 3. 将文档设置到窗口
        std::cout << "Setting document to window..." << std::endl;
        window->SetDocument(document);
        
        // 4. 创建事件循环
        std::cout << "Creating event loop..." << std::endl;
        EventLoop event_loop;
        
        // 设置更新回调
        event_loop.SetUpdateCallback([&](float delta_time) {
            // 更新逻辑（如果需要）
        });
        
        // 设置渲染回调
        event_loop.SetRenderCallback([&]() {
            // 渲染文档
            if (window->NeedsRepaint()) {
                window->RenderDocument();
                window->SwapBuffers();
            }
        });
        
        // 设置空闲回调
        event_loop.SetIdleCallback([&]() {
            // 空闲时的处理
        });
        
        // 5. 运行事件循环
        std::cout << "Starting event loop..." << std::endl;
        std::cout << "Press Ctrl+C or close the window to exit." << std::endl;
        
        while (!window->ShouldClose()) {
            event_loop.RunOnce();
            
            // 检查是否需要退出
            if (window->ShouldClose()) {
                break;
            }
        }
        
        std::cout << "Event loop stopped." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}

