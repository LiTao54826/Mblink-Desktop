/**
 * @file main.cpp
 * @brief MBink Window Demo - 完整的窗口应用示例
 * 
 * 功能：
 * - 展示各种 HTML 元素的渲染
 * - 演示事件处理和交互
 * - 展示 CSS 样式支持
 * - 演示 JavaScript 集成
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/event/task_scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

using namespace lightui;

// 读取文件内容
std::string ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open file: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    JSContext* ctx = nullptr;
    
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink Window Demo" << std::endl;
        std::cout << "  Version: 1.0" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        
        // 1. 创建窗口
        std::cout << "[1/7] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "MBink Window Demo - Interactive UI";
        config.width = 800;
        config.height = 600;
        config.resizable = true;
        config.vsync = true;
        
        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created: " << config.width << "x" << config.height << std::endl;
        
        // 注册窗口到 WindowManager
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window registered" << std::endl;
        
        // 2. 创建 Document
        std::cout << "[2/7] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;
        
        // 3. 创建 TaskScheduler
        std::cout << "[3/7] Creating task scheduler..." << std::endl;
        auto scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  ✓ Task scheduler created" << std::endl;
        
        // 4. 创建 QuickJS 运行时
        std::cout << "[4/7] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        ctx = runtime->GetContext();
        std::cout << "  ✓ QuickJS runtime created at address: " << runtime.get() << std::endl;
        
        // 5. 初始化 DOM 绑定
        std::cout << "[5/7] Initializing DOM bindings..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        DOMBindings::SetGlobalTaskScheduler(ctx, scheduler);
        std::cout << "  ✓ DOM bindings initialized" << std::endl;
        
        // 6. 加载 HTML 内容
        std::cout << "[6/7] Loading HTML content..." << std::endl;

        // 从文件加载 HTML
        std::string html = ReadFile("examples/window_demo/index.html");

        if (html.empty()) {
            std::cerr << "  ✗ Failed to load index.html!" << std::endl;
            std::cerr << "  Please make sure examples/window_demo/index.html exists" << std::endl;
            return 1;
        }

        std::cout << "  ✓ HTML loaded from file (" << html.length() << " bytes)" << std::endl;

        /*
        // 备用：内置 HTML（因为 MSVC 编码问题，暂时禁用）
        if (html.empty()) {
            std::cout << "  i Using built-in HTML content" << std::endl;
            html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>MBink Window Demo</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
        }
        .container {
            max-width: 760px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        h1 {
            color: #667eea;
            margin-top: 0;
            text-align: center;
        }
        .section {
            margin: 20px 0;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 5px;
        }
        .section h2 {
            margin-top: 0;
            color: #495057;
            font-size: 18px;
        }
        button {
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
        }
        .btn-success {
            background: #51cf66;
            color: white;
        }
        .btn-danger {
            background: #ff6b6b;
            color: white;
        }
        input[type="text"], input[type="password"], textarea, select {
            padding: 8px;
            margin: 5px;
            border: 1px solid #ced4da;
            border-radius: 4px;
            font-size: 14px;
        }
        input[type="text"]:focus, textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        .counter {
            font-size: 48px;
            font-weight: bold;
            color: #667eea;
            text-align: center;
            margin: 20px 0;
        }
        .output {
            padding: 10px;
            background: #e9ecef;
            border-radius: 4px;
            margin: 10px 0;
            font-family: monospace;
            min-height: 20px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 10px 0;
        }
        th, td {
            padding: 8px;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        th {
            background: #667eea;
            color: white;
        }
        .footer {
            text-align: center;
            margin-top: 20px;
            padding-top: 20px;
            border-top: 1px solid #dee2e6;
            color: #6c757d;
            font-size: 12px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 MBink Window Demo</h1>
        
        <!-- 计数器示例 -->
        <div class="section">
            <h2>📊 Interactive Counter</h2>
            <div class="counter" id="counter">0</div>
            <div style="text-align: center;">
                <button class="btn-primary" onclick="incrementCounter()">➕ Increment</button>
                <button class="btn-danger" onclick="decrementCounter()">➖ Decrement</button>
                <button class="btn-success" onclick="resetCounter()">🔄 Reset</button>
            </div>
        </div>
        
        <!-- 表单示例 -->
        <div class="section">
            <h2>📝 Form Elements</h2>
            <div>
                <input type="text" id="nameInput" placeholder="Enter your name" style="width: 200px;">
                <button class="btn-primary" onclick="greet()">👋 Greet</button>
            </div>
            <div class="output" id="greeting"></div>
        </div>
        
        <!-- 列表示例 -->
        <div class="section">
            <h2>📋 Todo List</h2>
            <div>
                <input type="text" id="todoInput" placeholder="New todo item" style="width: 300px;">
                <button class="btn-success" onclick="addTodo()">➕ Add</button>
            </div>
            <ul id="todoList" style="list-style: none; padding: 0;"></ul>
        </div>
        
        <!-- 定时器示例 -->
        <div class="section">
            <h2>⏱️ Timer</h2>
            <div class="output" id="timer">Timer: 0s</div>
            <button class="btn-primary" id="timerBtn" onclick="toggleTimer()">▶️ Start</button>
        </div>
        
        <!-- 元素展示 -->
        <div class="section">
            <h2>🎨 UI Elements</h2>
            <table>
                <tr>
                    <th>Element</th>
                    <th>Example</th>
                </tr>
                <tr>
                    <td>Checkbox</td>
                    <td><input type="checkbox" checked> Option 1 <input type="checkbox"> Option 2</td>
                </tr>
                <tr>
                    <td>Radio</td>
                    <td><input type="radio" name="r1" checked> Choice A <input type="radio" name="r1"> Choice B</td>
                </tr>
                <tr>
                    <td>Select</td>
                    <td>
                        <select>
                            <option>Option 1</option>
                            <option selected>Option 2</option>
                            <option>Option 3</option>
                        </select>
                    </td>
                </tr>
            </table>
        </div>
        
        <div class="footer">
            MBink Framework v1.0 | Powered by QuickJS, Skia & SDL3
        </div>
    </div>
    
    <script src="examples/window_demo/app.js"></script>
</body>
</html>
            )";
        }
        */
        
        if (!document->LoadHTML(html)) {
            std::cerr << "  ✗ Failed to load HTML!" << std::endl;
            return 1;
        }
        std::cout << "  ✓ HTML loaded successfully" << std::endl;
        
        // 7. 加载并执行 JavaScript
        std::cout << "[7/7] Loading JavaScript..." << std::endl;
        std::string js = ReadFile("examples/window_demo/app.js");
        
        if (!js.empty()) {
            runtime->Eval(js, "app.js");
            std::cout << "  ✓ JavaScript loaded and executed" << std::endl;
        } else {
            std::cout << "  ℹ No external JavaScript file found, using inline scripts" << std::endl;
        }
        
        // 将文档关联到窗口
        window->SetDocument(document);
        std::cout << "  ✓ Document attached to window" << std::endl;
        
        // 显示窗口
        window->Show();
        std::cout << "  ✓ Window shown" << std::endl;
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Features:" << std::endl;
        std::cout << "  - Interactive counter" << std::endl;
        std::cout << "  - Form input and validation" << std::endl;
        std::cout << "  - Todo list management" << std::endl;
        std::cout << "  - Timer functionality" << std::endl;
        std::cout << "  - Various UI elements" << std::endl;
        std::cout << std::endl;
        std::cout << "  Close window to exit" << std::endl;
        std::cout << std::endl;

        // 创建事件循环（使用同一个 TaskScheduler）
        EventLoop event_loop(scheduler);
        
        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->RenderDocument();
                window->SwapBuffers();
            }
        });
        
        // 运行事件循环
        std::cout << "[main] About to run event loop" << std::endl;
        event_loop.Run();
        std::cout << "[main] Event loop returned normally" << std::endl;

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  👋 Application Closed" << std::endl;
        std::cout << "========================================" << std::endl;

        // 清理 DOM 绑定（在 runtime 销毁之前）
        std::cout << "[main] Cleaning up DOM bindings" << std::endl;
        DOMBindings::Cleanup(ctx);
        std::cout << "[main] DOM bindings cleaned up" << std::endl;

        // 清理 window 对象（在 runtime 销毁之前）
        // 这很重要，因为 window 持有 document，而 document 可能持有 JSValue 引用
        std::cout << "[main] Releasing window object" << std::endl;
        window.reset();
        std::cout << "[main] Window object released" << std::endl;

        std::cout << "[main] About to destroy runtime" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error (std::exception): " << e.what() << std::endl;
        std::cerr << "[main] Exception caught, cleaning up" << std::endl;
        if (ctx) {
            DOMBindings::Cleanup(ctx);
        }
        std::cerr << "[main] About to destroy runtime after exception" << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "❌ Error: Unknown exception" << std::endl;
        std::cerr << "[main] Unknown exception caught, cleaning up" << std::endl;
        if (ctx) {
            DOMBindings::Cleanup(ctx);
        }
        std::cerr << "[main] About to destroy runtime after unknown exception" << std::endl;
        return 1;
    }
}

