# CodeMirror selectAllChildren 选择不完整问题

## 问题描述

### 现象
调用 `selection.selectAllChildren(contentDOM)` 后，CodeMirror 的选择范围不正确：
- 预期：`from=0, to=20`（全选）
- 实际：`from=0, to=14`（缺少最后一行）

### 错误信息
最初还伴随错误：
```
Error: Calls to EditorView.update are not allowed while an update is in progress
```

## 排查过程

### 1. 初步分析 - selectionchange 事件时序（已排除）

**假设**：`selectionchange` 事件同步触发导致 CodeMirror 嵌套更新

**验证**：将 `selectionchange` 改为微任务延迟触发

**结果**：解决了嵌套更新错误，但选择范围仍然不正确

### 2. ResolveElementPosition 解析错误（已排除）

**假设**：`selectAllChildren` 设置的 Element 节点位置没有正确解析为 Text 节点位置

**验证**：添加调试日志，检查 `ResolveElementPosition` 的输出

**结果**：解析正确，`focus_offset_=3` 正确解析为 `resolved_focus: offset=6`

### 3. maxOffset 函数兼容性（已排除）

**假设**：CodeMirror 的 `maxOffset(node)` 函数无法正确获取我们 Text 节点的长度

**验证**：
```javascript
function maxOffset(node) {
    return node.nodeType == 3 ? node.nodeValue.length : node.childNodes.length;
}
```

**结果**：`maxOffset` 正确返回 6，`nodeValue.length` 正确暴露

### 4. observer.selectionRange 缓存问题（关键线索）

**发现**：
```
[After selectAllChildren]
  DOM Selection focusOffset: 6              <-- 正确！
  observer.selectionRange.focusOffset: 0    <-- 错误！缓存是旧值
```

CodeMirror 的 `observer.selectionRange` 没有更新，导致后续处理使用了错误的值。

### 5. DOMChange 中的 contains 检查（根本原因）

**关键代码** (`codemirror6.bundle.js` 第 7383-7384 行)：
```javascript
let head = !contains(view.contentDOM, domSel.focusNode) ? 
           view.state.selection.main.head :  // 使用内部状态
           view.docView.posFromDOM(domSel.focusNode, domSel.focusOffset);  // 从 DOM 计算
```

**验证**：
```javascript
contentDOM.contains(sel.focusNode)  // 返回 false！应该返回 true
```

**原因**：`Element.contains()` 的 JS 绑定只能处理 `Element` 类型，无法处理 `Text` 节点

## 根本原因

`core/quickjs/bindings/js_element.cpp` 中的 `JSElement_contains` 函数：

```cpp
// 错误的实现
auto other = UnwrapElement(ctx, argv[0]);  // 只能解包 Element
if (!other) return JS_FALSE;  // Text 节点返回 false
```

当 `sel.focusNode` 是 Text 节点时，`contains()` 总是返回 `false`，导致 CodeMirror 认为选择不在编辑器内，使用内部缓存的旧值（offset=0）。

## 解决方案

修改 `JSElement_contains` 使用 `UnwrapNode` 支持任意节点类型：

```cpp
// 修复后的实现
static JSValue JSElement_contains(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto* data = static_cast<JSElementData*>(JS_GetOpaque(this_val, js_element_class_id));
    if (!data || !data->element) return JS_FALSE;
    
    if (argc < 1) return JS_FALSE;
    
    // 使用 UnwrapNode 支持 Element 和 Text 节点
    auto other = UnwrapNode(ctx, argv[0]);
    if (!other) return JS_FALSE;
    
    // 使用 Node::Contains 方法
    return JS_NewBool(ctx, data->element->Contains(other));
}
```

## 修改的文件

1. `core/quickjs/bindings/js_element.cpp` - 修复 `JSElement_contains` 函数
2. `core/dom/selection/selection.cpp` - 添加微任务延迟触发 selectionchange（解决嵌套更新问题）
3. `core/event/loop/event_loop.cpp` - 添加全局 TaskScheduler 微任务处理

## 经验总结

### 排查思路

1. **从错误信息入手**：先解决明显的错误（嵌套更新），再处理功能问题
2. **添加调试日志**：在关键路径添加日志，观察数据流
3. **对比预期和实际**：明确每一步的预期值和实际值
4. **二分法定位**：逐步缩小问题范围

### 关键检查点

当 JS 绑定的 DOM API 行为异常时，检查：
1. **参数类型支持**：是否支持所有 Node 类型（Element、Text、Comment 等）
2. **返回值类型**：是否正确包装为 JS 对象
3. **节点身份**：多次获取同一节点是否返回同一 JS 对象

### CodeMirror 集成注意事项

1. CodeMirror 依赖 `contains()` 检查节点是否在编辑器内
2. CodeMirror 缓存 DOM Selection 状态，需要正确触发 `selectionchange` 事件
3. CodeMirror 的 `updateSelection` 会调用 `collapse/extend`，可能触发嵌套更新
