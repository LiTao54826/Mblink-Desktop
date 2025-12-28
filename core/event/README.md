# Event Subsystem

事件处理子系统，负责处理所有用户输入和系统事件。

## 目录结构

```
core/event/
├── CMakeLists.txt
├── README.md
├── dispatch/           # 事件分发器
│   ├── mouse_event_dispatcher.*
│   ├── keyboard_event_dispatcher.*
│   └── wheel_event_dispatcher.*
├── loop/               # 事件循环核心
│   ├── event_loop.*
│   ├── frame_controller.*
│   └── task_scheduler.*
├── types/              # 事件类型定义
│   ├── event.*
│   ├── event_types.*
│   ├── mouse_event.*
│   ├── keyboard_event.*
│   └── data_transfer.*
├── input/              # 输入处理
│   ├── input_handler.*
│   ├── hit_testing.*
│   ├── focus_manager.*
│   └── keyboard_utils.*
└── event_system.*      # 事件系统（待评估）
```

## 模块列表

### loop/ - 事件循环核心
| 文件 | 描述 |
|------|------|
| `event_loop.h/cpp` | 主事件循环，处理 SDL 事件分发 |
| `frame_controller.h/cpp` | 帧率控制器，管理 FPS 和帧时间 |
| `task_scheduler.h/cpp` | 任务调度器，实现 setTimeout/setInterval/requestAnimationFrame |

### types/ - 事件类型定义
| 文件 | 描述 |
|------|------|
| `event.h/cpp` | 基础事件类 |
| `event_types.h/cpp` | 事件类型枚举和工具函数 |
| `mouse_event.h/cpp` | 鼠标事件类 |
| `keyboard_event.h/cpp` | 键盘事件类 |
| `data_transfer.h/cpp` | 拖拽数据传输（DataTransfer API） |

### input/ - 输入处理
| 文件 | 描述 |
|------|------|
| `input_handler.h/cpp` | 输入事件处理器 |
| `hit_testing.h/cpp` | Hit Testing 引擎 |
| `focus_manager.h/cpp` | 焦点管理器 |
| `keyboard_utils.h/cpp` | 键盘工具函数 |

### dispatch/ - 事件分发器
| 文件 | 描述 |
|------|------|
| `mouse_event_dispatcher.h/cpp` | 鼠标事件分发器 |
| `keyboard_event_dispatcher.h/cpp` | 键盘事件分发器 |
| `wheel_event_dispatcher.h/cpp` | 滚轮事件分发器 |

## 依赖关系

### 依赖的模块
- `core/dom` - DOM 元素和事件目标
- `core/render` - 渲染对象（用于 Hit Testing）
- `core/window` - 窗口管理
- `core/editing` - 编辑子系统
- `SDL3` - 底层事件处理

### 被依赖的模块
- `core/quickjs` - JavaScript 绑定
- `core/dom` - DOM 事件分发
- `tools/*` - 应用加载器

## 架构说明

事件循环 (`EventLoop`) 是整个系统的核心，负责：
1. 轮询 SDL 事件
2. 分发事件到 DOM 元素
3. 执行定时任务
4. 控制帧率和渲染

### 事件流程
```
SDL Event → EventLoop → Hit Testing → DOM Element → Event Handlers
```

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
