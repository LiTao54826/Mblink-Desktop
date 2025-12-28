# Event Dispatch 子模块

事件分发组件。

## 文件列表

| 文件 | 描述 |
|------|------|
| `mouse_event_dispatcher.h/cpp` | 鼠标事件分发器 |
| `keyboard_event_dispatcher.h/cpp` | 键盘事件分发器 |

## 当前状态

框架已创建，包含 TODO 占位符。
完整的代码迁移需要从 `event_loop.cpp` 逐步提取。

## 功能

- 鼠标事件分发（click, mousedown, mouseup, mousemove 等）
- 键盘事件分发（keydown, keyup, keypress 等）
- Hover 链管理
- 鼠标光标更新
- 快捷键处理
