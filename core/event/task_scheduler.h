/**
 * @file task_scheduler.h
 * @brief 任务调度器
 * 
 * 实现 setTimeout、setInterval 和 requestAnimationFrame
 */

#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <vector>
#include <queue>

namespace lightui {

/**
 * @brief 任务类型
 */
enum class TaskType {
    TIMEOUT,            // 一次性任务（setTimeout）
    INTERVAL,           // 重复任务（setInterval）
    ANIMATION_FRAME     // 动画帧任务（requestAnimationFrame）
};

/**
 * @brief 任务调度器类
 * 
 * 功能：
 * 1. setTimeout - 延迟执行一次
 * 2. setInterval - 定期重复执行
 * 3. requestAnimationFrame - 下一帧执行
 * 4. 任务优先级管理
 */
class TaskScheduler {
public:
    /**
     * @brief 构造函数
     */
    TaskScheduler();
    
    /**
     * @brief 析构函数
     */
    ~TaskScheduler() = default;

    /**
     * @brief setTimeout - 延迟执行
     * 
     * @param callback 回调函数
     * @param delay_ms 延迟时间（毫秒）
     * @return int 任务 ID
     */
    int SetTimeout(std::function<void()> callback, int delay_ms);
    
    /**
     * @brief setInterval - 定期执行
     * 
     * @param callback 回调函数
     * @param interval_ms 间隔时间（毫秒）
     * @return int 任务 ID
     */
    int SetInterval(std::function<void()> callback, int interval_ms);
    
    /**
     * @brief requestAnimationFrame - 下一帧执行
     * 
     * @param callback 回调函数，参数为帧时间（秒）
     * @return int 任务 ID
     */
    int RequestAnimationFrame(std::function<void(float)> callback);
    
    /**
     * @brief 取消任务
     *
     * @param task_id 任务 ID
     */
    void ClearTask(int task_id);

    /**
     * @brief 取消 setTimeout 任务（JavaScript 兼容 API）
     *
     * @param task_id 任务 ID
     */
    void ClearTimeout(int task_id) { ClearTask(task_id); }

    /**
     * @brief 取消 setInterval 任务（JavaScript 兼容 API）
     *
     * @param task_id 任务 ID
     */
    void ClearInterval(int task_id) { ClearTask(task_id); }

    /**
     * @brief 取消 requestAnimationFrame 任务（JavaScript 兼容 API）
     *
     * @param task_id 任务 ID
     */
    void CancelAnimationFrame(int task_id) { ClearTask(task_id); }

    /**
     * @brief 处理到期的任务
     * 
     * 执行所有到期的 timeout 和 interval 任务
     */
    void ProcessTasks();
    
    /**
     * @brief 处理动画帧任务
     * 
     * 执行所有 requestAnimationFrame 任务
     * 
     * @param delta_time 帧时间（秒）
     */
    void ProcessAnimationFrames(float delta_time);
    
    /**
     * @brief 检查是否有待处理的任务
     * 
     * @return true 如果有待处理的任务
     */
    bool HasPendingTasks() const;
    
    /**
     * @brief 清除所有任务
     */
    void ClearAllTasks();

private:
    /**
     * @brief 任务结构
     */
    struct Task {
        int id;                             // 任务 ID
        TaskType type;                      // 任务类型
        std::function<void()> callback;     // 回调函数（timeout/interval）
        std::function<void(float)> anim_callback;  // 动画回调函数
        Uint64 execute_time;                // 执行时间（性能计数器）
        int interval;                       // 间隔时间（毫秒，0 表示一次性）
        bool cancelled;                     // 是否已取消
        
        // 用于优先队列排序（执行时间早的优先）
        bool operator>(const Task& other) const {
            return execute_time > other.execute_time;
        }
    };

private:
    /**
     * @brief 获取当前时间
     * 
     * @return Uint64 当前时间（性能计数器）
     */
    Uint64 GetCurrentTime() const;
    
    /**
     * @brief 毫秒转性能计数器
     * 
     * @param ms 毫秒
     * @return Uint64 性能计数器值
     */
    Uint64 MillisecondsToTicks(int ms) const;

private:
    int next_task_id_;                      // 下一个任务 ID
    Uint64 performance_frequency_;          // 性能计数器频率
    
    // 使用优先队列管理定时任务（按执行时间排序）
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> tasks_;
    
    // 动画帧任务（每帧执行一次）
    std::vector<Task> animation_frame_tasks_;
};

} // namespace lightui

