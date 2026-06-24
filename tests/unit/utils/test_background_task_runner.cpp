#include <gtest/gtest.h>

#include "utils/background_task_runner.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

namespace mbink {
namespace test {
namespace {

using namespace std::chrono_literals;

TEST(BackgroundTaskRunnerTest, RunsPostedTask) {
    BackgroundTaskRunner runner(1);
    std::promise<void> done;
    auto future = done.get_future();

    ASSERT_TRUE(runner.Post([&done]() {
        done.set_value();
    }));

    EXPECT_EQ(future.wait_for(2s), std::future_status::ready);
    runner.Shutdown();
}

TEST(BackgroundTaskRunnerTest, RejectsPostAfterShutdown) {
    BackgroundTaskRunner runner(1);
    runner.Shutdown();

    EXPECT_FALSE(runner.Post([]() {}));
    EXPECT_TRUE(runner.IsShuttingDown());
}

TEST(BackgroundTaskRunnerTest, InvokesOnDropForQueuedTasks) {
    BackgroundTaskRunner runner(1);
    std::promise<void> first_started;
    std::promise<void> release_first;
    auto first_started_future = first_started.get_future();
    auto release_first_future = release_first.get_future().share();
    std::atomic<int> ran{0};
    std::atomic<int> dropped{0};

    ASSERT_TRUE(runner.Post([&]() {
        first_started.set_value();
        release_first_future.wait();
    }));
    ASSERT_TRUE(runner.Post([&]() {
        ran.fetch_add(1);
    }, [&]() {
        dropped.fetch_add(1);
    }));
    ASSERT_TRUE(runner.Post([&]() {
        ran.fetch_add(1);
    }, [&]() {
        dropped.fetch_add(1);
    }));

    ASSERT_EQ(first_started_future.wait_for(2s), std::future_status::ready);
    std::thread shutdown_thread([&runner]() {
        runner.Shutdown();
    });

    for (int i = 0; i < 100 && dropped.load() < 2; ++i) {
        std::this_thread::sleep_for(10ms);
    }

    release_first.set_value();
    shutdown_thread.join();

    EXPECT_EQ(ran.load(), 0);
    EXPECT_EQ(dropped.load(), 2);
}

TEST(BackgroundTaskRunnerTest, DestructorJoinsInFlightTask) {
    std::atomic<bool> completed{false};
    std::promise<void> started;
    auto started_future = started.get_future();

    {
        BackgroundTaskRunner runner(1);
        ASSERT_TRUE(runner.Post([&]() {
            started.set_value();
            std::this_thread::sleep_for(10ms);
            completed = true;
        }));

        ASSERT_EQ(started_future.wait_for(2s), std::future_status::ready);
    }

    EXPECT_TRUE(completed.load());
}

} // namespace
} // namespace test
} // namespace mbink
