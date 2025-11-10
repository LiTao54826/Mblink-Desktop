/**
 * @file test_window.cpp
 * @brief Window类单元测试
 *
 * 测试内容：
 * - 窗口创建和销毁
 * - 窗口属性设置
 * - 窗口显示/隐藏
 * - 窗口位置和大小
 * - 窗口状态（最小化、最大化、全屏）
 * - 窗口管理器
 * - 多窗口支持
 */

#include <gtest/gtest.h>
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include <thread>
#include <chrono>

using namespace lightui;

// 测试夹具
class WindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清空 SDL 事件队列，避免事件积累导致 CPU 占用高
        ClearSDLEvents();
    }

    void TearDown() override {
        // 清空 SDL 事件队列
        ClearSDLEvents();

        // 给 SDL 一点时间清理资源
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

private:
    // 清空 SDL 事件队列
    void ClearSDLEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 丢弃所有待处理事件
        }
    }
};

// 测试窗口创建
TEST_F(WindowTest, CreateWindow) {
    WindowConfig config;
    config.title = "Test Window";
    config.width = 800;
    config.height = 600;
    config.hidden = true;  // 隐藏窗口以避免干扰

    Window window(config);

    int width, height;
    window.GetSize(&width, &height);
    EXPECT_EQ(width, 800);
    EXPECT_EQ(height, 600);

    EXPECT_NE(window.GetSDLWindow(), nullptr);
    // GrContext 在 CPU 模式下可能为 nullptr，所以不检查
    // EXPECT_NE(window.GetGrContext(), nullptr);
    EXPECT_NE(window.GetCanvas(), nullptr);  // Canvas 应该总是存在
}

// 测试窗口标题设置
TEST_F(WindowTest, SetTitle) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    window.SetTitle("New Title");
    // SDL3 doesn't provide a way to get the title back, so we just verify no crash
    SUCCEED();
}

// 测试窗口大小设置
TEST_F(WindowTest, SetSize) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    window.SetSize(1024, 768);

    int width, height;
    window.GetSize(&width, &height);
    EXPECT_EQ(width, 1024);
    EXPECT_EQ(height, 768);
}

// 测试窗口位置设置
TEST_F(WindowTest, SetPosition) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    window.SetPosition(100, 200);

    int x, y;
    window.GetPosition(&x, &y);
    // 位置可能会被窗口管理器调整，所以只检查是否接近
    EXPECT_NEAR(x, 100, 50);
    EXPECT_NEAR(y, 200, 50);
}

// 测试窗口显示/隐藏
TEST_F(WindowTest, ShowHide) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    // 测试显示和隐藏不会崩溃
    window.Show();
    window.Hide();
    SUCCEED();
}

// 测试窗口最小化/最大化/恢复
TEST_F(WindowTest, MinimizeMaximizeRestore) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    // 测试这些操作不会崩溃
    window.Minimize();
    window.Restore();
    window.Maximize();
    window.Restore();
    SUCCEED();
}

// 测试全屏模式
TEST_F(WindowTest, Fullscreen) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    window.SetFullscreen(true);
    window.SetFullscreen(false);
    SUCCEED();
}

// 测试窗口属性
TEST_F(WindowTest, WindowProperties) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    window.SetResizable(false);
    window.SetResizable(true);

    window.SetBorderless(true);
    window.SetBorderless(false);

    window.SetAlwaysOnTop(true);
    window.SetAlwaysOnTop(false);
    SUCCEED();
}

// 测试事件监听器
TEST_F(WindowTest, EventListeners) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    bool resize_called = false;
    bool focus_called = false;

    // 添加事件监听器
    window.AddEventListener(WindowEventType::RESIZE, [&](const WindowEvent& event) {
        resize_called = true;
        EXPECT_EQ(event.GetType(), WindowEventType::RESIZE);
    });

    window.AddEventListener(WindowEventType::FOCUS, [&](const WindowEvent& event) {
        focus_called = true;
        EXPECT_EQ(event.GetType(), WindowEventType::FOCUS);
    });

    // 手动分发事件进行测试
    window.DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, 1024, 768));
    EXPECT_TRUE(resize_called);

    window.DispatchWindowEvent(WindowEvent(WindowEventType::FOCUS));
    EXPECT_TRUE(focus_called);
}

// 测试回调函数
TEST_F(WindowTest, Callbacks) {
    WindowConfig config;
    config.hidden = true;

    Window window(config);

    bool resize_callback_called = false;
    bool close_callback_called = false;

    window.SetOnResizeCallback([&](int width, int height) {
        resize_callback_called = true;
        EXPECT_GT(width, 0);
        EXPECT_GT(height, 0);
    });

    window.SetOnCloseCallback([&]() {
        close_callback_called = true;
    });

    // 触发resize
    window.SetSize(1024, 768);
    EXPECT_TRUE(resize_callback_called);

    // 手动触发close事件
    SDL_Event event;
    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    event.window.windowID = SDL_GetWindowID(window.GetSDLWindow());
    window.HandleSDLEvent(event);
    EXPECT_TRUE(close_callback_called);
    EXPECT_TRUE(window.ShouldClose());
}

// ========== WindowManager 测试 ==========

// 测试 WindowManager 单例
TEST_F(WindowTest, WindowManagerSingleton) {
    auto& manager1 = WindowManager::Instance();
    auto& manager2 = WindowManager::Instance();

    EXPECT_EQ(&manager1, &manager2);
}

// 测试窗口注册和注销
TEST_F(WindowTest, WindowRegistration) {
    std::cout << "[TEST] WindowRegistration started" << std::endl;

    auto& manager = WindowManager::Instance();
    std::cout << "[TEST] Got WindowManager instance" << std::endl;

    WindowConfig config;
    config.hidden = true;
    std::cout << "[TEST] Config created" << std::endl;

    std::cout << "[TEST] Creating window1..." << std::endl;
    std::cout.flush();
    auto window1 = std::make_shared<Window>(config);
    std::cout << "[TEST] Window1 created" << std::endl;

    std::cout << "[TEST] Creating window2..." << std::endl;
    std::cout.flush();
    auto window2 = std::make_shared<Window>(config);
    std::cout << "[TEST] Window2 created" << std::endl;

    // 注册窗口
    std::cout << "[TEST] Registering window1..." << std::endl;
    manager.RegisterWindow(window1);
    std::cout << "[TEST] Window1 registered" << std::endl;

    std::cout << "[TEST] Registering window2..." << std::endl;
    manager.RegisterWindow(window2);
    std::cout << "[TEST] Window2 registered" << std::endl;

    EXPECT_EQ(manager.GetWindowCount(), 2);
    EXPECT_TRUE(manager.HasWindows());

    // 注销窗口
    std::cout << "[TEST] Unregistering window1..." << std::endl;
    manager.UnregisterWindow(window1);
    std::cout << "[TEST] Window1 unregistered" << std::endl;
    EXPECT_EQ(manager.GetWindowCount(), 1);

    std::cout << "[TEST] Unregistering window2..." << std::endl;
    manager.UnregisterWindow(window2);
    std::cout << "[TEST] Window2 unregistered" << std::endl;
    EXPECT_EQ(manager.GetWindowCount(), 0);
    EXPECT_FALSE(manager.HasWindows());

    std::cout << "[TEST] WindowRegistration completed" << std::endl;
}

// 测试窗口查找
TEST_F(WindowTest, WindowLookup) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;

    auto window = std::make_shared<Window>(config);
    manager.RegisterWindow(window);

    // 通过 SDL 窗口 ID 查找
    Uint32 window_id = SDL_GetWindowID(window->GetSDLWindow());
    auto found = manager.FindWindowByID(window_id);
    EXPECT_EQ(found, window);

    // 通过 SDL_Window 指针查找
    auto found2 = manager.FindWindowBySDLWindow(window->GetSDLWindow());
    EXPECT_EQ(found2, window);

    // 清理
    manager.UnregisterWindow(window);
}

// 测试获取所有窗口
TEST_F(WindowTest, GetAllWindows) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;

    auto window1 = std::make_shared<Window>(config);
    auto window2 = std::make_shared<Window>(config);
    auto window3 = std::make_shared<Window>(config);

    manager.RegisterWindow(window1);
    manager.RegisterWindow(window2);
    manager.RegisterWindow(window3);

    auto windows = manager.GetAllWindows();
    EXPECT_EQ(windows.size(), 3);

    // 清理
    manager.UnregisterWindow(window1);
    manager.UnregisterWindow(window2);
    manager.UnregisterWindow(window3);
}

// 测试多窗口事件处理
TEST_F(WindowTest, MultiWindowEventHandling) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;

    auto window1 = std::make_shared<Window>(config);
    auto window2 = std::make_shared<Window>(config);

    manager.RegisterWindow(window1);
    manager.RegisterWindow(window2);

    bool window1_resized = false;
    bool window2_resized = false;

    window1->AddEventListener(WindowEventType::RESIZE, [&](const WindowEvent& e) {
        window1_resized = true;
    });

    window2->AddEventListener(WindowEventType::RESIZE, [&](const WindowEvent& e) {
        window2_resized = true;
    });

    // 模拟窗口1的resize事件
    SDL_Event event;
    event.type = SDL_EVENT_WINDOW_RESIZED;
    event.window.windowID = SDL_GetWindowID(window1->GetSDLWindow());
    event.window.data1 = 1024;
    event.window.data2 = 768;

    manager.HandleEvent(event);

    EXPECT_TRUE(window1_resized);
    EXPECT_FALSE(window2_resized);  // window2 不应该收到事件

    // 清理
    manager.UnregisterWindow(window1);
    manager.UnregisterWindow(window2);
}

// 测试关闭所有窗口
TEST_F(WindowTest, CloseAllWindows) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;

    auto window1 = std::make_shared<Window>(config);
    auto window2 = std::make_shared<Window>(config);

    manager.RegisterWindow(window1);
    manager.RegisterWindow(window2);

    EXPECT_FALSE(window1->ShouldClose());
    EXPECT_FALSE(window2->ShouldClose());

    manager.CloseAllWindows();

    EXPECT_TRUE(window1->ShouldClose());
    EXPECT_TRUE(window2->ShouldClose());

    // 清理
    manager.UnregisterWindow(window1);
    manager.UnregisterWindow(window2);
}

// 测试窗口自动清理
TEST_F(WindowTest, AutoCleanup) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;

    {
        auto window = std::make_shared<Window>(config);
        manager.RegisterWindow(window);
        EXPECT_EQ(manager.GetWindowCount(), 1);
    }  // window 超出作用域被销毁

    // 注册新窗口会触发清理
    auto new_window = std::make_shared<Window>(config);
    manager.RegisterWindow(new_window);

    // 应该只有新窗口
    EXPECT_EQ(manager.GetWindowCount(), 1);

    // 清理
    manager.UnregisterWindow(new_window);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

