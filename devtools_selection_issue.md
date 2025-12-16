# DevTools 元素选择问题分析

## 问题现象

静态HTML元素能在DOM树中看到并选中，但JavaScript动态添加的元素无法选中。

## 根本原因

**问题不在于 ElementPicker，而在于 DOM 树视图没有显示动态添加的元素！**

### 元素选择的两种方式

1. **DOM树视图点击**（工作正常）
   - 用户在DevTools的DOM树视图中点击元素
   - 触发 `DOMTreeView::HandleMouseEvent`
   - 调用 `SelectNode(hit_node)`
   - 通过回调 `on_selection_changed_` 通知 DevToolsManager

2. **元素选择器（Picker）**（之前有bug，已修复）
   - 用户点击选择器图标
   - 在主应用区域移动鼠标/点击
   - 触发 `ElementPicker::HandleMouseMove/HandleMouseClick`
   - 通过 `HitTest` 找到对应元素

### 为什么动态元素无法选中

虽然 `DevToolsManager::OnNodeAdded` 会调用 `panel_->RefreshDOMTree()`，但可能存在以下问题：

1. **DOM观察者未触发**
   - 动态添加元素时，`document->GetObserverManager().NotifyNodeAdded()` 可能没有被调用
   - 或者 DevToolsManager 没有正确注册为观察者

2. **RefreshDOMTree 实现问题**
   - `panel_->RefreshDOMTree()` 可能没有正确更新从document读取的节点树
   - 需要检查是否重新遍历了document

## 已修复问题

✅ **ElementPicker 的事件处理**
- 修复了 `DevToolsManager::HandleMouseMove` 和 `HandleMouseEvent`
- 现在在picker模式下会正确调用 `picker_->HandleMouseMove/HandleMouseClick`

## 需要进一步调查

❌ **为什么动态元素不在DOM树中显示**

需要检查：
1. JavaScript添加元素时是否调用了 `document->AppendChild()` 等方法
2. 这些方法是否触发了 `NotifyNodeAdded()`
3. `RefreshDOMTree()` 的实现是否正确

## 下一步测试

请在控制台查找：
- `[DOMTreeView]` 相关的调试日志
- 确认点击"GET Request"按钮后，是否看到新元素出现在DOM树中
- 如果DOM树中没有显示，说明问题在DOM更新通知机制
