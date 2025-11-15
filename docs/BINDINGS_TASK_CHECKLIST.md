# JavaScript 绑定任务清单

**日期**: 2025-11-15  
**目标**: 逐步完成所有 JavaScript 绑定

---

## 阶段 1: 核心 Node API (15 个绑定)

### Node 属性绑定 (6 个)

- [ ] **childNodes** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_child_nodes`
  - C++ API: `Node::GetChildNodes()`
  - 返回: JSValue 数组
  - 测试: DOM 操作测试 (19 个测试)

- [ ] **firstChild** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_first_child`
  - C++ API: `Node::GetFirstChild()`
  - 返回: JSValue (Element/Text/null)
  - 测试: 节点遍历测试

- [ ] **lastChild** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_last_child`
  - C++ API: `Node::GetLastChild()`
  - 返回: JSValue (Element/Text/null)
  - 测试: 节点遍历测试

- [ ] **nextSibling** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_next_sibling`
  - C++ API: `Node::GetNextSibling()`
  - 返回: JSValue (Element/Text/null)
  - 测试: 节点遍历测试

- [ ] **previousSibling** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_previous_sibling`
  - C++ API: `Node::GetPreviousSibling()`
  - 返回: JSValue (Element/Text/null)
  - 测试: 节点遍历测试

- [ ] **nodeType** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_node_type`
  - C++ API: `Node::GetNodeType()`
  - 返回: JSValue (number: 1=Element, 3=Text, 9=Document)
  - 测试: 节点类型测试

### Node 方法绑定 (3 个)

- [ ] **cloneNode(deep)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_clone_node`
  - C++ API: `Element::CloneNode(bool deep)`
  - 参数: deep (boolean)
  - 返回: JSValue (cloned Element)
  - 测试: cloneNode 测试 (2 个)

- [ ] **contains(node)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_contains`
  - C++ API: `Node::Contains(std::shared_ptr<Node> other)`
  - 参数: node (Element/Text)
  - 返回: JSValue (boolean)
  - 测试: contains 测试 (3 个)

- [ ] **hasChildNodes()**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_has_child_nodes`
  - C++ API: `Node::GetChildNodes().size() > 0`
  - 返回: JSValue (boolean)
  - 测试: hasChildNodes 测试 (2 个)

### Element 属性操作 (2 个)

- [ ] **hasAttribute(name)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_has_attribute`
  - C++ API: `Element::HasAttribute(const std::string& name)`
  - 参数: name (string)
  - 返回: JSValue (boolean)
  - 测试: hasAttribute 测试

- [ ] **removeAttribute(name)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_remove_attribute`
  - C++ API: `Element::RemoveAttribute(const std::string& name)`
  - 参数: name (string)
  - 返回: JSValue (undefined)
  - 测试: removeAttribute 测试

### HTML 内容 (4 个)

- [ ] **innerHTML** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_inner_html`
  - C++ API: `Element::GetInnerHTML()`
  - 返回: JSValue (string)
  - 测试: innerHTML getter 测试 (已通过)

- [ ] **innerHTML** (setter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_set_inner_html`
  - C++ API: `Element::SetInnerHTML(const std::string& html)`
  - 参数: html (string)
  - 返回: JSValue (undefined)
  - 测试: innerHTML setter 测试 (7 个)

- [ ] **outerHTML** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_outer_html`
  - C++ API: `Element::GetOuterHTML()`
  - 返回: JSValue (string)
  - 测试: outerHTML getter 测试

- [ ] **outerHTML** (setter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_set_outer_html`
  - C++ API: `Element::SetOuterHTML(const std::string& html)`
  - 参数: html (string)
  - 返回: JSValue (undefined)
  - 测试: outerHTML setter 测试

### 注册到 Element 原型

- [ ] 更新 `js_element_proto_funcs` 数组
  - 添加所有新的属性和方法
  - 确保正确的 getter/setter 配对

---

## 阶段 2: 查询选择器 API (8 个绑定)

### Element 查询方法 (4 个)

- [ ] **querySelector(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_query_selector`
  - C++ API: `Element::QuerySelector(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (Element/null)
  - 测试: querySelector 测试 (5 个)

- [ ] **querySelectorAll(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_query_selector_all`
  - C++ API: `Element::QuerySelectorAll(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (array of Elements)
  - 测试: querySelectorAll 测试 (3 个)

- [ ] **matches(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_matches`
  - C++ API: `Element::Matches(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (boolean)
  - 测试: matches 测试 (4 个)

- [ ] **closest(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_closest`
  - C++ API: `Element::Closest(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (Element/null)
  - 测试: closest 测试 (4 个)

### Document 查询方法 (4 个)

- [ ] **document.querySelector(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_document_query_selector`
  - C++ API: `Document::QuerySelector(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (Element/null)
  - 测试: document.querySelector 测试

- [ ] **document.querySelectorAll(selector)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_document_query_selector_all`
  - C++ API: `Document::QuerySelectorAll(const std::string& selector)`
  - 参数: selector (string)
  - 返回: JSValue (array of Elements)
  - 测试: document.querySelectorAll 测试

- [ ] **getElementsByClassName(className)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_document_get_elements_by_class_name`
  - C++ API: `Document::GetElementsByClassName(const std::string& className)` (需要实现)
  - 参数: className (string)
  - 返回: JSValue (array of Elements)
  - 测试: getElementsByClassName 测试

- [ ] **getElementsByTagName(tagName)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_document_get_elements_by_tag_name`
  - C++ API: `Document::GetElementsByTagName(const std::string& tagName)` (需要实现)
  - 参数: tagName (string)
  - 返回: JSValue (array of Elements)
  - 测试: getElementsByTagName 测试

### 注册到原型

- [ ] 更新 `js_element_proto_funcs` 数组
- [ ] 更新 `js_document_proto_funcs` 数组

---

## 阶段 3: 对象属性 API (20 个绑定)

### DOMTokenList 类绑定 (7 个)

- [ ] 创建 `core/dom/dom_token_list_bindings.cpp`
- [ ] 创建 `core/dom/dom_token_list_bindings.h`
- [ ] 实现 DOMTokenList 类绑定
  - [ ] `add(token1, token2, ...)`
  - [ ] `remove(token1, token2, ...)`
  - [ ] `toggle(token, force?)`
  - [ ] `contains(token)`
  - [ ] `item(index)`
  - [ ] `length` (getter)
  - [ ] 初始化函数 `InitDOMTokenListClass()`

### CSSStyleDeclaration 类绑定 (8 个)

- [ ] 创建 `core/dom/css_style_declaration_bindings.cpp`
- [ ] 创建 `core/dom/css_style_declaration_bindings.h`
- [ ] 实现 CSSStyleDeclaration 类绑定
  - [ ] `setProperty(property, value, priority?)`
  - [ ] `getPropertyValue(property)`
  - [ ] `removeProperty(property)`
  - [ ] `getPropertyPriority(property)`
  - [ ] `cssText` (getter/setter)
  - [ ] `length` (getter)
  - [ ] `item(index)`
  - [ ] 初始化函数 `InitCSSStyleDeclarationClass()`

### DOMStringMap 类绑定 (5 个)

- [ ] 创建 `core/dom/dom_string_map_bindings.cpp`
- [ ] 创建 `core/dom/dom_string_map_bindings.h`
- [ ] 实现 DOMStringMap 类绑定
  - [ ] `get(name)`
  - [ ] `set(name, value)`
  - [ ] `remove(name)`
  - [ ] `has(name)`
  - [ ] 初始化函数 `InitDOMStringMapClass()`

### Element 对象属性 (3 个)

- [ ] **classList** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_class_list`
  - C++ API: `Element::GetClassList()`
  - 返回: JSValue (DOMTokenList object)
  - 测试: classList 测试 (7 个)

- [ ] **style** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_style`
  - C++ API: `Element::GetStyleDeclaration()`
  - 返回: JSValue (CSSStyleDeclaration object)
  - 测试: style 测试 (8 个)

- [ ] **dataset** (getter)
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_get_dataset`
  - C++ API: `Element::GetDataset()`
  - 返回: JSValue (DOMStringMap object)
  - 测试: dataset 测试 (5 个)

### 集成到 DOMBindings

- [ ] 在 `DOMBindings::Init()` 中调用新的初始化函数
- [ ] 更新 `core/CMakeLists.txt` 添加新文件

---

## 阶段 4: Event 系统完善 (10 个绑定)

### Event 构造函数

- [ ] **new Event(type, options)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_event_constructor`
  - C++ API: `std::make_shared<Event>(type, bubbles, cancelable)`
  - 参数: type (string), options (object with bubbles, cancelable)
  - 返回: JSValue (Event object)
  - 测试: Event 构造测试

### Event 属性 (7 个)

- [ ] **target** (getter)
  - 函数: `js_event_get_target`
  - C++ API: `Event::GetTarget()`
  - 返回: JSValue (Element)

- [ ] **currentTarget** (getter)
  - 函数: `js_event_get_current_target`
  - C++ API: `Event::GetCurrentTarget()`
  - 返回: JSValue (Element)

- [ ] **bubbles** (getter)
  - 函数: `js_event_get_bubbles`
  - C++ API: `Event::GetBubbles()`
  - 返回: JSValue (boolean)

- [ ] **cancelable** (getter)
  - 函数: `js_event_get_cancelable`
  - C++ API: `Event::GetCancelable()`
  - 返回: JSValue (boolean)

- [ ] **defaultPrevented** (getter)
  - 函数: `js_event_get_default_prevented`
  - C++ API: `Event::IsDefaultPrevented()`
  - 返回: JSValue (boolean)

- [ ] **eventPhase** (getter)
  - 函数: `js_event_get_event_phase`
  - C++ API: `Event::GetEventPhase()`
  - 返回: JSValue (number: 0=NONE, 1=CAPTURING, 2=AT_TARGET, 3=BUBBLING)

- [ ] **timeStamp** (getter)
  - 函数: `js_event_get_time_stamp`
  - C++ API: `Event::GetTimeStamp()`
  - 返回: JSValue (number)

### Element 事件方法 (2 个)

- [ ] **dispatchEvent(event)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_element_dispatch_event`
  - C++ API: `Element::DispatchEvent(std::shared_ptr<Event> event)`
  - 参数: event (Event object)
  - 返回: JSValue (boolean - true if not cancelled)
  - 测试: dispatchEvent 测试

- [ ] 完善 **addEventListener** 选项支持
  - 已支持 capture 和 once
  - 需要验证是否正确工作

### 注册到原型

- [ ] 更新 `js_event_proto_funcs` 数组
- [ ] 注册 Event 构造函数到全局对象

---

## 阶段 5: 动画 API (2 个绑定)

### TaskScheduler 增强

- [ ] 在 `core/event/task_scheduler.h` 中添加
  - [ ] `RequestAnimationFrame(callback)` 方法
  - [ ] `CancelAnimationFrame(id)` 方法
  - [ ] 动画帧队列管理

- [ ] 在 `core/event/task_scheduler.cpp` 中实现
  - [ ] RAF 回调队列
  - [ ] 帧时间戳管理
  - [ ] 取消机制

### JavaScript 绑定

- [ ] **requestAnimationFrame(callback)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_request_animation_frame`
  - 绑定为全局函数
  - 参数: callback (function)
  - 返回: JSValue (number - frame ID)
  - 测试: requestAnimationFrame 测试 (3 个)

- [ ] **cancelAnimationFrame(id)**
  - 文件: `core/dom/dom_bindings.cpp`
  - 函数: `js_cancel_animation_frame`
  - 绑定为全局函数
  - 参数: id (number)
  - 返回: JSValue (undefined)
  - 测试: cancelAnimationFrame 测试

### 注册到全局对象

- [ ] 在 `DOMBindings::Init()` 中注册全局函数

---

## 阶段 6: 其他 API (11 个绑定) - 可选

### Document 属性 (3 个)

- [ ] **document.head** (getter)
- [ ] **document.documentElement** (getter)
- [ ] **document.title** (getter/setter)

### Node/Element 其他 (2 个)

- [ ] **nodeName** (getter)
- [ ] **element.attributes** (getter)

### Document 其他方法 (6 个)

- [ ] **createDocumentFragment()**
- [ ] **createComment(text)**
- [ ] **createAttribute(name)**
- [ ] **createEvent(type)**
- [ ] **activeElement** (getter)
- [ ] **readyState** (getter)

---

## 测试验证清单

### 每个阶段完成后

- [ ] 编译项目: `cmake --build build --target comprehensive_test_app --config Release`
- [ ] 运行测试: `./build/bin/Release/comprehensive_test_app.exe`
- [ ] 记录通过率
- [ ] 检查失败的测试
- [ ] 修复问题
- [ ] 重新测试直到达到目标通过率

### 最终验证

- [ ] 总体通过率 ≥ 90%
- [ ] 所有核心 API 测试通过
- [ ] 创建 Preact Hello World 示例
- [ ] 运行 Preact 示例验证

---

## Git 提交清单

### 每个阶段完成后

- [ ] `git add .`
- [ ] `git commit -m "feat: Complete Stage X - [description]"`
- [ ] 更新文档
- [ ] 记录问题和解决方案

---

**开始日期**: 2025-11-15  
**预计完成日期**: 2025-11-19 (4.5 天)

