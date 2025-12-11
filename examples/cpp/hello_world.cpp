/**
 * @file hello_world.cpp
 * @brief LightUI Hello World 示例
 * 
 * 这是一个最简单的 LightUI 应用示例，展示如何：
 * 1. 创建窗口
 * 2. 设置 DOM 结构
 * 3. 运行事件循环
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
    std::cout << "=== LightUI Hello World ===" << std::endl;
    
    try {
        // 1. 创建窗口
        WindowConfig config;
        config.title = "Hello LightUI";
        config.width = 800;
        config.height = 600;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        std::cout << "✓ Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口到 WindowManager
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "✓ Window registered" << std::endl;

        // 2. 创建 DOM 文档
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "✓ Document initialized" << std::endl;
        
        // 3. 构建 DOM 结构
        auto body = document->GetBody();

        // 给 body 设置白色背景
        body->SetAttribute("style", "background-color: white;");

        // 创建容器
        auto container = document->CreateElement("div");
        container->SetAttribute("id", "container");
        
        // 创建标题
        auto title = document->CreateElement("h1");
        title->SetTextContent("Hello, LightUI!");
        title->SetAttribute("id", "title");
        
        // 创建描述
        auto description = document->CreateElement("p");
        description->SetTextContent("Welcome to LightUI - A lightweight cross-platform UI framework");
        description->SetAttribute("id", "description");
        
        // 创建版本信息
        auto version = document->CreateElement("p");
        version->SetTextContent("Version: 0.2.0-alpha");
        version->SetAttribute("id", "version");
        
        // 组装 DOM 树
        body->AppendChild(container);
        container->AppendChild(title);
        container->AppendChild(description);
        container->AppendChild(version);
        
        // 设置 ID（必须在 AppendChild 之后）
        container->SetAttribute("id", "container");
        title->SetAttribute("id", "title");
        description->SetAttribute("id", "description");
        version->SetAttribute("id", "version");
        
        std::cout << "✓ DOM structure created" << std::endl;
        
        // 4. 将文档关联到窗口
        window->SetDocument(document);
        std::cout << "✓ Document attached to window" << std::endl;
        
        // 5. 显示窗口
        window->Show();
        std::cout << "✓ Window shown" << std::endl;
        
        // 6. 创建事件循环
        EventLoop event_loop;
        
        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();  // 显示到屏幕
            }
        });
        
        std::cout << "✓ Event loop configured" << std::endl;
        std::cout << "\n🚀 Application running... (Close window to exit)\n" << std::endl;
        
        // 7. 运行事件循环
        event_loop.Run();
        
        std::cout << "\n✓ Application exited normally" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

