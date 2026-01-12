/**
 * @file event_loop.cpp
 * @brief 主事件循环实现
 * 
 * @note 大文件说明 (3903 行)
 * 本文件包含 EventLoop 类的完整实现，是应用程序的核心事件处理组件。
 * 文件较大的原因：
 * 1. 包含完整的鼠标事件处理逻辑（点击、拖动、hover 链管理）
 * 2. 包含完整的键盘事件处理逻辑（快捷键、文本输入、IME）
 * 3. 包含滚轮事件处理和滚动逻辑
 * 4. 包含 DevTools 集成代码
 * 5. 包含表单元素交互处理（input、textarea、select）
 * 6. 包含 contentEditable 编辑支持
 * 7. 包含拖放功能支持
 *
 * 计划重构：
 * - 提取鼠标事件处理到 MouseEventDispatcher 类
 * - 提取键盘事件处理到 KeyboardEventDispatcher 类
 * - 提取滚动条控制到 ScrollbarController 类
 * 参见: .kiro/specs/code-structure-refactoring/tasks.md Phase 3
 */

#include "event_loop.h"

#include "../dispatch/keyboard_event_dispatcher.h"
#include "../dispatch/mouse_event_dispatcher.h"
#include "../dispatch/wheel_event_dispatcher.h"
#include "frame_controller.h"
#include "../input/input_handler.h"
#include "task_scheduler.h"
#include "../input/focus_manager.h"
#include "core/editing/drag_manager.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/editing/contenteditable_controller.h"
#include "core/editing/clipboard_manager.h"
#include "../types/mouse_event.h"
#include "../input/keyboard_utils.h"
#include "../input/hit_test_controller.h"
#include "../types/event_types.h"
#include "core/window/window_manager.h"
#include "core/render/layer/paint_layer.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/selection/selection.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_button_element.h"
#include "core/dom/elements/html_form_element.h"
#include "core/dom/elements/html_select_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/render/objects/select_dropdown.h"
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_inline_block.h"
#include "core/render/text/font_manager.h"
#include "core/render/text/text_renderer.h"
#include "core/utils/utf8_utils.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/devtools/devtools_manager.h"
#include "core/devtools/inspector/element_picker.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkFontMetrics.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <functional>
#include <limits>
#include <map>

namespace lightui {

EventLoop::EventLoop()
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(std::make_shared<TaskScheduler>())
    , focus_manager_(std::make_unique<FocusManager>())
    , drag_manager_(std::make_unique<DragManager>())
    , mouse_event_dispatcher_(std::make_unique<MouseEventDispatcher>())
    , keyboard_event_dispatcher_(std::make_unique<KeyboardEventDispatcher>())
    , wheel_event_dispatcher_(std::make_unique<WheelEventDispatcher>())
    , selection_manager_(std::make_unique<SelectionManager>())
    , vsync_detected_(false)
{
    // 初始化富文本编辑子系统（需要在 selection_manager_ 之后）
    contenteditable_handler_ = std::make_unique<ContentEditableHandler>(selection_manager_.get());
    contenteditable_controller_ = std::make_unique<ContentEditableController>(selection_manager_.get(), contenteditable_handler_.get());
    clipboard_manager_ = std::make_unique<ClipboardManager>(selection_manager_.get(), contenteditable_handler_.get());

    // 设置 MouseEventDispatcher 的依赖
    mouse_event_dispatcher_->SetManagers(
        drag_manager_.get(),
        selection_manager_.get(),
        contenteditable_handler_.get(),
        focus_manager_.get()
    );
    mouse_event_dispatcher_->SetCursorCallback([this](SDL_SystemCursor cursor) {
        SetSystemCursor(cursor);
    });

    // 设置 KeyboardEventDispatcher 的依赖
    keyboard_event_dispatcher_->SetManagers(
        focus_manager_.get(),
        contenteditable_handler_.get(),
        contenteditable_controller_.get(),
        clipboard_manager_.get()
    );

    // 延迟初始化光标（在第一次使用时初始化，避免 SDL 未初始化的问题）
    // InitSystemCursors() 将在 EnsureCursorsInitialized() 中调用
    
    // 设置所有现有窗口的 FocusManager
    auto& wm = WindowManager::Instance();
    for (auto& window : wm.GetAllWindows()) {
        window->SetFocusManager(focus_manager_.get());
    }
}

EventLoop::EventLoop(std::shared_ptr<TaskScheduler> task_scheduler)
    : running_(false)
    , should_quit_(false)
    , frame_controller_(std::make_unique<FrameController>(60))
    , input_handler_(std::make_unique<InputHandler>())
    , task_scheduler_(task_scheduler)
    , focus_manager_(std::make_unique<FocusManager>())
    , drag_manager_(std::make_unique<DragManager>())
    , mouse_event_dispatcher_(std::make_unique<MouseEventDispatcher>())
    , keyboard_event_dispatcher_(std::make_unique<KeyboardEventDispatcher>())
    , wheel_event_dispatcher_(std::make_unique<WheelEventDispatcher>())
    , selection_manager_(std::make_unique<SelectionManager>())
    , vsync_detected_(false)
{
    if (!task_scheduler_) {
        throw std::invalid_argument("TaskScheduler cannot be null");
    }

    // 初始化富文本编辑子系统（需要在 selection_manager_ 之后）
    contenteditable_handler_ = std::make_unique<ContentEditableHandler>(selection_manager_.get());
    contenteditable_controller_ = std::make_unique<ContentEditableController>(selection_manager_.get(), contenteditable_handler_.get());
    clipboard_manager_ = std::make_unique<ClipboardManager>(selection_manager_.get(), contenteditable_handler_.get());

    // 设置 MouseEventDispatcher 的依赖
    mouse_event_dispatcher_->SetManagers(
        drag_manager_.get(),
        selection_manager_.get(),
        contenteditable_handler_.get(),
        focus_manager_.get()
    );
    mouse_event_dispatcher_->SetCursorCallback([this](SDL_SystemCursor cursor) {
        SetSystemCursor(cursor);
    });

    // 设置 KeyboardEventDispatcher 的依赖
    keyboard_event_dispatcher_->SetManagers(
        focus_manager_.get(),
        contenteditable_handler_.get(),
        contenteditable_controller_.get(),
        clipboard_manager_.get()
    );

    // 延迟初始化光标（在第一次使用时初始化，避免 SDL 未初始化的问题）
    // InitSystemCursors() 将在 EnsureCursorsInitialized() 中调用
    
    // 设置所有现有窗口的 FocusManager
    auto& wm = WindowManager::Instance();
    for (auto& window : wm.GetAllWindows()) {
        window->SetFocusManager(focus_manager_.get());
    }
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

    // 设置所有窗口的 FocusManager
    auto& wm = WindowManager::Instance();
    for (auto& window : wm.GetAllWindows()) {
        window->SetFocusManager(focus_manager_.get());
    }

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
    
    // 2.1 处理全局单例 TaskScheduler 的微任务
    // Selection 等组件使用 TaskScheduler::Instance() 发布微任务
    TaskScheduler::Instance().ProcessMicrotasks();

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
    
    // 4.7 处理全局单例 TaskScheduler 的微任务（动画帧可能触发新的微任务）
    TaskScheduler::Instance().ProcessMicrotasks();

    // 4.5 处理光标闪烁（如果有聚焦的输入框或 contentEditable 元素）
    static Uint64 last_cursor_blink_time = SDL_GetTicks();
    static bool cursor_visible = true;
    auto focus_element = focus_manager_->GetFocusElement();
    if (focus_element) {
        std::string tag_name = focus_element->GetTagName();
        bool is_editable = (tag_name == "input" || tag_name == "textarea" || focus_element->IsContentEditable());
        
        if (is_editable) {
            // 每500毫秒切换光标显示状态
            Uint64 now = SDL_GetTicks();
            if (now - last_cursor_blink_time >= 500) {
                cursor_visible = !cursor_visible;
                cursor_visible_ = cursor_visible;  // 保存到成员变量供渲染使用
                last_cursor_blink_time = now;
                
                // 设置全局光标可见状态（供 RenderObject 使用）
                RenderObject::SetCursorVisible(cursor_visible);
                
                // 关键修复：标记元素的 RenderObject 需要重绘
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
        // 没有聚焦的可编辑元素时，重置光标状态
        cursor_visible = true;
        cursor_visible_ = true;
        RenderObject::SetCursorVisible(true);
    }

    // 5. 只在有窗口需要重绘时才渲染
    auto& wm = WindowManager::Instance();
    
    // 5.1 检查终端是否需要重绘（PTY 数据到达）
    bool terminal_repaint = TerminalNeedsRepaint();
    if (terminal_repaint) {
        for (auto& window : wm.GetAllWindows()) {
            window->SetNeedsRepaint();
            // 关键：强制重新光栅化，确保终端内容被重绘
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->ForceRasterize();
            }
        }
    }
    
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

SelectionManager* EventLoop::GetSelectionManager() {
    return selection_manager_.get();
}

ContentEditableHandler* EventLoop::GetContentEditableHandler() {
    return contenteditable_handler_.get();
}

ClipboardManager* EventLoop::GetClipboardManager() {
    return clipboard_manager_.get();
}

bool EventLoop::ProcessEvents() {
    bool has_events = false;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        has_events = true;

        // 调试：输出事件类型（可以通过环境变量控制）
#ifdef _WIN32
        static bool debug_events = []() {
            char* env_val = nullptr;
            size_t env_len = 0;
            bool result = (_dupenv_s(&env_val, &env_len, "LIGHTUI_DEBUG_EVENTS") == 0 && env_val != nullptr);
            free(env_val);
            return result;
        }();
#else
        static bool debug_events = std::getenv("LIGHTUI_DEBUG_EVENTS") != nullptr;
#endif
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
    // 调试：检查 mousemove 事件是否被触发
    // if (event.type == SDL_EVENT_MOUSE_MOTION) {
    //     std::cout << "[EventLoop] HandleMouseEventForDOM: MOTION event received" << std::endl;
    // }
    
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
    HitTestController hit_controller;
    HitTestRequest request;
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
                        auto result_ex = hit_controller.HitTest(root_render, app_relative_x, app_relative_y, request);
                        HitTestResult hit_result;
                        if (result_ex.IsValid()) {
                            hit_result.element = result_ex.element;
                            hit_result.render_object = result_ex.render_object;
                            hit_result.local_x = result_ex.local_x;
                            hit_result.local_y = result_ex.local_y;
                        }
                        
                        if (hit_result.IsValid() && hit_result.element) {
                            devtools.GetElementPicker()->SetHoverElement(hit_result.element, hit_result.render_object);
                            window->SetNeedsRepaint();
                        }
                    }
                    // 处理鼠标点击：选中元素
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && 
                             event.button.button == SDL_BUTTON_LEFT) {
                        auto result_ex = hit_controller.HitTest(root_render, app_relative_x, app_relative_y, request);
                        HitTestResult hit_result;
                        if (result_ex.IsValid()) {
                            hit_result.element = result_ex.element;
                            hit_result.render_object = result_ex.render_object;
                            hit_result.local_x = result_ex.local_x;
                            hit_result.local_y = result_ex.local_y;
                        }
                        
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
    // 委托给 MouseEventDispatcher 处理
    window->EnsureRenderTree();
    auto root_render = window->GetCachedRenderTree();
    
    mouse_event_dispatcher_->HandleMouseEvent(event, window, document, root_render);
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
    // 向后兼容版本：内部执行 Hit Testing 然后调用 MouseEventDispatcher
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

    HitTestController hit_controller;
    HitTestRequest request;
    auto result_ex = hit_controller.HitTest(root_render, logical_x, logical_y, request);
    HitTestResult hit_result;
    if (result_ex.IsValid()) {
        hit_result.element = result_ex.element;
        hit_result.render_object = result_ex.render_object;
        hit_result.local_x = result_ex.local_x;
        hit_result.local_y = result_ex.local_y;
    }

    // 委托给 MouseEventDispatcher
    mouse_event_dispatcher_->UpdateHoverChain(window, logical_x, logical_y, hit_result);
}

void EventLoop::UpdateHoverChainWithResult(Uint32 window_id, float mouse_x, float mouse_y, const HitTestResult& hit_result) {
    // 向后兼容版本：委托给 MouseEventDispatcher
    auto& window_manager = WindowManager::Instance();
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        return;
    }
    mouse_event_dispatcher_->UpdateHoverChain(window, mouse_x, mouse_y, hit_result);
}

void EventLoop::HandleKeyboardEventForDOM(const SDL_Event& event) {
    // 获取窗口管理器
    auto& window_manager = WindowManager::Instance();

    // 根据事件类型获取窗口 ID
    Uint32 window_id = 0;
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        window_id = event.key.windowID;
        std::cout << "[EventLoop] Keyboard event: type=" << (event.type == SDL_EVENT_KEY_DOWN ? "KEY_DOWN" : "KEY_UP") 
                  << ", windowID=" << window_id << std::endl;
    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
        window_id = event.text.windowID;
        std::cout << "[EventLoop] Text input event: '" << event.text.text << "', windowID=" << window_id << std::endl;
    }

    // 查找对应的窗口
    auto window = window_manager.FindWindowByID(window_id);
    if (!window) {
        // 尝试获取第一个窗口作为后备
        auto all_windows = window_manager.GetAllWindows();
        if (!all_windows.empty()) {
            window = all_windows[0];
            std::cout << "[EventLoop] Using fallback window" << std::endl;
        } else {
            std::cout << "[EventLoop] Window not found for keyboard event, no fallback available" << std::endl;
            return;
        }
    }

    // 获取窗口的文档
    auto document = window->GetDocument();
    if (!document) {
        std::cout << "[EventLoop] Document not found for keyboard event" << std::endl;
        return;
    }

    // 委托给 KeyboardEventDispatcher 处理
    keyboard_event_dispatcher_->HandleKeyboardEvent(event, window, document);
}

void EventLoop::HandleMouseWheelEventForDOM(const SDL_Event& event) {
    // 获取窗口管理器
    auto& window_manager = WindowManager::Instance();

    Uint32 window_id = event.wheel.windowID;

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

    // 委托给 WheelEventDispatcher 处理
    wheel_event_dispatcher_->HandleWheelEvent(event, window, document);
}

// ===== 系统光标管理实现 =====
// 参考：RmlUi/Backends/RmlUi_Platform_SDL.cpp

void EventLoop::EnsureCursorsInitialized() {
    if (!cursors_initialized_) {
        InitSystemCursors();
        cursors_initialized_ = true;
    }
}

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
    // 延迟初始化光标
    EnsureCursorsInitialized();
    
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

void EventLoop::UpdateMouseCursor(const HitTestResult& hit_result, Uint32 /*window_id*/) {
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
