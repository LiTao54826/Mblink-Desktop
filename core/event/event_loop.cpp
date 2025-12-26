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
#include "core/render/layer_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_button_element.h"
#include "core/dom/html_form_element.h"
#include "core/dom/html_select_element.h"
#include "core/render/select_dropdown.h"
#include "core/render/style_resolver.h"
#include "core/render/render_inline_block.h"
#include "core/render/text/font_manager.h"
#include "core/render/text_renderer.h"
#include "core/utils/utf8_utils.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/devtools/devtools_manager.h"
#include "core/devtools/inspector/element_picker.h"
#include "core/render/render_pipeline.h"
#include "core/render/render_object.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkFontMetrics.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace lightui {

EventLoop::EventLoop()
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(std::make_shared<TaskScheduler>())
    , focus_manager_(std::make_unique<FocusManager>())
    , drag_manager_(std::make_unique<DragManager>())
    , vsync_detected_(false)
{
    InitSystemCursors();
}

EventLoop::EventLoop(std::shared_ptr<TaskScheduler> task_scheduler)
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(task_scheduler)
    , focus_manager_(std::make_unique<FocusManager>())
    , drag_manager_(std::make_unique<DragManager>())
    , vsync_detected_(false)
{
    if (!task_scheduler_) {
        throw std::invalid_argument("TaskScheduler cannot be null");
    }
    InitSystemCursors();
}

EventLoop::~EventLoop() {
    if (running_) {
        Stop();
    }
    DestroySystemCursors();
}

void EventLoop::Run() {
    if (running_) {
        return;  // 已经在运行
    }

    running_ = true;
    should_quit_ = false;

    while (running_ && !should_quit_) {
        RunOnce();
    }

    running_ = false;
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

    // 2.5 处理 QuickJS 定时器和微任务
    if (quickjs_runtime_) {
        // 处理 QuickJS 内部的定时器队列
        quickjs_runtime_->RunEventLoop(1);  // 只运行一次迭代
    }

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

    // 4.6 关键修复：ProcessAnimationFrames 可能触发 Preact 等框架的状态更新
    // 这些更新可能通过微任务调度 DOM 变化，所以需要再次处理微任务
    // 否则 DOM 变化不会在当前帧被渲染，导致 UI 更新延迟
    if (quickjs_runtime_) {
        quickjs_runtime_->RunEventLoop(1);  // 处理可能产生的微任务
    }

    // 4.5 处理光标闪烁（如果有聚焦的输入框）
    static Uint64 last_cursor_blink_time = SDL_GetTicks();
    static bool cursor_visible = true;
    auto focus_element = focus_manager_->GetFocusElement();
    if (focus_element) {
        std::string tag_name = focus_element->GetTagName();
        if (tag_name == "input" || tag_name == "textarea") {
            // 每500毫秒切换光标显示状态
            Uint64 now = SDL_GetTicks();
            if (now - last_cursor_blink_time >= 500) {
                cursor_visible = !cursor_visible;
                cursor_visible_ = cursor_visible;  // 保存到成员变量供渲染使用
                last_cursor_blink_time = now;
                
                // 关键修复：标记输入框的 RenderObject 需要重绘
                // 这样增量渲染系统才会重绘光标区域
                if (auto render_obj = focus_element->GetRenderObject()) {
                    render_obj->MarkNeedsPaint();
                    render_obj->InvalidatePaintCache();
                }
                
                // 触发重绘以更新光标
                auto& wm = WindowManager::Instance();
                for (auto& window : wm.GetAllWindows()) {
                    window->SetNeedsRepaint();
                    // 关键修复：同时通知 RenderPipeline 需要重绘
                    // 否则 RenderPipeline::NeedsUpdate() 返回 false，导致快速路径跳过渲染
                    if (auto pipeline = window->GetRenderPipeline()) {
                        pipeline->MarkNeedsPaint();
                    }
                }
            }
        }
    } else {
        // 没有聚焦的输入框时，重置光标状态
        cursor_visible = true;
        cursor_visible_ = true;
    }

    // 5. 只在有窗口需要重绘时才渲染
    auto& wm = WindowManager::Instance();
    bool any_needs_repaint = false;
    for (auto& window : wm.GetAllWindows()) {
        if (window->NeedsRepaint()) {
            any_needs_repaint = true;
            break;
        }
    }

    if (any_needs_repaint) {
        Render();
    }

    // 5.5 检测 VSync 状态（只在第一次渲染后检测）
    if (!vsync_detected_ && total_frames_ > 5) {  // 改为5帧后检测，更快
        // 检查第一个窗口的 VSync 状态
        auto windows = wm.GetAllWindows();
        if (!windows.empty()) {
            auto& window = windows[0];
            
            // 关键修复：检查是否使用 CPU 渲染后端
            // CPU 模式下没有 VSync，必须使用帧率限制
            bool is_cpu_mode = (window->GetRenderBackend() == RenderBackend::CPU);
            
            if (is_cpu_mode) {
                // CPU 模式：强制使用帧率限制
                frame_controller_->SetUseVSync(false);
                vsync_detected_ = true;
                
                std::cout << "========================================" << std::endl;
                std::cout << "[EventLoop] ℹ CPU rendering mode detected" << std::endl;
                std::cout << "[EventLoop] → Using SDL_Delay for frame rate limiting (60 FPS)" << std::endl;
                std::cout << "========================================" << std::endl;
            } else {
                // GPU 模式：尝试查询 VSync 状态
                int swap_interval = 0;
                bool query_success = SDL_GL_GetSwapInterval(&swap_interval);
                
                bool vsync_enabled = query_success ? (swap_interval != 0) : true;
                
                frame_controller_->SetUseVSync(vsync_enabled);
                vsync_detected_ = true;
                
                std::cout << "========================================" << std::endl;
                if (query_success) {
                    if (vsync_enabled) {
                        std::cout << "[EventLoop] ✓ VSync ENABLED (swap_interval=" << swap_interval << ")" << std::endl;
                        std::cout << "[EventLoop] ✓ SDL_Delay DISABLED for maximum smoothness" << std::endl;
                        std::cout << "[EventLoop] ✓ Frame rate controlled by display refresh rate" << std::endl;
                    } else {
                        std::cout << "[EventLoop] ✗ VSync DISABLED (swap_interval=" << swap_interval << ")" << std::endl;
                        std::cout << "[EventLoop] → Using SDL_Delay for frame rate limiting" << std::endl;
                    }
                } else {
                    // 查询失败，使用默认假设
                    std::cout << "[EventLoop] ℹ VSync status query failed (OpenGL context issue)" << std::endl;
                    std::cout << "[EventLoop] ✓ Assuming VSync ENABLED (default configuration)" << std::endl;
                    std::cout << "[EventLoop] ✓ SDL_Delay DISABLED for maximum smoothness" << std::endl;
                }
                std::cout << "========================================" << std::endl;
            }
        }
    }
    total_frames_++;

    // 6. 检查是否应该退出
    auto& window_manager = WindowManager::Instance();
    if (!window_manager.HasWindows()) {
        should_quit_ = true;
    }

    // 7. 空闲处理
    if (!HasWork() && idle_callback_) {
        idle_callback_();
    }

    // 7.5 空闲时休眠以降低 CPU 占用
    // 当没有事件、没有任务、没有重绘需求时，休眠一小段时间
    // 这解决了 VSync 启用但没有渲染时的忙等待问题
    if (!has_events && !any_needs_repaint && !task_scheduler_->HasPendingTasks()) {
        SDL_Delay(1);  // 休眠 1ms，显著降低 CPU 占用同时保持响应性
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

std::shared_ptr<TaskScheduler> EventLoop::GetTaskSchedulerPtr() {
    return task_scheduler_;
}

bool EventLoop::ProcessEvents() {
    bool has_events = false;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        has_events = true;

        // 调试：输出事件类型（可以通过环境变量控制）
        static bool debug_events = std::getenv("LIGHTUI_DEBUG_EVENTS") != nullptr;
        if (debug_events) {
            // 过滤掉高频的鼠标移动事件
            if (event.type != SDL_EVENT_MOUSE_MOTION) {
                std::cout << "[EventLoop] SDL Event: type=" << event.type << std::endl;
            }
        }

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

        // 处理鼠标滚轮事件
        if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            HandleMouseWheelEventForDOM(event);
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

    // 渲染所有窗口
    auto& window_manager = WindowManager::Instance();
    for (auto& window : window_manager.GetAllWindows()) {
        if (window->NeedsRepaint()) {
            window->Render();
            window->SwapBuffers();
        }
    }
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

    // 将物理像素坐标转换为逻辑像素坐标（CSS 像素）
    float dpi_scale = window->GetDisplayScale();
    float logical_x = mouse_x / dpi_scale;
    float logical_y = mouse_y / dpi_scale;

    // ===== 处理 DevTools 鼠标事件 =====
    auto& devtools = DevToolsManager::GetInstance();
    if (devtools.IsOpen()) {
        // 获取窗口尺寸
        int win_width, win_height;
        SDL_GetWindowSize(window->GetSDLWindow(), &win_width, &win_height);
        float width = static_cast<float>(win_width) / dpi_scale;
        float height = static_cast<float>(win_height) / dpi_scale;
        
        // 获取 DevTools 面板区域
        float panel_x, panel_y, panel_width, panel_height;
        devtools.GetPanelBounds(width, height, panel_x, panel_y, panel_width, panel_height);
        
        // ===== 优先处理面板边界拖动 =====
        bool is_dragging_border = devtools.IsDraggingPanelBorder();
        bool on_panel_border = devtools.IsMouseOnPanelBorder(logical_x, logical_y, width, height);
        
        if (is_dragging_border || on_panel_border) {
            // 设置面板边界光标（根据停靠位置）- 对所有事件类型都设置
            if (devtools.GetDockPosition() == DockPosition::Bottom) {
                SetSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
            } else {
                SetSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
            }
            
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                devtools.HandlePanelBorderDrag(logical_x, logical_y, width, height, true);
                window->SetNeedsRepaint();
                return;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                devtools.HandlePanelBorderDrag(logical_x, logical_y, width, height, false);
                window->SetNeedsRepaint();
                return;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (is_dragging_border) {
                    if (devtools.UpdatePanelBorderDrag(logical_x, logical_y, width, height)) {
                        window->SetNeedsRepaint();
                    }
                }
                // 在边界上时直接返回，不再处理面板内部事件，避免光标闪烁
                return;
            }
        }
        
        // 检查是否正在拖动分隔线（即使鼠标不在面板区域内也要处理）
        bool is_dragging = devtools.IsDraggingSplitter();
        
        // 检查鼠标是否在 DevTools 面板区域内，或者正在拖动
        bool in_panel = (logical_x >= panel_x && logical_x < panel_x + panel_width &&
                         logical_y >= panel_y && logical_y < panel_y + panel_height);
        
        if (in_panel || is_dragging) {
            // 将事件传递给 DevTools
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                devtools.HandleMouseEvent(
                    static_cast<int>(logical_x - panel_x),
                    static_cast<int>(logical_y - panel_y),
                    event.button.button == SDL_BUTTON_LEFT ? 0 : 1,
                    true
                );
                window->SetNeedsRepaint();
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                devtools.HandleMouseEvent(
                    static_cast<int>(logical_x - panel_x),
                    static_cast<int>(logical_y - panel_y),
                    event.button.button == SDL_BUTTON_LEFT ? 0 : 1,
                    false
                );
                window->SetNeedsRepaint();
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // 处理鼠标移动事件（用于 BoxModel 悬停高亮和分隔线拖动）
                int rel_x = static_cast<int>(logical_x - panel_x);
                int rel_y = static_cast<int>(logical_y - panel_y);
                if (devtools.HandleMouseMove(rel_x, rel_y)) {
                    window->SetNeedsRepaint();
                }
                // 设置分隔线光标
                if (devtools.IsMouseOnSplitter(rel_x, rel_y) || devtools.IsDraggingSplitter()) {
                    SetSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
                } else {
                    SetSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
                }
            }
            if (in_panel) {
                return;  // 只有在面板区域内才消费事件
            }
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            // 鼠标不在面板区域内时，清除 Box Model 高亮
            devtools.ClearBoxModelHover();
            window->SetNeedsRepaint();
        }
        
        // ===== 处理元素选择器模式 =====
        if (devtools.IsPickerActive()) {
            // 确保渲染树已构建并且布局完成
            // 特别是在窗口最大化等尺寸变化后，必须确保使用最新的布局信息
            // 否则 hit testing 会使用旧的布局数据导致选择失败
            
            // 关键修复：先强制使渲染树失效，这样 Render() 才会重新构建和布局
            // 这解决了 Render() 内部静态变量导致的尺寸检查被跳过的问题
            window->InvalidateRenderTree();
            
            // 强制执行渲染以确保布局是最新的
            window->Render();
            
            // 再次确保渲染树有效（双重保险）
            window->EnsureRenderTree();
            auto root_render = window->GetCachedRenderTree();
            if (root_render) {
                // 获取主应用区域
                float app_x, app_y, app_width, app_height;
                devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);
                
                // 调试：输出关键信息
                std::cout << "[ElementPicker] window size: " << width << "x" << height 
                          << " dpi_scale=" << dpi_scale
                          << " app_bounds: (" << app_x << "," << app_y << "," << app_width << "," << app_height << ")"
                          << " mouse: (" << logical_x << "," << logical_y << ")"
                          << std::endl;
                
                // 检查是否在主应用区域内
                if (logical_x >= app_x && logical_x < app_x + app_width &&
                    logical_y >= app_y && logical_y < app_y + app_height) {
                    
                    // 将窗口绝对坐标转换为相对于主应用区域的坐标
                    // 这样 hit testing 才能正确命中元素，无论窗口是否最大化
                    float app_relative_x = logical_x - app_x;
                    float app_relative_y = logical_y - app_y;
                    
                    std::cout << "[ElementPicker] app_relative: (" << app_relative_x << "," << app_relative_y << ")" << std::endl;
                    
                    // 处理鼠标移动：更新悬停高亮
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        HitTestResult hit_result = hit_testing.HitTestRenderObject(
                            root_render, app_relative_x, app_relative_y, 0.0f, 0.0f);
                        
                        if (hit_result.IsValid() && hit_result.element) {
                            devtools.GetElementPicker()->SetHoverElement(hit_result.element, hit_result.render_object);
                            window->SetNeedsRepaint();
                        }
                    }
                    // 处理鼠标点击：选中元素
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && 
                             event.button.button == SDL_BUTTON_LEFT) {
                        HitTestResult hit_result = hit_testing.HitTestRenderObject(
                            root_render, app_relative_x, app_relative_y, 0.0f, 0.0f);
                        
                        if (hit_result.IsValid() && hit_result.element) {
                            // 选中元素并停止选择器模式
                            devtools.SelectElement(hit_result.element);
                            devtools.StopElementPicker();
                            window->SetNeedsRepaint();
                            return;  // 消费事件
                        }
                    }
                }
            }
        }
    }

    // ===== 处理滚动条拖动 =====
    // 如果正在拖动滚动条，处理拖动更新
    auto dragging_element = scrollbar_dragging_element_.lock();
    if (dragging_element && dragging_element->IsDraggingScrollbar()) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            // 获取旧的滚动位置
            float old_x = dragging_element->GetScrollX();
            float old_y = dragging_element->GetScrollY();

            // 更新滚动条位置 (这会更新 RenderObject 内部状态)
            dragging_element->UpdateScrollbarDrag(logical_x, logical_y);

            // 关键修复：在分层合成模式下，需要同步滚动位置到 compositor
            float new_x = dragging_element->GetScrollX();
            float new_y = dragging_element->GetScrollY();

            if (new_x != old_x || new_y != old_y) {
                // 如果滚动位置发生了变化，通知渲染管线
                // 这样 ScrollLayerManager 才能更新层偏移
                auto render_pipeline = window->GetRenderPipeline();
                if (render_pipeline) {
                    float delta_x = new_x - old_x;
                    float delta_y = new_y - old_y;
                    render_pipeline->HandleScroll(dragging_element.get(), delta_x, delta_y);
                }
            }

            window->SetNeedsRepaint();
            return;  // 拖动期间不处理其他鼠标事件
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            // 结束拖动
            dragging_element->EndScrollbarDrag();
            scrollbar_dragging_element_.reset();
            scrollbar_dragging_window_id_ = 0;
            window->SetNeedsRepaint();
            return;
        }
    }

    // ===== 处理 select 下拉菜单 =====
    auto& dropdown_manager = SelectDropdownManager::Instance();
    if (dropdown_manager.IsDropdownOpen()) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            // 更新悬停状态
            if (dropdown_manager.HandleMouseMove(logical_x, logical_y)) {
                window->SetNeedsRepaint();
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            // 处理点击
            if (dropdown_manager.HitTest(logical_x, logical_y)) {
                // 点击在下拉菜单内
                dropdown_manager.HandleClick(logical_x, logical_y);
                window->SetNeedsRepaint();
                return;  // 已处理，不继续
            } else {
                // 点击在下拉菜单外，关闭菜单
                dropdown_manager.CloseDropdown();
                window->SetNeedsRepaint();
                // 继续处理点击事件（可能点击了其他元素）
            }
        }
    }

    // 对于鼠标移动事件，只在位置真正改变时才更新 hover
    // 这可以显著减少不必要的处理
    static float last_mouse_x = -1, last_mouse_y = -1;
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        if (mouse_x == last_mouse_x && mouse_y == last_mouse_y) {
            return;  // 位置没变，跳过处理
        }
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
    }

    // ===== 性能优化：提前执行一次 Hit Testing，后续所有逻辑复用结果 =====
    // 使用缓存的渲染树进行 Hit Testing
    window->EnsureRenderTree();
    auto root_render = window->GetCachedRenderTree();

    HitTestResult hit_result;
    if (root_render) {
        // 先在 LayerManager 的 Overlay/Modal 层中测试
        auto& layer_manager = LayerManager::Instance();
        if (!layer_manager.HitTest(logical_x, logical_y, hit_result)) {
            // 如果 Overlay/Modal 层未命中，在 Base 层（渲染树）中测试
            hit_result = hit_testing.HitTestRenderObject(root_render, logical_x, logical_y, 0.0f, 0.0f);
        }
    }

    // 更新hover链（发送mouseover/mouseout事件并设置:hover伪类）
    // 使用优化版本，复用已计算的 HitTestResult
    UpdateHoverChainWithResult(window_id, mouse_x, mouse_y, hit_result);

    // 处理mousedown/mouseup和click/dblclick事件
    // 参考：W3C UI Events - dblclick事件需要在短时间内两次click同一元素
    static std::shared_ptr<Element> last_mousedown_element;
    static std::shared_ptr<Element> last_click_element;
    static Uint64 last_click_time = 0;
    static const Uint64 DOUBLE_CLICK_TIME_MS = 500;  // 500ms内的两次click算作dblclick

    // 注意：hit_result 已在上面计算，直接使用

    // ===== 更新鼠标光标样式 =====
    // 根据悬停元素更新系统光标（符合浏览器行为）
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        UpdateMouseCursor(hit_result, window_id);
    }

    // ===== 检测滚动条点击 =====
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT && root_render) {
        // 遍历渲染树找可滚动元素并检测滚动条
        std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>, float, float)> findScrollableAtPoint;
        findScrollableAtPoint = [&](std::shared_ptr<RenderObject> obj, float abs_x, float abs_y) -> std::shared_ptr<RenderObject> {
            const auto& layout = obj->GetLayoutInfo();
            float local_x = abs_x - layout.x;
            float local_y = abs_y - layout.y;

            // 检查点是否在元素范围内（使用有效可见尺寸，对于有滚动条的元素）
            float effective_width = obj->GetEffectiveVisibleWidth();
            float effective_height = obj->GetEffectiveVisibleHeight();
            // 对于非 body 元素，使用布局尺寸
            if (effective_width <= 0) effective_width = layout.width;
            if (effective_height <= 0) effective_height = layout.height;

            if (local_x >= 0 && local_x <= effective_width && local_y >= 0 && local_y <= effective_height) {
                const auto& style = obj->GetComputedStyle();
                // 检查 overflow, overflow-x, overflow-y 任一方向是否允许滚动
                std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
                std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
                bool allow_scroll = (overflow_x == "scroll" || overflow_x == "auto" ||
                                     overflow_y == "scroll" || overflow_y == "auto");
                if (allow_scroll) {
                    // 检测是否点击了滚动条
                    auto scrollbar_area = obj->HitTestScrollbar(local_x, local_y);
                    if (scrollbar_area != RenderObject::ScrollbarHitArea::None) {
                        return obj;
                    }
                }

                // 递归检查子元素（考虑父元素的滚动偏移）
                float child_offset_x = local_x + obj->GetScrollX();
                float child_offset_y = local_y + obj->GetScrollY();
                for (const auto& child : obj->GetChildren()) {
                    auto result = findScrollableAtPoint(child, child_offset_x, child_offset_y);
                    if (result) {
                        return result;
                    }
                }
            }
            return nullptr;
        };

        auto scrollable = findScrollableAtPoint(root_render, logical_x, logical_y);
        if (scrollable) {
            // 计算相对于元素的坐标
            // 需要累加所有祖先的偏移
            float elem_abs_x = 0, elem_abs_y = 0;
            std::vector<std::shared_ptr<RenderObject>> ancestors;
            auto current = scrollable;
            while (current) {
                ancestors.push_back(current);
                current = current->GetParent();
            }
            for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
                const auto& l = (*it)->GetLayoutInfo();
                elem_abs_x += l.x;
                elem_abs_y += l.y;
            }

            float local_x = logical_x - elem_abs_x;
            float local_y = logical_y - elem_abs_y;

            auto scrollbar_area = scrollable->HitTestScrollbar(local_x, local_y);
            if (scrollbar_area != RenderObject::ScrollbarHitArea::None) {
                // 开始拖动滚动条
                scrollable->StartScrollbarDrag(scrollbar_area, logical_x, logical_y);
                scrollbar_dragging_element_ = scrollable;
                scrollbar_dragging_window_id_ = window_id;
                window->SetNeedsRepaint();
                return;  // 滚动条点击不触发其他事件
            }
        }
    }

    // 如果没有命中任何元素
    if (!hit_result.IsValid()) {
        // 即使没有命中元素，mouseup时也要移除:active伪类
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && last_mousedown_element) {
            // 处理 textarea 的鼠标释放
            std::string tag_name = last_mousedown_element->GetTagName();
            if (tag_name == "textarea") {
                auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown_element);
                if (textarea_element) {
                    if (textarea_element->IsDraggingScrollbar()) {
                        textarea_element->EndScrollbarDrag();
                    }
                    if (textarea_element->IsDraggingSelection()) {
                        textarea_element->HandleMouseUp();
                    }
                }
            } else if (tag_name == "input") {
                auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown_element);
                if (input_element) {
                    // 结束 Range 拖动
                    if (input_element->IsDraggingRange()) {
                        input_element->EndRangeDrag();
                        window->SetNeedsRepaint();
                    }
                    // 结束文本选择拖动
                    else if (input_element->IsDraggingSelection()) {
                        HandleInputMouseInteraction(input_element, 0, event.type, 14.0f, "");
                    }
                }
            }

            last_mousedown_element->SetPseudoClass("active", false);
            last_mousedown_element = nullptr;

            // 结束拖拽
            if (event.button.button == SDL_BUTTON_LEFT) {
                drag_manager_->EndDrag(mouse_x, mouse_y);
            }
        }
        // 即使没有命中元素，mousemove 时也要处理正在进行的拖动选择
        else if (event.type == SDL_EVENT_MOUSE_MOTION && last_mousedown_element) {
            std::string tag_name = last_mousedown_element->GetTagName();
            if (tag_name == "textarea") {
                auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown_element);
                if (textarea_element && (textarea_element->IsDraggingSelection() || textarea_element->IsDraggingScrollbar())) {
                    // 查找 textarea 的渲染对象并计算绝对位置
                    auto root_render = window->GetCachedRenderTree();
                    if (root_render) {
                        struct FindResult {
                            std::shared_ptr<RenderObject> render_obj;
                            float abs_x = 0;
                            float abs_y = 0;
                        };
                        std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                        findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                            if (!obj) return {};
                            const auto& layout = obj->GetLayoutInfo();
                            float current_x = offset_x + layout.x;
                            float current_y = offset_y + layout.y;

                            auto node = obj->GetNode();
                            if (node && std::dynamic_pointer_cast<HTMLTextAreaElement>(node) == textarea_element) {
                                return {obj, current_x, current_y};
                            }
                            // 子元素需要考虑滚动偏移
                            float child_offset_x = current_x - obj->GetScrollX();
                            float child_offset_y = current_y - obj->GetScrollY();
                            for (auto& child : obj->GetChildren()) {
                                auto result = findRenderObj(child, child_offset_x, child_offset_y);
                                if (result.render_obj) return result;
                            }
                            return {};
                        };

                        auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                        if (find_result.render_obj) {
                            const auto& layout = find_result.render_obj->GetLayoutInfo();
                            const auto& style = find_result.render_obj->GetComputedStyle();
                            float padding_left = style.padding.left.ToPx();
                            float padding_top = style.padding.top.ToPx();
                            float padding_right = style.padding.right.ToPx();
                            float padding_bottom = style.padding.bottom.ToPx();

                            // 计算是否需要滚动条（与渲染保持一致）
                            const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
                            SkFont font;
                            font.setSize(style.font_size);
                            float line_height = style.font_size * 1.2f;
                            float content_height = textarea_element->GetContentHeight(line_height);
                            float max_line_width = textarea_element->GetMaxLineWidth(font);
                            float base_visible_width = layout.width - padding_left - padding_right;
                            float base_visible_height = layout.height - padding_top - padding_bottom;
                            bool need_v_scrollbar = content_height > base_visible_height;
                            bool need_h_scrollbar = max_line_width > base_visible_width;
                            float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                            float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

                            // 使用绝对位置计算 local 坐标
                            float text_local_x = logical_x - find_result.abs_x - padding_left;
                            float text_local_y = logical_y - find_result.abs_y - padding_top;
                            HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                                           style.font_size, style.font_family, false,
                                                           visible_width, visible_height);
                        }
                    }
                }
            } else if (tag_name == "input") {
                auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown_element);
                if (input_element) {
                    // 处理 Range 滑块拖动
                    if (input_element->IsDraggingRange()) {
                        auto root_render = window->GetCachedRenderTree();
                        if (root_render) {
                            struct FindResult {
                                std::shared_ptr<RenderObject> render_obj;
                                float abs_x = 0;
                                float abs_y = 0;
                            };
                            std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                            findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                                if (!obj) return {};
                                const auto& layout = obj->GetLayoutInfo();
                                float current_x = offset_x + layout.x;
                                float current_y = offset_y + layout.y;

                                auto node = obj->GetNode();
                                if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
                                    return {obj, current_x, current_y};
                                }
                                float child_offset_x = current_x - obj->GetScrollX();
                                float child_offset_y = current_y - obj->GetScrollY();
                                for (auto& child : obj->GetChildren()) {
                                    auto result = findRenderObj(child, child_offset_x, child_offset_y);
                                    if (result.render_obj) return result;
                                }
                                return {};
                            };

                            auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                            if (find_result.render_obj) {
                                const auto& layout = find_result.render_obj->GetLayoutInfo();
                                float local_x = logical_x - find_result.abs_x;
                                input_element->UpdateRangeDrag(local_x, layout.width);
                                window->SetNeedsRepaint();
                            }
                        }
                    }
                    // 处理文本选择拖动
                    else if (input_element->IsDraggingSelection()) {
                        auto root_render = window->GetCachedRenderTree();
                        if (root_render) {
                            struct FindResult {
                                std::shared_ptr<RenderObject> render_obj;
                                float abs_x = 0;
                                float abs_y = 0;
                            };
                            std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                            findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                                if (!obj) return {};
                                const auto& layout = obj->GetLayoutInfo();
                                float current_x = offset_x + layout.x;
                                float current_y = offset_y + layout.y;

                                auto node = obj->GetNode();
                                if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
                                    return {obj, current_x, current_y};
                                }
                                float child_offset_x = current_x - obj->GetScrollX();
                                float child_offset_y = current_y - obj->GetScrollY();
                                for (auto& child : obj->GetChildren()) {
                                    auto result = findRenderObj(child, child_offset_x, child_offset_y);
                                    if (result.render_obj) return result;
                                }
                                return {};
                            };

                            auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                            if (find_result.render_obj) {
                                const auto& style = find_result.render_obj->GetComputedStyle();
                                float padding_left = style.padding.left.ToPx();
                                float text_local_x = logical_x - find_result.abs_x - padding_left;
                                HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                           style.font_size, style.font_family);
                            }
                        }
                    }
                }
            }
        }

        return;
    }

    // 处理 Range 滑块拖动（即使命中了元素，也要优先处理正在进行的拖动）
    if (event.type == SDL_EVENT_MOUSE_MOTION && last_mousedown_element) {
        std::string tag_name = last_mousedown_element->GetTagName();
        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown_element);
            if (input_element && input_element->IsDraggingRange()) {
                auto root_render = window->GetCachedRenderTree();
                if (root_render) {
                    struct FindResult {
                        std::shared_ptr<RenderObject> render_obj;
                        float abs_x = 0;
                        float abs_y = 0;
                    };
                    std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                    findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                        if (!obj) return {};
                        const auto& layout = obj->GetLayoutInfo();
                        float current_x = offset_x + layout.x;
                        float current_y = offset_y + layout.y;

                        auto node = obj->GetNode();
                        if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
                            return {obj, current_x, current_y};
                        }
                        float child_offset_x = current_x - obj->GetScrollX();
                        float child_offset_y = current_y - obj->GetScrollY();
                        for (auto& child : obj->GetChildren()) {
                            auto result = findRenderObj(child, child_offset_x, child_offset_y);
                            if (result.render_obj) return result;
                        }
                        return {};
                    };

                    auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                    if (find_result.render_obj) {
                        const auto& layout = find_result.render_obj->GetLayoutInfo();
                        float local_x = logical_x - find_result.abs_x;
                        input_element->UpdateRangeDrag(local_x, layout.width);
                        window->SetNeedsRepaint();
                    }
                }
                // Range 拖动已处理，不需要继续处理 mousemove
                return;
            }
        }
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

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        last_mousedown_element = hit_result.element;
        // mousedown时设置:active伪类
        hit_result.element->SetPseudoClass("active", true);

        // 对于输入元素（input, textarea），在 mousedown 时立即设置焦点
        // 这样可以立即启用 SDL 文本输入
        std::string tag_name = hit_result.element->GetTagName();
        if (tag_name == "input" || tag_name == "textarea") {
            focus_manager_->SetWindow(window.get());
            focus_manager_->SetFocus(hit_result.element, false);

            // 处理输入框的鼠标点击定位光标
            if (tag_name == "input" && event.button.button == SDL_BUTTON_LEFT) {
                auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(hit_result.element);
                if (input_element && hit_result.render_object) {
                    // 获取渲染对象的样式信息
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    const auto& layout = hit_result.render_object->GetLayoutInfo();
                    float padding_left = style.padding.left.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float border_width = style.border.width.ToPx();

                    // 检查是否是 input[number] 并且点击在 spinner 区域
                    if (input_element->GetInputType() == InputType::Number) {
                        const float spinner_width = 16.0f;
                        float content_width = layout.width - padding_left - padding_right - border_width * 2;
                        float spinner_x = padding_left + border_width + content_width - spinner_width;

                        if (hit_result.local_x >= spinner_x) {
                            // 点击在 spinner 区域
                            float content_height = layout.height - style.padding.top.ToPx() - style.padding.bottom.ToPx() - border_width * 2;
                            float half_height = content_height / 2;
                            float spinner_y = style.padding.top.ToPx() + border_width;

                            if (hit_result.local_y < spinner_y + half_height) {
                                // 点击上半部分 - 增加
                                input_element->StepUp();
                            } else {
                                // 点击下半部分 - 减少
                                input_element->StepDown();
                            }
                            // 不处理文本点击 - spinner 区域已处理完毕
                        } else {
                            // 计算相对于文本内容区域的 X 坐标
                            float text_local_x = hit_result.local_x - padding_left;
                            HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                       style.font_size, style.font_family);
                        }
                    } else if (input_element->GetInputType() == InputType::Range) {
                        // Range 滑块 - 开始拖动
                        float content_width = layout.width;  // range 没有 padding/border
                        input_element->StartRangeDrag(content_width);
                        input_element->UpdateRangeDrag(hit_result.local_x, content_width);
                        window->SetNeedsRepaint();
                    } else {
                        // 计算相对于文本内容区域的 X 坐标
                        float text_local_x = hit_result.local_x - padding_left;
                        HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                   style.font_size, style.font_family);
                    }
                }
            }
            // 处理 textarea 的鼠标点击定位光标
            else if (tag_name == "textarea" && event.button.button == SDL_BUTTON_LEFT) {
                auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(hit_result.element);
                if (textarea_element && hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    float padding_left = style.padding.left.ToPx();
                    float padding_top = style.padding.top.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float padding_bottom = style.padding.bottom.ToPx();
                    const auto& layout = hit_result.render_object->GetLayoutInfo();

                    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;

                    // 计算内容尺寸以确定是否需要滚动条
                    FontDescriptor desc;
                    desc.family = style.font_family.empty() ? "sans-serif" : style.font_family;
                    desc.size = style.font_size > 0 ? style.font_size : 14.0f;
                    SkFont font = FontManager::GetInstance().LoadFont(desc);
                    SkFontMetrics fm;
                    font.getMetrics(&fm);
                    float line_height = -fm.fAscent + fm.fDescent;
                    if (fm.fLeading > 0) line_height += fm.fLeading;
                    else line_height += desc.size * 0.2f;

                    float content_height = textarea_element->GetContentHeight(line_height);
                    float max_line_width = textarea_element->GetMaxLineWidth(font);
                    float base_visible_width = layout.width - padding_left - padding_right;
                    float base_visible_height = layout.height - padding_top - padding_bottom;
                    bool need_v_scrollbar = content_height > base_visible_height;
                    bool need_h_scrollbar = max_line_width > base_visible_width;

                    // 检查是否点击在滚动条上
                    float local_x = hit_result.local_x;
                    float local_y = hit_result.local_y;

                    // 垂直滚动条区域
                    if (need_v_scrollbar) {
                        float v_scrollbar_x = layout.width - scrollbar_width - 2;
                        if (local_x >= v_scrollbar_x && local_x <= layout.width) {
                            // 点击在垂直滚动条上
                            float track_y = padding_top;
                            float track_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);
                            textarea_element->StartScrollbarDrag(
                                HTMLTextAreaElement::ScrollbarType::VERTICAL,
                                local_y - track_y
                            );
                            // 不处理文本点击
                            goto skip_text_interaction;
                        }
                    }

                    // 水平滚动条区域
                    if (need_h_scrollbar) {
                        float h_scrollbar_y = layout.height - scrollbar_width - 2;
                        if (local_y >= h_scrollbar_y && local_y <= layout.height) {
                            // 点击在水平滚动条上
                            float track_x = padding_left;
                            float track_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                            textarea_element->StartScrollbarDrag(
                                HTMLTextAreaElement::ScrollbarType::HORIZONTAL,
                                local_x - track_x
                            );
                            // 不处理文本点击
                            goto skip_text_interaction;
                        }
                    }

                    {
                        // 正常的文本区域点击
                        float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                        float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);
                        float text_local_x = hit_result.local_x - padding_left;
                        float text_local_y = hit_result.local_y - padding_top;
                        // 检查是否按住 Shift 键
                        SDL_Keymod mod_state = SDL_GetModState();
                        bool shift_key = (mod_state & SDL_KMOD_SHIFT) != 0;
                        HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                                       style.font_size, style.font_family, shift_key,
                                                       visible_width, visible_height);
                    }
                    skip_text_interaction:;
                }
            }
        }
        // 对于其他可聚焦元素（button 等），不在 mousedown 时设置焦点
        // 焦点将在 click 事件后设置，避免 focus 事件触发 Preact 重渲染导致元素被替换
        // 如果点击的是非可聚焦元素，清除当前焦点
        else if (!focus_manager_->IsFocusable(hit_result.element)) {
            focus_manager_->ClearFocus();
        }

        // 拖拽检测（参考RmlUi/Source/Core/Context.cpp - ProcessMouseButtonDown）
        // 只在左键按下时检测拖拽
        if (event.button.button == SDL_BUTTON_LEFT) {
            drag_manager_->StartDragDetection(hit_result.element, mouse_x, mouse_y);
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        // mouseup时移除:active伪类
        if (last_mousedown_element) {
            last_mousedown_element->SetPseudoClass("active", false);
        }

        // 处理输入框的鼠标释放（结束拖动选择）
        if (last_mousedown_element && event.button.button == SDL_BUTTON_LEFT) {
            std::string tag_name = last_mousedown_element->GetTagName();
            if (tag_name == "input") {
                auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown_element);
                if (input_element) {
                    // 结束 Range 拖动
                    if (input_element->IsDraggingRange()) {
                        input_element->EndRangeDrag();
                        window->SetNeedsRepaint();
                    }
                    // 结束文本选择拖动
                    else if (input_element->IsDraggingSelection()) {
                        // mouse up 时只需要结束拖动，不需要计算字符位置
                        HandleInputMouseInteraction(input_element, 0, event.type, 14.0f, "");
                    }
                }
            } else if (tag_name == "textarea") {
                auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown_element);
                if (textarea_element) {
                    // 结束滚动条拖动
                    if (textarea_element->IsDraggingScrollbar()) {
                        textarea_element->EndScrollbarDrag();
                    }
                    // 结束文本选择拖动
                    if (textarea_element->IsDraggingSelection()) {
                        textarea_element->HandleMouseUp();
                    }
                }
            }
        }

        if (last_mousedown_element == hit_result.element) {
            // 在同一个元素上 mousedown 和 mouseup，触发 click
            auto click_event = std::make_shared<MouseEvent>(
                "click",
                static_cast<int>(mouse_x),
                static_cast<int>(mouse_y),
                button
            );
            hit_result.element->DispatchEvent(click_event);

            // 对于非输入元素（button 等），在 click 事件后设置焦点
            // 这样可以确保 click 事件正确触发，避免 focus 事件导致 Preact 重渲染
            std::string tag_name = hit_result.element->GetTagName();
            if (tag_name != "input" && tag_name != "textarea") {
                focus_manager_->SetWindow(window.get());
                bool focus_set = focus_manager_->SetFocus(hit_result.element, false);
                if (!focus_set && !focus_manager_->IsFocusable(hit_result.element)) {
                    // 如果点击的是非可聚焦元素，清除当前焦点
                    focus_manager_->ClearFocus();
                }
            }

            // 处理表单元素的默认行为（参考 RmlUi InputTypeCheckbox::ProcessDefaultAction）
            ProcessFormElementDefaultAction(hit_result.element, hit_result);

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
        // 处理输入框的拖动选择
        if (last_mousedown_element) {
            std::string tag_name = last_mousedown_element->GetTagName();
            if (tag_name == "input") {
                auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(last_mousedown_element);
                if (input_element && input_element->IsDraggingSelection()) {
                    // 查找输入框对应的渲染对象
                    // 通过遍历渲染树或重新进行 hit testing 来获取
                    if (hit_result.element == last_mousedown_element && hit_result.render_object) {
                        // 如果鼠标仍在输入框上，使用 hit_result
                        const auto& style = hit_result.render_object->GetComputedStyle();
                        float padding_left = style.padding.left.ToPx();
                        float text_local_x = hit_result.local_x - padding_left;
                        HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                   style.font_size, style.font_family);
                    } else {
                        // 鼠标移出了输入框，但仍在拖动选择
                        // 使用输入框的布局信息计算相对坐标
                        // 需要从窗口缓存的渲染树中查找输入框的渲染对象
                        auto root_render = window->GetCachedRenderTree();
                        if (root_render) {
                            // 递归查找输入框对应的渲染对象
                            std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> findRenderObj;
                            findRenderObj = [&](std::shared_ptr<RenderObject> obj) -> std::shared_ptr<RenderObject> {
                                if (!obj) return nullptr;
                                auto node = obj->GetNode();
                                if (node && std::dynamic_pointer_cast<HTMLInputElement>(node) == input_element) {
                                    return obj;
                                }
                                for (auto& child : obj->GetChildren()) {
                                    auto result = findRenderObj(child);
                                    if (result) return result;
                                }
                                return nullptr;
                            };

                            auto input_render = findRenderObj(root_render);
                            if (input_render) {
                                const auto& layout = input_render->GetLayoutInfo();
                                const auto& style = input_render->GetComputedStyle();
                                float padding_left = style.padding.left.ToPx();
                                float text_local_x = logical_x - layout.x - padding_left;
                                HandleInputMouseInteraction(input_element, text_local_x, event.type,
                                                           style.font_size, style.font_family);
                            }
                        }
                    }
                }
            } else if (tag_name == "textarea") {
                auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(last_mousedown_element);
                if (textarea_element) {
                    // 处理滚动条拖动
                    if (textarea_element->IsDraggingScrollbar()) {
                        auto root_render = window->GetCachedRenderTree();
                        if (root_render) {
                            std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> findRenderObj;
                            findRenderObj = [&](std::shared_ptr<RenderObject> obj) -> std::shared_ptr<RenderObject> {
                                if (!obj) return nullptr;
                                auto node = obj->GetNode();
                                if (node && std::dynamic_pointer_cast<HTMLTextAreaElement>(node) == textarea_element) {
                                    return obj;
                                }
                                for (auto& child : obj->GetChildren()) {
                                    auto result = findRenderObj(child);
                                    if (result) return result;
                                }
                                return nullptr;
                            };

                            auto textarea_render = findRenderObj(root_render);
                            if (textarea_render) {
                                const auto& layout = textarea_render->GetLayoutInfo();
                                const auto& style = textarea_render->GetComputedStyle();
                                float padding_left = style.padding.left.ToPx();
                                float padding_top = style.padding.top.ToPx();
                                float padding_right = style.padding.right.ToPx();
                                float padding_bottom = style.padding.bottom.ToPx();

                                const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;

                                FontDescriptor desc;
                                desc.family = style.font_family.empty() ? "sans-serif" : style.font_family;
                                desc.size = style.font_size > 0 ? style.font_size : 14.0f;
                                SkFont font = FontManager::GetInstance().LoadFont(desc);
                                SkFontMetrics fm;
                                font.getMetrics(&fm);
                                float line_height = -fm.fAscent + fm.fDescent;
                                if (fm.fLeading > 0) line_height += fm.fLeading;
                                else line_height += desc.size * 0.2f;

                                float content_height = textarea_element->GetContentHeight(line_height);
                                float max_line_width = textarea_element->GetMaxLineWidth(font);
                                float base_visible_width = layout.width - padding_left - padding_right;
                                float base_visible_height = layout.height - padding_top - padding_bottom;
                                bool need_v_scrollbar = content_height > base_visible_height;
                                bool need_h_scrollbar = max_line_width > base_visible_width;

                                float local_x = logical_x - layout.x;
                                float local_y = logical_y - layout.y;

                                if (textarea_element->GetDraggingScrollbarType() == HTMLTextAreaElement::ScrollbarType::VERTICAL) {
                                    float track_y = padding_top;
                                    float track_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);
                                    float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);
                                    textarea_element->UpdateScrollbarDrag(
                                        local_y - track_y,
                                        track_height,
                                        content_height,
                                        visible_height
                                    );
                                } else {
                                    float track_x = padding_left;
                                    float track_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                                    float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                                    textarea_element->UpdateScrollbarDrag(
                                        local_x - track_x,
                                        track_width,
                                        max_line_width,
                                        visible_width
                                    );
                                }
                            }
                        }
                    }
                    // 处理文本选择拖动
                    else if (textarea_element->IsDraggingSelection()) {
                        if (hit_result.element == last_mousedown_element && hit_result.render_object) {
                            std::cout << "[DEBUG-A] hit_result.element == last_mousedown_element" << std::endl;
                            const auto& style = hit_result.render_object->GetComputedStyle();
                            const auto& layout = hit_result.render_object->GetLayoutInfo();
                            float padding_left = style.padding.left.ToPx();
                            float padding_top = style.padding.top.ToPx();
                            float padding_right = style.padding.right.ToPx();
                            float padding_bottom = style.padding.bottom.ToPx();

                            // 计算是否需要滚动条（与渲染保持一致）
                            const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
                            SkFont font;
                            font.setSize(style.font_size);
                            float line_height = style.font_size * 1.2f;
                            float content_height = textarea_element->GetContentHeight(line_height);
                            float max_line_width = textarea_element->GetMaxLineWidth(font);
                            float base_visible_width = layout.width - padding_left - padding_right;
                            float base_visible_height = layout.height - padding_top - padding_bottom;
                            bool need_v_scrollbar = content_height > base_visible_height;
                            bool need_h_scrollbar = max_line_width > base_visible_width;
                            float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                            float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

                            float text_local_x = hit_result.local_x - padding_left;
                            float text_local_y = hit_result.local_y - padding_top;
                            HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                                           style.font_size, style.font_family, false,
                                                           visible_width, visible_height);
                        } else {
                            // 鼠标移出 textarea，需要找到 textarea 的绝对位置
                            auto root_render = window->GetCachedRenderTree();
                            if (root_render) {
                                // 查找 textarea 的渲染对象并计算绝对位置
                                struct FindResult {
                                    std::shared_ptr<RenderObject> render_obj;
                                    float abs_x = 0;
                                    float abs_y = 0;
                                };
                                std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
                                findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
                                    if (!obj) return {};
                                    const auto& layout = obj->GetLayoutInfo();
                                    float current_x = offset_x + layout.x;
                                    float current_y = offset_y + layout.y;

                                    auto node = obj->GetNode();
                                    if (node && std::dynamic_pointer_cast<HTMLTextAreaElement>(node) == textarea_element) {
                                        return {obj, current_x, current_y};
                                    }
                                    // 子元素需要考虑滚动偏移
                                    float child_offset_x = current_x - obj->GetScrollX();
                                    float child_offset_y = current_y - obj->GetScrollY();
                                    for (auto& child : obj->GetChildren()) {
                                        auto result = findRenderObj(child, child_offset_x, child_offset_y);
                                        if (result.render_obj) return result;
                                    }
                                    return {};
                                };

                                auto find_result = findRenderObj(root_render, 0.0f, 0.0f);
                                if (find_result.render_obj) {
                                    const auto& layout = find_result.render_obj->GetLayoutInfo();
                                    const auto& style = find_result.render_obj->GetComputedStyle();
                                    float padding_left = style.padding.left.ToPx();
                                    float padding_top = style.padding.top.ToPx();
                                    float padding_right = style.padding.right.ToPx();
                                    float padding_bottom = style.padding.bottom.ToPx();

                                    // 计算是否需要滚动条（与渲染保持一致）
                                    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
                                    SkFont font;
                                    font.setSize(style.font_size);
                                    float line_height = style.font_size * 1.2f;
                                    float content_height = textarea_element->GetContentHeight(line_height);
                                    float max_line_width = textarea_element->GetMaxLineWidth(font);
                                    float base_visible_width = layout.width - padding_left - padding_right;
                                    float base_visible_height = layout.height - padding_top - padding_bottom;
                                    bool need_v_scrollbar = content_height > base_visible_height;
                                    bool need_h_scrollbar = max_line_width > base_visible_width;
                                    float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);
                                    float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);

                                    // 使用绝对位置计算 local 坐标
                                    float text_local_x = logical_x - find_result.abs_x - padding_left;
                                    float text_local_y = logical_y - find_result.abs_y - padding_top;
                                    HandleTextAreaMouseInteraction(textarea_element, text_local_x, text_local_y, event.type,
                                                                   style.font_size, style.font_family, false,
                                                                   visible_width, visible_height);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 更新拖拽状态（参考RmlUi/Source/Core/Context.cpp - ProcessMouseMove）
        // 支持拖拽阈值检测：在 Detecting 状态时也需要调用 UpdateDrag
        if (drag_manager_->IsDragging() || drag_manager_->IsDetecting()) {
            drag_manager_->UpdateDrag(logical_x, logical_y, document, root_render);
        }
    }
}

void EventLoop::ProcessFormElementDefaultAction(std::shared_ptr<Element> element, const HitTestResult& hit_result) {
    if (!element) {
        return;
    }

    std::string tag_name = element->GetTagName();

    // 处理 input 元素（参考 RmlUi InputTypeCheckbox::ProcessDefaultAction）
    if (tag_name == "input") {
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (!input_element) {
            return;
        }

        // 如果禁用，不处理
        if (input_element->IsDisabled()) {
            return;
        }

        InputType type = input_element->GetInputType();

        // Checkbox: 切换 checked 状态
        if (type == InputType::Checkbox) {
            bool checked = input_element->GetChecked();
            input_element->SetChecked(!checked, true);  // trigger_events=true

            // 标记窗口需要重绘
            auto& window_manager = WindowManager::Instance();
            for (auto& window : window_manager.GetAllWindows()) {
                window->SetNeedsRepaint();
            }
        }
        // Radio: 选中（不能取消选中）
        else if (type == InputType::Radio) {
            if (!input_element->GetChecked()) {
                // 取消同组其他 radio 的选中状态
                std::string name = input_element->GetAttribute("name");
                if (!name.empty()) {
                    // 查找同组的 radio
                    auto document = element->GetOwnerDocument();
                    if (document) {
                        auto body = document->GetBody();
                        if (body) {
                            UncheckRadioGroup(body, name, input_element);
                        }
                    }
                }

                input_element->SetChecked(true, true);  // trigger_events=true

                // 标记窗口需要重绘
                auto& window_manager = WindowManager::Instance();
                for (auto& window : window_manager.GetAllWindows()) {
                    window->SetNeedsRepaint();
                }
            }
        }
        // Submit 按钮: 提交表单
        else if (type == InputType::Submit) {
            // 查找关联的表单并提交
            auto form = FindParentForm(element);
            if (form) {
                form->Submit();
            }
        }
    }
    // 处理 button 元素
    else if (tag_name == "button") {
        auto button_element = std::dynamic_pointer_cast<HTMLButtonElement>(element);
        if (button_element) {
            // 如果禁用，不处理
            if (button_element->GetDisabled()) {
                return;
            }

            std::string button_type = button_element->GetAttribute("type");
            // 默认 type 是 "submit"
            if (button_type.empty()) {
                button_type = "submit";
            }

            if (button_type == "submit") {
                // 查找关联的表单并提交
                auto form = FindParentForm(element);
                if (form) {
                    form->Submit();
                }
            } else if (button_type == "reset") {
                // 查找关联的表单并重置
                auto form = FindParentForm(element);
                if (form) {
                    form->Reset();
                }
            }
            // type="button" 不执行任何默认操作
        }
    }
    // 处理 select 元素
    else if (tag_name == "select") {
        auto select_element = std::dynamic_pointer_cast<HTMLSelectElement>(element);
        if (select_element) {
            // 如果禁用，不处理
            if (select_element->GetDisabled()) {
                return;
            }

            auto& dropdown_manager = SelectDropdownManager::Instance();

            if (select_element->IsDropdownOpen()) {
                // 关闭下拉菜单
                dropdown_manager.CloseDropdown();
            } else {
                // 打开下拉菜单
                // 使用 hit_result 中的渲染对象获取位置
                auto render_obj = hit_result.render_object;
                if (render_obj) {
                    // 计算绝对位置（需要从当前元素向上累加，同时考虑滚动偏移）
                    float abs_x = 0, abs_y = 0;
                    auto current = render_obj;
                    while (current) {
                        const auto& layout = current->GetLayoutInfo();
                        abs_x += layout.x;
                        abs_y += layout.y;

                        // 减去父元素的滚动偏移
                        auto parent = current->GetParent();
                        if (parent) {
                            abs_x -= parent->GetScrollX();
                            abs_y -= parent->GetScrollY();
                        }

                        current = parent;
                    }

                    const auto& layout = render_obj->GetLayoutInfo();
                    SkRect trigger_rect = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);

                    select_element->SetDropdownOpen(true);
                    dropdown_manager.OpenDropdown(select_element, trigger_rect);
                }
            }

            // 标记窗口需要重绘
            auto& window_manager = WindowManager::Instance();
            for (auto& window : window_manager.GetAllWindows()) {
                window->SetNeedsRepaint();
            }
        }
    }
}

std::shared_ptr<HTMLFormElement> EventLoop::FindParentForm(std::shared_ptr<Element> element) {
    if (!element) {
        return nullptr;
    }

    auto parent = element->GetParentNode();
    while (parent) {
        auto parent_element = std::dynamic_pointer_cast<Element>(parent);
        if (parent_element && parent_element->GetTagName() == "form") {
            return std::dynamic_pointer_cast<HTMLFormElement>(parent_element);
        }
        parent = parent->GetParentNode();
    }
    return nullptr;
}

void EventLoop::UncheckRadioGroup(const std::shared_ptr<Node>& node, const std::string& group_name, const std::shared_ptr<HTMLInputElement>& except) {
    if (!node) {
        return;
    }

    // 检查当前节点
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element && element->GetTagName() == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input && input != except &&
            input->GetInputType() == InputType::Radio &&
            input->GetAttribute("name") == group_name) {
            input->SetChecked(false, false);  // 不触发事件
        }
    }

    // 递归处理子节点
    auto children = node->GetChildNodes();
    for (auto& child : children) {
        UncheckRadioGroup(child, group_name, except);
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
    // 向后兼容版本：内部执行 Hit Testing 然后调用优化版本
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

    // 使用缓存的渲染树进行 Hit Testing
    window->EnsureRenderTree();
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) {
        return;
    }

    // 将物理像素坐标转换为逻辑像素坐标
    float dpi_scale = window->GetDisplayScale();
    float logical_x = mouse_x / dpi_scale;
    float logical_y = mouse_y / dpi_scale;

    HitTesting hit_testing;
    HitTestResult hit_result = hit_testing.HitTestRenderObject(root_render, logical_x, logical_y, 0.0f, 0.0f);

    // 调用优化版本
    UpdateHoverChainWithResult(window_id, mouse_x, mouse_y, hit_result);
}

void EventLoop::UpdateHoverChainWithResult(Uint32 window_id, float mouse_x, float mouse_y, const HitTestResult& hit_result) {
    // 性能优化版本：复用外部已计算的 Hit Testing 结果
    // 避免在 HandleMouseEventForDOM 中重复执行 Hit Testing

    // 获取窗口（用于触发重绘）
    auto& window_manager = WindowManager::Instance();
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }

    // 获取新的 hover 目标元素
    std::shared_ptr<Element> new_hover = hit_result.IsValid() ? hit_result.element : nullptr;
    std::shared_ptr<Element> old_hover = hover_element_.lock();

    // 快速路径：如果 hover 元素没有变化，直接返回
    if (new_hover == old_hover) {
        return;  // 无需任何处理
    }

    // 构建新的hover链（从目标元素到根元素）
    std::vector<std::weak_ptr<Element>> new_hover_chain;

    if (new_hover) {
        // 从目标元素向上遍历到根元素
        auto current = new_hover;
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
    bool changed = SendEvents(hover_chain_, new_hover_chain, "mouseout", mouse_x, mouse_y);

    // 发送mouseover事件到进入的元素（在新链中但不在旧链中）
    changed |= SendEvents(new_hover_chain, hover_chain_, "mouseover", mouse_x, mouse_y);

    // 如果有伪类变化，触发重绘
    if (changed) {
        window->SetNeedsRepaint();
    }

    // 发送mouseleave/mouseenter事件（不冒泡版本）
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

    // 更新hover链
    hover_chain_ = std::move(new_hover_chain);
    hover_element_ = new_hover;
}

bool EventLoop::SendEvents(const std::vector<std::weak_ptr<Element>>& old_items,
                          const std::vector<std::weak_ptr<Element>>& new_items,
                          const std::string& event_type,
                          float mouse_x,
                          float mouse_y) {
    // 参考：RmlUi/Source/Core/Context.cpp - SendEvents
    // 找出在old_items中但不在new_items中的元素
    // 返回是否有视觉变化（需要重绘）
    //
    // 优化策略：
    // 1. 保留 hover 伪类传播给所有祖先（符合 CSS 规范）
    // 2. 但只对有视觉变化的元素返回 has_changes = true
    // 3. 这样可以避免对 div, span 等无 hover 样式的元素触发重绘

    bool has_changes = false;

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

            // 分发事件（符合 W3C 规范，所有祖先都应收到事件）
            element->DispatchEvent(mouse_event);

            // 检查元素当前的 hover 状态，避免重复设置
            bool current_hover = element->HasPseudoClass("hover");
            bool new_hover_state = (event_type == "mouseover");

            // 只有状态真正变化时才处理
            if (current_hover != new_hover_state) {
                // 设置/移除 :hover 伪类（符合 CSS 规范，所有祖先都应有 hover 状态）
                element->SetPseudoClass("hover", new_hover_state);

                // 只有具有内置 hover 样式的元素才需要触发重绘
                // 这些元素在 StyleResolver::ApplyPseudoClassStyles 中有特殊处理
                std::string tag = element->GetTagName();
                if (tag == "button" || tag == "a" || tag == "input" ||
                    tag == "textarea" || tag == "select") {
                    has_changes = true;
                }
                // TODO: 未来可以检查 CSS 规则是否匹配该元素的 :hover 选择器
                // 例如 .container:hover { background: red; }
            }
        }
    }

    return has_changes;
}

void EventLoop::HandleKeyboardEventForDOM(const SDL_Event& event) {
    // 参考：W3C UI Events - KeyboardEvent
    // 参考：RmlUi/Source/Core/Context.cpp - ProcessKeyDown, ProcessKeyUp

    // 首先检查 DevTools 快捷键
    if (event.type == SDL_EVENT_KEY_DOWN) {
        SDL_Keymod mod = SDL_GetModState();
        bool ctrl_key = (mod & SDL_KMOD_CTRL) != 0;
        bool shift_key = (mod & SDL_KMOD_SHIFT) != 0;
        bool alt_key = (mod & SDL_KMOD_ALT) != 0;
        
        // 将 SDL 键码转换为 DOM keyCode
        int key_code = SDLKeycodeToKeyCode(event.key.key);
        
        auto& devtools = DevToolsManager::GetInstance();
        if (devtools.HandleKeyboardShortcut(key_code, ctrl_key, shift_key, alt_key)) {
            // DevTools 消费了这个快捷键
            // 触发重绘
            auto& window_manager = WindowManager::Instance();
            for (auto& window : window_manager.GetAllWindows()) {
                window->SetNeedsRepaint();
            }
            return;
        }
    }

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
                textarea_element->HandleKeyPress(key, ctrl_key, shift_key);
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

void EventLoop::HandleMouseWheelEventForDOM(const SDL_Event& event) {
    // 获取窗口管理器
    auto& window_manager = WindowManager::Instance();

    Uint32 window_id = event.wheel.windowID;
    float mouse_x = event.wheel.mouse_x;
    float mouse_y = event.wheel.mouse_y;
    float wheel_x = event.wheel.x;
    float wheel_y = event.wheel.y;

    // 查找对应的窗口
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }

    // 将物理像素坐标转换为逻辑像素坐标
    float dpi_scale = window->GetDisplayScale();
    float logical_x = mouse_x / dpi_scale;
    float logical_y = mouse_y / dpi_scale;

    // ===== 处理 DevTools 滚轮事件 =====
    auto& devtools = DevToolsManager::GetInstance();
    if (devtools.IsOpen()) {
        // 获取窗口尺寸
        int win_width, win_height;
        SDL_GetWindowSize(window->GetSDLWindow(), &win_width, &win_height);
        float width = static_cast<float>(win_width) / dpi_scale;
        float height = static_cast<float>(win_height) / dpi_scale;
        
        // 获取 DevTools 面板区域
        float panel_x, panel_y, panel_width, panel_height;
        devtools.GetPanelBounds(width, height, panel_x, panel_y, panel_width, panel_height);
        
        // 检查鼠标是否在 DevTools 面板区域内
        if (logical_x >= panel_x && logical_x < panel_x + panel_width &&
            logical_y >= panel_y && logical_y < panel_y + panel_height) {
            // 将滚轮事件传递给 DevTools
            if (devtools.HandleMouseWheel(
                static_cast<int>(logical_x - panel_x),
                static_cast<int>(logical_y - panel_y),
                wheel_x, wheel_y
            )) {
                window->SetNeedsRepaint();
            }
            return;  // DevTools 消费了事件
        }
    }

    // 确保渲染树已构建
    window->EnsureRenderTree();

    // 获取缓存的渲染树
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) {
        return;
    }

    // 检查是否按住 Shift 键（用于水平滚动）
    const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
    bool shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];

    // 先在 LayerManager 的 Overlay/Modal 层中处理滚动
    auto& layer_manager = LayerManager::Instance();
    float scroll_delta_x = shift_pressed ? wheel_y * 40.0f : wheel_x * 40.0f;
    float scroll_delta_y = shift_pressed ? 0 : wheel_y * 40.0f;
    
    if (layer_manager.HandleWheel(logical_x, logical_y, scroll_delta_x, scroll_delta_y)) {
        // Overlay/Modal 层处理了滚动事件，不传递到 Base 层
        window->SetNeedsRepaint();
        return;
    }

    // 使用渲染树进行 Hit Testing（使用逻辑坐标）
    HitTesting hit_testing;
    HitTestResult hit_result = hit_testing.HitTestRenderObject(root_render, logical_x, logical_y, 0.0f, 0.0f);

    if (!hit_result.IsValid()) {
        return;
    }

    // 派发 wheel 事件到 DOM，让 JS 有机会阻止默认行为
    if (hit_result.element) {
        auto wheel_event = std::make_shared<MouseEvent>(
            "wheel",
            static_cast<int>(logical_x),
            static_cast<int>(logical_y),
            0  // button
        );
        // 设置滚轮增量（可以通过扩展 MouseEvent 来支持）
        hit_result.element->DispatchEvent(wheel_event);
        
        // 如果事件被阻止了默认行为，不处理滚动
        if (wheel_event->IsDefaultPrevented()) {
            return;
        }
        
        // 如果事件传播被停止，也不处理滚动（让元素自己处理）
        if (wheel_event->IsPropagationStopped()) {
            return;
        }
    }

    // shift_pressed 已在上面声明

    // 首先检查是否是 textarea 元素
    if (hit_result.element) {
        std::string tag_name = hit_result.element->GetTagName();
        if (tag_name == "textarea") {
            auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(hit_result.element);
            if (textarea_element && hit_result.render_object) {
                const auto& style = hit_result.render_object->GetComputedStyle();
                float padding_top = style.padding.top.ToPx();
                float padding_bottom = style.padding.bottom.ToPx();
                float padding_left = style.padding.left.ToPx();
                float padding_right = style.padding.right.ToPx();
                const auto& layout = hit_result.render_object->GetLayoutInfo();
                float visible_height = layout.height - padding_top - padding_bottom;
                float visible_width = layout.width - padding_left - padding_right;

                // 计算行高（与渲染保持一致）
                float font_size = style.font_size > 0 ? style.font_size : 14.0f;
                FontDescriptor desc;
                desc.family = style.font_family.empty() ? "sans-serif" : style.font_family;
                desc.size = font_size;
                desc.weight = FontWeight::NORMAL;
                desc.style = FontStyle::NORMAL;
                SkFont font = FontManager::GetInstance().LoadFont(desc);
                SkFontMetrics font_metrics;
                font.getMetrics(&font_metrics);
                float line_height = -font_metrics.fAscent + font_metrics.fDescent;
                if (font_metrics.fLeading > 0) {
                    line_height += font_metrics.fLeading;
                } else {
                    line_height += font_size * 0.2f;
                }

                // 检查 textarea 是否真的需要滚动（内容是否超出可见区域）
                float content_height = textarea_element->GetContentHeight(line_height);
                float max_scroll = std::max(0.0f, content_height - visible_height);
                float current_scroll = textarea_element->GetScrollTop();
                
                // 判断滚动方向和是否可以滚动
                bool scrolling_down = wheel_y < 0;
                bool scrolling_up = wheel_y > 0;
                bool can_scroll_down = current_scroll < max_scroll - 0.1f;
                bool can_scroll_up = current_scroll > 0.1f;
                
                // 如果 textarea 可以在当前方向滚动，则处理滚动
                if ((scrolling_down && can_scroll_down) || (scrolling_up && can_scroll_up)) {
                    // 处理滚轮事件
                    if (shift_pressed) {
                        // Shift+滚轮：横向滚动
                        textarea_element->HandleMouseWheelHorizontal(-wheel_y, visible_width, font);
                    } else {
                        // 普通滚轮：垂直滚动
                        textarea_element->HandleMouseWheel(-wheel_y, line_height, visible_height);
                    }

                    // 标记窗口需要重绘
                    window->SetNeedsRepaint();
                    return;
                }
                // 如果 textarea 不能滚动，让事件穿透到父元素
            }
        }
    }

    // 从命中的元素向上遍历，找到第一个可滚动的元素
    auto render_obj = hit_result.render_object;
    
    while (render_obj) {
        const auto& style = render_obj->GetComputedStyle();

        // 获取独立的 overflow-x 和 overflow-y 值
        std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
        std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

        bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
        bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");

        // 检查是否可滚动
        if (allow_h_scroll || allow_v_scroll) {
            // 检查内容是否真的需要滚动（内容是否超出可见区域）
            float max_scroll_x = render_obj->GetMaxScrollX();
            float max_scroll_y = render_obj->GetMaxScrollY();
            bool can_scroll_h = allow_h_scroll && max_scroll_x > 0;
            bool can_scroll_v = allow_v_scroll && max_scroll_y > 0;

            // 计算滚动量（负值向下滚动，正值向上滚动，所以要取反）
            // 每行滚动 40 像素（类似浏览器的默认行为）
            float scroll_delta_x = -wheel_x * 40.0f;
            float scroll_delta_y = -wheel_y * 40.0f;

            // 检查滚动方向是否与元素实际可滚动的方向匹配
            // 如果用户垂直滚动但元素实际上不能垂直滚动，应该穿透到父级
            bool wants_v_scroll = (wheel_y != 0);
            bool wants_h_scroll = (wheel_x != 0) || shift_pressed;

            // 如果滚动方向与元素实际可滚动的方向不匹配，继续向上查找
            if ((wants_v_scroll && !can_scroll_v && !wants_h_scroll) ||
                (wants_h_scroll && !can_scroll_h && !wants_v_scroll)) {
                render_obj = render_obj->GetParent();
                continue;
            }

            // 如果元素完全不能滚动，穿透到父级
            if (!can_scroll_h && !can_scroll_v) {
                render_obj = render_obj->GetParent();
                continue;
            }

            // 计算元素的绝对位置，用于检测鼠标是否在滚动条上
            float elem_abs_x = 0, elem_abs_y = 0;
            std::vector<std::shared_ptr<RenderObject>> ancestors;
            auto current = render_obj;
            while (current) {
                ancestors.push_back(current);
                current = current->GetParent();
            }
            for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
                const auto& l = (*it)->GetLayoutInfo();
                elem_abs_x += l.x;
                elem_abs_y += l.y;
            }

            float element_local_x = logical_x - elem_abs_x;
            float element_local_y = logical_y - elem_abs_y;
            auto scrollbar_area = render_obj->HitTestScrollbar(element_local_x, element_local_y);

            // 如果鼠标在水平滚动条上，或者按住 Shift 键，将垂直滚动转换为水平滚动
            bool use_horizontal_scroll = (scrollbar_area == RenderObject::ScrollbarHitArea::HorizontalTrack) ||
                                         (shift_pressed && wheel_y != 0 && wheel_x == 0);

            if (use_horizontal_scroll) {
                scroll_delta_x = -wheel_y * 40.0f;
                scroll_delta_y = 0;
            }


            // 应用滚动
            // 通过统一渲染管线处理滚动
            // 这样ScrollLayerManager中的层偏移才会正确更新
            auto render_pipeline = window->GetRenderPipeline();
            bool scrolled = false;
            
            if (render_pipeline) {
                // 通过渲染管线处理滚动
                scrolled = render_pipeline->HandleScroll(render_obj.get(), scroll_delta_x, scroll_delta_y);
            } else {
                // 降级模式：直接修改RenderObject
                render_obj->ScrollBy(scroll_delta_x, scroll_delta_y);
                scrolled = true;
            }
            
            if (!scrolled) {
                // 滚动失败（可能已经到达边界），继续向上查找
                render_obj = render_obj->GetParent();
                continue;
            }

            // 标记窗口需要重绘
            // 性能优化：检查滚动容器是否是 body 元素
            // - 如果是 body 滚动，需要全屏重绘（因为整个视口内容都在移动）
            // - 如果是局部滚动容器，只需要重绘该容器区域
            bool is_body_scroll = render_obj->IsBodyElement();
            
            if (is_body_scroll) {
                // body 滚动：全屏重绘
                window->SetForceFullRepaint(true);
            } else {
                // 局部滚动容器：只标记该容器区域为脏
                // 使用视口坐标，因为绘制时会应用滚动偏移
                SkRect scroll_bounds = render_obj->GetViewportBoundingRect();
                if (!scroll_bounds.isEmpty()) {
                    window->AddDirtyRect(scroll_bounds);
                }
                // 强制全量重绘该区域（避免增量渲染的坐标系问题）
                window->SetForceFullRepaint(true);
            }
            window->SetNeedsRepaint();

            break;
        }

        render_obj = render_obj->GetParent();
    }
}

void EventLoop::HandleInputMouseInteraction(std::shared_ptr<HTMLInputElement> input_element,
                                            float local_x,
                                            Uint32 event_type,
                                            float font_size,
                                            const std::string& font_family) {
    if (!input_element) {
        return;
    }

    // 获取输入框的值和类型
    std::string value = input_element->GetValue();
    InputType type = input_element->GetInputType();

    // 只有文本类型的输入框支持鼠标选择
    if (type != InputType::Text && type != InputType::Password &&
        type != InputType::Email && type != InputType::Tel &&
        type != InputType::Url && type != InputType::Search &&
        type != InputType::Number) {
        return;
    }

    // 获取字体信息以计算字符位置
    // 使用渲染时的实际字体大小和字体族
    FontDescriptor desc;
    desc.family = font_family.empty() ? "sans-serif" : font_family;
    desc.size = font_size > 0 ? font_size : 14.0f;  // 使用传入的字体大小
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 如果是密码类型，计算使用星号
    std::string display_text = value;
    if (type == InputType::Password) {
        size_t char_count = utf8::CharCount(value);
        display_text = std::string(char_count, '*');
    }

    // 根据 local_x 计算字符位置
    // local_x 是相对于文本起始位置的偏移
    int char_pos = 0;
    size_t total_chars = utf8::CharCount(value);

    if (local_x <= 0 || total_chars == 0) {
        char_pos = 0;
    } else {
        // 逐个字符测量，找到 local_x 落在哪个字符范围内
        float accumulated_width = 0.0f;
        size_t byte_pos = 0;

        for (size_t i = 0; i < total_chars; ++i) {
            // 获取当前字符的字节长度
            size_t next_byte_pos = utf8::CharPosToBytePos(value, i + 1);
            std::string char_str;

            if (type == InputType::Password) {
                char_str = "*";
            } else {
                char_str = value.substr(byte_pos, next_byte_pos - byte_pos);
            }

            // 测量当前字符宽度
            float char_width = font.measureText(char_str.c_str(), char_str.size(), SkTextEncoding::kUTF8);

            // 检查 local_x 是否在当前字符范围内
            // 如果 local_x 在字符中心之前，光标放在字符前；否则放在字符后
            if (local_x < accumulated_width + char_width / 2) {
                char_pos = static_cast<int>(i);
                break;
            }

            accumulated_width += char_width;
            byte_pos = next_byte_pos;
            char_pos = static_cast<int>(i + 1);  // 如果超过所有字符，光标放在最后
        }
    }

    // 根据事件类型处理
    if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // 鼠标按下：设置光标位置并开始拖动选择
        input_element->SetCursorPosition(char_pos);
        input_element->SetDragStartPos(char_pos);
        input_element->HandleMouseDown(local_x, 0);  // 通知开始拖动
    } else if (event_type == SDL_EVENT_MOUSE_MOTION) {
        // 鼠标移动：更新选择区域（如果正在拖动）
        if (input_element->IsDraggingSelection()) {
            int drag_start = input_element->GetDragStartPos();
            input_element->SetSelection(drag_start, char_pos);

        }
    } else if (event_type == SDL_EVENT_MOUSE_BUTTON_UP) {
        // 鼠标释放：结束拖动选择
        input_element->HandleMouseUp();
    }
}

void EventLoop::HandleTextAreaMouseInteraction(std::shared_ptr<HTMLTextAreaElement> textarea_element,
                                               float local_x,
                                               float local_y,
                                               Uint32 event_type,
                                               float font_size,
                                               const std::string& font_family,
                                               bool shift_key,
                                               float visible_width,
                                               float visible_height) {
    if (!textarea_element) {
        return;
    }

    // 获取字体
    FontDescriptor desc;
    desc.family = font_family.empty() ? "sans-serif" : font_family;
    desc.size = font_size > 0 ? font_size : 14.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    float line_height = -font_metrics.fAscent + font_metrics.fDescent;
    if (font_metrics.fLeading > 0) {
        line_height += font_metrics.fLeading;
    } else {
        line_height += font_size * 0.2f;
    }

    // 拖动选择时自动滚动
    if (event_type == SDL_EVENT_MOUSE_MOTION && textarea_element->IsDraggingSelection() &&
        visible_width > 0 && visible_height > 0) {
        float scroll_speed = line_height;  // 每帧滚动一行高度
        float scroll_top = textarea_element->GetScrollTop();
        float scroll_left = textarea_element->GetScrollLeft();

        // 计算内容高度和最大滚动值
        int line_count = textarea_element->GetLineCount();
        float content_height = line_count * line_height;
        float max_scroll_y = std::max(0.0f, content_height - visible_height);

        // 计算最大行宽度 - 使用支持 CJK/Emoji 的测量方法
        TextRenderer text_renderer(nullptr);
        float max_line_width = 0.0f;
        std::string value = textarea_element->GetValue();
        std::istringstream stream(value);
        std::string line;
        while (std::getline(stream, line)) {
            float w = text_renderer.MeasureTextWidthWithEmoji(line, font);
            if (w > max_line_width) max_line_width = w;
        }
        float max_scroll_x = std::max(0.0f, max_line_width - visible_width);

        // 调试输出
        std::cout << "[DEBUG] local_x=" << local_x << ", local_y=" << local_y
                  << ", visible_w=" << visible_width << ", visible_h=" << visible_height << std::endl;

        // 检测鼠标是否超出边界并自动滚动
        bool scrolled = false;
        if (local_y < 0) {
            // 鼠标在上边界外，向上滚动
            std::cout << "[DEBUG] Scrolling UP" << std::endl;
            float new_scroll = std::max(0.0f, scroll_top - scroll_speed);
            textarea_element->SetScrollTop(new_scroll);
            scrolled = true;
        } else if (local_y > visible_height) {
            // 鼠标在下边界外，向下滚动
            std::cout << "[DEBUG] Scrolling DOWN" << std::endl;
            float new_scroll = std::min(max_scroll_y, scroll_top + scroll_speed);
            textarea_element->SetScrollTop(new_scroll);
            scrolled = true;
        }

        if (local_x < 0) {
            // 鼠标在左边界外，向左滚动
            float new_scroll = std::max(0.0f, scroll_left - scroll_speed);
            textarea_element->SetScrollLeft(new_scroll);
            scrolled = true;
        } else if (local_x > visible_width) {
            // 鼠标在右边界外，向右滚动
            float new_scroll = std::min(max_scroll_x, scroll_left + scroll_speed);
            textarea_element->SetScrollLeft(new_scroll);
            scrolled = true;
        }
    }

    // 获取滚动偏移量，计算实际的坐标
    float scroll_top = textarea_element->GetScrollTop();
    float scroll_left = textarea_element->GetScrollLeft();

    // 限制坐标在有效范围内（防止鼠标拖出边界时计算出错）
    float clamped_local_x = local_x;
    float clamped_local_y = local_y;
    if (clamped_local_x < 0) clamped_local_x = 0;
    if (clamped_local_y < 0) clamped_local_y = 0;
    if (visible_width > 0 && clamped_local_x > visible_width) clamped_local_x = visible_width;
    if (visible_height > 0 && clamped_local_y > visible_height) clamped_local_y = visible_height;

    float actual_x = clamped_local_x + scroll_left;  // 考虑横向滚动
    float actual_y = clamped_local_y + scroll_top;

    // 计算点击的行号（使用考虑滚动偏移后的坐标）
    int clicked_line = static_cast<int>(actual_y / line_height);
    if (clicked_line < 0) clicked_line = 0;

    // 获取 textarea 的文本内容
    std::string value = textarea_element->GetValue();

    // 将文本分割成行
    std::vector<std::string> lines;
    std::istringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    if (value.empty() || (!value.empty() && value.back() == '\n')) {
        lines.push_back("");
    }

    // 确保行号在有效范围内
    if (clicked_line >= static_cast<int>(lines.size())) {
        clicked_line = static_cast<int>(lines.size()) - 1;
    }
    if (clicked_line < 0) clicked_line = 0;

    // 计算该行之前的字符总数（包含换行符）
    int char_offset = 0;
    for (int i = 0; i < clicked_line && i < static_cast<int>(lines.size()); i++) {
        char_offset += static_cast<int>(utf8::CharCount(lines[i])) + 1;  // +1 for newline
    }

    // 在当前行中查找点击位置对应的字符
    const std::string& current_line = clicked_line < static_cast<int>(lines.size()) ? lines[clicked_line] : "";
    int char_pos_in_line = 0;

    if (!current_line.empty()) {
        float accumulated_width = 0;
        size_t char_count = utf8::CharCount(current_line);

        // 使用支持 CJK/Emoji 的文本测量（与渲染一致）
        TextRenderer text_renderer(nullptr);

        for (size_t i = 0; i < char_count; i++) {
            // 获取当前字符
            size_t byte_start = utf8::CharPosToBytePos(current_line, static_cast<int>(i));
            size_t byte_end = utf8::CharPosToBytePos(current_line, static_cast<int>(i + 1));
            std::string char_str = current_line.substr(byte_start, byte_end - byte_start);

            float char_width = text_renderer.MeasureTextWidthWithEmoji(char_str, font);

            // 使用 actual_x（考虑横向滚动偏移后的坐标）
            if (actual_x < accumulated_width + char_width / 2) {
                break;
            }
            accumulated_width += char_width;
            char_pos_in_line++;
        }
    }

    int char_pos = char_offset + char_pos_in_line;

    // 根据事件类型处理
    if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (shift_key) {
            // Shift+点击：扩展选择区域，保持 selection_start 不变
            int current_start = textarea_element->GetSelectionStart();
            textarea_element->SetSelection(current_start, char_pos);
        } else {
            // 普通点击：设置光标位置
            textarea_element->SetCursorPosition(char_pos);
        }
        textarea_element->SetDragStartPos(char_pos);
        textarea_element->HandleMouseDown(local_x, local_y);
    } else if (event_type == SDL_EVENT_MOUSE_MOTION) {
        if (textarea_element->IsDraggingSelection()) {
            int drag_start = textarea_element->GetDragStartPos();
            textarea_element->SetSelection(drag_start, char_pos);
        }
    } else if (event_type == SDL_EVENT_MOUSE_BUTTON_UP) {
        textarea_element->HandleMouseUp();
    }
}

// ===== 系统光标管理实现 =====
// 参考：RmlUi/Backends/RmlUi_Platform_SDL.cpp

void EventLoop::InitSystemCursors() {
    // 创建系统光标（SDL3 API）
    cursor_default_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    cursor_pointer_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    cursor_text_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    cursor_ew_resize_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);  // 水平调整大小
    cursor_ns_resize_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);  // 垂直调整大小
    current_cursor_type_ = SDL_SYSTEM_CURSOR_DEFAULT;
}

void EventLoop::DestroySystemCursors() {
    if (cursor_default_) {
        SDL_DestroyCursor(cursor_default_);
        cursor_default_ = nullptr;
    }
    if (cursor_pointer_) {
        SDL_DestroyCursor(cursor_pointer_);
        cursor_pointer_ = nullptr;
    }
    if (cursor_text_) {
        SDL_DestroyCursor(cursor_text_);
        cursor_text_ = nullptr;
    }
    if (cursor_ew_resize_) {
        SDL_DestroyCursor(cursor_ew_resize_);
        cursor_ew_resize_ = nullptr;
    }
    if (cursor_ns_resize_) {
        SDL_DestroyCursor(cursor_ns_resize_);
        cursor_ns_resize_ = nullptr;
    }
}

void EventLoop::SetSystemCursor(SDL_SystemCursor cursor_type) {
    // 避免重复设置相同的光标
    if (cursor_type == current_cursor_type_) {
        return;
    }

    SDL_Cursor* cursor = nullptr;
    switch (cursor_type) {
        case SDL_SYSTEM_CURSOR_DEFAULT:
            cursor = cursor_default_;
            break;
        case SDL_SYSTEM_CURSOR_POINTER:
            cursor = cursor_pointer_;
            break;
        case SDL_SYSTEM_CURSOR_TEXT:
            cursor = cursor_text_;
            break;
        case SDL_SYSTEM_CURSOR_EW_RESIZE:
            cursor = cursor_ew_resize_;
            break;
        case SDL_SYSTEM_CURSOR_NS_RESIZE:
            cursor = cursor_ns_resize_;
            break;
        default:
            cursor = cursor_default_;
            break;
    }

    if (cursor) {
        SDL_SetCursor(cursor);
        current_cursor_type_ = cursor_type;
    }
}

void EventLoop::UpdateMouseCursor(const HitTestResult& hit_result, Uint32 window_id) {
    // 默认使用箭头光标
    SDL_SystemCursor target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;

    if (hit_result.IsValid() && hit_result.element) {
        std::string tag_name = hit_result.element->GetTagName();

        // input 元素
        if (tag_name == "input") {
            auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(hit_result.element);
            if (input_element) {
                InputType input_type = input_element->GetInputType();

                // 对于 number 类型，检查是否在 spinner 区域
                if (input_type == InputType::Number && hit_result.render_object) {
                    const auto& style = hit_result.render_object->GetComputedStyle();
                    const auto& layout = hit_result.render_object->GetLayoutInfo();
                    float padding_left = style.padding.left.ToPx();
                    float padding_right = style.padding.right.ToPx();
                    float border_width = style.border.width.ToPx();

                    const float spinner_width = 16.0f;
                    float content_width = layout.width - padding_left - padding_right - border_width * 2;
                    float spinner_x = padding_left + border_width + content_width - spinner_width;

                    if (hit_result.local_x >= spinner_x) {
                        // 在 spinner 区域使用箭头光标
                        target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
                    } else {
                        // 在文本区域使用文本光标
                        target_cursor = SDL_SYSTEM_CURSOR_TEXT;
                    }
                }
                // 对于文本类型输入框，使用文本光标
                else if (input_type == InputType::Text ||
                         input_type == InputType::Password ||
                         input_type == InputType::Email ||
                         input_type == InputType::Tel ||
                         input_type == InputType::Url ||
                         input_type == InputType::Search) {
                    target_cursor = SDL_SYSTEM_CURSOR_TEXT;
                }
                // 对于按钮类型（button, submit, reset），使用箭头光标
                else if (input_type == InputType::Button ||
                         input_type == InputType::Submit ||
                         input_type == InputType::Reset) {
                    target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
                }
                // 对于 checkbox 和 radio，使用箭头光标
                else if (input_type == InputType::Checkbox ||
                         input_type == InputType::Radio) {
                    target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
                }
            }
        }
        // textarea 元素使用文本光标
        else if (tag_name == "textarea") {
            target_cursor = SDL_SYSTEM_CURSOR_TEXT;
        }
        // button 元素使用箭头光标（浏览器默认行为）
        else if (tag_name == "button") {
            target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
        }
        // select 元素使用箭头光标
        else if (tag_name == "select") {
            target_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
        }
        // a 元素（链接）使用手型光标
        else if (tag_name == "a") {
            target_cursor = SDL_SYSTEM_CURSOR_POINTER;
        }
    }

    SetSystemCursor(target_cursor);
}

} // namespace lightui

