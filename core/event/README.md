# Event Subsystem | 事件子系统

## Overview | 概览

Handles user input, dispatch, scheduling, and event-loop orchestration.
负责用户输入、事件分发、任务调度和事件循环协调。

## Structure | 目录结构

- `dispatch/` — event dispatchers / 事件分发器
- `loop/` — event loop core / 事件循环核心
- `types/` — event type definitions / 事件类型定义
- `input/` — input processing / 输入处理
- `event_system.*` — legacy or under-evaluation entry / 待评估入口

## Main Areas | 主要区域

- `event_loop.h/cpp` — SDL event polling and dispatch / SDL 事件轮询与分发
- `frame_controller.h/cpp` — frame pacing / 帧率控制
- `task_scheduler.h/cpp` — timers and animation-frame scheduling / 定时器与帧调度
- `mouse_event_dispatcher.h/cpp` — mouse dispatch / 鼠标分发
- `keyboard_event_dispatcher.h/cpp` — keyboard dispatch / 键盘分发
- `wheel_event_dispatcher.h/cpp` — wheel dispatch / 滚轮分发

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/dom`
- `core/render`
- `core/window`
- `core/editing`
- `SDL3`

Used by | 被依赖：

- `core/quickjs`
- `core/dom`
- `tools/*`

## Event Flow | 事件流程

```text
SDL Event -> EventLoop -> Hit Testing -> DOM Element -> Event Handlers
```

## Notes | 说明

The event loop is a central runtime component and should be treated as implementation-critical infrastructure.
事件循环是核心运行时基础设施，具体行为应以实现代码为准。

