# LightUI Python API 重设计方案（B+C 组合）

> **版本**: v1.0 | **日期**: 2025-01
> **目标**: 让 Python 层的使用方式更优雅、更直观
> **约束**: 仅修改 Python 层 + JS hooks，不修改 C++ 代码，完全向后兼容

---

## 一、当前痛点分析

### 痛点 1：手写 JS 监听代码（最大痛点）

每个状态都需要手写 JS `host.on()` 监听器来更新 DOM：

```javascript
// 用户必须在 HTML 中写这段 JS 胶水代码
host.on("counter", function(val) {
    document.getElementById("count").textContent = val;
});
```

**问题**: Python 开发者不想也不应该为了简单的状态显示去手写 JS 代码。

### 痛点 2：`global` 关键字

```python
counter = app.state("counter", 0)

@app.bindable
def increment():
    global counter  # 丑！Python 反模式
    counter += 1
```

`+=` 在函数作用域内被视为局部变量赋值，必须用 `global` 声明。

### 痛点 3：大段内联 HTML

`borderless_demo.py` 有 120 行 HTML 内嵌在 Python 三引号字符串中，没有语法高亮和代码补全。

### 痛点 4：State Wrapper 不完整

| State 类型 | Pythonic Wrapper | 运算符支持 |
|------------|-----------------|-----------|
| IntState | ✅ `_IntStateWrapper` | `+=`, `-=`, `*=`, `int()`, 比较 |
| StringState | ❌ 原始 C++ 对象 | 无 |
| ListState | ❌ 原始 C++ 对象 | 无 |
| DictState | ❌ 原始 C++ 对象 | 无 |

### 痛点 5：`load_preact()` 未实现

`app.py` 第 687 行 `load_preact()` 只有 `return False`（TODO），但项目已有完整的 Preact 库文件和 `useSharedState` hook。

---

## 二、方案设计

### 方案 B：短期改进（消除痛点，仅改 Python 层）

#### B1. 自动状态绑定 — `data-bind` 运行时

**核心思路**: 在 `load_html()` / `load_html_file()` 后自动注入一段 JS 运行时，扫描 `data-bind` 属性并自动创建 `host.on()` 监听。

**改造前 (counter.py)**:
```html
<div id="count">0</div>
<script>
    host.on("counter", function(val) {
        document.getElementById("count").textContent = val;
    });
</script>
```

**改造后**:
```html
<div data-bind="counter">0</div>
<!-- 完了！不需要任何 JS 代码 -->
```

**绑定模式支持**:

| 属性 | 效果 | 适用元素 |
|------|------|---------|
| `data-bind="name"` | 更新 `textContent` | `div`, `span`, `p` 等 |
| `data-bind="name" data-bind-attr="value"` | 更新 `value` | `input`, `textarea` |
| `data-bind="name" data-bind-attr="innerHTML"` | 更新 `innerHTML` | 任意 |
| `data-bind="name" data-bind-attr="style.color"` | 更新 CSS 属性 | 任意 |

**注入的运行时 JS（约 30 行）**:
```javascript
(function() {
    document.querySelectorAll('[data-bind]').forEach(function(el) {
        var name = el.getAttribute('data-bind');
        var attr = el.getAttribute('data-bind-attr') || 'textContent';
        host.on(name, function(val) {
            if (attr === 'textContent') el.textContent = val;
            else if (attr === 'value') el.value = val;
            else if (attr === 'innerHTML') el.innerHTML = val;
            else if (attr.startsWith('style.')) el.style[attr.slice(6)] = val;
            else el.setAttribute(attr, val);
        });
    });
})();
```

#### B2. StringState / ListState / DictState Pythonic Wrappers

为所有 State 类型添加 Pythonic 包装器，统一接口风格。

**StringState Wrapper**:
```python
class _StringStateWrapper:
    # 代理所有原始方法 + 新增:
    # @property value → get()/set() 的属性形式
    # __str__()  → 直接转字符串
    # __iadd__() → greeting += "world" → append
    # __repr__() → StringState(name='hello')
    # __len__()  → 字符串长度
```

**ListState Wrapper**:
```python
class _ListStateWrapper:
    # 代理所有原始方法 + 新增:
    # @property value → get()/set() 的属性形式
    # __iter__() → for item in items
    # __repr__() → ListState(name=[...])
    # __bool__() → 空列表为 False
```

**DictState Wrapper**:
```python
class _DictStateWrapper:
    # 代理所有原始方法 + 新增:
    # @property value → get()/set() 的属性形式
    # __iter__() → for key in config
    # values()   → 获取所有值
    # items()    → 获取所有键值对
    # update()   → 批量更新
    # __repr__() → DictState(name={...})
    # __bool__() → 空字典为 False
```

#### B3. 消除 `global` 关键字需求

**问题根源**: `counter += 1` 在函数内部触发 Python 的局部变量赋值规则。

**解决方案**: 推荐使用方法调用模式，在示例和文档中统一使用 `counter.increment()` 而非 `counter += 1`。

```python
# ✅ 推荐：方法调用（不需要 global）
@app.bindable
def increment():
    counter.increment()

# ❌ 不推荐：运算符（需要 global）
@app.bindable
def increment():
    global counter
    counter += 1
```

#### B4. HTML 文件分离

改进示例，推荐 HTML 文件分离模式：

```python
# 简洁的 Python 逻辑
import lightui as ui

app = ui.App("LightUI 计数器", 400, 300)
counter = app.state("counter", 0)

@app.bindable
def increment():
    counter.increment()

@app.bindable
def decrement():
    counter.decrement()

app.load_html_file("counter.html")  # HTML 分离！
app.run()
```

对应的 `counter.html`（有语法高亮、代码补全）：
```html
<h1>LightUI 计数器</h1>
<div data-bind="counter">0</div>
<button onclick="py.decrement()">- 减少</button>
<button onclick="py.increment()">+ 增加</button>
```

---

### 方案 C：长期 Preact 深度集成

#### C1. `load_preact()` 正式实现

利用项目已有的 `js/preact/` 库和 `js/hooks/use_shared_state.js`：

```python
def load_preact(self, entry_file: str) -> bool:
    """加载 Preact 应用

    自动完成:
    1. 注册 preact/preact-hooks 虚拟 ES 模块
    2. 注册 lightui/hooks (useSharedState) 模块
    3. 设置模块基础路径
    4. 加载用户入口 JS 文件
    """
    # 1. 读取并注册 Preact 库
    preact_path = self._find_preact_lib()
    self._runtime.register_module("preact", read_file(preact_path / "preact.mjs"))
    self._runtime.register_module("preact/hooks", read_file(preact_path / "hooks.mjs"))

    # 2. 注册 useSharedState hook
    hooks_path = self._find_hooks_lib()
    self._runtime.register_module("lightui/hooks", read_file(hooks_path))

    # 3. 设置模块路径 + 加载入口
    self._runtime.set_base_module_path(str(Path(entry_file).parent.resolve()))
    self._runtime.eval_file(str(Path(entry_file).resolve()))
    return True
```

#### C2. Preact 使用示例

```python
# Python: todo_preact.py
import lightui as ui

app = ui.App("Todo App", 500, 450)
todos = app.state("todos", [])

@app.bindable
def addTodo(text):
    if text.strip():
        todos.append(text.strip())

@app.bindable
def removeTodo(index):
    todos.remove(int(index))

app.load_preact("./todo_app.js")
app.run()
```

```javascript
// JS: todo_app.js
import { h, render } from 'preact';
import { useSharedState } from 'lightui/hooks';

function TodoApp() {
    const [todos] = useSharedState('todos', []);

    const handleAdd = () => {
        const input = document.getElementById('todo-input');
        py.addTodo(input.value);
        input.value = '';
    };

    return h('div', { class: 'app' },
        h('h1', null, 'Todo App'),
        h('div', { class: 'input-row' },
            h('input', { id: 'todo-input', placeholder: '添加待办...' }),
            h('button', { onClick: handleAdd }, '添加')
        ),
        h('ul', null,
            todos.map((todo, i) =>
                h('li', { key: i },
                    h('span', null, todo),
                    h('button', { onClick: () => py.removeTodo(i) }, '×')
                )
            )
        )
    );
}

render(h(TodoApp), document.body);
```

---

## 三、实施计划

### 阶段 1 — B 方案（立即可做）

| # | 任务 | 文件 | 复杂度 | 优先级 |
|---|------|------|--------|--------|
| 1 | `_StringStateWrapper` 实现 | `app.py` | 中 | P0 |
| 2 | `_ListStateWrapper` 实现 | `app.py` | 中 | P0 |
| 3 | `_DictStateWrapper` 实现 | `app.py` | 中 | P0 |
| 4 | `state()` 方法适配所有 Wrapper | `app.py` | 低 | P0 |
| 5 | `data-bind` 运行时注入 | `app.py` | 低 | P0 |
| 6 | `load_html()` / `load_html_file()` 自动注入绑定 | `app.py` | 低 | P0 |
| 7 | 示例重写 — counter.py + counter.html | `examples/` | 低 | P1 |
| 8 | 示例重写 — todo_app.py + todo_app.html | `examples/` | 低 | P1 |

### 阶段 2 — C 方案（后续迭代）

| # | 任务 | 文件 | 复杂度 | 优先级 |
|---|------|------|--------|--------|
| 9 | `load_preact()` 正式实现 | `app.py` | 中 | P1 |
| 10 | `useSharedState` 注册为虚拟模块 | `app.py` | 低 | P1 |
| 11 | Preact 示例 — todo_preact.py | `examples/` | 低 | P2 |

---

## 四、改造前后对比

### Counter 示例

**Before（90 行，含手写 JS）**:
```python
# counter.py — 90 行
import lightui as ui
app = ui.App("LightUI 计数器", 400, 300)
counter = app.state("counter", 0)

@app.bindable
def increment():
    global counter
    counter += 1

@app.bindable
def decrement():
    global counter
    counter -= 1

app.load_html("""
<!DOCTYPE html>
<html><head>
    <style>/* ... 30行CSS ... */</style>
</head><body>
    <h1>LightUI 计数器</h1>
    <div id="count">0</div>
    <button onclick="py.decrement()">-</button>
    <button onclick="py.increment()">+</button>
    <script>
        host.on("counter", function(val) {
            document.getElementById("count").textContent = val;
        });
    </script>
</body></html>
""")
app.run()
```

**After（15 行 Python + 独立 HTML）**:
```python
# counter.py — 15 行
import lightui as ui

app = ui.App("LightUI 计数器", 400, 300)
counter = app.state("counter", 0)

@app.bindable
def increment():
    counter.increment()

@app.bindable
def decrement():
    counter.decrement()

app.load_html_file("counter.html")
app.run()
```

```html
<!-- counter.html — 有语法高亮、代码补全 -->
<style>/* CSS 独立管理 */</style>
<h1>LightUI 计数器</h1>
<div data-bind="counter">0</div>
<button onclick="py.decrement()">- 减少</button>
<button onclick="py.increment()">+ 增加</button>
```

**改进效果**:
- Python 代码从 90 行 → 15 行（-83%）
- 消除 `global` 关键字
- 消除手写 JS 监听代码
- HTML 有语法高亮和代码补全
- `data-bind` 自动处理状态→DOM 同步

---

## 五、向后兼容性

| 现有 API | 变化 | 兼容方式 |
|----------|------|---------|
| `app.state()` 返回原始 C++ 对象 | 返回 Wrapper | Wrapper 代理所有原始方法 |
| `app.load_html()` | 新增自动注入 | 不影响现有行为 |
| `host.on()` JS 手写监听 | 仍然有效 | `data-bind` 是可选的 |
| `@app.bindable` | 不变 | 无影响 |
| `app.load_preact()` | 从返回 False → 正式实现 | 新功能 |

**所有现有代码无需修改即可继续运行。**

---

## 六、技术风险

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| `data-bind` 运行时注入时机 | DOM 可能未就绪 | 使用 `DOMContentLoaded` 或在 `load_html()` 完成后立即注入 |
| Wrapper 对象行为差异 | 用户代码依赖 `type()` 检查 | 提供 `._state` 访问原始对象 |
| Preact 模块路径解析 | PyInstaller 打包后路径变化 | 使用 `_MEIPASS` 兼容逻辑 |
| JS 运行时注入与用户脚本冲突 | 覆盖用户的 `host.on()` | `data-bind` 运行时不影响用户手动监听 |
