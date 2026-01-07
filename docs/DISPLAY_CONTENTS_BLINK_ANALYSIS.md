# Blink display: contents 实现分析

## 核心概念

### 1. display: contents 的语义
- `display: contents` 元素**不生成任何盒子**（no box generation）
- 但其**子元素正常参与布局**，就像它们是父元素的直接子元素一样
- 元素本身仍然存在于 DOM 树中，只是在布局树中"消失"

### 2. Blink 的两棵树
- **DOM 树**：包含所有节点，包括 `display: contents` 元素
- **Layout 树**：只包含生成盒子的节点，`display: contents` 元素被跳过

## 核心类：LayoutTreeBuilderTraversal

这是 Blink 处理 `display: contents` 的核心工具类，提供在 DOM 树上遍历但返回 Layout 树视角结果的方法。

### 关键方法

#### 1. HasDisplayContentsStyle(node)
```cpp
inline static bool HasDisplayContentsStyle(const Node& node) {
  auto* element = DynamicTo<Element>(node);
  return element && element->HasDisplayContentsStyle();
}
```
判断节点是否是 `display: contents`。

#### 2. IsLayoutParent(node)
```cpp
bool LayoutTreeBuilderTraversal::IsLayoutParent(const Node& node) {
  return !HasDisplayContentsStyle(node) && !node.IsColumnPseudoElement();
}
```
判断节点是否能作为布局父级（有渲染对象）。

#### 3. LayoutParent(node) - 查找布局父级
```cpp
ContainerNode* LayoutTreeBuilderTraversal::LayoutParent(const Node& node) {
  ContainerNode* parent = Parent(node);
  
  // 关键：跳过所有 display: contents 祖先
  while (parent && !IsLayoutParent(*parent)) {
    parent = Parent(*parent);
  }
  
  return parent;
}
```
向上遍历，跳过所有 `display: contents` 元素，找到真正有渲染对象的父级。

#### 4. NextLayoutSibling(node) - 查找下一个布局兄弟
```cpp
// 简化版逻辑
Node* NextLayoutSibling(const Node& node, int32_t& limit) {
  // 1. 先在当前层级找
  if (Node* sibling = NextLayoutSiblingInternal(NextSibling(node), limit)) {
    return sibling;
  }
  
  // 2. 如果父节点是 display: contents，继续向上找
  Node* parent = Parent(node);
  while (parent && HasDisplayContentsStyle(*parent)) {
    if (Node* sibling = NextLayoutSiblingInternal(NextSibling(*parent), limit)) {
      return sibling;
    }
    parent = Parent(*parent);
  }
  
  return nullptr;
}
```

#### 5. NextLayoutSiblingInternal - 递归进入 display: contents
```cpp
static Node* NextLayoutSiblingInternal(Node* node, int32_t& limit) {
  for (Node* sibling = node; sibling && limit-- != 0;
       sibling = NextSibling(*sibling)) {
    // 如果不是 display: contents，直接返回
    if (!HasDisplayContentsStyle(*sibling))
      return sibling;

    // 如果是 display: contents，递归进入其子元素
    if (Node* inner = NextLayoutSiblingInternal(FirstChild(*sibling), limit))
      return inner;
  }
  return nullptr;
}
```

#### 6. PreviousLayoutSibling(node) - 查找前一个布局兄弟
```cpp
// 与 NextLayoutSibling 对称，但方向相反
Node* PreviousLayoutSibling(const Node& node, int32_t& limit) {
  if (Node* sibling = PreviousLayoutSiblingInternal(PreviousSibling(node), limit)) {
    return sibling;
  }
  
  Node* parent = Parent(node);
  while (parent && HasDisplayContentsStyle(*parent)) {
    if (Node* sibling = PreviousLayoutSiblingInternal(PreviousSibling(*parent), limit)) {
      return sibling;
    }
    parent = Parent(*parent);
  }
  
  return nullptr;
}
```

#### 7. PreviousLayoutSiblingInternal - 递归进入 display: contents（反向）
```cpp
static Node* PreviousLayoutSiblingInternal(Node* node, int32_t& limit) {
  for (Node* sibling = node; sibling && limit-- != 0;
       sibling = PreviousSibling(*sibling)) {
    if (!HasDisplayContentsStyle(*sibling))
      return sibling;

    // 关键：进入 display: contents 时，从 LastChild 开始
    if (Node* inner = PreviousLayoutSiblingInternal(LastChild(*sibling), limit))
      return inner;
  }
  return nullptr;
}
```

#### 8. NextSiblingLayoutObject - 获取下一个兄弟的渲染对象
```cpp
LayoutObject* NextSiblingLayoutObject(const Node& node, int32_t limit) {
  for (Node* sibling = NextLayoutSibling(node, limit); 
       sibling && limit != -1;
       sibling = NextLayoutSibling(*sibling, limit)) {
    LayoutObject* layout_object = sibling->GetLayoutObject();
    if (layout_object && !IsLayoutObjectReparented(layout_object))
      return layout_object;
  }
  return nullptr;
}
```

## 插入位置计算

### Blink 的方法
在 `layout_tree_builder.h` 中：
```cpp
LayoutObject* NextLayoutObject() const {
  if (!context_.next_sibling_valid) {
    context_.next_sibling = 
        LayoutTreeBuilderTraversal::NextSiblingLayoutObject(*node_);
    context_.next_sibling_valid = true;
  }
  return context_.next_sibling;
}
```

### 插入流程
1. 调用 `NextSiblingLayoutObject(node)` 获取下一个兄弟的渲染对象
2. 调用 `parent->AddChild(new_layout_object, next_layout_object)` 插入
3. 如果 `next_layout_object` 为 null，则插入到末尾

## 关键场景分析

### 场景 1：简单 display: contents
```html
<div id="parent">
  <div id="a">A</div>
  <div id="contents" style="display: contents">
    <div id="b">B</div>
    <div id="c">C</div>
  </div>
  <div id="d">D</div>
</div>
```

DOM 树：
```
parent
├── a
├── contents
│   ├── b
│   └── c
└── d
```

Layout 树：
```
parent
├── a
├── b  (contents 的子元素提升)
├── c  (contents 的子元素提升)
└── d
```

### 场景 2：嵌套 display: contents
```html
<div id="parent">
  <div id="outer" style="display: contents">
    <div id="inner" style="display: contents">
      <div id="child">Child</div>
    </div>
  </div>
</div>
```

Layout 树：
```
parent
└── child  (跳过两层 contents)
```

### 场景 3：查找布局兄弟
对于节点 `b`：
- `NextLayoutSibling(b)` = `c`
- `PreviousLayoutSibling(b)` = `a`

对于节点 `c`：
- `NextLayoutSibling(c)` = `d`（跳出 contents，找到父级的下一个兄弟）
- `PreviousLayoutSibling(c)` = `b`

## 增量更新的关键点

### 1. 插入节点
当在 `display: contents` 元素内插入新节点时：
1. 找到 `LayoutParent` - 跳过 contents 找到真正的布局父级
2. 找到 `NextSiblingLayoutObject` - 计算正确的插入位置
3. 调用 `parent->AddChild(child, next_sibling)`

### 2. 移除节点
当移除 `display: contents` 元素时：
- 需要递归移除其所有子元素的渲染对象
- 因为子元素的渲染对象实际上挂在 contents 的布局父级下

### 3. 移动节点
当移动节点时（如 Preact 的 key 优化）：
1. 从旧位置移除渲染对象（但不销毁）
2. 计算新位置的 `NextSiblingLayoutObject`
3. 在新位置插入

## 当前实现的问题

对比 Blink 的实现，当前 LightUI 的问题可能在于：

### 1. FindPreviousLayoutSiblingRenderObject 不完整
当前实现没有正确处理：
- 向上穿越 `display: contents` 父级
- 递归进入 `display: contents` 兄弟的子元素

### 2. 插入位置计算
Blink 使用 `NextSiblingLayoutObject` 而不是 `PreviousSiblingLayoutObject`：
- 找到下一个兄弟的渲染对象
- 在它**之前**插入

### 3. 移动操作
移动时需要重新计算插入位置，不能简单地保持原有顺序。

## 建议修复方案

1. 实现完整的 `NextLayoutSibling` 和 `PreviousLayoutSibling`
2. 使用 `NextSiblingLayoutObject` 计算插入位置
3. 移动操作时重新计算位置而不是简单移动
