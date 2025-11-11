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
     * @brief 构造函数
     */
    EventLoop();
    
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
     * @brief 获取任务调度器
     * 
     * @return TaskScheduler& 任务调度器引用
     */
    TaskScheduler& GetTaskScheduler();

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
     * @brief 发送事件到元素集合的差集
     *
     * 参考：RmlUi/Source/Core/Context.cpp - SendEvents
     *
     * @param old_items 旧元素集合
     * @param new_items 新元素集合
     * @param event_type 事件类型
     * @param mouse_x 鼠标X坐标
     * @param mouse_y 鼠标Y坐标
     */
    void SendEvents(const std::unordered_set<Element*>& old_items,
                   const std::unordered_set<Element*>& new_items,
                   const std::string& event_type,
                   float mouse_x,
                   float mouse_y);

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
    std::unique_ptr<TaskScheduler> task_scheduler_;

    // Hover链追踪（参考RmlUi的hover_chain）
    // 存储当前鼠标悬停的元素链（从目标元素到根元素）
    std::unordered_set<Element*> hover_chain_;

    // 当前悬停的元素（最深层的元素）
    Element* hover_element_ = nullptr;
};

} // namespace lightui

