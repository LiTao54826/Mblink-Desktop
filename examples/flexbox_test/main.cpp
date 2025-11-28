/**
 * @file main.cpp
 * @brief Flexbox layout test application
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/event/event_loop.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

using namespace lightui;

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

int main(int argc, char* argv[]) {
    // Determine which test to run
    std::string test_file = "examples/window_demo/flexbox_test.html";
    std::string test_name = "Flexbox";
    
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "grid") {
            test_file = "examples/window_demo/grid_test.html";
            test_name = "Grid";
        } else if (arg == "position") {
            test_file = "examples/window_demo/position_test.html";
            test_name = "Position & Overflow";
        } else if (arg == "comprehensive" || arg == "all") {
            test_file = "examples/window_demo/comprehensive_test.html";
            test_name = "Comprehensive Layout";
        }
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  " << test_name << " Layout Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        // Create window
        std::cout << "[1/4] Creating window..." << std::endl;
        WindowConfig config;
        config.title = test_name + " Test";
        config.width = 800;
        config.height = 600;
        config.resizable = true;
        config.vsync = true;
        config.backend = RenderBackend::CPU;  // 明确使用 CPU 渲染模式
        config.borderless = true;  // 测试无边框模式（虚拟机兼容）

        auto window = std::make_shared<Window>(config);
        std::cout << "  ✓ Window created: 800x600" << std::endl;

        // Register window
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window registered" << std::endl;

        // Create document
        std::cout << "[2/4] Creating document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "  ✓ Document initialized" << std::endl;

        // Load HTML
        std::cout << "[3/4] Loading HTML..." << std::endl;
        std::string html = ReadFile(test_file);
        if (html.empty()) {
            std::cerr << "  ✗ Failed to load HTML: " << test_file << std::endl;
            return 1;
        }
        std::cout << "  ✓ HTML loaded (" << html.size() << " bytes)" << std::endl;

        // Set HTML content
        if (!document->LoadHTML(html)) {
            std::cerr << "  ✗ Failed to parse HTML" << std::endl;
            return 1;
        }
        std::cout << "  ✓ HTML parsed successfully" << std::endl;

        // Attach document to window
        std::cout << "[4/4] Attaching document..." << std::endl;
        window->SetDocument(document);
        window->Show();
        std::cout << "  ✓ Document attached and window shown" << std::endl;

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  Testing Flexbox layout:" << std::endl;
        std::cout << "  - Row layout (space-between)" << std::endl;
        std::cout << "  - Column layout" << std::endl;
        std::cout << "  - Wrap layout" << std::endl;
        std::cout << std::endl;
        std::cout << "  Close window to exit" << std::endl;
        std::cout << std::endl;

        // Run event loop
        EventLoop event_loop;
        event_loop.Run();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Application Exited" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

