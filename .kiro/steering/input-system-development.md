---
inclusion: fileMatch
fileMatchPattern: "core/input/**"
---

# 统一输入管理系统开发规范

本文档定义了 `core/input/` 目录下统一输入管理系统的开发规范。

## 架构参考

开发时必须参考 Blink 源码：
- `参考/blink/renderer/core/input/event_handler.h` - InputController 参考
- `参考/blink/renderer/core/page/focus_controller.h` - FocusController 参考
- `参考/blink/renderer/core/editing/frame_selection.h` - FrameSelection 参考
- `参考/blink/renderer/core/editing/selection_controller.h` - SelectionController 参考
- `参考/blink/renderer/core/editing/editor.h` - EditContext 参考
- `参考/blink/renderer/core/editing/frame_caret.h` - FrameCaret 参考
- `参考/blink/renderer/core/editing/position.h` - Position 参考

## 组件职责

| 组件 | 职责 | Blink 对应 |
|------|------|-----------|
| `InputController` | 事件处理统一入口 | `EventHandler` |
| `FocusController` | 焦点状态和导航 | `FocusController` |
| `SelectionController` | 鼠标/手势选择操作 | `SelectionController` |
| `FrameSelection` | 选区状态管理 | `FrameSelection` |
| `EditContext` | 编辑命令执行 | `Editor` |
| `FrameCaret` | 光标渲染和闪烁 | `FrameCaret` |
| `Position` | DOM 位置表示 | `Position` |

## 代码复用

优先复用现有代码：

| 新组件 | 复用来源 |
|-------|---------|
| `FocusController` | `core/event/input/focus_manager.h` |
| `FrameSelection` | `core/editing/selection_manager.h` |
| `SelectionController` | `core/editing/selection_manager.h` (鼠标处理) |
| `EditContext` | `core/editing/contenteditable_controller.h` |
| `FrameCaret` | `core/editing/selection_manager.h` (光标逻辑) |

## 命名规范

### 枚举命名
- 使用 `k` 前缀：`kNone`, `kMouse`, `kKeyboard`
- 参考 Blink 命名风格

### 方法命名
- 参考 Blink API 命名
- 示例：`SetFocusedElement()`, `AdvanceFocus()`, `HandleMousePressEvent()`

## 属性测试要求

每个组件必须有对应的属性测试，验证设计文档中的正确性属性：

### FocusController 属性测试
- Property 1: 焦点唯一性
- Property 2: 焦点转移一致性
- Property 3: Tab 导航顺序
- Property 4: 焦点元素移除

### FrameSelection 属性测试
- Property 5: 点击定位准确性
- Property 6: 拖动选区连续性
- Property 7: 选区扩展正确性
- Property 14: 选区与光标互斥

### SelectionController 属性测试
- Property 8: 双击选词边界

### EditContext 属性测试
- Property 9: 文本插入位置
- Property 10: 删除操作正确性
- Property 11: 剪贴板往返一致性
- Property 12: IME 组合状态机

### FrameCaret 属性测试
- Property 13: 光标可见性

## 测试文件位置

```
tests/property/input/
├── test_focus_controller_properties.cpp
├── test_frame_selection_properties.cpp
├── test_selection_controller_properties.cpp
├── test_edit_context_properties.cpp
└── test_frame_caret_properties.cpp
```

## 渐进式迁移

1. 新代码放在 `core/input/` 目录
2. 保持与现有代码并存
3. 通过接口适配器连接新旧系统
4. 逐步迁移各元素类型
5. 最后删除旧代码

## 参考文档

- 设计文档：`.kiro/specs/unified-input-system/design.md`
- 需求文档：`.kiro/specs/unified-input-system/requirements.md`
- 任务列表：`.kiro/specs/unified-input-system/tasks.md`
