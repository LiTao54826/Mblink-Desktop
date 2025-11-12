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

        // 加载HTML内容 - 紧凑布局测试所有元素
        std::string html = R"(
            <!DOCTYPE html>
            <html>
                <head>
                    <title>MBink Test - Compact Layout</title>
                </head>
                <body style="padding: 10px; font-size: 14px;">
                    <h1 style="margin: 5px 0;">MBink Elements Test</h1>

                    <!-- 第一行：按钮 -->
                    <h2 style="margin: 5px 0;">Buttons</h2>
                    <p style="margin: 5px 0;">
                        <button>Default</button>
                        <button style="background-color: #4CAF50; color: white;">Green</button>
                        <button style="background-color: #f44336; color: white;">Red</button>
                    </p>

                    <!-- 第二行：文本输入 -->
                    <h2 style="margin: 5px 0;">Text Input</h2>
                    <p style="margin: 5px 0;">
                        <input type="text" placeholder="Text" style="padding: 4px; width: 120px;">
                        <input type="password" placeholder="Password" style="padding: 4px; width: 120px;">
                    </p>

                    <!-- 第三行：Checkbox 和 Radio -->
                    <h2 style="margin: 5px 0;">Checkbox & Radio</h2>
                    <p style="margin: 5px 0;">
                        <input type="checkbox"> CB1
                        <input type="checkbox" checked> CB2✓
                        <input type="radio" name="r1"> R1
                        <input type="radio" name="r1" checked> R2✓
                    </p>

                    <!-- 第四行：Textarea 和 Select -->
                    <h2 style="margin: 5px 0;">Textarea & Select</h2>
                    <p style="margin: 5px 0;">
                        <textarea rows="2" cols="30" placeholder="Multi-line text..."></textarea>
                        <select style="padding: 4px;">
                            <option>Option 1</option>
                            <option selected>Option 2 (selected)</option>
                            <option>Option 3</option>
                        </select>
                    </p>

                    <!-- 第五行：表格 -->
                    <h2 style="margin: 5px 0;">Table</h2>
                    <table border="1" style="border-collapse: collapse; font-size: 12px;">
                        <tr>
                            <th>H1</th>
                            <th>H2</th>
                            <th>H3</th>
                        </tr>
                        <tr>
                            <td>R1C1</td>
                            <td>R1C2</td>
                            <td>R1C3</td>
                        </tr>
                        <tr>
                            <td>R2C1</td>
                            <td>R2C2</td>
                            <td>R2C3</td>
                        </tr>
                    </table>

                    <!-- 第六行：代码和引用 -->
                    <h2 style="margin: 5px 0;">Code & Quote</h2>
                    <p style="margin: 5px 0;">Inline: <code>console.log('hi')</code></p>
                    <pre style="margin: 5px 0; padding: 5px; background: #f5f5f5;">function test() { return 42; }</pre>
                    <blockquote style="margin: 5px 0; padding: 5px;">Quote with background</blockquote>

                    <!-- 第七行：图片和HR -->
                    <h2 style="margin: 5px 0;">Image & HR</h2>
                    <p style="margin: 5px 0;">
                        <img src="test.png" alt="Img" width="60" height="60" style="background-color: #ddd; border: 1px solid #999;">
                    </p>
                    <hr style="margin: 5px 0;">
                    <p style="margin: 5px 0; font-size: 12px;">✅ All elements visible in one screen</p>

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

