/**
 * @file event_loop.cpp
 * @brief 主事件循环实现
 */

#include "event_loop.h"
#include "frame_controller.h"
#include "input_handler.h"
#include "task_scheduler.h"
#include "core/window/window_manager.h"
#include <iostream>

namespace lightui {

EventLoop::EventLoop()
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(std::make_unique<TaskScheduler>())
{
}

EventLoop::~EventLoop() {
    if (running_) {
        Stop();
    }
}

void EventLoop::Run() {
    if (running_) {
        return;  // 已经在运行
    }
    
    running_ = true;
    should_quit_ = false;
    
    std::cout << "[EventLoop] Starting main loop..." << std::endl;
    
    while (running_ && !should_quit_) {
        RunOnce();
    }
    
    running_ = false;
    std::cout << "[EventLoop] Main loop stopped" << std::endl;
}

void EventLoop::Stop() {
    should_quit_ = true;
}

void EventLoop::RunOnce() {
    frame_controller_->BeginFrame();

    // 1. 处理所有 SDL 事件
    bool has_events = ProcessEvents();

    // 2. 执行调度任务
    task_scheduler_->ProcessTasks();

    // 3. 更新应用状态
    float delta_time = frame_controller_->GetDeltaTime();
    Update(delta_time);

    // 4. 处理动画帧任务
    // 计算累积时间戳（毫秒）- requestAnimationFrame 需要这个
    static Uint64 start_time = SDL_GetPerformanceCounter();
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double timestamp_ms = ((current_time - start_time) * 1000.0) / frequency;

    task_scheduler_->ProcessAnimationFrames(timestamp_ms);

    // 5. 渲染
    Render();

    // 6. 检查是否应该退出
    auto& window_manager = WindowManager::Instance();
    if (!window_manager.HasWindows()) {
        should_quit_ = true;
    }

    // 7. 空闲处理
    if (!HasWork() && idle_callback_) {
        idle_callback_();
    }

    // 8. 帧率控制
    frame_controller_->EndFrame();
}

bool EventLoop::ShouldQuit() const {
    return should_quit_;
}

bool EventLoop::IsRunning() const {
    return running_;
}

void EventLoop::SetIdleCallback(std::function<void()> callback) {
    idle_callback_ = callback;
}

void EventLoop::SetUpdateCallback(std::function<void(float)> callback) {
    update_callback_ = callback;
}

void EventLoop::SetRenderCallback(std::function<void()> callback) {
    render_callback_ = callback;
}

FrameController& EventLoop::GetFrameController() {
    return *frame_controller_;
}

InputHandler& EventLoop::GetInputHandler() {
    return *input_handler_;
}

TaskScheduler& EventLoop::GetTaskScheduler() {
    return *task_scheduler_;
}

bool EventLoop::ProcessEvents() {
    bool has_events = false;
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        has_events = true;
        
        // 处理退出事件
        if (event.type == SDL_EVENT_QUIT) {
            should_quit_ = true;
            continue;
        }
        
        // 分发到输入处理器
        input_handler_->HandleSDLEvent(event);
        
        // 分发到窗口管理器
        auto& window_manager = WindowManager::Instance();
        window_manager.HandleEvent(event);
    }
    
    return has_events;
}

void EventLoop::Update(float delta_time) {
    if (update_callback_) {
        update_callback_(delta_time);
    }
}

void EventLoop::Render() {
    if (render_callback_) {
        render_callback_();
    }
    
    // 默认渲染：渲染所有窗口
    // 注意：这里只是示例，实际渲染逻辑应该在窗口中实现
    // auto& window_manager = WindowManager::Instance();
    // for (auto& window : window_manager.GetAllWindows()) {
    //     if (window->NeedsRedraw()) {
    //         window->Render();
    //     }
    // }
}

bool EventLoop::HasWork() const {
    // 检查是否有待处理的任务
    if (task_scheduler_->HasPendingTasks()) {
        return true;
    }
    
    // 检查是否有窗口需要重绘
    // auto& window_manager = WindowManager::Instance();
    // for (auto& window : window_manager.GetAllWindows()) {
    //     if (window->NeedsRedraw()) {
    //         return true;
    //     }
    // }
    
    return false;
}

} // namespace lightui

