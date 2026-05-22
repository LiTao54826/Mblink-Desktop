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

// Windows: 必须在 Skia 头文件之前 include windows.h
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// 取消 Windows 头文件中可能与项目代码冲突的宏
#ifdef ERROR
#undef ERROR
#endif
#endif

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <SDL3/SDL.h>
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "repaint_reason.h"
#include "window_event.h"
#include "display_backend.h"

namespace mbink {

// 前向声明
class Document;
class Renderer;
class DOMObserver;
class LayoutEngine;
class RenderObject;
class Node;
class AnimationTimeline;
class AnimationController;
class AnimationApplicator;
class RenderTreeBuilder;
class RenderPipeline;        // 统一渲染管线
class RenderTreeSynchronizer;
class IncrementalLayoutManager;  // 增量布局管理器
class FBOManager;
class WindowRenderer;        // 窗口渲染器
class FocusManager;          // 焦点管理器

/**
 * @brief 渲染后端类型
 */
enum class RenderBackend {
    AUTO,       // 自动选择（优先 OpenGL）
    OPENGL,     // OpenGL 硬件加速
    CPU,        // CPU 软件渲染（无头模式）
    SOFTWARE    // SDL 软件渲染
};

struct WindowConfig {
    std::string title = "MBink Window";
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
    bool transparent = false;  // 透明窗口（用于不规则窗体，需配合 borderless 使用）
    bool vsync = true;  // 启用 VSync
    int fps_limit = 60;
    RenderBackend backend = RenderBackend::AUTO;  // 渲染后端
    bool headless = false;  // 无头模式（不创建窗口，仅渲染到内存）
    bool gpu = true;  // 是否启用GPU加速（false时强制CPU渲染，适用于小挂件等轻量应用减少内存占用）
    int resize_border_width = 8;  // 无边框窗口的调整大小边缘宽度（像素）
    int min_width = 0;   // 窗口最小宽度（0 表示不限制）
    int min_height = 0;  // 窗口最小高度（0 表示不限制）
    int max_width = 0;   // 窗口最大宽度（0 表示不限制）
    int max_height = 0;  // 窗口最大高度（0 表示不限制）
};

/**
 * @brief 窗口类
 *
 * 管理SDL窗口和Skia渲染上下文
 */
class Window : public std::enable_shared_from_this<Window> {
    // 允许 WindowRenderer 访问私有成员
    friend class WindowRenderer;

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
     * @brief 显示窗口并激活到前台
     */
    void ShowAndFocus();

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
     * @brief 获取是否可调整大小
     */
    bool IsResizable() const { return config_.resizable; }

    /**
     * @brief 获取是否无边框模式
     */
    bool IsBorderless() const { return config_.borderless; }

    /**
     * @brief 获取是否透明窗口
     */
    bool IsTransparent() const { return config_.transparent; }

    /**
     * @brief 获取调整大小边缘宽度
     */
    int GetResizeBorderWidth() const { return config_.resize_border_width; }

    /**
     * @brief 设置窗口最小尺寸
     * @param width 最小宽度（0 表示不限制）
     * @param height 最小高度（0 表示不限制）
     */
    void SetMinSize(int width, int height);

    /**
     * @brief 设置窗口最大尺寸
     * @param width 最大宽度（0 表示不限制）
     * @param height 最大高度（0 表示不限制）
     */
    void SetMaxSize(int width, int height);

    /**
     * @brief 获取窗口最小尺寸
     * @param width 最小宽度输出
     * @param height 最小高度输出
     */
    void GetMinSize(int* width, int* height) const;

    /**
     * @brief 获取窗口最大尺寸
     * @param width 最大宽度输出
     * @param height 最大高度输出
     */
    void GetMaxSize(int* width, int* height) const;

    /**
     * @brief 检测屏幕坐标处是否为拖拽区域（-webkit-app-region: drag）
     * @param screen_x 屏幕 X 坐标
     * @param screen_y 屏幕 Y 坐标
     * @return true 表示该位置是拖拽区域
     *
     * 用于无边框窗口的 WM_NCHITTEST 处理，遍历渲染树查询
     * 命中元素的 app_region 样式属性。
     */
    bool HitTestDragRegion(int screen_x, int screen_y) const;

    /**
     * @brief 命中测试窗口控制区域
     * @param screen_x 屏幕坐标X
     * @param screen_y 屏幕坐标Y
     * @return 窗口控制类型: "close", "minimize", "maximize", "pin", 或空字符串
     *
     * 用于无边框窗口的 WM_NCHITTEST 处理，遍历渲染树查询
     * 命中元素的 window_control 样式属性。
     */
    std::string HitTestWindowControl(int screen_x, int screen_y) const;

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
     * @brief 获取Skia渲染表面
     * @return Skia渲染表面
     */
    sk_sp<SkSurface> GetSurface() const { return surface_; }

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
     * @brief 获取实际使用的渲染后端
     * @return 渲染后端类型
     */
    RenderBackend GetRenderBackend() const { return actual_backend_; }

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
     * @brief 设置窗口关闭请求拦截器
     * @param handler 返回 true 表示已处理，不执行默认关闭
     */
    void SetOnCloseRequestHandler(std::function<bool()> handler) {
        on_close_request_handler_ = handler;
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
     * @brief 渲染文档到窗口（唯一渲染入口）
     *
     * 自动判断使用全量渲染或增量渲染：
     * - 首次渲染/渲染树失效 → 全量渲染
     * - 只有脏区域 → 增量渲染
     */
    void Render();

    bool CaptureCurrentFramePng(std::vector<uint8_t>* bytes,
                                int* width = nullptr,
                                int* height = nullptr,
                                std::string* error = nullptr);

    /**
     * @brief 清空画布
     * @param color 清空颜色（默认白色）
     */
    void Clear(uint32_t color = 0xFFFFFFFF);

    /**
     * @brief 标记需要重绘
     */
    void SetNeedsRepaint() {
        SetNeedsRepaintFor(RepaintReason::Unknown);
    }

    void SetNeedsRepaintFor(RepaintReason reason) {
        RecordRepaintReason(reason);
        needs_repaint_ = true;
        if (RepaintReasonMayAffectLayout(reason)) {
            layout_sync_valid_ = false;
        }
    }

    void MarkRepaintReason(RepaintReason reason) {
        RecordRepaintReason(reason);
    }

    RepaintReason GetLastRepaintReason() const { return last_repaint_reason_; }
    const char* GetLastRepaintReasonName() const { return RepaintReasonName(last_repaint_reason_); }
    uint64_t GetRepaintReasonCount() const { return repaint_reason_count_; }
    void ResetRepaintReasonCount() { repaint_reason_count_ = 0; }

    /**
     * @brief 强制同步布局
     *
     * 立即执行布局计算，用于 getBoundingClientRect 等需要最新布局信息的操作。
     * 这模拟了浏览器的强制 reflow 行为。
     */
    void ForceLayoutSync();

    /**
     * @brief 标记渲染树需要重建
     *
     * 同时清理动画状态，防止悬空指针问题。
     */
    void InvalidateRenderTree();

    /**
     * @brief 检查是否需要重绘
     * @return true表示需要重绘
     */
    bool NeedsRepaint() const;

    /**
     * @brief Restore the normal Skia cache budget after a resize burst.
     * @return true when this call changed cache state and queued a repaint.
     */
    bool RestoreResizeBurstCacheLimitIfReady();

    /**
     * @brief 添加脏区域（用于增量渲染）
     * @param rect 脏区域矩形
     */
    void AddDirtyRect(const SkRect& rect);

    /**
     * @brief 获取脏区域列表
     * @return 脏区域列表
     */
    const std::vector<SkRect>& GetDirtyRects() const { return dirty_rects_; }

    /**
     * @brief 清空脏区域
     */
    void ClearDirtyRects() { dirty_rects_.clear(); }

    /**
     * @brief 获取缓存的渲染树
     * @return 渲染树根节点，如果没有则返回nullptr
     */
    std::shared_ptr<RenderObject> GetCachedRenderTree() const { return cached_render_tree_; }

    /**
     * @brief 设置是否启用增量渲染
     * @param enable true启用增量渲染（局部裁剪），false则始终全屏重绘
     */
    void SetEnableIncrementalRender(bool enable) { enable_incremental_render_ = enable; }

    /**
     * @brief 获取是否启用增量渲染
     */
    bool IsIncrementalRenderEnabled() const { return enable_incremental_render_; }

    /**
     * @brief 设置是否强制全屏重绘（调试用）
     * @param force true强制全屏重绘，但保留渲染树缓存
     */
    void SetForceFullRepaint(bool force) { force_full_repaint_ = force; }

    /**
     * @brief 获取是否强制全屏重绘
     */
    bool IsForceFullRepaint() const { return force_full_repaint_; }

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
     * @brief 获取动画应用器
     * @return 动画应用器指针
     */
    AnimationApplicator* GetAnimationApplicator() const { return animation_applicator_.get(); }

    /**
     * @brief 获取布局引擎
     * @return 布局引擎指针
     *
     * **Feature: incremental-layout-optimization**
     */
    LayoutEngine* GetLayoutEngine() const { return layout_engine_.get(); }

    /**
     * @brief 获取统一渲染管线
     * @return 渲染管线指针
     */
    RenderPipeline* GetRenderPipeline() const { return render_pipeline_.get(); }

    /**
     * @brief 投递任务到窗口所属 UI 线程执行
     */
    void PostUiTask(std::function<void()> task);

    /**
     * @brief 执行已投递的 UI 线程任务
     */
    void FlushUiTasks(size_t max_tasks = 64);

    /**
     * @brief 获取渲染树同步器
     * @return 渲染树同步器指针
     *
     * **Feature: incremental-update-system**
     */
    RenderTreeSynchronizer* GetRenderTreeSynchronizer() const { return render_tree_synchronizer_.get(); }

    /**
     * @brief 获取增量布局管理器
     * @return 增量布局管理器指针
     *
     * **Feature: incremental-layout-boundary**
     */
    IncrementalLayoutManager* GetIncrementalLayoutManager() const { return incremental_layout_manager_.get(); }

    // ========== 动画和渲染 ==========

    /**
     * @brief 更新动画（在渲染循环中调用）
     * @param current_time 当前时间（秒）
     */
    void UpdateAnimations(double current_time);

    /**
     * @brief 递归应用动画到渲染树
     * @param root 渲染树根节点
     */
    void ApplyAnimationsToRenderTree(RenderObject* root);

    /**
     * @brief 检查渲染树中是否有待启动的动画
     * @param root 渲染树根节点
     * @return 如果有待启动的动画返回 true
     */
    bool HasPendingAnimations(RenderObject* root) const;

    /**
     * @brief 获取 DPI 缩放比
     * @return DPI 缩放比（例如：1.0, 1.5, 2.0）
     */
    float GetDisplayScale() const;

    /**
     * @brief 将逻辑像素转换为物理像素
     */
    int LogicalToPhysicalPixels(int value) const;
    void GetPhysicalSize(int* width, int* height) const;
    RenderBackend GetActualBackend() const;
    bool HasSurface() const;
    bool HasGrContext() const;
    bool HasFBOManager() const;
    size_t GetEstimatedSurfaceBytes() const;
    size_t GetEstimatedFBOTextureBytes() const;
    size_t GetEstimatedFBODepthStencilBytes() const;
    size_t GetEstimatedFBOTotalBytes() const;
    size_t GetSkiaResourceCacheBytes() const;
    int GetSkiaResourceCacheCount() const;
    size_t GetSkiaResourceCacheLimit() const;
    void PurgeSkiaResourceCache();

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
     * @brief 渲染 DevTools 面板
     * @param canvas Skia 画布
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    void RenderDevTools(SkCanvas* canvas, float width, float height);

private:
    void RecordRepaintReason(RepaintReason reason) {
        if (repaint_reason_count_ == 0 || last_repaint_reason_ != reason) {
            last_repaint_reason_ = reason;
            repaint_reason_count_ = 1;
        } else {
            ++repaint_reason_count_;
        }
    }

    WindowConfig config_;
    SDL_Window* sdl_window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;  // SDL Renderer for CPU mode
    SDL_Texture* sdl_texture_ = nullptr;    // SDL Texture for CPU mode
    SDL_Surface* sdl_surface_ = nullptr;    // SDL Surface for direct rendering (no SDL_Renderer)
    sk_sp<GrDirectContext> gr_context_;
    sk_sp<SkSurface> surface_;
    sk_sp<SkSurface> retained_main_surface_;
    int retained_main_width_px_ = 0;
    int retained_main_height_px_ = 0;
    bool retained_main_has_content_ = false;
    bool should_close_ = false;
    RenderBackend actual_backend_ = RenderBackend::AUTO;  // 实际使用的渲染后端
    bool ui_tasks_accepting_ = true;
    std::mutex ui_tasks_mutex_;
    std::vector<std::function<void()>> ui_tasks_;

    // 事件回调（简单回调）
    std::function<void(int, int)> on_resize_callback_;
    std::function<void(int, int)> on_move_callback_;
    std::function<void()> on_close_callback_;
    std::function<bool()> on_close_request_handler_;
    std::function<void()> on_focus_callback_;
    std::function<void()> on_blur_callback_;

    // 事件监听器（支持多个监听器）
    std::unordered_map<WindowEventType, std::vector<std::function<void(const WindowEvent&)>>> event_listeners_;

    // 渲染集成
    std::shared_ptr<Document> document_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<DOMObserver> dom_observer_;  // DOM 观察者
    bool needs_repaint_ = true;  // 初始需要绘制
    RepaintReason last_repaint_reason_ = RepaintReason::Initial;
    uint64_t repaint_reason_count_ = 1;

    // Week 2: 增量渲染优化
    std::shared_ptr<RenderObject> cached_render_tree_;  // 缓存的渲染树
    bool render_tree_valid_ = false;  // 渲染树是否有效
    bool layout_sync_valid_ = false;  // 已缓存布局是否与当前 DOM/样式/视口同步
    std::vector<SkRect> dirty_rects_;  // 脏区域列表（用于局部重绘）
    SkRect last_dirty_bounds_;  // 上一帧的脏区域边界（用于 CPU 模式局部更新）
    bool has_dirty_bounds_ = false;  // 是否有脏区域边界

    // 增量渲染控制开关
    bool enable_incremental_render_ = true;  // 启用增量渲染（局部裁剪）
    bool force_full_repaint_ = false;         // 强制全屏重绘（调试用，但保留渲染树缓存）

    // 渲染树构建器
    std::shared_ptr<RenderTreeBuilder> render_tree_builder_;

    // CSS Transition 动画时间轴
    std::unique_ptr<AnimationTimeline> animation_timeline_;

    // CSS Animation 动画控制器
    std::unique_ptr<AnimationController> animation_controller_;

    // CSS Animation 动画应用器
    std::unique_ptr<AnimationApplicator> animation_applicator_;

    // Taffy CSS 布局引擎
    std::unique_ptr<LayoutEngine> layout_engine_;

    // 统一渲染管线
    std::unique_ptr<RenderPipeline> render_pipeline_;
    std::shared_ptr<RenderTreeSynchronizer> render_tree_synchronizer_;

    // 增量布局管理器
    std::unique_ptr<IncrementalLayoutManager> incremental_layout_manager_;

    // 窗口渲染器（负责动画和渲染辅助方法）
    std::unique_ptr<WindowRenderer> window_renderer_;

    // 显示后端（用于 CPU 渲染模式）
    std::unique_ptr<DisplayBackend> display_backend_;

    // FBO 管理器（用于 GPU 增量渲染）
    std::unique_ptr<FBOManager> fbo_manager_;
    bool use_fbo_incremental_ = false;  // 是否使用 FBO 增量渲染（暂时禁用，滚动时有问题）
    bool fbo_needs_full_paint_ = true; // FBO 是否需要首次全量绘制

    float last_body_scroll_x_ = 0.0f;  // 上一帧的 body 滚动位置
    float last_body_scroll_y_ = 0.0f;

    // 缓存的 body 背景色（用于 resize 时清除缓冲区）
    SkColor cached_body_bg_color_ = SK_ColorWHITE;

    // 待处理的 resize（用于节流后处理最后一次 resize）
    int pending_resize_width_ = 0;
    int pending_resize_height_ = 0;
    bool has_pending_resize_ = false;

    // 连续 resize 追踪（用于 burst 场景更激进回收）
    Uint64 resize_burst_window_start_tick_ = 0;
    Uint64 resize_burst_last_tick_ = 0;
    int resize_burst_count_ = 0;
    bool resize_burst_cache_limited_ = false;

    // 保存的滚动位置（用于 InvalidateRenderTree 后恢复）
    std::unordered_map<Node*, std::pair<float, float>> saved_scroll_positions_;

    // FocusManager 引用（不拥有所有权，由 EventLoop 管理）
    FocusManager* focus_manager_ = nullptr;

public:
    /**
     * @brief 设置 FocusManager
     * @param focus_manager FocusManager 指针
     */
    void SetFocusManager(FocusManager* focus_manager) { focus_manager_ = focus_manager; }

    /**
     * @brief 获取 FocusManager
     * @return FocusManager 指针
     */
    FocusManager* GetFocusManager() const { return focus_manager_; }
};

} // namespace mbink

