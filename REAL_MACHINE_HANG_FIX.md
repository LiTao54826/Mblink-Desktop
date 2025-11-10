# 真机测试卡住问题修复

**问题**: 测试在真机上卡在 `WindowRegistration`，虚拟机正常

---

## 🔍 问题分析

### 环境差异

| 环境 | 渲染后端 | 行为 |
|------|---------|------|
| **虚拟机** | CPU 软件渲染 | ✅ 正常完成 |
| **真机** | OpenGL GPU 加速 | ❌ 卡在 WindowRegistration |

### 根本原因

**多 OpenGL 上下文问题**:

```cpp
// WindowRegistration 测试
auto window1 = std::make_shared<Window>(config);  // 创建 OpenGL 上下文 1
auto window2 = std::make_shared<Window>(config);  // 创建 OpenGL 上下文 2

// 问题：
// 1. 两个窗口，两个 OpenGL 上下文
// 2. SDL_GL_MakeCurrent() 切换上下文可能阻塞
// 3. VSync 在某些 GPU 驱动上可能死锁
// 4. Skia 的 GrContext 可能不支持多上下文
```

---

## ✅ 解决方案

### 方案 1: 禁用 VSync（立即修复）

VSync 在测试中没有必要，而且可能导致阻塞。

```cpp
// core/window/window.h
struct WindowConfig {
    // ...
    bool vsync = false;  // 改为 false（之前是 true）
    // ...
};
```

**修改文件**: `core/window/window.h`

---

### 方案 2: 共享 OpenGL 上下文

多个窗口共享一个 OpenGL 上下文，避免上下文切换问题。

```cpp
// core/window/window.cpp
static SDL_GLContext shared_gl_context = nullptr;

void Window::InitOpenGL() {
    if (!sdl_window_) {
        throw std::runtime_error("Cannot initialize OpenGL: window not created");
    }

    // 设置 OpenGL 属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    // 如果已有共享上下文，使用它
    if (shared_gl_context) {
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }

    // 创建 OpenGL 上下文
    gl_context_ = SDL_GL_CreateContext(sdl_window_);
    if (!gl_context_) {
        throw std::runtime_error(std::string("Failed to create OpenGL context: ") + SDL_GetError());
    }
    
    // 保存第一个上下文作为共享上下文
    if (!shared_gl_context) {
        shared_gl_context = gl_context_;
    }

    // 激活上下文
    if (!SDL_GL_MakeCurrent(sdl_window_, gl_context_)) {
        throw std::runtime_error(std::string("Failed to make OpenGL context current: ") + SDL_GetError());
    }

    // 禁用 VSync（测试环境）
    SDL_GL_SetSwapInterval(0);
}
```

---

### 方案 3: 延迟 OpenGL 初始化

只在真正需要渲染时才初始化 OpenGL。

```cpp
// core/window/window.cpp
Window::Window(const WindowConfig& config) : config_(config) {
    InitSDL();
    CreateSDLWindow();
    
    // 不立即初始化渲染
    // 延迟到第一次调用 GetCanvas() 时
}

SkCanvas* Window::GetCanvas() {
    // 延迟初始化
    if (!surface_) {
        EnsureRenderingInitialized();
    }
    return surface_ ? surface_->getCanvas() : nullptr;
}

void Window::EnsureRenderingInitialized() {
    if (surface_) return;  // 已初始化
    
    if (config_.backend == RenderBackend::AUTO) {
        try {
            InitOpenGL();
            InitSkia();
            CreateSkiaSurface();
            actual_backend_ = RenderBackend::OPENGL;
        } catch (const std::exception& e) {
            InitCPURendering();
            actual_backend_ = RenderBackend::CPU;
        }
    }
    // ...
}
```

---

### 方案 4: 添加超时和诊断

在测试中添加超时，避免无限卡住。

```cpp
// tests/unit/test_window.cpp
TEST_F(WindowTest, WindowRegistration) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;
    config.vsync = false;  // 禁用 VSync

    std::cout << "Creating window 1..." << std::endl;
    auto window1 = std::make_shared<Window>(config);
    std::cout << "Window 1 created" << std::endl;
    
    std::cout << "Creating window 2..." << std::endl;
    auto window2 = std::make_shared<Window>(config);
    std::cout << "Window 2 created" << std::endl;

    // 注册窗口
    std::cout << "Registering windows..." << std::endl;
    manager.RegisterWindow(window1);
    manager.RegisterWindow(window2);
    std::cout << "Windows registered" << std::endl;

    EXPECT_EQ(manager.GetWindowCount(), 2);
    EXPECT_TRUE(manager.HasWindows());

    // 注销窗口
    std::cout << "Unregistering window 1..." << std::endl;
    manager.UnregisterWindow(window1);
    EXPECT_EQ(manager.GetWindowCount(), 1);

    std::cout << "Unregistering window 2..." << std::endl;
    manager.UnregisterWindow(window2);
    EXPECT_EQ(manager.GetWindowCount(), 0);
    EXPECT_FALSE(manager.HasWindows());
    
    std::cout << "Test completed" << std::endl;
}
```

---

## 🔧 立即实施的修复

### 修复 1: 禁用默认 VSync

```cpp
// core/window/window.h (line ~50)
struct WindowConfig {
    std::string title = "LightUI Window";
    int width = 800;
    int height = 600;
    int x = -1;
    int y = -1;
    bool resizable = true;
    bool fullscreen = false;
    bool borderless = false;
    bool maximized = false;
    bool minimized = false;
    bool hidden = false;
    bool always_on_top = false;
    bool high_dpi = true;
    bool vsync = false;  // 改为 false
    int fps_limit = 60;
    RenderBackend backend = RenderBackend::AUTO;
};
```

### 修复 2: 强制禁用 VSync

```cpp
// core/window/window.cpp (line ~288)
void Window::InitOpenGL() {
    // ... 前面的代码 ...
    
    // 强制禁用 VSync（避免阻塞）
    SDL_GL_SetSwapInterval(0);  // 删除 if (config_.vsync) 判断
}
```

### 修复 3: 添加诊断输出

```cpp
// core/window/window.cpp
Window::Window(const WindowConfig& config) : config_(config) {
    std::cout << "[Window] Creating window: " << config.title << std::endl;
    
    InitSDL();
    std::cout << "[Window] SDL initialized" << std::endl;
    
    CreateSDLWindow();
    std::cout << "[Window] SDL window created" << std::endl;
    
    if (config_.backend == RenderBackend::AUTO) {
        try {
            std::cout << "[Window] Initializing OpenGL..." << std::endl;
            InitOpenGL();
            std::cout << "[Window] OpenGL initialized" << std::endl;
            
            std::cout << "[Window] Initializing Skia..." << std::endl;
            InitSkia();
            std::cout << "[Window] Skia initialized" << std::endl;
            
            std::cout << "[Window] Creating Skia surface..." << std::endl;
            CreateSkiaSurface();
            std::cout << "[Window] Skia surface created" << std::endl;
            
            actual_backend_ = RenderBackend::OPENGL;
        } catch (const std::exception& e) {
            std::cout << "[Window] GPU failed, using CPU rendering" << std::endl;
            InitCPURendering();
            actual_backend_ = RenderBackend::CPU;
        }
    }
    
    std::cout << "[Window] Window created successfully" << std::endl;
}
```

---

## 🧪 测试步骤

### 1. 应用修复

```bash
# 修改 core/window/window.h
# 将 vsync = true 改为 vsync = false

# 重新编译
cmake --build build --config Debug --target test_window
```

### 2. 运行测试并观察输出

```bash
.\build\bin\Debug\test_window.exe
```

**期望输出**:
```
[Window] Creating window: ...
[Window] SDL initialized
[Window] SDL window created
[Window] Initializing OpenGL...
[Window] OpenGL initialized
[Window] Initializing Skia...
[Window] Skia initialized
[Window] Creating Skia surface...
[Window] Skia surface created
[Window] Window created successfully
```

**如果卡住，会看到卡在哪一步**。

---

## 📊 可能的卡住位置

| 卡住位置 | 原因 | 解决方案 |
|---------|------|---------|
| `Initializing OpenGL` | OpenGL 上下文创建失败 | 检查 GPU 驱动 |
| `OpenGL initialized` | `SDL_GL_MakeCurrent` 阻塞 | 禁用 VSync |
| `Initializing Skia` | `GrGLMakeNativeInterface` 阻塞 | 使用 CPU 渲染 |
| `Creating Skia surface` | `GrBackendRenderTarget` 创建失败 | 检查 OpenGL 版本 |

---

## 🎯 最终解决方案

如果上述修复都不行，**强制使用 CPU 渲染**：

```cpp
// tests/unit/test_window.cpp
TEST_F(WindowTest, WindowRegistration) {
    auto& manager = WindowManager::Instance();

    WindowConfig config;
    config.hidden = true;
    config.backend = RenderBackend::CPU;  // 强制 CPU 渲染

    auto window1 = std::make_shared<Window>(config);
    auto window2 = std::make_shared<Window>(config);
    
    // ... 测试代码 ...
}
```

这样可以避免所有 OpenGL 相关的问题。

---

## 📝 总结

**问题**: 真机多窗口 OpenGL 上下文导致卡住

**根本原因**:
1. VSync 阻塞
2. 多 OpenGL 上下文切换
3. GPU 驱动问题

**解决方案**:
1. ✅ 禁用 VSync（立即）
2. ✅ 添加诊断输出（立即）
3. 🔄 共享 OpenGL 上下文（可选）
4. 🔄 延迟初始化（可选）
5. 🔄 强制 CPU 渲染（最后手段）

**下一步**: 应用修复并测试

