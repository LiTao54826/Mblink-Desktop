/**
 * @file test_window_timeout.cpp
 * @brief 带超时的窗口测试，用于诊断真机卡住问题
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <iostream>
#include "core/window/window.h"
#include "core/window/window_manager.h"

using namespace lightui;

// 测试夹具
class WindowTimeoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "\n[SETUP] Clearing SDL events..." << std::endl;
        ClearSDLEvents();
    }

    void TearDown() override {
        std::cout << "[TEARDOWN] Clearing SDL events..." << std::endl;
        ClearSDLEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
private:
    void ClearSDLEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 丢弃所有待处理事件
        }
    }
};

// 带超时的窗口创建测试
TEST_F(WindowTimeoutTest, CreateWindowWithTimeout) {
    std::cout << "\n========== CreateWindowWithTimeout START ==========" << std::endl;
    
    WindowConfig config;
    config.hidden = true;
    config.title = "Timeout Test Window";
    
    // 使用 future 实现超时
    auto future = std::async(std::launch::async, [&config]() {
        std::cout << "[ASYNC] Creating window..." << std::endl;
        auto window = std::make_shared<Window>(config);
        std::cout << "[ASYNC] Window created successfully!" << std::endl;
        return window;
    });
    
    // 等待最多 10 秒
    std::cout << "[TEST] Waiting for window creation (max 10 seconds)..." << std::endl;
    auto status = future.wait_for(std::chrono::seconds(10));
    
    if (status == std::future_status::ready) {
        std::cout << "[TEST] ✅ Window created successfully!" << std::endl;
        auto window = future.get();
        EXPECT_NE(window, nullptr);
    } else if (status == std::future_status::timeout) {
        std::cout << "[TEST] ❌ TIMEOUT! Window creation took more than 10 seconds!" << std::endl;
        std::cout << "[TEST] This indicates a hang in Window constructor" << std::endl;
        FAIL() << "Window creation timeout";
    } else {
        std::cout << "[TEST] ❌ DEFERRED! Unexpected status" << std::endl;
        FAIL() << "Unexpected future status";
    }
    
    std::cout << "========== CreateWindowWithTimeout END ==========" << std::endl;
}

// 带超时的多窗口测试
TEST_F(WindowTimeoutTest, MultiWindowWithTimeout) {
    std::cout << "\n========== MultiWindowWithTimeout START ==========" << std::endl;
    
    WindowConfig config;
    config.hidden = true;
    
    // 创建第一个窗口
    std::cout << "[TEST] Creating window 1 with timeout..." << std::endl;
    auto future1 = std::async(std::launch::async, [&config]() {
        std::cout << "[ASYNC1] Creating window 1..." << std::endl;
        auto window = std::make_shared<Window>(config);
        std::cout << "[ASYNC1] Window 1 created!" << std::endl;
        return window;
    });
    
    auto status1 = future1.wait_for(std::chrono::seconds(10));
    if (status1 != std::future_status::ready) {
        FAIL() << "Window 1 creation timeout";
    }
    auto window1 = future1.get();
    std::cout << "[TEST] ✅ Window 1 created successfully" << std::endl;
    
    // 创建第二个窗口
    std::cout << "[TEST] Creating window 2 with timeout..." << std::endl;
    auto future2 = std::async(std::launch::async, [&config]() {
        std::cout << "[ASYNC2] Creating window 2..." << std::endl;
        auto window = std::make_shared<Window>(config);
        std::cout << "[ASYNC2] Window 2 created!" << std::endl;
        return window;
    });
    
    auto status2 = future2.wait_for(std::chrono::seconds(10));
    if (status2 != std::future_status::ready) {
        std::cout << "[TEST] ❌ Window 2 creation TIMEOUT!" << std::endl;
        std::cout << "[TEST] This suggests a multi-window OpenGL context issue" << std::endl;
        FAIL() << "Window 2 creation timeout (multi-window issue)";
    }
    auto window2 = future2.get();
    std::cout << "[TEST] ✅ Window 2 created successfully" << std::endl;
    
    EXPECT_NE(window1, nullptr);
    EXPECT_NE(window2, nullptr);
    
    std::cout << "========== MultiWindowWithTimeout END ==========" << std::endl;
}

// 测试 CPU 渲染（应该不会卡）
TEST_F(WindowTimeoutTest, CPURenderingNoHang) {
    std::cout << "\n========== CPURenderingNoHang START ==========" << std::endl;
    
    WindowConfig config;
    config.hidden = true;
    config.backend = RenderBackend::CPU;  // 强制 CPU 渲染
    
    std::cout << "[TEST] Creating window with CPU rendering..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    
    auto window = std::make_shared<Window>(config);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "[TEST] ✅ CPU rendering window created in " << duration << "ms" << std::endl;
    EXPECT_NE(window, nullptr);
    EXPECT_LT(duration, 1000);  // 应该在 1 秒内完成
    
    std::cout << "========== CPURenderingNoHang END ==========" << std::endl;
}

// 测试 GPU 渲染（可能会卡）
TEST_F(WindowTimeoutTest, GPURenderingMayHang) {
    std::cout << "\n========== GPURenderingMayHang START ==========" << std::endl;
    
    WindowConfig config;
    config.hidden = true;
    config.backend = RenderBackend::OPENGL;  // 强制 GPU 渲染
    
    std::cout << "[TEST] Creating window with GPU rendering..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    
    try {
        auto window = std::make_shared<Window>(config);
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "[TEST] ✅ GPU rendering window created in " << duration << "ms" << std::endl;
        EXPECT_NE(window, nullptr);
    } catch (const std::exception& e) {
        std::cout << "[TEST] ⚠️  GPU rendering failed: " << e.what() << std::endl;
        std::cout << "[TEST] This is OK, will fall back to CPU rendering" << std::endl;
    }
    
    std::cout << "========== GPURenderingMayHang END ==========" << std::endl;
}

