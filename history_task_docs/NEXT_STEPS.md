# LightUI 下一步工作计划

**更新日期**: 2025-11-10
**当前状态**: Phase 2.4 任务1-2 完成 ✅
**下一个里程碑**: 开始任务3 - 模块集成

---

## 🎯 立即任务：开始任务3 - 模块集成

**预计时间**: 3 天
**优先级**: 🔴 高
**依赖**: 任务1（窗口系统）✅ 已完成, 任务2（事件循环）✅ 已完成

### 任务2完成状态

✅ **已完成** (100%):
- EventLoop 核心实现
- FrameController 帧率控制
- InputHandler 输入处理
- TaskScheduler 任务调度
- 46 个测试用例全部通过
- 测试覆盖率 ~95%

### 目标

集成所有模块（DOM + 渲染 + 事件 + JavaScript），形成完整的应用框架。

---

## 📋 详细子任务

### 2.1 主事件循环 (0.5天)

**文件**: `core/event/event_loop.h`, `core/event/event_loop.cpp`

**功能需求**:

```cpp
class EventLoop {
public:
    // 启动事件循环
    void Run();
    
    // 停止事件循环
    void Stop();
    
    // 单次循环迭代
    void RunOnce();
    
    // 检查是否应该退出
    bool ShouldQuit() const;
    
private:
    bool running_ = false;
    bool should_quit_ = false;
};
```

**实现要点**:
1. 使用 `SDL_PollEvent` 轮询事件
2. 分发事件到 WindowManager
3. 处理退出条件（所有窗口关闭）
4. 支持手动停止

**测试用例**:
- 启动和停止事件循环
- 单次迭代测试
- 退出条件测试

---

### 2.2 帧率控制 (0.5天)

**文件**: `core/event/frame_controller.h`, `core/event/frame_controller.cpp`

**功能需求**:

```cpp
class FrameController {
public:
    // 设置目标帧率
    void SetTargetFPS(int fps);
    
    // 开始帧
    void BeginFrame();
    
    // 结束帧（自动延迟以达到目标帧率）
    void EndFrame();
    
    // 获取当前 FPS
    float GetCurrentFPS() const;
    
    // 获取帧时间（毫秒）
    float GetFrameTime() const;
    
private:
    int target_fps_ = 60;
    Uint64 frame_start_time_ = 0;
    float current_fps_ = 0.0f;
    float frame_time_ = 0.0f;
};
```

**实现要点**:
1. 使用 `SDL_GetPerformanceCounter` 计时
2. 计算帧时间和 FPS
3. 使用 `SDL_Delay` 控制帧率
4. 支持 VSync（通过窗口配置）

**测试用例**:
- 60 FPS 目标测试
- 帧时间计算测试
- FPS 统计测试

---

### 2.3 输入事件处理 (0.5天)

**文件**: `core/event/input_handler.h`, `core/event/input_handler.cpp`

**功能需求**:

```cpp
// 鼠标事件
struct MouseEvent {
    enum Type { MOVE, DOWN, UP, WHEEL };
    Type type;
    int x, y;
    int button;  // 0=left, 1=middle, 2=right
    int wheel_x, wheel_y;
};

// 键盘事件
struct KeyboardEvent {
    enum Type { DOWN, UP, TEXT };
    Type type;
    SDL_Keycode key;
    std::string text;  // 用于文本输入
    bool ctrl, shift, alt;
};

class InputHandler {
public:
    // 处理 SDL 事件
    void HandleSDLEvent(const SDL_Event& event);
    
    // 设置鼠标事件回调
    void SetMouseCallback(std::function<void(const MouseEvent&)> callback);
    
    // 设置键盘事件回调
    void SetKeyboardCallback(std::function<void(const KeyboardEvent&)> callback);
    
private:
    std::function<void(const MouseEvent&)> mouse_callback_;
    std::function<void(const KeyboardEvent&)> keyboard_callback_;
};
```

**实现要点**:
1. 转换 SDL 事件为统一的输入事件
2. 支持鼠标移动、点击、滚轮
3. 支持键盘按键和文本输入
4. 支持修饰键（Ctrl、Shift、Alt）

**测试用例**:
- 鼠标事件转换测试
- 键盘事件转换测试
- 回调触发测试

---

### 2.4 任务调度 (0.5天)

**文件**: `core/event/task_scheduler.h`, `core/event/task_scheduler.cpp`

**功能需求**:

```cpp
class TaskScheduler {
public:
    // setTimeout
    int SetTimeout(std::function<void()> callback, int delay_ms);
    
    // setInterval
    int SetInterval(std::function<void()> callback, int interval_ms);
    
    // clearTimeout / clearInterval
    void ClearTask(int task_id);
    
    // requestAnimationFrame
    int RequestAnimationFrame(std::function<void(float)> callback);
    
    // 执行到期的任务
    void ProcessTasks();
    
private:
    struct Task {
        int id;
        std::function<void()> callback;
        Uint64 execute_time;
        int interval;  // 0 表示一次性任务
        bool cancelled;
    };
    
    std::vector<Task> tasks_;
    int next_task_id_ = 1;
};
```

**实现要点**:
1. 使用优先队列管理任务
2. 基于时间戳调度任务
3. 支持一次性和重复任务
4. requestAnimationFrame 在每帧开始时执行

**测试用例**:
- setTimeout 测试
- setInterval 测试
- clearTimeout 测试
- requestAnimationFrame 测试

---

## 🔗 集成到事件循环

**主循环伪代码**:

```cpp
void EventLoop::Run() {
    running_ = true;
    
    while (running_ && !should_quit_) {
        frame_controller_.BeginFrame();
        
        // 1. 处理 SDL 事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 分发到窗口管理器
            window_manager_.HandleEvent(event);
            
            // 分发到输入处理器
            input_handler_.HandleSDLEvent(event);
            
            // 检查退出
            if (event.type == SDL_EVENT_QUIT) {
                should_quit_ = true;
            }
        }
        
        // 2. 执行调度任务
        task_scheduler_.ProcessTasks();
        
        // 3. 更新所有窗口
        for (auto& window : window_manager_.GetAllWindows()) {
            if (window->NeedsRedraw()) {
                window->Render();
            }
        }
        
        // 4. 控制帧率
        frame_controller_.EndFrame();
    }
    
    running_ = false;
}
```

---

## 📁 文件结构

```
core/event/
├── event_loop.h              # 主事件循环
├── event_loop.cpp
├── frame_controller.h        # 帧率控制
├── frame_controller.cpp
├── input_handler.h           # 输入事件处理
├── input_handler.cpp
├── task_scheduler.h          # 任务调度
├── task_scheduler.cpp
└── CMakeLists.txt

tests/unit/
├── test_event_loop.cpp       # 事件循环测试
├── test_frame_controller.cpp # 帧率控制测试
├── test_input_handler.cpp    # 输入处理测试
└── test_task_scheduler.cpp   # 任务调度测试
```

---

## ✅ 验收标准

### 功能要求

- ✅ 事件循环能够正常启动和停止
- ✅ 60 FPS 稳定运行（误差 ±5%）
- ✅ 所有输入事件正确分发
- ✅ setTimeout/setInterval 正常工作
- ✅ requestAnimationFrame 每帧调用一次

### 性能要求

- ✅ CPU 占用率 < 5%（空闲时）
- ✅ 帧时间稳定（标准差 < 2ms）
- ✅ 事件响应延迟 < 16ms

### 测试要求

- ✅ 至少 15 个单元测试
- ✅ 所有测试通过率 100%
- ✅ 代码覆盖率 > 85%

---

## 🧪 测试计划

### 单元测试

1. **EventLoop 测试** (5个)
   - 启动/停止测试
   - 单次迭代测试
   - 退出条件测试
   - 多窗口场景测试
   - 异常处理测试

2. **FrameController 测试** (4个)
   - FPS 计算测试
   - 帧时间测试
   - 延迟控制测试
   - 性能统计测试

3. **InputHandler 测试** (3个)
   - 鼠标事件测试
   - 键盘事件测试
   - 回调触发测试

4. **TaskScheduler 测试** (5个)
   - setTimeout 测试
   - setInterval 测试
   - clearTimeout 测试
   - requestAnimationFrame 测试
   - 任务优先级测试

### 集成测试

1. **完整事件循环测试**
   - 创建窗口 → 运行事件循环 → 处理输入 → 渲染 → 关闭

2. **性能测试**
   - 1000 帧稳定性测试
   - CPU/内存占用测试

---

## 📊 时间安排

| 子任务 | 预计时间 | 开始日期 | 完成日期 |
|--------|---------|---------|---------|
| 2.1 主事件循环 | 0.5天 | Day 1 上午 | Day 1 下午 |
| 2.2 帧率控制 | 0.5天 | Day 1 下午 | Day 2 上午 |
| 2.3 输入事件处理 | 0.5天 | Day 2 上午 | Day 2 下午 |
| 2.4 任务调度 | 0.5天 | Day 2 下午 | Day 2 晚上 |
| 测试和调试 | 0.5天 | Day 3 | Day 3 |

**总计**: 2.5天（包含测试）

---

## 🎯 成功指标

完成后，应该能够：

1. ✅ 创建一个窗口并运行事件循环
2. ✅ 窗口以 60 FPS 稳定刷新
3. ✅ 鼠标移动时能看到坐标变化
4. ✅ 键盘输入能被捕获
5. ✅ setTimeout 能在指定时间后执行
6. ✅ requestAnimationFrame 能驱动动画

**示例代码**:

```cpp
int main() {
    // 创建窗口
    WindowConfig config;
    config.title = "Event Loop Test";
    auto window = std::make_shared<Window>(config);
    
    // 创建事件循环
    EventLoop loop;
    
    // 设置输入回调
    loop.GetInputHandler().SetMouseCallback([](const MouseEvent& e) {
        if (e.type == MouseEvent::MOVE) {
            std::cout << "Mouse: " << e.x << ", " << e.y << std::endl;
        }
    });
    
    // 设置动画
    loop.GetTaskScheduler().RequestAnimationFrame([](float dt) {
        // 更新动画
    });
    
    // 运行事件循环
    loop.Run();
    
    return 0;
}
```

---

## 📝 注意事项

1. **线程安全**: 当前实现为单线程，所有操作在主线程
2. **性能优化**: 避免在事件循环中执行耗时操作
3. **错误处理**: 所有回调应该捕获异常，避免崩溃
4. **内存管理**: 使用智能指针管理任务和回调

---

## 🔜 后续任务预览

完成任务2后，将进入：

**任务3: 模块集成** (3天)
- 连接 DOM → 样式 → 布局 → 渲染
- 实现完整的渲染管线
- JavaScript 全局对象绑定

**任务4: 示例应用** (3天)
- Hello World
- 计数器应用
- 待办事项列表
- 动画演示

---

**准备好了吗？让我们开始实现事件循环！** 🚀

