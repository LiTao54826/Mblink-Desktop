# Phase 3: React生态支持 - 开发计划

> **开始日期**: 2025-11-12  
> **预计完成**: 2025-12-10 (4周)  
> **当前进度**: 0% → 目标100%  
> **优先级**: P0 (核心功能)

---

## 🎯 Phase 3 目标

### 核心目标
1. **集成Preact**：轻量级React替代品（3KB gzipped）
2. **支持React Hooks**：useState, useEffect, useRef, useContext等
3. **Virtual DOM映射**：Preact Virtual DOM → MBink DOM
4. **组件化开发**：支持函数组件和类组件
5. **验证组件库**：测试Ant Design/Material-UI等主流组件库

### 技术选型

| 技术 | 版本 | 大小 | 理由 |
|------|------|------|------|
| **Preact** | 10.x | 3KB | 轻量级、API兼容React、性能优秀 |
| **Preact Hooks** | 内置 | - | 完整Hooks支持 |
| **Preact Compat** | 可选 | 2KB | React生态兼容层 |
| **HTM** | 3.x | 0.7KB | JSX替代方案（无需编译） |

### 为什么选择Preact而不是React？

1. **体积优势**：Preact 3KB vs React 40KB（13倍差距）
2. **性能优势**：更快的Virtual DOM diff算法
3. **API兼容**：99%兼容React API
4. **生态兼容**：通过preact/compat支持React组件库
5. **符合MBink定位**：轻量级桌面应用框架

---

## 📋 任务分解

### P0: Preact核心集成 (必须完成，2周)

#### Task 1: Preact库集成和构建系统 (2天)

**目标**: 将Preact集成到MBink构建系统

**子任务**:
- [ ] Task 1.1: 下载Preact源码到third_party/preact
- [ ] Task 1.2: 配置CMake构建Preact
- [ ] Task 1.3: 创建Preact QuickJS绑定
- [ ] Task 1.4: 实现Preact模块加载
- [ ] Task 1.5: 编写基础测试

**技术细节**:
```cmake
# third_party/preact/CMakeLists.txt
add_library(preact INTERFACE)
target_include_directories(preact INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# 将preact.js编译为QuickJS字节码
add_custom_command(
    OUTPUT preact.qjsc
    COMMAND qjsc -o preact.qjsc -m preact.js
    DEPENDS preact.js
)
```

**验收标准**:
- ✅ Preact库成功集成到构建系统
- ✅ 可以在QuickJS中import Preact
- ✅ 基础测试通过（createElement, render）

**文件位置**:
- `third_party/preact/` - Preact源码
- `core/quickjs/preact_bindings.h` - Preact绑定
- `tests/test_preact_integration.cpp` - 集成测试

---

#### Task 2: Virtual DOM到MBink DOM的映射 (3天)

**目标**: 实现Preact Virtual DOM到MBink DOM的双向映射

**核心挑战**:
1. **Virtual DOM → Real DOM**: Preact渲染时创建MBink DOM节点
2. **Real DOM → Virtual DOM**: MBink事件触发Preact更新
3. **Diff算法集成**: 利用Preact的diff算法优化DOM更新

**架构设计**:
```
┌─────────────────────────────────────────┐
│  Preact Component                       │
│  (JSX/HTM)                              │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Preact Virtual DOM                     │
│  (VNode树)                              │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  PreactRenderer (新增)                  │
│  - createElement()                      │
│  - updateElement()                      │
│  - removeElement()                      │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  MBink DOM                              │
│  (Document, Element, Text)              │
└─────────────────────────────────────────┘
```

**实现方案**:
```cpp
// core/quickjs/preact_renderer.h
class PreactRenderer {
public:
    // Preact调用的渲染函数
    static JSValue Render(JSContext* ctx, JSValueConst this_val, 
                          int argc, JSValueConst* argv);
    
    // Virtual DOM节点 → MBink DOM节点
    static std::shared_ptr<Element> CreateElement(
        JSContext* ctx, JSValueConst vnode);
    
    // 更新现有DOM节点
    static void UpdateElement(
        std::shared_ptr<Element> element, 
        JSValueConst vnode);
    
    // 删除DOM节点
    static void RemoveElement(std::shared_ptr<Element> element);
    
private:
    // VNode属性 → Element属性
    static void ApplyProps(
        std::shared_ptr<Element> element,
        JSValueConst props);
    
    // VNode子节点 → Element子节点
    static void ApplyChildren(
        std::shared_ptr<Element> element,
        JSValueConst children);
};
```

**关键API映射**:

| Preact API | MBink DOM API | 说明 |
|------------|---------------|------|
| `h(type, props, ...children)` | `Document::CreateElement()` | 创建元素 |
| `render(vnode, container)` | `Element::AppendChild()` | 渲染到容器 |
| `vnode.props` | `Element::SetAttribute()` | 设置属性 |
| `vnode.props.style` | `Element::SetStyle()` | 设置样式 |
| `vnode.props.onClick` | `Element::AddEventListener()` | 绑定事件 |
| `vnode.children` | `Element::AppendChild()` | 添加子节点 |

**验收标准**:
- ✅ 可以渲染简单的Preact组件到MBink DOM
- ✅ 属性和样式正确映射
- ✅ 事件监听器正确绑定
- ✅ 子节点正确渲染
- ✅ 10个单元测试通过

**文件位置**:
- `core/quickjs/preact_renderer.h`
- `core/quickjs/preact_renderer.cpp`
- `tests/test_preact_renderer.cpp`

---

#### Task 3: React Hooks支持 (2天)

**目标**: 实现完整的React Hooks支持

**需要支持的Hooks**:

| Hook | 优先级 | 说明 |
|------|--------|------|
| `useState` | P0 | 状态管理 |
| `useEffect` | P0 | 副作用 |
| `useRef` | P0 | 引用DOM节点 |
| `useContext` | P1 | 上下文 |
| `useReducer` | P1 | 复杂状态 |
| `useMemo` | P2 | 性能优化 |
| `useCallback` | P2 | 性能优化 |
| `useLayoutEffect` | P2 | 同步副作用 |

**实现方案**:

Preact已经内置了完整的Hooks支持，我们只需要：
1. 确保Preact Hooks模块正确加载
2. 实现`useRef`与MBink DOM的桥接
3. 实现`useEffect`与MBink事件循环的集成

```javascript
// 示例：使用Hooks的计数器组件
import { h, render } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';

function Counter() {
    const [count, setCount] = useState(0);
    const buttonRef = useRef(null);
    
    useEffect(() => {
        console.log('Count changed:', count);
    }, [count]);
    
    return h('div', null,
        h('p', null, `Count: ${count}`),
        h('button', {
            ref: buttonRef,
            onClick: () => setCount(count + 1)
        }, 'Increment')
    );
}

render(h(Counter), document.body);
```

**验收标准**:
- ✅ useState正常工作
- ✅ useEffect正常工作
- ✅ useRef可以访问MBink DOM节点
- ✅ 15个Hooks测试通过

**文件位置**:
- `js/preact/hooks_integration.js`
- `tests/test_preact_hooks.cpp`

---

#### Task 4: HTM集成（JSX替代方案）(1天)

**目标**: 集成HTM，提供无需编译的JSX替代方案

**为什么需要HTM？**
- JSX需要编译（Babel/TypeScript）
- HTM使用模板字符串，无需编译
- 语法接近JSX，学习成本低
- 体积仅0.7KB

**HTM语法示例**:
```javascript
import { h, render } from 'preact';
import htm from 'htm';

const html = htm.bind(h);

function App() {
    return html`
        <div class="app">
            <h1>Hello MBink!</h1>
            <button onClick=${() => alert('Clicked!')}>
                Click Me
            </button>
        </div>
    `;
}

render(html`<${App} />`, document.body);
```

**验收标准**:
- ✅ HTM库成功集成
- ✅ 可以使用HTM语法编写组件
- ✅ 5个HTM测试通过

**文件位置**:
- `third_party/htm/` - HTM源码
- `examples/preact_htm_example.js`

---

#### Task 5: 基础组件示例 (2天)

**目标**: 创建一系列基础组件示例，验证功能完整性

**示例列表**:

1. **Hello World** (最简单)
```javascript
function HelloWorld() {
    return html`<h1>Hello MBink with Preact!</h1>`;
}
```

2. **Counter** (useState)
```javascript
function Counter() {
    const [count, setCount] = useState(0);
    return html`
        <div>
            <p>Count: ${count}</p>
            <button onClick=${() => setCount(count + 1)}>+</button>
        </div>
    `;
}
```

3. **Todo List** (useState + 列表渲染)
```javascript
function TodoList() {
    const [todos, setTodos] = useState([]);
    const [input, setInput] = useState('');
    
    const addTodo = () => {
        setTodos([...todos, input]);
        setInput('');
    };
    
    return html`
        <div>
            <input value=${input} onInput=${e => setInput(e.target.value)} />
            <button onClick=${addTodo}>Add</button>
            <ul>
                ${todos.map(todo => html`<li>${todo}</li>`)}
            </ul>
        </div>
    `;
}
```

4. **Form** (表单处理)
5. **Timer** (useEffect)
6. **Refs Example** (useRef)

**验收标准**:
- ✅ 6个示例全部运行成功
- ✅ 每个示例都有对应的测试
- ✅ 示例代码有详细注释

**文件位置**:
- `examples/preact/01_hello_world.js`
- `examples/preact/02_counter.js`
- `examples/preact/03_todo_list.js`
- `examples/preact/04_form.js`
- `examples/preact/05_timer.js`
- `examples/preact/06_refs.js`

---

### P1: 组件库验证 (重要，1周)

#### Task 6: Preact Compat集成 (2天)

**目标**: 集成preact/compat，支持React生态组件库

**preact/compat是什么？**
- React API的完整兼容层
- 允许使用React组件库（Ant Design, Material-UI等）
- 体积仅增加2KB

**实现方案**:
```javascript
// 配置别名，将react映射到preact/compat
import { createElement, Component } from 'preact/compat';
import { render } from 'preact';

// 现在可以使用React组件库
import { Button } from 'antd'; // 假设已集成

render(<Button type="primary">Click Me</Button>, document.body);
```

**验收标准**:
- ✅ preact/compat成功集成
- ✅ 可以使用React API
- ✅ 10个兼容性测试通过

---

#### Task 7: 组件库测试 (3天)

**目标**: 测试主流React组件库的兼容性

**测试组件库**:

| 组件库 | 优先级 | 测试内容 |
|--------|--------|---------|
| **Ant Design** | P0 | Button, Input, Form, Table |
| **Material-UI** | P1 | Button, TextField, Dialog |
| **Chakra UI** | P2 | Button, Box, Stack |

**测试用例**:
1. 组件渲染测试
2. 事件处理测试
3. 样式应用测试
4. 表单交互测试

**验收标准**:
- ✅ Ant Design核心组件可用
- ✅ Material-UI核心组件可用
- ✅ 20个组件测试通过

---

### P2: 性能优化和文档 (可选，1周)

#### Task 8: 性能优化 (3天)

**优化项**:
1. Virtual DOM diff优化
2. 事件委托优化
3. 批量更新优化
4. 内存管理优化

**性能目标**:
- 组件渲染 < 16ms (60 FPS)
- 状态更新 < 5ms
- 内存占用 < 50MB

---

#### Task 9: 文档和示例 (2天)

**文档内容**:
1. Preact集成指南
2. 组件开发指南
3. Hooks使用指南
4. 性能优化指南
5. 常见问题解答

---

## 📊 里程碑

| 里程碑 | 日期 | 描述 |
|--------|------|------|
| M1: Preact集成完成 | Day 7 | Preact + Virtual DOM映射 |
| M2: Hooks支持完成 | Day 10 | 完整Hooks支持 |
| M3: 组件库验证完成 | Day 17 | Ant Design可用 |
| M4: Phase 3完成 | Day 28 | 100%功能完成 |

---

## 🎯 验收标准

### 功能验收
- [ ] 可以使用Preact编写组件
- [ ] 支持完整的React Hooks
- [ ] 支持HTM语法（无需编译）
- [ ] 可以使用Ant Design组件库
- [ ] 6个基础示例全部运行
- [ ] 1个完整Todo App示例

### 性能验收
- [ ] 组件渲染 < 16ms
- [ ] 状态更新 < 5ms
- [ ] 内存占用 < 50MB

### 质量验收
- [ ] 单元测试覆盖率 > 90%
- [ ] 集成测试通过率 100%
- [ ] 无内存泄漏
- [ ] 无已知严重Bug

---

## 📚 参考资料

- [Preact官方文档](https://preactjs.com/)
- [Preact Hooks文档](https://preactjs.com/guide/v10/hooks/)
- [HTM文档](https://github.com/developit/htm)
- [React官方文档](https://react.dev/)

---

**最后更新**: 2025-11-12  
**维护者**: MBink Team

