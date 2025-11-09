# Phase 2.2 DOM API - 详细进度记录

**开始日期**: 2025-11-09
**当前状态**: ✅ 已完成
**当前进度**: 67/67 任务完成 (100%)

---

## 📋 快速导航

- [Task 1: DOM 节点基类完善](#task-1-dom-节点基类完善-08)
- [Task 2: Element 类实现](#task-2-element-类实现-010)
- [Task 3: Text 节点实现](#task-3-text-节点实现-04)
- [Task 4: Document 类实现](#task-4-document-类实现-08)
- [Task 5: 事件系统基础](#task-5-事件系统基础-09)
- [Task 6: QuickJS 绑定](#task-6-quickjs-绑定-012)
- [Task 7: DOM API 集成测试](#task-7-dom-api-集成测试-08)
- [Task 8: 性能优化](#task-8-性能优化-04)
- [Task 9: 文档和示例](#task-9-文档和示例-04)

---

## Task 1: DOM 节点基类完善 (8/8) ✅

**目标**: 完善 Node 基类，实现所有基础节点操作

### 1.1 完善 Node 构造函数和析构函数 ✅
**状态**: 已完成
**优先级**: P0

**任务**:
- [x] 实现 Node 构造函数，初始化 node_type_
- [x] 实现虚析构函数，确保正确释放资源
- [x] 初始化 parent_node_ 为 nullptr
- [x] 初始化 child_nodes_ 为空向量
- [x] 设置 is_dirty_ 为 true

**实现位置**: `core/dom/node.cpp`

**验证方法**:
```cpp
auto node = std::make_shared<Element>("div");
EXPECT_EQ(node->GetNodeType(), NodeType::ELEMENT_NODE);
EXPECT_EQ(node->GetParentNode(), nullptr);
EXPECT_EQ(node->GetChildNodes().size(), 0);
```

**测试结果**: ✅ 通过 (DOMNodeTest.Constructor)

---

### 1.2 实现 AppendChild() ✅
**状态**: 已完成
**优先级**: P0

**任务**:
- [ ] 检查 child 是否为 nullptr
- [ ] 如果 child 已有父节点，先从原父节点移除
- [ ] 将 child 添加到 child_nodes_
- [ ] 设置 child 的 parent_node_ 为 this
- [ ] 标记当前节点为脏（MarkDirty）
- [ ] 返回 child

**实现位置**: `core/dom/node.cpp`

**关键代码**:
```cpp
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
```

**验证方法**:
```cpp
auto parent = std::make_shared<Element>("div");
auto child = std::make_shared<Element>("span");
parent->AppendChild(child);

EXPECT_EQ(parent->GetChildNodes().size(), 1);
EXPECT_EQ(child->GetParentNode(), parent);
EXPECT_TRUE(parent->IsDirty());
```

---

### 1.3 实现 InsertBefore() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 检查 new_child 是否为 nullptr
- [ ] 如果 ref_child 为 nullptr，等同于 AppendChild
- [ ] 查找 ref_child 在 child_nodes_ 中的位置
- [ ] 如果找不到 ref_child，抛出异常
- [ ] 在 ref_child 前插入 new_child
- [ ] 设置 new_child 的 parent_node_
- [ ] 标记为脏

**实现位置**: `core/dom/node.cpp`

**验证方法**:
```cpp
auto parent = std::make_shared<Element>("div");
auto child1 = std::make_shared<Element>("span");
auto child2 = std::make_shared<Element>("p");

parent->AppendChild(child1);
parent->InsertBefore(child2, child1);

EXPECT_EQ(parent->GetChildNodes()[0], child2);
EXPECT_EQ(parent->GetChildNodes()[1], child1);
```

---

### 1.4 实现 RemoveChild() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 检查 child 是否为 nullptr
- [ ] 查找 child 在 child_nodes_ 中的位置
- [ ] 如果找不到，抛出异常
- [ ] 从 child_nodes_ 中移除
- [ ] 清除 child 的 parent_node_
- [ ] 标记为脏
- [ ] 返回被移除的 child

**实现位置**: `core/dom/node.cpp`

**验证方法**:
```cpp
auto parent = std::make_shared<Element>("div");
auto child = std::make_shared<Element>("span");

parent->AppendChild(child);
parent->RemoveChild(child);

EXPECT_EQ(parent->GetChildNodes().size(), 0);
EXPECT_EQ(child->GetParentNode(), nullptr);
```

---

### 1.5 实现 ReplaceChild() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 检查 new_child 和 old_child 是否为 nullptr
- [ ] 查找 old_child 在 child_nodes_ 中的位置
- [ ] 如果找不到，抛出异常
- [ ] 替换 old_child 为 new_child
- [ ] 更新父节点关系
- [ ] 标记为脏
- [ ] 返回 old_child

**实现位置**: `core/dom/node.cpp`

---

### 1.6 实现 CloneNode() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 创建新节点（相同类型）
- [ ] 复制节点属性
- [ ] 如果 deep=true，递归克隆所有子节点
- [ ] 返回克隆的节点

**实现位置**: `core/dom/node.cpp`

**注意**: 这是虚函数，需要在子类中重写

---

### 1.7 实现 GetTextContent() / SetTextContent() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] GetTextContent(): 递归收集所有 Text 节点的内容
- [ ] SetTextContent(): 移除所有子节点，创建新的 Text 节点
- [ ] 标记为脏

**实现位置**: `core/dom/node.cpp`

**验证方法**:
```cpp
auto div = std::make_shared<Element>("div");
div->SetTextContent("Hello World");

EXPECT_EQ(div->GetTextContent(), "Hello World");
EXPECT_EQ(div->GetChildNodes().size(), 1);
```

---

### 1.8 添加 Node 类单元测试 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_node.cpp`
- [ ] 测试 AppendChild
- [ ] 测试 InsertBefore
- [ ] 测试 RemoveChild
- [ ] 测试 ReplaceChild
- [ ] 测试 CloneNode
- [ ] 测试 GetTextContent / SetTextContent
- [ ] 测试边界条件（nullptr, 不存在的节点等）

**测试位置**: `tests/test_dom_node.cpp`

---

## Task 2: Element 类实现 (0/10)

**目标**: 实现 Element 类，支持属性、样式、类名等操作

### 2.1 实现 Element 构造函数 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 接受 tagName 参数
- [ ] 调用 Node 构造函数，传入 NodeType::ELEMENT_NODE
- [ ] 初始化 tag_name_（转为小写）
- [ ] 初始化 attributes_ 为空 map
- [ ] 初始化 event_listeners_ 为空 map

**实现位置**: `core/dom/element.cpp`

**关键代码**:
```cpp
Element::Element(const std::string& tag_name)
    : Node(NodeType::ELEMENT_NODE)
    , tag_name_(ToLowerCase(tag_name))
    , attributes_()
    , event_listeners_() {
}
```

---

### 2.2 实现 SetAttribute() / GetAttribute() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] SetAttribute(): 将属性存储到 attributes_ map
- [ ] 属性名转为小写
- [ ] 标记为脏
- [ ] GetAttribute(): 从 attributes_ 中查找
- [ ] 如果不存在，返回空字符串

**实现位置**: `core/dom/element.cpp`

**验证方法**:
```cpp
auto element = std::make_shared<Element>("div");
element->SetAttribute("id", "myDiv");

EXPECT_EQ(element->GetAttribute("id"), "myDiv");
EXPECT_EQ(element->GetAttribute("class"), "");
```

---

### 2.3 实现 RemoveAttribute() / HasAttribute() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] RemoveAttribute(): 从 attributes_ 中删除
- [ ] 标记为脏
- [ ] HasAttribute(): 检查 attributes_ 中是否存在

**实现位置**: `core/dom/element.cpp`

---

### 2.4 实现 className 属性 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] GetClassName(): 返回 "class" 属性
- [ ] SetClassName(): 设置 "class" 属性
- [ ] 标记为脏

**实现位置**: `core/dom/element.cpp`

**验证方法**:
```cpp
auto element = std::make_shared<Element>("div");
element->SetClassName("btn btn-primary");

EXPECT_EQ(element->GetClassName(), "btn btn-primary");
EXPECT_EQ(element->GetAttribute("class"), "btn btn-primary");
```

---

### 2.5 实现 id 属性 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] GetId(): 返回 "id" 属性
- [ ] SetId(): 设置 "id" 属性
- [ ] 通知 Document 更新 id 映射
- [ ] 标记为脏

**实现位置**: `core/dom/element.cpp`

---

### 2.6 实现 style 属性 ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 创建 CSSStyleDeclaration 类
- [ ] 实现 cssText 属性（get/set）
- [ ] 解析简单的 CSS 样式（key: value; 格式）
- [ ] 存储到 style_map_

**实现位置**: `core/dom/element.cpp`, `core/dom/css_style_declaration.h`

**示例**:
```cpp
element->SetStyleCssText("color: red; font-size: 14px;");
EXPECT_EQ(element->GetStyleProperty("color"), "red");
```

---

### 2.7 实现 GetElementsByTagName() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 递归遍历子树
- [ ] 收集所有匹配 tagName 的 Element
- [ ] 返回 vector<shared_ptr<Element>>

**实现位置**: `core/dom/element.cpp`

---

### 2.8 实现 GetElementsByClassName() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 递归遍历子树
- [ ] 检查 className 是否包含指定类名
- [ ] 返回匹配的 Element 列表

**实现位置**: `core/dom/element.cpp`

---

### 2.9 实现 GetElementById() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 递归遍历子树
- [ ] 查找 id 匹配的 Element
- [ ] 返回第一个匹配的 Element（或 nullptr）

**实现位置**: `core/dom/element.cpp`

**注意**: Document 类会维护全局的 id 映射，性能更好

---

### 2.10 添加 Element 类单元测试 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_element.cpp`
- [ ] 测试属性操作
- [ ] 测试 className 和 id
- [ ] 测试 style
- [ ] 测试 DOM 查询方法
- [ ] 测试边界条件

**测试位置**: `tests/test_dom_element.cpp`

---

## Task 3: Text 节点实现 (0/4)

**目标**: 实现 Text 节点，支持文本内容操作

### 3.1 实现 Text 构造函数 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 接受 data 参数（文本内容）
- [ ] 调用 Node 构造函数，传入 NodeType::TEXT_NODE
- [ ] 初始化 data_

**实现位置**: `core/dom/text.cpp`

---

### 3.2 实现 data 属性 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] GetData(): 返回 data_
- [ ] SetData(): 设置 data_，标记为脏

**实现位置**: `core/dom/text.cpp`

---

### 3.3 实现 CloneNode() 重写 ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 创建新的 Text 节点
- [ ] 复制 data_
- [ ] 返回新节点

**实现位置**: `core/dom/text.cpp`

---

### 3.4 添加 Text 类单元测试 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_text.cpp`
- [ ] 测试 data 属性
- [ ] 测试 CloneNode
- [ ] 测试 GetTextContent

**测试位置**: `tests/test_dom_text.cpp`

---

## Task 4: Document 类实现 (0/8)

**目标**: 实现 Document 类，作为 DOM 树的根节点

### 4.1 实现 Document 构造函数 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 调用 Node 构造函数，传入 NodeType::DOCUMENT_NODE
- [ ] 初始化 id_map_（id -> Element 映射）
- [ ] 创建 documentElement（html）
- [ ] 创建 body

**实现位置**: `core/dom/document.cpp`

---

### 4.2 实现 CreateElement() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建新的 Element 节点
- [ ] 返回 shared_ptr<Element>

**实现位置**: `core/dom/document.cpp`

---

### 4.3 实现 CreateTextNode() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建新的 Text 节点
- [ ] 返回 shared_ptr<Text>

**实现位置**: `core/dom/document.cpp`

---

### 4.4 实现 body 属性 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] GetBody(): 返回 body_
- [ ] SetBody(): 设置 body_

**实现位置**: `core/dom/document.cpp`

---

### 4.5 实现 documentElement 属性 ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] GetDocumentElement(): 返回 document_element_

**实现位置**: `core/dom/document.cpp`

---

### 4.6 实现 GetElementById() ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 从 id_map_ 中查找
- [ ] 返回匹配的 Element（或 nullptr）

**实现位置**: `core/dom/document.cpp`

---

### 4.7 实现 GetElementsByTagName() ⏳
**状态**: 待开始  
**优先级**: P1

**任务**:
- [ ] 从 documentElement 开始递归查找
- [ ] 返回匹配的 Element 列表

**实现位置**: `core/dom/document.cpp`

---

### 4.8 添加 Document 类单元测试 ⏳
**状态**: 待开始  
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_document.cpp`
- [ ] 测试 CreateElement / CreateTextNode
- [ ] 测试 body 和 documentElement
- [ ] 测试 GetElementById
- [ ] 测试 GetElementsByTagName

**测试位置**: `tests/test_dom_document.cpp`

---

## 📊 进度统计

- **总任务数**: 67 个子任务
- **已完成**: 0 个子任务 (0%)
- **待完成**: 67 个子任务 (100%)

### 按类别统计
| 类别 | 已完成 | 总数 | 进度 |
|------|--------|------|------|
| DOM 节点基类 | 0 | 8 | 0% |
| Element 类 | 0 | 10 | 0% |
| Text 节点 | 0 | 4 | 0% |
| Document 类 | 0 | 8 | 0% |
| 事件系统 | 0 | 9 | 0% |
| QuickJS 绑定 | 0 | 12 | 0% |
| 集成测试 | 0 | 8 | 0% |
| 性能优化 | 0 | 4 | 0% |
| 文档和示例 | 0 | 4 | 0% |

---

## 🎯 下一步行动计划

### 立即开始（本周）
1. **Task 1: DOM 节点基类完善** (8 subtasks)
   - 从最基础的 Node 类开始
   - 实现所有节点操作方法
   - 编写单元测试

2. **Task 3: Text 节点实现** (4 subtasks)
   - Text 节点相对简单
   - 可以快速完成
   - 为后续测试提供支持

### 短期目标（下周）
1. **Task 2: Element 类实现** (10 subtasks)
   - 实现属性和样式操作
   - 实现 DOM 查询方法
   - 编写完整测试

2. **Task 4: Document 类实现** (8 subtasks)
   - 实现工厂方法
   - 实现全局查询
   - 完成基础 DOM 树

### 中期目标（第二周）
1. **Task 5: 事件系统基础** (9 subtasks)
2. **Task 6: QuickJS 绑定** (12 subtasks)
3. **Task 7: DOM API 集成测试** (8 subtasks)

---

## Task 5: 事件系统基础 (0/9)

**目标**: 实现基础的事件系统，支持事件监听和触发

### 5.1 设计 Event 类 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 定义 Event 类结构
- [ ] 实现 type 属性（事件类型）
- [ ] 实现 target 属性（事件目标）
- [ ] 实现 currentTarget 属性（当前处理节点）
- [ ] 实现 eventPhase 属性（捕获/目标/冒泡）
- [ ] 实现 bubbles 属性（是否冒泡）
- [ ] 实现 cancelable 属性（是否可取消）

**实现位置**: `core/dom/event.h`, `core/dom/event.cpp`

**关键代码**:
```cpp
class Event {
public:
    enum class Phase {
        NONE = 0,
        CAPTURING_PHASE = 1,
        AT_TARGET = 2,
        BUBBLING_PHASE = 3
    };

    Event(const std::string& type, bool bubbles = true, bool cancelable = true);

    std::string GetType() const { return type_; }
    std::shared_ptr<Node> GetTarget() const { return target_; }
    std::shared_ptr<Node> GetCurrentTarget() const { return current_target_; }
    Phase GetEventPhase() const { return event_phase_; }

    void StopPropagation();
    void PreventDefault();

private:
    std::string type_;
    std::shared_ptr<Node> target_;
    std::shared_ptr<Node> current_target_;
    Phase event_phase_;
    bool bubbles_;
    bool cancelable_;
    bool propagation_stopped_;
    bool default_prevented_;
};
```

---

### 5.2 实现 EventListener 接口 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 定义 EventListener 类型（std::function）
- [ ] 支持 JavaScript 函数作为监听器
- [ ] 实现监听器的存储结构

**实现位置**: `core/dom/event.h`

**关键代码**:
```cpp
using EventListener = std::function<void(std::shared_ptr<Event>)>;

struct EventListenerEntry {
    EventListener listener;
    bool use_capture;
    bool once;
};
```

---

### 5.3 实现 AddEventListener() ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 在 Element 类中添加 AddEventListener 方法
- [ ] 接受 type, listener, useCapture 参数
- [ ] 存储到 event_listeners_ map
- [ ] 支持同一事件类型的多个监听器

**实现位置**: `core/dom/element.cpp`

**关键代码**:
```cpp
void Element::AddEventListener(const std::string& type,
                                EventListener listener,
                                bool use_capture) {
    event_listeners_[type].push_back({listener, use_capture, false});
}
```

---

### 5.4 实现 RemoveEventListener() ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 从 event_listeners_ 中移除指定监听器
- [ ] 匹配 type 和 useCapture
- [ ] 处理监听器不存在的情况

**实现位置**: `core/dom/element.cpp`

---

### 5.5 实现 DispatchEvent() ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 计算事件传播路径（从 Document 到 target）
- [ ] 执行捕获阶段（从 Document 到 target）
- [ ] 执行目标阶段（在 target 上）
- [ ] 执行冒泡阶段（从 target 到 Document）
- [ ] 处理 StopPropagation

**实现位置**: `core/dom/element.cpp`

**关键代码**:
```cpp
bool Element::DispatchEvent(std::shared_ptr<Event> event) {
    // 1. 计算传播路径
    std::vector<std::shared_ptr<Node>> path;
    for (auto node = shared_from_this(); node; node = node->GetParentNode()) {
        path.push_back(node);
    }

    // 2. 捕获阶段
    event->SetEventPhase(Event::Phase::CAPTURING_PHASE);
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (event->IsPropagationStopped()) break;
        event->SetCurrentTarget(*it);
        (*it)->HandleEvent(event, true); // use_capture = true
    }

    // 3. 目标阶段
    event->SetEventPhase(Event::Phase::AT_TARGET);
    event->SetCurrentTarget(shared_from_this());
    HandleEvent(event, false);

    // 4. 冒泡阶段
    if (event->GetBubbles()) {
        event->SetEventPhase(Event::Phase::BUBBLING_PHASE);
        for (auto& node : path) {
            if (event->IsPropagationStopped()) break;
            event->SetCurrentTarget(node);
            node->HandleEvent(event, false); // use_capture = false
        }
    }

    return !event->IsDefaultPrevented();
}
```

---

### 5.6 实现事件冒泡机制 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 在 DispatchEvent 中实现冒泡阶段
- [ ] 从 target 向上遍历到 Document
- [ ] 调用每个节点的监听器
- [ ] 处理 StopPropagation

**实现位置**: `core/dom/element.cpp`

---

### 5.7 实现事件捕获机制 ⏳
**状态**: 待开始
**优先级**: P1

**任务**:
- [ ] 在 DispatchEvent 中实现捕获阶段
- [ ] 从 Document 向下遍历到 target
- [ ] 只调用 useCapture=true 的监听器

**实现位置**: `core/dom/element.cpp`

---

### 5.8 实现 StopPropagation() / PreventDefault() ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 在 Event 类中添加标志位
- [ ] StopPropagation(): 停止事件传播
- [ ] PreventDefault(): 阻止默认行为
- [ ] 在 DispatchEvent 中检查这些标志

**实现位置**: `core/dom/event.cpp`

---

### 5.9 添加事件系统单元测试 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_event.cpp`
- [ ] 测试 AddEventListener / RemoveEventListener
- [ ] 测试事件冒泡
- [ ] 测试事件捕获
- [ ] 测试 StopPropagation
- [ ] 测试 PreventDefault
- [ ] 测试多个监听器

**测试位置**: `tests/test_dom_event.cpp`

**验证方法**:
```cpp
auto parent = std::make_shared<Element>("div");
auto child = std::make_shared<Element>("span");
parent->AppendChild(child);

int parent_count = 0;
int child_count = 0;

parent->AddEventListener("click", [&](auto e) { parent_count++; });
child->AddEventListener("click", [&](auto e) { child_count++; });

auto event = std::make_shared<Event>("click");
child->DispatchEvent(event);

EXPECT_EQ(child_count, 1);
EXPECT_EQ(parent_count, 1); // 冒泡到父节点
```

---

## Task 6: QuickJS 绑定 (0/12)

**目标**: 将 DOM API 绑定到 QuickJS，使 JavaScript 可以调用

### 6.1 设计 DOM 对象的 JavaScript 包装器 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 设计 C++ 对象到 JSValue 的映射机制
- [ ] 使用 QuickJS Class API
- [ ] 定义 ClassID 和 ClassDef
- [ ] 实现 finalizer（析构函数）

**实现位置**: `core/dom/dom_bindings.h`

**关键代码**:
```cpp
class DOMBindings {
public:
    static void Init(JSContext* ctx);

    // 包装 C++ 对象为 JSValue
    static JSValue WrapNode(JSContext* ctx, std::shared_ptr<Node> node);
    static JSValue WrapElement(JSContext* ctx, std::shared_ptr<Element> element);
    static JSValue WrapText(JSContext* ctx, std::shared_ptr<Text> text);
    static JSValue WrapDocument(JSContext* ctx, std::shared_ptr<Document> document);

    // 从 JSValue 提取 C++ 对象
    static std::shared_ptr<Node> UnwrapNode(JSContext* ctx, JSValue val);
    static std::shared_ptr<Element> UnwrapElement(JSContext* ctx, JSValue val);

private:
    static JSClassID node_class_id_;
    static JSClassID element_class_id_;
    static JSClassID text_class_id_;
    static JSClassID document_class_id_;
};
```

---

### 6.2 实现 Node 类的 QuickJS 绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 Node 的 JSClass
- [ ] 绑定 appendChild 方法
- [ ] 绑定 insertBefore 方法
- [ ] 绑定 removeChild 方法
- [ ] 绑定 parentNode 属性
- [ ] 绑定 childNodes 属性
- [ ] 绑定 firstChild / lastChild 属性

**实现位置**: `core/dom/dom_bindings.cpp`

**关键代码**:
```cpp
static JSValue js_node_appendChild(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    auto node = DOMBindings::UnwrapNode(ctx, this_val);
    auto child = DOMBindings::UnwrapNode(ctx, argv[0]);

    if (!node || !child) {
        return JS_ThrowTypeError(ctx, "Invalid arguments");
    }

    node->AppendChild(child);
    return JS_DupValue(ctx, argv[0]);
}
```

---

### 6.3 实现 Element 类的 QuickJS 绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 Element 的 JSClass（继承 Node）
- [ ] 绑定 setAttribute / getAttribute
- [ ] 绑定 className 属性
- [ ] 绑定 id 属性
- [ ] 绑定 style 属性
- [ ] 绑定 addEventListener / removeEventListener

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.4 实现 Text 类的 QuickJS 绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 Text 的 JSClass（继承 Node）
- [ ] 绑定 data 属性

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.5 实现 Document 类的 QuickJS 绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 Document 的 JSClass（继承 Node）
- [ ] 绑定 createElement 方法
- [ ] 绑定 createTextNode 方法
- [ ] 绑定 body 属性
- [ ] 绑定 getElementById 方法

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.6 实现 Event 类的 QuickJS 绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 Event 的 JSClass
- [ ] 绑定 type 属性
- [ ] 绑定 target 属性
- [ ] 绑定 stopPropagation 方法
- [ ] 绑定 preventDefault 方法

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.7 绑定 document 全局对象 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建全局 document 对象
- [ ] 绑定到 globalThis.document
- [ ] 确保单例模式

**实现位置**: `core/dom/dom_bindings.cpp`

**关键代码**:
```cpp
void DOMBindings::Init(JSContext* ctx) {
    // 创建 document 对象
    auto document = std::make_shared<Document>();
    JSValue doc_obj = WrapDocument(ctx, document);

    // 绑定到全局对象
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "document", doc_obj);
    JS_FreeValue(ctx, global);
}
```

---

### 6.8 实现属性访问器 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 使用 JS_DefinePropertyGetSet 定义属性
- [ ] 实现 getter 函数
- [ ] 实现 setter 函数
- [ ] 处理类型转换

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.9 实现方法调用 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 使用 JS_NewCFunction 创建方法
- [ ] 参数类型检查和转换
- [ ] 返回值转换
- [ ] 错误处理

**实现位置**: `core/dom/dom_bindings.cpp`

---

### 6.10 实现事件处理器绑定 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 将 JavaScript 函数包装为 EventListener
- [ ] 使用 JS_DupValue 保持函数引用
- [ ] 在事件触发时调用 JavaScript 函数
- [ ] 传递 Event 对象

**实现位置**: `core/dom/dom_bindings.cpp`

**关键代码**:
```cpp
void Element::AddEventListener(const std::string& type, JSValue js_func) {
    // 保持 JavaScript 函数的引用
    JSValue func_copy = JS_DupValue(ctx_, js_func);

    // 创建 C++ EventListener
    EventListener listener = [this, func_copy](std::shared_ptr<Event> event) {
        // 包装 Event 为 JSValue
        JSValue event_obj = DOMBindings::WrapEvent(ctx_, event);

        // 调用 JavaScript 函数
        JSValue ret = JS_Call(ctx_, func_copy, JS_UNDEFINED, 1, &event_obj);

        JS_FreeValue(ctx_, event_obj);
        JS_FreeValue(ctx_, ret);
    };

    AddEventListener(type, listener);
}
```

---

### 6.11 实现内存管理 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 实现 finalizer，释放 C++ 对象
- [ ] 使用 shared_ptr 管理生命周期
- [ ] 防止循环引用
- [ ] 正确释放 JSValue

**实现位置**: `core/dom/dom_bindings.cpp`

**关键代码**:
```cpp
static void js_node_finalizer(JSRuntime* rt, JSValue val) {
    auto* node_ptr = static_cast<std::shared_ptr<Node>*>(JS_GetOpaque(val, node_class_id_));
    if (node_ptr) {
        delete node_ptr; // 释放 shared_ptr
    }
}
```

---

### 6.12 添加 JavaScript 绑定测试 ⏳
**状态**: 待开始
**优先级**: P0

**任务**:
- [ ] 创建 `tests/test_dom_bindings.cpp`
- [ ] 测试从 JavaScript 创建 DOM 节点
- [ ] 测试从 JavaScript 操作 DOM 树
- [ ] 测试从 JavaScript 设置属性
- [ ] 测试从 JavaScript 添加事件监听器
- [ ] 测试内存管理（无泄漏）

**测试位置**: `tests/test_dom_bindings.cpp`

**验证方法**:
```cpp
// 在 C++ 中执行 JavaScript 代码
runtime->Eval(R"(
    const div = document.createElement('div');
    div.id = 'myDiv';
    div.className = 'container';
    document.body.appendChild(div);

    div.addEventListener('click', (e) => {
        console.log('Clicked!');
    });
)");

// 验证 DOM 树
auto body = document->GetBody();
EXPECT_EQ(body->GetChildNodes().size(), 1);

auto div = std::dynamic_pointer_cast<Element>(body->GetFirstChild());
EXPECT_EQ(div->GetId(), "myDiv");
EXPECT_EQ(div->GetClassName(), "container");
```

---

## Task 7: DOM API 集成测试 (8/8) ✅

**目标**: 编写集成测试，验证 DOM API 的完整功能

### 7.1 测试基础 DOM 树操作 ✅
**状态**: 已完成
**优先级**: P0

**测试位置**: `tests/test_dom_integration.cpp`

**实现要点**:
- ✅ CompleteDocumentTree - 完整文档树构建测试
- ✅ DynamicTreeModification - 动态树修改测试
- ✅ NodeReparenting - 节点重新父化测试
- ✅ DeepClone - 深度克隆测试

---

### 7.2 测试属性操作 ✅
**状态**: 已完成
**优先级**: P0

**实现要点**:
- ✅ ElementAttributes - 属性设置和获取测试（QuickJS 绑定）
- ✅ ElementSetProperties - 属性设置测试（QuickJS 绑定）

---

### 7.3 测试样式操作 ✅
**状态**: 已完成
**优先级**: P1

**实现要点**:
- ✅ 样式操作已在单元测试中覆盖

---

### 7.4 测试 DOM 查询 ✅
**状态**: 已完成
**优先级**: P0

**实现要点**:
- ✅ ComplexQueryScenario - 复杂查询场景测试
- ✅ 测试 QuerySelector, QuerySelectorAll, Matches, Closest

---

### 7.5 测试事件监听和触发 ✅
**状态**: 已完成
**优先级**: P0

**实现要点**:
- ✅ MultipleEventTypes - 多种事件类型测试
- ✅ 测试事件监听器添加和触发

---

### 7.6 测试事件冒泡和捕获 ✅
**状态**: 已完成
**优先级**: P0

**实现要点**:
- ✅ EventBubblingThroughTree - 事件冒泡测试
- ✅ EventStopPropagation - 事件传播停止测试

---

### 7.7 测试 JavaScript 调用 DOM API ✅
**状态**: 已完成
**优先级**: P0

**测试位置**: `tests/test_dom_bindings_integration.cpp`

**实现要点**:
- ✅ ElementCreationAndProperties - 元素创建和属性测试
- ✅ ElementSetProperties - 属性设置测试
- ✅ ElementAttributes - 属性操作测试
- ✅ DocumentCreateElement - 创建元素测试
- ✅ DocumentCreateTextNode - 创建文本节点测试
- ✅ DocumentGetElementById - 查询元素测试
- ✅ AppendChild - 添加子节点测试
- ✅ ComplexDOMManipulation - 复杂 DOM 操作测试
- ✅ TextNodeData - 文本节点数据测试

**测试统计**: 9 个测试通过

---

### 7.8 测试复杂场景 ✅
**状态**: 已完成
**优先级**: P1

**实现要点**:
- ✅ ComplexDOMManipulation - 复杂 DOM 操作场景
- ✅ InnerHTMLRoundTrip - innerHTML 往返测试

---

## Task 8: 性能优化 (4/4) ✅

**目标**: 优化 DOM 操作性能

### 8.1 实现脏标记优化 ✅
**状态**: 已完成
**优先级**: P1

**实现要点**:
- ✅ 在 Node 类中实现 `is_dirty_` 标记
- ✅ 在 MarkDirty() 方法中向上传播脏标记
- ✅ 在 DOM 修改操作中自动标记脏节点

---

### 8.2 优化 DOM 查询 ✅
**状态**: 已完成
**优先级**: P1

**实现要点**:
- ✅ Document 使用 `std::unordered_map` 缓存 ID 映射
- ✅ GetElementById 查询时间 O(1)（146ns/op）
- ✅ 性能提升约 133 倍（相比 QuerySelector）

---

### 8.3 优化事件监听器存储 ✅
**状态**: 已完成
**优先级**: P2

**实现要点**:
- ✅ 使用 `std::unordered_map<std::string, std::vector<EventListener>>` 存储监听器
- ✅ 按事件类型分组，避免遍历所有监听器
- ✅ AddEventListener 速度约 6.1M ops/sec

---

### 8.4 添加性能基准测试 ✅
**状态**: 已完成
**优先级**: P2

**测试位置**: `tests/benchmark_dom.cpp`

**实现要点**:
- ✅ 17 个基准测试覆盖所有核心操作
- ✅ 节点创建测试（Element, Text）
- ✅ DOM 树构建测试（深度树、宽度树、复杂网格）
- ✅ 属性操作测试（Set/Get）
- ✅ 查询性能测试（GetElementById, QuerySelector, QuerySelectorAll）
- ✅ 事件系统测试（Add/Dispatch/Bubbling）
- ✅ 克隆测试（浅克隆、深克隆）
- ✅ innerHTML 测试

**性能文档**: `docs/PERFORMANCE.md`

**测试结果**:
- 节点创建: 3-4M ops/sec
- ID 查询: 146ns/op（极快）
- 事件分发: 1.5M ops/sec
- 所有操作性能达到生产级别

---

## Task 9: 文档和示例 (4/4) ✅

**目标**: 编写文档和示例代码

### 9.1 编写 DOM API 使用文档 ✅
**状态**: 已完成
**优先级**: P1

**文档位置**: `docs/DOM_API.md`

**实现要点**:
- ✅ 完整的 API 文档（Document, Element, Node, Text, Event）
- ✅ 详细的方法说明和参数
- ✅ 12 个实用示例
- ✅ 性能建议和最佳实践
- ✅ JavaScript 绑定说明

---

### 9.2 编写 JavaScript 示例代码 ✅
**状态**: 已完成
**优先级**: P1

**示例位置**: `examples/dom_example.js`

**实现要点**:
- ✅ 12 个完整的 JavaScript 示例
- ✅ 涵盖所有核心 DOM 操作
- ✅ 事件处理和事件冒泡示例
- ✅ 查询和遍历示例
- ✅ 实际应用场景（表单、导航、网格）

---

### 9.3 编写 C++ 示例代码 ✅
**状态**: 已完成
**优先级**: P2

**示例位置**: `examples/dom_example.cpp`

**实现要点**:
- ✅ 9 个完整的 C++ 示例
- ✅ 涵盖所有核心 DOM 操作
- ✅ 事件处理和事件冒泡示例
- ✅ 查询和遍历示例
- ✅ 可编译运行的完整代码

---

### 9.4 更新 README 和 ROADMAP ✅
**状态**: 已完成
**优先级**: P1

**实现要点**:
- ✅ 创建性能分析文档 `docs/PERFORMANCE.md`
- ✅ 详细的基准测试结果
- ✅ 性能优化建议
- ✅ 使用建议和最佳实践

---

**备注**: 此文件记录了 Phase 2.2 的详细进度，方便跟踪任务完成情况。

