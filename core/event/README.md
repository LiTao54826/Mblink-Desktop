# Event 模块

## 📋 概述

Event 模块是 MBink 的事件系统核心，负责处理所有系统事件、用户输入和应用程序事件。它提供了完整的事件循环、事件分发、输入处理、任务调度和帧率控制功能。

## 🎯 主要功能

- **主事件循环**: 基于 SDL3 的高性能事件循环
- **事件分发**: DOM 事件的捕获、目标和冒泡阶段
- **输入处理**: 鼠标、键盘、触摸等输入事件
- **任务调度**: `setTimeout`、`setInterval`、`requestAnimationFrame`
- **帧率控制**: 60 FPS 渲染控制
- **焦点管理**: 键盘焦点和 Tab 导航
- **拖放管理**: 拖放操作支持
- **命中测试**: 精确的鼠标事件目标定位

## 📁 文件结构

```
event/
├── CMakeLists.txt              # 构建配置
├── event_loop.h/cpp            # 主事件循环
├── event_system.h/cpp          # 事件系统
├── input_handler.h/cpp         # 输入处理器
├── task_scheduler.h/cpp        # 任务调度器
├── frame_controller.h/cpp      # 帧率控制器
├── focus_manager.h/cpp         # 焦点管理器
├── drag_manager.h/cpp          # 拖放管理器
├── hit_testing.h/cpp           # 命中测试
├── mouse_event.h/cpp           # 鼠标事件
├── keyboard_event.h/cpp        # 键盘事件
├── keyboard_utils.h/cpp        # 键盘工具
├── event_types.h/cpp           # 事件类型定义
├── event.h/cpp                 # 基础事件类
└── data_transfer.h/cpp         # 拖放数据传输
```

## 🔌 核心类

### EventLoop (主事件循环)

```cpp
class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    // 启动和停止
    void Run();
    void Stop();
    
    // 窗口管理
    void SetWindowManager(std::shared_ptr<WindowManager> wm);
    
    // 帧率控制
    void SetTargetFPS(int fps);
    int GetTargetFPS() const;
    
    // 任务调度
    std::shared_ptr<TaskScheduler> GetTaskScheduler();
    
    // 焦点管理
    std::shared_ptr<FocusManager> GetFocusManager();
};
```

### EventSystem (事件分发)

```cpp
class EventSystem {
public:
    // 分发事件（捕获 -> 目标 -> 冒泡）
    void DispatchEvent(std::shared_ptr<Element> target, 
                       std::shared_ptr<Event> event);
    
    // 触发特定类型事件
    void TriggerMouseEvent(const std::string& type, 
                          std::shared_ptr<Element> target,
                          const MouseEvent& event);
    
    void TriggerKeyboardEvent(const std::string& type,
                             std::shared_ptr<Element> target,
                             const KeyboardEvent& event);
};
```

### InputHandler (输入处理)

```cpp
class InputHandler {
public:
    // 处理 SDL 事件
    void HandleSDLEvent(const SDL_Event& event);
    
    // 鼠标事件
    void HandleMouseMove(int x, int y);
    void HandleMouseDown(int button, int x, int y);
    void HandleMouseUp(int button, int x, int y);
    void HandleMouseWheel(int delta_x, int delta_y);
    
    // 键盘事件
    void HandleKeyDown(SDL_Keycode key, SDL_Keymod mod);
    void HandleKeyUp(SDL_Keycode key, SDL_Keymod mod);
    void HandleTextInput(const std::string& text);
};
```

### TaskScheduler (任务调度)

```cpp
class TaskScheduler {
public:
    // setTimeout
    uint32_t SetTimeout(std::function<void()> callback, uint32_t delay_ms);
    void ClearTimeout(uint32_t timer_id);
    
    // setInterval
    uint32_t SetInterval(std::function<void()> callback, uint32_t interval_ms);
    void ClearInterval(uint32_t timer_id);
    
    // requestAnimationFrame
    uint32_t RequestAnimationFrame(std::function<void(double)> callback);
    void CancelAnimationFrame(uint32_t frame_id);
    
    // 执行到期任务
    void ProcessTasks();
};
```

### FocusManager (焦点管理)

```cpp
class FocusManager {
public:
    // 焦点控制
    void SetFocus(std::shared_ptr<Element> element);
    std::shared_ptr<Element> GetFocusedElement() const;
    void Blur();
    
    // Tab 导航
    void FocusNext();
    void FocusPrevious();
    
    // 焦点事件
    void TriggerFocusEvent(std::shared_ptr<Element> element);
    void TriggerBlurEvent(std::shared_ptr<Element> element);
};
```

### DragManager (拖放管理)

```cpp
class DragManager {
public:
    // 拖放操作
    void StartDrag(std::shared_ptr<Element> element, 
                   std::shared_ptr<DataTransfer> data);
    void UpdateDrag(int x, int y);
    void EndDrag();
    
    // 拖放事件
    void TriggerDragStart(std::shared_ptr<Element> element);
    void TriggerDragOver(std::shared_ptr<Element> element);
    void TriggerDrop(std::shared_ptr<Element> element);
};
```

## 💡 使用示例

### 启动事件循环

```cpp
auto event_loop = std::make_shared<EventLoop>();
auto window_manager = std::make_shared<WindowManager>();

event_loop->SetWindowManager(window_manager);
event_loop->SetTargetFPS(60);

// 运行事件循环（阻塞）
event_loop->Run();
```

### 任务调度

```cpp
auto scheduler = event_loop->GetTaskScheduler();

// setTimeout
scheduler->SetTimeout([]() {
    std::cout << "Delayed task" << std::endl;
}, 1000);

// setInterval
auto interval_id = scheduler->SetInterval([]() {
    std::cout << "Repeating task" << std::endl;
}, 500);

// requestAnimationFrame
scheduler->RequestAnimationFrame([](double timestamp) {
    std::cout << "Animation frame: " << timestamp << std::endl;
});
```

### 事件监听

```cpp
auto button = doc->CreateElement("button");

button->AddEventListener("click", [](std::shared_ptr<Event> e) {
    auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(e);
    std::cout << "Clicked at: " << mouse_event->GetClientX() 
              << ", " << mouse_event->GetClientY() << std::endl;
});

button->AddEventListener("keydown", [](std::shared_ptr<Event> e) {
    auto kb_event = std::dynamic_pointer_cast<KeyboardEvent>(e);
    std::cout << "Key pressed: " << kb_event->GetKey() << std::endl;
});
```

### 焦点管理

```cpp
auto focus_manager = event_loop->GetFocusManager();

// 设置焦点
auto input = doc->QuerySelector("input");
focus_manager->SetFocus(input);

// Tab 导航
focus_manager->FocusNext();  // 下一个可聚焦元素
focus_manager->FocusPrevious();  // 上一个可聚焦元素
```

## 🔗 依赖关系

### 依赖的模块

- `core/window` - 窗口管理
- `core/dom` - DOM 元素
- `third_party/SDL3` - 系统事件

### 被依赖的模块

- `core/quickjs` - JavaScript 事件绑定
- `core/render` - 渲染触发

## 🏗️ 架构说明

Event 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  SDL3 Events                            │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Event Module (core/event) ← 当前模块    │
│  EventLoop, InputHandler, TaskScheduler │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  DOM Events (core/dom)                  │
└─────────────────────────────────────────┘
```

## 📊 事件流程

### 事件分发流程

```
SDL Event
    ↓
InputHandler (转换为 DOM 事件)
    ↓
HitTesting (查找目标元素)
    ↓
EventSystem (分发事件)
    ↓
1. 捕获阶段 (从根到目标)
2. 目标阶段 (目标元素)
3. 冒泡阶段 (从目标到根)
```

### 帧循环流程

```
1. 处理 SDL 事件
2. 执行到期任务 (setTimeout/setInterval)
3. 执行动画帧回调 (requestAnimationFrame)
4. 更新布局
5. 渲染
6. 等待下一帧 (60 FPS)
```

## 🎮 支持的事件类型

### 鼠标事件
- `click`, `dblclick`
- `mousedown`, `mouseup`
- `mousemove`, `mouseenter`, `mouseleave`
- `mouseover`, `mouseout`
- `wheel`

### 键盘事件
- `keydown`, `keyup`, `keypress`
- `input`, `change`

### 焦点事件
- `focus`, `blur`
- `focusin`, `focusout`

### 拖放事件
- `dragstart`, `drag`, `dragend`
- `dragenter`, `dragover`, `dragleave`
- `drop`

### 表单事件
- `submit`, `reset`
- `change`, `input`

## ⚠️ 注意事项

1. **线程安全**: 事件循环运行在主线程，所有 DOM 操作应在主线程
2. **事件顺序**: 事件按照捕获 -> 目标 -> 冒泡顺序分发
3. **性能**: 避免在事件处理器中执行耗时操作
4. **内存泄漏**: 及时移除不需要的事件监听器

## 🔧 性能优化

- **事件委托**: 在父元素上监听，减少监听器数量
- **节流/防抖**: 限制高频事件（如 `mousemove`）的处理频率
- **批量更新**: 在 `requestAnimationFrame` 中批量更新 DOM
- **命中测试缓存**: 缓存元素位置信息

## 📚 相关文档

- [事件系统设计](../../docs/ARCHITECTURE.md#事件系统)
- [DOM API 文档](../../docs/DOM_API.md)
- [性能优化指南](../../docs/PERFORMANCE.md)

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

