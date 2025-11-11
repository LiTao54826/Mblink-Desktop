/**
 * @file html_window_example.cpp
 * @brief 带窗口的HTML加载示例
 *
 * 演示如何在窗口中加载和渲染HTML内容
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/lexbor/lexbor_document.h"
#include "core/event/event_loop.h"
#include <iostream>

using namespace lightui;

int main() {
    std::cout << "=== MBink HTML Window Example ===" << std::endl;

    try {
        // 创建窗口
        WindowConfig config;
        config.title = "MBink HTML Comprehensive Test";
        config.width = 900;
        config.height = 800;  // 适中的高度
        config.resizable = true;
        config.vsync = true;

        auto window = std::make_shared<Window>(config);
        std::cout << "✅ Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口到 WindowManager
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "✅ Window registered" << std::endl;

        // 创建文档
        auto doc = std::make_shared<Document>();
        doc->Initialize();
        std::cout << "✅ Document initialized" << std::endl;

        // 加载HTML内容 - 测试交互元素和其他未测试的元素
        std::string html = R"(
            <!DOCTYPE html>
            <html>
                <head>
                    <title>MBink Interactive Elements Test</title>
                </head>
                <body style="padding: 20px;">
                    <h1>Interactive Elements Test</h1>

                    <!-- Buttons -->
                    <h2>Buttons</h2>
                    <p>
                        <button>Default Button</button>
                        <button style="background-color: #4CAF50; color: white; padding: 10px 20px; border: none;">Green Button</button>
                        <button style="background-color: #f44336; color: white; padding: 10px 20px; border: none;">Red Button</button>
                    </p>

                    <!-- Input Elements -->
                    <h2>Input Elements</h2>
                    <p>
                        <input type="text" placeholder="Text input" style="padding: 8px; width: 200px;">
                    </p>
                    <p>
                        <input type="password" placeholder="Password" style="padding: 8px; width: 200px;">
                    </p>
                    <p>
                        <input type="button" value="Button Input" style="padding: 8px 16px;">
                        <input type="submit" value="Submit" style="padding: 8px 16px;">
                    </p>
                    <p>
                        <input type="checkbox"> Checkbox 1
                        <input type="checkbox" checked> Checkbox 2 (checked)
                    </p>
                    <p>
                        <input type="radio" name="radio1"> Radio 1
                        <input type="radio" name="radio1" checked> Radio 2 (checked)
                    </p>

                    <!-- Textarea -->
                    <h2>Textarea</h2>
                    <p>
                        <textarea rows="4" cols="50" placeholder="Enter multiple lines..."></textarea>
                    </p>

                    <!-- Select -->
                    <h2>Select (Dropdown)</h2>
                    <p>
                        <select style="padding: 8px;">
                            <option>Option 1</option>
                            <option selected>Option 2 (selected)</option>
                            <option>Option 3</option>
                        </select>
                    </p>

                    <!-- Images -->
                    <h2>Images</h2>
                    <p>
                        <img src="test.png" alt="Test Image" width="100" height="100" style="background-color: #ddd; border: 1px solid #999;">
                    </p>

                    <!-- Tables -->
                    <h2>Tables</h2>
                    <table border="1" style="border-collapse: collapse;">
                        <tr>
                            <th>Header 1</th>
                            <th>Header 2</th>
                            <th>Header 3</th>
                        </tr>
                        <tr>
                            <td>Row 1, Col 1</td>
                            <td>Row 1, Col 2</td>
                            <td>Row 1, Col 3</td>
                        </tr>
                        <tr>
                            <td>Row 2, Col 1</td>
                            <td>Row 2, Col 2</td>
                            <td>Row 2, Col 3</td>
                        </tr>
                    </table>

                    <!-- Code blocks -->
                    <h2>Code Elements</h2>
                    <p>Inline code: <code>console.log('hello')</code></p>
                    <pre>function test() {
    return 42;
}</pre>

                    <!-- Blockquote -->
                    <h2>Blockquote</h2>
                    <blockquote style="border-left: 4px solid #ccc; padding-left: 16px; margin: 16px 0;">
                        This is a blockquote. It should have a left border and padding.
                    </blockquote>

                    <!-- Horizontal Rule -->
                    <h2>Horizontal Rule</h2>
                    <p>Text before HR</p>
                    <hr>
                    <p>Text after HR</p>

                </body>
            </html>
        )";

        if (!doc->LoadHTML(html)) {
            std::cerr << "Failed to load HTML!" << std::endl;
            return 1;
        }

        std::cout << "✅ HTML loaded successfully" << std::endl;

        // 打印文档结构
        auto body = doc->GetBody();
        if (body) {
            auto children = body->GetChildNodes();
            std::cout << "📄 Document structure:" << std::endl;
            std::cout << "   Body has " << children.size() << " child nodes" << std::endl;

            // 查找关键元素
            auto header = doc->GetElementById("header");
            if (header) {
                std::cout << "   ✓ Found #header" << std::endl;
            }

            auto content = doc->GetElementById("content");
            if (content) {
                std::cout << "   ✓ Found #content" << std::endl;
            }

            auto footer = doc->GetElementById("footer");
            if (footer) {
                std::cout << "   ✓ Found #footer" << std::endl;
            }
        }

        // 将文档关联到窗口
        window->SetDocument(doc);
        std::cout << "✅ Document attached to window" << std::endl;

        // 显示窗口
        window->Show();
        std::cout << "✅ Window shown" << std::endl;

        // 创建事件循环
        EventLoop event_loop;

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->RenderDocument();
                window->SwapBuffers();
            }
        });

        std::cout << "✅ Event loop configured" << std::endl;
        std::cout << std::endl;
        std::cout << "🚀 Starting application..." << std::endl;
        std::cout << "   Close window to exit" << std::endl;
        std::cout << std::endl;

        // 运行事件循环
        event_loop.Run();

        std::cout << std::endl;
        std::cout << "👋 Application closed" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

