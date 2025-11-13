# MBink 项目规范更新 - 2025-11-14

## 📊 更新摘要

**更新日期**: 2025-11-14  
**文档版本**: 2.0 → 2.1  
**更新类型**: 新增强制规范

---

## 🎯 核心更新

### 新增规范5: 前端框架集成规范

**核心原则**: **C++层只提供浏览器级别的API，不实现框架特定功能**

---

## 📋 更新详情

### 1. 设计理念

#### ✅ 正确的定位
```
MBink = 浏览器引擎 (提供标准DOM API)
```

#### ❌ 错误的定位
```
MBink = React引擎 (实现React特定功能)
```

### 2. 四大原则

1. **通用性优先** - C++层提供的API应该能支持任何前端框架
2. **标准化** - 遵循W3C DOM标准，而不是特定框架的API
3. **框架无关** - 框架特定的逻辑应该在JavaScript层实现
4. **可替换性** - 用户应该能够轻松切换前端框架

### 3. 架构分层

```
┌─────────────────────────────────────────┐
│  Application Code (app.js)              │  用户应用
│  - React/Vue/Angular/Svelte             │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Frontend Framework (preact.js)         │  前端框架
│  - Virtual DOM                          │  (JavaScript层)
│  - Component System                     │
│  - Hooks / Lifecycle                    │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Browser DOM API (dom_bindings.cpp)    │  标准DOM API
│  - document.createElement()             │  (C++层)
│  - element.appendChild()                │
│  - element.addEventListener()           │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  MBink Core (core/dom, core/render)    │  核心实现
└─────────────────────────────────────────┘
```

---

## ✅ 允许的C++层功能

以下是**允许**在C++层实现的功能（标准浏览器API）：

- ✅ 标准DOM API（createElement, appendChild, setAttribute等）
- ✅ 标准事件API（addEventListener, removeEventListener等）
- ✅ 标准选择器API（querySelector, querySelectorAll等）
- ✅ 标准样式API（style, classList, className等）
- ✅ 标准定时器API（setTimeout, setInterval等）
- ✅ 标准Console API（console.log, console.error等）

---

## ❌ 禁止的C++层功能

以下是**禁止**在C++层实现的功能（框架特定）：

- ❌ 框架特定的组件系统
- ❌ 框架特定的状态管理（useState, Redux等）
- ❌ 框架特定的生命周期钩子
- ❌ 框架特定的Virtual DOM实现
- ❌ 框架特定的Reconciliation算法
- ❌ 框架特定的JSX/模板编译

---

## 📝 代码示例

### ✅ 正确示例

```cpp
// ✅ 提供标准的浏览器DOM API
class Document {
public:
    std::shared_ptr<Element> CreateElement(const std::string& tag_name);
    std::shared_ptr<Element> GetElementById(const std::string& id);
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
};

class Element {
public:
    void SetAttribute(const std::string& name, const std::string& value);
    void AppendChild(std::shared_ptr<Node> child);
    void RemoveChild(std::shared_ptr<Node> child);
    void AddEventListener(const std::string& type, EventListener listener);
};
```

```javascript
// ✅ 在JavaScript层使用原生Preact
import { h, render } from './preact.js';

function App() {
    const [count, setCount] = useState(0);  // Preact的useState
    
    return h('div', null,
        h('h1', null, 'Counter'),
        h('p', null, `Count: ${count}`),
        h('button', { onClick: () => setCount(count + 1) }, 'Increment')
    );
}

// Preact通过标准DOM API操作DOM
render(h(App), document.body);
```

### ❌ 错误示例

```cpp
// ❌ 不要在C++层实现React特定功能
class PreactRenderer {
    void RenderComponent(JSValue component);     // ❌ 不要实现组件渲染
    void UpdateVirtualDOM(JSValue vnode);        // ❌ 不要实现Virtual DOM
    void ReconcileChildren(JSValue children);    // ❌ 不要实现Reconciliation
};

// ❌ 不要在C++层实现Hooks
class HooksManager {
    JSValue UseState(JSValue initial);           // ❌ 不要实现useState
    void UseEffect(JSValue callback);            // ❌ 不要实现useEffect
};

// ❌ 不要在C++层实现组件生命周期
class ComponentManager {
    void ComponentDidMount(JSValue component);   // ❌ 不要实现生命周期
    void ComponentWillUnmount(JSValue component);// ❌ 不要实现生命周期
};
```

---

## 🔍 违规检查清单

在添加新功能前，问自己以下问题：

1. ❓ **这个功能是否是标准浏览器API？**
2. ❓ **这个功能是否只对特定框架有用？**
3. ❓ **如果用户想用Vue而不是React，这个功能还有用吗？**
4. ❓ **这个功能是否可以在JavaScript层实现？**

**判断标准**：
- 如果答案是 "否、是、否、是" → ❌ **不应该**在C++层实现
- 如果答案是 "是、否、是、否" → ✅ **可以**在C++层实现

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

## 📚 新增文档

### 1. PROJECT_STANDARDS.md 更新
- 版本：2.0 → 2.1
- 新增：规范5 - 前端框架集成规范
- 新增：违规检查清单
- 新增：版本历史记录

### 2. FRONTEND_FRAMEWORK_INTEGRATION_GUIDE.md (新建)
- 详细的前端框架集成指南
- 正确和错误示例对比
- 支持的框架列表
- 开发决策树

---

## 🎯 影响范围

### 立即生效
- ✅ 所有新功能开发必须遵循此规范
- ✅ 代码审查时必须检查是否违反此规范

### 现有代码
- ⚠️ 现有的Preact集成代码已经符合此规范（使用原生Preact）
- ⚠️ 如果存在违规代码，应该在后续重构中修正

### 未来开发
- ✅ 支持更多前端框架（Vue、Svelte等）
- ✅ 完善标准浏览器API
- ❌ 不再添加框架特定功能

---

## 💡 关键收益

### 1. 通用性
- 用户可以选择任何前端框架
- 不被锁定在特定框架上

### 2. 可维护性
- C++代码更简洁
- 框架更新不影响C++层

### 3. 生态系统
- 可以直接使用现有的React/Vue组件库
- 无需重新实现框架功能

### 4. 开发效率
- 前端开发者使用熟悉的框架
- 无需学习自定义API

---

## 📖 参考资料

- [PROJECT_STANDARDS.md](docs/PROJECT_STANDARDS.md) - 项目开发规范
- [FRONTEND_FRAMEWORK_INTEGRATION_GUIDE.md](docs/FRONTEND_FRAMEWORK_INTEGRATION_GUIDE.md) - 前端框架集成指南
- [W3C DOM Standard](https://dom.spec.whatwg.org/) - DOM标准
- [MDN Web APIs](https://developer.mozilla.org/en-US/docs/Web/API) - Web API参考

---

## ✅ 总结

**这次更新明确了MBink的定位：**

> **MBink是一个浏览器引擎，而不是React引擎**

**核心价值：**
1. ✅ 提供标准浏览器API
2. ✅ 支持任何前端框架
3. ✅ 保持C++层简洁
4. ✅ 最大化通用性和可维护性

**强制执行：**
- 所有新功能必须遵循此规范
- 代码审查必须检查违规情况
- 违规代码应该被拒绝或重构

---

**维护者**: MBink Team  
**更新日期**: 2025-11-14

