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

#include "window.h"
#include <stdexcept>
#include <iostream>
#include <cstring>
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
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

    void OnNodeRemoved(Node* node, Node* parent) override {
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

    void OnAttributeChanged(Element* element,
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override {
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override {
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override {
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

    void OnSubtreeModified(Node* root) override {
        if (window_) {
            window_->SetNeedsRepaint();
        }
    }

private:
    Window* window_;
};

// 静态成员：SDL初始化计数器
static int sdl_init_count = 0;

Window::Window(const WindowConfig& config) : config_(config) {
    std::cout << "[Window] Constructor started for: " << config.title << std::endl;
    std::cout.flush();

    std::cout << "[Window] Calling InitSDL..." << std::endl;
    std::cout.flush();
    InitSDL();
    std::cout << "[Window] InitSDL completed" << std::endl;
    std::cout.flush();

    std::cout << "[Window] Calling CreateSDLWindow..." << std::endl;
    std::cout.flush();
    CreateSDLWindow();
    std::cout << "[Window] CreateSDLWindow completed" << std::endl;
    std::cout.flush();

    // 根据配置选择渲染后端
    if (config_.backend == RenderBackend::AUTO) {
        // 自动模式：先尝试 GPU，失败则降级到 CPU
        try {
            std::cout << "[Window] Trying GPU rendering..." << std::endl;
            std::cout.flush();

            std::cout << "[Window] Calling InitOpenGL..." << std::endl;
            std::cout.flush();
            InitOpenGL();
            std::cout << "[Window] InitOpenGL completed" << std::endl;
            std::cout.flush();

            std::cout << "[Window] Calling InitSkia..." << std::endl;
            std::cout.flush();
            InitSkia();
            std::cout << "[Window] InitSkia completed" << std::endl;
            std::cout.flush();

            std::cout << "[Window] Calling CreateSkiaSurface..." << std::endl;
            std::cout.flush();
            CreateSkiaSurface();
            std::cout << "[Window] CreateSkiaSurface completed" << std::endl;
            std::cout.flush();

            actual_backend_ = RenderBackend::OPENGL;
            std::cout << "[Window] GPU rendering initialized successfully" << std::endl;
            std::cout.flush();
        } catch (const std::exception& e) {
            // GPU 初始化失败，降级到 CPU 软件渲染
            std::cerr << "⚠️  GPU 渲染初始化失败: " << e.what() << std::endl;
            std::cerr << "   降级到 CPU 软件渲染..." << std::endl;
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

    std::cout << "[Window] Constructor completed" << std::endl;
    std::cout.flush();
}

Window::~Window() {
    std::cout << "[Window] Destructor START" << std::endl;
    std::cout.flush();

    // 关键修复：在释放 Skia 资源之前，先激活 OpenGL 上下文
    // Skia 的 GrContext 在释放时需要调用 OpenGL 清理函数
    if (gl_context_ && sdl_window_) {
        std::cout << "[Window] Making OpenGL context current before cleanup..." << std::endl;
        std::cout.flush();
        SDL_GL_MakeCurrent(sdl_window_, gl_context_);
        std::cout << "[Window] OpenGL context is now current" << std::endl;
        std::cout.flush();
    }

    // 释放Skia资源
    std::cout << "[Window] Releasing Skia surface..." << std::endl;
    std::cout.flush();
    surface_.reset();
    std::cout << "[Window] Skia surface released" << std::endl;
    std::cout.flush();

    std::cout << "[Window] Releasing Skia context..." << std::endl;
    std::cout.flush();
    gr_context_.reset();
    std::cout << "[Window] Skia context released" << std::endl;
    std::cout.flush();

    // 销毁 SDL Texture 和 Renderer（CPU 模式）
    if (sdl_texture_) {
        std::cout << "[Window] Destroying SDL texture..." << std::endl;
        std::cout.flush();
        SDL_DestroyTexture(sdl_texture_);
        std::cout << "[Window] SDL texture destroyed" << std::endl;
        std::cout.flush();
        sdl_texture_ = nullptr;
    }

    if (sdl_renderer_) {
        std::cout << "[Window] Destroying SDL renderer..." << std::endl;
        std::cout.flush();
        SDL_DestroyRenderer(sdl_renderer_);
        std::cout << "[Window] SDL renderer destroyed" << std::endl;
        std::cout.flush();
        sdl_renderer_ = nullptr;
    }

    // 销毁OpenGL上下文
    if (gl_context_) {
        std::cout << "[Window] Destroying OpenGL context..." << std::endl;
        std::cout.flush();
        SDL_GL_DestroyContext(gl_context_);
        std::cout << "[Window] OpenGL context destroyed" << std::endl;
        std::cout.flush();
        gl_context_ = nullptr;
    }

    // 销毁SDL窗口
    if (sdl_window_) {
        std::cout << "[Window] Destroying SDL window..." << std::endl;
        std::cout.flush();
        SDL_DestroyWindow(sdl_window_);
        std::cout << "[Window] SDL window destroyed" << std::endl;
        std::cout.flush();
        sdl_window_ = nullptr;
    }

    // 清理SDL（如果是最后一个窗口）
    sdl_init_count--;
    std::cout << "[Window] SDL init count: " << sdl_init_count << std::endl;
    std::cout.flush();

    if (sdl_init_count == 0) {
        std::cout << "[Window] Calling SDL_Quit()..." << std::endl;
        std::cout.flush();
        SDL_Quit();
        std::cout << "[Window] SDL_Quit() completed" << std::endl;
        std::cout.flush();
    }

    std::cout << "[Window] Destructor END" << std::endl;
    std::cout.flush();
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
        // CPU 模式：将 Raster surface 内容复制到 SDL Texture 并渲染

        static int swap_count = 0;
        if (swap_count == 0) {
            std::cout << "[SwapBuffers] CPU mode - first swap" << std::endl;

            // 检查 SDL Texture 大小
            float tex_w, tex_h;
            SDL_GetTextureSize(sdl_texture_, &tex_w, &tex_h);
            std::cout << "[SwapBuffers] Texture size: " << tex_w << "x" << tex_h << std::endl;

            // 检查 SDL Renderer 输出大小
            int out_w, out_h;
            SDL_GetRenderOutputSize(sdl_renderer_, &out_w, &out_h);
            std::cout << "[SwapBuffers] Renderer output size: " << out_w << "x" << out_h << std::endl;
        }
        swap_count++;

        if (!sdl_renderer_) {
            std::cerr << "SDL renderer is null!" << std::endl;
            return;
        }

        if (!sdl_texture_) {
            std::cerr << "SDL texture is null!" << std::endl;
            return;
        }

        // 获取 Skia surface 的像素数据
        SkPixmap pixmap;
        if (!surface_->peekPixels(&pixmap)) {
            std::cerr << "Failed to peek pixels from Skia surface" << std::endl;
            return;
        }

        // 使用 SDL_UpdateTexture 直接更新纹理（比 Lock/Unlock 更简单可靠）
        // 注意：SDL3 中成功返回 true (非零)，失败返回 false (0)
        const void* pixels = pixmap.addr();
        int pitch = pixmap.rowBytes();

        bool result = SDL_UpdateTexture(sdl_texture_, nullptr, pixels, pitch);
        if (!result) {
            const char* error = SDL_GetError();
            std::cerr << "Failed to update SDL texture: "
                      << (error ? error : "no error message") << std::endl;
            return;
        }

        // 渲染纹理到窗口（不需要清空，因为纹理会覆盖整个窗口）
        // SDL3 中成功返回 true (非零)，失败返回 false (0)
        if (!SDL_RenderTexture(sdl_renderer_, sdl_texture_, nullptr, nullptr)) {
            std::cerr << "Failed to render texture: " << SDL_GetError() << std::endl;
            return;
        }

        if (swap_count == 1) {
            std::cout << "[SwapBuffers] About to present" << std::endl;
        }

        // 显示
        SDL_RenderPresent(sdl_renderer_);

        if (swap_count == 1) {
            std::cout << "[SwapBuffers] Present completed" << std::endl;
        }
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
        // CPU 模式：重新创建 SDL Texture 和 Raster 表面
        if (sdl_texture_) {
            SDL_DestroyTexture(sdl_texture_);
        }

        sdl_texture_ = SDL_CreateTexture(
            sdl_renderer_,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );

        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface_ = SkSurfaces::Raster(info);
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
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL;

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
    int width = config_.width;
    int height = config_.height;

    // 创建 SDL Renderer
    sdl_renderer_ = SDL_CreateRenderer(sdl_window_, nullptr);
    if (!sdl_renderer_) {
        throw std::runtime_error(std::string("Failed to create SDL renderer: ") + SDL_GetError());
    }

    // 创建 Raster 表面（CPU 渲染）- 先创建 Skia surface 来确定像素格式
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

    // 创建 SDL Texture（用于显示 Skia 渲染结果）
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

    std::cout << "✅ 使用 CPU 软件渲染（类似 Chrome 的软件渲染模式）" << std::endl;
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
                int width = event.window.data1;
                int height = event.window.data2;
                OnResize();
                SetNeedsRepaint();  // 标记需要重新渲染
                DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, width, height));
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
                DispatchWindowEvent(WindowEvent(WindowEventType::EXPOSED));
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

    // 获取画布
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }

    // 调试：检查窗口和 surface 大小
    static bool size_checked = false;
    if (!size_checked) {
        int win_w, win_h;
        SDL_GetWindowSize(sdl_window_, &win_w, &win_h);

        int client_w, client_h;
        SDL_GetWindowSizeInPixels(sdl_window_, &client_w, &client_h);

        std::cout << "[RenderDocument] Window size: " << win_w << "x" << win_h << std::endl;
        std::cout << "[RenderDocument] Client size (pixels): " << client_w << "x" << client_h << std::endl;
        std::cout << "[RenderDocument] Surface size: " << surface_->width() << "x" << surface_->height() << std::endl;
        size_checked = true;
    }

    // 清空画布
    canvas->clear(SK_ColorWHITE);

    // TODO: 实现完整的渲染管线
    // 1. 样式解析
    // 2. 布局计算
    // 3. 渲染树构建
    // 4. 绘制

    // 简化实现：遍历 DOM 树并渲染
    auto body = document_->GetBody();
    if (body) {
        // 创建样式解析器
        StyleResolver style_resolver;

        // 递归构建渲染树
        std::function<std::shared_ptr<RenderObject>(std::shared_ptr<Node>, std::shared_ptr<RenderObject>)>
        build_render_tree = [&](std::shared_ptr<Node> node, std::shared_ptr<RenderObject> parent_render)
            -> std::shared_ptr<RenderObject> {

            if (!node) {
                return nullptr;
            }

            std::shared_ptr<RenderObject> render_object;

            // 根据节点类型创建渲染对象
            if (node->GetNodeType() == NodeType::TEXT_NODE) {
                // 文本节点
                auto text_node = std::static_pointer_cast<Text>(node);
                std::string text = text_node->GetData();

                std::cout << "[RenderDocument] Text node: '" << text << "'" << std::endl;

                // 跳过空白文本
                if (text.find_first_not_of(" \t\n\r") == std::string::npos) {
                    std::cout << "[RenderDocument] Skipping whitespace text" << std::endl;
                    return nullptr;
                }

                // 规范化空白字符：将连续的空白字符（包括换行）替换为单个空格
                std::string normalized_text;
                bool in_whitespace = false;
                for (char c : text) {
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                        if (!in_whitespace) {
                            normalized_text += ' ';
                            in_whitespace = true;
                        }
                    } else {
                        normalized_text += c;
                        in_whitespace = false;
                    }
                }

                auto render_text = std::make_shared<RenderText>(normalized_text);

                // 继承父元素样式
                if (parent_render) {
                    render_text->SetComputedStyle(parent_render->GetComputedStyle());
                }

                std::cout << "[RenderDocument] Created RenderText: '" << normalized_text << "'" << std::endl;
                render_object = render_text;
            }
            else if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                // 元素节点
                auto element = std::static_pointer_cast<Element>(node);

                std::cout << "[RenderDocument] Element: " << element->GetTagName() << std::endl;

                // 解析样式
                ComputedStyle style = style_resolver.ResolveStyle(element,
                    parent_render ? &(parent_render->GetComputedStyle()) : nullptr);

                // 根据 display 属性创建不同类型的渲染对象
                if (style.display == RenderObjectType::INLINE) {
                    render_object = std::make_shared<RenderInline>();
                    std::cout << "[RenderDocument] Created RenderInline" << std::endl;
                } else {
                    render_object = std::make_shared<RenderBlock>();
                    std::cout << "[RenderDocument] Created RenderBlock" << std::endl;
                }

                render_object->SetComputedStyle(style);

                // 关联DOM节点
                render_object->SetNode(element);

                // 递归处理子节点
                auto children = element->GetChildNodes();
                std::cout << "[RenderDocument] Element has " << children.size() << " children" << std::endl;
                for (auto& child : children) {
                    auto child_render = build_render_tree(child, render_object);
                    if (child_render) {
                        render_object->AppendChild(child_render);
                    }
                }
            }

            return render_object;
        };

        // 构建渲染树
        auto root_render = build_render_tree(body, nullptr);

        if (root_render) {
            std::cout << "[RenderDocument] Render tree built successfully" << std::endl;

            // 布局 - 使用客户区大小（与Skia surface一致）
            int width, height;
            SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);
            std::cout << "[RenderDocument] Layout: " << width << "x" << height << " (client area)" << std::endl;
            root_render->Layout(static_cast<float>(width), static_cast<float>(height));

            std::cout << "[RenderDocument] Starting paint..." << std::endl;
            // 绘制
            root_render->Paint(canvas);
            std::cout << "[RenderDocument] Paint completed" << std::endl;
        } else {
            std::cout << "[RenderDocument] Failed to build render tree!" << std::endl;
        }
    }

    // 刷新（Skia 使用 flush() 方法）
    if (gr_context_) {
        gr_context_->flush();
    }

    // 清除重绘标记
    needs_repaint_ = false;
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

} // namespace lightui


