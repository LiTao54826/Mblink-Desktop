#include "background_task_runner.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace mbink {
namespace {

struct QueuedTask {
    std::function<void()> task;
    std::function<void()> on_drop;
    std::string name;
};

} // namespace

struct BackgroundTaskRunner::State {
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::deque<QueuedTask> tasks;
    std::vector<std::thread> workers;
    bool shutting_down = false;
};

BackgroundTaskRunner::BackgroundTaskRunner(size_t worker_count)
    : state_(std::make_shared<State>()) {
    worker_count = std::max<size_t>(1, worker_count);
    state_->workers.reserve(worker_count);
    for (size_t i = 0; i < worker_count; ++i) {
        state_->workers.emplace_back([this]() {
            WorkerLoop();
        });
    }
}

BackgroundTaskRunner::~BackgroundTaskRunner() {
    Shutdown();
}

bool BackgroundTaskRunner::Post(std::function<void()> task, std::string name) {
    return Post(std::move(task), {}, std::move(name));
}

bool BackgroundTaskRunner::Post(std::function<void()> task,
                                std::function<void()> on_drop,
                                std::string name) {
    if (!task) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->shutting_down) {
            return false;
        }
        state_->tasks.push_back({std::move(task), std::move(on_drop), std::move(name)});
    }

    state_->cv.notify_one();
    return true;
}

void BackgroundTaskRunner::Shutdown() {
    std::deque<QueuedTask> dropped;
    std::vector<std::thread> workers;

    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->shutting_down && state_->workers.empty()) {
            return;
        }
        state_->shutting_down = true;
        dropped.swap(state_->tasks);
        workers.swap(state_->workers);
    }

    for (auto& queued : dropped) {
        if (queued.on_drop) {
            queued.on_drop();
        }
    }

    state_->cv.notify_all();

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

bool BackgroundTaskRunner::IsShuttingDown() const {
    std::lock_guard<std::mutex> lock(state_->mutex);
    return state_->shutting_down;
}

size_t BackgroundTaskRunner::DefaultWorkerCount() {
    const unsigned int hardware = std::thread::hardware_concurrency();
    if (hardware == 0) {
        return 2;
    }
    return std::clamp<size_t>(hardware > 1 ? hardware - 1 : 1, 1, 4);
}

void BackgroundTaskRunner::WorkerLoop() {
    while (true) {
        QueuedTask queued;
        {
            std::unique_lock<std::mutex> lock(state_->mutex);
            state_->cv.wait(lock, [this]() {
                return state_->shutting_down || !state_->tasks.empty();
            });

            if (state_->shutting_down && state_->tasks.empty()) {
                return;
            }

            queued = std::move(state_->tasks.front());
            state_->tasks.pop_front();
        }

        queued.task();
    }
}

} // namespace mbink
