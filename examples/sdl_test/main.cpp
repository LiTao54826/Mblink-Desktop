/**
 * @file main.cpp
 * @brief 纯 SDL 测试 - 用于隔离虚拟机闪烁问题
 */

#include <SDL3/SDL.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    std::cout << "=== Pure SDL Test ===" << std::endl;

    // 尝试各种 hint 来解决虚拟机问题
    SDL_SetHint(SDL_HINT_VIDEO_SYNC_WINDOW_OPERATIONS, "0");  // 禁用窗口操作同步
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");  // 启用 VSync
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");  // 允许屏保

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "SDL initialized" << std::endl;

    // 创建窗口（不使用 OpenGL）
    SDL_Window* window = SDL_CreateWindow(
        "Pure SDL Test",
        800, 600,
        0  // 不使用 SDL_WINDOW_RESIZABLE，测试固定大小窗口
    );
    
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    std::cout << "Window created" << std::endl;
    
    // 获取窗口 surface
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (!surface) {
        std::cerr << "SDL_GetWindowSurface failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // 填充红色
    SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), NULL, 255, 100, 100));
    SDL_UpdateWindowSurface(window);
    
    std::cout << "Surface filled with red" << std::endl;
    std::cout << std::endl;
    std::cout << "Move mouse over title bar and observe..." << std::endl;
    std::cout << "Press ESC or close window to exit" << std::endl;
    std::cout << std::endl;
    
    // 事件循环
    bool running = true;
    int frame_count = 0;
    Uint64 last_time = SDL_GetTicks();
    
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                    
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                    
                case SDL_EVENT_WINDOW_RESIZED:
                    std::cout << "[SDL] Window resized: " << event.window.data1 << "x" << event.window.data2 << std::endl;
                    // 重新获取 surface 并填充
                    surface = SDL_GetWindowSurface(window);
                    if (surface) {
                        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), NULL, 255, 100, 100));
                        SDL_UpdateWindowSurface(window);
                    }
                    break;
                    
                case SDL_EVENT_WINDOW_EXPOSED:
                    std::cout << "[SDL] Window exposed" << std::endl;
                    // 重绘
                    surface = SDL_GetWindowSurface(window);
                    if (surface) {
                        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), NULL, 255, 100, 100));
                        SDL_UpdateWindowSurface(window);
                    }
                    break;
                    
                case SDL_EVENT_WINDOW_MOVED:
                    std::cout << "[SDL] Window moved" << std::endl;
                    break;
            }
        }
        
        frame_count++;
        Uint64 now = SDL_GetTicks();
        if (now - last_time >= 1000) {
            std::cout << "[Stats] Frames: " << frame_count << "/s" << std::endl;
            frame_count = 0;
            last_time = now;
        }
        
        // 限制帧率
        SDL_Delay(16);
    }
    
    std::cout << "Exiting..." << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

