#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace mbink {

class BackgroundTaskRunner {
public:
    explicit BackgroundTaskRunner(size_t worker_count = DefaultWorkerCount());
    ~BackgroundTaskRunner();

    BackgroundTaskRunner(const BackgroundTaskRunner&) = delete;
    BackgroundTaskRunner& operator=(const BackgroundTaskRunner&) = delete;

    bool Post(std::function<void()> task, std::string name = {});
    bool Post(std::function<void()> task,
              std::function<void()> on_drop,
              std::string name = {});

    void Shutdown();
    bool IsShuttingDown() const;

    static size_t DefaultWorkerCount();

private:
    struct State;

    void WorkerLoop();

    std::shared_ptr<State> state_;
};

} // namespace mbink
