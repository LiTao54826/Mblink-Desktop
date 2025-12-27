# Implementation Plan

## 1. Range API 实现

- [x] 1.1 创建 Range 类核心结构
  - 创建 `core/dom/range.h` 和 `core/dom/range.cpp`
  - 实现 Range 构造函数和基本属性（startContainer, endContainer, startOffset, endOffset, collapsed）
  - 实现 SetStart/SetEnd 方法
  - _Requirements: 2.2, 2.3, 2.14, 2.15, 2.16, 2.17, 2.18_

- [ ]* 1.2 编写 Range 边界一致性属性测试
  - **Property 5: Range 边界一致性**
  - **Validates: Requirements 2.2, 2.3, 2.14, 2.15, 2.16, 2.17, 2.18**

- [x] 1.3 实现 Range 相对定位方法
  - 实现 SetStartBefore/SetStartAfter/SetEndBefore/SetEndAfter
  - 实现 GetNodeIndex 辅助方法
  - _Requirements: 2.4, 2.5, 2.6, 2.7_

- [ ]* 1.4 编写 Range 相对定位属性测试
  - **Property 6: Range 相对定位正确性**
  - **Validates: Requirements 2.4, 2.5, 2.6, 2.7**

- [x] 1.5 实现 Range 选择方法
  - 实现 SelectNode 和 SelectNodeContents
  - 实现 Collapse 方法
  - _Requirements: 2.8, 2.9, 2.10_

- [ ]* 1.6 编写 Range 选择方法属性测试
  - **Property 7: Range selectNode 完整选择**
  - **Property 8: Range selectNodeContents 内容选择**
  - **Property 9: Range collapse 位置正确性**
  - **Validates: Requirements 2.8, 2.9, 2.10**

- [x] 1.7 实现 Range 克隆和转换方法
  - 实现 CloneRange 方法
  - 实现 ToString 方法
  - 实现 GetCommonAncestorContainer 方法
  - _Requirements: 2.11, 2.12, 2.13_

- [ ]* 1.8 编写 Range 克隆和转换属性测试
  - **Property 10: Range cloneRange 独立副本**
  - **Property 11: Range toString 文本提取**
  - **Property 12: Range commonAncestorContainer 正确性**
  - **Validates: Requirements 2.11, 2.12, 2.13**

- [x] 1.9 创建 Range 的 QuickJS 绑定
  - 创建 `core/quickjs/bindings/js_range.h` 和 `js_range.cpp`
  - 绑定所有 Range 属性和方法
  - 在 Document 上实现 createRange() 方法
  - _Requirements: 2.1_

## 2. Selection API 实现

- [x] 2.1 创建 Selection 类核心结构
  - 创建 `core/dom/selection.h` 和 `core/dom/selection.cpp`
  - 实现 Selection 构造函数和基本属性（anchorNode, focusNode, anchorOffset, focusOffset, isCollapsed, rangeCount）
  - _Requirements: 1.2, 1.3, 1.4, 1.5, 1.6_

- [ ]* 2.2 编写 Selection 状态一致性属性测试
  - **Property 1: Selection 状态一致性**
  - **Validates: Requirements 1.2, 1.3, 1.4, 1.5, 1.6**

- [x] 2.3 实现 Selection 操作方法
  - 实现 Collapse 方法
  - 实现 Extend 方法
  - 实现 SelectAllChildren 方法
  - _Requirements: 1.7, 1.8, 1.9_

- [ ]* 2.4 编写 Selection 操作方法属性测试
  - **Property 2: Selection collapse 操作正确性**
  - **Property 3: Selection extend 操作保持锚点**
  - **Validates: Requirements 1.7, 1.8, 1.9**

- [x] 2.5 实现 Selection Range 管理
  - 实现 RemoveAllRanges 方法
  - 实现 AddRange 方法
  - 实现 GetRangeAt 方法
  - 实现 ToString 方法
  - _Requirements: 1.10, 1.11, 1.12, 1.13_

- [ ]* 2.6 编写 Selection toString 属性测试
  - **Property 4: Selection toString 返回选中文本**
  - **Validates: Requirements 1.13**

- [x] 2.7 创建 SelectionManager 类
  - 创建 `core/event/selection_manager.h` 和 `selection_manager.cpp`
  - 实现 GetSelection 方法
  - 实现 HitTestToCaretPosition 方法
  - _Requirements: 1.1_

- [x] 2.8 创建 Selection 的 QuickJS 绑定
  - 创建 `core/quickjs/bindings/js_selection.h` 和 `js_selection.cpp`
  - 绑定所有 Selection 属性和方法
  - 在 Window 上实现 getSelection() 方法
  - _Requirements: 1.1_

- [ ] 2.9 Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## 3. MutationObserver 实现

- [x] 3.1 创建 MutationObserver 核心结构
  - 创建 `core/dom/mutation_observer.h` 和 `mutation_observer.cpp`
  - 定义 MutationRecord 和 MutationObserverInit 结构
  - 实现 MutationObserver 构造函数
  - _Requirements: 3.1, 3.13_

- [x] 3.2 实现 MutationObserver 观察功能
  - 实现 Observe 方法
  - 实现 Disconnect 方法
  - 实现 TakeRecords 方法
  - _Requirements: 3.2, 3.11, 3.12_

- [ ]* 3.3 编写 MutationObserver disconnect 和 takeRecords 属性测试
  - **Property 16: MutationObserver disconnect 停止观察**
  - **Property 17: MutationObserver takeRecords 清空队列**
  - **Validates: Requirements 3.11, 3.12**

- [x] 3.4 实现 MutationObserver 过滤逻辑
  - 实现 childList 选项支持
  - 实现 attributes 选项支持
  - 实现 characterData 选项支持
  - 实现 attributeFilter 选项支持
  - _Requirements: 3.3, 3.4, 3.5, 3.9_

- [ ]* 3.5 编写 MutationObserver 过滤属性测试
  - **Property 13: MutationObserver 过滤正确性**
  - **Validates: Requirements 3.3, 3.4, 3.5, 3.9**

- [x] 3.6 实现 MutationObserver subtree 和 oldValue 支持
  - 实现 subtree 选项支持
  - 实现 attributeOldValue 选项支持
  - 实现 characterDataOldValue 选项支持
  - _Requirements: 3.6, 3.7, 3.8_

- [ ]* 3.7 编写 MutationObserver subtree 和 oldValue 属性测试
  - **Property 14: MutationObserver subtree 递归观察**
  - **Property 15: MutationObserver oldValue 记录**
  - **Validates: Requirements 3.6, 3.7, 3.8**

- [x] 3.8 实现 MutationObserver 回调调度
  - 实现 ScheduleCallback 方法
  - 实现 FlushRecords 方法
  - 集成到事件循环
  - _Requirements: 3.10_

- [x] 3.9 创建 MutationObserver 的 QuickJS 绑定
  - 创建 `core/quickjs/bindings/js_mutation_observer.h` 和 `js_mutation_observer.cpp`
  - 绑定 MutationObserver 构造函数和方法
  - 绑定 MutationRecord 属性
  - _Requirements: 3.1, 3.2, 3.13_

- [ ] 3.10 Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## 4. ContentEditable 实现

- [x] 4.1 实现 contenteditable 属性支持
  - 在 Element 类中添加 contenteditable 属性解析
  - 实现 IsContentEditable 方法（包含继承逻辑）
  - _Requirements: 4.1, 4.2, 4.3, 4.13_

- [ ]* 4.2 编写 contenteditable 继承属性测试
  - **Property 18: contenteditable 继承正确性**
  - **Validates: Requirements 4.1, 4.2, 4.3, 4.13**

- [x] 4.3 创建 ContentEditableHandler 类
  - 创建 `core/event/contenteditable_handler.h` 和 `contenteditable_handler.cpp`
  - 实现 IsEditable 和 IsContentEditable 方法
  - _Requirements: 4.1, 4.2, 4.3_

- [x] 4.4 实现文本输入处理
  - 实现 HandleTextInput 方法
  - 实现 InsertText 方法
  - 集成 SelectionManager 获取光标位置
  - _Requirements: 4.5_

- [ ]* 4.5 编写文本插入属性测试
  - **Property 19: 文本插入位置正确性**
  - **Validates: Requirements 4.5**

- [x] 4.6 实现删除操作
  - 实现 DeleteSelection 方法
  - 实现 DeleteCharacter 方法（支持 forward 参数）
  - 处理 Backspace 和 Delete 键
  - _Requirements: 4.6, 4.7_

- [ ]* 4.7 编写删除操作属性测试
  - **Property 20: Backspace 删除正确性**
  - **Property 21: Delete 删除正确性**
  - **Validates: Requirements 4.6, 4.7**

- [x] 4.8 实现换行处理
  - 实现 InsertLineBreak 方法
  - 处理 Enter 键
  - _Requirements: 4.8_

- [x] 4.9 实现鼠标选择支持
  - 在 SelectionManager 中实现 HandleMouseDown/Move/Up
  - 更新 Selection 状态
  - _Requirements: 4.4, 4.9_

- [x] 4.10 实现键盘选择支持
  - 实现 HandleShiftArrow 方法
  - 支持 Shift+Arrow 键扩展选择
  - _Requirements: 4.10_

- [x] 4.11 实现光标渲染
  - 实现 RenderCaret 方法
  - 处理焦点获取/失去时的光标显示
  - _Requirements: 4.11, 4.12_

- [ ] 4.12 Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## 5. Input 事件实现

- [x] 5.1 实现 beforeinput 事件
  - 实现 DispatchBeforeInputEvent 方法（已在 ContentEditableHandler 中实现）
  - 支持事件取消
  - _Requirements: 6.1, 6.3_

- [ ]* 5.2 编写 beforeinput 事件可取消性属性测试
  - **Property 22: beforeinput 事件可取消性**
  - **Validates: Requirements 6.1, 6.3**

- [x] 5.3 实现 input 事件
  - 实现 DispatchInputEvent 方法（已在 ContentEditableHandler 中实现）
  - _Requirements: 6.2_

- [x] 5.4 创建 InputEvent 类
  - 在 event.h/event.cpp 中添加 InputEvent 类（继承自 Event）
  - 添加 inputType 和 data 属性
  - 更新 ContentEditableHandler 使用 InputEvent
  - _Requirements: 6.4, 6.5_

- [ ]* 5.5 编写 input 事件数据属性测试
  - **Property 23: input 事件数据正确性**
  - **Validates: Requirements 6.2, 6.4, 6.5**

- [x] 5.6 创建 InputEvent 的 QuickJS 绑定
  - 在 js_event.cpp 中添加 InputEvent 属性绑定（inputType, data, isComposing）
  - 确保事件可以被 JavaScript 监听和取消
  - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

## 6. Clipboard 实现

- [x] 6.1 创建 ClipboardManager 类
  - 创建 `core/event/clipboard_manager.h` 和 `clipboard_manager.cpp`
  - 实现系统剪贴板访问（GetText/SetText）
  - _Requirements: 5.1, 5.2, 5.3_

- [x] 6.2 实现 Copy/Cut/Paste 操作
  - 实现 Copy 方法
  - 实现 Cut 方法
  - 实现 Paste 方法
  - 集成 SelectionManager 和 ContentEditableHandler
  - _Requirements: 5.1, 5.2, 5.3, 5.4_

- [x] 6.3 实现剪贴板事件
  - 实现 DispatchClipboardEvent 方法
  - 使用 Event 类分发 copy/cut/paste 事件
  - 支持事件取消
  - _Requirements: 5.5, 5.6, 5.7_

- [ ]* 6.4 编写 Clipboard 事件结构属性测试
  - **Property 26: Clipboard 事件结构**
  - **Validates: Requirements 5.5, 5.6, 5.7**

- [x] 6.5 处理键盘快捷键
  - 处理 Ctrl+C/Cmd+C（复制）
  - 处理 Ctrl+X/Cmd+X（剪切）
  - 处理 Ctrl+V/Cmd+V（粘贴）
  - _Requirements: 5.1, 5.2, 5.3_

- [x] 6.6 创建 ClipboardEvent 的 QuickJS 绑定
  - 在 event.h/event.cpp 中添加 ClipboardEvent 类
  - 在 js_event.cpp 中绑定 clipboardData 属性（包含 getData/setData 方法）
  - 更新 ClipboardManager 使用 ClipboardEvent
  - _Requirements: 5.5, 5.6, 5.7_

- [ ] 6.7 Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## 7. execCommand 实现

- [x] 7.1 实现格式化命令
  - 实现 ApplyBold 方法
  - 实现 ApplyItalic 方法
  - 实现 ApplyUnderline 方法
  - _Requirements: 7.1, 7.2, 7.3_

- [ ]* 7.2 编写 execCommand 格式切换属性测试
  - **Property 24: execCommand 格式切换**
  - **Validates: Requirements 7.1, 7.2, 7.3, 7.7**

- [x] 7.3 实现文本操作命令
  - 实现 insertText 命令（已实现）
  - 实现 delete 命令（已实现）
  - 实现 selectAll 命令（已实现）
  - _Requirements: 7.4, 7.5, 7.6_

- [ ]* 7.4 编写 execCommand insertText 属性测试
  - **Property 25: execCommand insertText 正确性**
  - **Validates: Requirements 7.4**

- [x] 7.5 完善命令状态查询
  - 完善 QueryCommandState 方法
  - 实现格式状态检测逻辑（检查祖先元素的标签）
  - _Requirements: 7.7_

- [x] 7.6 实现 QueryCommandEnabled 方法
  - 已实现基本的命令可用性检查
  - _Requirements: 7.8_

- [x] 7.7 创建 execCommand 的 QuickJS 绑定
  - 将 SelectionManager、ContentEditableHandler、ClipboardManager 集成到 EventLoop
  - 在 DOMBindings 中添加 SetGlobalEventLoop/GetGlobalEventLoop
  - 在 Document 上绑定 execCommand 方法
  - 绑定 queryCommandState 方法
  - 绑定 queryCommandEnabled 方法
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7, 7.8_

## 8. 最终集成和测试

- [x] 8.1 集成所有组件
  - 确保 SelectionManager、ContentEditableHandler、ClipboardManager 正确协作
  - 在所有加载器（html_loader, esm_loader, app_loader）中添加 SetGlobalEventLoop 调用
  - 验证事件流程完整性
  - _Requirements: All_

- [ ]* 8.2 编写集成测试
  - 测试完整的编辑流程
  - 测试 JavaScript API 调用
  - _Requirements: All_

- [ ] 8.3 Final Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.
