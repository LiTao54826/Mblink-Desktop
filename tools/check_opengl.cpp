/**
 * @file check_opengl.cpp
 * @brief OpenGL 环境检测工具
 * 
 * 用于检测系统是否支持 OpenGL，以及 SDL3 是否能创建窗口
 */

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

int main(int argc, char* argv[]) {
    std::cout << "=== OpenGL 环境检测工具 ===" << std::endl;
    std::cout << std::endl;
    
    // 1. 检查 SDL 初始化
    std::cout << "[1/5] 初始化 SDL..." << std::endl;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "❌ SDL 初始化失败: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "✅ SDL 初始化成功" << std::endl;
    std::cout << std::endl;
    
    // 2. 检查视频驱动
    std::cout << "[2/5] 检查视频驱动..." << std::endl;
    const char* video_driver = SDL_GetCurrentVideoDriver();
    if (video_driver) {
        std::cout << "✅ 当前视频驱动: " << video_driver << std::endl;
    } else {
        std::cerr << "❌ 无法获取视频驱动" << std::endl;
    }
    std::cout << std::endl;
    
    // 3. 检查显示器
    std::cout << "[3/5] 检查显示器..." << std::endl;
    SDL_DisplayID* displays = SDL_GetDisplays(nullptr);
    if (displays) {
        int display_count = 0;
        while (displays[display_count] != 0) {
            display_count++;
        }
        std::cout << "✅ 检测到 " << display_count << " 个显示器" << std::endl;
        
        for (int i = 0; i < display_count; i++) {
            const char* name = SDL_GetDisplayName(displays[i]);
            std::cout << "   显示器 " << i << ": " << (name ? name : "未知") << std::endl;
        }
        SDL_free(displays);
    } else {
        std::cerr << "⚠️  无法检测显示器（可能在无头环境中）" << std::endl;
    }
    std::cout << std::endl;
    
    // 4. 尝试创建窗口
    std::cout << "[4/5] 尝试创建窗口..." << std::endl;
    
    // 设置 OpenGL 属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    SDL_Window* window = SDL_CreateWindow(
        "OpenGL Test",
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );
    
    if (!window) {
        std::cerr << "❌ 创建窗口失败: " << SDL_GetError() << std::endl;
        std::cerr << std::endl;
        std::cerr << "可能的原因：" << std::endl;
        std::cerr << "1. 虚拟机未启用 3D 加速" << std::endl;
        std::cerr << "2. 缺少 OpenGL 驱动" << std::endl;
        std::cerr << "3. 在无头环境中运行" << std::endl;
        std::cerr << std::endl;
        std::cerr << "解决方案：" << std::endl;
        std::cerr << "- VMware: 启用 3D 图形加速" << std::endl;
        std::cerr << "- VirtualBox: 启用 3D 加速" << std::endl;
        std::cerr << "- 使用远程桌面连接" << std::endl;
        SDL_Quit();
        return 1;
    }
    std::cout << "✅ 窗口创建成功" << std::endl;
    std::cout << std::endl;
    
    // 5. 尝试创建 OpenGL 上下文
    std::cout << "[5/5] 尝试创建 OpenGL 上下文..." << std::endl;
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    
    if (!gl_context) {
        std::cerr << "❌ OpenGL 上下文创建失败: " << SDL_GetError() << std::endl;
        std::cerr << std::endl;
        std::cerr << "这意味着系统不支持 OpenGL 3.3" << std::endl;
        std::cerr << "请检查显卡驱动或启用虚拟机 3D 加速" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::cout << "✅ OpenGL 上下文创建成功" << std::endl;
    std::cout << std::endl;
    
    // 激活上下文并获取信息
    if (SDL_GL_MakeCurrent(window, gl_context)) {
        std::cout << "=== OpenGL 信息 ===" << std::endl;
        
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);
        const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        if (vendor) std::cout << "供应商: " << vendor << std::endl;
        if (renderer) std::cout << "渲染器: " << renderer << std::endl;
        if (version) std::cout << "OpenGL 版本: " << version << std::endl;
        if (glsl_version) std::cout << "GLSL 版本: " << glsl_version << std::endl;
        std::cout << std::endl;
    }
    
    // 清理
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "=== 检测完成 ===" << std::endl;
    std::cout << "✅ 你的环境支持 OpenGL！" << std::endl;
    std::cout << "✅ MBlink 应用可以在此环境中运行！" << std::endl;
    
    return 0;
}

