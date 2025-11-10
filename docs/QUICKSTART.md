# LightUI 快速开始指南

本指南将帮助你在 5 分钟内创建第一个 LightUI 应用。

---

## 📋 前提条件

- Windows 10/11 (64-bit)
- Visual Studio 2022 或 CMake 3.15+
- Git

---

## 🚀 5 分钟快速开始

### 1. 克隆项目

```bash
git clone https://github.com/yourusername/LightUI.git
cd LightUI
```

### 2. 编译项目

```bash
# 配置项目
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 编译（Debug 模式）
cmake --build build --config Debug
```

### 3. 运行示例

```bash
# Hello World 示例
./build/bin/Debug/hello_world.exe

# 计数器应用
./build/bin/Debug/counter_app.exe

# 动画演示
./build/bin/Debug/animation_demo.exe
```

---

## 📝 创建你的第一个应用

### 最简单的 Hello World

创建文件 `my_app.cpp`:

```cpp
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/event/event_loop.h"

using namespace lightui;

int main() {
    // 1. 创建窗口
    WindowConfig config;
    config.title = "My First App";
    config.width = 800;
    config.height = 600;
    
    auto window = std::make_shared<Window>(config);
    
    // 2. 注册窗口（重要！）
    auto& window_manager = WindowManager::Instance();
    window_manager.RegisterWindow(window);
    
    // 3. 创建文档
    auto document = std::make_shared<Document>();
    document->Initialize();
    
    // 4. 构建 DOM
    auto body = document->GetBody();
    auto title = document->CreateElement("h1");
    title->SetTextContent("Hello, World!");
    body->AppendChild(title);
    
    // 5. 关联文档到窗口
    window->SetDocument(document);
    window->Show();
    
    // 6. 运行事件循环
    EventLoop event_loop;
    event_loop.SetRenderCallback([window]() {
        if (window->NeedsRepaint()) {
            window->RenderDocument();
        }
    });
    event_loop.Run();
    
    return 0;
}
```

### 编译和运行

在 `examples/CMakeLists.txt` 中添加：

```cmake
add_executable(my_app my_app.cpp)

target_link_libraries(my_app PRIVATE
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    SDL3::SDL3-static
    skia
)

set_target_properties(my_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
```

编译运行：

```bash
cmake --build build --config Debug --target my_app
./build/bin/Debug/my_app.exe
```

---

## 🎨 添加 JavaScript 交互

创建 `interactive_app.cpp`:

```cpp
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/event/event_loop.h"
#include "core/event/task_scheduler.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"

using namespace lightui;

int main() {
    // 1. 创建窗口
    WindowConfig config;
    config.title = "Interactive App";
    config.width = 600;
    config.height = 400;
    
    auto window = std::make_shared<Window>(config);
    
    // 2. 注册窗口
    auto& window_manager = WindowManager::Instance();
    window_manager.RegisterWindow(window);
    
    // 3. 创建文档和调度器
    auto document = std::make_shared<Document>();
    document->Initialize();
    auto scheduler = std::make_shared<TaskScheduler>();
    
    // 4. 创建 JavaScript 运行时和绑定
    QuickJSRuntime runtime;
    WindowBindings bindings(&runtime, window, scheduler);
    
    // 5. 构建 DOM
    auto body = document->GetBody();
    auto counter = document->CreateElement("div");
    counter->SetTextContent("Count: 0");
    body->AppendChild(counter);
    counter->SetAttribute("id", "counter");
    
    // 6. 初始化 JavaScript
    runtime.Eval(R"(
        globalThis.count = 0;
        
        function updateCounter() {
            globalThis.count++;
            const counter = document.getElementById('counter');
            if (counter) {
                counter.textContent = 'Count: ' + globalThis.count;
            }
        }
        
        // 每秒自动增加
        setInterval(updateCounter, 1000);
    )");
    
    // 7. 关联文档到窗口
    window->SetDocument(document);
    window->Show();
    
    // 8. 运行事件循环
    EventLoop event_loop;
    
    event_loop.SetUpdateCallback([scheduler, &runtime](float delta_time) {
        scheduler->ProcessTasks();
        runtime.ProcessMicrotasks();
    });
    
    event_loop.SetRenderCallback([window]() {
        if (window->NeedsRepaint()) {
            window->RenderDocument();
        }
    });
    
    event_loop.Run();
    
    return 0;
}
```

---

## ⚠️ 常见错误

### 1. 窗口一闪而过

**原因**: 忘记注册窗口到 WindowManager

**解决**:
```cpp
auto& window_manager = WindowManager::Instance();
window_manager.RegisterWindow(window);
```

### 2. getElementById 返回 null

**原因**: 在 AppendChild 之前设置 ID

**解决**: 先 AppendChild，再 SetAttribute
```cpp
body->AppendChild(element);  // 先添加
element->SetAttribute("id", "my-id");  // 后设置 ID
```

### 3. setInterval 只执行一次

**原因**: 没有在事件循环中调用 ProcessTasks

**解决**: 设置 UpdateCallback
```cpp
event_loop.SetUpdateCallback([scheduler](float delta_time) {
    scheduler->ProcessTasks();
});
```

---

## 📚 下一步

- 查看 [示例文档](EXAMPLES.md) 了解更多示例
- 阅读 [API 文档](API.md) 了解完整 API
- 查看 [架构设计](ARCHITECTURE.md) 了解框架设计

---

## 🆘 获取帮助

- 查看 [常见问题](FAQ.md)
- 提交 [Issue](https://github.com/yourusername/LightUI/issues)
- 加入 [讨论](https://github.com/yourusername/LightUI/discussions)

---

**创建日期**: 2025-11-10
**最后更新**: 2025-11-10

