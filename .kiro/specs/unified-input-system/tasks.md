# 统一输入管理系统 - 实现任务列表

## 实现计划

- [ ] 1. 创建基础架构和核心类型
  - [ ] 1.1 创建 `core/input/` 目录结构和 CMakeLists.txt
    - 创建目录 `core/input/`
    - 创建 `CMakeLists.txt` 配置文件
    - 创建 `README.md` 说明文档
    - _Requirements: 所有需求的基础_

  - [ ] 1.2 实现 Position 类
    - 创建 `position.h` 和 `position.cpp`
    - 实现 `PositionAnchorType` 枚举
    - 实现 `Position` 类的构造函数和工厂方法
    - 实现 `ComputeContainerNode()` 和 `ComputeOffsetInContainerNode()`
    - 实现比较运算符
    - _Requirements: 2.1, 2.2, 2.3_

  - [ ] 1.3 编写 Position 类的属性测试
    - **Property 5: 点击定位准确性**
    - **Validates: Requirements 2.1**

  - [ ] 1.4 实现 SelectionInDOMTree 结构
    - 在 `position.h` 中定义 `SelectionInDOMTree`
    - 实现 `IsCollapsed()`, `IsNone()`, `Start()`, `End()` 方法
    - _Requirements: 2.1, 2.2_

- [ ] 2. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 3. 实现 FocusController (重构自 FocusManager)
  - [ ] 3.1 创建 FocusController 基础结构
    - 创建 `focus_controller.h` 和 `focus_controller.cpp`
    - 定义 `FocusType` 枚举和 `FocusParams` 结构
    - 从 `FocusManager` 复用焦点设置逻辑
    - _Requirements: 1.1, 1.2, 1.3, 1.4_

  - [ ] 3.2 实现焦点设置和清除
    - 实现 `SetFocusedElement()` 方法
    - 实现 `GetFocusedElement()` 方法
    - 实现 `ClearFocus()` 方法
    - 实现焦点事件触发 (focus, blur, focusin, focusout)
    - _Requirements: 1.1, 1.2, 1.3, 1.4_

  - [ ] 3.3 编写焦点唯一性属性测试
    - **Property 1: 焦点唯一性**
    - **Validates: Requirements 1.1, 1.2**

  - [ ] 3.4 编写焦点转移一致性属性测试
    - **Property 2: 焦点转移一致性**
    - **Validates: Requirements 1.3, 1.4**

  - [ ] 3.5 实现 Tab 导航
    - 实现 `AdvanceFocus()` 方法
    - 从 `FocusManager` 复用 `CollectFocusableElements()` 逻辑
    - 从 `FocusManager` 复用 `GetTabIndex()` 逻辑
    - _Requirements: 1.5_

  - [ ] 3.6 编写 Tab 导航顺序属性测试
    - **Property 3: Tab 导航顺序**
    - **Validates: Requirements 1.5**

  - [ ] 3.7 实现 DOMObserver 接口
    - 实现 `OnNodeRemoved()` 处理焦点元素移除
    - _Requirements: 1.6_

  - [ ] 3.8 编写焦点元素移除属性测试
    - **Property 4: 焦点元素移除**
    - **Validates: Requirements 1.6**

  - [ ] 3.9 实现 FocusChangedObserver 机制
    - 定义 `FocusChangedObserver` 接口
    - 实现 `RegisterFocusChangedObserver()` 方法
    - 实现焦点变化通知
    - _Requirements: 1.1, 1.3_

- [ ] 4. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 5. 实现 FrameCaret (光标控制器)
  - [ ] 5.1 创建 FrameCaret 基础结构
    - 创建 `frame_caret.h` 和 `frame_caret.cpp`
    - 定义 `CaretShape` 枚举
    - 从 `SelectionManager` 复用光标闪烁逻辑
    - _Requirements: 5.1, 5.2, 5.3, 5.4_

  - [ ] 5.2 实现光标状态管理
    - 实现 `SetCaretEnabled()` 方法
    - 实现 `SetCaretBlinkingSuspended()` 方法
    - 实现 `StartBlinkCaret()` 和 `StopCaretBlinkTimer()` 方法
    - 实现 `CaretBlinkTimerFired()` 定时器回调
    - _Requirements: 5.1, 5.2, 5.3, 5.4_

  - [ ] 5.3 编写光标可见性属性测试
    - **Property 13: 光标可见性**
    - **Validates: Requirements 5.1, 5.2**

  - [ ] 5.4 实现光标渲染
    - 实现 `AbsoluteCaretBounds()` 方法
    - 实现 `PaintCaret()` 方法
    - 从 `SelectionManager` 复用光标渲染逻辑
    - _Requirements: 5.1, 5.4_

- [ ] 6. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.


- [ ] 7. 实现 FrameSelection (选区管理器)
  - [ ] 7.1 创建 FrameSelection 基础结构
    - 创建 `frame_selection.h` 和 `frame_selection.cpp`
    - 定义 `TextGranularity`, `SelectionModifyAlteration`, `SelectionModifyDirection` 枚举
    - 定义 `SetSelectionOptions` 结构
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6_

  - [ ] 7.2 实现选区状态管理
    - 实现 `SetSelection()` 方法
    - 实现 `GetSelection()` 方法
    - 实现 `Clear()` 方法
    - 实现 `HasSelection()` 方法
    - 从 `SelectionManager` 复用选区管理逻辑
    - _Requirements: 2.1, 2.2_

  - [ ] 7.3 编写拖动选区连续性属性测试
    - **Property 6: 拖动选区连续性**
    - **Validates: Requirements 2.2**

  - [ ] 7.4 实现选区修改
    - 实现 `Modify()` 方法 (Move/Extend, 各方向, 各粒度)
    - 实现 `SelectAll()` 方法
    - _Requirements: 2.3, 2.4_

  - [ ] 7.5 编写选区扩展正确性属性测试
    - **Property 7: 选区扩展正确性**
    - **Validates: Requirements 2.3**

  - [ ] 7.6 实现选区查询
    - 实现 `SelectedText()` 方法
    - 实现 `SelectionHasFocus()` 方法
    - _Requirements: 2.1_

  - [ ] 7.7 集成 FrameCaret
    - 在 FrameSelection 中创建和管理 FrameCaret
    - 实现 `AbsoluteCaretBounds()` 委托
    - 实现 `SetCaretBlinkingSuspended()` 委托
    - 实现 `PaintCaret()` 委托
    - _Requirements: 5.1, 5.2, 5.5_

  - [ ] 7.8 编写选区与光标互斥属性测试
    - **Property 14: 选区与光标互斥**
    - **Validates: Requirements 5.5**

- [ ] 8. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 9. 实现 SelectionController (选择控制器)
  - [ ] 9.1 创建 SelectionController 基础结构
    - 创建 `selection_controller.h` 和 `selection_controller.cpp`
    - 定义 `SelectionState` 枚举
    - _Requirements: 2.1, 2.2, 2.5, 2.6_

  - [ ] 9.2 实现鼠标事件处理
    - 实现 `HandleMousePressEvent()` 方法
    - 实现 `HandleMouseDraggedEvent()` 方法
    - 实现 `HandleMouseReleaseEvent()` 方法
    - 从 `SelectionManager` 复用鼠标处理逻辑
    - _Requirements: 2.1, 2.2_

  - [ ] 9.3 实现单击/双击/三击处理
    - 实现 `HandleSingleClick()` - 定位光标
    - 实现 `HandleDoubleClick()` - 选中单词
    - 实现 `HandleTripleClick()` - 选中行/段落
    - _Requirements: 2.1, 2.5, 2.6_

  - [ ] 9.4 编写双击选词边界属性测试
    - **Property 8: 双击选词边界**
    - **Validates: Requirements 2.5**

  - [ ] 9.5 实现拖动选择
    - 实现 `UpdateSelectionForMouseDrag()` 方法
    - 实现 `SetMouseDownMayStartSelect()` 方法
    - _Requirements: 2.2_

- [ ] 10. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 11. 实现 EditContext (编辑上下文)
  - [ ] 11.1 创建 EditContext 基础结构
    - 创建 `edit_context.h` 和 `edit_context.cpp`
    - 定义 `DeleteMode` 和 `DeleteDirection` 枚举
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6_

  - [ ] 11.2 实现编辑能力查询
    - 实现 `CanEdit()`, `CanEditRichly()` 方法
    - 实现 `CanCut()`, `CanCopy()`, `CanPaste()`, `CanDelete()` 方法
    - _Requirements: 3.1, 3.2, 3.3_

  - [ ] 11.3 实现文本编辑操作
    - 实现 `InsertText()` 方法
    - 实现 `DeleteSelection()` 方法
    - 实现 `InsertLineBreak()` 方法
    - 实现 `InsertParagraphSeparator()` 方法
    - 从 `ContentEditableController` 复用编辑逻辑
    - _Requirements: 3.1, 3.2, 3.3_

  - [ ] 11.4 编写文本插入位置属性测试
    - **Property 9: 文本插入位置**
    - **Validates: Requirements 3.1**

  - [ ] 11.5 编写删除操作正确性属性测试
    - **Property 10: 删除操作正确性**
    - **Validates: Requirements 3.2, 3.3**

  - [ ] 11.6 实现剪贴板操作
    - 实现 `Cut()` 方法
    - 实现 `Copy()` 方法
    - 实现 `Paste()` 方法
    - 从 `ClipboardManager` 复用剪贴板逻辑
    - _Requirements: 3.4, 3.5_

  - [ ] 11.7 编写剪贴板往返一致性属性测试
    - **Property 11: 剪贴板往返一致性**
    - **Validates: Requirements 3.4, 3.5**

  - [ ] 11.8 实现 IME 支持
    - 实现 `StartComposition()` 方法
    - 实现 `UpdateComposition()` 方法
    - 实现 `CommitComposition()` 方法
    - 实现 `CancelComposition()` 方法
    - _Requirements: 3.6_

  - [ ] 11.9 编写 IME 组合状态机属性测试
    - **Property 12: IME 组合状态机**
    - **Validates: Requirements 3.6**

  - [ ] 11.10 实现撤销/重做
    - 创建 `UndoStack` 类
    - 实现 `CanUndo()`, `CanRedo()` 方法
    - 实现 `Undo()`, `Redo()` 方法
    - _Requirements: 3.1, 3.2, 3.3_

- [ ] 12. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 13. 实现 Editable 接口
  - [ ] 13.1 创建 Editable 接口定义
    - 创建 `editable.h`
    - 定义 `Editable` 抽象类
    - 定义 `CaretRect` 结构
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

  - [ ] 13.2 为 HTMLInputElement 实现 Editable 接口
    - 在 `HTMLInputElement` 中实现 `Editable` 接口方法
    - _Requirements: 4.1_

  - [ ] 13.3 为 HTMLTextAreaElement 实现 Editable 接口
    - 在 `HTMLTextAreaElement` 中实现 `Editable` 接口方法
    - _Requirements: 4.2_

- [ ] 14. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 15. 实现 InputController (统一入口)
  - [ ] 15.1 创建 InputController 基础结构
    - 创建 `input_controller.h` 和 `input_controller.cpp`
    - 定义 `WebInputEventResult` 枚举
    - 集成 FocusController, SelectionController, EditContext
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

  - [ ] 15.2 实现鼠标事件处理
    - 实现 `HandleMousePressEvent()` 方法
    - 实现 `HandleMouseMoveEvent()` 方法
    - 实现 `HandleMouseReleaseEvent()` 方法
    - 实现 Hit Testing 集成
    - _Requirements: 6.2_

  - [ ] 15.3 实现键盘事件处理
    - 实现 `HandleKeyboardEvent()` 方法
    - 创建 `KeyboardEventManager` 类
    - _Requirements: 6.3_

  - [ ] 15.4 实现文本输入和 IME 事件处理
    - 实现 `HandleTextInputEvent()` 方法
    - 实现 `HandleIMEEvent()` 方法
    - _Requirements: 6.4_

  - [ ] 15.5 实现帧更新
    - 实现 `Update()` 方法
    - 集成光标闪烁更新
    - _Requirements: 5.3, 5.4_

- [ ] 16. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 17. 集成到 Window 和验证 API 兼容性
  - [ ] 17.1 修改 Window 使用 InputController
    - 在 `Window` 中创建 `InputController` 实例
    - 将事件处理委托给 `InputController`
    - _Requirements: 6.1_

  - [ ] 17.2 验证 JavaScript API 兼容性
    - 验证 `element.focus()` 行为
    - 验证 `element.blur()` 行为
    - 验证 `document.activeElement` 行为
    - 验证 Selection API 行为
    - 验证 `selectionStart/selectionEnd` 行为
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6_

  - [ ] 17.3 编写 API 兼容性属性测试
    - **Property 15: API 兼容性**
    - **Validates: Requirements 7.1, 7.2, 7.3, 7.4, 7.5, 7.6**

- [ ] 18. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 19. 渐进式迁移支持
  - [ ] 19.1 实现迁移兼容层
    - 确保新旧系统可以并存
    - 实现元素级别的迁移开关
    - _Requirements: 8.1, 8.2, 8.3, 8.4_

  - [ ] 19.2 编写渐进式迁移隔离属性测试
    - **Property 16: 渐进式迁移隔离**
    - **Validates: Requirements 8.2, 8.4**

- [ ] 20. 清理和文档
  - [ ] 20.1 更新 README 文档
    - 更新 `core/input/README.md`
    - 添加架构说明和使用示例
    - _Requirements: 所有需求_

  - [ ] 20.2 添加代码注释
    - 为所有公共 API 添加 Doxygen 注释
    - _Requirements: 所有需求_

- [ ] 21. Final Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.
