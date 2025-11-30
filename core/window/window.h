/**
 * @file window.h
 * @brief 窗口管理模块
 * 
 * 功能：
 * - 使用SDL3创建和管理应用窗口
 * - 初始化OpenGL/Metal/Vulkan上下文
 * - 处理窗口事件（调整大小、关闭等）
 * - 提供Skia渲染表面
 * 
 * 依赖：
 * - SDL3
 * - Skia (GrDirectContext)
 * 
 * 实现要点：
 * - 跨平台窗口创建（Windows/macOS/Linux）
 * - 支持OpenGL/Metal/Vulkan后端
 * - 窗口属性管理（标题、大小、位置）
 * - VSync和帧率控制
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "window_event.h"
#include "display_backend.h"

namespace lightui {

// 前向声明
class Document;
class Renderer;
class DOMObserver;
class LayoutEngine;
class RenderObject;
class Node;
class AnimationTimeline;
class AnimationController;

/**
 * @brief 渲染后端类型
 */
enum class RenderBackend {
    AUTO,       // 自动选择（优先 OpenGL）
    OPENGL,     // OpenGL 硬件加速
    CPU,        // CPU 软件渲染（无头模式）
    SOFTWARE    // SDL 软件渲染
};

/**
 * @brief 窗口配置
 */
struct WindowConfig {
    std::string title = "LightUI Window";
    int width = 800;
    int height = 600;
    int x = -1;  // -1表示居中
    int y = -1;
    bool resizable = true;
    bool fullscreen = false;
    bool borderless = false;
    bool maximized = false;
    bool minimized = false;
    bool hidden = false;
    bool always_on_top = false;
    bool high_dpi = true;
    bool vsync = true;  // 启用 VSync
    int fps_limit = 60;
    RenderBackend backend = RenderBackend::AUTO;  // 渲染后端
    bool headless = false;  // 无头模式（不创建窗口，仅渲染到内存）
};

/**
 * @brief 窗口类
 * 
 * 管理SDL窗口和Skia渲染上下文
 */
class Window {
public:
    /**
     * @brief 构造函数
     * @param config 窗口配置
     */
    explicit Window(const WindowConfig& config);
    
    /**
     * @brief 析构函数
     */
    ~Window();
    
    // 禁止拷贝
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    /**
     * @brief 显示窗口
     */
    void Show();
    
    /**
     * @brief 隐藏窗口
     */
    void Hide();
    
    /**
     * @brief 获取窗口标题
     * @return 窗口标题
     */
    std::string GetTitle() const;

    /**
     * @brief 设置窗口标题
     * @param title 新标题
     */
    void SetTitle(const std::string& title);

    /**
     * @brief 设置窗口大小
     * @param width 宽度
     * @param height 高度
     */
    void SetSize(int width, int height);
    
    /**
     * @brief 获取窗口大小
     * @param width 宽度输出
     * @param height 高度输出
     */
    void GetSize(int* width, int* height) const;

    /**
     * @brief 设置窗口位置
     * @param x X坐标
     * @param y Y坐标
     */
    void SetPosition(int x, int y);

    /**
     * @brief 获取窗口位置
     * @param x X坐标输出
     * @param y Y坐标输出
     */
    void GetPosition(int* x, int* y) const;

    /**
     * @brief 最小化窗口
     */
    void Minimize();

    /**
     * @brief 最大化窗口
     */
    void Maximize();

    /**
     * @brief 恢复窗口（从最小化/最大化状态）
     */
    void Restore();

    /**
     * @brief 设置全屏模式
     * @param fullscreen 是否全屏
     */
    void SetFullscreen(bool fullscreen);

    /**
     * @brief 设置窗口可调整大小
     * @param resizable 是否可调整大小
     */
    void SetResizable(bool resizable);

    /**
     * @brief 设置无边框模式
     * @param borderless 是否无边框
     */
    void SetBorderless(bool borderless);

    /**
     * @brief 设置窗口置顶
     * @param on_top 是否置顶
     */
    void SetAlwaysOnTop(bool on_top);

    /**
     * @brief 获取SDL窗口句柄
     * @return SDL窗口指针
     */
    SDL_Window* GetSDLWindow() const { return sdl_window_; }
    
    /**
     * @brief 获取Skia画布
     * @return Skia画布指针
     */
    SkCanvas* GetCanvas() const;
    
    /**
     * @brief 获取Skia上下文
     * @return Skia上下文
     */
    GrDirectContext* GetGrContext() const { return gr_context_.get(); }

    /**
     * @brief 获取 PaintMode 显示后端（如果正在使用）
     * @return PaintModeDisplayBackend 指针，如果不是 PaintMode 模式则返回 nullptr
     */
    PaintModeDisplayBackend* GetPaintModeBackend() const;

    /**
     * @brief 交换缓冲区（显示渲染结果）
     */
    void SwapBuffers();
    
    /**
     * @brief 处理窗口调整大小
     */
    void OnResize();
    
    /**
     * @brief 检查窗口是否应该关闭
     * @return true表示应该关闭
     */
    bool ShouldClose() const { return should_close_; }
    
    /**
     * @brief 设置关闭标志
     */
    void SetShouldClose(bool should_close) { should_close_ = should_close; }

    /**
     * @brief 设置窗口大小调整回调
     * @param callback 回调函数
     */
    void SetOnResizeCallback(std::function<void(int, int)> callback) {
        on_resize_callback_ = callback;
    }

    /**
     * @brief 设置窗口移动回调
     * @param callback 回调函数
     */
    void SetOnMoveCallback(std::function<void(int, int)> callback) {
        on_move_callback_ = callback;
    }

    /**
     * @brief 设置窗口关闭回调
     * @param callback 回调函数
     */
    void SetOnCloseCallback(std::function<void()> callback) {
        on_close_callback_ = callback;
    }

    /**
     * @brief 设置窗口获得焦点回调
     * @param callback 回调函数
     */
    void SetOnFocusCallback(std::function<void()> callback) {
        on_focus_callback_ = callback;
    }

    /**
     * @brief 设置窗口失去焦点回调
     * @param callback 回调函数
     */
    void SetOnBlurCallback(std::function<void()> callback) {
        on_blur_callback_ = callback;
    }

    /**
     * @brief 处理SDL事件
     * @param event SDL事件
     * @return true表示事件已处理
     */
    bool HandleSDLEvent(const SDL_Event& event);

    /**
     * @brief 分发窗口事件
     * @param event 窗口事件
     */
    void DispatchWindowEvent(const WindowEvent& event);

    /**
     * @brief 添加窗口事件监听器
     * @param type 事件类型
     * @param listener 监听器函数
     */
    void AddEventListener(WindowEventType type, std::function<void(const WindowEvent&)> listener);

    /**
     * @brief 移除所有指定类型的事件监听器
     * @param type 事件类型
     */
    void RemoveEventListeners(WindowEventType type);

    // ========== 渲染集成 ==========

    /**
     * @brief 设置文档
     * @param document 文档对象
     */
    void SetDocument(std::shared_ptr<Document> document);

    /**
     * @brief 获取文档
     * @return 文档对象
     */
    std::shared_ptr<Document> GetDocument() const { return document_; }

    /**
     * @brief 渲染文档到窗口（全量渲染 - 旧版本）
     */
    void RenderDocument();

    /**
     * @brief 增量渲染文档到窗口（Week 2优化版本）
     *
     * 优化策略:
     * - 缓存渲染树，只在DOM结构改变时重建
     * - 收集脏区域，只渲染改变的部分
     * - 增量布局，只重新布局脏子树
     */
    void RenderDocumentIncremental();

    /**
     * @brief 清空画布
     * @param color 清空颜色（默认白色）
     */
    void Clear(uint32_t color = 0xFFFFFFFF);

    /**
     * @brief 标记需要重绘
     */
    void SetNeedsRepaint() {
        needs_repaint_ = true;
    }

    /**
     * @brief 标记渲染树需要重建
     */
    void InvalidateRenderTree() { render_tree_valid_ = false; }

    /**
     * @brief 检查是否需要重绘
     * @return true表示需要重绘
     */
    bool NeedsRepaint() const { return needs_repaint_; }

    /**
     * @brief 获取缓存的渲染树
     * @return 渲染树根节点，如果没有则返回nullptr
     */
    std::shared_ptr<RenderObject> GetCachedRenderTree() const { return cached_render_tree_; }

    /**
     * @brief 确保渲染树已构建（如果无效则重建）
     */
    void EnsureRenderTree();

    /**
     * @brief 获取动画时间轴
     * @return 动画时间轴指针
     */
    AnimationTimeline* GetAnimationTimeline() const { return animation_timeline_.get(); }

    /**
     * @brief 获取动画控制器
     * @return 动画控制器指针
     */
    AnimationController* GetAnimationController() const { return animation_controller_.get(); }

    /**
     * @brief 更新动画（在渲染循环中调用）
     * @param current_time 当前时间（秒）
     */
    void UpdateAnimations(double current_time);

    /**
     * @brief 获取 DPI 缩放比
     * @return DPI 缩放比（例如：1.0, 1.5, 2.0）
     */
    float GetDisplayScale() const;

private:
    /**
     * @brief 初始化SDL
     */
    void InitSDL();
    
    /**
     * @brief 创建SDL窗口
     */
    void CreateSDLWindow();
    
    /**
     * @brief 初始化OpenGL上下文
     */
    void InitOpenGL();
    
    /**
     * @brief 初始化Skia
     */
    void InitSkia();
    
    /**
     * @brief 创建Skia渲染表面
     */
    void CreateSkiaSurface();

    /**
     * @brief 初始化 CPU 软件渲染
     */
    void InitCPURendering();

    /**
     * @brief 递归清除DOM节点的脏标记
     * @param node 要清除的节点
     */
    void ClearDirtyFlags(Node* node);

    /**
     * @brief 增量布局：只布局需要布局的子树
     * @param render_obj 渲染对象
     * @param parent_width 父元素宽度
     * @param parent_height 父元素高度
     * @return true表示该节点或其子节点被重新布局
     */
    bool LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height);

    /**
     * @brief 将DOM节点的脏标记传播到对应的RenderObject
     * @param dom_node DOM节点
     * @param render_obj 渲染对象
     */
    void MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj);

    /**
     * @brief 保存渲染树中所有元素的滚动位置
     * @param render_obj 渲染对象
     * @param scroll_positions 滚动位置映射表（DOM节点指针 -> 滚动位置）
     */
    void SaveScrollPositions(RenderObject* render_obj,
                             std::unordered_map<Node*, std::pair<float, float>>& scroll_positions);

    /**
     * @brief 恢复渲染树中元素的滚动位置
     * @param render_obj 渲染对象
     * @param scroll_positions 滚动位置映射表
     */
    void RestoreScrollPositions(RenderObject* render_obj,
                                const std::unordered_map<Node*, std::pair<float, float>>& scroll_positions);

private:
    WindowConfig config_;
    SDL_Window* sdl_window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;  // SDL Renderer for CPU mode
    SDL_Texture* sdl_texture_ = nullptr;    // SDL Texture for CPU mode
    SDL_Surface* sdl_surface_ = nullptr;    // SDL Surface for direct rendering (no SDL_Renderer)
    sk_sp<GrDirectContext> gr_context_;
    sk_sp<SkSurface> surface_;
    bool should_close_ = false;
    RenderBackend actual_backend_ = RenderBackend::AUTO;  // 实际使用的渲染后端

    // 事件回调（简单回调）
    std::function<void(int, int)> on_resize_callback_;
    std::function<void(int, int)> on_move_callback_;
    std::function<void()> on_close_callback_;
    std::function<void()> on_focus_callback_;
    std::function<void()> on_blur_callback_;

    // 事件监听器（支持多个监听器）
    std::unordered_map<WindowEventType, std::vector<std::function<void(const WindowEvent&)>>> event_listeners_;

    // 渲染集成
    std::shared_ptr<Document> document_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<DOMObserver> dom_observer_;  // DOM 观察者
    bool needs_repaint_ = true;  // 初始需要绘制

    // Week 2: 增量渲染优化
    std::shared_ptr<RenderObject> cached_render_tree_;  // 缓存的渲染树
    bool render_tree_valid_ = false;  // 渲染树是否有效

    // CSS Transition 动画时间轴
    std::unique_ptr<AnimationTimeline> animation_timeline_;

    // CSS Animation 动画控制器
    std::unique_ptr<AnimationController> animation_controller_;

    // Taffy CSS 布局引擎
    std::unique_ptr<LayoutEngine> layout_engine_;

    // 显示后端（用于 CPU 渲染模式）
    std::unique_ptr<DisplayBackend> display_backend_;
};

} // namespace lightui

