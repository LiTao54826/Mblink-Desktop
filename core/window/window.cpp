/**
 * @file window.cpp
 * @brief 窗口管理模块实现
 *
 * @note 大文件说明 (3352 行)
 * 本文件包含 Window 类的完整实现，是应用程序的核心窗口管理组件。
 * 文件较大的原因：
 * 1. 包含 SDL3 窗口创建和生命周期管理
 * 2. 包含 OpenGL/CPU 渲染后端初始化
 * 3. 包含 Skia 渲染表面管理
 * 4. 包含 DOM 观察者实现 (WindowDOMObserver)
 * 5. 包含渲染管线集成
 * 6. 包含 Windows 平台特定代码 (子类化窗口)
 *
 * 计划重构：
 * - 提取 WindowDOMObserver 到独立文件
 * - 提取渲染相关代码到 WindowRenderer 类
 * - 提取平台特定代码到 platform/ 子目录
 * 参见: .kiro/specs/code-structure-refactoring/tasks.md Phase 5
 *
 * 实现内容：
 * - SDL3窗口创建和管理
 * - OpenGL上下文初始化
 * - Skia渲染表面创建
 * - 窗口事件处理
 */

// 性能优化：默认关闭调试日志（可以通过定义MBINK_DEBUG_RENDERING启用）
// #define MBINK_DEBUG_RENDERING

#ifdef MBINK_DEBUG_RENDERING
    #define DEBUG_LOG_FLUSH() ((void)0)
#else
    #define DEBUG_LOG(msg) ((void)0)
    #define DEBUG_LOG_FLUSH() ((void)0)
#endif

#include "window.h"
#include "window_dom_observer.h"
#include "window_renderer.h"
#ifdef _WIN32
#include "window_win32.h"
#endif
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "include/core/SkRefCnt.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkFont.h"
#include "include/core/SkRegion.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/observers/dom_observer.h"
#include "core/render/pipeline/renderer.h"
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"
#include "core/render/text/font_manager.h"
#include "core/render/utils/dirty_region.h"
#include "core/render/utils/dirty_region_collector.h"
#include "core/render/animation/transition.h"
#include "core/render/animation/animation_timeline.h"
#include "core/render/animation/animation_controller.h"
#include "core/render/animation/animation_applicator.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/pipeline/render_tree_synchronizer.h"
#include "core/layout/layout_engine.h"
#include "core/layout/native_layout_engine.h"
#include "core/layout/incremental_layout_manager.h"
#include "core/render/utils/color.h"
#include "core/render/objects/select_dropdown.h"
#include "core/render/layer/paint_layer.h"
#include "core/utils/encoding_utils.h"
#include "core/devtools/devtools_manager.h"
#include "core/lexbor/style_manager.h"
#include "core/render/layer/fbo_manager.h"
#include "core/render/image/image_cache.h"
#include "core/event/input/hit_test_controller.h"

namespace mbink {

// 从 render_object.cpp 导入的绘制统计变量
extern std::atomic<int> g_paint_total_calls;
extern std::atomic<int> g_paint_culled_calls;



// 静态成员：SDL初始化计数器
static int sdl_init_count = 0;

// P0 内存优化参数
static constexpr int kResizeDebounceMs = 120;
static constexpr size_t kSkiaResizeCacheLimitBytes = 48 * 1024 * 1024;      // 48MB
static constexpr size_t kSkiaRestoreCacheLimitBytes = 32 * 1024 * 1024;     // 32MB
static constexpr size_t kImageCacheShrinkBytes = 48 * 1024 * 1024;          // 48MB

// P1 连续 resize 累积治理参数
static constexpr int kResizeBurstWindowMs = 1200;  // 连续 resize 视窗
static constexpr int kResizeBurstThreshold = 6;     // 视窗内触发次数阈值
static constexpr size_t kSkiaBurstCacheLimitBytes = 24 * 1024 * 1024;  // 24MB
static constexpr size_t kImageCacheBurstShrinkBytes = 24 * 1024 * 1024; // 24MB


namespace {
inline bool IsAnimFrameDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_FRAME") != nullptr);
    return enabled;
}

inline float NormalizeScale(float scale) {
    return scale > 0.0f ? scale : 1.0f;
}

inline int ScaleCssToWindowUnits(int value, float content_scale) {
    if (value <= 0) {
        return value;
    }
    return std::max(static_cast<int>(std::lround(static_cast<float>(value) * NormalizeScale(content_scale))), 1);
}

inline int ScaleWindowUnitsToCss(int value, float content_scale) {
    if (value <= 0) {
        return value;
    }
    return std::max(static_cast<int>(std::lround(static_cast<float>(value) / NormalizeScale(content_scale))), 1);
}

inline float GetDisplayContentScaleSafe(SDL_DisplayID display_id) {
    if (display_id == 0) {
        return 1.0f;
    }

    const float scale = SDL_GetDisplayContentScale(display_id);
    return NormalizeScale(scale);
}

inline SDL_DisplayID GetTargetDisplayForConfig(const WindowConfig& config) {
    if (config.x >= 0 && config.y >= 0) {
        SDL_Point point{config.x, config.y};
        const SDL_DisplayID display_id = SDL_GetDisplayForPoint(&point);
        if (display_id != 0) {
            return display_id;
        }
    }

    return SDL_GetPrimaryDisplay();
}

inline float GetWindowContentScale(SDL_Window* window) {
    if (!window) {
        return 1.0f;
    }

    const SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
    return GetDisplayContentScaleSafe(display_id);
}

inline bool ShouldCreateOpenGLWindow(const WindowConfig& config) {
    if (config.transparent || !config.gpu) {
        return false;
    }

    return config.backend != RenderBackend::CPU &&
           config.backend != RenderBackend::SOFTWARE;
}

}

// SDL 事件过滤器：过滤掉可能导致闪烁的事件
// 返回 true 表示保留事件，返回 false 表示丢弃事件
static bool SDLCALL SDLEventFilter(void* userdata, SDL_Event* event) {
    (void)userdata;
    // 过滤掉 EXPOSED 事件，避免在 Windows 上触发闪烁
    if (event->type == SDL_EVENT_WINDOW_EXPOSED) {
        return false;  // 丢弃此事件
    }
    return true;  // 保留其他事件
}

Window::Window(const WindowConfig& config) : config_(config) {
    // 透明窗口（不规则窗体）本质上没有系统标题栏，必须启用无边框模式
    // 否则 WM_NCHITTEST 中的自定义 hit test（拖拽区域、窗口控制按钮）不会生效
    if (config_.transparent) {
        config_.borderless = true;
    }

    InitSDL();
    CreateSDLWindow();

#ifdef _WIN32
    // 检查调试环境变量
    if (getenv("MBINK_DEBUG_MESSAGES")) {
        win32::SetDebugMessages(true);
    }

    // Windows: 子类化窗口以拦截 WM_PAINT 和 WM_ERASEBKGND，防止闪烁
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdl_window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
        win32::SubclassWindow(hwnd, this);

        // 无边框窗口：启用 DWM 阴影效果
        // 透明窗口（不规则窗体）不需要系统阴影，阴影会破坏不规则形状
        if (config_.borderless && !config_.transparent) {
            win32::EnableBorderlessShadow(hwnd);
        }
    }
#endif

    // 根据配置选择渲染后端
    if (config_.transparent) {
        // 透明窗口（不规则窗体）：强制使用 CPU 渲染 + LayeredWindow 后端
        // 因为 WS_EX_LAYERED + UpdateLayeredWindow 与 OpenGL 不兼容
        InitCPURendering();
        actual_backend_ = RenderBackend::CPU;
    } else if (!config_.gpu) {
        // GPU加速已关闭：强制使用 CPU 渲染，适用于小挂件等轻量应用减少内存占用
        InitCPURendering();
        actual_backend_ = RenderBackend::CPU;
    } else if (config_.backend == RenderBackend::AUTO) {
        // 自动模式：先尝试 GPU，失败则降级到 CPU
        try {
            InitOpenGL();
            InitSkia();
            CreateSkiaSurface();
            actual_backend_ = RenderBackend::OPENGL;
        } catch (const std::exception& e) {
            // GPU 初始化失败，降级到 CPU 软件渲染
            InitCPURendering();
            actual_backend_ = RenderBackend::CPU;
        }
    } else if (config_.backend == RenderBackend::OPENGL) {
        // 仅 GPU 模式
        InitOpenGL();
        InitSkia();
        CreateSkiaSurface();
        actual_backend_ = RenderBackend::OPENGL;
    } else if (config_.backend == RenderBackend::CPU) {
        // 仅 CPU 模式
        InitCPURendering();
        actual_backend_ = RenderBackend::CPU;
    }


    // 初始化动画时间轴
    animation_timeline_ = std::make_unique<AnimationTimeline>();

    // 初始化动画控制器
    animation_controller_ = std::make_unique<AnimationController>();

    // 初始化动画应用器
    animation_applicator_ = std::make_unique<AnimationApplicator>(*animation_controller_);

    // 初始化 Taffy CSS 布局引擎
    layout_engine_ = std::make_unique<LayoutEngine>();

    // 初始化 FBO 管理器（用于 GPU 增量渲染）
    if (actual_backend_ == RenderBackend::OPENGL && gr_context_) {
        int physical_width, physical_height;
        SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

        fbo_manager_ = std::make_unique<FBOManager>();
        if (!fbo_manager_->Initialize(physical_width, physical_height, gr_context_.get())) {
            fbo_manager_.reset();
            use_fbo_incremental_ = false;
        } else {
        }
    }

    // 初始化统一渲染管线
    render_pipeline_ = std::make_unique<RenderPipeline>();

    // 初始化窗口渲染器
    window_renderer_ = std::make_unique<WindowRenderer>(this);

    // 初始化视口尺寸（使用 DPI 缩放后的逻辑尺寸）
    // 这确保 position: fixed 元素在首次布局时能正确使用视口尺寸
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
    float dpi_scale = GetDisplayScale();
    float logical_width = static_cast<float>(physical_width) / dpi_scale;
    float logical_height = static_cast<float>(physical_height) / dpi_scale;
    RenderObject::SetViewportSize(logical_width, logical_height);
}

Window::~Window() {
    // 关键修复：在释放 Skia 资源之前，先激活 OpenGL 上下文
    // Skia 的 GrContext 在释放时需要调用 OpenGL 清理函数
    if (gl_context_ && sdl_window_) {
        SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    }

    on_resize_callback_ = {};
    on_move_callback_ = {};
    on_close_callback_ = {};
    on_close_request_handler_ = {};
    on_focus_callback_ = {};
    on_blur_callback_ = {};
    event_listeners_.clear();

    document_.reset();

    renderer_.reset();

    // 释放统一渲染管线（在释放其他资源之前）
    if (render_pipeline_) {
        render_pipeline_->Shutdown();
        render_pipeline_.reset();
    }

    // 释放 DisplayBackend（在销毁窗口之前）
    if (display_backend_) {
        display_backend_->Shutdown();
        display_backend_.reset();
    }

    // 释放 FBO 管理器（在释放 Skia 资源之前）
    fbo_manager_.reset();

    // 释放Skia资源
    surface_.reset();

    gr_context_.reset();

    // 注意：sdl_surface_ 不需要手动销毁，它由 SDL_DestroyWindow 自动处理
    sdl_surface_ = nullptr;

#ifdef _WIN32
    // 移除窗口子类化
    if (sdl_window_) {
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdl_window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwnd) {
            win32::UnsubclassWindow(hwnd);
        }
    }
#endif

    // 销毁OpenGL上下文
    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }

    // 销毁SDL窗口
    if (sdl_window_) {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }

    // 清理SDL（如果是最后一个窗口）
    sdl_init_count--;
    if (sdl_init_count == 0) {
        SDL_Quit();
    }

    dom_observer_.reset();

    cached_render_tree_.reset();
    render_tree_builder_.reset();
    render_tree_synchronizer_.reset();

    animation_timeline_.reset();
    animation_controller_.reset();
    animation_applicator_.reset();
    layout_engine_.reset();
    incremental_layout_manager_.reset();

    window_renderer_.reset();

    dirty_rects_.clear();
    saved_scroll_positions_.clear();
    focus_manager_ = nullptr;
}

void Window::Show() {
    if (sdl_window_) {
        SDL_ShowWindow(sdl_window_);
        SetForceFullRepaint(true);
        SetNeedsRepaint();
    }
}

void Window::ShowAndFocus() {
    if (!sdl_window_) {
        return;
    }
    SDL_RestoreWindow(sdl_window_);
    SDL_ShowWindow(sdl_window_);
    SDL_RaiseWindow(sdl_window_);
    SetForceFullRepaint(true);
    SetNeedsRepaint();
}

void Window::Hide() {
    if (sdl_window_) {
        SDL_HideWindow(sdl_window_);
    }
}

std::string Window::GetTitle() const {
    return config_.title;
}

void Window::SetTitle(const std::string& title) {
    config_.title = title;
    if (sdl_window_) {
        // title 应该已经是 UTF-8 编码，SDL 需要 UTF-8
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
}

void Window::SetSize(int width, int height) {
    // Clamp to min/max constraints
    if (config_.min_width > 0 && width < config_.min_width) width = config_.min_width;
    if (config_.min_height > 0 && height < config_.min_height) height = config_.min_height;
    if (config_.max_width > 0 && width > config_.max_width) width = config_.max_width;
    if (config_.max_height > 0 && height > config_.max_height) height = config_.max_height;

    config_.width = width;
    config_.height = height;
    if (sdl_window_) {
        const float content_scale = GetWindowContentScale(sdl_window_);
        SDL_SetWindowSize(sdl_window_,
            ScaleCssToWindowUnits(width, content_scale),
            ScaleCssToWindowUnits(height, content_scale));
        OnResize();
    }
}

void Window::GetSize(int* width, int* height) const {
    if (sdl_window_) {
        int window_width = 0, window_height = 0;
        SDL_GetWindowSize(sdl_window_, &window_width, &window_height);
        const float content_scale = GetWindowContentScale(sdl_window_);
        if (width) *width = ScaleWindowUnitsToCss(window_width, content_scale);
        if (height) *height = ScaleWindowUnitsToCss(window_height, content_scale);
    } else {
        if (width) *width = config_.width;
        if (height) *height = config_.height;
    }
}

void Window::SetMinSize(int width, int height) {
    config_.min_width = width;
    config_.min_height = height;
    if (sdl_window_) {
        const float content_scale = GetWindowContentScale(sdl_window_);
        SDL_SetWindowMinimumSize(sdl_window_,
            width > 0 ? ScaleCssToWindowUnits(width, content_scale) : 0,
            height > 0 ? ScaleCssToWindowUnits(height, content_scale) : 0);
    }
}

void Window::SetMaxSize(int width, int height) {
    config_.max_width = width;
    config_.max_height = height;
    if (sdl_window_) {
        const float content_scale = GetWindowContentScale(sdl_window_);
        SDL_SetWindowMaximumSize(sdl_window_,
            width > 0 ? ScaleCssToWindowUnits(width, content_scale) : 0,
            height > 0 ? ScaleCssToWindowUnits(height, content_scale) : 0);
    }
}

void Window::GetMinSize(int* width, int* height) const {
    if (width) *width = config_.min_width;
    if (height) *height = config_.min_height;
}

void Window::GetMaxSize(int* width, int* height) const {
    if (width) *width = config_.max_width;
    if (height) *height = config_.max_height;
}

void Window::SetPosition(int x, int y) {
    config_.x = x;
    config_.y = y;
    if (sdl_window_) {
        SDL_SetWindowPosition(sdl_window_, x, y);
    }
}

void Window::GetPosition(int* x, int* y) const {
    if (sdl_window_) {
        SDL_GetWindowPosition(sdl_window_, x, y);
    } else {
        if (x) *x = config_.x;
        if (y) *y = config_.y;
    }
}

void Window::Minimize() {
    if (sdl_window_) {
        SDL_MinimizeWindow(sdl_window_);
    }
}

void Window::Maximize() {
    if (sdl_window_) {
        SDL_MaximizeWindow(sdl_window_);
    }
}

void Window::Restore() {
    if (sdl_window_) {
        SDL_RestoreWindow(sdl_window_);
    }
}

void Window::SetFullscreen(bool fullscreen) {
    config_.fullscreen = fullscreen;
    if (sdl_window_) {
        SDL_SetWindowFullscreen(sdl_window_, fullscreen);
    }
}

void Window::SetResizable(bool resizable) {
    config_.resizable = resizable;
    if (sdl_window_) {
        SDL_SetWindowResizable(sdl_window_, resizable);
    }
}

void Window::SetBorderless(bool borderless) {
    config_.borderless = borderless;
    if (sdl_window_) {
        SDL_SetWindowBordered(sdl_window_, !borderless);
    }
}

void Window::SetAlwaysOnTop(bool on_top) {
    config_.always_on_top = on_top;
    if (sdl_window_) {
        SDL_SetWindowAlwaysOnTop(sdl_window_, on_top);
    }
}

SkCanvas* Window::GetCanvas() const {
    return surface_ ? surface_->getCanvas() : nullptr;
}

PaintModeDisplayBackend* Window::GetPaintModeBackend() const {
#ifdef _WIN32
    if (display_backend_ && display_backend_->GetType() == DisplayBackendType::PAINT_MODE) {
        return static_cast<PaintModeDisplayBackend*>(display_backend_.get());
    }
#endif
    return nullptr;
}

void Window::SwapBuffers() {
    if (!sdl_window_) {
        return;
    }

    const bool debug_anim_frame = IsAnimFrameDebugEnabled();
    static uint64_t swap_frame = 0;
    ++swap_frame;

    if (actual_backend_ == RenderBackend::OPENGL && gr_context_) {
        // GPU 模式：刷新 Skia 命令并交换 OpenGL 缓冲区
        gr_context_->flush();
        SDL_GL_SwapWindow(sdl_window_);

        if (debug_anim_frame && (swap_frame <= 120 || (swap_frame % 60 == 0))) {
            std::cout << "[ANIM_FRAME_SWAP] frame=" << swap_frame
                      << " backend=OPENGL"
                      << " present=SDL_GL_SwapWindow"
                      << "\n";
        }
    }
    else if (actual_backend_ == RenderBackend::CPU && surface_) {
        // CPU 模式：使用 DisplayBackend 显示像素
        SkPixmap pixmap;
        if (!surface_->peekPixels(&pixmap)) {
            return;
        }

        if (display_backend_) {
#ifdef _WIN32
            win32::IncrementPresentCount();
            win32::PrintStats();
#endif
            // 使用局部更新优化 CPU 模式性能
            if (has_dirty_bounds_ && !last_dirty_bounds_.isEmpty()) {
                // 有脏区域边界：只更新脏区域
                int dirty_x = static_cast<int>(last_dirty_bounds_.left());
                int dirty_y = static_cast<int>(last_dirty_bounds_.top());
                int dirty_width = static_cast<int>(last_dirty_bounds_.width());
                int dirty_height = static_cast<int>(last_dirty_bounds_.height());

                // 边界检查
                int surface_width = static_cast<int>(pixmap.width());
                int surface_height = static_cast<int>(pixmap.height());
                if (dirty_x < 0) dirty_x = 0;
                if (dirty_y < 0) dirty_y = 0;
                if (dirty_x + dirty_width > surface_width) dirty_width = surface_width - dirty_x;
                if (dirty_y + dirty_height > surface_height) dirty_height = surface_height - dirty_y;

                if (dirty_width > 0 && dirty_height > 0) {
                    if (debug_anim_frame && (swap_frame <= 120 || (swap_frame % 60 == 0))) {
                        std::cout << "[ANIM_FRAME_SWAP] frame=" << swap_frame
                                  << " backend=CPU"
                                  << " present=PresentPartial"
                                  << " dirty=" << dirty_x << "," << dirty_y
                                  << "," << dirty_width << "x" << dirty_height
                                  << "\n";
                    }

                    display_backend_->PresentPartial(
                        pixmap.addr(),
                        surface_width,
                        surface_height,
                        static_cast<int>(pixmap.rowBytes()),
                        dirty_x, dirty_y, dirty_width, dirty_height
                    );
                }
            } else {
                if (debug_anim_frame && (swap_frame <= 120 || (swap_frame % 60 == 0))) {
                    std::cout << "[ANIM_FRAME_SWAP] frame=" << swap_frame
                              << " backend=CPU"
                              << " present=Present(full)"
                              << " size=" << pixmap.width() << "x" << pixmap.height()
                              << "\n";
                }

                // 无脏区域边界或全量渲染：更新整个 surface
                display_backend_->Present(
                    pixmap.addr(),
                    static_cast<int>(pixmap.width()),
                    static_cast<int>(pixmap.height()),
                    static_cast<int>(pixmap.rowBytes())
                );
            }
            has_dirty_bounds_ = false;
        }
    }
}

void Window::OnResize() {
    if (!sdl_window_) return;

    // 获取客户区大小（像素，不包括标题栏和边框）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 对外统一维护为 CSS 逻辑尺寸，SDL 内部窗口尺寸则按 display content scale 放大
    int window_width = 0, window_height = 0;
    SDL_GetWindowSize(sdl_window_, &window_width, &window_height);
    const float content_scale = GetWindowContentScale(sdl_window_);
    config_.width = ScaleWindowUnitsToCss(window_width, content_scale);
    config_.height = ScaleWindowUnitsToCss(window_height, content_scale);

    // 重新创建Skia渲染表面（使用客户区像素大小）
    if (actual_backend_ == RenderBackend::OPENGL) {
        // 更新 OpenGL viewport
        glViewport(0, 0, width, height);

        // 关键修复：在重新创建 surface 之前，清除两个缓冲区
        // OpenGL 双缓冲需要清除前后两个缓冲区，否则新增区域会显示垃圾数据
        // 使用缓存的 body 背景色，避免浮点精度问题导致的边缘颜色不一致
        float r = SkColorGetR(cached_body_bg_color_) / 255.0f;
        float g = SkColorGetG(cached_body_bg_color_) / 255.0f;
        float b = SkColorGetB(cached_body_bg_color_) / 255.0f;
        float a = SkColorGetA(cached_body_bg_color_) / 255.0f;
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(sdl_window_);  // 交换到后缓冲
        glClear(GL_COLOR_BUFFER_BIT);    // 清除后缓冲

        CreateSkiaSurface();

        // 调整 FBO 大小
        if (fbo_manager_) {
            if (!fbo_manager_->Resize(width, height)) {
                fbo_manager_.reset();
                use_fbo_incremental_ = false;
            } else {
                // FBO resize 后需要全量重绘
                fbo_needs_full_paint_ = true;
            }
        }

        // P0: resize 后主动做一次 GPU 资源预算与延迟回收，降低高水位驻留
        if (gr_context_) {
            gr_context_->setResourceCacheLimit(kSkiaResizeCacheLimitBytes);
            gr_context_->performDeferredCleanup(std::chrono::milliseconds(0));
        }
    } else if (actual_backend_ == RenderBackend::CPU) {
        // CPU 模式：重新创建 Skia Raster 表面
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface_ = SkSurfaces::Raster(info);

        // 通知 DisplayBackend 窗口大小变化
        if (display_backend_) {
            display_backend_->OnResize(width, height);
        }
    }

    // 触发resize回调（使用 CSS 逻辑大小）
    if (on_resize_callback_) {
        on_resize_callback_(config_.width, config_.height);
    }
}

void Window::InitSDL() {
    // 只在第一次调用时初始化SDL
    if (sdl_init_count == 0) {
        // 设置 SDL 提示，禁用 Windows 上可能导致问题的行为
        // 禁用 Windows 消息循环中的某些处理
        SDL_SetHint(SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP, "1");
        // 禁用屏幕保护程序（可选）
        SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");
        // 允许点击穿透：当窗口失去焦点后，点击窗口时同时激活窗口并生成点击事件
        // 解决"窗口失去焦点后直接点击按钮需要点击两次才能响应"的问题
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
        // 由应用自行绘制 composition 文本，避免 Windows 原生 composition UI 字号/样式
        // 与页面字体不同步；候选窗仍可继续使用系统原生 UI。
        SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
        }

        // 设置事件过滤器，过滤掉可能导致闪烁的 EXPOSED 事件
        SDL_SetEventFilter(SDLEventFilter, nullptr);
    }
    sdl_init_count++;
}

void Window::CreateSDLWindow() {
    // 构建窗口标志
    SDL_WindowFlags flags = 0;
    const bool request_opengl_window = ShouldCreateOpenGLWindow(config_);

    // 透明窗口（不规则窗体）使用 LayeredWindow 后端，不能用 OpenGL
    // 因为 WS_EX_LAYERED + UpdateLayeredWindow 与 OpenGL 渲染管线不兼容
    // GPU加速关闭时也不创建 OpenGL 窗口，避免不必要的 GPU 资源占用
    // 显式 CPU/SOFTWARE 模式也不应申请 OpenGL 窗口，避免在 VM/无 3D 驱动环境里提前触发 OpenGL 路径
    if (request_opengl_window) {
        flags |= SDL_WINDOW_OPENGL;
    }

    if (config_.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (config_.borderless) flags |= SDL_WINDOW_BORDERLESS;
    if (config_.transparent) flags |= SDL_WINDOW_TRANSPARENT;
    if (config_.maximized) flags |= SDL_WINDOW_MAXIMIZED;
    if (config_.minimized) flags |= SDL_WINDOW_MINIMIZED;
    if (config_.hidden) flags |= SDL_WINDOW_HIDDEN;
    if (config_.always_on_top) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (config_.high_dpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;


    const SDL_DisplayID target_display = GetTargetDisplayForConfig(config_);
    const float content_scale = GetDisplayContentScaleSafe(target_display);
    const int initial_width = ScaleCssToWindowUnits(config_.width, content_scale);
    const int initial_height = ScaleCssToWindowUnits(config_.height, content_scale);

    // 创建窗口
    // config_.title 应该已经是 UTF-8 编码，SDL 需要 UTF-8
    sdl_window_ = SDL_CreateWindow(
        config_.title.c_str(),
        initial_width,
        initial_height,
        flags
    );

    if (!sdl_window_) {
        throw std::runtime_error(std::string("Failed to create SDL window: ") + SDL_GetError());
    }

    // 设置窗口最小/最大尺寸限制
    if (config_.min_width > 0 || config_.min_height > 0) {
        SDL_SetWindowMinimumSize(sdl_window_,
            config_.min_width > 0 ? ScaleCssToWindowUnits(config_.min_width, content_scale) : 0,
            config_.min_height > 0 ? ScaleCssToWindowUnits(config_.min_height, content_scale) : 0);
    }
    if (config_.max_width > 0 || config_.max_height > 0) {
        SDL_SetWindowMaximumSize(sdl_window_,
            config_.max_width > 0 ? ScaleCssToWindowUnits(config_.max_width, content_scale) : 0,
            config_.max_height > 0 ? ScaleCssToWindowUnits(config_.max_height, content_scale) : 0);
    }

    // 设置窗口位置（如果指定）
    if (config_.x >= 0 && config_.y >= 0) {
        SDL_SetWindowPosition(sdl_window_, config_.x, config_.y);
    }
}

void Window::InitOpenGL() {
    if (!sdl_window_) {
        throw std::runtime_error("Cannot initialize OpenGL: window not created");
    }

    // 设置OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 创建OpenGL上下文
    gl_context_ = SDL_GL_CreateContext(sdl_window_);
    if (!gl_context_) {
        throw std::runtime_error(std::string("Failed to create OpenGL context: ") + SDL_GetError());
    }

    // 激活上下文
    if (!SDL_GL_MakeCurrent(sdl_window_, gl_context_)) {
        throw std::runtime_error(std::string("Failed to make OpenGL context current: ") + SDL_GetError());
    }

    // 设置 VSync
    if (config_.vsync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
}

void Window::InitSkia() {
    if (!gl_context_) {
        throw std::runtime_error("Cannot initialize Skia: OpenGL context not created");
    }

    // 创建Skia OpenGL接口
    auto interface = GrGLMakeNativeInterface();
    if (!interface) {
        throw std::runtime_error("Failed to create Skia OpenGL interface");
    }

    // 创建Skia上下文 (使用新的 API)
    gr_context_ = GrDirectContexts::MakeGL(interface);
    if (!gr_context_) {
        throw std::runtime_error("Failed to create Skia context");
    }
}

void Window::CreateSkiaSurface() {
    if (!gr_context_) {
        throw std::runtime_error("Cannot create Skia surface: Skia context not initialized");
    }

    // 获取窗口大小（像素）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 创建OpenGL帧缓冲信息
    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;  // 0表示默认帧缓冲
    framebuffer_info.fFormat = GL_RGBA8;

    // 创建后端渲染目标
    GrBackendRenderTarget backend_render_target =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebuffer_info);

    // 创建Skia表面
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        gr_context_.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr
    );

    if (!surface_) {
        throw std::runtime_error("Failed to create Skia surface");
    }

    // 关键修复：清除新创建的表面，避免显示垃圾数据
    // 这在窗口大小改变时特别重要
    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        canvas->clear(SK_ColorWHITE);
    }
}

void Window::InitCPURendering() {
    // CPU 软件渲染模式 - 使用 DisplayBackend 进行无闪烁显示

    // 使用物理像素大小创建渲染表面（支持高 DPI）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 创建 Skia Raster 表面（CPU 渲染）- 使用物理像素大小
    // 注意：使用 BGRA 格式以匹配显示后端
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface_ = SkSurfaces::Raster(info);

    if (!surface_) {
        throw std::runtime_error("Failed to create CPU rendering surface");
    }

#ifdef _WIN32
    if (config_.transparent) {
        // 透明窗口（不规则窗体）：强制使用 LayeredWindow 后端
        // LayeredWindow 使用 UpdateLayeredWindow + ULW_ALPHA 实现逐像素透明
        display_backend_ = DisplayBackend::Create(DisplayBackendType::LAYERED_WINDOW);
        if (display_backend_ && display_backend_->Initialize(sdl_window_, width, height)) {
            // 透明窗口：清除画布为全透明
            SkCanvas* canvas = surface_->getCanvas();
            if (canvas) {
                canvas->clear(SK_ColorTRANSPARENT);
            }
            return;
        }
        // LayeredWindow 失败，回退到普通后端
        display_backend_.reset();
        std::cerr << "[Window] LayeredWindow backend failed, falling back to normal backend" << std::endl;
    }
#endif

    // 创建最佳显示后端（按优先级：D3D11 → PaintMode → GDI → OpenGL → SDL_Surface）
    display_backend_ = DisplayBackend::CreateBest(sdl_window_, width, height);
    if (!display_backend_) {
        // 如果 CreateBest 失败，尝试 SDL Surface 作为最后回退
        display_backend_ = DisplayBackend::Create(DisplayBackendType::SDL_SURFACE);
        if (display_backend_) {
            display_backend_->Initialize(sdl_window_, width, height);
        }
    }
}

bool Window::HandleSDLEvent(const SDL_Event& event) {
    // 只处理与此窗口相关的事件
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        if (event.window.windowID != SDL_GetWindowID(sdl_window_)) {
            return false;  // 不是此窗口的事件
        }

        switch (event.type) {
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                int new_width = event.window.data1;
                int new_height = event.window.data2;

                // 检查是否真的改变了大小（避免重复处理）
                static int last_processed_width = 0, last_processed_height = 0;
                if (new_width == last_processed_width && new_height == last_processed_height) {
                    return true;  // 大小没变，跳过
                }
                last_processed_width = new_width;
                last_processed_height = new_height;

                // P1: 识别连续 resize burst（例如用户持续拖拽/反复放大缩小）
                const Uint64 now = SDL_GetTicks();
                if (resize_burst_window_start_tick_ == 0 ||
                    (now - resize_burst_window_start_tick_) > static_cast<Uint64>(kResizeBurstWindowMs)) {
                    resize_burst_window_start_tick_ = now;
                    resize_burst_count_ = 1;
                } else {
                    ++resize_burst_count_;
                }

                // P0: resize 事件节流（合并短时间连续 resize）
                static Uint64 last_resize_tick = 0;
                const bool should_debounce = (last_resize_tick != 0) &&
                                             ((now - last_resize_tick) < static_cast<Uint64>(kResizeDebounceMs));

                // P1: burst 期间更激进收缩缓存，降低连续 resize 的累积高水位
                const bool in_resize_burst = resize_burst_count_ >= kResizeBurstThreshold;
                if (in_resize_burst) {
                    ImageCache::GetInstance().SetMaxCacheSize(kImageCacheBurstShrinkBytes);
                    if (gr_context_) {
                        gr_context_->setResourceCacheLimit(kSkiaBurstCacheLimitBytes);
                        gr_context_->performDeferredCleanup(std::chrono::milliseconds(0));
                    }
                }

                if (should_debounce) {
                    pending_resize_width_ = new_width;
                    pending_resize_height_ = new_height;
                    has_pending_resize_ = true;
                    SetNeedsRepaint();
                    return true;
                }

                last_resize_tick = now;

                OnResize();
                InvalidateRenderTree();  // 窗口大小改变，需要用新尺寸重建渲染树和布局
                SetForceFullRepaint(true);  // 关键修复：强制全量重绘，避免新区域显示垃圾数据
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, config_.width, config_.height));
                return true;
            }

            case SDL_EVENT_WINDOW_MOVED: {
                int x = event.window.data1;
                int y = event.window.data2;
                if (on_move_callback_) {
                    on_move_callback_(x, y);
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::MOVE, x, y));
                return true;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                if (on_focus_callback_) {
                    on_focus_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::FOCUS));
                return true;
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                if (on_blur_callback_) {
                    on_blur_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::BLUR));
                return true;
            }

            case SDL_EVENT_WINDOW_MINIMIZED: {
                DispatchWindowEvent(WindowEvent(WindowEventType::MINIMIZE));
                return true;
            }

            case SDL_EVENT_WINDOW_MAXIMIZED: {
                // 窗口最大化时需要触发重绘
                // 注意：不在这里调用 InvalidateRenderTree()，因为此时窗口尺寸可能还未更新
                // RESIZED 事件会随后触发，届时会正确处理渲染树重建
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::MAXIMIZE));
                return true;
            }

            case SDL_EVENT_WINDOW_RESTORED: {
                // 窗口还原时需要触发重绘
                // 注意：不在这里调用 InvalidateRenderTree()，因为此时窗口尺寸可能还未更新
                // RESIZED 事件会随后触发，届时会正确处理渲染树重建
                SetNeedsRepaint();

                // P0: 还原后主动触发缓存回收，帮助内存从高水位回落
                ImageCache::GetInstance().SetMaxCacheSize(kImageCacheShrinkBytes);
                ImageCache::GetInstance().Clear();

                if (gr_context_) {
                    gr_context_->setResourceCacheLimit(kSkiaRestoreCacheLimitBytes);
                    gr_context_->performDeferredCleanup(std::chrono::milliseconds(0));
                    gr_context_->purgeUnlockedResources(GrPurgeResourceOptions::kAllResources);
                    gr_context_->flush();
                    gr_context_->freeGpuResources();
                }

                DispatchWindowEvent(WindowEvent(WindowEventType::RESTORE));
                return true;
            }

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                bool handled = false;
                if (on_close_request_handler_) {
                    handled = on_close_request_handler_();
                }
                if (handled) {
                    suppress_next_native_close_ = true;
                    should_close_ = false;
                    return true;
                }
                if (on_close_callback_) {
                    on_close_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::CLOSE));
                should_close_ = true;
                return true;
            }

            case SDL_EVENT_WINDOW_SHOWN: {
                DispatchWindowEvent(WindowEvent(WindowEventType::SHOWN));
                return true;
            }

            case SDL_EVENT_WINDOW_HIDDEN: {
                DispatchWindowEvent(WindowEvent(WindowEventType::HIDDEN));
                return true;
            }

            case SDL_EVENT_WINDOW_EXPOSED: {
                // 完全忽略 EXPOSED 事件
                // 在 Windows 上，SDL_RenderPresent 会触发 EXPOSED 事件，形成无限循环
                // 我们的渲染由 needs_repaint_ 标志控制，不需要响应 EXPOSED 事件
                return true;
            }

            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
                // 忽略显示缩放变化事件，避免可能的循环
                return true;
            }

            case SDL_EVENT_WINDOW_OCCLUDED: {
                // 窗口被遮挡，不需要处理
                return true;
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                DispatchWindowEvent(WindowEvent(WindowEventType::ENTER));
                return true;
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                DispatchWindowEvent(WindowEvent(WindowEventType::LEAVE));
                return true;
            }

            default:
                return false;
        }
    }

    return false;
}

void Window::DispatchWindowEvent(const WindowEvent& event) {
    auto it = event_listeners_.find(event.GetType());
    if (it != event_listeners_.end()) {
        for (const auto& listener : it->second) {
            listener(event);
        }
    }
}

void Window::AddEventListener(WindowEventType type, std::function<void(const WindowEvent&)> listener) {
    event_listeners_[type].push_back(listener);
}

void Window::RemoveEventListeners(WindowEventType type) {
    event_listeners_.erase(type);
}

// ========== 渲染集成 ==========

void Window::SetDocument(std::shared_ptr<Document> document) {
    // 移除旧文档的观察者
    if (document_ && dom_observer_) {
        document_->RemoveObserver(dom_observer_.get());
    }

    document_ = document;

    // 创建渲染器（如果还没有）
    if (!renderer_ && surface_) {
        renderer_ = std::make_unique<Renderer>(surface_);
    }

    // 创建并注册 DOM 观察者
    if (document_) {
        // 关键：设置 Document 对 Window 的引用，用于 Element::Focus() 等方法
        document_->SetWindow(this);

        dom_observer_ = std::make_unique<WindowDOMObserver>(this);
        document_->AddObserver(dom_observer_.get());

        // 注册同步布局回调（用于 getBoundingClientRect 等需要强制 reflow 的操作）
        document_->SetSyncLayoutCallback([this]() {
            ForceLayoutSync();
        });

        // 重新创建动画应用器，使用 StyleManager 的 AnimationController
        // 这样 @keyframes 规则可以被正确找到
        if (document_->GetStyleManager()) {
            animation_applicator_ = std::make_unique<AnimationApplicator>(
                document_->GetStyleManager()->GetAnimationController());
        }
    }

    // 关键修复：设置新文档时需要完整初始化渲染
    // 1. 标记渲染树无效，需要重建
    InvalidateRenderTree();
    // 2. 强制全量重绘（避免增量渲染导致的显示问题）
    SetForceFullRepaint(true);
    // 3. 标记需要重绘
    SetNeedsRepaint();
}

void Window::Render() {
    DEBUG_LOG("[Window::Render] Called, needs_repaint_=" << needs_repaint_
              << ", render_tree_valid_=" << render_tree_valid_);

    // 处理待处理的 resize（节流期间被跳过的最后一次 resize）
    if (has_pending_resize_) {
        has_pending_resize_ = false;
        OnResize();
        if (render_pipeline_) {
            render_pipeline_->ForceFullUpdate();
        }
        InvalidateRenderTree();
        SetForceFullRepaint(true);
        SetNeedsRepaint();
        DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, config_.width, config_.height));
    }

    if (!document_ || !surface_) {
        return;
    }

    // =========================================================================
    // 检查是否有活动动画
    // =========================================================================
    bool has_active_animations = false;
    if (animation_timeline_) {
        has_active_animations = animation_timeline_->HasRunningTransitions();
    }
    if (!has_active_animations && document_ && document_->GetStyleManager()) {
        has_active_animations = !document_->GetStyleManager()->GetAnimationController().GetRunningAnimations().empty();
    }
    if (!has_active_animations && animation_controller_) {
        has_active_animations = !animation_controller_->GetRunningAnimations().empty();
    }
    if (!has_active_animations && animation_applicator_ && cached_render_tree_) {
        has_active_animations = HasPendingAnimations(cached_render_tree_.get());
    }

    // 快速路径：无需重绘且无活动动画时直接返回
    if (!needs_repaint_ && !has_active_animations && dirty_rects_.empty() && render_tree_valid_) {
        if (render_pipeline_ && !render_pipeline_->NeedsUpdate()) {
            return;
        }
    }

    // 获取画布
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }

    // =========================================================================
    // 获取视口尺寸
    // =========================================================================
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
    float dpi_scale = GetDisplayScale();
    // 使用浮点数保持精度，避免截断导致的白边问题
    float logical_width = physical_width / dpi_scale;
    float logical_height = physical_height / dpi_scale;

    // 检查 DevTools 是否打开，如果打开则调整主应用区域
    auto& devtools = DevToolsManager::GetInstance();
    float app_x = 0, app_y = 0;
    float app_width = logical_width;
    float app_height = logical_height;

    if (devtools.IsOpen()) {
        devtools.GetMainAppBounds(logical_width, logical_height,
                                   app_x, app_y, app_width, app_height);
    }

    // 设置视口尺寸
    RenderObject::SetViewportSize(app_width, app_height);

    // =========================================================================
    // 初始化统一渲染管线
    // =========================================================================
    if (render_pipeline_ && !render_pipeline_->IsInitialized()) {
        if (!render_pipeline_->Initialize(static_cast<int>(app_width), static_cast<int>(app_height))) {
            return;
        }
        render_pipeline_->SetDocument(document_);
        render_pipeline_->SetDpiScale(dpi_scale);

        // 连接属性树系统到动画应用器
        if (animation_applicator_ && render_pipeline_->IsUsingPropertyTreeSystem()) {
            animation_applicator_->SetPaintArtifactCompositor(
                render_pipeline_->GetPaintArtifactCompositor());
            animation_applicator_->SetPropertyTrees(
                render_pipeline_->GetPropertyTrees());
        }
    }

    // =========================================================================
    // 检查窗口大小是否改变（需要重建布局树）
    // =========================================================================
    // 关键修复：视口尺寸按像素取整后比较，避免浮点抖动导致每帧都被判定为尺寸变化。
    // 之前使用 float + epsilon(0.01) 在部分 DPI/DevTools 场景下会持续触发，
    // 从而每帧置位 needs_layer_tree_rebuild_，压制增量路径。
    static int last_app_width_px = -1;
    static int last_app_height_px = -1;

    const int app_width_px = std::max(0, static_cast<int>(std::lround(app_width)));
    const int app_height_px = std::max(0, static_cast<int>(std::lround(app_height)));
    bool app_size_changed_unified =
        (app_width_px != last_app_width_px) ||
        (app_height_px != last_app_height_px);

    if (app_size_changed_unified) {
        last_app_width_px = app_width_px;
        last_app_height_px = app_height_px;
        render_tree_valid_ = false;  // 窗口大小改变，需要重建布局树

        // 关键修复：窗口大小改变时，需要强制重建层树
        // 因为层的边界需要根据新的视口尺寸更新
        if (render_pipeline_) {
            render_pipeline_->InvalidateLayerTree();
            render_pipeline_->Resize(app_width_px, app_height_px);
        }
    }

    // =========================================================================
    // 确保渲染树已构建
    // =========================================================================
    const bool render_tree_rebuild_required = (!render_tree_valid_ || !cached_render_tree_);
    EnsureRenderTree();

    if (!cached_render_tree_) {
        return;
    }

    // 关键修复：全量重建前先清理已脱离文档的 DOM binding，避免 Clear() 吃掉 removed/replaced 记录
    if (render_tree_rebuild_required && document_) {
        RenderTreeSynchronizer::CleanupDetachedDOMBindings(document_->GetDirtyTracker());
        document_->GetDirtyTracker().Clear();
    }

    // =========================================================================
    // 增量同步：处理 DOM 变化
    // =========================================================================
    // 关键修复：全量重建帧跳过增量同步，避免同帧重复插入 out-of-flow 节点
    bool needs_layout_update = false;
    if (!render_tree_rebuild_required && document_ && render_tree_synchronizer_ && cached_render_tree_ && render_tree_valid_) {
        auto& tracker = document_->GetDirtyTracker();
        const bool has_pending_changes = tracker.HasPendingChanges();
        if (has_pending_changes) {
            // 调用 RenderTreeSynchronizer 来同步变化
            bool synced = render_tree_synchronizer_->Synchronize(tracker, cached_render_tree_);
            if (synced) {
                needs_layout_update = true;
            }
        }
    }

    // =========================================================================
    // 增量样式更新：处理 style 属性变化导致的样式重算
    // =========================================================================
    // 当 style 属性变化时，DOM 节点会被标记为 IsStyleDirty()
    // 需要遍历 DOM 树，将脏标记同步到 RenderObject 并重新计算样式
    // 关键修复：结构变更帧跳过这轮递归，避免用旧 layout/render 映射继续更新新树
    if (!needs_layout_update && document_ && cached_render_tree_ && render_tree_valid_) {
        auto body = document_->GetBody();
        if (body && cached_render_tree_) {
            // 检查是否有样式脏标记需要处理
            bool has_style_dirty = body->IsLayoutDirty() || body->IsStyleDirty() || body->IsPaintDirty() ||
                                   body->ChildNeedsStyleRecalc() || body->ChildNeedsLayout();
            if (has_style_dirty) {
                MarkRenderObjectsDirty(body.get(), cached_render_tree_.get());
                // 清除 DOM 节点的脏标记和增量标记（递归清除整个子树）
                std::function<void(Node*)> clearDirtyRecursive = [&](Node* node) {
                    if (!node) return;
                    node->ClearDirty();
                    node->ClearNeedsStyleRecalc();
                    node->ClearNeedsLayout();
                    for (const auto& child : node->GetChildNodes()) {
                        clearDirtyRecursive(child.get());
                    }
                };
                clearDirtyRecursive(body.get());
            }
        }
    }

    // =========================================================================
    // 增量布局：处理样式变更导致的布局需求
    // =========================================================================
    // 即使没有 DOM 结构变化，样式变更（如 overflow）也可能需要重新布局
    if (layout_engine_ && cached_render_tree_) {
        // 获取窗口尺寸
        int physical_width, physical_height;
        SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
        float dpi_scale = GetDisplayScale();
        float width = static_cast<float>(physical_width) / dpi_scale;
        float height = static_cast<float>(physical_height) / dpi_scale;

        auto& devtools = DevToolsManager::GetInstance();
        float sync_app_width = width;
        float sync_app_height = height;
        if (devtools.IsOpen()) {
            float app_x, app_y;
            devtools.GetMainAppBounds(width, height, app_x, app_y, sync_app_width, sync_app_height);
        }

        if (needs_layout_update) {
            // DOM 结构变化，需要重建布局树
            // force_rebuild=true 确保即使缓存有效也会重建
            layout_engine_->BuildLayoutTree(cached_render_tree_, true);
            layout_engine_->ComputeLayout(sync_app_width, sync_app_height);
            layout_engine_->GetLayoutInfo(cached_render_tree_);

            // 关键修复：结构变化后强制层树重建，避免父层残留旧位图导致“重影/双实例”
            if (render_pipeline_) {
                render_pipeline_->InvalidateLayerTree();
                render_pipeline_->ForceFullUpdate();
            }
            // 注意：ViewportBounds 缓存失效已在 ReadLayoutResults 中按需处理
        } else {
            // 尝试增量布局（处理样式变更导致的布局需求）
            bool did_incremental = layout_engine_->ComputeIncrementalLayout(sync_app_width, sync_app_height);
            if (did_incremental) {
                layout_engine_->GetLayoutInfo(cached_render_tree_);
                needs_layout_update = true;
                // 注意：ViewportBounds 缓存失效已在 ReadLayoutResults 中按需处理
            }
        }

        // 注意：不再在每次布局更新时触发完整层树重建
        // 层树会在 RenderPipeline::DoLayerTreeBuild 中通过 DetectAndCreateNewLayers 增量更新
        // 只有在窗口大小改变等重大变化时才需要完整重建（在上面的 app_size_changed_unified 分支处理）
    }

    // 关键：将渲染树传递给统一渲染管线
    if (render_pipeline_ && render_pipeline_->GetRenderTree() != cached_render_tree_) {
        render_pipeline_->SetRenderTree(cached_render_tree_);
    }

    // =========================================================================
    // 更新动画
    // =========================================================================
    static Uint64 anim_start_time = SDL_GetPerformanceCounter();
    Uint64 anim_current_time = SDL_GetPerformanceCounter();
    Uint64 anim_frequency = SDL_GetPerformanceFrequency();
    double timestamp_sec = static_cast<double>(anim_current_time - anim_start_time) / anim_frequency;
    UpdateAnimations(timestamp_sec);

    // =========================================================================
    // 使用统一渲染管线渲染
    // =========================================================================
    if (render_pipeline_) {
        // 获取背景色 - 优先使用 body 的背景色，避免白边问题
        // 透明窗口（不规则窗体）：始终使用透明背景
        SkColor clear_color;
        if (config_.transparent) {
            clear_color = SK_ColorTRANSPARENT;
        } else {
            clear_color = cached_body_bg_color_;  // 使用缓存的背景色
            if (cached_render_tree_) {
                const auto& body_style = cached_render_tree_->GetComputedStyle();
                if (!body_style.background_color.empty() && body_style.background_color != "transparent") {
                    clear_color = Color::Parse(body_style.background_color);
                    cached_body_bg_color_ = clear_color;  // 更新缓存
                }
            }
        }
        canvas->clear(clear_color);

        // 应用 DPI 缩放
        canvas->save();
        canvas->scale(dpi_scale, dpi_scale);

        // 如果 DevTools 打开，裁剪到主应用区域
        if (devtools.IsOpen()) {
            canvas->clipRect(SkRect::MakeXYWH(app_x, app_y, app_width, app_height));
        }

        // 处理一帧
        bool process_ok = render_pipeline_->ProcessFrame(canvas);

        if (IsAnimFrameDebugEnabled()) {
            static uint64_t render_frame = 0;
            ++render_frame;
            if (render_frame <= 120 || (render_frame % 60 == 0)) {
                std::cout << "[ANIM_FRAME_RENDER] frame=" << render_frame
                          << " processOk=" << (process_ok ? 1 : 0)
                          << " needsRepaint=" << (needs_repaint_ ? 1 : 0)
                          << " pipelineNeedsUpdate=" << (render_pipeline_->NeedsUpdate() ? 1 : 0)
                          << "\n";
            }
        }

        // 更新并绘制 select 下拉菜单
        auto& dropdown_manager = SelectDropdownManager::Instance();
        if (dropdown_manager.IsDropdownOpen()) {
            dropdown_manager.UpdatePositionFromRenderTree(cached_render_tree_);
        }
        dropdown_manager.Paint(canvas);

        canvas->restore();

        // 渲染 DevTools
        RenderDevTools(canvas, static_cast<float>(logical_width), static_cast<float>(logical_height));

        // 刷新 GPU 命令（如果使用 GPU）
        if (gr_context_) {
            gr_context_->flush();
        }
        // 注意：Present 由 SwapBuffers() 调用，不在这里调用
    }

    // =========================================================================
    // 检查是否有活动动画，决定是否继续重绘
    // =========================================================================
    bool has_running_animations = false;
    if (document_ && document_->GetStyleManager()) {
        has_running_animations = !document_->GetStyleManager()->GetAnimationController().GetRunningAnimations().empty();
    }
    if (!has_running_animations && animation_controller_) {
        has_running_animations = !animation_controller_->GetRunningAnimations().empty();
    }
    if (!has_running_animations && animation_timeline_) {
        has_running_animations = animation_timeline_->HasRunningTransitions();
    }

    // 关键修复：除了“正在运行”的动画，还要考虑“待启动”动画。
    // 典型场景：Spinner 首次渲染时动态注入 <style>@keyframes ...</style>，
    // 本帧可能尚未完成 keyframes 注册，StartAnimation 会暂时失败。
    // 若此时直接停止重绘，后续帧不会再推进，动画表现为“卡住不动”。
    bool has_pending_animations = false;
    if (cached_render_tree_) {
        has_pending_animations = HasPendingAnimations(cached_render_tree_.get());
    }

    // 清除重绘标记
    if (!has_running_animations && !has_pending_animations) {
        needs_repaint_ = false;
    }
    dirty_rects_.clear();
    force_full_repaint_ = false;
}

void Window::RenderDevTools(SkCanvas* canvas, float width, float height) {
    auto& devtools = DevToolsManager::GetInstance();

    if (!devtools.IsOpen()) {
        return;
    }

    // 获取 DPI 缩放比
    float dpi_scale = GetDisplayScale();

    // 注意：传入的 width 和 height 已经是逻辑尺寸（CSS 像素）
    // 不需要再除以 dpi_scale

    // 获取主应用区域
    float app_x, app_y, app_width, app_height;
    devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);

    // 先渲染元素高亮覆盖层（在主应用区域内）
    canvas->save();
    canvas->scale(dpi_scale, dpi_scale);
    canvas->clipRect(SkRect::MakeXYWH(app_x, app_y, app_width, app_height));
    devtools.RenderHighlight(canvas);
    canvas->restore();

    // 再渲染 DevTools 面板
    canvas->save();
    canvas->scale(dpi_scale, dpi_scale);
    devtools.Render(canvas, width, height);
    canvas->restore();
}

// 辅助函数：规范化文本内容（与 RenderTreeBuilder::CreateRenderObjectForText 保持一致）
// 将连续的空白字符（空格、制表符、换行符）折叠为单个空格
static std::string NormalizeTextContent(const std::string& text_data, const ComputedStyle* parent_style) {
    // 检查父元素的 white-space 属性，决定是否保留换行符
    bool preserve_newlines = false;
    if (parent_style) {
        const std::string& ws = parent_style->white_space;
        preserve_newlines = (ws == "pre" || ws == "pre-wrap" || ws == "pre-line");
    }

    std::string final_text;
    if (preserve_newlines) {
        // 保留换行符，但根据 white-space 的不同处理空格/制表符
        const std::string& ws = parent_style ? parent_style->white_space : "normal";
        if (ws == "pre") {
            // 完全保留原始文本
            final_text = text_data;
        } else {
            // pre-wrap 或 pre-line: 保留换行，合并连续空格
            bool in_space = false;
            for (char c : text_data) {
                if (c == '\n') {
                    final_text += c;
                    in_space = false;
                } else if (c == ' ' || c == '\t' || c == '\r') {
                    if (!in_space) {
                        final_text += ' ';
                        in_space = true;
                    }
                } else {
                    final_text += c;
                    in_space = false;
                }
            }
        }
    } else {
        // 规范化空白字符：将连续的空白字符（包括换行）替换为单个空格
        bool in_whitespace = false;
        for (char c : text_data) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!in_whitespace) {
                    final_text += ' ';
                    in_whitespace = true;
                }
            } else {
                final_text += c;
                in_whitespace = false;
            }
        }
    }
    return final_text;
}

void Window::MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj) {
    if (!dom_node || !render_obj) {
        return;
    }

    // 增量优化：如果 DOM 节点及其子树都不需要更新，直接返回
    // 检查 DOM 节点的脏标记
    bool node_is_dirty = dom_node->IsLayoutDirty() || dom_node->IsPaintDirty() || dom_node->IsStyleDirty();
    bool child_needs_update = dom_node->ChildNeedsStyleRecalc() || dom_node->ChildNeedsLayout();

    if (!node_is_dirty && !child_needs_update) {
        return;
    }

    // 检查DOM节点是否有布局脏标记
    if (dom_node->IsLayoutDirty()) {
        render_obj->MarkNeedsLayout();
    }

    // 获取父样式（用于继承）
    const ComputedStyle* parent_style = nullptr;
    ComputedStyle root_parent_style;
    auto parent = render_obj->GetParent();
    if (parent) {
        parent_style = &parent->GetComputedStyle();
    } else if (document_ && dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(dom_node->shared_from_this());
        if (element->GetTagName() == "body") {
            auto document_element = document_->GetDocumentElement();
            if (document_element) {
                StyleResolver root_resolver;
                if (document_->GetStyleManager()) {
                    root_resolver.SetStyleManager(document_->GetStyleManager());
                }
                root_parent_style = root_resolver.ResolveStyle(document_element, nullptr);
                parent_style = &root_parent_style;
            }
        }
    }

    // 检查DOM节点是否有绘制脏标记或样式脏标记（包括伪类变化如:focus）
    if (dom_node->IsPaintDirty() || dom_node->IsStyleDirty()) {
        render_obj->MarkNeedsPaint();

        // 对于 Text 节点，需要同步更新 RenderText 的文本内容和样式
        if (dom_node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = static_cast<Text*>(dom_node);
            auto render_text = dynamic_cast<RenderText*>(render_obj);
            if (text_node && render_text) {
                // 关键修复：使用规范化后的文本进行比较
                // RenderText 中存储的是规范化后的文本，所以比较时也需要规范化
                std::string raw_text = text_node->GetData();
                std::string normalized_text = NormalizeTextContent(raw_text, parent_style);
                if (render_text->GetText() != normalized_text) {
                    render_text->SetText(normalized_text);
                    render_obj->MarkNeedsLayout();  // 文本改变需要重新布局

                    // Update content version for incremental layout optimization
                    // **Feature: incremental-layout-optimization**
                    // **Validates: Requirements 1.1**
                    if (layout_engine_) {
                        layout_engine_->UpdateContentVersion(render_obj);
                    }
                }

                // 更新 Text 节点的样式（从父元素继承可继承属性）
                if (parent_style) {
                    ComputedStyle text_style = render_obj->GetComputedStyle();
                    text_style.color = parent_style->color;
                    text_style.font_family = parent_style->font_family;
                    text_style.font_size = parent_style->font_size;
                    text_style.font_weight = parent_style->font_weight;
                    text_style.font_style = parent_style->font_style;
                    text_style.line_height = parent_style->line_height;
                    text_style.text_align = parent_style->text_align;
                    text_style.text_decoration = parent_style->text_decoration;
                    text_style.text_shadow = parent_style->text_shadow;  // 继承 text-shadow
                    render_obj->SetComputedStyle(text_style);
                }
            }
        }

        // 重新计算 Element 样式（处理伪类变化如:focus, :hover等）
        if (dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            // 使用 StyleResolver 重新计算样式
            StyleResolver resolver;
            if (document_ && document_->GetStyleManager()) {
                resolver.SetStyleManager(document_->GetStyleManager());
            }
            auto element = std::static_pointer_cast<Element>(dom_node->shared_from_this());
            auto new_style = resolver.ResolveStyle(element, parent_style);

            render_obj->SetComputedStyle(new_style);

            // 关键修复：通知布局引擎样式已更新
            // 这确保 left/top 等位置属性的变化能正确触发重新布局
            if (layout_engine_) {
                layout_engine_->UpdateStyle(render_obj, new_style);
            }
        }
    }

    // 递归处理子节点
    // 优化：使用哈希表加速查找，避免O(n²)复杂度
    const auto& dom_children = dom_node->GetChildNodes();
    const auto& render_children = render_obj->GetChildren();

    // 构建DOM节点指针到节点的映射（O(n)）
    std::unordered_map<Node*, std::shared_ptr<Node>> dom_map;
    dom_map.reserve(dom_children.size());
    for (const auto& dom_child : dom_children) {
        dom_map[dom_child.get()] = dom_child;
    }

    // 获取当前节点的新样式（用于子节点继承）
    const ComputedStyle* current_style = &render_obj->GetComputedStyle();

    // 遍历渲染子节点并查找对应的DOM节点（O(n)）
    for (const auto& render_child : render_children) {
        if (!render_child || render_child.get() == render_obj) {
            continue;
        }

        auto render_child_parent = render_child->GetParent();
        if (!render_child_parent || render_child_parent.get() != render_obj) {
            continue;
        }

        auto render_child_node = render_child->GetNode();
        if (!render_child_node) {
            continue;
        }

        // O(1)查找
        auto it = dom_map.find(render_child_node.get());
        if (it != dom_map.end()) {
            Node* child_dom_node = it->second.get();
            RenderObject* child_render_obj = render_child.get();

            // 如果当前节点样式改变，子节点的可继承样式也需要更新
            bool current_is_dirty = dom_node->IsStyleDirty() || dom_node->NeedsStyleRecalc();
            if (current_is_dirty) {
                // 对于 Text 子节点，更新继承的样式
                if (child_dom_node->GetNodeType() == NodeType::TEXT_NODE) {
                    auto render_text = dynamic_cast<RenderText*>(child_render_obj);
                    if (render_text) {
                        // 同步文本内容
                        // 关键修复：使用规范化后的文本进行比较
                        auto text_node = static_cast<Text*>(child_dom_node);
                        std::string raw_text = text_node->GetData();
                        std::string normalized_text = NormalizeTextContent(raw_text, current_style);
                        if (render_text->GetText() != normalized_text) {
                            render_text->SetText(normalized_text);
                            child_render_obj->MarkNeedsLayout();

                            // Update content version for incremental layout optimization
                            // **Feature: incremental-layout-optimization**
                            // **Validates: Requirements 1.1**
                            if (layout_engine_) {
                                layout_engine_->UpdateContentVersion(child_render_obj);
                            }
                        }

                        // 更新继承的样式
                        ComputedStyle text_style = child_render_obj->GetComputedStyle();
                        text_style.color = current_style->color;
                        text_style.font_family = current_style->font_family;
                        text_style.font_size = current_style->font_size;
                        text_style.font_weight = current_style->font_weight;
                        text_style.font_style = current_style->font_style;
                        text_style.line_height = current_style->line_height;
                        text_style.text_align = current_style->text_align;
                        text_style.text_decoration = current_style->text_decoration;
                        child_render_obj->SetComputedStyle(text_style);
                        child_render_obj->MarkNeedsPaint();
                    }
                }
                // 对于 Element 子节点，重新计算样式（会自动继承父样式）
                else if (child_dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    StyleResolver resolver;
                    if (document_ && document_->GetStyleManager()) {
                        resolver.SetStyleManager(document_->GetStyleManager());
                    }
                    auto child_element = std::static_pointer_cast<Element>(it->second);
                    auto new_style = resolver.ResolveStyle(child_element, current_style);
                    child_render_obj->SetComputedStyle(new_style);
                    child_render_obj->MarkNeedsPaint();

                    // 关键修复：通知布局引擎样式已更新
                    // 这确保 LayoutNode 的样式与 RenderObject 一致
                    if (layout_engine_) {
                        layout_engine_->UpdateStyle(child_render_obj, new_style);
                    }
                }
            }

            // 递归处理
            MarkRenderObjectsDirty(child_dom_node, child_render_obj);
        }
    }
}

bool Window::LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height) {
    if (!render_obj) {
        return false;
    }

    // 优先使用 NativeLayoutEngine 的增量布局
    if (layout_engine_) {
        // 增量优化：只标记需要布局的 RenderObject，跳过干净的子树
        std::function<void(RenderObject*)> markDirty = [&](RenderObject* obj) {
            if (!obj) return;

            // 检查是否需要布局
            bool needs_layout = obj->NeedsLayout();
            bool child_needs_layout = obj->ChildNeedsLayout();

            // 如果当前节点和子树都不需要布局，跳过
            if (!needs_layout && !child_needs_layout) {
                return;
            }

            if (needs_layout) {
                layout_engine_->MarkNeedsLayout(obj);
            }

            // 只有子树需要布局时才递归
            if (child_needs_layout) {
                for (const auto& child : obj->GetChildren()) {
                    markDirty(child.get());
                }
            }
        };
        markDirty(render_obj);

        // 执行增量布局
        bool did_layout = layout_engine_->ComputeIncrementalLayout(parent_width, parent_height);

        if (did_layout) {
            // 更新 RenderObject 的布局信息
            layout_engine_->GetLayoutInfo(cached_render_tree_);
        }

        return did_layout;
    }

    // 回退到传统的递归布局
    bool needs_layout = render_obj->NeedsLayout();
    bool any_child_laid_out = false;

    // 检查子节点是否需要布局
    const auto& children = render_obj->GetChildren();
    for (const auto& child : children) {
        if (LayoutDirtySubtree(child.get(), parent_width, parent_height)) {
            any_child_laid_out = true;
            needs_layout = true;  // 子节点布局改变，父节点也需要重新布局
        }
    }

    // 如果当前节点或任何子节点需要布局，执行布局
    if (needs_layout) {
        render_obj->Layout(parent_width, parent_height);
        render_obj->ClearNeedsLayout();
        return true;
    }

    return false;
}

void Window::Clear(uint32_t color) {
    if (!surface_) {
        return;
    }

    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        // 将 uint32_t 转换为 SkColor (ARGB)
        SkColor sk_color = SkColorSetARGB(
            (color >> 24) & 0xFF,  // A
            (color >> 16) & 0xFF,  // R
            (color >> 8) & 0xFF,   // G
            color & 0xFF           // B
        );
        canvas->clear(sk_color);
    }
}

void Window::AddDirtyRect(const SkRect& rect) {
    // 忽略空矩形
    if (rect.isEmpty()) {
        return;
    }

    // 自动膨胀脏区域以容纳抗锯齿、子像素偏移和阴影
    SkRect inflated_rect = rect.makeOutset(2.0f, 2.0f);

    // 合并策略：如果新矩形与已有矩形重叠或距离较近，合并它们
    constexpr float kMergeThreshold = 10.0f;

    for (auto& existing : dirty_rects_) {
        // 扩展现有矩形检测重叠
        SkRect expanded = existing.makeOutset(kMergeThreshold, kMergeThreshold);
        if (expanded.intersects(inflated_rect)) {
            // 合并矩形
            existing.join(inflated_rect);
            return;
        }
    }

    // 没有重叠，添加新矩形
    dirty_rects_.push_back(inflated_rect);

    // 如果脏区域太多，尝试合并相邻的区域
    constexpr size_t kMaxDirtyRects = 100;  // 提高阈值
    if (dirty_rects_.size() > kMaxDirtyRects) {
        // 策略：找到最近的两个矩形并合并它们
        // 重复这个过程直到数量降到阈值以下
        while (dirty_rects_.size() > kMaxDirtyRects) {
            float min_distance = std::numeric_limits<float>::max();
            size_t merge_i = 0, merge_j = 1;

            // 找到距离最近的两个矩形
            for (size_t i = 0; i < dirty_rects_.size(); ++i) {
                for (size_t j = i + 1; j < dirty_rects_.size(); ++j) {
                    const auto& r1 = dirty_rects_[i];
                    const auto& r2 = dirty_rects_[j];

                    // 计算两个矩形中心点的距离
                    float cx1 = (r1.left() + r1.right()) / 2.0f;
                    float cy1 = (r1.top() + r1.bottom()) / 2.0f;
                    float cx2 = (r2.left() + r2.right()) / 2.0f;
                    float cy2 = (r2.top() + r2.bottom()) / 2.0f;

                    float dx = cx2 - cx1;
                    float dy = cy2 - cy1;
                    float distance = dx * dx + dy * dy;  // 不需要开方，比较大小即可

                    if (distance < min_distance) {
                        min_distance = distance;
                        merge_i = i;
                        merge_j = j;
                    }
                }
            }

            // 合并最近的两个矩形
            dirty_rects_[merge_i].join(dirty_rects_[merge_j]);
            dirty_rects_.erase(dirty_rects_.begin() + merge_j);
        }
    }
}

void Window::UpdateAnimations(double current_time) {
    // 委托给 WindowRenderer 处理
    if (window_renderer_) {
        window_renderer_->UpdateAnimations(current_time);
    }
}

void Window::ApplyAnimationsToRenderTree(RenderObject* root) {
    // 委托给 WindowRenderer 处理
    if (window_renderer_) {
        window_renderer_->ApplyAnimationsToRenderTree(root);
    }
}

bool Window::HasPendingAnimations(RenderObject* root) const {
    // 委托给 WindowRenderer 处理
    if (window_renderer_) {
        return window_renderer_->HasPendingAnimations(root);
    }
    return false;
}

float Window::GetDisplayScale() const {
    if (!sdl_window_) {
        return 1.0f;
    }

    // 方法 1: 尝试使用 SDL_GetWindowDisplayScale (SDL3)
    float scale = SDL_GetWindowDisplayScale(sdl_window_);
    if (scale > 1.0f) {
        return scale;
    }

    // 方法 2: 通过物理像素和逻辑像素的比值计算
    int logical_width, logical_height;
    SDL_GetWindowSize(sdl_window_, &logical_width, &logical_height);

    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

    if (logical_width > 0 && physical_width != logical_width) {
        float calculated_scale = static_cast<float>(physical_width) / static_cast<float>(logical_width);
        if (calculated_scale > 1.0f) {
            return calculated_scale;
        }
    }

    // 方法 3: 使用 SDL_GetDisplayContentScale
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(sdl_window_);
    if (display_id != 0) {
        float content_scale = SDL_GetDisplayContentScale(display_id);
        if (content_scale > 1.0f) {
            return content_scale;
        }
    }

    return 1.0f;
}

int Window::LogicalToPhysicalPixels(int value) const {
    if (value <= 0) {
        return value;
    }

    const float scale = GetDisplayScale();
    const int physical = static_cast<int>(std::lround(static_cast<float>(value) * scale));
    return std::max(physical, 1);
}

void Window::GetPhysicalSize(int* width, int* height) const {
    if (sdl_window_) {
        int physical_width = 0;
        int physical_height = 0;
        SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
        if (width) *width = physical_width;
        if (height) *height = physical_height;
        return;
    }

    if (width) *width = LogicalToPhysicalPixels(config_.width);
    if (height) *height = LogicalToPhysicalPixels(config_.height);
}

RenderBackend Window::GetActualBackend() const {
    return actual_backend_;
}

bool Window::HasSurface() const {
    return surface_ != nullptr;
}

bool Window::HasGrContext() const {
    return gr_context_ != nullptr;
}

bool Window::HasFBOManager() const {
    return fbo_manager_ != nullptr;
}

size_t Window::GetEstimatedSurfaceBytes() const {
    int width = 0;
    int height = 0;
    GetPhysicalSize(&width, &height);
    if (width <= 0 || height <= 0 || !surface_) {
        return 0;
    }
    return static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
}

size_t Window::GetEstimatedFBOTextureBytes() const {
    return fbo_manager_ ? fbo_manager_->GetEstimatedTextureBytes() : 0;
}

size_t Window::GetEstimatedFBODepthStencilBytes() const {
    return fbo_manager_ ? fbo_manager_->GetEstimatedDepthStencilBytes() : 0;
}

size_t Window::GetEstimatedFBOTotalBytes() const {
    return fbo_manager_ ? fbo_manager_->GetEstimatedTotalBytes() : 0;
}

size_t Window::GetSkiaResourceCacheBytes() const {
    if (!gr_context_) {
        return 0;
    }

    int resource_count = 0;
    size_t resource_bytes = 0;
    gr_context_->getResourceCacheUsage(&resource_count, &resource_bytes);
    return resource_bytes;
}

int Window::GetSkiaResourceCacheCount() const {
    if (!gr_context_) {
        return 0;
    }

    int resource_count = 0;
    size_t resource_bytes = 0;
    gr_context_->getResourceCacheUsage(&resource_count, &resource_bytes);
    return resource_count;
}

size_t Window::GetSkiaResourceCacheLimit() const {
    return gr_context_ ? gr_context_->getResourceCacheLimit() : 0;
}

void Window::PurgeSkiaResourceCache() {
    if (!gr_context_) {
        return;
    }

    gr_context_->performDeferredCleanup(std::chrono::milliseconds(0));
    gr_context_->purgeUnlockedResources(GrPurgeResourceOptions::kAllResources);
    gr_context_->flush();
    gr_context_->freeGpuResources();
}

void Window::ForceLayoutSync() {
    // 强制同步布局 - 模拟浏览器的 forced reflow
    // 当 JS 调用 getBoundingClientRect 等方法时，需要立即获取最新的布局信息

    if (!document_ || !layout_engine_) {
        return;
    }

    // 确保渲染树已构建
    const bool render_tree_rebuild_required = (!render_tree_valid_ || !cached_render_tree_);
    EnsureRenderTree();

    if (!cached_render_tree_) {
        return;
    }

    // 关键修复：ForceLayoutSync 全量重建前也要先清理 detached binding，避免 Clear() 直接丢失卸载记录
    if (render_tree_rebuild_required) {
        RenderTreeSynchronizer::CleanupDetachedDOMBindings(document_->GetDirtyTracker());
        document_->GetDirtyTracker().Clear();
    }

    // 获取视口尺寸
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
    float dpi_scale = GetDisplayScale();
    float width = static_cast<float>(physical_width) / dpi_scale;
    float height = static_cast<float>(physical_height) / dpi_scale;

    // 考虑 DevTools 面板
    auto& devtools = DevToolsManager::GetInstance();
    float app_width = width;
    float app_height = height;
    if (devtools.IsOpen()) {
        float app_x, app_y;
        devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);
    }

    // 处理待处理的 DOM 变化
    bool needs_rebuild = false;
    if (!render_tree_rebuild_required && render_tree_synchronizer_) {
        auto& tracker = document_->GetDirtyTracker();
        bool has_pending = tracker.HasPendingChanges();

        if (has_pending) {
            needs_rebuild = render_tree_synchronizer_->Synchronize(tracker, cached_render_tree_);
        }
    }

    // 重建布局树并计算布局
    // 如果有 DOM 变化，强制重建布局树
    layout_engine_->BuildLayoutTree(cached_render_tree_, needs_rebuild);
    layout_engine_->ComputeLayout(app_width, app_height);
    layout_engine_->GetLayoutInfo(cached_render_tree_);
}

void Window::InvalidateRenderTree() {
    // 标记渲染树需要重建
    render_tree_valid_ = false;

    // 关键修复：在清空渲染树之前保存滚动位置
    // 这样 EnsureRenderTree() 重建时可以恢复滚动状态
    if (cached_render_tree_ && window_renderer_) {
        saved_scroll_positions_.clear();
        window_renderer_->SaveScrollPositions(cached_render_tree_.get(), saved_scroll_positions_);
    }

    // 关键修复：清理 LayoutEngine 的映射
    // NativeLayoutEngine 持有 render_to_node_ 映射（RenderObject* -> NodeId）
    // 当渲染树重建时，旧的 RenderObject 被销毁，映射中的指针变成悬空指针
    // 必须在销毁渲染树之前清理，否则后续访问会崩溃
    if (layout_engine_) {
        layout_engine_->Clear();
    }

    // 清空旧的渲染树
    cached_render_tree_.reset();

    // 通知统一渲染管线需要重建层树
    if (render_pipeline_) {
        render_pipeline_->InvalidateLayerTree();
        render_pipeline_->ForceFullUpdate();
    }

    // 注意：不再清理运行中的动画状态
    // 动画现在通过 Element 引用关联，而不是 RenderObject 指针
    // 渲染树重建时，动画会通过 Element 获取新的 RenderObject
    // 这样动画可以在 DOM 变化（如添加 Modal）时继续运行

    // 只清理 AnimationApplicator 的跟踪信息（started_animations_ map）
    // 因为它使用 RenderObject* 作为键
    if (animation_applicator_) {
        animation_applicator_->Clear();
    }
}

void Window::EnsureRenderTree() {
    if (render_tree_valid_ && cached_render_tree_) {
        return;  // 渲染树已经有效
    }

    if (!document_) {
        return;
    }

    auto body = document_->GetBody();
    if (!body) {
        return;
    }

    // 在重建渲染树前，优先使用 InvalidateRenderTree 保存的滚动位置
    // 如果没有（说明不是通过 InvalidateRenderTree 触发的重建），则尝试从当前渲染树保存
    std::unordered_map<Node*, std::pair<float, float>> scroll_positions;
    if (!saved_scroll_positions_.empty()) {
        scroll_positions = std::move(saved_scroll_positions_);
        saved_scroll_positions_.clear();
    } else if (cached_render_tree_ && window_renderer_) {
        window_renderer_->SaveScrollPositions(cached_render_tree_.get(), scroll_positions);
    }

    // 构建渲染树
    if (!render_tree_builder_) {
        render_tree_builder_ = std::make_shared<RenderTreeBuilder>();
    }
    render_tree_builder_->SetDocument(document_.get());

    auto& resolver = render_tree_builder_->GetStyleResolver();
    if (document_->GetStyleManager()) {
        resolver.SetStyleManager(document_->GetStyleManager());
    }

    // 关键：body 作为渲染树入口时，仍然必须继承 html 根元素的计算样式。
    // 否则 :root/html 上定义的 CSS 变量、color、font 等可继承值不会进入 body，
    // 看起来就会像 style 标签整体失效，而 inline var/fallback 仍然正常。
    const ComputedStyle* body_parent_style = nullptr;
    ComputedStyle html_style;
    if (auto document_element = document_->GetDocumentElement()) {
        html_style = resolver.ResolveStyle(document_element, nullptr);
        body_parent_style = &html_style;
    }

    cached_render_tree_ = render_tree_builder_->BuildRenderTree(body, body_parent_style);

    if (!cached_render_tree_) {
        return;
    }

    // 初始化渲染树同步器
    if (!render_tree_synchronizer_) {
        render_tree_synchronizer_ = std::make_shared<RenderTreeSynchronizer>();
        render_tree_synchronizer_->SetDocument(document_);
        render_tree_synchronizer_->SetRenderTreeBuilder(render_tree_builder_);
        if (layout_engine_) {
            render_tree_synchronizer_->SetLayoutEngine(std::shared_ptr<LayoutEngine>(
                layout_engine_.get(), [](LayoutEngine*) {}));
        }
    }

    // 初始化增量布局管理器
    if (!incremental_layout_manager_) {
        incremental_layout_manager_ = std::make_unique<IncrementalLayoutManager>(this);
    }

    // 获取窗口尺寸
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

    // 获取 DPI 缩放比
    float dpi_scale = GetDisplayScale();

    // 计算逻辑大小
    float width = static_cast<float>(physical_width) / dpi_scale;
    float height = static_cast<float>(physical_height) / dpi_scale;

    // 检查 DevTools 是否打开，如果打开则调整主应用区域
    auto& devtools = DevToolsManager::GetInstance();
    float app_width = width;
    float app_height = height;

    if (devtools.IsOpen()) {
        float app_x, app_y;
        devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);
    }

    // 设置视口尺寸
    RenderObject::SetViewportSize(app_width, app_height);

    // 使用 Taffy 布局引擎计算布局（使用调整后的尺寸）
    if (layout_engine_) {
        layout_engine_->BuildLayoutTree(cached_render_tree_);
        layout_engine_->ComputeLayout(app_width, app_height);
        layout_engine_->GetLayoutInfo(cached_render_tree_);
    } else {
        cached_render_tree_->Layout(app_width, app_height);
    }

    // 关键修复：在布局计算完成后恢复滚动位置
    // 这样可以确保滚动位置不会被布局计算重置
    if (!scroll_positions.empty() && window_renderer_) {
        window_renderer_->RestoreScrollPositions(cached_render_tree_.get(), scroll_positions);
    }

    render_tree_valid_ = true;
}

bool Window::HitTestDragRegion(int screen_x, int screen_y) const {
    // 无边框模式下才需要判断拖拽区域
    if (!config_.borderless || !sdl_window_ || !cached_render_tree_) {
        return false;
    }

    // 将屏幕坐标转换为窗口客户区坐标
    int window_x, window_y;
    SDL_GetWindowPosition(sdl_window_, &window_x, &window_y);
    int client_x = screen_x - window_x;
    int client_y = screen_y - window_y;

    // 转换为逻辑坐标（考虑 DPI 缩放）
    float dpi_scale = GetDisplayScale();
    float logical_x = static_cast<float>(client_x) / dpi_scale;
    float logical_y = static_cast<float>(client_y) / dpi_scale;

    // 使用 HitTestController 查找命中的元素
    HitTestController hit_controller;
    HitTestRequest request;
    request.ignore_pointer_events = true;  // 拖拽区域不受 pointer-events 影响
    request.test_visibility = true;
    request.test_opacity = false;          // 透明元素也可以是拖拽区域

    auto result = hit_controller.HitTest(cached_render_tree_, logical_x, logical_y, request);
    if (!result.IsValid() || !result.element) {
        return false;
    }

    // 从命中元素向上遍历 DOM 树，查找最近的 app_region 设置
    // 规则：最近的 app_region 设置生效（no-drag 覆盖 drag）
    auto element = result.element;
    while (element) {
        // 查找元素关联的渲染对象
        auto node = std::dynamic_pointer_cast<Node>(element);
        if (node) {
            auto render_obj = node->GetRenderObject();
            if (render_obj) {
                const auto& style = render_obj->GetComputedStyle();
                if (!style.app_region.empty()) {
                    return style.app_region == "drag";
                }
            }
        }

        // 向上遍历到父元素
        auto parent_node = element->GetParentNode();
        if (!parent_node || parent_node->GetNodeType() != NodeType::ELEMENT_NODE) {
            break;
        }
        element = std::dynamic_pointer_cast<Element>(parent_node);
    }

    return false;
}

std::string Window::HitTestWindowControl(int screen_x, int screen_y) const {
    // 无边框模式下才需要判断窗口控制区域
    if (!config_.borderless || !sdl_window_ || !cached_render_tree_) {
        return "";
    }

    // 将屏幕坐标转换为窗口客户区坐标
    int window_x, window_y;
    SDL_GetWindowPosition(sdl_window_, &window_x, &window_y);
    int client_x = screen_x - window_x;
    int client_y = screen_y - window_y;

    // 转换为逻辑坐标（考虑 DPI 缩放）
    float dpi_scale = GetDisplayScale();
    float logical_x = static_cast<float>(client_x) / dpi_scale;
    float logical_y = static_cast<float>(client_y) / dpi_scale;

    // 使用 HitTestController 查找命中的元素
    HitTestController hit_controller;
    HitTestRequest request;
    request.ignore_pointer_events = true;
    request.test_visibility = true;
    request.test_opacity = false;

    auto result = hit_controller.HitTest(cached_render_tree_, logical_x, logical_y, request);
    if (!result.IsValid() || !result.element) {
        return "";
    }

    // 从命中元素向上遍历 DOM 树，查找最近的 window_control 设置
    auto element = result.element;
    while (element) {
        auto node = std::dynamic_pointer_cast<Node>(element);
        if (node) {
            auto render_obj = node->GetRenderObject();
            if (render_obj) {
                const auto& style = render_obj->GetComputedStyle();
                if (!style.window_control.empty()) {
                    return style.window_control;
                }
            }
        }

        auto parent_node = element->GetParentNode();
        if (!parent_node || parent_node->GetNodeType() != NodeType::ELEMENT_NODE) {
            break;
        }
        element = std::dynamic_pointer_cast<Element>(parent_node);
    }

    return "";
}

} // namespace mbink


