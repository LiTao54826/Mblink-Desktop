# LightUI 示例应用

本文档介绍 LightUI 框架提供的示例应用，帮助你快速上手。

---

## 📚 示例列表

### 1. Hello World (`hello_world.cpp`)

**最简单的 LightUI 应用**

展示如何：
- 创建窗口
- 构建 DOM 结构
- 运行事件循环

```cpp
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/event/event_loop.h"

using namespace lightui;

int main() {
    // 1. 创建窗口
    WindowConfig config;
    config.title = "Hello LightUI";
    config.width = 800;
    config.height = 600;
    auto window = std::make_shared<Window>(config);
    
    // 2. 创建文档
    auto document = std::make_shared<Document>();
    document->Initialize();
    
    // 3. 构建 DOM
    auto body = document->GetBody();
    auto title = document->CreateElement("h1");
    title->SetTextContent("Hello, LightUI!");
    body->AppendChild(title);
    
    // 4. 关联文档到窗口
    window->SetDocument(document);
    window->Show();
    
    // 5. 运行事件循环
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

**运行方式**:
```bash
# 编译
cmake --build build --config Debug --target hello_world

# 运行
./build/bin/Debug/hello_world.exe
```

---

### 2. Counter App (`counter_app.cpp`)

**交互式计数器应用**

展示如何：
- 使用 JavaScript 绑定
- 处理用户交互
- 动态更新 DOM
- 使用定时器（setTimeout, setInterval）

**功能**:
- ➕ 增加计数
- ➖ 减少计数
- 🔄 重置计数
- ⏱️ 自动计数（每秒 +1）

**核心代码**:
```javascript
// 初始化计数器
globalThis.count = 0;

// 更新显示
function updateDisplay() {
    const counter = document.getElementById('counter');
    if (counter) {
        counter.textContent = 'Count: ' + globalThis.count;
    }
}

// 自动计数
function toggleAutoCount() {
    if (globalThis.autoIntervalId === null) {
        globalThis.autoIntervalId = setInterval(function() {
            globalThis.count++;
            updateDisplay();
        }, 1000);
    } else {
        clearInterval(globalThis.autoIntervalId);
        globalThis.autoIntervalId = null;
    }
}
```

**运行方式**:
```bash
cmake --build build --config Debug --target counter_app
./build/bin/Debug/counter_app.exe
```

---

### 3. Animation Demo (`animation_demo.cpp`)

**流畅动画演示**

展示如何：
- 使用 requestAnimationFrame
- 创建流畅的动画
- 动态更新样式
- 性能监控（FPS 显示）

**功能**:
- 📊 实时 FPS 显示
- 🎬 流畅的动画效果
- ▶️ 开始/停止动画
- 🔄 边界检测和反弹

**核心代码**:
```javascript
// 动画循环
function animate(timestamp) {
    if (!globalThis.animationRunning) {
        return;
    }
    
    // 计算 FPS
    if (globalThis.lastTime > 0) {
        const delta = timestamp - globalThis.lastTime;
        if (delta > 0) {
            globalThis.fps = Math.round(1000 / delta);
        }
    }
    globalThis.lastTime = timestamp;
    
    // 更新位置
    globalThis.position += globalThis.velocity;
    
    // 边界检测
    if (globalThis.position > 500 || globalThis.position < 0) {
        globalThis.velocity = -globalThis.velocity;
    }
    
    // 更新显示
    const box = document.getElementById('box');
    if (box) {
        box.textContent = 'Position: ' + Math.round(globalThis.position);
    }
    
    // 请求下一帧
    globalThis.animationId = requestAnimationFrame(animate);
}

// 开始动画
startAnimation();
```

**运行方式**:
```bash
cmake --build build --config Debug --target animation_demo
./build/bin/Debug/animation_demo.exe
```

---

### 4. Integration Example (`integration_example.cpp`)

**完整的模块集成示例**

展示如何：
- 集成所有核心模块
- Window + DOM + Renderer + EventLoop
- 自动重渲染机制

**运行方式**:
```bash
cmake --build build --config Debug --target integration_example
./build/bin/Debug/integration_example.exe
```

---

### 5. JavaScript Integration (`javascript_integration_example.cpp`)

**JavaScript 绑定完整示例**

展示如何：
- 使用 QuickJS 运行时
- 绑定 window 和 document 对象
- 使用所有定时器 API
- JavaScript 与 C++ 交互

**运行方式**:
```bash
cmake --build build --config Debug --target javascript_integration_example
./build/bin/Debug/javascript_integration_example.exe
```

---

## 🎯 学习路径

### 初学者
1. **Hello World** - 了解基本结构
2. **Integration Example** - 理解模块集成
3. **Counter App** - 学习交互和 JavaScript

### 进阶
4. **Animation Demo** - 掌握动画和性能
5. **JavaScript Integration** - 深入 JavaScript 绑定

---

## 🔧 编译所有示例

```bash
# 编译所有示例
cmake --build build --config Debug --target hello_world counter_app animation_demo integration_example javascript_integration_example

# 或者编译整个项目
cmake --build build --config Debug
```

---

## 📊 示例对比

| 示例 | 难度 | JavaScript | 定时器 | 动画 | 交互 |
|------|------|-----------|--------|------|------|
| Hello World | ⭐ | ❌ | ❌ | ❌ | ❌ |
| Integration Example | ⭐⭐ | ❌ | ❌ | ❌ | ❌ |
| Counter App | ⭐⭐⭐ | ✅ | ✅ | ❌ | ✅ |
| Animation Demo | ⭐⭐⭐⭐ | ✅ | ✅ | ✅ | ✅ |
| JavaScript Integration | ⭐⭐⭐⭐⭐ | ✅ | ✅ | ✅ | ✅ |

---

## 💡 提示

### 窗口配置和注册
```cpp
// 1. 创建窗口
WindowConfig config;
config.title = "My App";
config.width = 800;
config.height = 600;
config.resizable = true;
config.vsync = true;  // 启用垂直同步

auto window = std::make_shared<Window>(config);

// 2. 注册窗口到 WindowManager（重要！）
auto& window_manager = WindowManager::Instance();
window_manager.RegisterWindow(window);
```

**⚠️ 重要**: 必须将窗口注册到 `WindowManager`，否则事件循环会立即退出！

### DOM 操作
```cpp
// 创建元素
auto div = document->CreateElement("div");
div->SetTextContent("Hello");

// 必须先添加到 DOM 树
body->AppendChild(div);

// 然后设置 ID（顺序很重要！）
div->SetAttribute("id", "my-div");
```

### JavaScript 定时器
```javascript
// setTimeout - 延迟执行
setTimeout(function() {
    console.log('Delayed');
}, 1000);

// setInterval - 定期执行
const id = setInterval(function() {
    console.log('Repeated');
}, 1000);

// clearInterval - 取消定期执行
clearInterval(id);

// requestAnimationFrame - 动画帧
requestAnimationFrame(function(timestamp) {
    console.log('Frame:', timestamp);
});
```

---

## 🐛 常见问题

### Q: 窗口一闪而过，立即退出？
A: **必须将窗口注册到 WindowManager**！
```cpp
auto& window_manager = WindowManager::Instance();
window_manager.RegisterWindow(window);
```

### Q: 窗口显示黑屏？
A: 确保调用了 `window->Show()` 和 `event_loop.Run()`

### Q: DOM 修改不生效？
A: 检查是否先 `AppendChild` 再 `SetAttribute("id", ...)`

### Q: setInterval 只执行一次？
A: 确保在事件循环中多次调用 `scheduler->ProcessTasks()`

### Q: 动画不流畅？
A: 启用 VSync (`config.vsync = true`) 并使用 `requestAnimationFrame`

---

## 📚 更多资源

- [API 文档](API.md)
- [架构设计](ARCHITECTURE.md)
- [开发指南](DEVELOPMENT.md)
- [Phase 2.4 计划](../PHASE_2_4_PLAN.md)

---

**创建日期**: 2025-11-10
**最后更新**: 2025-11-10

