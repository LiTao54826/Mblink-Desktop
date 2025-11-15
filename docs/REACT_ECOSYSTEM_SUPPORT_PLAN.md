# 🚀 MBink React 生态支持计划

**版本**: v1.0.0  
**开始日期**: 2025-11-15  
**预计完成**: 3 周  
**状态**: 📋 计划中

---

## 🎯 核心原则

### ⚠️ 重要：遵循 MBink 开发规范

根据 **PROJECT_STANDARDS.md 规范5**：

✅ **正确做法**：
- C++ 层只提供**浏览器级别的 DOM API**
- 让**真正的 Preact/React（JavaScript 版本）**直接运行
- 补充 React 运行所需的缺失浏览器 API

❌ **错误做法**：
- 在 C++ 层实现虚拟 DOM
- 在 C++ 层实现 Hooks
- 在 C++ 层实现组件系统

### 架构图

```
┌─────────────────────────────────────┐
│  React/Preact (JavaScript)          │  ← 真正的 React 框架
│  - 虚拟 DOM                          │
│  - Hooks                             │
│  - 组件系统                          │
└─────────────────────────────────────┘
                ↓ 使用
┌─────────────────────────────────────┐
│  浏览器 API (C++ 实现)               │  ← MBink 提供
│  - DOM API (querySelector, etc.)    │
│  - Event API (addEventListener)     │
│  - Timer API (setTimeout, etc.)     │
│  - 其他标准浏览器 API                │
└─────────────────────────────────────┘
```

---

## 📊 项目概览

### 目标

补充 MBink 缺失的浏览器 API，使 **真正的 Preact/React** 可以直接运行。

### 核心价值

1. **标准兼容**: 提供标准浏览器 API，不是自定义 API
2. **生态兼容**: 支持现有的 React 组件库和工具链
3. **性能优势**: 结合 MBink 的高性能渲染引擎
4. **轻量级**: 使用 Preact 减小包体积

---

## 📅 开发计划

### 第一步：API 缺口分析 (1 天)

**目标**: 分析 Preact 运行所需的浏览器 API，找出 MBink 缺失的 API

**任务**:
- [ ] 研究 Preact 源码，列出所需的浏览器 API
- [ ] 检查 MBink 现有的 DOM API 实现
- [ ] 创建 API 缺口清单文档
- [ ] 按优先级排序

**预期成果**:
- ✅ 完整的 API 缺口清单
- ✅ 优先级排序的实现计划

---

### Week 1: 核心 DOM API 补充 (5 天)

#### Day 1-2: 元素操作 API

**目标**: 补充 Preact 需要的元素操作 API

**任务**:
- [ ] 检查并补充以下 API:
  - [ ] `innerHTML` / `outerHTML` (getter/setter)
  - [ ] `textContent` (getter/setter)
  - [ ] `insertBefore(newNode, referenceNode)`
  - [ ] `replaceChild(newChild, oldChild)`
  - [ ] `cloneNode(deep)`
  - [ ] `contains(node)`
- [ ] 创建测试文件 `tests/unit/test_dom_element_api.cpp`
  - [ ] 测试 innerHTML 读写
  - [ ] 测试 textContent 读写
  - [ ] 测试节点插入/替换
  - [ ] 测试节点克隆
- [ ] 更新 `core/dom/element.h/cpp`

**预期成果**:
- ✅ 6+ 个 DOM API 实现
- ✅ 20+ 测试通过

---

#### Day 3: 属性和样式 API

**目标**: 补充属性和样式相关 API

**任务**:
- [ ] 检查并补充以下 API:
  - [ ] `classList.add/remove/toggle/contains`
  - [ ] `style.setProperty/getPropertyValue/removeProperty`
  - [ ] `dataset` (data-* 属性)
  - [ ] `removeAttribute(name)`
  - [ ] `hasAttributes()`
  - [ ] `attributes` (NamedNodeMap)
- [ ] 创建测试文件 `tests/unit/test_dom_attributes.cpp`
- [ ] 更新 `core/dom/element.h/cpp`

**预期成果**:
- ✅ 6+ 个 API 实现
- ✅ 15+ 测试通过

---

#### Day 4: 事件 API

**目标**: 补充事件相关 API

**任务**:
- [ ] 检查并补充以下 API:
  - [ ] `addEventListener(type, listener, options)`
  - [ ] `removeEventListener(type, listener, options)`
  - [ ] `dispatchEvent(event)`
  - [ ] Event 对象属性:
    - [ ] `target`, `currentTarget`
    - [ ] `preventDefault()`, `stopPropagation()`
    - [ ] `bubbles`, `cancelable`
- [ ] 创建测试文件 `tests/unit/test_event_api.cpp`
- [ ] 更新 `core/event/event.h/cpp`

**预期成果**:
- ✅ 事件 API 完善
- ✅ 15+ 测试通过

---

#### Day 5: 查询和遍历 API

**目标**: 补充 DOM 查询和遍历 API

**任务**:
- [ ] 检查并补充以下 API:
  - [ ] `querySelector(selector)`
  - [ ] `querySelectorAll(selector)`
  - [ ] `getElementById(id)`
  - [ ] `getElementsByClassName(className)`
  - [ ] `getElementsByTagName(tagName)`
  - [ ] `closest(selector)`
  - [ ] `matches(selector)`
- [ ] 创建测试文件 `tests/unit/test_dom_query.cpp`
- [ ] 更新 `core/dom/element.h/cpp`

**预期成果**:
- ✅ 查询 API 完善
- ✅ 20+ 测试通过

---

### Week 2: 定时器和异步 API (5 天)

#### Day 1-2: 定时器 API

**目标**: 实现 setTimeout/setInterval

**任务**:
- [ ] 创建 `core/api/timer.h/cpp`
- [ ] 实现以下 API:
  - [ ] `setTimeout(callback, delay, ...args)`
  - [ ] `clearTimeout(timeoutId)`
  - [ ] `setInterval(callback, delay, ...args)`
  - [ ] `clearInterval(intervalId)`
- [ ] 与事件循环集成
- [ ] 创建测试文件 `tests/unit/test_timer_api.cpp`
- [ ] JavaScript 绑定

**预期成果**:
- ✅ 定时器 API 可用
- ✅ 15+ 测试通过

---

#### Day 3: Promise 和 MutationObserver

**目标**: 实现 Promise 和 MutationObserver

**任务**:
- [ ] 检查 QuickJS 的 Promise 支持
- [ ] 实现 `MutationObserver`
  - [ ] `observe(target, options)`
  - [ ] `disconnect()`
  - [ ] `takeRecords()`
- [ ] 创建测试文件 `tests/unit/test_async_api.cpp`

**预期成果**:
- ✅ Promise 可用
- ✅ MutationObserver 可用
- ✅ 10+ 测试通过

---

#### Day 4-5: Preact Hello World

**目标**: 运行第一个 Preact 应用

**任务**:
- [ ] 集成 Preact (JavaScript 库)
- [ ] 创建 Hello World 示例
  ```javascript
  import { h, render } from 'preact';
  
  function App() {
    return h('div', null, 'Hello, MBink!');
  }
  
  render(h(App), document.body);
  ```
- [ ] 测试基础渲染
- [ ] 测试组件更新
- [ ] 创建文档 `docs/PREACT_INTEGRATION.md`

**预期成果**:
- ✅ Preact Hello World 运行成功
- ✅ 基础文档

---

### Week 3: Hooks 和组件库测试 (5 天)

#### Day 1-2: Hooks 测试

**目标**: 测试 Preact Hooks 是否正常工作

**任务**:
- [ ] 测试 `useState`
  ```javascript
  function Counter() {
    const [count, setCount] = useState(0);
    return h('button', { onClick: () => setCount(count + 1) }, count);
  }
  ```
- [ ] 测试 `useEffect`
- [ ] 测试 `useRef`
- [ ] 测试 `useContext`
- [ ] 测试 `useReducer`
- [ ] 创建示例应用 `examples/preact_hooks_demo/`

**预期成果**:
- ✅ 所有 Hooks 正常工作
- ✅ Hooks 示例应用

---

#### Day 3-4: 组件库测试

**目标**: 测试主流 React 组件库的兼容性

**任务**:
- [ ] 测试 Ant Design (Preact 版本)
  - [ ] Button 组件
  - [ ] Input 组件
  - [ ] Form 组件
- [ ] 测试 Preact Material Components
- [ ] 记录兼容性问题
- [ ] 修复发现的 API 缺失
- [ ] 创建兼容性报告

**预期成果**:
- ✅ 主流组件库基本可用
- ✅ 兼容性报告

---

#### Day 5: 示例应用和文档

**目标**: 创建完整的示例应用

**任务**:
- [ ] 创建 Todo List 应用
  - [ ] 使用 Preact
  - [ ] 使用 Hooks
  - [ ] 使用组件库
- [ ] 完善文档
  - [ ] 快速开始指南
  - [ ] API 参考
  - [ ] 最佳实践
- [ ] 发布 v1.0.0

**预期成果**:
- ✅ 完整的 Todo List 应用
- ✅ 完整文档
- ✅ v1.0.0 发布

---

## 📊 成功指标

### 功能指标

| 指标 | 目标 | 说明 |
|------|------|------|
| **浏览器 API 覆盖** | 25+ 个 | Preact 运行所需的 API |
| **Hooks 支持** | 100% | 所有 Preact Hooks 可用 |
| **组件库兼容** | 2+ 个 | 主流组件库可用 |
| **测试覆盖** | > 80% | 核心功能测试覆盖 |

### 性能指标

| 指标 | 目标 | 说明 |
|------|------|------|
| **初始渲染** | < 50ms | 1000 个组件 |
| **更新渲染** | < 16ms | 60 FPS |
| **内存占用** | < 10MB | 1000 个组件 |

### 文档指标

| 指标 | 目标 | 说明 |
|------|------|------|
| **API 文档** | 100% | 所有 API 有文档 |
| **示例代码** | 5+ 个 | 覆盖常见场景 |
| **快速开始** | 完整 | 5 分钟上手 |

---

**最后更新**: 2025-11-15  
**维护者**: MBink Team

