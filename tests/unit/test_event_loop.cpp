/**
 * @file test_event_loop.cpp
 * @brief EventLoop 单元测试
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "core/event/event_loop.h"
#include "core/event/frame_controller.h"
#include "core/event/input_handler.h"
#include "core/event/task_scheduler.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"

using namespace lightui;

// 测试夹具
class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清空 SDL 事件队列
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 丢弃所有待处理事件
        }
    }

    void TearDown() override {
        // 清空 SDL 事件队列
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 丢弃所有待处理事件
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

// 测试构造和析构
TEST_F(EventLoopTest, ConstructorDestructor) {
    EventLoop loop;
    
    EXPECT_FALSE(loop.IsRunning());
    EXPECT_FALSE(loop.ShouldQuit());
}

// 测试 RunOnce
TEST_F(EventLoopTest, RunOnce) {
    EventLoop loop;
    
    bool update_called = false;
    loop.SetUpdateCallback([&](float dt) {
        update_called = true;
    });
    
    loop.RunOnce();
    
    EXPECT_TRUE(update_called);
}

// 测试 Stop
TEST_F(EventLoopTest, Stop) {
    EventLoop loop;
    
    EXPECT_FALSE(loop.ShouldQuit());
    
    loop.Stop();
    
    EXPECT_TRUE(loop.ShouldQuit());
}

// 测试更新回调
TEST_F(EventLoopTest, UpdateCallback) {
    EventLoop loop;
    
    int update_count = 0;
    float total_delta_time = 0.0f;
    
    loop.SetUpdateCallback([&](float dt) {
        update_count++;
        total_delta_time += dt;
    });
    
    // 运行几帧
    for (int i = 0; i < 5; i++) {
        loop.RunOnce();
    }
    
    EXPECT_EQ(update_count, 5);
    EXPECT_GT(total_delta_time, 0.0f);
}

// 测试渲染回调
TEST_F(EventLoopTest, RenderCallback) {
    EventLoop loop;
    
    int render_count = 0;
    
    loop.SetRenderCallback([&]() {
        render_count++;
    });
    
    // 运行几帧
    for (int i = 0; i < 5; i++) {
        loop.RunOnce();
    }
    
    EXPECT_EQ(render_count, 5);
}

// 测试空闲回调
TEST_F(EventLoopTest, IdleCallback) {
    EventLoop loop;
    
    bool idle_called = false;
    
    loop.SetIdleCallback([&]() {
        idle_called = true;
    });
    
    // 运行一帧（没有工作，应该调用空闲回调）
    loop.RunOnce();
    
    // 注意：空闲回调只在没有工作时调用
    // 由于我们没有窗口和任务，应该会调用
    EXPECT_TRUE(idle_called);
}

// 测试 FrameController 访问
TEST_F(EventLoopTest, FrameControllerAccess) {
    EventLoop loop;
    
    auto& frame_controller = loop.GetFrameController();
    
    EXPECT_EQ(frame_controller.GetTargetFPS(), 60);
    
    frame_controller.SetTargetFPS(30);
    EXPECT_EQ(frame_controller.GetTargetFPS(), 30);
}

// 测试 InputHandler 访问
TEST_F(EventLoopTest, InputHandlerAccess) {
    EventLoop loop;
    
    auto& input_handler = loop.GetInputHandler();
    
    bool callback_called = false;
    input_handler.SetMouseCallback([&](const InputMouseEvent& e) {
        callback_called = true;
    });
    
    // 创建鼠标事件
    SDL_Event event;
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 100.0f;
    event.motion.y = 200.0f;
    event.motion.state = 0;
    event.window.windowID = 1;
    
    input_handler.HandleSDLEvent(event);
    
    EXPECT_TRUE(callback_called);
}

// 测试 TaskScheduler 访问
TEST_F(EventLoopTest, TaskSchedulerAccess) {
    EventLoop loop;
    
    auto& task_scheduler = loop.GetTaskScheduler();
    
    bool task_executed = false;
    task_scheduler.SetTimeout([&]() {
        task_executed = true;
    }, 0);
    
    loop.RunOnce();
    
    EXPECT_TRUE(task_executed);
}

// 测试集成：更新 + 渲染 + 任务
TEST_F(EventLoopTest, IntegratedCallbacks) {
    EventLoop loop;
    
    int update_count = 0;
    int render_count = 0;
    int task_count = 0;
    
    loop.SetUpdateCallback([&](float dt) {
        update_count++;
    });
    
    loop.SetRenderCallback([&]() {
        render_count++;
    });
    
    loop.GetTaskScheduler().SetTimeout([&]() {
        task_count++;
    }, 0);
    
    loop.RunOnce();
    
    EXPECT_EQ(update_count, 1);
    EXPECT_EQ(render_count, 1);
    EXPECT_EQ(task_count, 1);
}

// 测试帧率控制
TEST_F(EventLoopTest, FrameRateControl) {
    EventLoop loop;
    
    // 设置 30 FPS
    loop.GetFrameController().SetTargetFPS(30);
    loop.GetFrameController().SetFrameRateLimitEnabled(true);
    
    auto start = std::chrono::steady_clock::now();
    
    // 运行 5 帧
    for (int i = 0; i < 5; i++) {
        loop.RunOnce();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 5 帧 @ 30 FPS = 166.67ms
    EXPECT_GT(duration, 150);
    EXPECT_LT(duration, 200);
}

// 测试任务调度集成
TEST_F(EventLoopTest, TaskSchedulingIntegration) {
    EventLoop loop;
    
    std::vector<int> execution_order;
    
    loop.GetTaskScheduler().SetTimeout([&]() {
        execution_order.push_back(1);
    }, 10);
    
    loop.GetTaskScheduler().SetTimeout([&]() {
        execution_order.push_back(2);
    }, 20);
    
    // 运行足够长的时间
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < 30) {
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    EXPECT_EQ(execution_order.size(), 2);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
}

// 测试 requestAnimationFrame 集成
TEST_F(EventLoopTest, RequestAnimationFrameIntegration) {
    EventLoop loop;

    int frame_count = 0;

    // 每帧都添加一个新的动画帧任务
    for (int i = 0; i < 5; i++) {
        loop.GetTaskScheduler().RequestAnimationFrame([&](float dt) {
            frame_count++;
        });
    }

    // 运行一帧，所有任务都应该执行
    loop.RunOnce();

    EXPECT_EQ(frame_count, 5);
}

// 测试多个回调的执行顺序
TEST_F(EventLoopTest, CallbackExecutionOrder) {
    EventLoop loop;
    
    std::vector<std::string> execution_order;
    
    loop.SetUpdateCallback([&](float dt) {
        execution_order.push_back("update");
    });
    
    loop.SetRenderCallback([&]() {
        execution_order.push_back("render");
    });
    
    loop.GetTaskScheduler().SetTimeout([&]() {
        execution_order.push_back("task");
    }, 0);
    
    loop.RunOnce();
    
    // 执行顺序应该是：task -> update -> render
    ASSERT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], "task");
    EXPECT_EQ(execution_order[1], "update");
    EXPECT_EQ(execution_order[2], "render");
}

