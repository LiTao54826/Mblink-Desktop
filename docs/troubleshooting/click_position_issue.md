# CodeMirror 点击位置问题

## 问题描述

点击 CodeMirror 编辑器时，光标能切换到正确的行，但总是定位到行首（offset=0），而不是点击的字符位置。

## 症状

- `selectionchange` 事件中 `anchorOffset` 和 `focusOffset` 总是 0
- `posAtDOM` 返回行首位置而非点击位置
- 拖动选择时起始位置总是行首

## 根本原因

在 `core/event/dispatch/mouse_event_dispatcher.cpp` 中，有两处代码在点击后将 Selection 重置为 `(element, 0)`：

1. **HandleMouseDown** 中的 contentEditable 处理（约第 1454 行）：
```cpp
// 简化处理：将光标设置到元素开始位置
selection->Collapse(hit_result.element, 0);
```

2. **HandleMouseUp** 中的 click 事件后处理（约第 1588 行）：
```cpp
// 为 contentEditable 元素初始化 Selection
if (hit_result.element->IsContentEditable() && !was_contenteditable_dragging) {
    selection->Collapse(hit_result.element, 0);
}
```

这两处代码覆盖了 `UpdateSelectionFromClick` 在 mousedown 时正确计算的字符偏移位置。

## 事件流程

1. `mousedown` → `UpdateSelectionFromClick` 设置正确的字符偏移（如 offset=5）
2. `mousedown` → contentEditable 处理重置为 `(element, 0)` ❌
3. `mouseup` → 触发 `click` 事件
4. `click` 后 → 再次重置为 `(element, 0)` ❌

## 解决方案

移除这两处重置 Selection 的代码，因为 `UpdateSelectionFromClick` 已经在 mousedown 时正确设置了 Selection。

### 修改 1: HandleMouseDown

```cpp
// 处理 contentEditable 元素
else if (hit_result.element->IsContentEditable()) {
    // ... focus 处理 ...

    // 初始化拖动选择状态
    if (selection_manager_) {
        auto selection = selection_manager_->GetSelection(document);
        if (selection && hit_result.render_object) {
            contenteditable_dragging_ = true;
            // 注意：Selection 已经在 UpdateSelectionFromClick 中正确设置
            // 这里只需要记录拖动起始位置
            contenteditable_drag_start_node_ = selection->GetAnchorNode();
            contenteditable_drag_start_offset_ = selection->GetAnchorOffset();
        }
    }
}
```

### 修改 2: HandleMouseUp

```cpp
// 对于非输入元素，在 click 事件后设置焦点
if (tag_name != "input" && tag_name != "textarea") {
    // ... focus 处理 ...
    
    // 注意：不再在这里重置 Selection
    // Selection 已经在 mousedown 时通过 UpdateSelectionFromClick 正确设置
}
```

## 验证

修复后，点击编辑器任意位置，`selectionchange` 事件中的 offset 应该正确反映点击位置。

## 相关文件

- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/dom/selection/selection.cpp`

## 日期

2024-12-30
