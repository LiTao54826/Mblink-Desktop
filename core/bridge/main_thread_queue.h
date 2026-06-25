#pragma once
/**
 * @file main_thread_queue.h
 * @brief 主线程调度队列 — 线程安全的 MPSC (Multiple Producer, Single Consumer) 队列
 *
 * 任意线程调用 post() 投递操作，主线程在 event loop 中调用 flush() 执行。
 * 复用 HostBridge 已有的 alive flag 模式保证生命周期安全。
 */

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace mblink {

class MainThreadQueue {
public:
    MainThreadQueue()
        : mainThreadId_(std::this_thread::get_id())
        , alive_(std::make_shared<std::atomic<bool>>(true)) {}

    ~MainThreadQueue() {
        *alive_ = false;
    }

    MainThreadQueue(const MainThreadQueue&) = delete;
    MainThreadQueue& operator=(const MainThreadQueue&) = delete;

    bool isMainThread() const {
        return std::this_thread::get_id() == mainThreadId_;
    }

    void setWakeCallback(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        wakeCallback_ = std::move(callback);
    }

    void post(std::function<void()> fn) {
        std::function<void()> wake;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(fn));
            wake = wakeCallback_;
        }
        if (wake) {
            wake();
        }
    }

    void flush() {
        std::vector<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(batch, queue_);
        }
        for (auto& fn : batch) {
            if (alive_->load()) {
                fn();
            }
        }
    }

    std::shared_ptr<std::atomic<bool>> aliveFlag() const {
        return alive_;
    }

private:
    std::thread::id mainThreadId_;
    std::mutex mutex_;
    std::vector<std::function<void()>> queue_;
    std::function<void()> wakeCallback_;
    std::shared_ptr<std::atomic<bool>> alive_;
};

} // namespace mblink

