/**
 * @file window.cpp
 * @brief 窗口管理模块实现
 * 
 * 实现内容：
 * - SDL3窗口创建和管理
 * - OpenGL上下文初始化
 * - Skia渲染表面创建
 * - 窗口事件处理
 * 
 * TODO:
 * - [ ] 实现SDL初始化
 * - [ ] 实现窗口创建
 * - [ ] 实现OpenGL上下文创建
 * - [ ] 实现Skia集成
 * - [ ] 实现窗口大小调整
 * - [ ] 添加Metal/Vulkan支持（macOS/Linux）
 */

#include "window.h"
#include <stdexcept>
#include "include/gpu/ganesh/gl/GrGLInterface.h"

namespace lightui {

Window::Window(const WindowConfig& config) : config_(config) {
    // TODO: 实现构造函数
    // 1. 初始化SDL
    // 2. 创建SDL窗口
    // 3. 创建OpenGL上下文
    // 4. 初始化Skia
    // 5. 创建渲染表面
}

Window::~Window() {
    // TODO: 实现析构函数
    // 1. 释放Skia资源
    // 2. 销毁OpenGL上下文
    // 3. 销毁SDL窗口
    // 4. 清理SDL
}

void Window::Show() {
    // TODO: 显示窗口
    // SDL_ShowWindow(sdl_window_);
}

void Window::Hide() {
    // TODO: 隐藏窗口
    // SDL_HideWindow(sdl_window_);
}

void Window::SetTitle(const std::string& title) {
    // TODO: 设置窗口标题
    // config_.title = title;
    // SDL_SetWindowTitle(sdl_window_, title.c_str());
}

void Window::SetSize(int width, int height) {
    // TODO: 设置窗口大小
    // config_.width = width;
    // config_.height = height;
    // SDL_SetWindowSize(sdl_window_, width, height);
    // OnResize();
}

void Window::GetSize(int* width, int* height) const {
    // TODO: 获取窗口大小
    // SDL_GetWindowSize(sdl_window_, width, height);
}

SkCanvas* Window::GetCanvas() const {
    // TODO: 返回Skia画布
    // return surface_ ? surface_->getCanvas() : nullptr;
    return nullptr;
}

void Window::SwapBuffers() {
    // TODO: 交换缓冲区
    // 1. 刷新Skia命令
    // 2. 交换OpenGL缓冲区
    // gr_context_->flush();
    // SDL_GL_SwapWindow(sdl_window_);
}

void Window::OnResize() {
    // TODO: 处理窗口大小调整
    // 1. 重新创建Skia渲染表面
    // 2. 更新视口
}

void Window::InitSDL() {
    // TODO: 初始化SDL
    // if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    //     throw std::runtime_error("Failed to initialize SDL");
    // }
}

void Window::CreateSDLWindow() {
    // TODO: 创建SDL窗口
    // Uint32 flags = SDL_WINDOW_OPENGL;
    // if (config_.resizable) flags |= SDL_WINDOW_RESIZABLE;
    // if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    // if (config_.borderless) flags |= SDL_WINDOW_BORDERLESS;
    //
    // sdl_window_ = SDL_CreateWindow(
    //     config_.title.c_str(),
    //     config_.width, config_.height,
    //     flags
    // );
}

void Window::InitOpenGL() {
    // TODO: 初始化OpenGL上下文
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    //
    // gl_context_ = SDL_GL_CreateContext(sdl_window_);
    // SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    //
    // if (config_.vsync) {
    //     SDL_GL_SetSwapInterval(1);
    // }
}

void Window::InitSkia() {
    // TODO: 初始化Skia
    // auto interface = GrGLMakeNativeInterface();
    // gr_context_ = GrDirectContext::MakeGL(interface);
}

void Window::CreateSkiaSurface() {
    // TODO: 创建Skia渲染表面
    // GrGLFramebufferInfo framebuffer_info;
    // framebuffer_info.fFBOID = 0;
    // framebuffer_info.fFormat = GL_RGBA8;
    //
    // GrBackendRenderTarget backend_render_target(
    //     config_.width, config_.height,
    //     0, 8,
    //     framebuffer_info
    // );
    //
    // surface_ = SkSurface::MakeFromBackendRenderTarget(
    //     gr_context_.get(),
    //     backend_render_target,
    //     kBottomLeft_GrSurfaceOrigin,
    //     kRGBA_8888_SkColorType,
    //     nullptr,
    //     nullptr
    // );
}

} // namespace lightui

