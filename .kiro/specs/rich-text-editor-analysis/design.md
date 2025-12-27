# Design Document: Rich Text Editor Analysis and Refactoring

## Overview

本设计文档描述了 LightUI 引擎富文本编辑功能的重构方案，旨在解决现有的结构问题、逻辑问题，并补充缺失和不完整的功能。

重构的核心目标是：
1. **清晰的职责分离**：将事件处理、选择管理、编辑操作分离到独立的模块
2. **统一的状态管理**：消除分散的 static 变量，使用集中的状态管理
3. **完整的功能实现**：补充缺失的功能，完善不完整的功能
4. **可测试性**：使代码易于单元测试和集成测试

## Architecture

### 当前架构问题

```
┌─────────────────────────────────────────────────────────────────┐
│                      event_loop.cpp                              │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ HandleMouseEventForDOM()                                     ││
│  │ - static contenteditable_dragging                            ││
│  │ - static contenteditable_drag_start_node                     ││
│  │ - static last_mousedown_element                              ││
│  │ - 大量的 hit testing 和选择逻辑                               ││
│  │ - 光标闪烁控制                                                ││
│  │ - 剪贴板快捷键处理                                            ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                ContentEditableHandler                            │
│  - HandleTextInput()                                             │
│  - HandleKeyDown()                                               │
│  - InsertText(), DeleteCharacter(), etc.                         │
│  - 撤销/重做栈                                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   SelectionManager                               │
│  - GetSelection()                                                │
│  - 光标位置计算（未完全实现）                                     │
└─────────────────────────────────────────────────────────────────┘
```

### 目标架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      event_loop.cpp                              │
│  - 事件分发（仅分发，不处理业务逻辑）                             │
│  - 调用 ContentEditableController 处理 contentEditable 事件      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              ContentEditableController (新增)                    │
│  - HandleMouseDown()                                             │
│  - HandleMouseMove()                                             │
│  - HandleMouseUp()                                               │
│  - HandleKeyDown()                                               │
│  - HandleTextInput()                                             │
│  - 管理拖拽选择状态                                              │
│  - 协调 SelectionManager 和 ContentEditableHandler               │
└─────────────────────────────────────────────────────────────────┘
                    │                   │
                    ▼                   ▼
┌───────────────────────────┐  ┌───────────────────────────────────┐
│    SelectionManager       │  │    ContentEditableHandler         │
│  - 选择状态管理            │  │  - 文本编辑操作                    │
│  - 拖拽选择状态            │  │  - 格式化命令                      │
│  - 光标位置计算            │  │  - 撤销/重做                       │
│  - 选择高亮渲染            │  │  - 事件分发                        │
└───────────────────────────┘  └───────────────────────────────────┘
```

## Components and Interfaces

### 1. ContentEditableController (新增)

负责协调 contentEditable 相关的所有事件处理。

```cpp
class ContentEditableController {
public:
    ContentEditableController(
        SelectionManager* selection_manager,
        ContentEditableHandler* editable_handler
    );

    // 鼠标事件处理
    bool HandleMouseDown(
        std::shared_ptr<Element> target,
        float x, float y,
        bool shift_key
    );
    
    bool HandleMouseMove(
        std::shared_ptr<Document> document,
        float x, float y
    );
    
    bool HandleMouseUp(
        std::shared_ptr<Element> target,
        float x, float y
    );

    // 键盘事件处理
    bool HandleKeyDown(
        std::shared_ptr<Element> target,
        int key_code,
        bool ctrl_key,
        bool shift_key,
        bool alt_key
    );

    // 文本输入处理
    bool HandleTextInput(
        std::shared_ptr<Element> target,
        const std::string& text
    );

    // 状态查询
    bool IsDragging() const;
    std::shared_ptr<Element> GetContentEditableRoot(std::shared_ptr<Node> node);

private:
    SelectionManager* selection_manager_;
    ContentEditableHandler* editable_handler_;
    
    // 拖拽选择状态（从 event_loop.cpp 移入）
    bool is_dragging_ = false;
    std::shared_ptr<Node> drag_start_node_;
    int drag_start_offset_ = 0;
    std::shared_ptr<Element> drag_editable_root_;
};
```

### 2. SelectionManager 扩展

扩展 SelectionManager 以支持拖拽选择。

```cpp
class SelectionManager {
public:
    // 现有方法...

    // 新增：拖拽选择支持
    void StartDragSelection(
        std::shared_ptr<Document> document,
        std::shared_ptr<Node> start_node,
        int start_offset
    );
    
    void UpdateDragSelection(
        std::shared_ptr<Document> document,
        std::shared_ptr<Node> end_node,
        int end_offset
    );
    
    void EndDragSelection(std::shared_ptr<Document> document);
    
    bool IsDragSelecting() const;

    // 新增：光标位置计算（使用渲染树）
    CaretPosition HitTestToCaretPosition(
        std::shared_ptr<RenderObject> render_root,
        float x, float y
    );

private:
    // 拖拽选择状态
    bool is_drag_selecting_ = false;
    std::shared_ptr<Node> drag_start_node_;
    int drag_start_offset_ = 0;
};
```

### 3. ContentEditableHandler 改进

改进现有的 ContentEditableHandler。

```cpp
class ContentEditableHandler {
public:
    // 现有方法...

    // 改进：统一的格式化方法
    bool ApplyFormatting(
        std::shared_ptr<Document> document,
        const std::string& tag_name  // "strong", "em", "u"
    );
    
    bool RemoveFormatting(
        std::shared_ptr<Document> document,
        const std::string& tag_name
    );
    
    bool ToggleFormatting(
        std::shared_ptr<Document> document,
        const std::string& tag_name
    );

    // 改进：跨节点格式化
    bool ApplyFormattingToRange(
        std::shared_ptr<Document> document,
        std::shared_ptr<Range> range,
        const std::string& tag_name
    );

    // 改进：撤销状态
    struct UndoState {
        std::string innerHTML;
        std::weak_ptr<Element> element;
        std::shared_ptr<Node> anchor_node;  // 改进：保存节点引用
        int anchor_offset;
        std::shared_ptr<Node> focus_node;   // 新增：保存 focus 节点
        int focus_offset;
    };

    // 新增：操作合并
    void BeginUndoGroup();
    void EndUndoGroup();

private:
    // 新增：操作合并状态
    bool in_undo_group_ = false;
    UndoState group_start_state_;
};
```

## Data Models

### CaretPosition

```cpp
struct CaretPosition {
    std::shared_ptr<Node> node;     // 光标所在节点
    int offset = 0;                  // 节点内偏移量
    float x = 0.0f;                  // 屏幕 X 坐标
    float y = 0.0f;                  // 屏幕 Y 坐标
    float height = 0.0f;             // 光标高度
    
    bool IsValid() const { return node != nullptr; }
    
    // 新增：比较操作
    bool operator==(const CaretPosition& other) const;
    bool operator<(const CaretPosition& other) const;  // 文档顺序比较
};
```

### DragSelectionState

```cpp
struct DragSelectionState {
    bool is_active = false;
    std::shared_ptr<Element> editable_root;
    CaretPosition start_position;
    CaretPosition current_position;
    
    void Reset() {
        is_active = false;
        editable_root = nullptr;
        start_position = {};
        current_position = {};
    }
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Drag selection state consistency
*For any* contentEditable element and any mouse drag sequence (mousedown, mousemove*, mouseup), the selection state after mouseup should match the range from the mousedown position to the mouseup position.
**Validates: Requirements 1.1, 1.2, 1.3**

### Property 2: Cross-element selection coverage
*For any* contentEditable root containing multiple child elements, dragging from element A to element B should result in a selection that includes all text between the start and end positions.
**Validates: Requirements 1.4, 2.5**

### Property 3: Cursor movement character boundary
*For any* text content and cursor position, pressing Left Arrow should move the cursor exactly one Unicode character to the left, and pressing Right Arrow should move exactly one Unicode character to the right.
**Validates: Requirements 3.1, 3.2**

### Property 4: Selection extension with Shift
*For any* cursor position and Shift+Arrow key press, the anchor position should remain fixed while the focus position moves, resulting in an extended selection.
**Validates: Requirements 3.9**

### Property 5: Backspace deletion correctness
*For any* text content and cursor position (not at the beginning), pressing Backspace should remove exactly one Unicode character before the cursor and move the cursor to the deletion point.
**Validates: Requirements 4.1**

### Property 6: Delete key deletion correctness
*For any* text content and cursor position (not at the end), pressing Delete should remove exactly one Unicode character after the cursor without moving the cursor.
**Validates: Requirements 4.2**

### Property 7: Selection deletion completeness
*For any* selection range, pressing Backspace or Delete should remove exactly the selected content and collapse the selection to the start position.
**Validates: Requirements 4.3**

### Property 8: Formatting toggle idempotence
*For any* selected text, applying the same formatting twice should result in the original unformatted text.
**Validates: Requirements 5.4**

### Property 9: Undo/Redo round trip
*For any* edit operation, performing Undo followed by Redo should restore the document to the state after the original edit.
**Validates: Requirements 7.1, 7.2, 7.3, 7.4**

### Property 10: Redo stack clearing
*For any* sequence of edits followed by Undo, performing a new edit should clear the redo stack, making Redo unavailable.
**Validates: Requirements 7.6**

### Property 11: beforeinput event cancellation
*For any* edit operation, if the beforeinput event is cancelled (preventDefault called), the document content should remain unchanged.
**Validates: Requirements 10.3**

### Property 12: Event bubbling
*For any* input event dispatched on a contentEditable element, the event should be received by all ancestor elements in the DOM tree.
**Validates: Requirements 10.5**

### Property 13: Clipboard round trip
*For any* selected text, copying and then pasting should insert the exact same text at the cursor position.
**Validates: Requirements 6.1, 6.3**

## Error Handling

### 错误类型

1. **无效选择**：Selection 对象为空或无效
   - 处理：返回 false，不执行操作

2. **无效节点**：目标节点为空或已从 DOM 中移除
   - 处理：返回 false，记录警告日志

3. **跨文档操作**：尝试在不同文档间操作
   - 处理：返回 false，记录错误日志

4. **撤销栈溢出**：撤销栈超过最大限制
   - 处理：移除最旧的状态

### 日志策略

```cpp
// 调试日志（仅在 DEBUG 模式）
#ifdef DEBUG
#define CE_DEBUG(msg) std::cout << "[ContentEditable] " << msg << std::endl
#else
#define CE_DEBUG(msg)
#endif

// 警告日志（始终输出）
#define CE_WARN(msg) std::cerr << "[ContentEditable WARNING] " << msg << std::endl

// 错误日志（始终输出）
#define CE_ERROR(msg) std::cerr << "[ContentEditable ERROR] " << msg << std::endl
```

## Testing Strategy

### 单元测试

1. **SelectionManager 测试**
   - 测试拖拽选择状态管理
   - 测试光标位置计算
   - 测试选择范围更新

2. **ContentEditableHandler 测试**
   - 测试文本插入/删除
   - 测试格式化命令
   - 测试撤销/重做

3. **ContentEditableController 测试**
   - 测试事件处理流程
   - 测试状态协调

### 属性测试

使用 [RapidCheck](https://github.com/emil-e/rapidcheck) 或类似的 C++ 属性测试库。

```cpp
// 示例：测试 Property 5 - Backspace deletion correctness
RC_GTEST_PROP(ContentEditableHandler, BackspaceDeletesOneCharacter,
              (const std::string& text, size_t cursor_pos)) {
    RC_PRE(!text.empty());
    RC_PRE(cursor_pos > 0 && cursor_pos <= text.length());
    
    // 设置初始状态
    auto document = CreateTestDocument();
    auto editable = CreateContentEditable(document, text);
    SetCursorPosition(document, editable, cursor_pos);
    
    // 执行 Backspace
    handler.DeleteCharacter(document, false);
    
    // 验证结果
    std::string result = editable->GetTextContent();
    RC_ASSERT(result.length() == text.length() - GetCharLengthAt(text, cursor_pos - 1));
}
```

### 集成测试

1. **拖拽选择测试**
   - 模拟完整的鼠标拖拽序列
   - 验证选择结果

2. **键盘导航测试**
   - 模拟键盘输入序列
   - 验证光标位置和选择状态

3. **剪贴板测试**
   - 模拟复制/粘贴操作
   - 验证内容正确性

### 测试框架配置

```cpp
// 属性测试配置
// 每个属性测试运行 100 次迭代
static constexpr int PBT_ITERATIONS = 100;

// 测试标注格式
// **Feature: rich-text-editor-analysis, Property {number}: {property_text}**
```
