# Window 模块

## 📋 概述

Window 模块是 MBink 的窗口管理核心，基于 SDL3 实现跨平台的窗口创建、事件处理和渲染上下文管理。它提供了完整的窗口生命周期管理、多窗口支持和窗口属性控制。

## 🎯 主要功能

- **窗口创建**: 创建和销毁应用程序窗口
- **窗口属性**: 标题、尺寸、位置、可见性等
- **窗口状态**: 最大化、最小化、全屏等
- **渲染上下文**: Skia 渲染上下文管理
- **多窗口支持**: 同时管理多个窗口
- **事件处理**: 窗口事件（关闭、调整大小等）
- **DPI 感知**: 高 DPI 显示器支持

## 📁 文件结构

```
window/
├── CMakeLists.txt          # 构建配置
├── window.h/cpp            # 窗口类
└── window_manager.h/cpp    # 窗口管理器
```

## 🔌 核心类

### Window (窗口类)

```cpp
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();
    
    // 窗口控制
    void Show();
    void Hide();
    void Close();
    void Focus();
    
    // 窗口属性
    void SetTitle(const std::string& title);
    std::string GetTitle() const;
    
    void SetSize(int width, int height);
    void GetSize(int* width, int* height) const;
    int GetWidth() const;
    int GetHeight() const;
    
    void SetPosition(int x, int y);
    void GetPosition(int* x, int* y) const;
    
    // 窗口状态
    void Maximize();
    void Minimize();
    void Restore();
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const;
    bool IsMaximized() const;
    bool IsMinimized() const;
    bool IsVisible() const;
    
    // 渲染
    void BeginFrame();
    void EndFrame();
    SkCanvas* GetCanvas();
    
    // HTML/JavaScript
    void LoadHTML(const std::string& html);
    void LoadURL(const std::string& url);
    void ExecuteScript(const std::string& script);
    
    // 文档和运行时
    std::shared_ptr<Document> GetDocument();
    std::shared_ptr<QuickJSRuntime> GetRuntime();
    
    // SDL 窗口
    SDL_Window* GetSDLWindow();
    uint32_t GetWindowID() const;
};
```

### WindowManager (窗口管理器)

```cpp
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    // 窗口创建和销毁
    std::shared_ptr<Window> CreateWindow(int width, int height,
                                        const std::string& title);
    void DestroyWindow(std::shared_ptr<Window> window);
    void DestroyAllWindows();
    
    // 窗口查询
    std::shared_ptr<Window> GetWindow(uint32_t window_id);
    std::vector<std::shared_ptr<Window>> GetAllWindows();
    int GetWindowCount() const;
    
    // 事件处理
    void HandleEvent(const SDL_Event& event);
    
    // 渲染所有窗口
    void RenderAll();
};
```

## 💡 使用示例

### 创建窗口

```cpp
auto window_manager = std::make_shared<WindowManager>();

// 创建窗口
auto window = window_manager->CreateWindow(800, 600, "MBink App");

// 显示窗口
window->Show();
```

### 设置窗口属性

```cpp
// 设置标题
window->SetTitle("My Application");

// 设置尺寸
window->SetSize(1024, 768);

// 设置位置
window->SetPosition(100, 100);

// 全屏
window->SetFullscreen(true);

// 最大化
window->Maximize();
```

### 加载 HTML

```cpp
std::string html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Hello MBink</title>
        <style>
            body {
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
                margin: 0;
                font-family: Arial, sans-serif;
            }
            h1 {
                color: #333;
            }
        </style>
    </head>
    <body>
        <h1>Hello, MBink!</h1>
    </body>
    </html>
)";

window->LoadHTML(html);
```

### 执行 JavaScript

```cpp
// 执行脚本
window->ExecuteScript(R"(
    console.log('Hello from JavaScript!');
    
    const h1 = document.querySelector('h1');
    h1.textContent = 'Updated by JavaScript';
    h1.style.color = 'blue';
)");
```

### 多窗口管理

```cpp
auto window_manager = std::make_shared<WindowManager>();

// 创建多个窗口
auto window1 = window_manager->CreateWindow(800, 600, "Window 1");
auto window2 = window_manager->CreateWindow(640, 480, "Window 2");

window1->Show();
window2->Show();

// 获取所有窗口
auto windows = window_manager->GetAllWindows();
std::cout << "Total windows: " << windows.size() << std::endl;

// 销毁特定窗口
window_manager->DestroyWindow(window1);
```

### 自定义渲染

```cpp
window->BeginFrame();

auto canvas = window->GetCanvas();

// 使用 Skia 绘制
SkPaint paint;
paint.setColor(SK_ColorRED);
canvas->drawCircle(400, 300, 50, paint);

window->EndFrame();
```

## 🔗 依赖关系

### 依赖的模块

- `third_party/SDL3` - SDL3 窗口系统
- `third_party/skia` - Skia 渲染引擎
- `core/dom` - DOM 文档
- `core/quickjs` - JavaScript 运行时
- `core/render` - 渲染引擎

### 被依赖的模块

- `core/api` - C API 层
- `core/event` - 事件循环

## 🏗️ 架构说明

Window 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  Application / C API                    │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Window Module (core/window) ← 当前模块  │
│  Window + WindowManager                 │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  SDL3 (Window System)                   │
│  Skia (Rendering)                       │
└─────────────────────────────────────────┘
```

## 📊 窗口生命周期

```
创建 (CreateWindow)
    ↓
初始化 (SDL + Skia + DOM + QuickJS)
    ↓
显示 (Show)
    ↓
事件循环 (HandleEvent + Render)
    ↓
关闭 (Close)
    ↓
销毁 (DestroyWindow)
```

## 🎨 渲染流程

```
BeginFrame()
    ↓
清空画布
    ↓
布局计算 (LayoutEngine)
    ↓
DOM 渲染 (RenderEngine)
    ↓
自定义绘制 (可选)
    ↓
EndFrame()
    ↓
Present (显示到屏幕)
```

## ⚠️ 注意事项

1. **线程安全**: 窗口操作必须在主线程进行
2. **资源释放**: 确保窗口销毁时释放所有资源
3. **事件处理**: 窗口关闭事件需要正确处理
4. **DPI 缩放**: 高 DPI 显示器需要考虑缩放因子

## 🚀 性能优化

### 减少重绘

```cpp
// 只在需要时重绘
if (window->IsDirty()) {
    window->BeginFrame();
    // 渲染...
    window->EndFrame();
    window->ClearDirty();
}
```

### 使用 VSync

```cpp
// SDL3 默认启用 VSync
// 避免不必要的高帧率渲染
```

### 延迟加载

```cpp
// 延迟加载大型资源
window->Show();  // 先显示窗口

// 异步加载内容
std::thread([window]() {
    auto html = FileUtils::ReadFile("large_page.html");
    window->LoadHTML(html);
}).detach();
```

## 🔧 窗口事件

### 支持的事件类型

- `SDL_EVENT_WINDOW_CLOSE` - 窗口关闭
- `SDL_EVENT_WINDOW_RESIZED` - 窗口调整大小
- `SDL_EVENT_WINDOW_MOVED` - 窗口移动
- `SDL_EVENT_WINDOW_MINIMIZED` - 窗口最小化
- `SDL_EVENT_WINDOW_MAXIMIZED` - 窗口最大化
- `SDL_EVENT_WINDOW_RESTORED` - 窗口恢复
- `SDL_EVENT_WINDOW_FOCUS_GAINED` - 获得焦点
- `SDL_EVENT_WINDOW_FOCUS_LOST` - 失去焦点

### 事件处理示例

```cpp
void WindowManager::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_WINDOW_CLOSE) {
        auto window = GetWindow(event.window.windowID);
        if (window) {
            window->Close();
            DestroyWindow(window);
        }
    }
    else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        auto window = GetWindow(event.window.windowID);
        if (window) {
            // 处理窗口调整大小
            window->HandleResize(event.window.data1, event.window.data2);
        }
    }
}
```

## 🐛 调试技巧

### 窗口信息打印

```cpp
void PrintWindowInfo(std::shared_ptr<Window> window) {
    int width, height;
    window->GetSize(&width, &height);
    
    int x, y;
    window->GetPosition(&x, &y);
    
    Logger::Info("Window: {}", window->GetTitle());
    Logger::Info("  Size: {}x{}", width, height);
    Logger::Info("  Position: ({}, {})", x, y);
    Logger::Info("  Fullscreen: {}", window->IsFullscreen());
    Logger::Info("  Maximized: {}", window->IsMaximized());
    Logger::Info("  Visible: {}", window->IsVisible());
}
```

### 渲染调试

```cpp
// 显示 FPS
void ShowFPS(std::shared_ptr<Window> window) {
    static int frame_count = 0;
    static auto last_time = TimeUtils::GetTimestampMs();
    
    frame_count++;
    auto current_time = TimeUtils::GetTimestampMs();
    
    if (current_time - last_time >= 1000) {
        double fps = frame_count * 1000.0 / (current_time - last_time);
        window->SetTitle(StringUtils::Format("MBink - FPS: {:.1f}", fps));
        
        frame_count = 0;
        last_time = current_time;
    }
}
```

## 📚 相关文档

- [SDL3 官方文档](https://wiki.libsdl.org/SDL3/)
- [Skia 官方文档](https://skia.org/)
- [窗口系统设计](../../docs/ARCHITECTURE.md#窗口系统)
- [事件循环文档](../event/README.md)

## 🔮 未来改进

1. **透明窗口**: 支持窗口透明和异形窗口
2. **多显示器**: 更好的多显示器支持
3. **窗口动画**: 窗口打开/关闭动画
4. **原生菜单**: 集成原生菜单栏
5. **系统托盘**: 系统托盘图标支持

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

