# LightUI Python Binding

LightUI 的 Python 绑定，使用 pybind11 编译成原生扩展模块。

## 功能特性

- **窗口管理**: 创建和管理原生窗口
- **状态管理**: 响应式数据绑定，支持多种数据类型
- **Python ↔ JS 通信**: 双向函数调用和数据传递
- **DOM 操作**: 创建和操作 HTML 元素
- **事件循环**: 完整的事件处理系统

## 安装

### 从源码构建

```bash
# 安装依赖
pip install pybind11

# 配置 CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLIGHTUI_BUILD_PYTHON_BINDING=ON

# 构建
cmake --build build --config Release --target lightui_core

# 模块位置
# Windows: build/python/Release/lightui_core.pyd
# Linux/macOS: build/python/lightui_core.so
```

### 使用

```python
# 方式 1: 使用高级 API
from lightui import LightUIApp

with LightUIApp("My App", 800, 600) as app:
    counter = app.state("counter", 0)
    
    @app.bind("increment")
    def increment(args):
        counter.increment()
        return counter.get()
    
    app.load_html("<button onclick='host.call(\"increment\")'>Click</button>")
    app.run()

# 方式 2: 使用低级 API
import lightui_core as lui

app = lui.App()
window = lui.Window("Test", 800, 600)
runtime = lui.Runtime()
bridge = lui.HostBridge(runtime, app)
```

## API 文档

### 高级 API

#### LightUIApp

整合所有组件的高级应用类。

```python
from lightui import LightUIApp

app = LightUIApp(
    title="My App",      # 窗口标题
    width=800,           # 窗口宽度
    height=600,          # 窗口高度
    headless=False       # 是否无头模式
)
```

**方法:**

| 方法 | 说明 |
|------|------|
| `state(name, initial)` | 创建或获取状态 |
| `bind(name)` | 绑定 Python 函数（装饰器） |
| `unbind(name)` | 解绑函数 |
| `load_html(html)` | 加载 HTML 字符串 |
| `load_html_file(path)` | 加载 HTML 文件 |
| `load_js(code)` | 执行 JavaScript 代码 |
| `load_js_file(path)` | 执行 JavaScript 文件 |
| `run()` | 运行事件循环 |
| `stop()` | 停止事件循环 |
| `batch()` | 返回批量操作上下文管理器 |

### 状态类型

根据初始值类型自动推断：

| Python 类型 | State 类型 | 特殊方法 |
|------------|-----------|---------|
| `int` | `IntState` | `increment()`, `multiply()` |
| `str` | `StringState` | `append()`, `prepend()`, `__len__()` |
| `list` | `ListState` | `append()`, `pop()`, `remove()`, `clear()`, `__len__()`, `__getitem__()`, `__setitem__()` |
| `dict` | `DictState` | `set_key()`, `remove_key()`, `clear()`, `keys()`, `__getitem__()`, `__setitem__()`, `__contains__()` |
| `float` | `State` | - |
| `bool` | `State` | - |
| `None` | `State` | - |

**通用方法:**

| 方法 | 说明 |
|------|------|
| `get()` | 获取当前值 |
| `set(value)` | 设置值 |
| `watch(callback)` | 监听变化，返回 watch_id |
| `unwatch(watch_id)` | 取消监听 |

### Python ↔ JS 通信

#### 绑定 Python 函数

```python
@app.bind("getData")
def get_data(args):
    return {"items": [1, 2, 3]}
```

#### JS 调用 Python 函数

```javascript
const result = host.call("getData", {param: "value"});
```

#### JS 访问状态

```javascript
// 读取状态
const value = host.state.get("counter");

// 设置状态
host.state.set("counter", 42);

// 监听状态变化
const watchId = host.state.watch("counter", (name, value) => {
    console.log(`${name} changed to ${value}`);
});

// 取消监听
host.state.unwatch(watchId);
```

### 低级 API

#### Window

```python
window = lui.Window("Title", 800, 600, headless=False)
window.show()
window.hide()
window.close()
window.title = "New Title"
window.set_size(1024, 768)
window.set_position(100, 100)
```

#### Document

```python
doc = window.document
div = doc.create_element("div")
div.id = "my-div"
div.class_name = "container"
div.inner_html = "<span>Hello</span>"
doc.load_html("<html><body>...</body></html>")
```

#### Runtime

```python
runtime = lui.Runtime()
result = runtime.eval("1 + 2")  # 3
result = runtime.eval_module("export const x = 42;")
result = runtime.eval_file("script.js")
```

#### EventLoop

```python
loop = lui.EventLoop()
loop.set_update_callback(lambda dt: print(f"Update: {dt}"))
loop.set_render_callback(lambda: print("Render"))
loop.run()  # 阻塞
loop.stop()
```

#### HostBridge

```python
bridge = lui.HostBridge(runtime, app)
bridge.bind("myFunc", lambda args: {"result": "ok"})
bridge.unbind("myFunc")
```

## 示例

### Hello World

```python
from lightui import LightUIApp

with LightUIApp("Hello World", 400, 300) as app:
    app.load_html('''
        <html>
        <body style="display:flex;justify-content:center;align-items:center;height:100vh;">
            <h1>Hello, LightUI!</h1>
        </body>
        </html>
    ''')
    app.run()
```

### 计数器

```python
from lightui import LightUIApp

with LightUIApp("Counter", 400, 300) as app:
    counter = app.state("counter", 0)
    
    @app.bind("increment")
    def increment(args):
        counter.increment()
        return counter.get()
    
    @app.bind("getCount")
    def get_count(args):
        return counter.get()
    
    app.load_html('''
        <html>
        <body>
            <h1 id="count">0</h1>
            <button onclick="increment()">+</button>
            <script>
                function increment() {
                    const value = host.call('increment', null);
                    document.getElementById('count').textContent = value;
                }
            </script>
        </body>
        </html>
    ''')
    app.run()
```

### 状态监听

```python
from lightui import LightUIApp

with LightUIApp("Watch Demo", 800, 600, headless=True) as app:
    counter = app.state("counter", 0)
    
    def on_change(value):
        print(f"Counter changed to: {value}")
    
    watch_id = counter.watch(on_change)
    
    counter.increment()  # 触发回调
    counter.increment()  # 触发回调
    
    counter.unwatch(watch_id)
    counter.increment()  # 不触发回调
```

### 批量操作

```python
from lightui import LightUIApp

with LightUIApp("Batch Demo", 800, 600, headless=True) as app:
    counter = app.state("counter", 0)
    
    def on_change(value):
        print(f"Notification: {value}")
    
    counter.watch(on_change)
    
    # 批量模式：多次修改只触发一次通知
    with app.batch():
        counter.increment()
        counter.increment()
        counter.increment()
    # 退出批量模式时触发一次通知
```

## 测试

```bash
# 运行单元测试
python bindings/python/tests/test_binding.py
python bindings/python/tests/test_window.py
python bindings/python/tests/test_host_bridge.py
python bindings/python/tests/test_app.py

# 运行属性测试
python -m pytest bindings/python/tests/test_property.py -v

# 运行示例
python bindings/python/examples/state_demo.py
python bindings/python/examples/counter.py
python bindings/python/examples/todo_app.py
```

## 类型提示

类型存根文件位于 `bindings/python/lightui_core.pyi`，支持 IDE 自动补全。

```python
from lightui import LightUIApp, IntState, StringState, ListState, DictState
```

## 架构

```
Python Application
        ↓
  lightui.pyd (C++ Extension)
    (App, State, IntState, ListState, DictState, StringState)
        ↓
  StateManager (C++)  ←→  Host Bridge (JS)
        ↓
    QuickJS Runtime
```

## 许可证

MIT License
