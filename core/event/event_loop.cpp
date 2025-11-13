/**
 * @file event_loop.cpp
 * @brief 主事件循环实现
 */

#include "event_loop.h"
#include "frame_controller.h"
#include "input_handler.h"
#include "task_scheduler.h"
#include "focus_manager.h"
#include "drag_manager.h"
#include "mouse_event.h"
#include "keyboard_utils.h"
#include "hit_testing.h"
#include "event_types.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/render/style_resolver.h"
#include <iostream>
#include <algorithm>

namespace lightui {

EventLoop::EventLoop()
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(std::make_unique<TaskScheduler>())
    , focus_manager_(std::make_unique<FocusManager>())
    , drag_manager_(std::make_unique<DragManager>())
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

        // 处理键盘事件并分发到 DOM
        if (event.type == SDL_EVENT_KEY_DOWN ||
            event.type == SDL_EVENT_KEY_UP ||
            event.type == SDL_EVENT_TEXT_INPUT) {
            HandleKeyboardEventForDOM(event);
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
        std::cout << "[EventLoop] Mouse button at (" << mouse_x << ", " << mouse_y << ")" << std::endl;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
    }

    // 更新hover链（发送mouseover/mouseout事件并设置:hover伪类）
    // 参考：RmlUi/Source/Core/Context.cpp - ProcessMouseMove
    UpdateHoverChain(window_id, mouse_x, mouse_y);

    // 处理mousedown/mouseup和click/dblclick事件
    // 参考：W3C UI Events - dblclick事件需要在短时间内两次click同一元素
    static std::shared_ptr<Element> last_mousedown_element;
    static std::shared_ptr<Element> last_click_element;
    static Uint64 last_click_time = 0;
    static const Uint64 DOUBLE_CLICK_TIME_MS = 500;  // 500ms内的两次click算作dblclick

    // 使用渲染树进行 Hit Testing
    HitTestResult hit_result;

    // 构建渲染树用于 Hit Testing
    auto body = document->GetBody();
    if (body) {
        RenderTreeBuilder builder;
        auto root_render = builder.BuildRenderTree(body, nullptr);

        if (root_render) {
            // 布局渲染树
            int width, height;
            SDL_GetWindowSizeInPixels(window->GetSDLWindow(), &width, &height);
            root_render->Layout(static_cast<float>(width), static_cast<float>(height));

            // 使用渲染树进行 Hit Testing
            hit_result = hit_testing.HitTestRenderObject(root_render, mouse_x, mouse_y, 0.0f, 0.0f);
        }
    }

    // 如果没有命中任何元素
    if (!hit_result.IsValid()) {
        std::cout << "[EventLoop] No element hit at (" << mouse_x << ", " << mouse_y << ")" << std::endl;

        // 即使没有命中元素，mouseup时也要移除:active伪类
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && last_mousedown_element) {
            last_mousedown_element->SetPseudoClass("active", false);
            last_mousedown_element = nullptr;

            // 结束拖拽
            if (event.button.button == SDL_BUTTON_LEFT) {
                drag_manager_->EndDrag(mouse_x, mouse_y);
            }
        }

        return;
    }

    std::cout << "[EventLoop] Hit element: <" << hit_result.element->GetTagName() << ">" << std::endl;

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

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        last_mousedown_element = hit_result.element;
        // mousedown时设置:active伪类
        hit_result.element->SetPseudoClass("active", true);

        // 鼠标点击时设置焦点（参考RmlUi/Source/Core/Context.cpp - ProcessMouseButtonDown）
        // 使用FocusManager设置焦点，focus_visible=false（鼠标点击不显示焦点指示器）
        focus_manager_->SetFocus(hit_result.element, false);

        // 拖拽检测（参考RmlUi/Source/Core/Context.cpp - ProcessMouseButtonDown）
        // 只在左键按下时检测拖拽
        if (event.button.button == SDL_BUTTON_LEFT) {
            drag_manager_->StartDragDetection(hit_result.element);
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        // mouseup时移除:active伪类
        if (last_mousedown_element) {
            last_mousedown_element->SetPseudoClass("active", false);
        }

        if (last_mousedown_element == hit_result.element) {
            // 在同一个元素上 mousedown 和 mouseup，触发 click
            std::cout << "[EventLoop] Dispatching click event to <" << hit_result.element->GetTagName() << ">" << std::endl;
            auto click_event = std::make_shared<MouseEvent>(
                "click",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                button
            );
            hit_result.element->DispatchEvent(click_event);

            // 检测dblclick：在短时间内两次click同一元素
            Uint64 current_time = SDL_GetTicks();
            if (last_click_element == hit_result.element &&
                (current_time - last_click_time) <= DOUBLE_CLICK_TIME_MS) {
                // 触发dblclick事件
                auto dblclick_event = std::make_shared<MouseEvent>(
                    "dblclick",
                    static_cast<int>(mouse_x),
                    static_cast<int>(mouse_y),
                    button
                );
                hit_result.element->DispatchEvent(dblclick_event);

                // 重置click跟踪（避免三击触发两次dblclick）
                last_click_element = nullptr;
                last_click_time = 0;
            } else {
                // 记录这次click
                last_click_element = hit_result.element;
                last_click_time = current_time;
            }
        }
        last_mousedown_element = nullptr;

        // 结束拖拽（参考RmlUi/Source/Core/Context.cpp - ProcessMouseButtonUp）
        if (event.button.button == SDL_BUTTON_LEFT) {
            drag_manager_->EndDrag(mouse_x, mouse_y);
        }
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        // 更新拖拽状态（参考RmlUi/Source/Core/Context.cpp - ProcessMouseMove）
        if (drag_manager_->IsDragging()) {
            drag_manager_->UpdateDrag(mouse_x, mouse_y, document);
        }
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

void EventLoop::UpdateHoverChain(Uint32 window_id, float mouse_x, float mouse_y) {
    // 参考：RmlUi/Source/Core/Context.cpp - UpdateHoverChain

    // 获取窗口和文档
    auto& window_manager = WindowManager::Instance();
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }

    auto document = window->GetDocument();
    if (!document) {
        return;
    }

    // 执行Hit Testing获取当前鼠标下的元素（使用渲染树）
    HitTesting hit_testing;
    HitTestResult hit_result;

    auto body = document->GetBody();
    if (body) {
        RenderTreeBuilder builder;
        auto root_render = builder.BuildRenderTree(body, nullptr);

        if (root_render) {
            // 布局渲染树
            int width, height;
            SDL_GetWindowSizeInPixels(window->GetSDLWindow(), &width, &height);
            root_render->Layout(static_cast<float>(width), static_cast<float>(height));

            // 使用渲染树进行 Hit Testing
            hit_result = hit_testing.HitTestRenderObject(root_render, mouse_x, mouse_y, 0.0f, 0.0f);
        }
    }

    // 构建新的hover链（从目标元素到根元素）
    // 使用 weak_ptr 避免悬空指针问题
    std::vector<std::weak_ptr<Element>> new_hover_chain;
    std::weak_ptr<Element> new_hover_element;

    if (hit_result.IsValid()) {
        new_hover_element = hit_result.element;

        // 从目标元素向上遍历到根元素
        auto current = hit_result.element;
        while (current) {
            new_hover_chain.push_back(current);

            // 获取父元素
            auto parent_node = current->GetParentNode();
            if (parent_node && parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                current = std::static_pointer_cast<Element>(parent_node);
            } else {
                current = nullptr;
            }
        }
    }

    // 发送mouseout事件到离开的元素（在旧链中但不在新链中）
    SendEvents(hover_chain_, new_hover_chain, "mouseout", mouse_x, mouse_y);

    // 发送mouseover事件到进入的元素（在新链中但不在旧链中）
    SendEvents(new_hover_chain, hover_chain_, "mouseover", mouse_x, mouse_y);

    // 发送mouseleave/mouseenter事件（不冒泡版本）
    // 只发送到hover_element_本身，不发送到父元素
    auto old_hover = hover_element_.lock();
    auto new_hover = new_hover_element.lock();

    if (old_hover != new_hover) {
        // 发送mouseleave到旧的hover元素
        if (old_hover) {
            auto leave_event = std::make_shared<MouseEvent>(
                "mouseleave",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );
            old_hover->DispatchEvent(leave_event);
        }

        // 发送mouseenter到新的hover元素
        if (new_hover) {
            auto enter_event = std::make_shared<MouseEvent>(
                "mouseenter",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0
            );
            new_hover->DispatchEvent(enter_event);
        }
    }

    // 更新hover链
    hover_chain_ = std::move(new_hover_chain);
    hover_element_ = new_hover_element;
}

void EventLoop::SendEvents(const std::vector<std::weak_ptr<Element>>& old_items,
                          const std::vector<std::weak_ptr<Element>>& new_items,
                          const std::string& event_type,
                          float mouse_x,
                          float mouse_y) {
    // 参考：RmlUi/Source/Core/Context.cpp - SendEvents
    // 找出在old_items中但不在new_items中的元素

    for (const auto& weak_elem : old_items) {
        // 尝试锁定 weak_ptr
        auto element = weak_elem.lock();
        if (!element) {
            // 元素已被销毁，跳过
            continue;
        }

        // 检查是否在新集合中
        bool found = false;
        for (const auto& new_weak : new_items) {
            auto new_elem = new_weak.lock();
            if (new_elem && new_elem == element) {
                found = true;
                break;
            }
        }

        if (!found) {
            // 这个元素在旧集合中但不在新集合中

            // 创建鼠标事件
            auto mouse_event = std::make_shared<MouseEvent>(
                event_type,
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                0  // button = 0 for mouseover/mouseout
            );

            // 分发事件
            element->DispatchEvent(mouse_event);

            // 根据事件类型设置/移除:hover伪类
            if (event_type == "mouseover") {
                element->SetPseudoClass("hover", true);
            } else if (event_type == "mouseout") {
                element->SetPseudoClass("hover", false);
            }
        }
    }
}

void EventLoop::HandleKeyboardEventForDOM(const SDL_Event& event) {
    // 参考：W3C UI Events - KeyboardEvent
    // 参考：RmlUi/Source/Core/Context.cpp - ProcessKeyDown, ProcessKeyUp

    // 获取焦点元素
    auto focus_element = focus_manager_->GetFocusElement();
    if (!focus_element) {
        // 没有焦点元素，不分发键盘事件
        return;
    }

    // 获取修饰键状态
    SDL_Keymod mod = SDL_GetModState();
    bool ctrl_key = (mod & SDL_KMOD_CTRL) != 0;
    bool shift_key = (mod & SDL_KMOD_SHIFT) != 0;
    bool alt_key = (mod & SDL_KMOD_ALT) != 0;
    bool meta_key = (mod & SDL_KMOD_GUI) != 0;

    // 处理不同类型的键盘事件
    if (event.type == SDL_EVENT_KEY_DOWN) {
        // keydown事件
        std::string key = SDLKeycodeToKey(event.key.key, shift_key);
        std::string code = SDLScancodeToCode(event.key.scancode);
        int key_code = SDLKeycodeToKeyCode(event.key.key);
        bool repeat = event.key.repeat;

        auto keydown_event = std::make_shared<KeyboardEvent>(
            "keydown",
            key,
            code,
            key_code,
            ctrl_key,
            shift_key,
            alt_key,
            meta_key,
            repeat
        );

        focus_element->DispatchEvent(keydown_event);

        // 如果事件未被阻止，处理表单元素的键盘输入
        if (!keydown_event->IsDefaultPrevented()) {
            // 检查是否是表单元素
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focus_element);
            auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element);

            if (input_element) {
                input_element->HandleKeyPress(key, ctrl_key);
            } else if (textarea_element) {
                textarea_element->HandleKeyPress(key, ctrl_key);
            }

            // 处理Tab键导航
            if (event.key.key == SDLK_TAB) {
                // Tab键导航到下一个可聚焦元素
                // Shift+Tab反向导航
                auto document = std::dynamic_pointer_cast<Document>(focus_element->GetOwnerDocument());
                if (document) {
                    focus_manager_->TabToNextFocusableElement(document, shift_key);
                }
            }
        }

    } else if (event.type == SDL_EVENT_KEY_UP) {
        // keyup事件
        std::string key = SDLKeycodeToKey(event.key.key, shift_key);
        std::string code = SDLScancodeToCode(event.key.scancode);
        int key_code = SDLKeycodeToKeyCode(event.key.key);

        auto keyup_event = std::make_shared<KeyboardEvent>(
            "keyup",
            key,
            code,
            key_code,
            ctrl_key,
            shift_key,
            alt_key,
            meta_key,
            false  // keyup不会是repeat
        );

        focus_element->DispatchEvent(keyup_event);

    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
        // textinput事件（用于输入法等）
        // 注意：这是SDL特有的事件，W3C标准中没有直接对应
        // 用于处理IME输入和普通文本输入

        // 检查是否是表单元素
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focus_element);
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element);

        if (input_element) {
            input_element->HandleTextInput(event.text.text);
        } else if (textarea_element) {
            textarea_element->HandleTextInput(event.text.text);
        }
    }
}

} // namespace lightui

