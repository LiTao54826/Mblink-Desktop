# Event Loop 子模块

事件循环核心组件。

## 文件列表

| 文件 | 描述 |
|------|------|
| `event_loop.h/cpp` | 主事件循环，处理 SDL 事件分发 |
| `frame_controller.h/cpp` | 帧率控制器，管理 FPS 和帧时间 |
| `task_scheduler.h/cpp` | 任务调度器，实现 setTimeout/setInterval/requestAnimationFrame |

## 依赖关系

### 依赖的模块
- `../dispatch/` - 事件分发器
- `../input/` - 输入处理（待迁移）
- `../types/` - 事件类型定义（待迁移）
- `core/dom` - DOM 元素
- `core/window` - 窗口管理
- `core/editing` - 编辑子系统

### 被依赖的模块
- `core/quickjs` - JavaScript 绑定
- `tools/*` - 应用加载器

## 使用示例

```cpp
#include "core/event/loop/event_loop.h"
#include "core/event/loop/task_scheduler.h"

// 创建事件循环
auto event_loop = std::make_unique<EventLoop>();

// 设置定时任务
auto& scheduler = event_loop->GetTaskScheduler();
scheduler.SetTimeout([]() {
    std::cout << "Hello after 1 second!" << std::endl;
}, 1000);

// 运行事件循环
event_loop->Run();
```
