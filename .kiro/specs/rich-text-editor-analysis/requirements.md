# Requirements Document

## Introduction

本文档对 LightUI 引擎的富文本编辑（contentEditable）功能进行全面分析，识别现有的结构问题、逻辑问题、缺失功能和不完整功能，并提出改进需求。

富文本编辑是浏览器引擎的核心功能之一，涉及多个子系统的协作：
- **事件处理**：鼠标事件、键盘事件的捕获和分发
- **选择管理**：Selection/Range API 的实现
- **DOM 操作**：文本插入、删除、格式化
- **渲染系统**：光标绘制、选择高亮、增量更新

## Glossary

- **contentEditable**: HTML 属性，使元素内容可编辑
- **Selection**: 表示用户选择的文本范围或光标位置
- **Range**: 表示文档中的一个连续范围
- **Caret**: 文本光标（插入点）
- **Hit Testing**: 将屏幕坐标转换为 DOM 节点位置
- **execCommand**: 执行编辑命令的 API（如 bold、italic）
- **beforeinput/input**: 输入相关的 DOM 事件

---

## Requirements

### Requirement 1: 鼠标拖拽选择功能

**User Story:** As a user, I want to drag my mouse to select text in a contentEditable element, so that I can select multiple characters or words at once.

#### Acceptance Criteria

1. WHEN a user presses the mouse button down on a contentEditable element THEN the system SHALL record the starting position and begin drag selection mode
2. WHEN a user moves the mouse while holding the button down THEN the system SHALL update the selection to extend from the start position to the current position
3. WHEN a user releases the mouse button after dragging THEN the system SHALL finalize the selection and preserve it without resetting
4. WHEN a user drags across multiple DOM elements within the same contentEditable root THEN the system SHALL correctly select text spanning those elements
5. WHEN a user drags outside the contentEditable element boundaries THEN the system SHALL extend the selection to the nearest valid position

**Current Issues:**
- mousemove 事件在拖拽选择时没有被正确处理
- `contenteditable_dragging` 状态在 mousemove 时检测不到
- 跨元素选择时，只在 `last_mousedown_element` 中查找文本节点，而不是在整个 contentEditable 根元素中查找

---

### Requirement 2: 选择状态管理架构

**User Story:** As a developer, I want a clean separation between selection state management and event handling, so that the code is maintainable and testable.

#### Acceptance Criteria

1. THE SelectionManager SHALL maintain selection state independently from event handling code
2. THE SelectionManager SHALL provide methods for starting, updating, and ending drag selection
3. WHEN selection state changes THEN the SelectionManager SHALL notify relevant components for repaint
4. THE system SHALL use a single source of truth for drag selection state instead of scattered static variables
5. THE system SHALL properly handle the contentEditable root element lookup for cross-node operations

**Current Issues:**
- 拖拽选择状态（`contenteditable_dragging`、`contenteditable_drag_start_node`）分散在 `event_loop.cpp` 中作为 static 变量
- 选择逻辑与事件处理逻辑混杂在一起，难以维护
- `FindEditableElement` 在多个地方重复实现

---

### Requirement 3: 光标移动功能

**User Story:** As a user, I want to use arrow keys to move the cursor within contentEditable elements, so that I can navigate through text efficiently.

#### Acceptance Criteria

1. WHEN a user presses the Left Arrow key THEN the system SHALL move the cursor one character to the left
2. WHEN a user presses the Right Arrow key THEN the system SHALL move the cursor one character to the right
3. WHEN a user presses the Up Arrow key THEN the system SHALL move the cursor to the same horizontal position on the previous line
4. WHEN a user presses the Down Arrow key THEN the system SHALL move the cursor to the same horizontal position on the next line
5. WHEN a user presses Home THEN the system SHALL move the cursor to the beginning of the current line
6. WHEN a user presses End THEN the system SHALL move the cursor to the end of the current line
7. WHEN a user presses Ctrl+Left THEN the system SHALL move the cursor to the beginning of the previous word
8. WHEN a user presses Ctrl+Right THEN the system SHALL move the cursor to the beginning of the next word
9. WHEN a user holds Shift while pressing arrow keys THEN the system SHALL extend the selection instead of moving the cursor

**Current Issues:**
- `MoveCursorUp` 和 `MoveCursorDown` 只是简单地移动到行首/行尾，没有真正实现上下行移动
- 上下行移动需要布局信息，但当前实现没有访问渲染树

---

### Requirement 4: 文本删除功能

**User Story:** As a user, I want to delete text using Backspace and Delete keys, so that I can edit content efficiently.

#### Acceptance Criteria

1. WHEN a user presses Backspace with no selection THEN the system SHALL delete the character before the cursor
2. WHEN a user presses Delete with no selection THEN the system SHALL delete the character after the cursor
3. WHEN a user presses Backspace or Delete with a selection THEN the system SHALL delete the selected content
4. WHEN a user deletes at the beginning of a block element THEN the system SHALL merge with the previous block element
5. WHEN a user deletes at the end of a block element THEN the system SHALL merge with the next block element
6. WHEN deleting results in an empty formatting element THEN the system SHALL remove the empty element
7. WHEN deleting crosses multiple nodes THEN the system SHALL correctly handle the DOM structure

**Current Issues:**
- `DeleteCharacter` 的 forward=true（Delete 键）在文本末尾时没有实现向后合并
- 跨节点删除后的 DOM 结构清理不完整

---

### Requirement 5: 文本格式化功能

**User Story:** As a user, I want to apply formatting (bold, italic, underline) to selected text, so that I can style my content.

#### Acceptance Criteria

1. WHEN a user presses Ctrl+B with a selection THEN the system SHALL wrap the selected text in a `<strong>` element
2. WHEN a user presses Ctrl+I with a selection THEN the system SHALL wrap the selected text in an `<em>` element
3. WHEN a user presses Ctrl+U with a selection THEN the system SHALL wrap the selected text in a `<u>` element
4. WHEN a user applies formatting to already formatted text THEN the system SHALL toggle the formatting off
5. WHEN a user applies formatting across multiple nodes THEN the system SHALL correctly handle the DOM structure
6. WHEN formatting is applied THEN the system SHALL preserve the selection on the formatted text

**Current Issues:**
- 格式化只支持同一文本节点内的选择，不支持跨节点格式化
- 没有实现格式切换（toggle）功能
- `ApplyBold`、`ApplyItalic`、`ApplyUnderline` 代码高度重复

---

### Requirement 6: 剪贴板功能

**User Story:** As a user, I want to copy, cut, and paste text using keyboard shortcuts, so that I can efficiently edit content.

#### Acceptance Criteria

1. WHEN a user presses Ctrl+C with a selection THEN the system SHALL copy the selected text to the clipboard
2. WHEN a user presses Ctrl+X with a selection THEN the system SHALL cut the selected text to the clipboard
3. WHEN a user presses Ctrl+V THEN the system SHALL paste text from the clipboard at the cursor position
4. WHEN pasting THEN the system SHALL replace any existing selection with the pasted content
5. WHEN copying or cutting THEN the system SHALL dispatch copy/cut events to allow JavaScript interception

**Current Issues:**
- 剪贴板快捷键处理分散在 `event_loop.cpp` 中，与 `ClipboardManager` 的职责重叠
- 没有正确分发 clipboard 事件

---

### Requirement 7: 撤销/重做功能

**User Story:** As a user, I want to undo and redo my edits, so that I can recover from mistakes.

#### Acceptance Criteria

1. WHEN a user presses Ctrl+Z THEN the system SHALL undo the last edit operation
2. WHEN a user presses Ctrl+Y THEN the system SHALL redo the last undone operation
3. WHEN undoing THEN the system SHALL restore the previous content and cursor position
4. WHEN redoing THEN the system SHALL restore the content and cursor position before the undo
5. THE system SHALL maintain a reasonable undo history limit (e.g., 100 operations)
6. WHEN the document is modified after undo THEN the system SHALL clear the redo stack

**Current Issues:**
- 撤销状态只保存 innerHTML，没有保存精确的光标位置
- 撤销后光标位置恢复不准确
- 没有实现操作合并（如连续输入应该合并为一个撤销操作）

---

### Requirement 8: 光标渲染功能

**User Story:** As a user, I want to see a blinking cursor at my current position, so that I know where my input will appear.

#### Acceptance Criteria

1. WHEN a contentEditable element has focus THEN the system SHALL display a blinking cursor at the current position
2. THE cursor SHALL blink at a consistent interval (approximately 500ms)
3. WHEN the cursor position changes THEN the system SHALL reset the blink state to visible
4. THE cursor SHALL be rendered at the correct position based on text layout
5. WHEN there is a selection THEN the system SHALL hide the cursor and show selection highlight instead

**Current Issues:**
- 光标位置计算依赖于 `findTextPosition` 函数，该函数在某些情况下可能返回错误位置
- 光标闪烁状态管理分散在 `event_loop.cpp` 中

---

### Requirement 9: 选择高亮渲染功能

**User Story:** As a user, I want to see my selected text highlighted, so that I know what content is selected.

#### Acceptance Criteria

1. WHEN text is selected THEN the system SHALL render a highlight background behind the selected text
2. THE highlight SHALL correctly span across multiple lines and elements
3. THE highlight color SHALL be consistent with system selection color
4. WHEN the selection changes THEN the system SHALL update the highlight immediately

**Current Issues:**
- 选择高亮渲染在 `render_object.cpp` 中实现，但与 Selection 状态的同步可能存在问题
- 跨行选择的高亮渲染可能不正确

---

### Requirement 10: 输入事件分发

**User Story:** As a developer, I want beforeinput and input events to be dispatched correctly, so that JavaScript can intercept and customize editing behavior.

#### Acceptance Criteria

1. WHEN text is about to be inserted THEN the system SHALL dispatch a beforeinput event with inputType="insertText"
2. WHEN text is about to be deleted THEN the system SHALL dispatch a beforeinput event with appropriate inputType
3. IF the beforeinput event is cancelled THEN the system SHALL not perform the edit operation
4. WHEN an edit operation completes THEN the system SHALL dispatch an input event
5. THE events SHALL bubble up through the DOM tree

**Current Issues:**
- beforeinput 和 input 事件的 inputType 值可能不完全符合规范
- 事件分发的目标元素选择可能不正确

---

### Requirement 11: 增量渲染集成

**User Story:** As a developer, I want contentEditable edits to trigger efficient incremental rendering, so that the UI remains responsive.

#### Acceptance Criteria

1. WHEN text is inserted or deleted THEN the system SHALL only repaint the affected area
2. WHEN the edit affects a small area THEN the system SHALL use incremental rendering
3. WHEN the edit affects a large area THEN the system SHALL fall back to full rendering
4. THE system SHALL correctly mark dirty regions for repaint

**Current Issues:**
- `OnSubtreeModified` 的区域计算可能不准确
- 某些编辑操作可能触发不必要的全量重绘

---

### Requirement 12: 代码架构改进

**User Story:** As a developer, I want the contentEditable implementation to follow clean architecture principles, so that it is maintainable and extensible.

#### Acceptance Criteria

1. THE system SHALL separate event handling from editing logic
2. THE system SHALL use a single ContentEditableHandler for all editing operations
3. THE system SHALL avoid code duplication in formatting commands
4. THE system SHALL use consistent error handling and logging
5. THE system SHALL provide clear interfaces between components

**Current Issues:**
- 事件处理代码（`event_loop.cpp`）过于庞大，包含了大量应该在其他模块中的逻辑
- 格式化命令（bold、italic、underline）代码高度重复
- 静态变量分散在函数中，难以追踪状态
- 缺少统一的错误处理机制

---

## Summary of Current Issues

### 结构问题
1. **状态管理分散**：拖拽选择状态、光标闪烁状态等分散在 `event_loop.cpp` 的 static 变量中
2. **职责不清**：事件处理、选择管理、编辑操作混杂在一起
3. **代码重复**：格式化命令、文本节点查找等逻辑重复实现

### 逻辑问题
1. **拖拽选择失效**：mousemove 事件没有正确触发 contentEditable 拖拽选择处理
2. **跨元素选择**：只在 `last_mousedown_element` 中查找，而不是在 contentEditable 根元素中
3. **光标移动**：上下行移动没有真正实现，只是移动到行首/行尾

### 缺失功能
1. **Delete 键向后合并**：在文本末尾按 Delete 键没有实现合并下一个节点
2. **格式切换**：没有实现已格式化文本的格式移除
3. **跨节点格式化**：格式化只支持同一文本节点内的选择
4. **操作合并**：撤销系统没有合并连续的相同操作

### 不完整功能
1. **撤销/重做**：光标位置恢复不准确
2. **选择高亮**：跨行选择的渲染可能不正确
3. **事件分发**：beforeinput/input 事件的 inputType 可能不完整
