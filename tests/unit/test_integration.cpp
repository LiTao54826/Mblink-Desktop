/**
 * @file test_integration.cpp
 * @brief 集成测试 - Window + DOM + Renderer + JavaScript
 */

#include <gtest/gtest.h>
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/event/task_scheduler.h"
#include "core/quickjs/window_bindings.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace lightui;

/**
 * @brief 集成测试基类
 */
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化 SDL
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            SDL_Init(SDL_INIT_VIDEO);
        }
    }

    void TearDown() override {
        // 清理
    }
};

/**
 * @brief 测试 Window 和 Document 集成
 */
TEST_F(IntegrationTest, WindowDocumentIntegration) {
    // 创建窗口
    WindowConfig config;
    config.title = "Integration Test";
    config.width = 800;
    config.height = 600;
    config.hidden = true;  // 隐藏窗口以避免显示
    
    auto window = std::make_shared<Window>(config);
    ASSERT_NE(window, nullptr);
    
    // 创建文档
    auto document = std::make_shared<Document>();
    document->Initialize();
    ASSERT_NE(document, nullptr);

    // 设置文档到窗口
    window->SetDocument(document);
    EXPECT_EQ(window->GetDocument(), document);
    
    // 验证文档结构
    auto body = document->GetBody();
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetTagName(), "body");
}

/**
 * @brief 测试 DOM 变化触发重绘
 */
TEST_F(IntegrationTest, DOMChangeTriggersRepaint) {
    // 创建窗口和文档
    WindowConfig config;
    config.title = "Repaint Test";
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    
    auto window = std::make_shared<Window>(config);
    auto document = std::make_shared<Document>();
    document->Initialize();
    window->SetDocument(document);

    // SetDocument 会触发重绘，先渲染一次
    EXPECT_TRUE(window->NeedsRepaint());
    window->RenderDocument();

    // 渲染后不再需要重绘
    EXPECT_FALSE(window->NeedsRepaint());
    
    // 修改 DOM - 添加元素
    auto body = document->GetBody();
    auto test_div = document->CreateElement("div");
    body->AppendChild(test_div);

    // 应该触发重绘
    EXPECT_TRUE(window->NeedsRepaint());

    // 清除重绘标志
    window->RenderDocument();
    EXPECT_FALSE(window->NeedsRepaint());

    // 修改属性
    test_div->SetAttribute("id", "test");
    EXPECT_TRUE(window->NeedsRepaint());

    // 清除重绘标志
    window->RenderDocument();
    EXPECT_FALSE(window->NeedsRepaint());

    // 修改样式
    test_div->SetStyle("color", "red");
    EXPECT_TRUE(window->NeedsRepaint());
}

/**
 * @brief 测试 TaskScheduler 集成
 */
TEST_F(IntegrationTest, TaskSchedulerIntegration) {
    auto scheduler = std::make_shared<TaskScheduler>();
    
    // 测试 setTimeout
    bool timeout_executed = false;
    int timeout_id = scheduler->SetTimeout([&timeout_executed]() {
        timeout_executed = true;
    }, 100);
    
    EXPECT_GT(timeout_id, 0);
    EXPECT_FALSE(timeout_executed);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    scheduler->ProcessTasks();
    
    EXPECT_TRUE(timeout_executed);
    
    // 测试 setInterval
    int interval_count = 0;
    int interval_id = scheduler->SetInterval([&interval_count]() {
        interval_count++;
    }, 50);
    
    EXPECT_GT(interval_id, 0);
    
    // 等待多次执行（需要多次调用 ProcessTasks）
    for (int i = 0; i < 4; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        scheduler->ProcessTasks();
    }

    EXPECT_GE(interval_count, 3);

    // 取消 interval
    scheduler->ClearInterval(interval_id);
    int count_before_clear = interval_count;

    // 再等待一段时间
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        scheduler->ProcessTasks();
    }

    EXPECT_EQ(interval_count, count_before_clear);
}

/**
 * @brief 测试 requestAnimationFrame
 */
TEST_F(IntegrationTest, RequestAnimationFrameIntegration) {
    auto scheduler = std::make_shared<TaskScheduler>();
    
    bool frame_executed = false;
    float received_delta = 0.0f;
    
    int frame_id = scheduler->RequestAnimationFrame([&](float delta_time) {
        frame_executed = true;
        received_delta = delta_time;
    });
    
    EXPECT_GT(frame_id, 0);
    EXPECT_FALSE(frame_executed);
    
    // 处理动画帧
    scheduler->ProcessAnimationFrames(0.016f);  // 60 FPS
    
    EXPECT_TRUE(frame_executed);
    EXPECT_FLOAT_EQ(received_delta, 0.016f);
}

/**
 * @brief 测试 JavaScript 兼容 API
 */
TEST_F(IntegrationTest, JavaScriptCompatibleAPI) {
    auto scheduler = std::make_shared<TaskScheduler>();
    
    // 测试 clearTimeout (JavaScript 兼容)
    bool executed = false;
    int timeout_id = scheduler->SetTimeout([&executed]() {
        executed = true;
    }, 100);
    
    scheduler->ClearTimeout(timeout_id);  // 使用 JavaScript 兼容的方法名
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    scheduler->ProcessTasks();
    
    EXPECT_FALSE(executed);  // 应该被取消
    
    // 测试 clearInterval (JavaScript 兼容)
    int count = 0;
    int interval_id = scheduler->SetInterval([&count]() {
        count++;
    }, 50);
    
    scheduler->ClearInterval(interval_id);  // 使用 JavaScript 兼容的方法名
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    scheduler->ProcessTasks();
    
    EXPECT_EQ(count, 0);  // 应该被取消
    
    // 测试 cancelAnimationFrame (JavaScript 兼容)
    bool frame_executed = false;
    int frame_id = scheduler->RequestAnimationFrame([&frame_executed](float) {
        frame_executed = true;
    });
    
    scheduler->CancelAnimationFrame(frame_id);  // 使用 JavaScript 兼容的方法名
    scheduler->ProcessAnimationFrames(0.016f);
    
    EXPECT_FALSE(frame_executed);  // 应该被取消
}

/**
 * @brief 测试 Window + DOM + TaskScheduler 完整集成
 */
TEST_F(IntegrationTest, FullIntegration) {
    // 创建所有组件
    WindowConfig config;
    config.title = "Full Integration Test";
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    
    auto window = std::make_shared<Window>(config);
    auto document = std::make_shared<Document>();
    document->Initialize();
    auto scheduler = std::make_shared<TaskScheduler>();

    // 连接组件
    window->SetDocument(document);
    
    // 创建 DOM 结构
    auto body = document->GetBody();
    auto div = document->CreateElement("div");
    div->SetTextContent("0");
    body->AppendChild(div);  // 先添加到 DOM 树
    div->SetAttribute("id", "counter");  // 然后设置 ID
    
    // 验证初始状态
    EXPECT_TRUE(window->NeedsRepaint());
    window->RenderDocument();
    EXPECT_FALSE(window->NeedsRepaint());
    
    // 使用定时器更新 DOM
    int counter = 0;
    scheduler->SetInterval([document, &counter]() {
        auto div = document->GetElementById("counter");
        if (div) {
            counter++;
            div->SetTextContent(std::to_string(counter));
        }
    }, 50);
    
    // 模拟几帧（需要多次调用 ProcessTasks）
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        scheduler->ProcessTasks();

        if (window->NeedsRepaint()) {
            window->RenderDocument();
        }
    }
    
    // 验证计数器更新
    EXPECT_GT(counter, 0);
    auto counter_div = document->GetElementById("counter");
    ASSERT_NE(counter_div, nullptr);
    EXPECT_EQ(counter_div->GetTextContent(), std::to_string(counter));
}

/**
 * @brief 测试多窗口场景
 */
TEST_F(IntegrationTest, MultiWindowIntegration) {
    auto& manager = WindowManager::Instance();
    
    // 创建第一个窗口
    WindowConfig config1;
    config1.title = "Window 1";
    config1.width = 800;
    config1.height = 600;
    config1.hidden = true;
    
    auto window1 = std::make_shared<Window>(config1);
    auto document1 = std::make_shared<Document>();
    document1->Initialize();
    window1->SetDocument(document1);
    manager.RegisterWindow(window1);

    // 创建第二个窗口
    WindowConfig config2;
    config2.title = "Window 2";
    config2.width = 640;
    config2.height = 480;
    config2.hidden = true;

    auto window2 = std::make_shared<Window>(config2);
    auto document2 = std::make_shared<Document>();
    document2->Initialize();
    window2->SetDocument(document2);
    manager.RegisterWindow(window2);

    // SetDocument 会触发重绘，先渲染两个窗口
    window1->RenderDocument();
    window2->RenderDocument();

    // 验证两个窗口独立
    EXPECT_NE(window1->GetDocument(), window2->GetDocument());
    EXPECT_FALSE(window1->NeedsRepaint());
    EXPECT_FALSE(window2->NeedsRepaint());

    // 修改第一个窗口的 DOM
    auto body1 = document1->GetBody();
    auto div_elem1 = document1->CreateElement("div");
    body1->AppendChild(div_elem1);

    EXPECT_TRUE(window1->NeedsRepaint());
    EXPECT_FALSE(window2->NeedsRepaint());  // 第二个窗口不受影响
    
    // 清理
    manager.UnregisterWindow(window1);
    manager.UnregisterWindow(window2);
}

