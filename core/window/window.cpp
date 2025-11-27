/**
 * @file window.cpp
 * @brief 窗口管理模块实现
 *
 * 实现内容：
 * - SDL3窗口创建和管理
 * - OpenGL上下文初始化
 * - Skia渲染表面创建
 * - 窗口事件处理
 */

// 性能优化：默认关闭调试日志（可以通过定义LIGHTUI_DEBUG_RENDERING启用）
// #define LIGHTUI_DEBUG_RENDERING

#ifdef LIGHTUI_DEBUG_RENDERING
    #define DEBUG_LOG(msg) std::cout << msg << std::endl
    #define DEBUG_LOG_FLUSH() std::cout.flush()
#else
    #define DEBUG_LOG(msg) ((void)0)
    #define DEBUG_LOG_FLUSH() ((void)0)
#endif

#include "window.h"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "include/core/SkColorSpace.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkFont.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/dom_observer.h"
#include "core/render/renderer.h"
#include "core/render/render_object.h"
#include "core/render/style_resolver.h"
#include "core/render/text/font_manager.h"
#include "core/render/dirty_region.h"
#include "core/render/dirty_region_collector.h"
#include "core/render/transition.h"
#include "core/render/animation_timeline.h"
#include "core/render/animation_controller.h"
#include "core/layout/layout_engine.h"
#include "core/render/color.h"

namespace lightui {

/**
 * @brief Window 的 DOM 观察者
 *
 * 监听 DOM 变化并触发窗口重绘
 */
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window) : window_(window) {}

    void OnNodeAdded(Node* node, Node* parent) override {
        if (window_ && !IsInBatch(node)) {
            window_->SetNeedsRepaint();
            window_->InvalidateRenderTree();
        }
    }

    void OnNodeRemoved(Node* node, Node* parent) override {
        if (window_ && !IsInBatch(node)) {
            window_->SetNeedsRepaint();
            window_->InvalidateRenderTree();
        }
    }

    void OnAttributeChanged(Element* element,
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override {
        if (window_ && !IsInBatch(element)) {
            window_->SetNeedsRepaint();
        }
    }

    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override {
        if (window_ && !IsInBatch(element)) {
            window_->SetNeedsRepaint();
        }
    }

    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override {
        if (window_ && !IsInBatch(node)) {
            window_->SetNeedsRepaint();
        }
    }

    void OnSubtreeModified(Node* root) override {
        if (window_) {
            window_->SetNeedsRepaint();
            window_->InvalidateRenderTree();
        }
    }

    void OnPseudoClassChanged(std::shared_ptr<Element> element,
                             const std::string& pseudo_class,
                             bool activate) override {
        if (window_ && !IsInBatch(element.get())) {
            // 性能优化：只有可能有视觉变化的伪类才触发重绘
            bool needs_repaint = false;

            if (pseudo_class == "hover") {
                // 只有这些元素有内置的 :hover 样式
                std::string tag = element->GetTagName();
                if (tag == "button" || tag == "a" || tag == "input" ||
                    tag == "textarea" || tag == "select") {
                    needs_repaint = true;
                }
            } else if (pseudo_class == "active" || pseudo_class == "focus" ||
                       pseudo_class == "focus-visible" || pseudo_class == "checked" ||
                       pseudo_class == "disabled") {
                needs_repaint = true;
            }

            if (needs_repaint) {
                window_->SetNeedsRepaint();
            }
        }
    }

private:
    /**
     * @brief 检查节点是否在批量更新中
     */
    bool IsInBatch(Node* node) const {
        if (!node) return false;

        auto doc = node->GetOwnerDocument();
        if (!doc) return false;

        auto document = std::dynamic_pointer_cast<Document>(doc);
        if (!document) return false;

        return document->IsInBatch();
    }

    Window* window_;
};

// 静态成员：SDL初始化计数器
static int sdl_init_count = 0;

Window::Window(const WindowConfig& config) : config_(config) {
    InitSDL();
    CreateSDLWindow();

    // 根据配置选择渲染后端
    if (config_.backend == RenderBackend::AUTO) {
        // 自动模式：先尝试 GPU，失败则降级到 CPU
        try {
            InitOpenGL();
            InitSkia();
            CreateSkiaSurface();
            actual_backend_ = RenderBackend::OPENGL;
        } catch (const std::exception& e) {
            // GPU 初始化失败，降级到 CPU 软件渲染
            std::cerr << "GPU rendering failed: " << e.what() << ", falling back to CPU" << std::endl;
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

    // 初始化 Taffy CSS 布局引擎
    layout_engine_ = std::make_unique<LayoutEngine>();
}

Window::~Window() {
    // 关键修复：在释放 Skia 资源之前，先激活 OpenGL 上下文
    // Skia 的 GrContext 在释放时需要调用 OpenGL 清理函数
    if (gl_context_ && sdl_window_) {
        SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    }

    // 释放Skia资源
    surface_.reset();
    gr_context_.reset();

    // 销毁 SDL Texture 和 Renderer（CPU 模式）
    if (sdl_texture_) {
        SDL_DestroyTexture(sdl_texture_);
        sdl_texture_ = nullptr;
    }

    if (sdl_renderer_) {
        SDL_DestroyRenderer(sdl_renderer_);
        sdl_renderer_ = nullptr;
    }

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
}

void Window::Show() {
    if (sdl_window_) {
        SDL_ShowWindow(sdl_window_);
    }
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
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
}

void Window::SetSize(int width, int height) {
    config_.width = width;
    config_.height = height;
    if (sdl_window_) {
        SDL_SetWindowSize(sdl_window_, width, height);
        OnResize();
    }
}

void Window::GetSize(int* width, int* height) const {
    if (sdl_window_) {
        SDL_GetWindowSize(sdl_window_, width, height);
    } else {
        if (width) *width = config_.width;
        if (height) *height = config_.height;
    }
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

void Window::SwapBuffers() {
    if (!sdl_window_) {
        return;
    }

    if (actual_backend_ == RenderBackend::OPENGL && gr_context_) {
        // GPU 模式：刷新 Skia 命令并交换 OpenGL 缓冲区
        gr_context_->flush();
        SDL_GL_SwapWindow(sdl_window_);
    }
    else if (actual_backend_ == RenderBackend::CPU && surface_) {
        // CPU 模式：将 Skia 渲染结果复制到 SDL Texture 并显示

        if (!sdl_renderer_ || !sdl_texture_) {
            return;
        }

        // 获取 Skia surface 的像素数据
        SkPixmap pixmap;
        if (!surface_->peekPixels(&pixmap)) {
            std::cerr << "Failed to peek pixels from Skia surface" << std::endl;
            return;
        }

        // 更新纹理
        const void* pixels = pixmap.addr();
        int pitch = pixmap.rowBytes();

        if (!SDL_UpdateTexture(sdl_texture_, nullptr, pixels, pitch)) {
            std::cerr << "Failed to update SDL texture: " << SDL_GetError() << std::endl;
            return;
        }

        // 清空渲染器并绘制纹理
        SDL_RenderClear(sdl_renderer_);
        SDL_RenderTexture(sdl_renderer_, sdl_texture_, nullptr, nullptr);
        SDL_RenderPresent(sdl_renderer_);
    }
}

void Window::OnResize() {
    if (!sdl_window_) return;

    // 获取客户区大小（像素，不包括标题栏和边框）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 同时更新config中的窗口大小（逻辑大小）
    int logical_width, logical_height;
    SDL_GetWindowSize(sdl_window_, &logical_width, &logical_height);
    config_.width = logical_width;
    config_.height = logical_height;

    // 重新创建Skia渲染表面（使用客户区像素大小）
    if (actual_backend_ == RenderBackend::OPENGL) {
        CreateSkiaSurface();
    } else if (actual_backend_ == RenderBackend::CPU) {
        // CPU 模式：重新创建 Skia Raster 表面和 SDL Texture
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface_ = SkSurfaces::Raster(info);

        // 重新创建 SDL Texture
        if (sdl_texture_) {
            SDL_DestroyTexture(sdl_texture_);
        }

        SDL_PixelFormat sdl_format;
        if (info.colorType() == kRGBA_8888_SkColorType) {
            sdl_format = SDL_PIXELFORMAT_RGBA32;
        } else if (info.colorType() == kBGRA_8888_SkColorType) {
            sdl_format = SDL_PIXELFORMAT_BGRA32;
        } else {
            sdl_format = SDL_PIXELFORMAT_ARGB8888;
        }

        sdl_texture_ = SDL_CreateTexture(
            sdl_renderer_,
            sdl_format,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );
    }

    // 触发resize回调（使用逻辑大小）
    if (on_resize_callback_) {
        on_resize_callback_(logical_width, logical_height);
    }
}

void Window::InitSDL() {
    // 只在第一次调用时初始化SDL
    if (sdl_init_count == 0) {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
        }
    }
    sdl_init_count++;
}

void Window::CreateSDLWindow() {
    // 构建窗口标志
    SDL_WindowFlags flags = 0;

    // 只有 GPU 模式才需要 OpenGL 标志
    if (config_.backend == RenderBackend::OPENGL || config_.backend == RenderBackend::AUTO) {
        flags |= SDL_WINDOW_OPENGL;
    }

    if (config_.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (config_.borderless) flags |= SDL_WINDOW_BORDERLESS;
    if (config_.maximized) flags |= SDL_WINDOW_MAXIMIZED;
    if (config_.minimized) flags |= SDL_WINDOW_MINIMIZED;
    if (config_.hidden) flags |= SDL_WINDOW_HIDDEN;
    if (config_.always_on_top) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (config_.high_dpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    // 创建窗口
    sdl_window_ = SDL_CreateWindow(
        config_.title.c_str(),
        config_.width,
        config_.height,
        flags
    );

    if (!sdl_window_) {
        throw std::runtime_error(std::string("Failed to create SDL window: ") + SDL_GetError());
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
}

void Window::InitCPURendering() {
    // CPU 软件渲染模式 - 不需要 OpenGL
    // 创建 SDL Renderer 和 Texture

    // 使用物理像素大小创建渲染表面（支持高 DPI）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 创建 SDL Renderer
    sdl_renderer_ = SDL_CreateRenderer(sdl_window_, nullptr);
    if (!sdl_renderer_) {
        throw std::runtime_error(std::string("Failed to create SDL renderer: ") + SDL_GetError());
    }

    // 禁用 VSync，由 FrameController 控制帧率
    SDL_SetRenderVSync(sdl_renderer_, 0);

    // 创建 Raster 表面（CPU 渲染）- 使用物理像素大小
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface_ = SkSurfaces::Raster(info);

    if (!surface_) {
        throw std::runtime_error("Failed to create CPU rendering surface");
    }

    // 根据 Skia 的颜色类型选择 SDL 像素格式
    SDL_PixelFormat sdl_format;
    if (info.colorType() == kRGBA_8888_SkColorType) {
        sdl_format = SDL_PIXELFORMAT_RGBA32;
    } else if (info.colorType() == kBGRA_8888_SkColorType) {
        sdl_format = SDL_PIXELFORMAT_BGRA32;
    } else {
        sdl_format = SDL_PIXELFORMAT_ARGB8888;
    }

    // 创建 SDL Texture
    sdl_texture_ = SDL_CreateTexture(
        sdl_renderer_,
        sdl_format,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!sdl_texture_) {
        throw std::runtime_error(std::string("Failed to create SDL texture: ") + SDL_GetError());
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
                static int last_resize_width = 0, last_resize_height = 0;
                static Uint64 last_resize_time = 0;
                Uint64 current_time = SDL_GetTicks();

                if (new_width == last_resize_width && new_height == last_resize_height) {
                    return true;  // 大小没变，跳过
                }

                // 节流：如果距离上次 resize 不到 16ms (约60fps)，延迟处理
                // 但如果是最终的大小，我们需要处理它
                if (current_time - last_resize_time < 16 && event.type == SDL_EVENT_WINDOW_RESIZED) {
                    // 记录期望的大小，但不立即处理
                    // SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED 通常是最终事件
                    return true;
                }

                last_resize_width = new_width;
                last_resize_height = new_height;
                last_resize_time = current_time;

                OnResize();
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, new_width, new_height));
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
                DispatchWindowEvent(WindowEvent(WindowEventType::MAXIMIZE));
                return true;
            }

            case SDL_EVENT_WINDOW_RESTORED: {
                DispatchWindowEvent(WindowEvent(WindowEventType::RESTORE));
                return true;
            }

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                should_close_ = true;
                if (on_close_callback_) {
                    on_close_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::CLOSE));
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
        dom_observer_ = std::make_unique<WindowDOMObserver>(this);
        document_->AddObserver(dom_observer_.get());
    }

    // 标记需要重绘
    SetNeedsRepaint();
}

void Window::RenderDocument() {
    if (!document_ || !surface_) {
        return;
    }

    // 更新动画
    static Uint64 start_time = SDL_GetPerformanceCounter();
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double timestamp_sec = static_cast<double>(current_time - start_time) / frequency;
    UpdateAnimations(timestamp_sec);

    // 获取画布
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }

    // TODO: 实现完整的渲染管线
    // 1. 样式解析
    // 2. 布局计算
    // 3. 渲染树构建
    // 4. 绘制

    // 使用缓存的渲染树，避免每次重建导致滚动状态丢失
    auto body = document_->GetBody();
    if (body) {
        // 获取物理像素大小
        int physical_width, physical_height;
        SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

        // 获取 DPI 缩放比
        float dpi_scale = GetDisplayScale();

        // 计算逻辑大小（CSS 像素）- 像浏览器一样
        int logical_width = static_cast<int>(physical_width / dpi_scale);
        int logical_height = static_cast<int>(physical_height / dpi_scale);

        // 检查是否需要重建渲染树和重新布局
        bool needs_layout = false;
        if (!render_tree_valid_ || !cached_render_tree_) {
            // 使用 RenderTreeBuilder 构建渲染树
            RenderTreeBuilder builder;
            builder.SetDocument(document_.get());
            cached_render_tree_ = builder.BuildRenderTree(body, nullptr);
            render_tree_valid_ = true;
            needs_layout = true;
        }

        if (cached_render_tree_) {
            // 获取 body 的背景色并清空画布
            SkColor clear_color = SK_ColorWHITE;  // 默认白色
            const auto& body_style = cached_render_tree_->GetComputedStyle();
            if (!body_style.background_color.empty() && body_style.background_color != "transparent") {
                clear_color = Color::Parse(body_style.background_color);
            }
            canvas->clear(clear_color);

            // 只在需要时重新计算布局（渲染树重建或窗口大小改变）
            static int last_logical_width = 0, last_logical_height = 0;
            if (needs_layout || logical_width != last_logical_width || logical_height != last_logical_height) {
                last_logical_width = logical_width;
                last_logical_height = logical_height;

                // 使用逻辑大小进行布局计算（CSS 像素）
                if (layout_engine_) {
                    layout_engine_->BuildLayoutTree(cached_render_tree_);
                    layout_engine_->ComputeLayout(static_cast<float>(logical_width), static_cast<float>(logical_height));
                    layout_engine_->GetLayoutInfo(cached_render_tree_);
                } else {
                    cached_render_tree_->Layout(static_cast<float>(logical_width), static_cast<float>(logical_height));
                }
            }

            // 应用 DPI 缩放到 canvas（将逻辑像素缩放到物理像素）
            canvas->save();
            canvas->scale(dpi_scale, dpi_scale);

            // 绘制（使用逻辑坐标）
            cached_render_tree_->Paint(canvas);

            canvas->restore();
        }
    }

    // 刷新（Skia 使用 flush() 方法）
    if (gr_context_) {
        gr_context_->flush();
    }

    // 清除重绘标记
    needs_repaint_ = false;
}

void Window::RenderDocumentIncremental() {
    DEBUG_LOG("[Window::RenderDocumentIncremental] Called, needs_repaint_=" << needs_repaint_
              << ", render_tree_valid_=" << render_tree_valid_);

    if (!document_ || !surface_) {
        return;
    }

    // 更新动画
    static Uint64 start_time = SDL_GetPerformanceCounter();
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double timestamp_sec = static_cast<double>(current_time - start_time) / frequency;
    UpdateAnimations(timestamp_sec);

    // 获取画布
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }

    // 获取窗口尺寸
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // Step 1: 构建或复用渲染树
    auto body = document_->GetBody();
    if (!body) {
        return;
    }

    if (!render_tree_valid_ || !cached_render_tree_) {
        // 渲染树无效，需要重建
        DEBUG_LOG("[RenderDocumentIncremental] Rebuilding render tree...");
        RenderTreeBuilder builder;
        builder.SetDocument(document_.get());
        cached_render_tree_ = builder.BuildRenderTree(body, nullptr);
        render_tree_valid_ = true;

        if (!cached_render_tree_) {
            DEBUG_LOG("[RenderDocumentIncremental] Failed to build render tree!");
            return;
        }

        // 新渲染树需要完整布局
        DEBUG_LOG("[RenderDocumentIncremental] Full layout: " << width << "x" << height);

        // 使用 Taffy 布局引擎计算布局
        if (layout_engine_) {
            DEBUG_LOG("[RenderDocumentIncremental] Building Taffy layout tree...");
            layout_engine_->BuildLayoutTree(cached_render_tree_);

            DEBUG_LOG("[RenderDocumentIncremental] Computing layout with Taffy...");
            layout_engine_->ComputeLayout(static_cast<float>(width), static_cast<float>(height));

            DEBUG_LOG("[RenderDocumentIncremental] Reading layout results...");
            layout_engine_->GetLayoutInfo(cached_render_tree_);
        } else {
            // 降级到传统布局
            cached_render_tree_->Layout(static_cast<float>(width), static_cast<float>(height));
        }

        // 清空画布并完整绘制
        canvas->clear(SK_ColorWHITE);
        DEBUG_LOG("[RenderDocumentIncremental] Full paint...");
        cached_render_tree_->Paint(canvas);
    } else {
        // 渲染树有效，执行增量渲染
        DEBUG_LOG("[RenderDocumentIncremental] Incremental rendering...");

        // Step 2: 收集脏区域
        DirtyRegionCollector collector;
        collector.SetViewportSize(static_cast<float>(width), static_cast<float>(height));

        DirtyRegion dirty_region;
        bool has_dirty = collector.CollectFromDOM(body.get(), dirty_region);

        if (has_dirty) {
            // 优化脏区域（合并相邻区域）
            dirty_region.Optimize();

            const auto& dirty_rects = dirty_region.GetRegions();
            DEBUG_LOG("[RenderDocumentIncremental] Found " << dirty_rects.size() << " dirty regions");

            // Step 3: 增量布局 - 只布局脏子树
            DEBUG_LOG("[RenderDocumentIncremental] Incremental layout...");

            // 首先标记渲染树中对应脏DOM节点的RenderObject需要布局
            MarkRenderObjectsDirty(body.get(), cached_render_tree_.get());

            // 执行增量布局
            bool laid_out = LayoutDirtySubtree(cached_render_tree_.get(),
                                               static_cast<float>(width),
                                               static_cast<float>(height));

            if (laid_out) {
                DEBUG_LOG("[RenderDocumentIncremental] Incremental layout completed");
            } else {
                DEBUG_LOG("[RenderDocumentIncremental] No layout needed");
            }

            // Step 4: 局部绘制
            for (const auto& rect : dirty_rects) {
                DEBUG_LOG("[RenderDocumentIncremental] Painting dirty rect: "
                          << rect.x() << "," << rect.y() << " "
                          << rect.width() << "x" << rect.height());

                // 保存画布状态
                canvas->save();

                // 清除脏区域
                SkPaint clear_paint;
                clear_paint.setColor(SK_ColorWHITE);
                canvas->drawRect(rect, clear_paint);

                // 裁剪到脏区域
                canvas->clipRect(rect);

                // 绘制（只有在裁剪区域内的内容会被绘制）
                cached_render_tree_->Paint(canvas);

                // 恢复画布状态
                canvas->restore();
            }

            // 清除DOM节点的脏标记
            ClearDirtyFlags(body.get());
        } else {
            DEBUG_LOG("[RenderDocumentIncremental] No dirty regions, skipping paint");
        }
    }

    // 刷新
    if (gr_context_) {
        gr_context_->flush();
    }

    // 清除重绘标记
    needs_repaint_ = false;
}

void Window::MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj) {
    if (!dom_node || !render_obj) {
        return;
    }

    // 检查DOM节点是否有布局脏标记
    if (dom_node->IsLayoutDirty()) {
        render_obj->MarkNeedsLayout();
        DEBUG_LOG("[MarkRenderObjectsDirty] Marked RenderObject for layout");
    }

    // 检查DOM节点是否有绘制脏标记
    if (dom_node->IsPaintDirty()) {
        render_obj->MarkNeedsPaint();
        DEBUG_LOG("[MarkRenderObjectsDirty] Marked RenderObject for paint");
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

    // 遍历渲染子节点并查找对应的DOM节点（O(n)）
    for (const auto& render_child : render_children) {
        auto render_child_node = render_child->GetNode();
        if (!render_child_node) {
            continue;
        }

        // O(1)查找
        auto it = dom_map.find(render_child_node.get());
        if (it != dom_map.end()) {
            MarkRenderObjectsDirty(it->second.get(), render_child.get());
        }
    }
}

bool Window::LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height) {
    if (!render_obj) {
        return false;
    }

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

void Window::ClearDirtyFlags(Node* node) {
    if (!node) {
        return;
    }

    // 清除当前节点的脏标记
    node->ClearDirty(DirtyType::ALL);

    // 递归清除子节点
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        ClearDirtyFlags(child.get());
    }
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

void Window::UpdateAnimations(double current_time) {
    // 更新 CSS Transition 动画
    bool has_active_animations = false;
    if (animation_timeline_) {
        animation_timeline_->Update(current_time);
        // TODO: 检查是否有活跃的动画
    }

    // 更新 CSS Animation 动画
    if (animation_controller_) {
        animation_controller_->Update(current_time);
        // TODO: 检查是否有活跃的动画
    }

    // 只在有活跃动画时才标记需要重绘
    // 注意：不要在这里无条件调用 SetNeedsRepaint()，否则会导致无限重绘循环
    (void)has_active_animations;
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

    // 构建渲染树
    RenderTreeBuilder builder;
    builder.SetDocument(document_.get());
    cached_render_tree_ = builder.BuildRenderTree(body, nullptr);

    if (!cached_render_tree_) {
        return;
    }

    // 获取窗口尺寸
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 使用 Taffy 布局引擎计算布局
    if (layout_engine_) {
        layout_engine_->BuildLayoutTree(cached_render_tree_);
        layout_engine_->ComputeLayout(static_cast<float>(width), static_cast<float>(height));
        layout_engine_->GetLayoutInfo(cached_render_tree_);
    } else {
        cached_render_tree_->Layout(static_cast<float>(width), static_cast<float>(height));
    }

    render_tree_valid_ = true;
}

} // namespace lightui


