# MBink 前端框架集成指南

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **状态**: 正式发布

---

## 📋 目录

1. [核心原则](#核心原则)
2. [架构设计](#架构设计)
3. [正确示例](#正确示例)
4. [错误示例](#错误示例)
5. [支持的框架](#支持的框架)
6. [开发指南](#开发指南)

---

## 🎯 核心原则

### 原则1: C++层只提供浏览器级别的API

**MBink的C++层应该像浏览器一样，提供标准的Web API，而不是框架特定的功能。**

```
✅ 正确：MBink = 浏览器引擎 (提供DOM API)
❌ 错误：MBink = React引擎 (实现React功能)
```

### 原则2: 框架无关性

**任何前端框架都应该能在MBink上运行，无需修改C++代码。**

支持的框架应该包括但不限于：
- ✅ React / Preact
- ✅ Vue.js
- ✅ Angular
- ✅ Svelte
- ✅ Solid.js
- ✅ 原生JavaScript

### 原则3: 标准化优先

**遵循W3C标准，而不是特定框架的API设计。**

```cpp
// ✅ 遵循W3C DOM标准
element->AppendChild(child);
element->SetAttribute("id", "test");
element->AddEventListener("click", handler);

// ❌ 不要创建框架特定API
element->RenderReactComponent(component);
element->SetReactState(state);
```

---

## 🏗️ 架构设计

### 正确的分层架构

```
┌─────────────────────────────────────────────────┐
│  Layer 5: User Application                      │
│  - app.js (用户的React/Vue应用代码)              │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│  Layer 4: Frontend Framework (JavaScript)       │
│  - preact.js / vue.js / react.js                │
│  - Virtual DOM, Components, Hooks               │
│  - 框架特定的状态管理和生命周期                    │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│  Layer 3: Browser DOM API (JavaScript Bindings) │
│  - document.createElement()                     │
│  - element.appendChild()                        │
│  - element.addEventListener()                   │
│  - 标准Web API (C++实现，JS暴露)                 │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│  Layer 2: MBink Core (C++)                      │
│  - core/dom: DOM树管理                          │
│  - core/event: 事件系统                         │
│  - core/render: Skia渲染                        │
│  - core/layout: Yoga布局                        │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│  Layer 1: Third Party Libraries                 │
│  - QuickJS, Skia, SDL3, Yoga, Lexbor           │
└─────────────────────────────────────────────────┘
```

### 关键点

1. **Layer 4 (框架层)** - 完全在JavaScript中实现
2. **Layer 3 (API层)** - 提供标准浏览器API
3. **Layer 2 (核心层)** - 框架无关的实现

---

## ✅ 正确示例

### 示例1: Preact集成

```javascript
// ✅ 在JavaScript层使用原生Preact
import { h, render, Component } from './preact.js';

class Counter extends Component {
    constructor() {
        super();
        this.state = { count: 0 };
    }
    
    increment = () => {
        this.setState({ count: this.state.count + 1 });
    }
    
    render() {
        return h('div', null,
            h('h1', null, 'Counter'),
            h('p', null, `Count: ${this.state.count}`),
            h('button', { onClick: this.increment }, 'Increment')
        );
    }
}

// Preact通过标准DOM API操作DOM
render(h(Counter), document.body);
```

```cpp
// ✅ C++层只提供标准DOM API
// core/dom/dom_bindings.cpp

JSValue js_create_element(JSContext* ctx, JSValueConst this_val,
                          int argc, JSValueConst* argv) {
    const char* tag_name = JS_ToCString(ctx, argv[0]);
    auto element = document->CreateElement(tag_name);
    JS_FreeCString(ctx, tag_name);
    return WrapElement(ctx, element);
}

JSValue js_append_child(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv) {
    auto parent = UnwrapElement(ctx, this_val);
    auto child = UnwrapNode(ctx, argv[0]);
    parent->AppendChild(child);
    return JS_UNDEFINED;
}

JSValue js_add_event_listener(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto element = UnwrapElement(ctx, this_val);
    const char* event_type = JS_ToCString(ctx, argv[0]);
    JSValue callback = argv[1];
    
    // 存储回调函数
    auto listener = [ctx, callback](std::shared_ptr<Event> event) {
        JSValue event_obj = WrapEvent(ctx, event);
        JS_Call(ctx, callback, JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx, event_obj);
    };
    
    element->AddEventListener(event_type, listener);
    JS_FreeCString(ctx, event_type);
    return JS_UNDEFINED;
}
```

### 示例2: Vue.js集成（理论）

```javascript
// ✅ Vue.js也应该能在MBink上运行
import { createApp } from './vue.js';

const app = createApp({
    data() {
        return { count: 0 };
    },
    methods: {
        increment() {
            this.count++;
        }
    },
    template: `
        <div>
            <h1>Counter</h1>
            <p>Count: {{ count }}</p>
            <button @click="increment">Increment</button>
        </div>
    `
});

// Vue通过标准DOM API操作DOM
app.mount('#app');
```

**关键点**: 无需修改C++代码，Vue.js就能工作！

---

## ❌ 错误示例

### 错误1: 在C++层实现React组件系统

```cpp
// ❌ 错误：不要在C++层实现组件系统
class ComponentManager {
public:
    void RegisterComponent(const std::string& name, JSValue component);
    JSValue RenderComponent(const std::string& name, JSValue props);
    void UpdateComponent(const std::string& name, JSValue new_props);
};

// ❌ 错误：不要在C++层实现Virtual DOM
class VirtualDOMRenderer {
public:
    void Render(JSValue vnode);
    void Diff(JSValue old_vnode, JSValue new_vnode);
    void Patch(Element* element, JSValue patches);
};
```

**问题**:
1. 这些功能只对React有用，Vue/Angular用户无法使用
2. 增加了C++代码的复杂度
3. 难以维护和更新
4. 违背了框架无关性原则

### 错误2: 在C++层实现Hooks

```cpp
// ❌ 错误：不要在C++层实现Hooks
class HooksManager {
public:
    JSValue UseState(JSValue initial_value);
    void UseEffect(JSValue callback, JSValue deps);
    JSValue UseRef(JSValue initial_value);
    JSValue UseMemo(JSValue factory, JSValue deps);
};
```

**问题**:
1. Hooks是React特定的概念
2. Vue使用Composition API，不是Hooks
3. 应该让框架自己实现状态管理

### 错误3: 在C++层实现生命周期

```cpp
// ❌ 错误：不要在C++层实现生命周期
class LifecycleManager {
public:
    void ComponentDidMount(JSValue component);
    void ComponentWillUnmount(JSValue component);
    void ComponentDidUpdate(JSValue component, JSValue prev_props);
};
```

**问题**:
1. 不同框架有不同的生命周期概念
2. 应该让框架自己管理生命周期

---

## 🎯 支持的框架

### 当前已验证

| 框架 | 状态 | 版本 | 示例 |
|------|------|------|------|
| **Preact** | ✅ 完全支持 | 10.19.3 | examples/preact_counter |
| **原生JS** | ✅ 完全支持 | - | examples/cpp/dom_example.js |

### 理论上支持（待验证）

| 框架 | 预期状态 | 说明 |
|------|---------|------|
| **React** | ✅ 应该支持 | 需要Preact compat层 |
| **Vue.js** | ✅ 应该支持 | 需要完整的DOM API |
| **Svelte** | ✅ 应该支持 | 编译为原生JS |
| **Solid.js** | ✅ 应该支持 | 使用标准DOM API |
| **Angular** | ⚠️ 可能需要额外工作 | 依赖较多浏览器API |

---

## 📚 开发指南

### 添加新功能前的检查清单

在添加任何新功能到C++层之前，问自己以下问题：

1. ❓ **这是标准浏览器API吗？**
   - 如果是 → ✅ 可以添加
   - 如果否 → 继续下一个问题

2. ❓ **这个功能只对特定框架有用吗？**
   - 如果是 → ❌ 不应该添加到C++层
   - 如果否 → 继续下一个问题

3. ❓ **如果用户想用Vue而不是React，这个功能还有用吗？**
   - 如果是 → ✅ 可以添加
   - 如果否 → ❌ 不应该添加到C++层

4. ❓ **这个功能可以在JavaScript层实现吗？**
   - 如果是 → ❌ 应该在JavaScript层实现
   - 如果否 → ✅ 可以添加到C++层

### 决策树

```
新功能
  ↓
是标准浏览器API？
  ├─ 是 → ✅ 添加到C++层
  └─ 否 → 只对特定框架有用？
           ├─ 是 → ❌ 不添加到C++层
           └─ 否 → 可以在JS层实现？
                    ├─ 是 → ❌ 在JS层实现
                    └─ 否 → ✅ 添加到C++层
```

---

## 📖 参考资料

- [W3C DOM Standard](https://dom.spec.whatwg.org/)
- [MDN Web APIs](https://developer.mozilla.org/en-US/docs/Web/API)
- [Preact Documentation](https://preactjs.com/)
- [Vue.js Documentation](https://vuejs.org/)
- [React Documentation](https://react.dev/)

---

**维护者**: MBink Team  
**最后更新**: 2025-11-14

