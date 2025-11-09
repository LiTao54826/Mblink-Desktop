# LightUI DOM API 文档

**版本**: Phase 2.2
**日期**: 2025-11-09

---

## 📋 目录

- [概述](#概述)
- [核心类](#核心类)
- [Document API](#document-api)
- [Element API](#element-api)
- [Node API](#node-api)
- [Text API](#text-api)
- [Event API](#event-api)
- [JavaScript 绑定](#javascript-绑定)
- [使用示例](#使用示例)

---

## 概述

LightUI DOM API 是一个轻量级的 DOM 实现，遵循 W3C DOM 标准，支持：

- ✅ DOM 树构建和操作
- ✅ 元素属性和样式管理
- ✅ CSS 选择器查询（QuerySelector/QuerySelectorAll）
- ✅ 事件系统（冒泡、捕获、preventDefault）
- ✅ QuickJS JavaScript 绑定
- ✅ 高性能优化（ID 缓存、Hash Map）

---

## 核心类

### 类层次结构

```
Node (基类)
├── Element (元素节点)
├── Text (文本节点)
└── Document (文档节点)
```

### 节点类型

```cpp
enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    DOCUMENT_NODE = 9
};
```

---

## Document API

### 创建 Document

```cpp
#include "core/dom/document.h"

auto doc = std::make_shared<Document>();
```

### 工厂方法

#### CreateElement
创建元素节点。

```cpp
auto div = doc->CreateElement("div");
auto span = doc->CreateElement("span");
```

#### CreateTextNode
创建文本节点。

```cpp
auto text = doc->CreateTextNode("Hello World");
```

### 查询方法

#### GetElementById
通过 ID 查询元素（O(1) 时间复杂度）。

```cpp
auto elem = doc->GetElementById("myId");
if (elem) {
    // 找到元素
}
```

#### GetElementsByTagName
通过标签名查询所有元素。

```cpp
auto divs = doc->GetElementsByTagName("div");
for (const auto& div : divs) {
    // 处理每个 div
}
```

#### GetElementsByClassName
通过类名查询所有元素。

```cpp
auto items = doc->GetElementsByClassName("item");
```

### 文档属性

```cpp
// 获取文档元素（<html>）
auto html = doc->GetDocumentElement();

// 获取 body 元素
auto body = doc->GetBody();

// 设置 body 元素
doc->SetBody(bodyElement);
```

---

## Element API

### 创建和基本属性

```cpp
auto elem = doc->CreateElement("div");

// 获取标签名
std::string tag = elem->GetTagName();  // "div"

// 获取节点类型
NodeType type = elem->GetNodeType();  // NodeType::ELEMENT_NODE
```

### 属性操作

#### SetAttribute / GetAttribute
设置和获取属性。

```cpp
// 设置属性
elem->SetAttribute("id", "myDiv");
elem->SetAttribute("class", "container");
elem->SetAttribute("data-value", "123");

// 获取属性
std::string id = elem->GetAttribute("id");  // "myDiv"
std::string cls = elem->GetAttribute("class");  // "container"

// 获取不存在的属性返回空字符串
std::string missing = elem->GetAttribute("missing");  // ""
```

#### HasAttribute / RemoveAttribute
检查和删除属性。

```cpp
// 检查属性是否存在
if (elem->HasAttribute("id")) {
    // 有 id 属性
}

// 删除属性
elem->RemoveAttribute("data-value");
```

### 样式操作

```cpp
// 设置样式
elem->SetStyle("color", "red");
elem->SetStyle("font-size", "16px");

// 获取样式
std::string color = elem->GetStyle("color");  // "red"

// 检查样式
if (elem->HasStyle("color")) {
    // 有 color 样式
}

// 删除样式
elem->RemoveStyle("font-size");
```

### DOM 树操作

#### AppendChild
添加子节点到末尾。

```cpp
auto parent = doc->CreateElement("div");
auto child = doc->CreateElement("span");

parent->AppendChild(child);
```

#### InsertBefore
在指定节点前插入。

```cpp
auto parent = doc->CreateElement("div");
auto child1 = doc->CreateElement("span");
auto child2 = doc->CreateElement("p");

parent->AppendChild(child1);
parent->InsertBefore(child2, child1);  // child2 在 child1 前面
```

#### RemoveChild
移除子节点。

```cpp
parent->RemoveChild(child);
```

#### ReplaceChild
替换子节点。

```cpp
auto newChild = doc->CreateElement("div");
parent->ReplaceChild(newChild, oldChild);
```

### 查询方法

#### QuerySelector
查询第一个匹配的元素。

```cpp
// 标签选择器
auto div = elem->QuerySelector("div");

// ID 选择器
auto myElem = elem->QuerySelector("#myId");

// 类选择器
auto item = elem->QuerySelector(".item");

// 属性选择器
auto input = elem->QuerySelector("[name='username']");

// 通配符
auto any = elem->QuerySelector("*");
```

#### QuerySelectorAll
查询所有匹配的元素。

```cpp
auto items = elem->QuerySelectorAll(".item");
for (const auto& item : items) {
    // 处理每个 item
}
```

#### Matches
检查元素是否匹配选择器。

```cpp
if (elem->Matches(".active")) {
    // 元素有 active 类
}
```

#### Closest
查找最近的匹配祖先元素（包括自身）。

```cpp
auto container = elem->Closest(".container");
if (container) {
    // 找到最近的 container
}
```

### innerHTML

#### GetInnerHTML
获取 HTML 字符串。

```cpp
std::string html = elem->GetInnerHTML();
// 例如: "<span>Hello</span><p>World</p>"
```

#### SetInnerHTML
设置 HTML 内容（简化实现）。

```cpp
elem->SetInnerHTML("<span>New Content</span>");
```

### 事件监听

#### AddEventListener
添加事件监听器。

```cpp
elem->AddEventListener("click", [](std::shared_ptr<Event> e) {
    std::cout << "Clicked!" << std::endl;
});
```

#### DispatchEvent
分发事件。

```cpp
auto event = std::make_shared<Event>("click", true);  // bubbles = true
elem->DispatchEvent(event);
```

### 克隆

#### CloneNode
克隆节点。

```cpp
// 浅克隆（不包括子节点）
auto clone1 = elem->CloneNode(false);

// 深克隆（包括所有子节点）
auto clone2 = elem->CloneNode(true);
```

---

## Node API

### 节点关系

```cpp
// 获取父节点
auto parent = node->GetParentNode();

// 获取子节点列表
const auto& children = node->GetChildNodes();

// 获取第一个子节点
auto first = node->GetFirstChild();

// 获取最后一个子节点
auto last = node->GetLastChild();

// 获取下一个兄弟节点
auto next = node->GetNextSibling();

// 获取上一个兄弟节点
auto prev = node->GetPreviousSibling();
```

### 节点操作

```cpp
// 检查是否有子节点
if (node->HasChildNodes()) {
    // 有子节点
}

// 添加子节点
node->AppendChild(child);

// 插入子节点
node->InsertBefore(newChild, refChild);

// 移除子节点
node->RemoveChild(child);

// 替换子节点
node->ReplaceChild(newChild, oldChild);

// 克隆节点
auto clone = node->CloneNode(true);
```

### 脏标记

```cpp
// 标记节点为脏（需要重新布局/渲染）
node->MarkDirty();

// 检查是否为脏节点
if (node->IsDirty()) {
    // 需要更新
}

// 清除脏标记
node->ClearDirty();
```

---

## Text API

### 创建和操作

```cpp
// 创建文本节点
auto text = doc->CreateTextNode("Hello World");

// 获取文本数据
std::string data = text->GetData();  // "Hello World"

// 设置文本数据
text->SetData("New Text");

// 获取文本长度
size_t len = text->GetLength();  // 8
```

---

## Event API

### Event（基类）

```cpp
// 创建事件
auto event = std::make_shared<Event>("click", true, true);
// 参数: type, bubbles, cancelable

// 事件属性
std::string type = event->GetType();  // "click"
bool bubbles = event->GetBubbles();  // true
bool cancelable = event->GetCancelable();  // true

// 事件目标
auto target = event->GetTarget();
auto currentTarget = event->GetCurrentTarget();

// 事件阶段
EventPhase phase = event->GetEventPhase();

// 停止传播
event->StopPropagation();
event->StopImmediatePropagation();

// 阻止默认行为
event->PreventDefault();

// 检查是否已阻止默认行为
if (event->IsDefaultPrevented()) {
    // 默认行为已被阻止
}
```

### MouseEvent

```cpp
auto mouseEvent = std::make_shared<MouseEvent>(
    "click",
    100,  // clientX
    200,  // clientY
    1     // button (0=left, 1=middle, 2=right)
);

int x = mouseEvent->GetClientX();  // 100
int y = mouseEvent->GetClientY();  // 200
int button = mouseEvent->GetButton();  // 1
```

### KeyboardEvent

```cpp
auto keyEvent = std::make_shared<KeyboardEvent>(
    "keydown",
    "Enter",  // key
    13        // keyCode
);

std::string key = keyEvent->GetKey();  // "Enter"
int code = keyEvent->GetKeyCode();  // 13
```

---

## JavaScript 绑定

### 初始化

```cpp
#include "core/dom/dom_bindings.h"

JSRuntime* rt = JS_NewRuntime();
JSContext* ctx = JS_NewContext(rt);

// 初始化 DOM 绑定
DOMBindings::Init(ctx);
```

### JavaScript 使用示例

```javascript
// 创建元素
var div = document.createElement('div');
div.id = 'myDiv';
div.className = 'container';

// 设置属性
div.setAttribute('data-value', '123');

// 创建文本节点
var text = document.createTextNode('Hello World');

// 添加子节点
div.appendChild(text);

// 查询元素
var elem = document.getElementById('myDiv');

// 添加事件监听器
div.addEventListener('click', function(e) {
    console.log('Clicked!');
});
```

---

## 使用示例

### 示例 1: 构建简单 DOM 树

```cpp
auto doc = std::make_shared<Document>();

// 创建 HTML 结构
auto html = doc->CreateElement("html");
auto body = doc->CreateElement("body");
auto div = doc->CreateElement("div");

div->SetAttribute("id", "container");
div->SetAttribute("class", "main");

auto text = doc->CreateTextNode("Hello World");
div->AppendChild(text);

body->AppendChild(div);
html->AppendChild(body);

doc->SetBody(body);
```

### 示例 2: 事件处理

```cpp
auto button = doc->CreateElement("button");
button->SetAttribute("id", "myButton");

// 添加点击事件监听器
button->AddEventListener("click", [](std::shared_ptr<Event> e) {
    std::cout << "Button clicked!" << std::endl;
    
    // 阻止默认行为
    e->PreventDefault();
    
    // 停止事件冒泡
    e->StopPropagation();
});

// 触发事件
auto event = std::make_shared<MouseEvent>("click", 100, 200, 0);
button->DispatchEvent(event);
```

### 示例 3: 查询和遍历

```cpp
// 创建网格结构
auto container = doc->CreateElement("div");
for (int i = 0; i < 10; i++) {
    auto row = doc->CreateElement("div");
    row->SetAttribute("class", "row");
    
    for (int j = 0; j < 10; j++) {
        auto cell = doc->CreateElement("div");
        cell->SetAttribute("class", "cell");
        cell->SetAttribute("id", "cell-" + std::to_string(i) + "-" + std::to_string(j));
        row->AppendChild(cell);
    }
    
    container->AppendChild(row);
}

// 查询所有行
auto rows = container->QuerySelectorAll(".row");
std::cout << "Total rows: " << rows.size() << std::endl;  // 10

// 查询特定单元格
auto cell = container->QuerySelector("#cell-5-5");

// 查找最近的行
auto row = cell->Closest(".row");
```

### 示例 4: innerHTML

```cpp
auto div = doc->CreateElement("div");

auto h1 = doc->CreateElement("h1");
h1->AppendChild(doc->CreateTextNode("Title"));

auto p = doc->CreateElement("p");
p->SetAttribute("class", "intro");
p->AppendChild(doc->CreateTextNode("Introduction"));

div->AppendChild(h1);
div->AppendChild(p);

// 获取 HTML 字符串
std::string html = div->GetInnerHTML();
// 输出: <h1>Title</h1><p class="intro">Introduction</p>
```

---

## 性能建议

### 1. 优先使用 GetElementById

```cpp
// ✅ 推荐（O(1)，极快）
auto elem = doc->GetElementById("myId");

// ❌ 避免（O(n)，慢 133 倍）
auto elem = container->QuerySelector("#myId");
```

### 2. 缓存查询结果

```cpp
// ❌ 避免重复查询
for (int i = 0; i < 100; i++) {
    auto elem = container->QuerySelector(".item");
    // ...
}

// ✅ 缓存结果
auto elem = container->QuerySelector(".item");
for (int i = 0; i < 100; i++) {
    // 使用缓存的 elem
}
```

### 3. 使用事件委托

```cpp
// ❌ 为每个元素添加监听器
for (const auto& item : items) {
    item->AddEventListener("click", handler);
}

// ✅ 在父元素添加一个监听器
container->AddEventListener("click", [](std::shared_ptr<Event> e) {
    auto target = std::dynamic_pointer_cast<Element>(e->GetTarget());
    if (target && target->Matches(".item")) {
        // 处理点击
    }
});
```

---

## 参考资料

- [W3C DOM 标准](https://www.w3.org/TR/dom/)
- [MDN DOM 文档](https://developer.mozilla.org/en-US/docs/Web/API/Document_Object_Model)
- [性能分析文档](PERFORMANCE.md)

---

**文档版本**: Phase 2.2
**最后更新**: 2025-11-09

