/**
 * @file test_javascript_bindings.cpp
 * @brief JavaScript 绑定集成测试
 */

#include <gtest/gtest.h>
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/event/task_scheduler.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace lightui;
using json = nlohmann::json;

/**
 * @brief JavaScript 绑定测试基类
 */
class JavaScriptBindingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化 SDL
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            SDL_Init(SDL_INIT_VIDEO);
        }

        // 创建窗口
        WindowConfig config;
        config.title = "JS Bindings Test";
        config.width = 800;
        config.height = 600;
        config.hidden = true;

        window_ = std::make_shared<Window>(config);
        document_ = std::make_shared<Document>();
        document_->Initialize();  // 初始化 DOM 结构
        scheduler_ = std::make_shared<TaskScheduler>();

        window_->SetDocument(document_);

        // 创建 QuickJS 运行时
        runtime_ = std::make_shared<QuickJSRuntime>();

        // 创建 JavaScript 绑定
        bindings_ = std::make_unique<WindowBindings>(runtime_.get(), window_, scheduler_);
        bindings_->InitBindings();
    }

    void TearDown() override {
        bindings_.reset();
        runtime_.reset();
        scheduler_.reset();
        document_.reset();
        window_.reset();
    }

    // 辅助方法：执行 JavaScript 并返回结果
    json EvalJS(const std::string& code) {
        try {
            return runtime_->Eval(code);
        } catch (const std::exception& e) {
            ADD_FAILURE() << "JavaScript execution failed: " << e.what();
            return json();
        }
    }

    std::shared_ptr<Window> window_;
    std::shared_ptr<Document> document_;
    std::shared_ptr<TaskScheduler> scheduler_;
    std::shared_ptr<QuickJSRuntime> runtime_;
    std::unique_ptr<WindowBindings> bindings_;
};

/**
 * @brief 测试 window.innerWidth 和 window.innerHeight
 */
TEST_F(JavaScriptBindingsTest, WindowSize) {
    auto result = EvalJS("window.innerWidth");
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 800);

    result = EvalJS("window.innerHeight");
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 600);
}

/**
 * @brief 测试 window.devicePixelRatio
 */
TEST_F(JavaScriptBindingsTest, DevicePixelRatio) {
    auto result = EvalJS("window.devicePixelRatio");
    EXPECT_TRUE(result.is_number());
    EXPECT_GE(result.get<double>(), 1.0);
}

/**
 * @brief 测试 window.title
 */
TEST_F(JavaScriptBindingsTest, WindowTitle) {
    // 读取标题
    auto result = EvalJS("window.title");
    EXPECT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "JS Bindings Test");

    // 设置标题
    EvalJS("window.title = 'New Title'");
    result = EvalJS("window.title");
    EXPECT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "New Title");
}

/**
 * @brief 测试 document.body
 */
TEST_F(JavaScriptBindingsTest, DocumentBody) {
    auto result = EvalJS("typeof document.body");
    EXPECT_EQ(result.get<std::string>(), "object");

    result = EvalJS("document.body !== null");
    EXPECT_TRUE(result.get<bool>());
}

/**
 * @brief 测试 document.getElementById
 */
TEST_F(JavaScriptBindingsTest, GetElementById) {
    // 先创建一个带 ID 的元素
    auto body = document_->GetBody();
    if (body) {
        auto div = document_->CreateElement("div");
        div->SetTextContent("Hello");
        body->AppendChild(div);  // 先添加到 DOM 树
        div->SetAttribute("id", "test-div");  // 然后设置 ID

        // 从 JavaScript 查找
        auto result = EvalJS("document.getElementById('test-div') !== null");
        EXPECT_TRUE(result.get<bool>());
    }
}

/**
 * @brief 测试 document.createElement
 */
TEST_F(JavaScriptBindingsTest, CreateElement) {
    auto result = EvalJS("typeof document.createElement('span')");
    EXPECT_EQ(result.get<std::string>(), "object");

    result = EvalJS("document.createElement('span') !== null");
    EXPECT_TRUE(result.get<bool>());
}

/**
 * @brief 测试 setTimeout
 */
TEST_F(JavaScriptBindingsTest, SetTimeout) {
    // 设置 timeout
    EvalJS(R"(
        globalThis.executed = false;
        setTimeout(function() {
            globalThis.executed = true;
        }, 50);
    )");

    // 检查初始状态
    auto result = EvalJS("globalThis.executed");
    EXPECT_FALSE(result.get<bool>());

    // 等待并处理任务
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler_->ProcessTasks();
    runtime_->ProcessMicrotasks();

    // 检查是否执行
    result = EvalJS("globalThis.executed");
    EXPECT_TRUE(result.get<bool>());
}

/**
 * @brief 测试 setInterval
 */
TEST_F(JavaScriptBindingsTest, SetInterval) {
    // 设置 interval
    EvalJS(R"(
        globalThis.count = 0;
        globalThis.intervalId = setInterval(function() {
            globalThis.count++;
        }, 30);
    )");

    // 检查初始状态
    auto result = EvalJS("globalThis.count");
    EXPECT_EQ(result.get<int>(), 0);

    // 等待多次执行（需要多次调用 ProcessTasks）
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
        scheduler_->ProcessTasks();
        runtime_->ProcessMicrotasks();
    }

    // 检查计数
    result = EvalJS("globalThis.count");
    int count_before = result.get<int>();
    EXPECT_GE(count_before, 3);

    // 清除 interval
    EvalJS("clearInterval(globalThis.intervalId)");

    // 再等待一段时间
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
        scheduler_->ProcessTasks();
    }

    // 验证停止执行
    result = EvalJS("globalThis.count");
    EXPECT_EQ(result.get<int>(), count_before);
}

/**
 * @brief 测试 clearTimeout
 */
TEST_F(JavaScriptBindingsTest, ClearTimeout) {
    // 设置并立即取消 timeout
    EvalJS(R"(
        globalThis.executed = false;
        var timeoutId = setTimeout(function() {
            globalThis.executed = true;
        }, 50);
        clearTimeout(timeoutId);
    )");

    // 等待
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler_->ProcessTasks();
    runtime_->ProcessMicrotasks();

    // 验证未执行
    auto result = EvalJS("globalThis.executed");
    EXPECT_FALSE(result.get<bool>());
}

/**
 * @brief 测试 requestAnimationFrame
 */
TEST_F(JavaScriptBindingsTest, RequestAnimationFrame) {
    // 设置动画帧回调
    EvalJS(R"(
        globalThis.frameExecuted = false;
        globalThis.receivedTimestamp = 0;
        requestAnimationFrame(function(timestamp) {
            globalThis.frameExecuted = true;
            globalThis.receivedTimestamp = timestamp;
        });
    )");

    // 检查初始状态
    auto result = EvalJS("globalThis.frameExecuted");
    EXPECT_FALSE(result.get<bool>());

    // 处理动画帧
    scheduler_->ProcessAnimationFrames(0.016f);
    runtime_->ProcessMicrotasks();

    // 验证执行
    result = EvalJS("globalThis.frameExecuted");
    EXPECT_TRUE(result.get<bool>());

    result = EvalJS("globalThis.receivedTimestamp");
    EXPECT_NEAR(result.get<double>(), 0.016, 0.001);
}

/**
 * @brief 测试完整的 JavaScript 应用场景
 */
TEST_F(JavaScriptBindingsTest, FullApplicationScenario) {
    // 创建 DOM 结构并设置定时器
    EvalJS(R"(
        // 初始化计数
        globalThis.count = 0;

        // 使用 setInterval 更新计数
        globalThis.intervalId = setInterval(function() {
            globalThis.count++;

            if (globalThis.count >= 3) {
                clearInterval(globalThis.intervalId);
            }
        }, 30);
    )");

    // 模拟几帧
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        scheduler_->ProcessTasks();
        runtime_->ProcessMicrotasks();

        if (window_->NeedsRepaint()) {
            window_->RenderDocument();
        }
    }

    // 验证最终状态（由于定时器的异步性质，count 可能略大于 3）
    auto result = EvalJS("globalThis.count");
    int final_count = result.get<int>();
    EXPECT_GE(final_count, 3);  // 至少执行了 3 次
    EXPECT_LE(final_count, 6);  // 不会执行太多次
}

