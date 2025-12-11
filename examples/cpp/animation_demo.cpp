/**
 * @file animation_demo.cpp
 * @brief LightUI 动画演示
 * 
 * 展示如何：
 * 1. 使用 requestAnimationFrame
 * 2. 创建流畅的动画
 * 3. 动态更新样式
 * 4. 性能监控
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
    std::cout << "=== LightUI Animation Demo ===" << std::endl;
    
    try {
        // 1. 创建窗口
        WindowConfig config;
        config.title = "Animation Demo";
        config.width = 600;
        config.height = 400;
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

        // 3. 创建事件循环（先创建，以便获取其 TaskScheduler）
        EventLoop event_loop;

        // 4. 创建 JavaScript 运行时
        QuickJSRuntime runtime;

        // 5. 创建 JavaScript 绑定（使用 EventLoop 的 TaskScheduler）
        // 注意：这里传递的是指针，不是 shared_ptr，因为 EventLoop 拥有 TaskScheduler 的生命周期
        auto scheduler_ptr = std::shared_ptr<TaskScheduler>(&event_loop.GetTaskScheduler(), [](TaskScheduler*){});
        WindowBindings bindings(&runtime, window, scheduler_ptr);
        bindings.InitBindings();  // 初始化绑定

        std::cout << "✓ JavaScript runtime initialized" << std::endl;
        
        // 6. 构建 DOM 结构
        auto body = document->GetBody();
        
        // 容器
        auto container = document->CreateElement("div");
        body->AppendChild(container);
        container->SetAttribute("id", "container");
        
        // 标题
        auto title = document->CreateElement("h1");
        title->SetTextContent("Animation Demo");
        container->AppendChild(title);
        
        // FPS 显示
        auto fps_display = document->CreateElement("div");
        fps_display->SetTextContent("FPS: 0");
        container->AppendChild(fps_display);
        fps_display->SetAttribute("id", "fps");
        
        // 动画盒子
        auto box = document->CreateElement("div");
        box->SetTextContent("Animated Box");
        container->AppendChild(box);
        box->SetAttribute("id", "box");
        
        // 控制按钮
        auto button_container = document->CreateElement("div");
        container->AppendChild(button_container);
        
        auto start_btn = document->CreateElement("button");
        start_btn->SetTextContent("Start Animation");
        button_container->AppendChild(start_btn);
        start_btn->SetAttribute("id", "start");
        
        auto stop_btn = document->CreateElement("button");
        stop_btn->SetTextContent("Stop Animation");
        button_container->AppendChild(stop_btn);
        stop_btn->SetAttribute("id", "stop");
        
        std::cout << "✓ DOM structure created" << std::endl;
        
        // 7. 将文档关联到窗口
        window->SetDocument(document);
        
        // 8. 初始化 JavaScript 动画逻辑
        try {
            runtime.Eval(R"(
                // 动画状态
                globalThis.animationRunning = false;
                globalThis.animationId = null;
                globalThis.position = 0;
                globalThis.velocity = 2;
                globalThis.frameCount = 0;
                globalThis.lastTime = 0;
                globalThis.fps = 0;

                // 动画循环
                function animate(timestamp) {
                    if (!globalThis.animationRunning) {
                        return;
                    }

                    // 计算 FPS
                    if (globalThis.lastTime > 0) {
                        const delta = timestamp - globalThis.lastTime;
                        if (delta > 0) {
                            globalThis.fps = Math.round(1000 / delta);
                        }
                    }
                    globalThis.lastTime = timestamp;
                    globalThis.frameCount++;

                    // 更新 FPS 显示（每 10 帧更新一次）
                    if (globalThis.frameCount % 10 === 0) {
                        const fpsDisplay = document.getElementById('fps');
                        if (fpsDisplay) {
                            fpsDisplay.textContent = 'FPS: ' + globalThis.fps + ' | Frames: ' + globalThis.frameCount;
                        }
                    }

                    // 更新位置
                    globalThis.position += globalThis.velocity;

                    // 边界检测（假设窗口宽度 600，盒子宽度 100）
                    if (globalThis.position > 500) {
                        globalThis.position = 500;
                        globalThis.velocity = -globalThis.velocity;
                    } else if (globalThis.position < 0) {
                        globalThis.position = 0;
                        globalThis.velocity = -globalThis.velocity;
                    }

                    // 更新盒子位置（通过修改文本内容模拟）
                    const box = document.getElementById('box');
                    if (box) {
                        box.textContent = 'Position: ' + Math.round(globalThis.position);
                    }

                    // 请求下一帧
                    globalThis.animationId = requestAnimationFrame(animate);
                }

                // 开始动画
                function startAnimation() {
                    if (!globalThis.animationRunning) {
                        globalThis.animationRunning = true;
                        globalThis.frameCount = 0;
                        globalThis.lastTime = 0;
                        console.log('Animation started');
                        globalThis.animationId = requestAnimationFrame(animate);
                    }
                }

                // 停止动画
                function stopAnimation() {
                    if (globalThis.animationRunning) {
                        globalThis.animationRunning = false;
                        console.log('Animation stopped');
                    }
                }

                console.log('Animation demo initialized');

                // 自动开始动画
                startAnimation();

                // 注意：按钮点击事件需要完整的事件系统支持
                // 当前版本自动启动动画，按钮暂时不可用
                // TODO: 实现完整的 DOM 事件系统后，添加按钮点击事件
            )", "animation_demo.js");

            std::cout << "✓ JavaScript initialized" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ JavaScript error: " << e.what() << std::endl;
        }
        
        // 9. 显示窗口
        window->Show();
        std::cout << "✓ Window shown" << std::endl;

        // 10. 设置事件循环回调
        // 设置更新回调（处理 QuickJS microtasks）
        event_loop.SetUpdateCallback([&runtime](float delta_time) {
            runtime.ProcessMicrotasks();
        });

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();  // 显示到屏幕
            }
        });
        
        std::cout << "✓ Event loop configured" << std::endl;
        std::cout << "\n🚀 Animation Demo running..." << std::endl;
        std::cout << "   - Animation starts automatically" << std::endl;
        std::cout << "   - Watch the FPS counter" << std::endl;
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

