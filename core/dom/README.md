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

## 📁 目录结构

```
dom/
├── CMakeLists.txt                # 构建配置
├── README.md                     # 本文档
│
├── node.h/cpp                    # 基础节点类
├── element.h/cpp                 # 元素节点类
├── text.h/cpp                    # 文本节点类
├── document.h/cpp                # 文档对象
├── event.h/cpp                   # 事件基类
├── drag_event.h/cpp              # 拖拽事件
│
├── bindings/                     # JavaScript 绑定
│   ├── dom_bindings.h/cpp        # DOM API 绑定
│   └── canvas_bindings.h/cpp     # Canvas API 绑定
│
├── observers/                    # 观察者模式实现
│   ├── dom_observer.h/cpp        # DOM 观察器基类
│   ├── mutation_observer.h/cpp   # MutationObserver 实现
│   └── dirty_node_tracker.h/cpp  # 脏节点追踪器
│
├── selection/                    # 文本选择功能
│   ├── selection.h/cpp           # Selection API
│   ├── range.h/cpp               # Range 对象
│   └── selector_engine.h/cpp     # CSS 选择器引擎
│
├── style/                        # 样式相关
│   ├── css_style_declaration.h/cpp    # style 属性实现
│   └── incremental_style_recalc.h/cpp # 增量样式重算
│
├── utils/                        # 工具类
│   ├── dom_token_list.h/cpp      # classList 实现
│   └── dom_string_map.h/cpp      # dataset 实现
│
└── elements/                     # HTML 元素
    ├── html_input_element.h/cpp
    ├── html_textarea_element.h/cpp
    ├── html_button_element.h/cpp
    └── ... (40+ 元素类)
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
    
    // 树遍历
    std::shared_ptr<Node> GetParentNode() const;
    std::shared_ptr<Node> GetFirstChild() const;
    std::shared_ptr<Node> GetLastChild() const;
};
```

### Element (元素节点类)

```cpp
class Element : public Node {
public:
    // 属性管理
    void SetAttribute(const std::string& name, const std::string& value);
    std::string GetAttribute(const std::string& name) const;
    
    // 样式管理
    std::shared_ptr<CSSStyleDeclaration> GetStyle();
    std::shared_ptr<DOMTokenList> GetClassList();
    
    // 选择器查询
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
    std::vector<std::shared_ptr<Element>> QuerySelectorAll(
        const std::string& selector);
};
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

## ⚠️ 注意事项

1. **内存管理**: 使用 `std::shared_ptr` 管理节点生命周期
2. **循环引用**: 父子节点使用 `weak_ptr` 避免循环引用
3. **线程安全**: DOM 操作应在主线程进行

---

**维护者**: MBink Team  
**最后更新**: 2025-12
