# Implementation Plan: 增量更新系统优化

## 概述

按优先级从高到低实现 6 个修复点：先修复实际 Bug（SetValue），再补齐延迟路径，最后修复 JS 绑定层缺陷。每个修改点紧跟对应的属性测试，尽早发现回归。

## Tasks

- [x] 1. 修复 HTMLInputElement::SetValue() 缺失脏标记
  - [x] 1.1 在 `core/dom/elements/html_input_element.cpp` 的 SetValue() 中，当 old_value != new_value 时添加 MarkDirty(DirtyType::PAINT) 调用和 Window::SetNeedsRepaint() 通知
    - 在 `value_ = new_value;` 和 selection 调整之后、事件触发之前插入
    - 需要 include `core/window/window.h`（已存在）
    - _Requirements: 1.1, 1.2, 1.3, 1.4_
  - [~]* 1.2 编写属性测试 Property 1：SetValue 脏标记不变量
    - 创建 `tests/property/dom/test_input_dirty_properties.cpp`
    - 生成随机 HTMLInputElement，随机 old_value 和 new_value（确保不同），验证 SetValue 后 IsPaintDirty() 为 true
    - 包含边界情况：相同值不标脏、空字符串、maxlength 截断
    - **Property 1: SetValue 脏标记不变量**
    - **Validates: Requirements 1.1**

- [x] 2. 修复 Node::RemoveAllChildren() DirtyNodeTracker 记录缺失
  - [x] 2.1 在 `core/dom/node.cpp` 的 RemoveAllChildren() 中，为每个子节点调用 `doc->GetDirtyTracker().RecordNodeRemoved(child, shared_from_this(), i)`
    - 在现有 ObserverManager 通知循环中增加 DirtyNodeTracker 记录
    - 使用索引 i 记录每个子节点的原始位置
    - _Requirements: 2.1, 2.2_
  - [~]* 2.2 编写属性测试 Property 2：RemoveAllChildren 移除记录计数不变量
    - 创建 `tests/property/dom/test_dirty_tracking_properties.cpp`
    - 生成随机 Document + Element + N 个子节点，调用 RemoveAllChildren，验证 tracker 中恰好 N 条 Removed 记录
    - **Property 2: RemoveAllChildren 移除记录计数不变量**
    - **Validates: Requirements 2.1**
  - [~]* 2.3 编写属性测试 Property 3：SetTextContent 追踪器记录完整性
    - 在同一测试文件中添加
    - 生成随机 Element（含 N ≥ 2 个子节点），调用 SetTextContent(随机文本)，验证 N 条 Removed + 1 条 Added
    - **Property 3: SetTextContent 追踪器记录完整性**
    - **Validates: Requirements 2.3**
  - [~]* 2.4 编写属性测试 Property 4：Optimize 取消 Add+Remove 对称操作
    - 在同一测试文件中添加
    - 生成随机 Node，AppendChild 后 RemoveAllChildren，调用 Optimize，验证该子节点的记录被取消
    - **Property 4: Optimize 取消 Add+Remove 对称操作**
    - **Validates: Requirements 2.4**

- [x] 3. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，ask the user if questions arise.

- [x] 4. Element::SetStyle() 增加 DirtyNodeTracker 记录
  - [x] 4.1 在 `core/dom/element.cpp` 的 SetStyle() 中，ObserverManager 通知之前增加 `doc->GetDirtyTracker().RecordStyleChanged()` 调用
    - 合并两处 GetOwnerDocument() 调用为一次，避免重复
    - _Requirements: 3.1, 3.4_
  - [~]* 4.2 编写属性测试 Property 5：SetStyle 延迟路径记录
    - 在 `tests/property/dom/test_dirty_tracking_properties.cpp` 中添加
    - 生成随机 Element + 随机 CSS 属性/值，调用 SetStyle，验证 tracker 中有对应 StyleChange 记录
    - **Property 5: SetStyle 延迟路径记录**
    - **Validates: Requirements 3.1**
  - [~]* 4.3 编写属性测试 Property 6：Optimize 样式变化合并不变量
    - 在同一测试文件中添加
    - 生成随机 Element + K 个不同属性各调用 M 次 SetStyle，Optimize 后验证恰好 K 条记录
    - **Property 6: Optimize 样式变化合并不变量**
    - **Validates: Requirements 3.2, 3.3**

- [x] 5. Element::SetAttribute() 和 RemoveAttribute() 增加 DirtyNodeTracker 记录
  - [x] 5.1 在 `core/dom/element.cpp` 的 SetAttribute() 中，ObserverManager 通知之前增加 `doc->GetDirtyTracker().RecordStyleChanged()` 调用
    - 仅在属性值实际变化时记录（已有 early return 检查）
    - _Requirements: 4.1, 4.4_
  - [x] 5.2 在 `core/dom/element.cpp` 的 RemoveAttribute() 中，增加 DirtyNodeTracker 记录
    - 将 GetOwnerDocument() 提前，在 ObserverManager 通知之前记录
    - new_value 为空字符串
    - _Requirements: 4.2, 4.4_
  - [~]* 5.3 编写属性测试 Property 7 和 Property 8：SetAttribute/RemoveAttribute 延迟路径记录
    - 在 `tests/property/dom/test_dirty_tracking_properties.cpp` 中添加
    - Property 7：随机属性名/值，SetAttribute 后验证 tracker 记录
    - Property 8：先 SetAttribute 设置属性，再 RemoveAttribute，验证 tracker 记录 new_value 为空
    - **Property 7: SetAttribute 延迟路径记录**
    - **Property 8: RemoveAttribute 延迟路径记录**
    - **Validates: Requirements 4.1, 4.2**

- [x] 6. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，ask the user if questions arise.

- [x] 7. 修复 JS classList 子串误匹配
  - [x] 7.1 在 `core/quickjs/bindings/js_element.cpp` 中提取 `HasExactClass()` 和 `RemoveExactClass()` 辅助函数
    - 使用 istringstream 按空格分词后精确匹配
    - 与 Element::HasClass() / Element::RemoveClass() 的 C++ 实现保持一致
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5_
  - [x] 7.2 将 classList 的 add/remove/toggle/contains 四个 lambda 中的 `string::find()` 替换为辅助函数调用
    - add：`HasExactClass()` 检查是否已存在
    - remove：`RemoveExactClass()` 精确移除
    - toggle：`HasExactClass()` 判断 + 对应 add/remove 逻辑
    - contains：`HasExactClass()` 精确判断
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5_
  - [~]* 7.3 编写属性测试 Property 9：classList 精确匹配一致性
    - 创建 `tests/property/dom/test_classlist_properties.cpp`
    - 生成随机 class 列表（含易混淆的前缀/后缀 class 如 "btn"/"btn-primary"/"btn-lg"）
    - 验证 add/remove/toggle/contains 与参考实现一致
    - **Property 9: classList 精确匹配一致性**
    - **Validates: Requirements 5.1, 5.2, 5.3, 5.4, 5.5**

- [x] 8. 更新 CMakeLists.txt 和最终验证
  - [x] 8.1 更新 `tests/property/CMakeLists.txt` 添加新的 dom 属性测试源文件
    - 添加 `dom/test_dirty_tracking_properties.cpp`
    - 添加 `dom/test_input_dirty_properties.cpp`
    - 添加 `dom/test_classlist_properties.cpp`
    - _Requirements: all_

- [x] 9. Final checkpoint - 确保所有测试通过
  - 确保所有测试通过，ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- 修改顺序按优先级排列：Bug 修复 → 延迟路径补齐 → JS 绑定修复
- 每个修改点紧跟属性测试，尽早发现回归
- RenderTreeSynchronizer 无需额外修改，补齐 DirtyNodeTracker 记录后自动生效
- Property tests 使用 Google Test + 手动随机生成器，与项目现有模式一致
