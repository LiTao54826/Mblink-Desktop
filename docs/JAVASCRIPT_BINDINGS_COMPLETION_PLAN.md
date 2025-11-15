# JavaScript 绑定完善计划

**日期**: 2025-11-15  
**目标**: 将所有已实现的 C++ DOM API 暴露到 JavaScript  
**当前状态**: 23% 测试通过率 → 目标 90%+ 通过率

---

## 📊 当前绑定状态分析

### 已绑定的 API (14 个)

#### Element 属性 (6 个)
- ✅ `tagName` (getter)
- ✅ `id` (getter/setter)
- ✅ `className` (getter/setter)
- ✅ `textContent` (getter/setter)
- ✅ `parentNode` (getter)
- ✅ `children` (getter)

#### Element 方法 (8 个)
- ✅ `getAttribute(name)`
- ✅ `setAttribute(name, value)`
- ✅ `appendChild(child)`
- ✅ `removeChild(child)`
- ✅ `replaceChild(newChild, oldChild)`
- ✅ `insertBefore(newNode, referenceNode)`
- ✅ `addEventListener(type, listener)`
- ✅ `removeEventListener(type, listenerId)`

#### Document 属性 (1 个)
- ✅ `body` (getter)

#### Document 方法 (3 个)
- ✅ `createElement(tagName)`
- ✅ `createTextNode(data)`
- ✅ `getElementById(id)`

#### Event 属性 (1 个)
- ✅ `type` (getter)

#### Event 方法 (2 个)
- ✅ `stopPropagation()`
- ✅ `preventDefault()`

---

## 🔴 缺失的 API (76 个)

### 优先级 1: 核心 Node API (15 个) - 最高优先级

#### Node 属性 (6 个)
- ❌ `childNodes` (getter) - 返回 NodeList/数组
- ❌ `firstChild` (getter)
- ❌ `lastChild` (getter)
- ❌ `nextSibling` (getter)
- ❌ `previousSibling` (getter)
- ❌ `nodeType` (getter)

**C++ 实现位置**:
- `Node::GetChildNodes()` - core/dom/node.h:113
- `Node::GetFirstChild()` - core/dom/node.h:119
- `Node::GetLastChild()` - core/dom/node.h:125
- `Node::GetNextSibling()` - core/dom/node.h:131
- `Node::GetPreviousSibling()` - core/dom/node.h:137
- `Node::GetNodeType()` - core/dom/node.h:95

#### Node 方法 (3 个)
- ❌ `cloneNode(deep)`
- ❌ `contains(node)`
- ❌ `hasChildNodes()`

**C++ 实现位置**:
- `Element::CloneNode()` - core/dom/element.h:324
- `Node::Contains()` - core/dom/node.h:183
- `Node::GetChildNodes().size() > 0` - 需要包装

#### Element 属性操作 (2 个)
- ❌ `hasAttribute(name)`
- ❌ `removeAttribute(name)`

**C++ 实现位置**:
- `Element::HasAttribute()` - core/dom/element.h:96
- `Element::RemoveAttribute()` - core/dom/element.h:108

#### HTML 内容 (4 个)
- ❌ `innerHTML` (getter/setter)
- ❌ `outerHTML` (getter/setter)

**C++ 实现位置**:
- `Element::GetInnerHTML()` - core/dom/element.h:344
- `Element::SetInnerHTML()` - core/dom/element.h:350
- `Element::GetOuterHTML()` - core/dom/element.h:356
- `Element::SetOuterHTML()` - core/dom/element.h:362

---

### 优先级 2: 查询选择器 API (8 个) - 高优先级

#### Element 查询方法 (4 个)
- ❌ `querySelector(selector)`
- ❌ `querySelectorAll(selector)`
- ❌ `matches(selector)`
- ❌ `closest(selector)`

**C++ 实现位置**:
- `Element::QuerySelector()` - core/dom/element.h:294
- `Element::QuerySelectorAll()` - core/dom/element.h:301
- `Element::Matches()` - core/dom/element.h:308
- `Element::Closest()` - core/dom/element.h:315

#### Document 查询方法 (4 个)
- ❌ `document.querySelector(selector)`
- ❌ `document.querySelectorAll(selector)`
- ❌ `getElementsByClassName(className)`
- ❌ `getElementsByTagName(tagName)`

**C++ 实现位置**:
- `Document::QuerySelector()` - 继承自 Element
- `Document::QuerySelectorAll()` - 继承自 Element
- `Document::GetElementsByClassName()` - 需要实现
- `Document::GetElementsByTagName()` - 需要实现

---

### 优先级 3: 对象属性 API (30 个) - 高优先级

#### classList 对象 (7 个)
- ❌ `classList.add(token1, token2, ...)`
- ❌ `classList.remove(token1, token2, ...)`
- ❌ `classList.toggle(token, force?)`
- ❌ `classList.contains(token)`
- ❌ `classList.item(index)`
- ❌ `classList.length` (getter)
- ❌ `classList` (getter) - 返回 DOMTokenList 对象

**C++ 实现位置**:
- `Element::GetClassList()` - core/dom/element.h:166
- `DOMTokenList` 类 - core/dom/dom_token_list.h

#### style 对象 (8 个)
- ❌ `style.setProperty(property, value, priority?)`
- ❌ `style.getPropertyValue(property)`
- ❌ `style.removeProperty(property)`
- ❌ `style.getPropertyPriority(property)`
- ❌ `style.cssText` (getter/setter)
- ❌ `style.length` (getter)
- ❌ `style.item(index)`
- ❌ `style` (getter) - 返回 CSSStyleDeclaration 对象

**C++ 实现位置**:
- `Element::GetStyleDeclaration()` - core/dom/element.h:198
- `CSSStyleDeclaration` 类 - core/dom/css_style_declaration.h

#### dataset 对象 (5 个)
- ❌ `dataset.get(name)`
- ❌ `dataset.set(name, value)`
- ❌ `dataset.remove(name)`
- ❌ `dataset.has(name)`
- ❌ `dataset` (getter) - 返回 DOMStringMap 对象

**C++ 实现位置**:
- `Element::GetDataset()` - core/dom/element.h:219
- `DOMStringMap` 类 - core/dom/dom_string_map.h

#### Document 属性 (3 个)
- ❌ `document.head` (getter)
- ❌ `document.documentElement` (getter)
- ❌ `document.title` (getter/setter)

**C++ 实现位置**:
- `Document::GetHead()` - core/dom/document.h
- `Document::GetDocumentElement()` - core/dom/document.h
- `Document::GetTitle()` / `SetTitle()` - core/dom/document.h

---

### 优先级 4: Event 系统完善 (10 个) - 中优先级

#### Event 构造函数
- ❌ `new Event(type, {bubbles, cancelable})`

**需要实现**: JavaScript 构造函数绑定

#### Event 属性 (7 个)
- ❌ `event.target` (getter)
- ❌ `event.currentTarget` (getter)
- ❌ `event.bubbles` (getter)
- ❌ `event.cancelable` (getter)
- ❌ `event.defaultPrevented` (getter)
- ❌ `event.eventPhase` (getter)
- ❌ `event.timeStamp` (getter)

**C++ 实现位置**:
- `Event::GetTarget()` - core/event/event.h
- `Event::GetCurrentTarget()` - core/event/event.h
- `Event::GetBubbles()` - core/event/event.h
- `Event::GetCancelable()` - core/event/event.h
- `Event::IsDefaultPrevented()` - core/event/event.h
- `Event::GetEventPhase()` - core/event/event.h
- `Event::GetTimeStamp()` - core/event/event.h

#### Element 事件方法 (2 个)
- ❌ `dispatchEvent(event)`
- ❌ `addEventListener` 选项支持 (已部分实现，需要完善)

**C++ 实现位置**:
- `Element::DispatchEvent()` - core/dom/element.h:285
- `Element::AddEventListener()` - core/dom/element.h:270 (支持 capture 和 once)

---

### 优先级 5: 动画 API (2 个) - 中优先级

- ❌ `requestAnimationFrame(callback)`
- ❌ `cancelAnimationFrame(id)`

**需要实现**: 需要在 TaskScheduler 中添加动画帧支持

---

### 优先级 6: 其他 API (11 个) - 低优先级

#### Text 节点 (2 个)
- ❌ `text.data` (getter/setter) - 已实现但可能需要验证
- ❌ `text.length` (getter)

#### Node 名称 (1 个)
- ❌ `nodeName` (getter)

#### Element 其他 (2 个)
- ❌ `element.attributes` (getter) - 返回 NamedNodeMap
- ❌ `element.namespaceURI` (getter)

#### Document 其他 (6 个)
- ❌ `document.createDocumentFragment()`
- ❌ `document.createComment(text)`
- ❌ `document.createAttribute(name)`
- ❌ `document.createEvent(type)`
- ❌ `document.activeElement` (getter)
- ❌ `document.readyState` (getter)

---

## 📅 实施计划

### 第 1 阶段: 核心 Node API (1 天)

**目标**: 实现 Node 属性和方法绑定，预计通过率提升到 50%

**任务**:
1. 绑定 Node 属性 (6 个)
   - `childNodes`, `firstChild`, `lastChild`
   - `nextSibling`, `previousSibling`, `nodeType`

2. 绑定 Node 方法 (3 个)
   - `cloneNode(deep)`
   - `contains(node)`
   - `hasChildNodes()`

3. 绑定 Element 属性操作 (2 个)
   - `hasAttribute(name)`
   - `removeAttribute(name)`

4. 绑定 HTML 内容 (4 个)
   - `innerHTML` (getter/setter)
   - `outerHTML` (getter/setter)

**文件修改**:
- `core/dom/dom_bindings.cpp` - 添加绑定函数
- `core/dom/dom_bindings.h` - 添加声明（如需要）

**测试验证**:
- 运行 `comprehensive_test_app`
- 预期 DOM 操作测试通过率: 80%+
- 预期 HTML 内容测试通过率: 80%+

---

### 第 2 阶段: 查询选择器 API (0.5 天)

**目标**: 实现查询选择器绑定，预计通过率提升到 65%

**任务**:
1. 绑定 Element 查询方法 (4 个)
   - `querySelector(selector)`
   - `querySelectorAll(selector)`
   - `matches(selector)`
   - `closest(selector)`

2. 绑定 Document 查询方法 (4 个)
   - `document.querySelector(selector)`
   - `document.querySelectorAll(selector)`
   - `getElementsByClassName(className)`
   - `getElementsByTagName(tagName)`

**文件修改**:
- `core/dom/dom_bindings.cpp` - 添加查询方法绑定

**测试验证**:
- 预期查询选择器测试通过率: 90%+

---

### 第 3 阶段: 对象属性 API (1.5 天)

**目标**: 实现 classList, style, dataset 对象绑定，预计通过率提升到 80%

**任务**:
1. 创建 DOMTokenList 类绑定 (7 个方法)
   - 新建 `core/dom/dom_token_list_bindings.cpp`
   - 绑定 `add`, `remove`, `toggle`, `contains`, `item`, `length`

2. 创建 CSSStyleDeclaration 类绑定 (8 个方法)
   - 新建 `core/dom/css_style_declaration_bindings.cpp`
   - 绑定 `setProperty`, `getPropertyValue`, `removeProperty`, 等

3. 创建 DOMStringMap 类绑定 (5 个方法)
   - 新建 `core/dom/dom_string_map_bindings.cpp`
   - 绑定 `get`, `set`, `remove`, `has`

4. 在 Element 中暴露这些对象
   - `element.classList` (getter)
   - `element.style` (getter)
   - `element.dataset` (getter)

**文件修改**:
- 新建 3 个绑定文件
- 修改 `core/dom/dom_bindings.cpp` - 集成新的类绑定
- 修改 `core/CMakeLists.txt` - 添加新文件

**测试验证**:
- 预期属性和样式测试通过率: 90%+

---

### 第 4 阶段: Event 系统完善 (0.5 天)

**目标**: 完善 Event 系统绑定，预计通过率提升到 85%

**任务**:
1. 绑定 Event 构造函数
   - 支持 `new Event(type, {bubbles, cancelable})`

2. 绑定 Event 属性 (7 个)
   - `target`, `currentTarget`, `bubbles`, `cancelable`, 等

3. 绑定 `dispatchEvent(event)`

**文件修改**:
- `core/dom/dom_bindings.cpp` - 添加 Event 构造函数和属性

**测试验证**:
- 预期事件系统测试通过率: 90%+

---

### 第 5 阶段: 动画 API (0.5 天)

**目标**: 实现动画帧 API，预计通过率提升到 90%+

**任务**:
1. 在 TaskScheduler 中添加 RAF 支持
   - 修改 `core/event/task_scheduler.h/cpp`
   - 添加 `RequestAnimationFrame()` 和 `CancelAnimationFrame()`

2. 绑定到 JavaScript
   - 在 `dom_bindings.cpp` 中添加全局函数

**文件修改**:
- `core/event/task_scheduler.h/cpp`
- `core/dom/dom_bindings.cpp`

**测试验证**:
- 预期定时器测试通过率: 100%

---

### 第 6 阶段: 其他 API (可选，0.5 天)

**目标**: 完善剩余 API

**任务**:
1. 绑定 Document 属性
   - `head`, `documentElement`, `title`

2. 绑定其他 Node/Element 属性
   - `nodeName`, `attributes`

**测试验证**:
- 预期总体通过率: 95%+

---

## 📊 预期进度

| 阶段 | 耗时 | 累计耗时 | 预期通过率 | 新增绑定 |
|------|------|----------|------------|----------|
| **当前** | - | - | 23% | 14 个 |
| **阶段 1** | 1 天 | 1 天 | 50% | +15 个 |
| **阶段 2** | 0.5 天 | 1.5 天 | 65% | +8 个 |
| **阶段 3** | 1.5 天 | 3 天 | 80% | +20 个 |
| **阶段 4** | 0.5 天 | 3.5 天 | 85% | +10 个 |
| **阶段 5** | 0.5 天 | 4 天 | 90%+ | +2 个 |
| **阶段 6** | 0.5 天 | 4.5 天 | 95%+ | +11 个 |

**总计**: 4.5 天，新增 66 个绑定，从 23% 提升到 95%+

---

## 🎯 成功标准

### 测试通过率目标
- ✅ DOM 操作测试: 90%+ (当前 0%)
- ✅ 属性和样式测试: 90%+ (当前 18%)
- ✅ 事件系统测试: 90%+ (当前 0%)
- ✅ 查询选择器测试: 90%+ (当前 5%)
- ✅ 定时器测试: 100% (当前 67%)
- ✅ 表单元素测试: 95%+ (当前 89%)
- ✅ HTML 内容测试: 90%+ (当前 5%)

### 总体目标
- ✅ 总体测试通过率: 90%+
- ✅ 所有核心 Preact API 可用
- ✅ 可以运行真实的 Preact 应用

---

## 📝 技术要点

### QuickJS 绑定模式

#### 1. 属性绑定 (getter/setter)
```cpp
// Getter
static JSValue js_element_get_property(JSContext* ctx, JSValueConst this_val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;
    return JS_NewString(ctx, element->GetProperty().c_str());
}

// Setter
static JSValue js_element_set_property(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;
    const char* value = JS_ToCString(ctx, val);
    if (!value) return JS_EXCEPTION;
    element->SetProperty(value);
    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
}

// 注册
JS_CGETSET_MAGIC_DEF("property", js_element_get_property, js_element_set_property, 0)
```

#### 2. 方法绑定
```cpp
static JSValue js_element_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto element = DOMBindings::UnwrapElement(ctx, this_val);
    if (!element) return JS_EXCEPTION;
    
    // 参数检查
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "method requires 1 argument");
    }
    
    // 参数转换
    const char* arg = JS_ToCString(ctx, argv[0]);
    if (!arg) return JS_EXCEPTION;
    
    // 调用 C++ 方法
    auto result = element->Method(arg);
    JS_FreeCString(ctx, arg);
    
    // 返回结果
    return JS_NewString(ctx, result.c_str());
}

// 注册
JS_CFUNC_DEF("method", 1, js_element_method)
```

#### 3. 对象包装
```cpp
JSValue DOMBindings::WrapObject(JSContext* ctx, std::shared_ptr<Object> obj) {
    if (!obj) return JS_NULL;
    
    JSValue js_obj = JS_NewObjectClass(ctx, object_class_id);
    if (JS_IsException(js_obj)) return js_obj;
    
    auto ptr = new std::shared_ptr<Object>(obj);
    JS_SetOpaque(js_obj, ptr);
    
    return js_obj;
}
```

---

## 🚀 下一步行动

1. **立即开始阶段 1** - 核心 Node API 绑定
2. **每完成一个阶段** - 运行测试验证
3. **记录问题** - 创建问题跟踪文档
4. **持续集成** - 每个阶段完成后提交 git

---

**文档版本**: 1.0  
**最后更新**: 2025-11-15

