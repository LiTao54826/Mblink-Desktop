/**
 * @file event_loop.cpp
 * @brief 主事件循环实现
 */

#include "event_loop.h"
#include "frame_controller.h"
#include "input_handler.h"
#include "task_scheduler.h"
#include "mouse_event.h"
#include "hit_testing.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
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

        // 处理鼠标事件并分发到 DOM
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
            event.type == SDL_EVENT_MOUSE_MOTION) {
            HandleMouseEventForDOM(event);
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

void EventLoop::HandleMouseEventForDOM(const SDL_Event& event) {
    // 获取窗口管理器
    auto& window_manager = WindowManager::Instance();

    // 根据事件类型获取窗口 ID
    Uint32 window_id = 0;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        window_id = event.button.windowID;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        window_id = event.motion.windowID;
    }

    // 查找对应的窗口
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }

    // 获取窗口的文档
    auto document = window->GetDocument();
    if (!document) {
        return;
    }

    // 执行 Hit Testing
    HitTesting hit_testing;
    float mouse_x = 0, mouse_y = 0;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        mouse_x = event.button.x;
        mouse_y = event.button.y;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
    }

    auto hit_result = hit_testing.HitTest(document, mouse_x, mouse_y);

    // 如果没有命中任何元素，返回
    if (!hit_result.IsValid()) {
        return;
    }

    // 创建 MouseEvent（使用 core/dom/event.h 中的简化版本）
    std::string event_type;
    int button = 0;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        event_type = "mousedown";
        button = SDLButtonToMouseButton(event.button.button);
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        event_type = "mouseup";
        button = SDLButtonToMouseButton(event.button.button);
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        event_type = "mousemove";
        button = 0;
    }

    // 创建 DOM MouseEvent（使用简化的构造函数）
    auto mouse_event = std::make_shared<MouseEvent>(
        event_type,
        static_cast<int>(mouse_x),
        static_cast<int>(mouse_y),
        button
    );

    // 分发事件到目标元素
    hit_result.element->DispatchEvent(mouse_event);

    // 如果是 mousedown + mouseup，还需要触发 click 事件
    static std::shared_ptr<Element> last_mousedown_element;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        last_mousedown_element = hit_result.element;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && last_mousedown_element == hit_result.element) {
        // 在同一个元素上 mousedown 和 mouseup，触发 click
        auto click_event = std::make_shared<MouseEvent>(
            "click",
            static_cast<int>(mouse_x),
            static_cast<int>(mouse_y),
            button
        );
        hit_result.element->DispatchEvent(click_event);
        last_mousedown_element = nullptr;
    }
}

int EventLoop::SDLButtonToMouseButton(Uint8 sdl_button) {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:
            return 1;
        case SDL_BUTTON_MIDDLE:
            return 2;
        case SDL_BUTTON_RIGHT:
            return 3;
        default:
            return 0;
    }
}

} // namespace lightui

