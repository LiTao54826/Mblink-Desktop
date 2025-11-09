# Phase 2.2 DOM API - 开发任务清单

**开始日期**: 2025-11-09  
**预计完成**: 2周 (Week 5-6)  
**当前进度**: 0/67 任务完成 (0%)  
**状态**: 🚧 准备开始

---

## 📋 任务概览

Phase 2.2 的目标是实现一个**最小可用的 DOM API**，支持基础的 DOM 树操作和事件系统，为后续的 Preact 集成打下基础。

### 核心目标
- ✅ 实现 DOM 节点类层次结构（Node, Element, Text, Document）
- ✅ 实现 15 个核心 DOM API
- ✅ 实现基础事件系统（addEventListener/removeEventListener）
- ✅ 绑定到 QuickJS，使 JavaScript 可以调用
- ✅ 编写完整的测试用例（覆盖率 > 80%）

### 交付物
- 完整的 DOM 节点类实现
- 15+ 核心 DOM API
- 基础事件系统
- QuickJS 绑定
- 单元测试（覆盖率 > 80%）

---

## 📊 任务分解

### Task 1: DOM 节点基类完善 (8 subtasks)

**目标**: 完善 Node 基类，实现所有基础节点操作

- [ ] 1.1 完善 Node 构造函数和析构函数
- [ ] 1.2 实现 AppendChild() - 添加子节点
- [ ] 1.3 实现 InsertBefore() - 在指定节点前插入
- [ ] 1.4 实现 RemoveChild() - 移除子节点
- [ ] 1.5 实现 ReplaceChild() - 替换子节点
- [ ] 1.6 实现 CloneNode() - 克隆节点（深度/浅度）
- [ ] 1.7 实现 GetTextContent() / SetTextContent()
- [ ] 1.8 添加 Node 类单元测试

**实现位置**: `core/dom/node.h`, `core/dom/node.cpp`

**测试位置**: `tests/test_dom_node.cpp`

**关键点**:
- 使用 `std::shared_ptr` 和 `std::weak_ptr` 管理节点生命周期
- 正确维护父子关系和兄弟关系
- 实现脏标记机制（MarkDirty）
- 防止循环引用

---

### Task 2: Element 类实现 (10 subtasks)

**目标**: 实现 Element 类，支持属性、样式、类名等操作

- [ ] 2.1 实现 Element 构造函数（tagName）
- [ ] 2.2 实现 SetAttribute() / GetAttribute()
- [ ] 2.3 实现 RemoveAttribute() / HasAttribute()
- [ ] 2.4 实现 className 属性（get/set）
- [ ] 2.5 实现 id 属性（get/set）
- [ ] 2.6 实现 style 属性（CSSStyleDeclaration）
- [ ] 2.7 实现 GetElementsByTagName()
- [ ] 2.8 实现 GetElementsByClassName()
- [ ] 2.9 实现 GetElementById()
- [ ] 2.10 添加 Element 类单元测试

**实现位置**: `core/dom/element.h`, `core/dom/element.cpp`

**测试位置**: `tests/test_dom_element.cpp`

**关键点**:
- 使用 `std::unordered_map` 存储属性
- 实现简单的 CSS 样式解析（cssText）
- 支持 className 的空格分隔
- 实现 DOM 树遍历查询

---

### Task 3: Text 节点实现 (4 subtasks)

**目标**: 实现 Text 节点，支持文本内容操作

- [ ] 3.1 实现 Text 构造函数（data）
- [ ] 3.2 实现 data 属性（get/set）
- [ ] 3.3 实现 CloneNode() 重写
- [ ] 3.4 添加 Text 类单元测试

**实现位置**: `core/dom/text.h`, `core/dom/text.cpp`

**测试位置**: `tests/test_dom_text.cpp`

**关键点**:
- Text 节点不能有子节点
- data 属性变化时标记为脏
- 正确实现 GetTextContent()

---

### Task 4: Document 类实现 (8 subtasks)

**目标**: 实现 Document 类，作为 DOM 树的根节点

- [ ] 4.1 实现 Document 构造函数（单例模式）
- [ ] 4.2 实现 CreateElement(tagName)
- [ ] 4.3 实现 CreateTextNode(text)
- [ ] 4.4 实现 body 属性（get/set）
- [ ] 4.5 实现 documentElement 属性（get）
- [ ] 4.6 实现 GetElementById(id)
- [ ] 4.7 实现 GetElementsByTagName(tagName)
- [ ] 4.8 添加 Document 类单元测试

**实现位置**: `core/dom/document.h`, `core/dom/document.cpp`

**测试位置**: `tests/test_dom_document.cpp`

**关键点**:
- Document 是 DOM 树的根节点
- 管理全局的 id -> Element 映射
- 提供工厂方法创建节点
- 维护 body 和 documentElement 引用

---

### Task 5: 事件系统基础 (9 subtasks)

**目标**: 实现基础的事件系统，支持事件监听和触发

- [ ] 5.1 设计 Event 类（type, target, currentTarget）
- [ ] 5.2 实现 EventListener 接口
- [ ] 5.3 实现 AddEventListener(type, handler)
- [ ] 5.4 实现 RemoveEventListener(type, handler)
- [ ] 5.5 实现 DispatchEvent(event)
- [ ] 5.6 实现事件冒泡机制
- [ ] 5.7 实现事件捕获机制
- [ ] 5.8 实现 StopPropagation() / PreventDefault()
- [ ] 5.9 添加事件系统单元测试

**实现位置**: `core/dom/event.h`, `core/dom/event.cpp`, `core/dom/element.cpp`

**测试位置**: `tests/test_dom_event.cpp`

**关键点**:
- 使用 `std::function` 存储事件处理器
- 实现事件冒泡路径计算
- 支持捕获阶段和冒泡阶段
- 正确管理事件监听器的生命周期

---

### Task 6: QuickJS 绑定 (12 subtasks)

**目标**: 将 DOM API 绑定到 QuickJS，使 JavaScript 可以调用

- [ ] 6.1 设计 DOM 对象的 JavaScript 包装器
- [ ] 6.2 实现 Node 类的 QuickJS 绑定
- [ ] 6.3 实现 Element 类的 QuickJS 绑定
- [ ] 6.4 实现 Text 类的 QuickJS 绑定
- [ ] 6.5 实现 Document 类的 QuickJS 绑定
- [ ] 6.6 实现 Event 类的 QuickJS 绑定
- [ ] 6.7 绑定 document 全局对象
- [ ] 6.8 实现属性访问器（getter/setter）
- [ ] 6.9 实现方法调用
- [ ] 6.10 实现事件处理器绑定
- [ ] 6.11 实现内存管理（GC 集成）
- [ ] 6.12 添加 JavaScript 绑定测试

**实现位置**: `core/dom/dom_bindings.h`, `core/dom/dom_bindings.cpp`

**测试位置**: `tests/test_dom_bindings.cpp`

**关键点**:
- 使用 QuickJS 的 Class API 创建 JavaScript 类
- 正确管理 C++ 对象和 JSValue 的生命周期
- 实现属性的 getter/setter
- 支持事件处理器的 JavaScript 函数
- 防止内存泄漏和悬空指针

---

### Task 7: DOM API 集成测试 (8 subtasks)

**目标**: 编写集成测试，验证 DOM API 的完整功能

- [ ] 7.1 测试基础 DOM 树操作（创建、添加、删除）
- [ ] 7.2 测试属性操作（setAttribute, getAttribute）
- [ ] 7.3 测试样式操作（className, style）
- [ ] 7.4 测试 DOM 查询（getElementById, getElementsByTagName）
- [ ] 7.5 测试事件监听和触发
- [ ] 7.6 测试事件冒泡和捕获
- [ ] 7.7 测试 JavaScript 调用 DOM API
- [ ] 7.8 测试复杂场景（嵌套节点、多事件）

**测试位置**: `tests/test_dom_integration.cpp`

**关键点**:
- 模拟真实的 DOM 操作场景
- 验证内存管理正确性
- 测试边界条件和错误处理
- 确保 JavaScript 和 C++ 互操作正常

---

### Task 8: 性能优化 (4 subtasks)

**目标**: 优化 DOM 操作性能，减少不必要的开销

- [ ] 8.1 实现脏标记优化（只重新布局/渲染变化的节点）
- [ ] 8.2 优化 DOM 查询（缓存 id 映射）
- [ ] 8.3 优化事件监听器存储（使用 hash map）
- [ ] 8.4 添加性能基准测试

**实现位置**: `core/dom/*.cpp`

**测试位置**: `tests/benchmark_dom.cpp`

**关键点**:
- 避免不必要的 DOM 树遍历
- 使用缓存加速查询
- 减少内存分配
- 实现增量更新

---

### Task 9: 文档和示例 (4 subtasks)

**目标**: 编写文档和示例代码

- [ ] 9.1 编写 DOM API 使用文档
- [ ] 9.2 编写 JavaScript 示例代码
- [ ] 9.3 编写 C++ 示例代码
- [ ] 9.4 更新 README 和 ROADMAP

**文档位置**: `docs/DOM_API.md`, `examples/dom_example.js`, `examples/dom_example.cpp`

**关键点**:
- 提供清晰的 API 文档
- 包含常见用例的示例
- 说明最佳实践
- 更新项目进度

---

## 📈 进度跟踪

### 总体进度
- **总任务数**: 67 个子任务
- **已完成**: 0 个子任务 (0%)
- **进行中**: 0 个子任务
- **待开始**: 67 个子任务

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

## 🎯 核心 API 列表

### P0 - 必须实现（9 个）
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
```

### P1 - 重要（6 个）
```javascript
// Element API
element.className
element.id
element.style.cssText
element.addEventListener(type, handler)
element.removeEventListener(type, handler)

// Text API
textNode.data
```

### P2 - 可选（扩展）
```javascript
element.removeAttribute(name)
element.hasAttribute(name)
element.getElementsByTagName(tagName)
element.getElementsByClassName(className)
element.textContent
node.cloneNode(deep)
```

---

## 🔧 技术要点

### 1. 内存管理
- 使用 `std::shared_ptr` 管理节点生命周期
- 使用 `std::weak_ptr` 避免循环引用（parent_node_）
- QuickJS 绑定时正确管理 JSValue 引用计数
- 实现 RAII 模式确保资源释放

### 2. DOM 树结构
```
Document (root)
  └─ documentElement (html)
      └─ body
          ├─ Element (div)
          │   ├─ Text ("Hello")
          │   └─ Element (span)
          └─ Element (p)
```

### 3. 事件流
```
Capture Phase (捕获阶段)
  Document → Element → Target

Target Phase (目标阶段)
  Target

Bubble Phase (冒泡阶段)
  Target → Element → Document
```

### 4. QuickJS 绑定模式
```cpp
// 创建 JavaScript 类
JSClassID element_class_id;
JS_NewClassID(&element_class_id);
JS_NewClass(rt, element_class_id, &element_class_def);

// 创建原型对象
JSValue proto = JS_NewObject(ctx);
JS_SetPropertyStr(ctx, proto, "appendChild", JS_NewCFunction(...));

// 创建实例
JSValue obj = JS_NewObjectClass(ctx, element_class_id);
JS_SetOpaque(obj, element_ptr);
```

---

## 📝 开发建议

### 开发顺序
1. **先实现 C++ 类**（Node → Text → Element → Document）
2. **再实现事件系统**（Event → EventListener → DispatchEvent）
3. **然后实现 QuickJS 绑定**（逐个类绑定）
4. **最后编写测试**（单元测试 → 集成测试）

### 测试策略
- 每个类都有对应的单元测试
- 使用 Google Test 框架
- 测试覆盖率 > 80%
- 包含边界条件和错误处理测试

### 参考资料
- [DOM Standard](https://dom.spec.whatwg.org/)
- [QuickJS Documentation](https://bellard.org/quickjs/)
- [MDN Web Docs - DOM](https://developer.mozilla.org/en-US/docs/Web/API/Document_Object_Model)

---

## ✅ 完成标准

Phase 2.2 完成的标准：
- ✅ 所有 67 个子任务完成
- ✅ 15+ 核心 DOM API 实现并通过测试
- ✅ 事件系统实现并通过测试
- ✅ QuickJS 绑定完成，JavaScript 可以调用所有 API
- ✅ 单元测试覆盖率 > 80%
- ✅ 集成测试通过
- ✅ 文档完整

---

**下一阶段**: Phase 2.3 - 布局引擎（Yoga 集成）

