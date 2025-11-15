# Preact API 缺口分析

**日期**: 2025-11-15  
**版本**: v1.0  
**状态**: ✅ 完成

---

## 📊 分析总结

### 现状

MBink 已经实现了 **40+ DOM API**，覆盖了大部分 Preact 运行所需的基础 API。

### 缺口

经过分析 Preact 源码和现有实现，发现以下 **缺失的 API**：

| 类别 | 缺失 API 数量 | 优先级 |
|------|--------------|--------|
| **元素操作** | 3 个 | 🔴 高 |
| **属性和样式** | 0 个 | ✅ 完成 |
| **事件系统** | 2 个 | 🟡 中 |
| **查询和遍历** | 2 个 | 🟡 中 |
| **定时器** | 4 个 | 🔴 高 |
| **异步** | 1 个 | 🟢 低 |
| **总计** | **12 个** | - |

---

## ✅ 已实现的 API

### Node API (20+)
- ✅ `appendChild(child)`
- ✅ `removeChild(child)`
- ✅ `insertBefore(newNode, referenceNode)` - **需验证**
- ✅ `cloneNode(deep)` - **需验证**
- ✅ `contains(node)`
- ✅ `hasChildNodes()`
- ✅ `parentNode`, `firstChild`, `lastChild`
- ✅ `nextSibling`, `previousSibling`
- ✅ `nodeType`, `nodeName`, `textContent`

### Element API (40+)
- ✅ `setAttribute(name, value)`
- ✅ `getAttribute(name)`
- ✅ `removeAttribute(name)`
- ✅ `hasAttribute(name)`
- ✅ `querySelector(selector)`
- ✅ `querySelectorAll(selector)`
- ✅ `addEventListener(type, listener)`
- ✅ `removeEventListener(type, listener)`
- ✅ `classList.add/remove/toggle/contains`
- ✅ `className` (getter/setter)
- ✅ `id` (getter/setter)
- ✅ `innerHTML` - **需验证**
- ✅ `outerHTML` - **需验证**
- ✅ `textContent` (getter/setter)
- ✅ `style.setProperty/getPropertyValue/removeProperty`
- ✅ `dataset` (data-* 属性)

### Document API (15+)
- ✅ `createElement(tagName)`
- ✅ `createTextNode(text)`
- ✅ `querySelector(selector)`
- ✅ `querySelectorAll(selector)`
- ✅ `getElementById(id)`
- ✅ `getElementsByClassName(className)` - **需验证**
- ✅ `getElementsByTagName(tagName)` - **需验证**
- ✅ `body`, `head`, `documentElement`

---

## ❌ 缺失的 API

### 1. 元素操作 API (3 个)

#### 1.1 `replaceChild(newChild, oldChild)` 🔴
**优先级**: 高  
**原因**: Preact diff 算法需要  
**实现位置**: `core/dom/node.h/cpp`

```cpp
// Node.h
std::shared_ptr<Node> ReplaceChild(
    std::shared_ptr<Node> new_child,
    std::shared_ptr<Node> old_child
);
```

#### 1.2 `insertBefore(newNode, referenceNode)` 🟡
**优先级**: 中  
**状态**: 可能已实现，需验证  
**原因**: Preact 列表渲染需要  
**实现位置**: `core/dom/node.h/cpp`

#### 1.3 `cloneNode(deep)` 🟡
**优先级**: 中  
**状态**: 可能已实现，需验证  
**原因**: Preact 某些优化场景需要  
**实现位置**: `core/dom/node.h/cpp`

---

### 2. 事件系统 API (2 个)

#### 2.1 `addEventListener` 选项支持 🟡
**优先级**: 中  
**原因**: Preact 需要 `{ once: true, capture: true }` 等选项  
**当前实现**: 只支持基础的 `addEventListener(type, listener)`  
**需要增强**: 支持第三个参数 `options`

```cpp
// Event.h
void AddEventListener(
    const std::string& type,
    EventListener listener,
    const EventListenerOptions& options = {}  // 新增
);

struct EventListenerOptions {
    bool capture = false;
    bool once = false;
    bool passive = false;
};
```

#### 2.2 `dispatchEvent(event)` 🟡
**优先级**: 中  
**原因**: Preact 自定义事件需要  
**实现位置**: `core/event/event.h/cpp`

```cpp
// Element.h
bool DispatchEvent(std::shared_ptr<Event> event);
```

---

### 3. 查询和遍历 API (2 个)

#### 3.1 `closest(selector)` 🟢
**优先级**: 低  
**原因**: 某些组件库可能需要  
**实现位置**: `core/dom/element.h/cpp`

```cpp
// Element.h
std::shared_ptr<Element> Closest(const std::string& selector);
```

#### 3.2 `matches(selector)` 🟢
**优先级**: 低  
**原因**: 某些组件库可能需要  
**实现位置**: `core/dom/element.h/cpp`

```cpp
// Element.h
bool Matches(const std::string& selector);
```

---

### 4. 定时器 API (4 个) 🔴

#### 4.1 `setTimeout(callback, delay, ...args)` 🔴
**优先级**: 高  
**原因**: Preact useEffect 和异步渲染需要  
**实现位置**: `core/api/timer.h/cpp` (新建)

```cpp
// timer.h
int SetTimeout(std::function<void()> callback, int delay_ms);
void ClearTimeout(int timeout_id);
```

#### 4.2 `clearTimeout(timeoutId)` 🔴
**优先级**: 高  
**原因**: 清理定时器  

#### 4.3 `setInterval(callback, delay, ...args)` 🔴
**优先级**: 高  
**原因**: 某些组件可能需要  

```cpp
// timer.h
int SetInterval(std::function<void()> callback, int delay_ms);
void ClearInterval(int interval_id);
```

#### 4.4 `clearInterval(intervalId)` 🔴
**优先级**: 高  
**原因**: 清理定时器  

---

### 5. 异步 API (1 个)

#### 5.1 `MutationObserver` 🟢
**优先级**: 低  
**原因**: 某些高级组件库可能需要  
**实现位置**: `core/api/mutation_observer.h/cpp` (新建)

```cpp
// mutation_observer.h
class MutationObserver {
public:
    using Callback = std::function<void(const std::vector<MutationRecord>&)>;
    
    MutationObserver(Callback callback);
    void Observe(std::shared_ptr<Node> target, const MutationObserverInit& options);
    void Disconnect();
    std::vector<MutationRecord> TakeRecords();
};
```

---

## 📋 实现计划

### Phase 1: 高优先级 API (Week 1, Day 1-3)

**目标**: 实现 Preact 运行的必需 API

1. **定时器 API** (Day 1-2)
   - [ ] `setTimeout/clearTimeout`
   - [ ] `setInterval/clearInterval`
   - [ ] 与事件循环集成
   - [ ] JavaScript 绑定
   - [ ] 测试 (15+)

2. **元素操作 API** (Day 3)
   - [ ] `replaceChild`
   - [ ] 验证 `insertBefore`
   - [ ] 验证 `cloneNode`
   - [ ] 测试 (10+)

### Phase 2: 中优先级 API (Week 1, Day 4-5)

**目标**: 增强事件系统

1. **事件系统增强** (Day 4)
   - [ ] `addEventListener` 选项支持
   - [ ] `dispatchEvent`
   - [ ] 测试 (10+)

2. **查询 API** (Day 5)
   - [ ] `closest`
   - [ ] `matches`
   - [ ] 测试 (5+)

### Phase 3: 低优先级 API (Week 2, Day 3)

**目标**: 高级功能支持

1. **MutationObserver** (可选)
   - [ ] 基础实现
   - [ ] 测试 (5+)

---

## 🎯 验证计划

### 验证方法

1. **单元测试**: 每个 API 至少 3 个测试用例
2. **集成测试**: Preact Hello World 应用
3. **组件库测试**: 运行真实的 Preact 组件

### 验证清单

- [ ] 所有新 API 有单元测试
- [ ] Preact Hello World 运行成功
- [ ] Preact Hooks 运行成功
- [ ] 至少一个组件库可用

---

## 📊 总结

### 好消息 ✅

- MBink 已经实现了 **40+ DOM API**
- 覆盖了 **75%** 的 Preact 所需 API
- 大部分核心 API 已经可用

### 需要补充 ⚠️

- **12 个 API** 需要实现或验证
- 其中 **7 个高优先级** API 必须实现
- 预计 **3-5 天** 可以完成

### 下一步 🚀

1. 开始实现定时器 API (最重要)
2. 验证现有的 `insertBefore` 和 `cloneNode`
3. 实现 `replaceChild`
4. 增强事件系统
5. 运行 Preact Hello World

---

**最后更新**: 2025-11-15  
**维护者**: MBink Team

