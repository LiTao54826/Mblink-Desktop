# Event Subsystem

事件处理子系统，负责处理所有用户输入和系统事件。

## 目录结构

```
core/event/
├── dispatch/          # 事件分发器（计划中）
├── input/             # 输入处理组件（计划中）
├── loop/              # 事件循环核心（计划中）
├── types/             # 事件类型定义（计划中）
├── event_loop.cpp     # 主事件循环
├── frame_controller.* # 帧控制器
├── task_scheduler.*   # 任务调度器
├── event.*            # 基础事件类
├── mouse_event.*      # 鼠标事件
├── keyboard_event.*   # 键盘事件
├── hit_testing.*      # 命中测试
├── focus_manager.*    # 焦点管理
└── ...
```

## 模块列表

### 核心循环
| 文件 | 描述 |
|------|------|
| `event_loop.h/cpp` | 主事件循环，处理 SDL 事件分发 |
| `frame_controller.h/cpp` | 帧率控制器，管理 FPS 和帧时间 |
| `task_scheduler.h/cpp` | 任务调度器，实现 setTimeout/setInterval/requestAnimationFrame |

### 事件类型
| 文件 | 描述 |
|------|------|
| `event.h/cpp` | 基础事件类 |
| `event_types.h/cpp` | 事件类型定义 |
| `mouse_event.h/cpp` | 鼠标事件 |
| `keyboard_event.h/cpp` | 键盘事件 |
| `keyboard_utils.h/cpp` | 键盘工具函数 |

### 输入处理
| 文件 | 描述 |
|------|------|
| `input_handler.h/cpp` | 输入事件处理器 |
| `hit_testing.h/cpp` | Hit Testing 引擎 |
| `focus_manager.h/cpp` | 焦点管理器 |

### 其他
| 文件 | 描述 |
|------|------|
| `event_system.h/cpp` | 事件系统 |
| `data_transfer.h/cpp` | 数据传输（拖放） |

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

## 注意事项

- `event_loop.cpp` 当前超过 3900 行，计划拆分为独立的事件分发器
- 鼠标和键盘事件处理逻辑较复杂，包含 hover 链管理和焦点处理
