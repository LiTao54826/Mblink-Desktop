# Implementation Plan

## Phase 1: 修复紧急问题 - 拖拽选择

- [x] 1. 修复鼠标拖拽选择功能


  - [x] 1.1 修复跨元素选择


    - 当前实现已在 `event_loop.cpp` 中查找 contentEditable 根元素
    - 需要确保在整个 contentEditable 根元素中查找文本节点，而不仅是 `last_mousedown_element`
    - 验证跨多个子元素（如 `<p>`, `<strong>`, `<em>`）的选择是否正常工作
    - _Requirements: 1.4, 2.5_
  - [ ]* 1.2 Write property test for drag selection
    - **Property 1: Drag selection state consistency**
    - **Validates: Requirements 1.1, 1.2, 1.3**

- [x] 2. Checkpoint - 确保拖拽选择功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 2: 架构重构 - 状态管理集中化

- [x] 3. 创建 ContentEditableController


  - [x] 3.1 创建 ContentEditableController 类


    - 创建 `core/event/contenteditable_controller.h`
    - 创建 `core/event/contenteditable_controller.cpp`
    - 定义基本接口：HandleMouseDown, HandleMouseMove, HandleMouseUp
    - 实现 `GetContentEditableRoot()` 辅助函数
    - _Requirements: 2.1, 2.2_
  - [x] 3.2 迁移拖拽选择状态


    - 将 `contenteditable_dragging` 等 static 变量从 `event_loop.cpp` 移入 ContentEditableController
    - 实现 `IsDragging()` 方法
    - _Requirements: 2.4_
  - [x] 3.3 集成到 EventLoop

    - 在 EventLoop 中创建 ContentEditableController 实例
    - 修改 HandleMouseEventForDOM 调用 ContentEditableController
    - _Requirements: 2.1_
  - [ ]* 3.4 Write property test for cross-element selection
    - **Property 2: Cross-element selection coverage**
    - **Validates: Requirements 1.4, 2.5**

- [x] 4. 扩展 SelectionManager



  - [x] 4.1 添加拖拽选择方法

    - 实现 `StartDragSelection()`
    - 实现 `UpdateDragSelection()`
    - 实现 `EndDragSelection()`
    - _Requirements: 2.2, 2.3_
  - [x] 4.2 改进光标位置计算

    - 实现基于 RenderObject 的 `HitTestToCaretPosition()` 改进版
    - 支持跨元素的光标定位
    - _Requirements: 8.4_

- [x] 5. Checkpoint - 确保架构重构后功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 3: 光标移动功能完善

- [x] 6. 实现真正的上下行移动


  - [x] 6.1 实现 MoveCursorUp


    - 获取当前光标的屏幕坐标（需要访问 RenderObject）
    - 计算上一行相同 X 坐标的位置
    - 使用 hit testing 找到目标位置
    - 当前实现只是调用 MoveCursorToLineStart，需要真正实现
    - _Requirements: 3.3_
  - [x] 6.2 实现 MoveCursorDown

    - 获取当前光标的屏幕坐标
    - 计算下一行相同 X 坐标的位置
    - 使用 hit testing 找到目标位置
    - 当前实现只是调用 MoveCursorToLineEnd，需要真正实现
    - _Requirements: 3.4_
  - [ ]* 6.3 Write property test for cursor movement
    - **Property 3: Cursor movement character boundary**
    - **Validates: Requirements 3.1, 3.2**
  - [ ]* 6.4 Write property test for selection extension
    - **Property 4: Selection extension with Shift**
    - **Validates: Requirements 3.9**

- [x] 7. Checkpoint - 确保光标移动功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 4: 删除功能完善

- [x] 8. 完善删除功能


  - [x] 8.1 实现 Delete 键向后合并


    - 在 `DeleteCharacter(forward=true)` 中处理文本末尾情况
    - 实现 `MergeToNextNode()` 方法（类似于已有的 `MergeToPreviousNode()`）
    - 处理跨元素合并（如合并两个 `<p>` 元素）
    - _Requirements: 4.5_
  - [x] 8.2 改进跨节点删除

    - 完善 `DeleteSelection()` 的 DOM 结构清理
    - 确保空元素被正确移除
    - 处理跨多个格式化元素（`<strong>`, `<em>` 等）的删除
    - _Requirements: 4.6, 4.7_
  - [ ]* 8.3 Write property test for backspace deletion
    - **Property 5: Backspace deletion correctness**
    - **Validates: Requirements 4.1**
  - [ ]* 8.4 Write property test for delete key
    - **Property 6: Delete key deletion correctness**
    - **Validates: Requirements 4.2**
  - [ ]* 8.5 Write property test for selection deletion
    - **Property 7: Selection deletion completeness**
    - **Validates: Requirements 4.3**

- [x] 9. Checkpoint - 确保删除功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 5: 格式化功能完善

- [x] 10. 统一格式化实现

  - [x] 10.1 重构格式化方法


    - 创建统一的 `ApplyFormatting(tag_name)` 方法
    - 移除重复的 `ApplyBold`, `ApplyItalic`, `ApplyUnderline` 代码
    - 当前三个方法代码几乎相同，只是标签名不同
    - _Requirements: 5.1, 5.2, 5.3, 12.3_
  - [x] 10.2 实现格式切换

    - 实现 `ToggleFormatting()` 方法
    - 检测当前选择是否已有格式（使用 `QueryCommandState`）
    - 如果已有格式则移除，否则添加
    - _Requirements: 5.4_

  - [x] 10.3 实现跨节点格式化
    - 实现 `ApplyFormattingToRange()` 方法
    - 处理跨多个文本节点的选择
    - 当前 ApplyBold 等方法只处理同一文本节点内的选择
    - _Requirements: 5.5_
  - [ ]* 10.4 Write property test for formatting toggle
    - **Property 8: Formatting toggle idempotence**
    - **Validates: Requirements 5.4**

- [x] 11. Checkpoint - 确保格式化功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 6: 撤销/重做功能完善

- [x] 12. 改进撤销/重做

  - [x] 12.1 改进撤销状态保存


    - 当前 UndoState 只保存 innerHTML 和简单的 offset
    - 需要保存精确的光标位置（节点引用 + 偏移量）
    - 保存 anchor_node 和 focus_node 信息
    - _Requirements: 7.3, 7.4_

  - [x] 12.2 实现操作合并
    - 实现 `BeginUndoGroup()` 和 `EndUndoGroup()`
    - 连续输入合并为一个撤销操作（如连续打字）
    - _Requirements: 7.5_
  - [ ]* 12.3 Write property test for undo/redo round trip
    - **Property 9: Undo/Redo round trip**
    - **Validates: Requirements 7.1, 7.2, 7.3, 7.4**
  - [ ]* 12.4 Write property test for redo stack clearing
    - **Property 10: Redo stack clearing**
    - **Validates: Requirements 7.6**

- [x] 13. Checkpoint - 确保撤销/重做功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 7: 剪贴板功能完善

- [x] 14. 完善剪贴板功能



  - [x] 14.1 整合剪贴板快捷键处理

    - 将 `event_loop.cpp` 中的剪贴板快捷键处理移入 ClipboardManager
    - 确保正确分发 clipboard 事件（copy, cut, paste）
    - _Requirements: 6.5_
  - [ ]* 14.2 Write property test for clipboard round trip
    - **Property 13: Clipboard round trip**
    - **Validates: Requirements 6.1, 6.3**

- [x] 15. Checkpoint - 确保剪贴板功能正常
  - Ensure all tests pass, ask the user if questions arise.

## Phase 8: 事件分发完善

- [x] 16. 完善事件分发

  - [x] 16.1 完善 beforeinput 事件



    - 当前已实现基本的 beforeinput 事件分发
    - 确保所有编辑操作都分发 beforeinput 事件
    - 验证 inputType 值符合规范
    - _Requirements: 10.1, 10.2_
  - [x] 16.2 实现事件取消

    - 当前已实现 beforeinput 事件取消检查
    - 确保 beforeinput 事件被取消时不执行操作
    - _Requirements: 10.3_
  - [ ]* 16.3 Write property test for event cancellation
    - **Property 11: beforeinput event cancellation**
    - **Validates: Requirements 10.3**
  - [ ]* 16.4 Write property test for event bubbling
    - **Property 12: Event bubbling**
    - **Validates: Requirements 10.5**

- [x] 17. Final Checkpoint - 确保所有功能正常
  - Ensure all tests pass, ask the user if questions arise.
