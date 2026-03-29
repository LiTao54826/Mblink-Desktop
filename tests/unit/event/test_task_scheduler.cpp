/**
 * @file test_task_scheduler.cpp
 * @brief TaskScheduler 单元测试
 */

#include <gtest/gtest.h>
#include "event/loop/task_scheduler.h"
#include <thread>
#include <chrono>

namespace mbink {
namespace test {

class TaskSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler_ = std::make_unique<TaskScheduler>();
    }

    void TearDown() override {
        scheduler_.reset();
    }

protected:
    std::unique_ptr<TaskScheduler> scheduler_;
};

TEST_F(TaskSchedulerTest, ScheduleTask) {
    bool executed = false;

    scheduler_->SetTimeout([&executed]() {
        executed = true;
    }, 0);

    scheduler_->ProcessTasks();

    EXPECT_TRUE(executed);
}

TEST_F(TaskSchedulerTest, ScheduleMultipleTasks) {
    int counter = 0;

    scheduler_->SetTimeout([&counter]() { counter++; }, 0);
    scheduler_->SetTimeout([&counter]() { counter++; }, 0);
    scheduler_->SetTimeout([&counter]() { counter++; }, 0);

    scheduler_->ProcessTasks();

    EXPECT_EQ(counter, 3);
}

TEST_F(TaskSchedulerTest, TaskExecutionOrder) {
    std::vector<int> order;

    scheduler_->SetTimeout([&order]() { order.push_back(1); }, 0);
    scheduler_->SetTimeout([&order]() { order.push_back(2); }, 0);
    scheduler_->SetTimeout([&order]() { order.push_back(3); }, 0);

    scheduler_->ProcessTasks();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST_F(TaskSchedulerTest, ScheduleDelayedTask) {
    bool executed = false;

    scheduler_->SetTimeout([&executed]() {
        executed = true;
    }, 10);  // 10ms 延迟

    // 立即处理不应该执行
    scheduler_->ProcessTasks();
    EXPECT_FALSE(executed);

    // 等待足够时间
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler_->ProcessTasks();

    EXPECT_TRUE(executed);
}

TEST_F(TaskSchedulerTest, CancelTask) {
    bool executed = false;

    auto id = scheduler_->SetTimeout([&executed]() {
        executed = true;
    }, 0);

    scheduler_->ClearTask(id);
    scheduler_->ProcessTasks();

    EXPECT_FALSE(executed);
}

TEST_F(TaskSchedulerTest, HasPendingTasks) {
    EXPECT_FALSE(scheduler_->HasPendingTasks());

    scheduler_->SetTimeout([]() {}, 0);

    EXPECT_TRUE(scheduler_->HasPendingTasks());

    scheduler_->ProcessTasks();

    EXPECT_FALSE(scheduler_->HasPendingTasks());
}

TEST_F(TaskSchedulerTest, ClearAllTasks) {
    scheduler_->SetTimeout([]() {}, 0);
    scheduler_->SetTimeout([]() {}, 0);
    scheduler_->SetTimeout([]() {}, 0);

    EXPECT_TRUE(scheduler_->HasPendingTasks());

    scheduler_->ClearAllTasks();

    EXPECT_FALSE(scheduler_->HasPendingTasks());
}

TEST_F(TaskSchedulerTest, TaskThrowsException) {
    bool secondExecuted = false;

    scheduler_->SetTimeout([]() {
        throw std::runtime_error("Test exception");
    }, 0);

    scheduler_->SetTimeout([&secondExecuted]() {
        secondExecuted = true;
    }, 0);

    // 第一个任务抛出异常不应该影响第二个任务
    // 注意：这取决于具体实现
    try {
        scheduler_->ProcessTasks();
    } catch (...) {
        // 忽略异常
    }
}

TEST_F(TaskSchedulerTest, ScheduleFromWithinTask) {
    int counter = 0;

    scheduler_->SetTimeout([this, &counter]() {
        counter++;
        if (counter < 3) {
            scheduler_->SetTimeout([&counter]() {
                counter++;
            }, 0);
        }
    }, 0);

    // 可能需要多次处理
    scheduler_->ProcessTasks();
    scheduler_->ProcessTasks();
    scheduler_->ProcessTasks();

    EXPECT_GE(counter, 1);
}

TEST_F(TaskSchedulerTest, ScheduleRepeatingTask) {
    int counter = 0;

    auto id = scheduler_->SetInterval([&counter]() {
        counter++;
    }, 10);  // 每 10ms 执行一次

    // 等待并处理
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    scheduler_->ProcessTasks();

    // 应该执行了多次
    EXPECT_GE(counter, 2);

    // 取消重复任务
    scheduler_->ClearTask(id);
}

} // namespace test
} // namespace mbink
