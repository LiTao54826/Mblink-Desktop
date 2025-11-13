# Phase 4: Preact 原生 Hooks 集成计划

> **创建日期**: 2025-11-13  
> **分支**: `feature/phase4-preact-native-hooks`  
> **预计时间**: 2周  
> **优先级**: P0

---

## 🎯 核心目标

**使用 Preact 原生 Hooks 实现，而不是用 C++ 重新实现**

### 当前状态分析

根据 `PREACT_INTEGRATION_COMPLETE.md`：

✅ **已完成**:
- Preact 核心库 (`preact.js`) - Virtual DOM 创建和渲染
- Hooks API 框架 (`hooks.js`) - 仅提供 API，无实际功能
- PreactRenderer - VNode → DOM 转换
- 20个测试全部通过

❌ **问题**:
- **Hooks 仅是 API 框架**：useState、useEffect 等没有实际状态管理功能
- **没有重渲染机制**：状态更新不会触发组件重新渲染
- **没有事件处理**：onClick 等事件不工作
- **没有 Virtual DOM Diffing**：每次都是全量渲染

### 错误的方向（已暂停）

之前在 `feature/phase4-hooks-state-management` 分支尝试用 C++ 实现 Hooks：
- ❌ 创建了 ComponentManager、HooksBindings 等 C++ 类
- ❌ 试图在 C++ 层管理组件状态
- ❌ 违背了"使用 Preact 原生实现"的设计原则

### 正确的方向

**集成完整的 Preact 库（包括原生 Hooks 实现）**

---

## 📋 任务分解

### Task 1: 集成完整的 Preact 库 (3天)

**目标**: 替换当前的简化版 preact.js，使用官方完整版

#### 1.1 下载 Preact 官方库
- [ ] 下载 Preact 10.x 源码到 `third_party/preact-official/`
- [ ] 下载 preact/hooks 模块
- [ ] 下载 preact/compat (可选，用于 React 生态兼容)

#### 1.2 构建 Preact 为单文件
- [ ] 使用 Rollup/Webpack 打包 Preact + Hooks 为单个 JS 文件
- [ ] 或直接使用 CDN 的 UMD 版本
- [ ] 确保导出 `h`, `render`, `Component`, `createContext` 等 API
- [ ] 确保导出所有 Hooks: `useState`, `useEffect`, `useRef`, `useMemo`, `useCallback`, `useContext`, `useReducer`

#### 1.3 替换现有的 preact.js
- [ ] 备份当前的 `js/preact/preact.js` 为 `preact.js.old`
- [ ] 替换为官方版本
- [ ] 删除 `js/preact/hooks.js`（官方版本已包含）

#### 1.4 更新 QuickJS 加载逻辑
- [ ] 修改 `QuickJSRuntime` 加载 Preact 的方式
- [ ] 确保 Preact 全局对象正确暴露
- [ ] 测试基础 API 可用性

**验收标准**:
- ✅ 可以在 QuickJS 中 `import { h, render } from 'preact'`
- ✅ 可以在 QuickJS 中 `import { useState, useEffect } from 'preact/hooks'`
- ✅ 基础渲染测试通过

---

### Task 2: 实现组件重渲染机制 (4天)

**目标**: 让 Preact 的状态更新能触发 MBink DOM 更新

#### 2.1 理解 Preact 的渲染流程
- [ ] 研究 Preact 如何调度重渲染
- [ ] 研究 Preact 的 Virtual DOM Diff 算法
- [ ] 确定需要桥接的关键点

#### 2.2 实现 DOM 更新桥接
Preact 更新 Virtual DOM 后，需要同步到 MBink DOM：

```cpp
// core/quickjs/preact_renderer.h
class PreactRenderer {
public:
    // 新增：Diff 和 Patch 方法
    static void Patch(Element* container, JSValue oldVNode, JSValue newVNode);
    static void DiffAndPatch(Element* element, JSValue oldVNode, JSValue newVNode);
    
private:
    // 新增：Diff 辅助方法
    static void UpdateElement(Element* element, JSValue oldProps, JSValue newProps);
    static void UpdateChildren(Element* parent, JSValue oldChildren, JSValue newChildren);
};
```

#### 2.3 集成 Preact 的调度器
- [ ] 监听 Preact 的 `options.debounceRendering` 钩子
- [ ] 将 Preact 的重渲染请求转发到 MBink 事件循环
- [ ] 确保渲染在正确的时机执行

#### 2.4 测试重渲染
```javascript
function Counter() {
    const [count, setCount] = useState(0);
    return h('div', null,
        h('p', null, `Count: ${count}`),
        h('button', { onClick: () => setCount(count + 1) }, '+')
    );
}
```

**验收标准**:
- ✅ useState 更新后组件自动重渲染
- ✅ DOM 正确更新（不是全量替换）
- ✅ 10个重渲染测试通过

---

### Task 3: 实现事件处理 (3天)

**目标**: 让 onClick、onChange 等事件正常工作

#### 3.1 扩展 PreactRenderer 的事件处理
```cpp
void PreactRenderer::ApplyProps(Element* element, JSValue props) {
    // ... 现有代码 ...
    
    // 新增：事件处理
    if (name.starts_with("on")) {
        std::string eventName = name.substr(2);  // "onClick" -> "click"
        std::transform(eventName.begin(), eventName.end(), eventName.begin(), ::tolower);
        
        // 注册事件监听器
        element->addEventListener(eventName, value);
    }
}
```

#### 3.2 实现 addEventListener 的 JSValue 重载
```cpp
// core/dom/element.h
class Element {
public:
    void addEventListener(const std::string& type, JSValue listener);
    void removeEventListener(const std::string& type, JSValue listener);
    
private:
    std::unordered_map<std::string, std::vector<JSValue>> js_event_listeners_;
};
```

#### 3.3 事件触发时调用 JavaScript 回调
```cpp
void Element::DispatchEvent(Event* event) {
    // ... 现有代码 ...
    
    // 调用 JavaScript 监听器
    auto it = js_event_listeners_.find(event->type());
    if (it != js_event_listeners_.end()) {
        for (auto& listener : it->second) {
            JSValue result = JS_Call(ctx, listener, JS_UNDEFINED, 1, &event_obj);
            JS_FreeValue(ctx, result);
        }
    }
}
```

**验收标准**:
- ✅ onClick 事件正常触发
- ✅ onChange 事件正常触发
- ✅ 事件对象正确传递
- ✅ 8个事件测试通过

---

### Task 4: useRef 与 DOM 桥接 (2天)

**目标**: 让 useRef 可以访问真实的 MBink DOM 节点

#### 4.1 实现 ref 回调支持
```cpp
void PreactRenderer::ApplyProps(Element* element, JSValue props) {
    // ... 现有代码 ...
    
    // 处理 ref
    JSValue ref = JS_GetPropertyStr(ctx, props, "ref");
    if (JS_IsFunction(ctx, ref)) {
        // 调用 ref 回调，传入 DOM 元素
        JSValue args[] = { element->ToJSValue(ctx) };
        JSValue result = JS_Call(ctx, ref, JS_UNDEFINED, 1, args);
        JS_FreeValue(ctx, result);
    } else if (JS_IsObject(ref)) {
        // 设置 ref.current
        JS_SetPropertyStr(ctx, ref, "current", element->ToJSValue(ctx));
    }
    JS_FreeValue(ctx, ref);
}
```

**验收标准**:
- ✅ useRef 可以获取 DOM 元素
- ✅ ref.current 指向正确的元素
- ✅ 5个 ref 测试通过

---

### Task 5: useEffect 与事件循环集成 (2天)

**目标**: 让 useEffect 在正确的时机执行

#### 5.1 监听 Preact 的 effect 调度
Preact 会在渲染后调用 effects，我们需要确保这发生在 MBink 事件循环中：

```javascript
// 在 Preact 初始化时设置
Preact.options.requestAnimationFrame = (callback) => {
    // 使用 MBink 的 requestAnimationFrame
    requestAnimationFrame(callback);
};
```

#### 5.2 确保 cleanup 函数正确执行
- [ ] 测试组件卸载时 cleanup 被调用
- [ ] 测试依赖变化时 cleanup 被调用

**验收标准**:
- ✅ useEffect 在渲染后执行
- ✅ cleanup 函数正确执行
- ✅ 依赖数组正确工作
- ✅ 6个 effect 测试通过

---

### Task 6: 创建完整示例 (2天)

**目标**: 验证所有功能正常工作

#### 6.1 Counter 示例（useState + 事件）
```javascript
function Counter() {
    const [count, setCount] = useState(0);
    return h('div', null,
        h('p', null, `Count: ${count}`),
        h('button', { onClick: () => setCount(count + 1) }, '+'),
        h('button', { onClick: () => setCount(count - 1) }, '-')
    );
}
```

#### 6.2 Timer 示例（useEffect）
```javascript
function Timer() {
    const [seconds, setSeconds] = useState(0);
    useEffect(() => {
        const timer = setInterval(() => setSeconds(s => s + 1), 1000);
        return () => clearInterval(timer);
    }, []);
    return h('div', null, `Seconds: ${seconds}`);
}
```

#### 6.3 Input 示例（useRef）
```javascript
function FocusInput() {
    const inputRef = useRef(null);
    return h('div', null,
        h('input', { ref: inputRef }),
        h('button', { onClick: () => inputRef.current.focus() }, 'Focus')
    );
}
```

#### 6.4 Todo App 示例（综合）
完整的 Todo 应用，包含：
- useState 管理 todo 列表
- 输入框添加 todo
- 点击删除 todo
- 显示 todo 数量

**验收标准**:
- ✅ 4个示例全部运行成功
- ✅ 每个示例都有对应的测试

---

## 📊 里程碑

| 里程碑 | 日期 | 描述 |
|--------|------|------|
| M1: Preact 集成完成 | Day 3 | 官方 Preact 库可用 |
| M2: 重渲染机制完成 | Day 7 | useState 触发重渲染 |
| M3: 事件处理完成 | Day 10 | onClick 等事件工作 |
| M4: Hooks 完全可用 | Day 12 | useRef、useEffect 工作 |
| M5: Phase 4 完成 | Day 14 | 所有示例运行成功 |

---

## 🎯 验收标准

### 功能验收
- [ ] 可以使用 Preact 官方库（不是简化版）
- [ ] useState 完全工作（状态更新 + 重渲染）
- [ ] useEffect 完全工作（副作用 + cleanup）
- [ ] useRef 完全工作（访问 DOM）
- [ ] 事件处理完全工作（onClick、onChange 等）
- [ ] 4个完整示例运行成功

### 性能验收
- [ ] 组件渲染 < 16ms (60 FPS)
- [ ] 状态更新 < 5ms
- [ ] Virtual DOM Diff < 10ms

### 质量验收
- [ ] 50+ 测试用例全部通过
- [ ] 测试覆盖率 > 85%
- [ ] 无内存泄漏
- [ ] 无崩溃

---

## 📚 参考资料

- [Preact 官方文档](https://preactjs.com/)
- [Preact Hooks 文档](https://preactjs.com/guide/v10/hooks/)
- [Preact 源码](https://github.com/preactjs/preact)
- [Virtual DOM 原理](https://preactjs.com/guide/v10/differences-to-react/#raw-html-attributedangerouslysetinnerhtml)

---

## 🔄 与错误方向的对比

| 方面 | ❌ C++ 实现 Hooks | ✅ Preact 原生 Hooks |
|------|------------------|---------------------|
| 开发时间 | 4周+ | 2周 |
| 代码量 | 2000+ 行 C++ | 500 行桥接代码 |
| 维护成本 | 高（需要维护完整实现） | 低（只维护桥接） |
| 生态兼容 | 差（可能不兼容） | 好（完全兼容） |
| 性能 | 理论上更好 | 实际差异不大 |
| 调试难度 | 高（跨语言） | 低（纯 JavaScript） |

**结论**: Preact 原生 Hooks 是正确的选择！


