# LightUI Python Binding

用 Python 构建原生桌面应用——基于 **ctypes + C ABI**，零编译依赖，开箱即用。

## 功能特性

- **零编译依赖** — 纯 Python + ctypes，无需 C++ 编译器或 pybind11
- **跨平台** — Windows / Linux / macOS 统一 C ABI
- **窗口管理** — 创建和控制原生 SDL 窗口，支持无边框/透明/置顶/全屏
- **SharedState** — Python ↔ JS 共享数据通道，写属性即同步
- **Python ↔ JS 通信** — `@app.bind` 注册 Python 函数，JS 通过 `py.func()` 调用
- **Preact 集成** — 内置 Preact 加载器，`SharedState` 变更自动触发重渲染
- **窗口事件** — `on_resize` / `on_focus` / `on_blur` / `on_close` / `on_update`
- **批量更新** — `with state.batch()` 合并多次写入，只触发一次 JS 通知
- **DevTools** — `devtools_open()` / `devtools_close()`

---

## 快速开始

### 安装

```bash
# 1. 编译 C 核心库（仅需一次）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target lightui_api

# 2. 安装 Python 包（开发模式）
cd bindings/python
pip install -e .
```

### 最小示例

```python
from lightui import App

app = App("Hello LightUI", 400, 300)
app.load_html("""
<!DOCTYPE html>
<html>
<body style="display:flex;justify-content:center;align-items:center;height:100vh;">
  <h1>Hello, LightUI!</h1>
</body>
</html>
""")
app.run()
```

---

## API 文档

### `App` 类

```python
from lightui import App

app = App(
    title="My App",        # 窗口标题（默认 "LightUI"）
    width=800,             # 窗口宽度
    height=600,            # 窗口高度
    # ── 关键字参数（均可选）──
    dll_path=None,         # 手动指定 DLL 路径（覆盖自动搜索）
    headless=False,        # 无头模式（不显示窗口，用于测试）
    borderless=False,      # 无边框自定义窗口
    transparent=False,     # 透明背景
    always_on_top=False,   # 始终置顶
    resizable=True,        # 允许用户调整大小
    gpu=True,              # GPU 加速渲染
    fullscreen=False,      # 全屏启动
    min_size=None,         # 最小尺寸 (w, h)，例如 (400, 300)
    max_size=None,         # 最大尺寸 (w, h)
)
```

#### 生命周期

| 方法 | 说明 |
|------|------|
| `run()` | 启动事件循环（**阻塞**，窗口关闭后返回） |
| `stop()` | 请求停止事件循环 |
| `poll() -> bool` | 手动轮询一次事件，返回 `True` 表示窗口存活 |

#### UI 加载

| 方法 | 说明 |
|------|------|
| `load_html(html)` | 加载 HTML 字符串，自动执行内联 `<script>` |
| `load_html_file(path)` | 从文件加载 HTML |
| `eval_js(code)` | 执行 JavaScript 代码（错误打印到 stdout） |
| `eval_module(code, filename="<module>")` | 以 ES6 模块模式执行 JS |
| `load_js_file(path)` | 从文件执行 JavaScript |
| `load_preact(js_file)` | 加载 Preact 库后以普通脚本模式执行入口文件 |

#### SharedState 数据通道

`app.shared(name)` 创建一个共享 C 对象，注册为 JS 全局变量 `globalThis.<name>`。
Python 侧通过属性赋值写入，JS 侧直接通过全局名访问读取。

```python
# 创建共享对象，JS 中通过 globalThis.data 访问
data = app.shared("data")

# 写入各种类型（自动映射 C 原生类型）
data.count   = 0          # int
data.ratio   = 0.5        # float
data.message = "Hello"    # str
data.flag    = True       # bool
data.items   = [1, 2, 3]  # list → JSON
data.info    = {"k": "v"} # dict → JSON
data.empty   = None       # null

# 读回
print(data.count)    # 0
print(data.message)  # "Hello"

# 检查键是否存在 / 删除键
"count" in data      # True
del data.message
```

JS 侧访问（无需任何桥接代码）：
```javascript
// 直接读取全局变量
console.log(data.count);    // 0
console.log(data.message);  // "Hello"
```

每次 Python 写入都会触发 `globalThis.__onSharedUpdate()`，
Preact 应用通过该钩子调度重渲染（见示例）。

#### 批量更新

```python
# 多次写入合并为一次 JS 通知，避免中间状态闪烁
with data.batch():
    data.count   = 10
    data.message = "Updated"
    data.items   = [4, 5, 6]
# 退出 with 块时触发一次 __onSharedUpdate
```

`app.batch()` 是全局批量（跨多个 SharedState）：
```python
with app.batch():
    data.count = 10
    info.title = "New"
```

#### Python ↔ JS 通信

```python
# 注册 Python 函数，JS 通过 py.funcName(args) 调用
@app.bind("increment")
def _(args):
    # args 是 JS 传来的值（已反序列化为 Python 对象）
    # 可以是 None / bool / int / float / str / list / dict
    data.count += 1
    return {"ok": True}   # 返回值序列化为 JSON 传回 JS

# 也可以不用装饰器形式
app.bind("reset")(lambda args: setattr(data, "count", 0))

# 解绑
app.unbind("increment")

# Python 主动向 JS 发送事件（线程安全）
import time
app.emit("tick", {"ts": time.time()})
# JS 监听：window.addEventListener('tick', e => console.log(e.detail))
```

JS 侧调用示例：
```javascript
// 调用无参函数
py.increment()

// 调用带参函数（传 dict）
py.addTodo({text: "buy milk"})

// 获取返回值（同步）
const result = py.compute({x: 1, y: 2})
```

#### 窗口属性与控制

| 属性/方法 | 说明 |
|----------|------|
| `app.title` | 获取/设置窗口标题（property，读写） |
| `app.size` | 获取/设置窗口尺寸 `(w, h)`（property，读写） |
| `app.position` | 获取/设置窗口位置 `(x, y)`（property，读写） |
| `app.set_min_size(w, h)` | 设置最小尺寸（`0` = 不限制） |
| `app.set_max_size(w, h)` | 设置最大尺寸（`0` = 不限制） |
| `app.minimize()` | 最小化 |
| `app.maximize()` | 最大化 |
| `app.restore()` | 从最小/最大化恢复 |
| `app.show()` | 显示窗口 |
| `app.hide()` | 隐藏窗口 |
| `app.set_fullscreen(bool)` | 切换全屏 |
| `app.set_resizable(bool)` | 动态切换可调整大小 |
| `app.set_borderless(bool)` | 动态切换无边框 |
| `app.set_always_on_top(bool)` | 动态切换置顶 |

#### 事件回调

回调通过装饰器注册，均在主线程执行：

```python
@app.on_resize
def _(width, height):
    print(f"窗口尺寸: {width}x{height}")

@app.on_focus
def _():
    print("窗口获得焦点")

@app.on_blur
def _():
    print("窗口失去焦点")

@app.on_close
def _():
    # 窗口关闭时回调（void，无法阻止关闭）
    print("窗口关闭")

@app.on_update
def _(delta_time: float):
    # 每帧回调，delta_time 单位为秒
    pass
```

> **注意**：`on_close` 回调无返回值，**不能**阻止窗口关闭。

#### 开发者工具

```python
app.devtools_open()   # 打开 DevTools 面板
app.devtools_close()  # 关闭 DevTools 面板
```

---

## 示例

### 计数器（SharedState + Preact）

**`main.py`**：
```python
from lightui import App

app  = App("Preact Counter", 400, 350)
data = app.shared("data")   # JS 中可用 data.count
data.count = 0

@app.bind("increment")
def _(args):
    data.count += 1

@app.bind("decrement")
def _(args):
    data.count -= 1

@app.bind("reset")
def _(args):
    data.count = 0

app.load_html("""
<!DOCTYPE html>
<html><head><meta charset="utf-8"></head>
<body><div id="root"></div></body>
</html>""")

app.load_preact("ui/app.js")
app.run()
```

**`ui/app.js`**（Preact，无 import/export，全局变量模式）：
```javascript
// Preact 已由 load_preact() 注入到 globalThis.Preact / globalThis.PreactHooks
const { h, render } = Preact;

function App() {
  return h('div', null,
    h('h1', null, data.count),           // 直接读 globalThis.data.count
    h('button', { onClick: () => py.decrement() }, '−1'),
    h('button', { onClick: () => py.increment() }, '+1'),
    h('button', { onClick: () => py.reset()     }, 'Reset'),
  );
}

// 渲染函数
var _root = document.getElementById('root');
function rerender() { render(h(App), _root); }

// SharedState 更新钩子 — 每次 Python 写入后触发
globalThis.__onSharedUpdate = function() {
  setTimeout(rerender, 0);   // 批处理：下一 tick 才重渲染
};

rerender();  // 首次渲染
```

---

### Todo App

参见 `bindings/python/examples/todo_app/`，演示：
- `app.shared("state")` 多字段（list + str）
- `@app.bind` 增删改 Todo
- `app.title` 动态更新标题
- `with state.batch()` 批量推送

---

### 无边框自定义窗口

```python
from lightui import App

# 构造时直接传 borderless=True（推荐，避免后期切换闪烁）
app = App("Custom Window", 800, 600,
          borderless=True, resizable=True, gpu=False)

app.load_html("""
<!DOCTYPE html>
<html><head>
<style>
  * { margin: 0; box-sizing: border-box; }
  .titlebar {
    -webkit-app-region: drag;        /* 可拖拽移动窗口 */
    height: 40px; background: #333; color: #fff;
    display: flex; align-items: center; padding: 0 12px;
  }
  .controls { -webkit-app-region: no-drag; margin-left: auto; display: flex; gap: 8px; }
  /* -webkit-window-control 让浏览器处理最小化/最大化/关闭/置顶 */
  .btn-pin      { -webkit-window-control: pin;      width:14px; height:14px; border-radius:50%; background:#59a8f8; border:none; cursor:pointer; }
  .btn-minimize { -webkit-window-control: minimize; width:14px; height:14px; border-radius:50%; background:#febc2e; border:none; cursor:pointer; }
  .btn-maximize { -webkit-window-control: maximize; width:14px; height:14px; border-radius:50%; background:#28c840; border:none; cursor:pointer; }
  .btn-close    { -webkit-window-control: close;    width:14px; height:14px; border-radius:50%; background:#ff5f57; border:none; cursor:pointer; }
</style>
</head><body>
<div class="titlebar">
  <span>My App</span>
  <div class="controls">
    <button class="btn-pin"></button>
    <button class="btn-minimize"></button>
    <button class="btn-maximize"></button>
    <button class="btn-close"></button>
  </div>
</div>
<div style="padding:20px">内容区域</div>
</body></html>
""")

app.run()
```

CSS 关键属性说明：

| CSS 属性 | 值 | 效果 |
|----------|-----|------|
| `-webkit-app-region` | `drag` | 区域可拖拽移动窗口 |
| `-webkit-app-region` | `no-drag` | 区域不参与拖拽（按钮等交互控件） |
| `-webkit-window-control` | `pin` | 按钮控制置顶切换 |
| `-webkit-window-control` | `minimize` | 按钮控制最小化 |
| `-webkit-window-control` | `maximize` | 按钮控制最大化/还原 |
| `-webkit-window-control` | `close` | 按钮控制关闭窗口 |

---

## DLL 查找路径

`App` 初始化时按以下顺序搜索 `lightui.dll` / `liblightui.so` / `liblightui.dylib`：

1. `dll_path` 构造参数（若指定）
2. 环境变量 `LIGHTUI_DLL_PATH` 指向的目录
3. 包目录（`lightui/`）
4. 包目录下 `bin/` 子目录
5. 当前工作目录
6. `<项目根>/build/bin/Release/`
7. `<项目根>/build/bin/Debug/`
8. 系统库路径（`ctypes.util.find_library`）

```bash
# Windows
set LIGHTUI_DLL_PATH=D:\path\to\build\bin\Release

# Linux / macOS
export LIGHTUI_DLL_PATH=/path/to/build/bin/Release
```

## Preact 文件路径

`load_preact()` 按以下优先级查找 `preact.js` / `hooks.js`：

1. 环境变量 `LIGHTUI_ROOT` 指向的 `js/preact/` 目录
2. 从包目录向上推导的项目根目录下 `js/preact/`

```bash
set LIGHTUI_ROOT=D:\path\to\MBink   # Windows
export LIGHTUI_ROOT=/path/to/MBink  # Linux/macOS
```

---

## 运行示例

```bash
# 计数器（Preact + SharedState）
cd bindings/python/examples/counter_preact
python main.py

# Todo App
cd bindings/python/examples/todo_app
python main.py

# 财务追踪器
cd bindings/python/examples/finance_tracker
python main.py

# 无边框窗口演示
cd bindings/python
python examples/borderless_demo.py
```

---

## 架构

```
Python 应用（ctypes）
       │
       ▼
lightui.dll / liblightui.so     ← C ABI（lightui.h）
       │
       ▼
C++ 核心层
  ┌────────────────────────────────────────┐
  │  lightui_create_ex(LightUIConfig)      │  窗口 + JS 运行时初始化
  │  lightui_shared_create(handle, name)   │  创建 SharedObject → JS globalThis.name
  │  lightui_bind(handle, name, callback)  │  注册 py.func → Python 回调
  │  lightui_emit(handle, event, data)     │  Python → JS CustomEvent
  └────────────────────────────────────────┘
       │
       ▼
QuickJS 运行时
  globalThis.<name>     ← SharedObject（C 原生对象，Python 读写）
  globalThis.__onSharedUpdate()   ← 写入触发 Preact 重渲染
  py.<funcName>(args)  ← 调用已绑定的 Python 函数
```

---

## 注意事项

1. **DLL 路径**：若 DLL 找不到会抛出 `FileNotFoundError`，设置 `LIGHTUI_DLL_PATH` 解决。
2. **SharedState 初始化时机**：建议在 `load_html()` **之后**设置初始值，确保 JS runtime 已就绪。
3. **JS 全局访问**：`app.shared("data")` 创建的对象在 JS 中是 `globalThis.data`（直接全局变量），**不是** `window.shared.data`。
4. **Preact 模式**：`load_preact()` 将 Preact 注入为 `globalThis.Preact` / `globalThis.PreactHooks`，UI 文件用普通脚本模式执行（无 import/export）。
5. **回调线程**：所有 `@app.bind` / `on_*` 回调均在主线程执行，无需加锁。
6. **`on_close` 无法阻止关闭**：回调为 void，若需确认关闭逻辑，请在 `@app.bind` 中实现自定义关闭按钮。
7. **`run()` 后自动清理**：事件循环结束后自动调用 `_cleanup()`，销毁所有 SharedObject 和窗口句柄。

---

## 许可证

MIT License
