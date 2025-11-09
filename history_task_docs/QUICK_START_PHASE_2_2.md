# Phase 2.2 DOM API - 快速开始指南

**版本**: 0.1.0  
**日期**: 2025-11-09  
**状态**: 准备开始

---

## 📋 概述

Phase 2.2 的目标是实现一个**最小可用的 DOM API**，包括：
- DOM 节点类层次结构（Node, Element, Text, Document）
- 15+ 核心 DOM API
- 基础事件系统
- QuickJS 绑定

---

## 🎯 开发目标

### 核心 API（P0 - 必须实现）
```javascript
// Document API
document.createElement(tagName)
document.createTextNode(text)
document.body
document.getElementById(id)

// Element API
element.appendChild(child)
element.insertBefore(newNode, refNode)
element.removeChild(child)
element.setAttribute(name, value)
element.getAttribute(name)
element.addEventListener(type, handler)
element.removeEventListener(type, handler)

// Node API
node.parentNode
node.childNodes
node.firstChild
node.lastChild

// Text API
textNode.data
```

---

## 🚀 快速开始

### 1. 环境准备

确保已完成 Phase 2.1（JavaScript Runtime）：
```bash
# 检查 QuickJS Runtime 是否可用
cd d:/code/C/MBink
./build/tests/test_quickjs_runtime.exe
```

### 2. 创建测试文件

创建第一个 DOM 测试：
```bash
# 创建测试文件
touch tests/test_dom_node.cpp
```

### 3. 实现 Node 基类

编辑 `core/dom/node.cpp`，实现基础方法：

```cpp
#include "node.h"
#include <algorithm>
#include <stdexcept>

namespace lightui {

Node::Node(NodeType type)
    : node_type_(type)
    , parent_node_()
    , child_nodes_()
    , is_dirty_(true) {
}

std::shared_ptr<Node> Node::AppendChild(std::shared_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("Cannot append null child");
    }
    
    // 如果已有父节点，先移除
    if (auto parent = child->GetParentNode()) {
        parent->RemoveChild(child);
    }
    
    child_nodes_.push_back(child);
    child->SetParentNode(shared_from_this());
    MarkDirty();
    
    return child;
}

// ... 实现其他方法

} // namespace lightui
```

### 4. 编写单元测试

编辑 `tests/test_dom_node.cpp`：

```cpp
#include <gtest/gtest.h>
#include "core/dom/element.h"
#include "core/dom/text.h"

using namespace lightui;

TEST(DOMNodeTest, AppendChild) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(child->GetParentNode(), parent);
    EXPECT_TRUE(parent->IsDirty());
}

TEST(DOMNodeTest, RemoveChild) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    parent->RemoveChild(child);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 0);
    EXPECT_EQ(child->GetParentNode(), nullptr);
}

// ... 更多测试
```

### 5. 更新 CMakeLists.txt

添加测试目标：

```cmake
# tests/CMakeLists.txt

# DOM Node 测试
add_executable(test_dom_node test_dom_node.cpp)
target_link_libraries(test_dom_node PRIVATE
    lightui_dom
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME DOMNodeTest COMMAND test_dom_node)
```

### 6. 编译和运行测试

```bash
# 编译
cmake --build build --target test_dom_node

# 运行测试
./build/tests/test_dom_node.exe
```

---

## 📚 开发流程

### 推荐开发顺序

#### Week 1: 基础 DOM 类
1. **Day 1-2**: Node 基类
   - 实现 AppendChild, InsertBefore, RemoveChild
   - 编写单元测试
   
2. **Day 3**: Text 节点
   - 实现 Text 类
   - 实现 data 属性
   - 编写单元测试

3. **Day 4-5**: Element 类
   - 实现属性操作（setAttribute, getAttribute）
   - 实现 className 和 id
   - 编写单元测试

4. **Day 6-7**: Document 类
   - 实现工厂方法（createElement, createTextNode）
   - 实现 body 和 documentElement
   - 实现 getElementById
   - 编写单元测试

#### Week 2: 事件系统和绑定
1. **Day 1-2**: 事件系统
   - 实现 Event 类
   - 实现 addEventListener / removeEventListener
   - 实现事件冒泡和捕获
   - 编写单元测试

2. **Day 3-5**: QuickJS 绑定
   - 绑定 Node, Element, Text, Document
   - 绑定 Event
   - 绑定全局 document 对象
   - 编写绑定测试

3. **Day 6-7**: 集成测试和文档
   - 编写集成测试
   - 编写 API 文档
   - 编写示例代码
   - 更新 README

---

## 🧪 测试策略

### 单元测试

每个类都有对应的单元测试文件：
- `tests/test_dom_node.cpp` - Node 类测试
- `tests/test_dom_element.cpp` - Element 类测试
- `tests/test_dom_text.cpp` - Text 类测试
- `tests/test_dom_document.cpp` - Document 类测试
- `tests/test_dom_event.cpp` - 事件系统测试

### 集成测试

测试完整的 DOM 操作场景：
- `tests/test_dom_integration.cpp` - DOM API 集成测试
- `tests/test_dom_bindings.cpp` - QuickJS 绑定测试

### 测试覆盖率目标

- 单元测试覆盖率 > 80%
- 所有核心 API 都有测试
- 包含边界条件和错误处理测试

---

## 📖 示例代码

### C++ 示例

```cpp
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"

using namespace lightui;

int main() {
    // 创建 Document
    auto document = std::make_shared<Document>();
    
    // 创建元素
    auto div = document->CreateElement("div");
    div->SetId("container");
    div->SetClassName("main-content");
    
    // 创建文本节点
    auto text = document->CreateTextNode("Hello, World!");
    
    // 构建 DOM 树
    div->AppendChild(text);
    document->GetBody()->AppendChild(div);
    
    // 查询元素
    auto found = document->GetElementById("container");
    std::cout << "Found: " << found->GetTagName() << std::endl;
    
    return 0;
}
```

### JavaScript 示例（通过 QuickJS）

```javascript
// 创建元素
const div = document.createElement('div');
div.id = 'container';
div.className = 'main-content';

// 创建文本节点
const text = document.createTextNode('Hello, World!');

// 构建 DOM 树
div.appendChild(text);
document.body.appendChild(div);

// 添加事件监听器
div.addEventListener('click', (event) => {
    console.log('Clicked!', event.target.id);
});

// 查询元素
const found = document.getElementById('container');
console.log('Found:', found.tagName);
```

---

## 🔧 技术要点

### 1. 内存管理

使用智能指针管理节点生命周期：

```cpp
// 使用 shared_ptr 管理节点
std::shared_ptr<Element> element = std::make_shared<Element>("div");

// 使用 weak_ptr 避免循环引用（parent_node_）
class Node {
    std::weak_ptr<Node> parent_node_;  // 不增加引用计数
    std::vector<std::shared_ptr<Node>> child_nodes_;  // 增加引用计数
};
```

### 2. QuickJS 绑定

创建 JavaScript 类：

```cpp
// 1. 定义 ClassID
static JSClassID element_class_id;

// 2. 定义 ClassDef
static JSClassDef element_class_def = {
    "Element",
    .finalizer = js_element_finalizer,
};

// 3. 注册类
JS_NewClassID(&element_class_id);
JS_NewClass(rt, element_class_id, &element_class_def);

// 4. 创建原型
JSValue proto = JS_NewObject(ctx);
JS_SetPropertyStr(ctx, proto, "appendChild", 
                  JS_NewCFunction(ctx, js_element_appendChild, "appendChild", 1));

// 5. 设置原型
JS_SetClassProto(ctx, element_class_id, proto);
```

### 3. 事件系统

实现事件冒泡：

```cpp
bool Element::DispatchEvent(std::shared_ptr<Event> event) {
    // 1. 计算传播路径
    std::vector<std::shared_ptr<Node>> path;
    for (auto node = shared_from_this(); node; node = node->GetParentNode()) {
        path.push_back(node);
    }
    
    // 2. 捕获阶段（从 Document 到 target）
    event->SetEventPhase(Event::Phase::CAPTURING_PHASE);
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (event->IsPropagationStopped()) break;
        (*it)->HandleEvent(event, true);
    }
    
    // 3. 目标阶段
    event->SetEventPhase(Event::Phase::AT_TARGET);
    HandleEvent(event, false);
    
    // 4. 冒泡阶段（从 target 到 Document）
    if (event->GetBubbles()) {
        event->SetEventPhase(Event::Phase::BUBBLING_PHASE);
        for (auto& node : path) {
            if (event->IsPropagationStopped()) break;
            node->HandleEvent(event, false);
        }
    }
    
    return !event->IsDefaultPrevented();
}
```

---

## 📝 常见问题

### Q1: 如何避免循环引用？

**A**: 使用 `weak_ptr` 存储父节点引用：

```cpp
class Node {
    std::weak_ptr<Node> parent_node_;  // 不增加引用计数
};
```

### Q2: 如何在 QuickJS 中管理 C++ 对象生命周期？

**A**: 使用 finalizer 释放 C++ 对象：

```cpp
static void js_element_finalizer(JSRuntime* rt, JSValue val) {
    auto* element_ptr = static_cast<std::shared_ptr<Element>*>(
        JS_GetOpaque(val, element_class_id));
    if (element_ptr) {
        delete element_ptr;  // 释放 shared_ptr
    }
}
```

### Q3: 如何测试事件冒泡？

**A**: 创建父子节点，添加监听器，触发事件：

```cpp
auto parent = std::make_shared<Element>("div");
auto child = std::make_shared<Element>("span");
parent->AppendChild(child);

int parent_count = 0;
parent->AddEventListener("click", [&](auto e) { parent_count++; });

auto event = std::make_shared<Event>("click");
child->DispatchEvent(event);

EXPECT_EQ(parent_count, 1);  // 事件冒泡到父节点
```

---

## 📚 参考资料

### 标准文档
- [DOM Standard](https://dom.spec.whatwg.org/) - DOM 标准规范
- [MDN Web Docs - DOM](https://developer.mozilla.org/en-US/docs/Web/API/Document_Object_Model) - DOM API 参考

### QuickJS 文档
- [QuickJS Documentation](https://bellard.org/quickjs/) - QuickJS 官方文档
- [QuickJS Source Code](https://github.com/bellard/quickjs) - QuickJS 源码

### 项目文档
- [PHASE_2_2_TASKS.md](PHASE_2_2_TASKS.md) - 任务清单
- [PHASE_2_2_PROGRESS.md](PHASE_2_2_PROGRESS.md) - 详细进度
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - 架构设计

---

## ✅ 完成标准

Phase 2.2 完成的标准：
- ✅ 所有 67 个子任务完成
- ✅ 15+ 核心 DOM API 实现并通过测试
- ✅ 事件系统实现并通过测试
- ✅ QuickJS 绑定完成
- ✅ 单元测试覆盖率 > 80%
- ✅ 集成测试通过
- ✅ 文档完整

---

## 🎯 下一步

完成 Phase 2.2 后，进入 **Phase 2.3 - 布局引擎（Yoga 集成）**：
- 集成 Yoga 布局引擎
- 实现 Flexbox 布局
- 实现样式计算
- 实现布局缓存

---

**祝开发顺利！** 🚀

