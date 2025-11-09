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
#include <SDL3/SDL.h>
#include "include/core/SkSurface.h"
#include "include/gpu/GrDirectContext.h"

namespace lightui {

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
    bool vsync = true;
    int fps_limit = 60;
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

private:
    WindowConfig config_;
    SDL_Window* sdl_window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    sk_sp<GrDirectContext> gr_context_;
    sk_sp<SkSurface> surface_;
    bool should_close_ = false;
};

} // namespace lightui

