# Design Document: 原生 JS 操作 UI 增量更新分析

## Overview

本文档全面分析 LightUI 框架中原生 JavaScript 操作 UI 时的增量更新机制。

## Architecture

### 当前架构流程

1. DOM 操作触发 → DirtyNodeTracker 记录 + DOMObserver 通知
2. WindowDOMObserver 接收通知 → 决定更新策略
3. 增量更新 / 子树重建 / 全量重建

### 关键文件

- `core/dom/node.cpp` - DOM 节点操作
- `core/dom/element.cpp` - 元素属性和样式操作
- `core/window/window_dom_observer.cpp` - DOM 变化监听
- `core/render/css/style_resolver.cpp` - CSS 样式解析


## Components and Interfaces

### 1. DOM 结构操作分析

#### 1.1 appendChild

**代码路径**: `Node::AppendChild()` → `DirtyNodeTracker::RecordNodeAdded()` → `DOMObserver::NotifyNodeAdded()` → `WindowDOMObserver::OnNodeAdded()`

**当前行为**:
```cpp
// node.cpp
std::shared_ptr<Node> Node::AppendChild(std::shared_ptr<Node> child) {
    // 1. 修改 DOM 树
    child_nodes_.push_back(child);
    child->SetParentNode(shared_from_this());
    
    // 2. 记录到 DirtyNodeTracker（延迟处理）
    doc->GetDirtyTracker().RecordNodeAdded(child, shared_from_this(), index);
    
    // 3. 通知观察者（立即处理）
    doc->GetObserverManager().NotifyNodeAdded(child.get(), this);
}
```

**WindowDOMObserver 处理**:
- 检查是否为脱离文档流元素 (position: fixed/absolute)
- 查找最近的布局边界祖先
- 标记节点需要样式重算和布局
- 调用 `SetNeedsRepaint()`

**状态**: ✅ 正常工作

#### 1.2 removeChild

**代码路径**: `Node::RemoveChild()` → `DirtyNodeTracker::RecordNodeRemoved()` → `WindowDOMObserver::OnNodeRemoved()`

**当前行为**:
- 先通知观察者，再从 DOM 树移除
- 处理 fixed 元素的层移除
- 标记父节点需要布局

**状态**: ✅ 正常工作

#### 1.3 insertBefore

**代码路径**: `Node::InsertBefore()` → 同 appendChild

**当前行为**:
- 计算插入索引
- 如果 ref_child 为 null，等同于 appendChild

**状态**: ✅ 正常工作

#### 1.4 replaceChild

**代码路径**: `Node::ReplaceChild()` → `DirtyNodeTracker::RecordNodeReplaced()`

**当前行为**:
```cpp
// 记录为原子替换操作（不是先删后加）
doc->GetDirtyTracker().RecordNodeReplaced(old_child, new_child, shared_from_this(), index);

// 通知观察者：旧节点被移除
doc->GetObserverManager().NotifyNodeRemoved(old_child.get(), this);

// 替换节点
*it = new_child;

// 通知观察者：新节点被添加
doc->GetObserverManager().NotifyNodeAdded(new_child.get(), this);
```

**状态**: ✅ 正常工作（已修复时序问题）

#### 1.5 innerHTML / outerHTML

**代码路径**: `Element::SetInnerHTML()` → Lexbor 解析 → 批量 appendChild

**当前行为**:
- 使用 Lexbor 解析 HTML 片段
- 清空所有子节点
- 逐个添加解析后的节点

**状态**: ✅ 正常工作


### 2. 属性操作分析

#### 2.1 setAttribute

**代码路径**: `Element::SetAttribute()` → `DOMObserver::NotifyAttributeChanged()` → `WindowDOMObserver::OnAttributeChanged()`

**当前行为**:
```cpp
// element.cpp
void Element::SetAttribute(const std::string& name, const std::string& value) {
    // 1. 更新属性存储
    attributes_[name] = value;
    
    // 2. 智能脏标记
    if (name == "style") {
        MarkDirty(DirtyType::STYLE | DirtyType::PAINT);
    } else if (IsLayoutAttribute(name)) {
        MarkDirty(DirtyType::LAYOUT | DirtyType::PAINT);
    }
    
    // 3. 通知观察者
    doc->GetObserverManager().NotifyAttributeChanged(this, name, old_value, value);
}
```

**WindowDOMObserver::OnAttributeChanged 处理**:
```cpp
if (name == "style" || name == "class") {
    // 重新解析样式
    auto new_style = resolver.ResolveStyle(elem_ptr, parent_style);
    render_obj->SetComputedStyle(new_style);
    
    // 同步更新布局引擎中的样式
    window_->GetLayoutEngine()->UpdateStyle(render_obj.get(), new_style);
}
```

**状态**: ⚠️ 部分问题（见 2.2）

#### 2.2 className 属性变化 - 已知 Bug

**问题描述**: 通过 `element.className = 'xxx'` 切换 CSS 类时，如果类对应的样式包含 `display` 属性变化，增量渲染不会正确更新 UI。

**复现步骤**:
```javascript
element.className = 'page';        // display: none
element.className = 'page active'; // display: block
// UI 不更新！
```

**根本原因分析**:

`OnAttributeChanged` 处理 `class` 属性时：
1. ✅ 重新解析样式
2. ✅ 更新 ComputedStyle
3. ❌ **没有检测 display 属性是否从 none 变为其他值**

对比 `OnStyleChanged` 的正确处理：
```cpp
if (property == "display") {
    bool was_none = (old_value == "none" || old_value.empty());
    bool is_none = (new_value == "none");
    if (was_none != is_none) {
        window_->InvalidateRenderTree();  // ← 正确触发重建
        return;
    }
}
```

**修复方案**:
```cpp
// 在 OnAttributeChanged 中添加
if (name == "style" || name == "class") {
    // 获取旧的 display 值
    const auto& old_style = render_obj->GetComputedStyle();
    std::string old_display = old_style.display;
    
    // 重新解析样式
    auto new_style = resolver.ResolveStyle(elem_ptr, parent_style);
    
    // 检测 display 属性变化
    bool was_none = (old_display == "none" || old_display.empty());
    bool is_none = (new_style.display == "none");
    if (was_none != is_none) {
        window_->InvalidateRenderTree();
        window_->SetNeedsRepaint();
        return;
    }
    
    render_obj->SetComputedStyle(new_style);
    // ...
}
```

**状态**: ❌ Bug - 需要修复

#### 2.3 classList 操作

**代码路径**: `DOMTokenList::add/remove/toggle()` → `Element::SetClassName()` → `Element::SetAttribute("class", ...)`

**当前行为**: 最终调用 SetAttribute，继承其问题

**状态**: ⚠️ 继承 className 的 bug


### 3. 样式操作分析

#### 3.1 element.style.xxx

**代码路径**: `Element::SetStyle()` → `DOMObserver::NotifyStyleChanged()` → `WindowDOMObserver::OnStyleChanged()`

**当前行为**:
```cpp
// element.cpp
void Element::SetStyle(const std::string& property, const std::string& value) {
    // 1. 更新样式存储
    styles_[property] = value;
    
    // 2. 同步更新 style attribute
    UpdateStyleAttribute();
    
    // 3. 智能脏标记
    if (is_position_offset && skip_layout) {
        MarkDirty(DirtyType::PAINT);  // 位置偏移只需重绘
    } else if (always_layout_properties.count(property) > 0) {
        MarkDirty(DirtyType::LAYOUT | DirtyType::PAINT);
    } else {
        MarkDirty(DirtyType::PAINT);  // 只影响外观
    }
    
    // 4. 通知观察者
    doc->GetObserverManager().NotifyStyleChanged(this, property, old_value, value);
}
```

**状态**: ✅ 正常工作

#### 3.2 display 属性特殊处理

**代码路径**: `WindowDOMObserver::OnStyleChanged()`

**当前行为**:
```cpp
if (property == "display") {
    bool was_none = (old_value == "none" || old_value.empty());
    bool is_none = (new_value == "none");
    if (was_none != is_none) {
        // 可见性发生变化，需要重建渲染树
        window_->InvalidateRenderTree();
        window_->SetNeedsRepaint();
        return;
    }
}
```

**状态**: ✅ 正常工作

#### 3.3 Paint-only 属性

以下属性只触发重绘，不触发布局：
- `color`, `background-color`, `background-image`
- `border-color`, `opacity`, `visibility`
- `box-shadow`, `text-shadow`, `outline`
- `cursor`, `caret-color`, `text-decoration-color`

**状态**: ✅ 正常工作

#### 3.4 Layout-affecting 属性

以下属性触发布局重算：
- 尺寸: `width`, `height`, `min-*`, `max-*`
- 间距: `padding-*`, `margin-*`
- 边框: `border-*-width`
- 定位: `position`, `display`
- Flexbox: `flex-*`, `justify-content`, `align-items`
- 字体: `font-size`, `line-height`

**状态**: ✅ 正常工作

### 4. 文本内容操作分析

#### 4.1 textContent

**代码路径**: `Node::SetTextContent()` → `Text::SetData()` → `DOMObserver::NotifyTextChanged()` → `WindowDOMObserver::OnTextChanged()`

**当前行为**:
```cpp
// node.cpp - 优化：单个 Text 子节点直接更新
if (child_nodes_.size() == 1 && 
    child_nodes_[0]->GetNodeType() == NodeType::TEXT_NODE &&
    !content.empty()) {
    auto text_node = std::static_pointer_cast<Text>(child_nodes_[0]);
    if (text_node->GetData() != content) {
        text_node->SetData(content);  // 触发 OnTextChanged
    }
    return;
}
```

**WindowDOMObserver::OnTextChanged 处理**:
```cpp
// 1. 标记节点需要样式重算和布局
node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
node->SetNeedsLayout();

// 2. 更新 RenderText
if (render_obj->GetType() == RenderObjectType::TEXT) {
    auto render_text = static_cast<RenderText*>(render_obj.get());
    render_text->SetText(new_text);
}

// 3. 标记需要布局和重绘
render_obj->MarkNeedsLayout();
render_obj->MarkNeedsPaint();
```

**状态**: ✅ 正常工作

#### 4.2 Text.data

**代码路径**: `Text::SetData()` → `DOMObserver::NotifyTextChanged()`

**状态**: ✅ 正常工作


### 5. 伪类状态变化分析

#### 5.1 :hover 伪类

**代码路径**: `Element::SetPseudoClass("hover", ...)` → `DOMObserver::NotifyPseudoClassChanged()` → `WindowDOMObserver::OnPseudoClassChanged()`

**当前行为**:
```cpp
// 性能优化：只有当元素有 :hover 相关的 CSS 规则时才重新解析样式
if (!style_manager || !style_manager->HasHoverRules(element.get())) {
    return;  // 没有 hover 规则，不需要重新解析样式
}

// 重新解析样式以获取 :hover 伪类的样式（包括动画）
auto new_style = resolver.ResolveStyle(element, parent_style);
render_obj->SetComputedStyle(new_style);
render_obj->MarkNeedsPaint();
```

**状态**: ✅ 正常工作

#### 5.2 :focus 伪类

**代码路径**: `Element::Focus()` → `FocusManager::SetFocus()` → 设置伪类

**当前行为**:
- 使用 FocusManager 管理焦点
- 触发 SDL 文本输入
- 更新光标渲染

**状态**: ✅ 正常工作

#### 5.3 其他伪类

`:active`, `:checked`, `:disabled`, `:focus-visible`

**当前行为**: 只标记需要重绘，不影响布局

**状态**: ✅ 正常工作

### 6. 批量操作分析

#### 6.1 BeginBatch / EndBatch

**代码路径**: `Document::BeginBatch()` / `Document::EndBatch()` → `WindowDOMObserver::OnSubtreeModified()`

**当前行为**:
```cpp
// WindowDOMObserver 中检查
if (window_ && !IsInBatch(node)) {
    // 只有不在批量操作中才立即处理
}
```

**状态**: ✅ 正常工作

#### 6.2 增量 vs 全量重建决策

**代码路径**: `WindowDOMObserver::OnSubtreeModified()`

**决策逻辑**:
```cpp
// 1. 只有文本变化 → 增量更新
if (tracker.GetTextChangeCount() > 0 && 
    tracker.GetStructuralChangeCount() == 0) {
    // 走增量更新路径
    return;
}

// 2. 变化区域 < 50% 视口 → 增量更新
if (total_change_area < viewport_area * 0.5f) {
    // 标记受影响的节点需要重新布局
    return;
}

// 3. 大量变化 → 全量重建
window_->InvalidateRenderTree();
```

**状态**: ✅ 正常工作

### 7. 已知问题汇总

#### Bug 1: CSS 类名变化后增量渲染不生效

| 项目 | 内容 |
|------|------|
| **问题** | `element.className = 'xxx'` 切换 CSS 类时，display 属性变化不触发渲染树重建 |
| **影响** | 标签页切换、侧边栏导航等依赖 CSS 类切换的 UI 交互 |
| **根因** | `OnAttributeChanged` 没有检测 display 属性变化 |
| **文件** | `core/window/window_dom_observer.cpp` 第 259-310 行 |
| **修复** | 在处理 class 属性时，比较新旧样式的 display 属性 |
| **优先级** | 中 |

#### Bug 2: 鼠标滚轮无法滚动到容器最底部

| 项目 | 内容 |
|------|------|
| **问题** | 使用鼠标滚轮滚动时无法到达最底部，但拖动滚动条可以 |
| **影响** | 所有 overflow-y: auto/scroll 的可滚动容器 |
| **根因** | 待分析，可能是滚轮事件处理中的边界计算问题 |
| **文件** | `core/event/` 相关文件 |
| **优先级** | 低 |


## Data Models

### 操作类型与更新策略映射

| 操作类型 | 触发方法 | Observer 回调 | 更新策略 | 状态 |
|---------|---------|--------------|---------|------|
| appendChild | Node::AppendChild | OnNodeAdded | 增量 | ✅ |
| removeChild | Node::RemoveChild | OnNodeRemoved | 增量 | ✅ |
| insertBefore | Node::InsertBefore | OnNodeAdded | 增量 | ✅ |
| replaceChild | Node::ReplaceChild | OnNodeRemoved + OnNodeAdded | 增量 | ✅ |
| innerHTML | Element::SetInnerHTML | 批量 OnNodeAdded | 增量/全量 | ✅ |
| setAttribute | Element::SetAttribute | OnAttributeChanged | 增量 | ⚠️ |
| className | Element::SetClassName | OnAttributeChanged | 增量 | ❌ |
| classList.add/remove | DOMTokenList | OnAttributeChanged | 增量 | ❌ |
| element.style.xxx | Element::SetStyle | OnStyleChanged | 增量 | ✅ |
| style.display | Element::SetStyle | OnStyleChanged | 全量重建 | ✅ |
| textContent | Node::SetTextContent | OnTextChanged | 增量 | ✅ |
| Text.data | Text::SetData | OnTextChanged | 增量 | ✅ |
| :hover | Element::SetPseudoClass | OnPseudoClassChanged | 增量 | ✅ |
| :focus | Element::Focus | OnPseudoClassChanged | 增量 | ✅ |

### 脏标记类型

```cpp
enum class DirtyType {
    NONE = 0,
    STYLE = 1 << 0,   // 需要样式重算
    LAYOUT = 1 << 1,  // 需要布局
    PAINT = 1 << 2,   // 需要重绘
    ALL = STYLE | LAYOUT | PAINT
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system.*

由于本文档主要是分析文档而非功能实现，大部分需求是文档性质的，不适合作为可测试的属性。但我们可以定义一个验证 Bug 存在的测试：

Property 1: CSS 类切换 display 属性应触发渲染树更新
*For any* element with a CSS class that changes `display` from `none` to `block` (or vice versa), changing the element's `className` property should result in the element's visibility being correctly updated in the rendered output.
**Validates: Requirements 2.4, 7.2**

## Error Handling

### 错误场景

1. **空节点操作**: appendChild/removeChild 传入 null 时抛出 `std::invalid_argument`
2. **节点未找到**: removeChild 找不到子节点时抛出 `std::invalid_argument`
3. **无 Document**: 节点没有 owner document 时跳过观察者通知

## Testing Strategy

### 单元测试

现有测试位于 `tests/unit/dom/test_incremental_update.cpp`，覆盖：
- 文本变化的增量更新
- 脏标记传播
- 样式变化处理

### 需要补充的测试

1. **CSS 类切换测试**: 验证 className 变化时 display 属性的正确处理
2. **批量操作测试**: 验证 BeginBatch/EndBatch 的正确行为
3. **边界情况测试**: 空内容、深层嵌套、大量节点

### 测试命令

```cmd
build\bin\Release\esm_loader.exe tests\js\test_incremental_update.js -q 5
```
