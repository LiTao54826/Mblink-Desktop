# Editing Subsystem | 编辑子系统

## Overview | 概览

Text-editing support for selection, clipboard, contenteditable, and drag interactions.
提供选区、剪贴板、contenteditable 和拖拽相关的文本编辑支持。

## Main Files | 主要文件

- `selection_manager.h/cpp` — selection handling / 选区管理
- `clipboard_manager.h/cpp` — clipboard support / 剪贴板支持
- `contenteditable_handler.h/cpp` — event handling / 事件处理
- `contenteditable_controller.h/cpp` — editing control / 编辑控制
- `drag_manager.h/cpp` — drag-and-drop logic / 拖放逻辑

## Dependencies | 依赖关系

- `core/dom`
- `core/event`
- `SDL3`

## Notes | 说明

Exact behavior should follow the current implementation.
具体行为以当前源码实现为准。
