/**
 * @file event_loop.h
 * @brief 主事件循环
 * 
 * 负责处理所有系统事件、用户输入和渲染更新
 */

#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <functional>
#include <unordered_set>
#include <string>

namespace lightui {

// 前向声明
class WindowManager;
class FrameController;
class InputHandler;
class TaskScheduler;
class Element;
class Node;
class HTMLInputElement;
class HTMLTextAreaElement;
class HTMLButtonElement;
class HTMLFormElement;
class FocusManager;
class DragManager;
struct HitTestResult;
class RenderObject;
class QuickJSRuntime;

/**
 * @brief 主事件循环类
 * 
 * 负责：
 * 1. 轮询和分发 SDL 事件
 * 2. 控制帧率
 * 3. 处理输入事件
 * 4. 执行调度任务
 * 5. 触发渲染更新
 */
class EventLoop {
public:
    /**
     * @brief 构造函数（创建内部 TaskScheduler）
     */
    EventLoop();

    /**
     * @brief 构造函数（使用外部 TaskScheduler）
     * @param task_scheduler 外部任务调度器
     */
    explicit EventLoop(std::shared_ptr<TaskScheduler> task_scheduler);

    /**
     * @brief 析构函数
     */
    ~EventLoop();

    /**
     * @brief 启动事件循环
     * 
     * 阻塞运行，直到调用 Stop() 或所有窗口关闭
     */
    void Run();
    
    /**
     * @brief 停止事件循环
     * 
     * 设置退出标志，事件循环将在当前帧结束后退出
     */
    void Stop();
    
    /**
     * @brief 单次循环迭代
     * 
     * 执行一次完整的事件处理、更新和渲染循环
     * 用于测试或手动控制循环
     */
    void RunOnce();
    
    /**
     * @brief 检查是否应该退出
     * 
     * @return true 如果应该退出
     */
    bool ShouldQuit() const;
    
    /**
     * @brief 检查是否正在运行
     * 
     * @return true 如果事件循环正在运行
     */
    bool IsRunning() const;
    
    /**
     * @brief 设置空闲回调
     * 
     * 当没有事件需要处理时调用
     * 
     * @param callback 空闲回调函数
     */
    void SetIdleCallback(std::function<void()> callback);
    
    /**
     * @brief 设置更新回调
     * 
     * 每帧调用一次，用于更新应用状态
     * 
     * @param callback 更新回调函数，参数为帧时间（秒）
     */
    void SetUpdateCallback(std::function<void(float)> callback);
    
    /**
     * @brief 设置渲染回调
     * 
     * 每帧调用一次，用于渲染
     * 
     * @param callback 渲染回调函数
     */
    void SetRenderCallback(std::function<void()> callback);
    
    /**
     * @brief 获取帧率控制器
     *
     * @return FrameController& 帧率控制器引用
     */
    FrameController& GetFrameController();

    /**
     * @brief 获取输入处理器
     *
     * @return InputHandler& 输入处理器引用
     */
    InputHandler& GetInputHandler();

    /**
     * @brief 获取任务调度器引用
     *
     * @return TaskScheduler& 任务调度器引用
     */
    TaskScheduler& GetTaskScheduler();

    /**
     * @brief 获取任务调度器智能指针
     *
     * @return std::shared_ptr<TaskScheduler> 任务调度器智能指针
     */
    std::shared_ptr<TaskScheduler> GetTaskSchedulerPtr();

    /**
     * @brief 获取光标是否可见（用于闪烁效果）
     *
     * @return true 如果光标应该显示
     */
    bool IsCursorVisible() const { return cursor_visible_; }

    /**
     * @brief 设置 QuickJS 运行时（用于处理 JS 定时器和微任务）
     * @param runtime QuickJS 运行时指针
     */
    void SetQuickJSRuntime(QuickJSRuntime* runtime) { quickjs_runtime_ = runtime; }

private:
    /**
     * @brief 处理 SDL 事件
     * 
     * @return true 如果有事件被处理
     */
    bool ProcessEvents();
    
    /**
     * @brief 更新应用状态
     * 
     * @param delta_time 帧时间（秒）
     */
    void Update(float delta_time);
    
    /**
     * @brief 渲染所有窗口
     */
    void Render();
    
    /**
     * @brief 检查是否有工作需要做
     *
     * @return true 如果有待处理的事件、任务或渲染
     */
    bool HasWork() const;

private:
    /**
     * @brief 处理鼠标事件并分发到 DOM
     *
     * @param event SDL 鼠标事件
     */
    void HandleMouseEventForDOM(const SDL_Event& event);

    /**
     * @brief 处理键盘事件并分发到 DOM
     *
     * @param event SDL 键盘事件
     */
    void HandleKeyboardEventForDOM(const SDL_Event& event);

    /**
     * @brief 处理鼠标滚轮事件并分发到 DOM
     *
     * @param event SDL 鼠标滚轮事件
     */
    void HandleMouseWheelEventForDOM(const SDL_Event& event);

    /**
     * @brief 处理表单元素的默认行为（参考 RmlUi InputTypeCheckbox::ProcessDefaultAction）
     *
     * @param element 被点击的元素
     * @param hit_result Hit Testing 结果（包含渲染对象）
     */
    void ProcessFormElementDefaultAction(std::shared_ptr<Element> element, const HitTestResult& hit_result);

    /**
     * @brief 取消同组 radio 的选中状态
     *
     * @param node 起始节点
     * @param group_name radio 组名
     * @param except 排除的元素（当前选中的 radio）
     */
    void UncheckRadioGroup(const std::shared_ptr<Node>& node, const std::string& group_name, const std::shared_ptr<HTMLInputElement>& except);

    /**
     * @brief 查找父级表单元素
     *
     * @param element 起始元素
     * @return 父级表单元素，如果没有找到则返回 nullptr
     */
    std::shared_ptr<HTMLFormElement> FindParentForm(std::shared_ptr<Element> element);

    /**
     * @brief 将 SDL 鼠标按钮转换为鼠标按钮编号
     *
     * @param sdl_button SDL 鼠标按钮
     * @return 鼠标按钮编号 (0=无, 1=左, 2=中, 3=右)
     */
    static int SDLButtonToMouseButton(Uint8 sdl_button);

    /**
     * @brief 更新hover链并发送mouseover/mouseout事件
     *
     * 参考：RmlUi/Source/Core/Context.cpp - UpdateHoverChain
     *
     * @param window_id 窗口ID
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     */
    void UpdateHoverChain(Uint32 window_id, float mouse_x, float mouse_y);

    /**
     * @brief 更新hover链（使用已计算的 HitTestResult）
     *
     * 性能优化版本：复用外部已计算的 Hit Testing 结果，避免重复遍历渲染树
     *
     * @param window_id 窗口ID
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @param hit_result 已计算的 Hit Testing 结果
     */
    void UpdateHoverChainWithResult(Uint32 window_id, float mouse_x, float mouse_y, const HitTestResult& hit_result);

    /**
     * @brief 发送事件到元素集合的差集
     *
     * 参考：RmlUi/Source/Core/Context.cpp - SendEvents
     *
     * @param old_items 旧元素集合（使用 weak_ptr 避免悬空指针）
     * @param new_items 新元素集合（使用 weak_ptr 避免悬空指针）
     * @param event_type 事件类型
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     * @return 是否有伪类变化（需要重绘）
     */
    bool SendEvents(const std::vector<std::weak_ptr<Element>>& old_items,
                   const std::vector<std::weak_ptr<Element>>& new_items,
                   const std::string& event_type,
                   float mouse_x,
                   float mouse_y);

    /**
     * @brief 处理输入框的鼠标交互（点击定位光标、拖动选择）
     *
     * @param input_element 输入元素
     * @param local_x 相对于输入框内容区域的X坐标
     * @param event_type 事件类型 (SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_BUTTON_UP)
     * @param font_size 字体大小（用于计算字符宽度）
     * @param font_family 字体族（用于计算字符宽度）
     */
    void HandleInputMouseInteraction(std::shared_ptr<HTMLInputElement> input_element,
                                     float local_x,
                                     Uint32 event_type,
                                     float font_size,
                                     const std::string& font_family);

    /**
     * @brief 处理 TextArea 元素的鼠标交互
     * @param textarea_element TextArea 元素
     * @param local_x 相对于文本内容区域的 X 坐标
     * @param local_y 相对于文本内容区域的 Y 坐标
     * @param event_type 事件类型
     * @param font_size 字体大小
     * @param font_family 字体族
     * @param shift_key 是否按住Shift键
     * @param visible_width 可见区域宽度（用于自动滚动）
     * @param visible_height 可见区域高度（用于自动滚动）
     */
    void HandleTextAreaMouseInteraction(std::shared_ptr<HTMLTextAreaElement> textarea_element,
                                        float local_x,
                                        float local_y,
                                        Uint32 event_type,
                                        float font_size,
                                        const std::string& font_family,
                                        bool shift_key = false,
                                        float visible_width = 0.0f,
                                        float visible_height = 0.0f);

private:
    bool running_ = false;          // 是否正在运行
    bool should_quit_ = false;      // 是否应该退出

    // 回调函数
    std::function<void()> idle_callback_;
    std::function<void(float)> update_callback_;
    std::function<void()> render_callback_;

    // 子系统（前向声明，实现文件中定义）
    std::unique_ptr<FrameController> frame_controller_;
    std::unique_ptr<InputHandler> input_handler_;
    std::shared_ptr<TaskScheduler> task_scheduler_;  // 可以是外部的或内部创建的
    std::unique_ptr<FocusManager> focus_manager_;
    std::unique_ptr<DragManager> drag_manager_;

    // Hover链追踪（参考RmlUi的hover_chain）
    // 存储当前鼠标悬停的元素链（从目标元素到根元素）
    // 使用 weak_ptr 避免悬空指针问题
    std::vector<std::weak_ptr<Element>> hover_chain_;

    // 当前悬停的元素（最深层的元素）
    // 使用 weak_ptr 避免悬空指针问题
    std::weak_ptr<Element> hover_element_;

    // 滚动条拖动状态
    std::weak_ptr<RenderObject> scrollbar_dragging_element_;  // 正在拖动滚动条的元素
    Uint32 scrollbar_dragging_window_id_ = 0;                 // 拖动所在窗口的ID

    // 光标闪烁状态
    bool cursor_visible_ = true;  // 光标是否可见（用于闪烁效果）

    // QuickJS 运行时（用于处理 JS 定时器和微任务）
    QuickJSRuntime* quickjs_runtime_ = nullptr;

    // ===== 系统光标管理 =====
    // 参考：RmlUi/Backends/RmlUi_Platform_SDL.cpp
    SDL_Cursor* cursor_default_ = nullptr;    // 默认箭头光标
    SDL_Cursor* cursor_pointer_ = nullptr;    // 手型光标（链接、按钮）
    SDL_Cursor* cursor_text_ = nullptr;       // 文本光标（输入框）
    SDL_Cursor* cursor_ew_resize_ = nullptr;  // 水平调整大小光标（分割线）
    SDL_Cursor* cursor_ns_resize_ = nullptr;  // 垂直调整大小光标（面板边界）
    SDL_SystemCursor current_cursor_type_ = SDL_SYSTEM_CURSOR_DEFAULT;  // 当前光标类型

    /**
     * @brief 初始化系统光标
     */
    void InitSystemCursors();

    /**
     * @brief 销毁系统光标
     */
    void DestroySystemCursors();

    /**
     * @brief 设置系统光标类型
     * @param cursor_type 光标类型
     */
    void SetSystemCursor(SDL_SystemCursor cursor_type);

    /**
     * @brief 更新鼠标光标样式（根据悬停元素）
     * @param hit_result Hit Testing 结果
     * @param window_id 窗口ID
     */
    void UpdateMouseCursor(const HitTestResult& hit_result, Uint32 window_id);
};

} // namespace lightui

