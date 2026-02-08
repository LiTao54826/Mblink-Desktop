/**
 * @file task_scheduler.cpp
 * @brief 任务调度器实现
 */

#include "task_scheduler.h"
#include <algorithm>

namespace lightui {

TaskScheduler& TaskScheduler::Instance() {
    static TaskScheduler instance;
    return instance;
}

TaskScheduler::TaskScheduler()
    : next_task_id_(1)
    , performance_frequency_(SDL_GetPerformanceFrequency())
{
}

int TaskScheduler::SetTimeout(std::function<void()> callback, int delay_ms) {
    Task task;
    task.id = next_task_id_++;
    task.type = TaskType::TIMEOUT;
    task.callback = callback;
    task.execute_time = GetCurrentTime() + MillisecondsToTicks(delay_ms);
    task.interval = 0;  // 一次性任务
    task.cancelled = false;
    
    tasks_.push(task);
    
    return task.id;
}

int TaskScheduler::SetInterval(std::function<void()> callback, int interval_ms) {
    Task task;
    task.id = next_task_id_++;
    task.type = TaskType::INTERVAL;
    task.callback = callback;
    task.execute_time = GetCurrentTime() + MillisecondsToTicks(interval_ms);
    task.interval = interval_ms;
    task.cancelled = false;

    tasks_.push(task);

    return task.id;
}

int TaskScheduler::RequestAnimationFrame(std::function<void(double)> callback) {
    Task task;
    task.id = next_task_id_++;
    task.type = TaskType::ANIMATION_FRAME;
    task.anim_callback = callback;
    task.execute_time = 0;
    task.interval = 0;
    task.cancelled = false;

    animation_frame_tasks_.push_back(task);

    return task.id;
}

void TaskScheduler::ClearTask(int task_id) {
    // 标记任务为已取消（实际删除在处理时进行）
    // 注意：由于使用优先队列，无法直接删除，只能标记
    
    // 取消动画帧任务
    for (auto& task : animation_frame_tasks_) {
        if (task.id == task_id) {
            task.cancelled = true;
            return;
        }
    }
    
    // 对于定时任务，我们无法直接从优先队列中删除
    // 只能在执行时检查 cancelled 标志
    // 这里我们需要重建队列来标记任务
    std::vector<Task> temp_tasks;
    while (!tasks_.empty()) {
        Task task = tasks_.top();
        tasks_.pop();
        
        if (task.id == task_id) {
            task.cancelled = true;
        }
        temp_tasks.push_back(task);
    }
    
    for (const auto& task : temp_tasks) {
        tasks_.push(task);
    }
}

void TaskScheduler::ProcessTasks() {
    Uint64 current_time = GetCurrentTime();
    std::vector<Task> requeue_tasks;  // 需要重新入队的 interval 任务

    while (!tasks_.empty()) {
        const Task& task = tasks_.top();

        // 如果任务还没到执行时间，退出循环
        if (task.execute_time > current_time) {
            break;
        }

        // 取出任务
        Task current_task = task;
        tasks_.pop();

        // 跳过已取消的任务
        if (current_task.cancelled) {
            continue;
        }

        // 执行任务
        if (current_task.callback) {
            current_task.callback();
        }

        // 如果是 interval 任务，重新入队
        if (current_task.type == TaskType::INTERVAL && current_task.interval > 0) {
            current_task.execute_time = current_time + MillisecondsToTicks(current_task.interval);
            requeue_tasks.push_back(current_task);
        }
    }

    // 重新入队 interval 任务
    for (const auto& task : requeue_tasks) {
        tasks_.push(task);
    }
}

void TaskScheduler::ProcessAnimationFrames(double timestamp) {
    // 复制当前的任务列表，然后清空原列表
    // 这样在执行回调时，新的 requestAnimationFrame 调用会添加到空列表中
    std::vector<Task> tasks_to_execute = std::move(animation_frame_tasks_);
    animation_frame_tasks_.clear();

    // 执行所有动画帧任务
    for (const auto& task : tasks_to_execute) {
        if (!task.cancelled && task.anim_callback) {
            task.anim_callback(timestamp);
        }
    }

    // 不需要额外的清理，因为我们已经移动了任务列表
    // 在回调中新添加的任务已经在 animation_frame_tasks_ 中了
}

bool TaskScheduler::HasPendingTasks() const {
    // 检查是否有未取消的定时任务
    if (!tasks_.empty()) {
        return true;
    }
    
    // 检查是否有未取消的动画帧任务
    for (const auto& task : animation_frame_tasks_) {
        if (!task.cancelled) {
            return true;
        }
    }
    
    return false;
}

void TaskScheduler::ClearAllTasks() {
    // 清空优先队列
    while (!tasks_.empty()) {
        tasks_.pop();
    }

    // 清空动画帧任务
    animation_frame_tasks_.clear();

    // 清空微任务，避免闭包继续持有 JSValue
    microtasks_.clear();
}

Uint64 TaskScheduler::GetCurrentTime() const {
    return SDL_GetPerformanceCounter();
}

Uint64 TaskScheduler::MillisecondsToTicks(int ms) const {
    return (static_cast<Uint64>(ms) * performance_frequency_) / 1000;
}

void TaskScheduler::PostMicrotask(std::function<void()> callback) {
    if (callback) {
        microtasks_.push_back(std::move(callback));
    }
}

void TaskScheduler::ProcessMicrotasks() {
    // 处理所有微任务，注意微任务可能会添加新的微任务
    // 所以需要循环处理直到队列为空
    while (!microtasks_.empty()) {
        // 取出当前所有微任务
        std::vector<std::function<void()>> tasks_to_execute = std::move(microtasks_);
        microtasks_.clear();

        // 执行所有微任务
        for (auto& task : tasks_to_execute) {
            if (task) {
                task();
            }
        }
    }
}

} // namespace lightui

