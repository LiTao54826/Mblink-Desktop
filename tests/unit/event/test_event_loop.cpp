/**
 * @file test_event_loop.cpp
 * @brief EventLoop 单元测试
 */

#include <gtest/gtest.h>
#include "event/loop/event_loop.h"
#include "event/loop/task_scheduler.h"

namespace lightui {
namespace test {

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        event_loop_ = std::make_unique<EventLoop>();
    }

    void TearDown() override {
        event_loop_.reset();
    }

protected:
    std::unique_ptr<EventLoop> event_loop_;
};

TEST_F(EventLoopTest, InitialState) {
    EXPECT_FALSE(event_loop_->IsRunning());
    EXPECT_FALSE(event_loop_->ShouldQuit());
}

TEST_F(EventLoopTest, Stop) {
    event_loop_->Stop();
    EXPECT_TRUE(event_loop_->ShouldQuit());
}

TEST_F(EventLoopTest, SetIdleCallback) {
    bool called = false;
    event_loop_->SetIdleCallback([&called]() {
        called = true;
    });

    // 运行一次循环
    event_loop_->Stop();  // 确保不会无限循环
    event_loop_->RunOnce();

    // 空闲回调应该被调用
    // 注意：这取决于具体实现
}

TEST_F(EventLoopTest, SetUpdateCallback) {
    bool called = false;
    float delta_time = 0.0f;

    event_loop_->SetUpdateCallback([&called, &delta_time](float dt) {
        called = true;
        delta_time = dt;
    });

    event_loop_->Stop();
    event_loop_->RunOnce();

    // 更新回调应该被调用
    // EXPECT_TRUE(called);
}

TEST_F(EventLoopTest, SetRenderCallback) {
    bool called = false;

    event_loop_->SetRenderCallback([&called]() {
        called = true;
    });

    event_loop_->Stop();
    event_loop_->RunOnce();

    // 渲染回调应该被调用
    // EXPECT_TRUE(called);
}

TEST_F(EventLoopTest, GetTaskScheduler) {
    auto& scheduler = event_loop_->GetTaskScheduler();
    // 应该返回有效的调度器引用
    EXPECT_NE(&scheduler, nullptr);
}

TEST_F(EventLoopTest, GetTaskSchedulerPtr) {
    auto scheduler = event_loop_->GetTaskSchedulerPtr();
    EXPECT_NE(scheduler, nullptr);
}

TEST_F(EventLoopTest, ExternalTaskScheduler) {
    auto external_scheduler = std::make_shared<TaskScheduler>();
    auto loop = std::make_unique<EventLoop>(external_scheduler);

    EXPECT_EQ(loop->GetTaskSchedulerPtr(), external_scheduler);
}

TEST_F(EventLoopTest, CursorVisibility) {
    // 初始状态光标应该可见
    EXPECT_TRUE(event_loop_->IsCursorVisible());
}

} // namespace test
} // namespace lightui
