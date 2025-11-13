# DOM 模块

## 📋 概述

DOM (Document Object Model) 模块是 MBink 的核心模块之一，提供了完整的 DOM 树结构和操作 API。它实现了 W3C DOM 标准的子集，支持动态创建、修改和查询 HTML 文档结构。

## 🎯 主要功能

- **DOM 树管理**: 完整的节点树结构（Node, Element, Text, Document）
- **属性和样式管理**: 支持 `setAttribute`、`style`、`classList` 等 API
- **事件监听器**: `addEventListener`、`removeEventListener` 等事件 API
- **CSS 选择器**: `querySelector`、`querySelectorAll` 支持
- **HTML 元素**: 40+ 专用 HTML 元素类（Button, Input, Form 等）
- **DOM 观察器**: MutationObserver 支持
- **innerHTML/outerHTML**: 动态 HTML 内容操作

## 📁 文件结构

```
dom/
├── CMakeLists.txt                # 构建配置
├── node.h/cpp                    # 基础节点类
├── element.h/cpp                 # 元素节点类
├── text.h/cpp                    # 文本节点类
├── document.h/cpp                # 文档对象
├── event.h/cpp                   # 事件对象
├── dom_bindings.h/cpp            # JavaScript 绑定
├── dom_observer.h/cpp            # DOM 观察器
├── selector_engine.h/cpp         # CSS 选择器引擎
├── dom_token_list.h/cpp          # classList 实现
├── css_style_declaration.h/cpp   # style 属性实现
├── dom_string_map.h/cpp          # dataset 实现
└── html_*.h/cpp                  # 专用 HTML 元素类
    ├── html_input_element.h/cpp
    ├── html_textarea_element.h/cpp
    ├── html_button_element.h/cpp
    ├── html_form_element.h/cpp
    ├── html_select_element.h/cpp
    ├── html_option_element.h/cpp
    ├── html_anchor_element.h/cpp
    ├── html_label_element.h/cpp
    ├── html_image_element.h/cpp
    ├── html_div_element.h/cpp
    ├── html_span_element.h/cpp
    ├── html_paragraph_element.h/cpp
    └── html_heading_element.h/cpp
```

## 🔌 核心类

### Node (基础节点类)

```cpp
class Node {
public:
    // 节点类型
    enum NodeType {
        ELEMENT_NODE = 1,
        TEXT_NODE = 3,
        DOCUMENT_NODE = 9
    };
    
    // 树结构操作
    void AppendChild(std::shared_ptr<Node> child);
    void RemoveChild(std::shared_ptr<Node> child);
    void InsertBefore(std::shared_ptr<Node> new_node, 
                      std::shared_ptr<Node> ref_node);
    
    // 属性访问
    NodeType GetNodeType() const;
    std::string GetNodeName() const;
    std::string GetTextContent() const;
    void SetTextContent(const std::string& content);
    
    // 树遍历
    std::shared_ptr<Node> GetParentNode() const;
    std::shared_ptr<Node> GetFirstChild() const;
    std::shared_ptr<Node> GetLastChild() const;
    std::shared_ptr<Node> GetNextSibling() const;
    std::shared_ptr<Node> GetPreviousSibling() const;
};
```

### Element (元素节点类)

```cpp
class Element : public Node {
public:
    // 属性管理
    void SetAttribute(const std::string& name, const std::string& value);
    std::string GetAttribute(const std::string& name) const;
    bool HasAttribute(const std::string& name) const;
    void RemoveAttribute(const std::string& name);
    
    // 样式管理
    std::shared_ptr<CSSStyleDeclaration> GetStyle();
    std::shared_ptr<DOMTokenList> GetClassList();
    
    // 选择器查询
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
    std::vector<std::shared_ptr<Element>> QuerySelectorAll(
        const std::string& selector);
    
    // 事件监听
    void AddEventListener(const std::string& type, EventListener listener);
    void RemoveEventListener(const std::string& type, uint64_t listener_id);
    
    // HTML 内容
    std::string GetInnerHTML() const;
    void SetInnerHTML(const std::string& html);
    std::string GetOuterHTML() const;
    
    // 常用属性
    std::string GetId() const;
    void SetId(const std::string& id);
    std::string GetClassName() const;
    void SetClassName(const std::string& class_name);
};
```

### Document (文档对象)

```cpp
class Document : public Node {
public:
    // 元素创建
    std::shared_ptr<Element> CreateElement(const std::string& tag_name);
    std::shared_ptr<Text> CreateTextNode(const std::string& data);
    
    // 选择器查询
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
    std::vector<std::shared_ptr<Element>> QuerySelectorAll(
        const std::string& selector);
    
    // ID 查询
    std::shared_ptr<Element> GetElementById(const std::string& id);
    
    // 文档结构
    std::shared_ptr<Element> GetDocumentElement();
    std::shared_ptr<Element> GetBody();
    std::shared_ptr<Element> GetHead();
    
    // HTML 加载
    void LoadHTML(const std::string& html);
};
```

## 💡 使用示例

### 创建 DOM 树

```cpp
auto doc = std::make_shared<Document>();

// 创建元素
auto div = doc->CreateElement("div");
div->SetAttribute("id", "container");
div->SetAttribute("class", "main-content");

// 创建文本节点
auto text = doc->CreateTextNode("Hello World!");

// 构建树结构
div->AppendChild(text);
doc->GetBody()->AppendChild(div);
```

### 样式操作

```cpp
auto element = doc->CreateElement("div");

// 使用 style 属性
auto style = element->GetStyle();
style->SetProperty("color", "red");
style->SetProperty("font-size", "16px");

// 使用 classList
auto classList = element->GetClassList();
classList->Add("active");
classList->Add("highlight");
classList->Toggle("hidden");
```

### 事件监听

```cpp
auto button = doc->CreateElement("button");

button->AddEventListener("click", [](std::shared_ptr<Event> e) {
    std::cout << "Button clicked!" << std::endl;
});
```

### 选择器查询

```cpp
// 单个元素
auto header = doc->QuerySelector("h1.title");

// 多个元素
auto buttons = doc->QuerySelectorAll("button.primary");

// ID 查询
auto container = doc->GetElementById("main-container");
```

### innerHTML 操作

```cpp
auto div = doc->CreateElement("div");

// 设置 HTML 内容
div->SetInnerHTML("<h1>Title</h1><p>Content</p>");

// 获取 HTML 内容
std::string html = div->GetInnerHTML();
```

## 🔗 依赖关系

### 依赖的模块

- `core/utils` - 日志和工具函数
- `third_party/lexbor` - HTML 解析

### 被依赖的模块

- `core/quickjs` - JavaScript 绑定
- `core/event` - 事件处理
- `core/render` - 渲染引擎
- `core/layout` - 布局计算

## 🏗️ 架构说明

DOM 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  JavaScript Runtime (core/quickjs)      │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  DOM Module (core/dom) ← 当前模块        │
│  Node, Element, Document                │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Lexbor (HTML Parsing)                  │
└─────────────────────────────────────────┘
```

## 📚 支持的 DOM API

### Node API (20+)
- `appendChild`, `removeChild`, `insertBefore`
- `cloneNode`, `contains`, `hasChildNodes`
- `parentNode`, `firstChild`, `lastChild`
- `nextSibling`, `previousSibling`
- `nodeType`, `nodeName`, `textContent`

### Element API (40+)
- `setAttribute`, `getAttribute`, `removeAttribute`
- `querySelector`, `querySelectorAll`
- `addEventListener`, `removeEventListener`
- `classList`, `className`, `id`
- `innerHTML`, `outerHTML`, `textContent`
- `style`, `dataset`
- `getBoundingClientRect` (部分支持)

### Document API (15+)
- `createElement`, `createTextNode`
- `querySelector`, `querySelectorAll`
- `getElementById`, `getElementsByClassName`
- `body`, `head`, `documentElement`

## 🔧 HTML 元素支持

### 表单元素
- `HTMLInputElement` - `<input>` 元素
- `HTMLTextAreaElement` - `<textarea>` 元素
- `HTMLButtonElement` - `<button>` 元素
- `HTMLFormElement` - `<form>` 元素
- `HTMLSelectElement` - `<select>` 元素
- `HTMLOptionElement` - `<option>` 元素
- `HTMLLabelElement` - `<label>` 元素

### 容器元素
- `HTMLDivElement` - `<div>` 元素
- `HTMLSpanElement` - `<span>` 元素
- `HTMLParagraphElement` - `<p>` 元素
- `HTMLHeadingElement` - `<h1>`-`<h6>` 元素

### 其他元素
- `HTMLAnchorElement` - `<a>` 元素
- `HTMLImageElement` - `<img>` 元素

## ⚠️ 注意事项

1. **内存管理**: 使用 `std::shared_ptr` 管理节点生命周期
2. **循环引用**: 父子节点使用 `weak_ptr` 避免循环引用
3. **线程安全**: DOM 操作应在主线程进行
4. **性能**: 大量 DOM 操作时考虑批量更新

## 📊 性能优化

- **延迟渲染**: DOM 修改不立即触发渲染
- **批量更新**: 使用 DocumentFragment 批量插入
- **选择器缓存**: 缓存常用选择器结果
- **事件委托**: 使用事件冒泡减少监听器数量

## 📚 相关文档

- [DOM API 文档](../../docs/DOM_API.md)
- [HTML 元素指南](../../docs/HTML_ELEMENTS_GUIDE.md)
- [事件系统文档](../event/README.md)
- [JavaScript 绑定](../quickjs/README.md)

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

