/**
 * @file display_backend.cpp
 * @brief 显示后端实现
 */

#include "display_backend.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <cstring>
#include <algorithm>  // for std::min

#ifdef _WIN32
// 避免 Windows.h 中的 min/max 宏与 std::min/std::max 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// OpenGL 头文件
#ifdef _WIN32
#include <GL/gl.h>
// Windows GL.h 不包含 GL_CLAMP_TO_EDGE，需要手动定义
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace lightui {

// ============================================================================
// DisplayBackend 静态方法
// ============================================================================

std::unique_ptr<DisplayBackend> DisplayBackend::Create(DisplayBackendType type) {
    switch (type) {
        case DisplayBackendType::OPENGL:
            return std::make_unique<OpenGLDisplayBackend>();
#ifdef _WIN32
        case DisplayBackendType::LAYERED_WINDOW:
            return std::make_unique<LayeredWindowDisplayBackend>();
        case DisplayBackendType::GDI:
            return std::make_unique<GDIDisplayBackend>();
#endif
        case DisplayBackendType::SDL_SURFACE:
            return std::make_unique<SDLSurfaceDisplayBackend>();
        case DisplayBackendType::AUTO:
        default:
            return nullptr;  // 使用 CreateBest
    }
}

std::unique_ptr<DisplayBackend> DisplayBackend::CreateBest(SDL_Window* window, int width, int height) {
    // 按优先级尝试各后端

    // 1. 优先使用 OpenGL - 利用 VSync 避免闪烁
    if (IsOpenGLAvailable()) {
        auto backend = std::make_unique<OpenGLDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            std::cout << "[DisplayBackend] Using OpenGL backend" << std::endl;
            return backend;
        }
    }

#ifdef _WIN32
    // 2. GDI 作为回退
    {
        auto backend = std::make_unique<GDIDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            std::cout << "[DisplayBackend] Using GDI backend" << std::endl;
            return backend;
        }
    }
#endif

    // 3. 尝试 OpenGL (暂时放到后面，因为可能有问题)
    if (IsOpenGLAvailable()) {
        auto backend = std::make_unique<OpenGLDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            std::cout << "[DisplayBackend] Using OpenGL backend" << std::endl;
            return backend;
        }
    }

    // 4. SDL Surface 回退
    {
        auto backend = std::make_unique<SDLSurfaceDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            std::cout << "[DisplayBackend] Using SDL_Surface backend (fallback)" << std::endl;
            return backend;
        }
    }

    std::cerr << "[DisplayBackend] Failed to create any display backend!" << std::endl;
    return nullptr;
}

bool DisplayBackend::IsOpenGLAvailable() {
    // 简单检测：尝试创建临时 OpenGL 上下文
    // 这里先返回 true，实际检测在 Initialize 中进行
    return true;
}

// ============================================================================
// OpenGLDisplayBackend 实现
// ============================================================================

OpenGLDisplayBackend::OpenGLDisplayBackend() = default;

OpenGLDisplayBackend::~OpenGLDisplayBackend() {
    Shutdown();
}

bool OpenGLDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 设置 OpenGL 属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // 创建 OpenGL 上下文
    gl_context_ = SDL_GL_CreateContext(window);
    if (!gl_context_) {
        std::cerr << "[OpenGLDisplayBackend] Failed to create GL context: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, (SDL_GLContext)gl_context_);

    // 启用 VSync
    SetVSync(true);

    // 创建纹理
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 预分配纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);

    // 设置正交投影
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    std::cout << "[OpenGLDisplayBackend] Initialized successfully (" << width << "x" << height << ")" << std::endl;
    return true;
}

void OpenGLDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!gl_context_ || !texture_id_) return;

    SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);

    // 设置视口
    glViewport(0, 0, width, height);

    // 更新纹理
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // 如果尺寸变化，重新分配纹理
    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

        // 更新投影
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        // 使用 glTexSubImage2D 更高效
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    }

    // 清屏
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 确保纹理已启用
    glEnable(GL_TEXTURE_2D);

    // 绘制全屏四边形
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f((float)width, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f((float)width, (float)height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)height);
    glEnd();

    // 交换缓冲区
    SDL_GL_SwapWindow(window_);
}

void OpenGLDisplayBackend::OnResize(int width, int height) {
    if (!gl_context_) return;

    SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    width_ = width;
    height_ = height;
}

void OpenGLDisplayBackend::Shutdown() {
    if (gl_context_) {
        SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);
        
        if (texture_id_) {
            glDeleteTextures(1, &texture_id_);
            texture_id_ = 0;
        }

        SDL_GL_DestroyContext((SDL_GLContext)gl_context_);
        gl_context_ = nullptr;
    }
}

bool OpenGLDisplayBackend::SetVSync(bool enabled) {
    vsync_enabled_ = enabled;
    int result = SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    return result == 0;
}

// ============================================================================
// SDLSurfaceDisplayBackend 实现
// ============================================================================

SDLSurfaceDisplayBackend::SDLSurfaceDisplayBackend() = default;

SDLSurfaceDisplayBackend::~SDLSurfaceDisplayBackend() {
    Shutdown();
}

bool SDLSurfaceDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    sdl_surface_ = SDL_GetWindowSurface(window);
    if (!sdl_surface_) {
        std::cerr << "[SDLSurfaceDisplayBackend] Failed to get window surface: " << SDL_GetError() << std::endl;
        return false;
    }

    std::cout << "[SDLSurfaceDisplayBackend] Initialized successfully" << std::endl;
    return true;
}

void SDLSurfaceDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    SDL_Surface* surface = (SDL_Surface*)sdl_surface_;
    if (!surface) {
        sdl_surface_ = SDL_GetWindowSurface(window_);
        surface = (SDL_Surface*)sdl_surface_;
        if (!surface) return;
    }

    // 锁定表面
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    // 复制像素数据
    int copy_height = std::min(height, surface->h);
    int copy_width = std::min(width, surface->w);
    int src_stride = stride;
    int dst_stride = surface->pitch;

    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)surface->pixels;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * dst_stride, src + y * src_stride, copy_width * 4);
    }

    // 解锁表面
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    // 更新窗口
    SDL_UpdateWindowSurface(window_);
}

void SDLSurfaceDisplayBackend::OnResize(int width, int height) {
    width_ = width;
    height_ = height;
    // SDL Surface 会在窗口大小变化时自动更新
    sdl_surface_ = SDL_GetWindowSurface(window_);
}

void SDLSurfaceDisplayBackend::Shutdown() {
    // SDL_Surface 由 SDL 管理，不需要手动释放
    sdl_surface_ = nullptr;
}

// ============================================================================
// Windows 特定后端实现
// ============================================================================

#ifdef _WIN32

// ----------------------------------------------------------------------------
// LayeredWindowDisplayBackend 实现
// ----------------------------------------------------------------------------

LayeredWindowDisplayBackend::LayeredWindowDisplayBackend() = default;

LayeredWindowDisplayBackend::~LayeredWindowDisplayBackend() {
    Shutdown();
}

bool LayeredWindowDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Win32 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                           SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd_) {
        std::cerr << "[LayeredWindowDisplayBackend] Failed to get HWND" << std::endl;
        return false;
    }

    HWND hWnd = (HWND)hwnd_;

    // 设置为分层窗口
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

    // 创建兼容 DC 和位图
    HDC hdcScreen = GetDC(NULL);
    hdc_mem_ = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);

    if (!hdc_mem_) {
        std::cerr << "[LayeredWindowDisplayBackend] Failed to create memory DC" << std::endl;
        return false;
    }

    // 创建 DIB Section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (!hbitmap_) {
        std::cerr << "[LayeredWindowDisplayBackend] Failed to create DIB section" << std::endl;
        Shutdown();
        return false;
    }

    hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
    bitmap_width_ = width;
    bitmap_height_ = height;

    std::cout << "[LayeredWindowDisplayBackend] Initialized successfully (" << width << "x" << height << ")" << std::endl;
    return true;
}

void LayeredWindowDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!hwnd_ || !hdc_mem_ || !bitmap_bits_) return;

    // 检查是否需要重建位图
    if (width != bitmap_width_ || height != bitmap_height_) {
        OnResize(width, height);
    }

    // 复制像素数据到 DIB
    int copy_height = std::min(height, bitmap_height_);
    int copy_width = std::min(width, bitmap_width_);
    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)bitmap_bits_;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * bitmap_width_ * 4, src + y * stride, copy_width * 4);
    }

    // 使用 UpdateLayeredWindow
    HWND hWnd = (HWND)hwnd_;
    HDC hdcScreen = GetDC(NULL);

    POINT ptDst = {0, 0};
    RECT rc;
    GetWindowRect(hWnd, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    SIZE size = {bitmap_width_, bitmap_height_};
    POINT ptSrc = {0, 0};

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;  // 使用源的 alpha

    // 注意：对于不透明窗口，使用 ULW_OPAQUE 会更快
    // 但 Skia 输出是预乘 alpha，我们使用 AC_SRC_ALPHA
    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &size, (HDC)hdc_mem_, &ptSrc, 0, &blend, ULW_ALPHA);

    ReleaseDC(NULL, hdcScreen);
}

void LayeredWindowDisplayBackend::OnResize(int width, int height) {
    if (width == bitmap_width_ && height == bitmap_height_) return;

    // 释放旧位图
    if (hdc_mem_ && hbitmap_old_) {
        SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
        hbitmap_old_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
        bitmap_bits_ = nullptr;
    }

    // 创建新位图
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (hbitmap_) {
        hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
        bitmap_width_ = width;
        bitmap_height_ = height;
    }

    width_ = width;
    height_ = height;
}

void LayeredWindowDisplayBackend::Shutdown() {
    if (hdc_mem_) {
        if (hbitmap_old_) {
            SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
            hbitmap_old_ = nullptr;
        }
        DeleteDC((HDC)hdc_mem_);
        hdc_mem_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
    }
    bitmap_bits_ = nullptr;

    // 移除分层窗口样式
    if (hwnd_) {
        HWND hWnd = (HWND)hwnd_;
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        SetWindowLong(hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        hwnd_ = nullptr;
    }
}

// ----------------------------------------------------------------------------
// GDIDisplayBackend 实现
// ----------------------------------------------------------------------------

GDIDisplayBackend::GDIDisplayBackend() = default;

GDIDisplayBackend::~GDIDisplayBackend() {
    Shutdown();
}

bool GDIDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Win32 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                           SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd_) {
        std::cerr << "[GDIDisplayBackend] Failed to get HWND" << std::endl;
        return false;
    }

    // 创建兼容 DC
    HDC hdcWindow = GetDC((HWND)hwnd_);
    hdc_mem_ = CreateCompatibleDC(hdcWindow);
    ReleaseDC((HWND)hwnd_, hdcWindow);

    if (!hdc_mem_) {
        std::cerr << "[GDIDisplayBackend] Failed to create memory DC" << std::endl;
        return false;
    }

    // 创建 DIB Section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (!hbitmap_) {
        std::cerr << "[GDIDisplayBackend] Failed to create DIB section" << std::endl;
        Shutdown();
        return false;
    }

    hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
    bitmap_width_ = width;
    bitmap_height_ = height;

    std::cout << "[GDIDisplayBackend] Initialized successfully (" << width << "x" << height << ")" << std::endl;
    return true;
}

void GDIDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!hwnd_ || !hdc_mem_ || !bitmap_bits_) return;

    // 检查是否需要重建位图
    if (width != bitmap_width_ || height != bitmap_height_) {
        OnResize(width, height);
    }

    // 复制像素数据到 DIB
    int copy_height = std::min(height, bitmap_height_);
    int copy_width = std::min(width, bitmap_width_);
    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)bitmap_bits_;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * bitmap_width_ * 4, src + y * stride, copy_width * 4);
    }

    // 使用 BitBlt 复制到窗口
    HDC hdcWindow = GetDC((HWND)hwnd_);
    if (hdcWindow) {
        BitBlt(hdcWindow, 0, 0, bitmap_width_, bitmap_height_, (HDC)hdc_mem_, 0, 0, SRCCOPY);
        ReleaseDC((HWND)hwnd_, hdcWindow);
    }

    // 验证整个客户区，阻止 WM_PAINT 消息
    ValidateRect((HWND)hwnd_, NULL);
}

void GDIDisplayBackend::OnResize(int width, int height) {
    if (width == bitmap_width_ && height == bitmap_height_) return;

    // 释放旧位图
    if (hdc_mem_ && hbitmap_old_) {
        SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
        hbitmap_old_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
        bitmap_bits_ = nullptr;
    }

    // 创建新位图
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (hbitmap_) {
        hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
        bitmap_width_ = width;
        bitmap_height_ = height;
    }

    width_ = width;
    height_ = height;
}

void GDIDisplayBackend::Shutdown() {
    if (hdc_mem_) {
        if (hbitmap_old_) {
            SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
            hbitmap_old_ = nullptr;
        }
        DeleteDC((HDC)hdc_mem_);
        hdc_mem_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
    }
    bitmap_bits_ = nullptr;
    hwnd_ = nullptr;
}

#endif // _WIN32

} // namespace lightui

