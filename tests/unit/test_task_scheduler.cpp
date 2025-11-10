/**
 * @file test_task_scheduler.cpp
 * @brief TaskScheduler 单元测试
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "core/event/task_scheduler.h"

using namespace lightui;

// 测试夹具
class TaskSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler = std::make_unique<TaskScheduler>();
    }

    void TearDown() override {
        scheduler.reset();
    }
    
    std::unique_ptr<TaskScheduler> scheduler;
};

// 测试 setTimeout
TEST_F(TaskSchedulerTest, SetTimeout) {
    bool executed = false;
    
    int task_id = scheduler->SetTimeout([&executed]() {
        executed = true;
    }, 50);  // 50ms 延迟
    
    EXPECT_GT(task_id, 0);
    EXPECT_FALSE(executed);
    
    // 等待 30ms，任务不应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    scheduler->ProcessTasks();
    EXPECT_FALSE(executed);
    
    // 再等待 30ms，任务应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    scheduler->ProcessTasks();
    EXPECT_TRUE(executed);
}

// 测试 setTimeout 立即执行
TEST_F(TaskSchedulerTest, SetTimeoutImmediate) {
    bool executed = false;
    
    scheduler->SetTimeout([&executed]() {
        executed = true;
    }, 0);  // 立即执行
    
    scheduler->ProcessTasks();
    EXPECT_TRUE(executed);
}

// 测试多个 setTimeout
TEST_F(TaskSchedulerTest, MultipleSetTimeout) {
    int count = 0;

    scheduler->SetTimeout([&count]() { count++; }, 20);
    scheduler->SetTimeout([&count]() { count++; }, 40);
    scheduler->SetTimeout([&count]() { count++; }, 60);

    EXPECT_EQ(count, 0);

    // 等待 25ms，第一个任务应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    scheduler->ProcessTasks();
    EXPECT_EQ(count, 1);

    // 再等待 20ms，第二个任务应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler->ProcessTasks();
    EXPECT_EQ(count, 2);

    // 再等待 25ms，第三个任务应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    scheduler->ProcessTasks();
    EXPECT_EQ(count, 3);
}

// 测试 setInterval
TEST_F(TaskSchedulerTest, SetInterval) {
    int count = 0;
    
    int task_id = scheduler->SetInterval([&count]() {
        count++;
    }, 20);  // 每 20ms 执行一次
    
    EXPECT_GT(task_id, 0);
    EXPECT_EQ(count, 0);
    
    // 等待并处理任务，应该执行多次
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        scheduler->ProcessTasks();
    }
    
    EXPECT_GE(count, 4);  // 至少执行 4 次
    EXPECT_LE(count, 6);  // 不超过 6 次
}

// 测试 clearTimeout
TEST_F(TaskSchedulerTest, ClearTimeout) {
    bool executed = false;
    
    int task_id = scheduler->SetTimeout([&executed]() {
        executed = true;
    }, 50);
    
    // 取消任务
    scheduler->ClearTask(task_id);
    
    // 等待足够长的时间
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    scheduler->ProcessTasks();
    
    // 任务不应该执行
    EXPECT_FALSE(executed);
}

// 测试 clearInterval
TEST_F(TaskSchedulerTest, ClearInterval) {
    int count = 0;
    
    int task_id = scheduler->SetInterval([&count]() {
        count++;
    }, 20);
    
    // 执行几次
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        scheduler->ProcessTasks();
    }
    
    int count_before_clear = count;
    EXPECT_GE(count_before_clear, 2);
    
    // 取消任务
    scheduler->ClearTask(task_id);
    
    // 再等待一段时间
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        scheduler->ProcessTasks();
    }
    
    // 计数不应该增加
    EXPECT_EQ(count, count_before_clear);
}

// 测试 requestAnimationFrame
TEST_F(TaskSchedulerTest, RequestAnimationFrame) {
    bool executed = false;
    float received_delta_time = 0.0f;
    
    int task_id = scheduler->RequestAnimationFrame([&](float dt) {
        executed = true;
        received_delta_time = dt;
    });
    
    EXPECT_GT(task_id, 0);
    EXPECT_FALSE(executed);
    
    // 处理动画帧
    scheduler->ProcessAnimationFrames(0.016f);  // 16ms = 60 FPS
    
    EXPECT_TRUE(executed);
    EXPECT_FLOAT_EQ(received_delta_time, 0.016f);
}

// 测试多个 requestAnimationFrame
TEST_F(TaskSchedulerTest, MultipleRequestAnimationFrame) {
    int count = 0;
    
    scheduler->RequestAnimationFrame([&](float dt) { count++; });
    scheduler->RequestAnimationFrame([&](float dt) { count++; });
    scheduler->RequestAnimationFrame([&](float dt) { count++; });
    
    EXPECT_EQ(count, 0);
    
    // 处理一帧，所有任务都应该执行
    scheduler->ProcessAnimationFrames(0.016f);
    
    EXPECT_EQ(count, 3);
    
    // 再处理一帧，任务不应该再次执行（一次性任务）
    scheduler->ProcessAnimationFrames(0.016f);
    
    EXPECT_EQ(count, 3);
}

// 测试取消 requestAnimationFrame
TEST_F(TaskSchedulerTest, CancelRequestAnimationFrame) {
    bool executed = false;
    
    int task_id = scheduler->RequestAnimationFrame([&](float dt) {
        executed = true;
    });
    
    // 取消任务
    scheduler->ClearTask(task_id);
    
    // 处理动画帧
    scheduler->ProcessAnimationFrames(0.016f);
    
    // 任务不应该执行
    EXPECT_FALSE(executed);
}

// 测试 HasPendingTasks
TEST_F(TaskSchedulerTest, HasPendingTasks) {
    EXPECT_FALSE(scheduler->HasPendingTasks());
    
    // 添加 timeout 任务
    int task_id1 = scheduler->SetTimeout([]() {}, 100);
    EXPECT_TRUE(scheduler->HasPendingTasks());
    
    // 取消任务
    scheduler->ClearTask(task_id1);
    // 注意：取消的任务仍在队列中，直到被处理
    
    // 添加动画帧任务
    int task_id2 = scheduler->RequestAnimationFrame([](float dt) {});
    EXPECT_TRUE(scheduler->HasPendingTasks());
    
    // 处理动画帧
    scheduler->ProcessAnimationFrames(0.016f);
    
    // 处理定时任务
    scheduler->ProcessTasks();
}

// 测试 ClearAllTasks
TEST_F(TaskSchedulerTest, ClearAllTasks) {
    int count = 0;
    
    scheduler->SetTimeout([&count]() { count++; }, 50);
    scheduler->SetInterval([&count]() { count++; }, 20);
    scheduler->RequestAnimationFrame([&count](float dt) { count++; });
    
    EXPECT_TRUE(scheduler->HasPendingTasks());
    
    // 清除所有任务
    scheduler->ClearAllTasks();
    
    EXPECT_FALSE(scheduler->HasPendingTasks());
    
    // 等待并处理
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    scheduler->ProcessTasks();
    scheduler->ProcessAnimationFrames(0.016f);
    
    // 没有任务应该执行
    EXPECT_EQ(count, 0);
}

// 测试任务执行顺序
TEST_F(TaskSchedulerTest, TaskExecutionOrder) {
    std::vector<int> execution_order;
    
    scheduler->SetTimeout([&execution_order]() { execution_order.push_back(3); }, 30);
    scheduler->SetTimeout([&execution_order]() { execution_order.push_back(1); }, 10);
    scheduler->SetTimeout([&execution_order]() { execution_order.push_back(2); }, 20);
    
    // 等待所有任务到期
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    scheduler->ProcessTasks();
    
    // 任务应该按延迟时间顺序执行
    ASSERT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

// 测试 interval 任务的重复执行
TEST_F(TaskSchedulerTest, IntervalRepetition) {
    std::vector<int> execution_times;
    
    scheduler->SetInterval([&execution_times]() {
        execution_times.push_back(1);
    }, 15);
    
    // 运行约 100ms
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        scheduler->ProcessTasks();
    }
    
    // 应该执行约 6-7 次（100ms / 15ms ≈ 6.67）
    EXPECT_GE(execution_times.size(), 5);
    EXPECT_LE(execution_times.size(), 8);
}

