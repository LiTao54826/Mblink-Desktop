/**
 * @file main.cpp
 * @brief Table表格测试应用
 *
 * 测试功能:
 * 1. 基本表格渲染（table, tr, td, th）
 * 2. 表格头和表格体（thead, tbody）
 * 3. 表格边框和背景色
 * 4. 表格单元格padding
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/event/task_scheduler.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

using namespace lightui;

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  Table表格测试" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // 1. 创建窗口
        std::cout << "[1/8] Creating window..." << std::endl;
        WindowConfig config;
        config.title = "Table表格测试";
        config.width = 800;
        config.height = 800;
        auto window = std::make_shared<Window>(config);
        std::cout << "  Window created: " << config.width << "x" << config.height << std::endl;

        // 注册窗口
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  Window registered" << std::endl;

        // 2. 创建文档
        std::cout << "[2/8] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  Document initialized" << std::endl;

        // 3. 创建QuickJS运行时
        std::cout << "[3/8] Creating QuickJS runtime..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();
        std::cout << "  QuickJS runtime created" << std::endl;

        // 4. 创建 TaskScheduler
        std::cout << "[4/8] Creating task scheduler..." << std::endl;
        auto scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  Task scheduler created" << std::endl;

        // 5. 初始化DOM绑定
        std::cout << "[5/8] Initializing DOM bindings..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        DOMBindings::SetGlobalTaskScheduler(ctx, scheduler);
        std::cout << "  DOM bindings initialized" << std::endl;

        // 6. 创建body元素
        std::cout << "[6/8] Creating body element..." << std::endl;
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "overflow: auto;");
        document->SetBody(body);
        std::cout << "  Body element created" << std::endl;

        // 7. 加载Preact库 (相对于 build/bin/Release/ 或 build/bin/Debug/)
        std::cout << "[7/8] Loading Preact library..." << std::endl;
        std::string preact_code = ReadFile("../../../js/preact/preact.js");
        if (preact_code.empty()) {
            preact_code = ReadFile("js/preact/preact.js");
        }
        if (preact_code.empty()) {
            std::cerr << "Failed to load preact.js" << std::endl;
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");
        std::cout << "  Preact library loaded" << std::endl;

        // 加载Hooks库
        std::string hooks_code = ReadFile("../../../js/preact/hooks.js");
        if (hooks_code.empty()) {
            hooks_code = ReadFile("js/preact/hooks.js");
        }
        if (hooks_code.empty()) {
            std::cerr << "Failed to load hooks.js" << std::endl;
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");
        std::cout << "  Hooks library loaded" << std::endl;

        // 8. 加载并运行表格测试应用
        std::cout << "[8/8] Loading table test application..." << std::endl;
        std::string app_code = ReadFile("../../../examples/demo_html/table_test/app.js");
        if (app_code.empty()) {
            app_code = ReadFile("examples/demo_html/table_test/app.js");
        }
        if (app_code.empty()) {
            std::cerr << "Failed to load table_test app.js" << std::endl;
            return 1;
        }
        runtime->Eval(app_code, "table_test_app.js");
        std::cout << "  Table test application loaded and rendered" << std::endl;

        // 将文档关联到窗口并显示
        window->SetDocument(document);
        window->Show();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Table测试应用已启动!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  测试内容:" << std::endl;
        std::cout << "  - 基本表格渲染（table, tr, td, th）" << std::endl;
        std::cout << "  - 表格头和表格体（thead, tbody）" << std::endl;
        std::cout << "  - 表格边框和背景色" << std::endl;
        std::cout << "  - 表格单元格padding" << std::endl;
        std::cout << std::endl;
        std::cout << "  关闭窗口退出" << std::endl;
        std::cout << std::endl;

        // 先进行一次渲染
        window->Render();
        window->SwapBuffers();

        // 保存截图
        auto surface = window->GetSurface();
        if (surface) {
            sk_sp<SkImage> image = surface->makeImageSnapshot();
            if (image) {
                SkPixmap pixmap;
                if (image->peekPixels(&pixmap)) {
                    SkFILEWStream stream("table_test_screenshot.png");
                    if (SkPngEncoder::Encode(&stream, pixmap, SkPngEncoder::Options())) {
                        std::cout << "  Screenshot saved: table_test_screenshot.png" << std::endl;
                    } else {
                        std::cerr << "  Failed to encode PNG" << std::endl;
                    }
                } else {
                    std::cerr << "  Failed to peek pixels" << std::endl;
                }
            } else {
                std::cerr << "  Failed to create image snapshot" << std::endl;
            }
        } else {
            std::cerr << "  Surface is null, cannot save screenshot" << std::endl;
        }

        // 创建事件循环
        EventLoop event_loop;

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });

        // 运行事件循环
        event_loop.Run();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  应用已关闭" << std::endl;
        std::cout << "========================================" << std::endl;

        // 清理
        DOMBindings::Cleanup(ctx);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

