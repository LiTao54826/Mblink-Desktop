# LightUI 跨语言绑定完整重设计方案 v2

> 📅 生成日期：2025-01
> 🎯 目标：推翻现有 pybind11 方案，建立基于 C ABI + ctypes 的全新绑定架构

---

## 1. 现状诊断

### 1.1 当前架构

```
Python app.py (1308行)
    → pybind11 bindings.cpp (1400行)
        → C++ 内部类 (Window/Runtime/EventLoop/HostBridge/StateManager)
            → LightUI Core
```

### 1.2 核心问题

| # | 问题 | 严重程度 | 影响 |
|---|------|----------|------|
| 1 | **C API 是空壳** | 🔴 致命 | `lightui_run()` / `lightui_load_js()` 全是 TODO stub，C API 形同虚设 |
| 2 | **pybind11 硬绑定** | 🔴 致命 | 每个 Python 版本需重编译 .pyd；Go/Rust 完全无法复用 1400 行绑定代码 |
| 3 | **AST 黑魔法** | 🟡 严重 | `_inject_global_for_states()` 使用 `inspect.getsource()` + AST + `compile()` + `exec()`，REPL/eval 中崩溃 |
| 4 | **4 层 State 包装** | 🟡 严重 | `_IntStateWrapper` / `_StringStateWrapper` / `_ListStateWrapper` / `_DictStateWrapper` 各 ~100 行，几乎相同 |
| 5 | **5+ 序列化层** | 🟠 中等 | Python obj → pythonToJson → nlohmann::json → StateManager → JSON string → HostBridge → QuickJS |
| 6 | **GIL 手动管理** | 🟠 中等 | bindings.cpp 中 ~30 处 `py::gil_scoped_acquire/release`，极易出错 |

### 1.3 数据流对比

```
❌ 现在（5+ 序列化层）:
Python dict → pythonToJson() → nlohmann::json → StateManager.setJson()
→ json.dump() → HostBridge → jsValueToJson() → QuickJS

✅ 目标（2 层）:
Python dict → json.dumps() → lightui_state_set_json() [C ABI]
→ StateManager → HostBridge → QuickJS
```

---

## 2. 新架构设计

### 2.1 架构总览

```
┌─────────────────────────────────────────────┐
│              用户代码（各语言）                 │
├──────────┬──────────┬──────────┬─────────────┤
│ Python   │ Go       │ Rust     │ Node.js     │
│ ctypes   │ cgo      │ FFI      │ node-ffi    │
│ ~200行    │ ~100行   │ ~100行   │ ~100行       │
├──────────┴──────────┴──────────┴─────────────┤
│         lightui.h  C ABI（唯一入口）           │
│           lightui.dll / liblightui.so         │
├──────────────────────────────────────────────┤
│         LightUI C++ Core 实现                 │
│  Window│Document│Runtime│EventLoop│HostBridge │
│  StateManager│Skia│SDL3│QuickJS│DevTools      │
└──────────────────────────────────────────────┘
```

### 2.2 核心设计原则

1. **C ABI 是唯一入口** — 所有语言绑定都通过 `lightui.h` 定义的 C 函数
2. **DLL/SO 共享库** — 编译一次，所有 Python 版本通用，所有语言通用
3. **零编译安装** — Python 用户只需 `pip install lightui`，无需 C++ 编译器
4. **无 AST 魔法** — State 通过 `.value` 属性读写，清晰直观
5. **HTML/CSS/JS 是 UI 层** — LightUI 的核心优势，继续发挥
6. **统一 State 类** — 一个 `State` 类替代四个 `_XxxStateWrapper`

### 2.3 组件初始化流程

```
lightui_init()
    └→ SDL_Init()

lightui_create("Title", 800, 600)
    ├→ WindowConfig → Window (SDL3 + Skia + OpenGL)
    ├→ Document() → Window.SetDocument()
    ├→ TaskScheduler()
    ├→ WindowManager.RegisterWindow()
    ├→ QuickJSRuntime()
    ├→ DOMBindings::Init() + SetGlobalDocument()
    ├→ WindowBindings(runtime, window, taskScheduler).InitBindings()
    ├→ StateManager()
    ├→ HostBridge(ctx, stateManager).registerGlobal()
    ├→ FetchBindings(runtime)
    └→ EventLoop(taskScheduler) + SetQuickJSRuntime() + SetStateManager() + SetHostBridge()

lightui_load_html(handle, "<html>...")
    └→ Document.LoadHTML()

lightui_bind(handle, "increment", callback, user_data)
    └→ HostBridge.bind("increment", ...) → JS 端 py.increment() 可用

lightui_run(handle)
    └→ EventLoop.Run() → [主循环：事件处理 → 任务调度 → QuickJS → 状态更新 → 渲染]

lightui_destroy(handle)
    └→ 按序清理所有组件
```

---

## 3. C API v2 设计

### 3.1 新增类型定义

```c
// 窗口配置
typedef struct {
    const char* title;
    int width;
    int height;
    bool headless;
    bool borderless;
    bool transparent;
    bool always_on_top;
    bool resizable;
    bool gpu;
    bool fullscreen;
    int resize_border_width;
    int min_width, min_height;
    int max_width, max_height;
} LightUIConfig;

// 事件回调类型
typedef void (*LightUIEventCallback)(const char* event_name, const char* data_json, void* user_data);
typedef void (*LightUIResizeCallback)(int width, int height, void* user_data);
typedef void (*LightUIVoidCallback)(void* user_data);
typedef void (*LightUIUpdateCallback)(float delta_time, void* user_data);

// ========== 窗口属性 ==========
int lightui_get_position(LightUIHandle h, int* x, int* y);
int lightui_set_position(LightUIHandle h, int x, int y);
int lightui_get_size(LightUIHandle h, int* width, int* height);
int lightui_set_min_size(LightUIHandle h, int width, int height);
int lightui_set_max_size(LightUIHandle h, int width, int height);
int lightui_minimize(LightUIHandle h);
int lightui_maximize(LightUIHandle h);
int lightui_restore(LightUIHandle h);
int lightui_set_fullscreen(LightUIHandle h, bool fullscreen);
int lightui_set_resizable(LightUIHandle h, bool resizable);
int lightui_set_borderless(LightUIHandle h, bool borderless);
int lightui_set_always_on_top(LightUIHandle h, bool on_top);
int lightui_show(LightUIHandle h);
int lightui_hide(LightUIHandle h);

// ========== 事件回调 ==========
int lightui_on_resize(LightUIHandle h, LightUIResizeCallback cb, void* ud);
int lightui_on_close(LightUIHandle h, LightUIVoidCallback cb, void* ud);
int lightui_on_focus(LightUIHandle h, LightUIVoidCallback cb, void* ud);
int lightui_on_blur(LightUIHandle h, LightUIVoidCallback cb, void* ud);
int lightui_on_update(LightUIHandle h, LightUIUpdateCallback cb, void* ud);

// ========== 事件发送 ==========
int lightui_emit(LightUIHandle h, const char* event_name, const char* data_json);

// ========== DevTools ==========
int lightui_devtools_open(LightUIHandle h);
int lightui_devtools_close(LightUIHandle h);
```

### 3.3 保留的已有 API

以下 API 已在 `lightui.h` v1 中定义并实现，保持不变：

- 生命周期：`lightui_init()`, `lightui_cleanup()`, `lightui_version()`
- 窗口基础：`lightui_create()`, `lightui_destroy()`, `lightui_run()`, `lightui_stop()`
- 状态 CRUD：全部 `lightui_state_*` 函数（已完整实现）
- 函数绑定：`lightui_bind()`, `lightui_unbind()`
- 监听/批量/队列：`lightui_state_watch()`, `lightui_state_batch_*()`, `lightui_process_queue()`
- 工具：`lightui_free()`, `lightui_last_error()`

---

## 4. Python API v2 设计

### 4.1 用户视角

```python
import lightui

# 一行创建应用（内部完成全部初始化）
app = lightui.App("My App", 800, 600)

# 状态管理（清晰的 .value 属性，无 AST 魔法）
counter = app.state("counter", 0)
name = app.state("name", "World")
items = app.state("items", ["apple", "banana"])

# 绑定函数（简单 @app.bind 装饰器）
@app.bind
def increment():
    counter.value += 1
    return counter.value

@app.bind
def add_item(item_name):
    items.append(item_name)  # 直接方法调用

# 加载 HTML UI
app.load_html('''
<div style="display: flex; flex-direction: column; align-items: center; padding: 32px;">
    <h1 style="font-size: 48px; color: #333;">
        Count: <span data-bind="counter">0</span>
    </h1>
    <div style="display: flex; gap: 8px;">
        <button onclick="py.increment()"
                style="padding: 8px 24px; font-size: 18px; border-radius: 4px;">
            +1
        </button>
    </div>
</div>
''')

# 运行（阻塞直到窗口关闭）
app.run()
```

### 4.2 文件结构

```
bindings/python/lightui/
├── __init__.py      # 导出 App, State, version
├── app.py           # App 高级封装（~300行，无 AST 魔法）
├── _state.py        # State 统一类（~100行，替代4个 Wrapper）
├── _ffi.py          # ctypes 声明（~200行）
└── bin/
    └── lightui.dll  # 编译产物（用户不需要编译）
```

### 4.3 State 类设计

```python
class State:
    """统一的状态封装 — 通过 .value 属性读写"""

    def __init__(self, handle, name):
        self._handle = handle
        self._name = name.encode('utf-8')

    @property
    def value(self):
        """读取状态值（自动类型转换）"""
        t = _lib.lightui_state_type(self._handle, self._name)
        if t == LIGHTUI_TYPE_INT:
            return _lib.lightui_state_get_int(self._handle, self._name)
        elif t == LIGHTUI_TYPE_DOUBLE:
            return _lib.lightui_state_get_double(self._handle, self._name)
        elif t == LIGHTUI_TYPE_STRING:
            return _ffi_get_string(self._handle, self._name)
        elif t == LIGHTUI_TYPE_BOOL:
            return _lib.lightui_state_get_bool(self._handle, self._name)
        elif t in (LIGHTUI_TYPE_ARRAY, LIGHTUI_TYPE_OBJECT):
            json_ptr = _lib.lightui_state_get_json(self._handle, self._name)
            result = json.loads(ctypes.string_at(json_ptr).decode())
            _lib.lightui_free(json_ptr)
            return result
        return None

    @value.setter
    def value(self, new_val):
        """写入状态值（自动类型推导）"""
        if isinstance(new_val, bool):
            _lib.lightui_state_set_bool(self._handle, self._name, new_val)
        elif isinstance(new_val, int):
            _lib.lightui_state_set_int(self._handle, self._name, new_val)
        elif isinstance(new_val, float):
            _lib.lightui_state_set_double(self._handle, self._name, new_val)
        elif isinstance(new_val, str):
            _lib.lightui_state_set_string(self._handle, self._name, new_val.encode())
        elif isinstance(new_val, (list, dict)):
            _lib.lightui_state_set_json(self._handle, self._name,
                                         json.dumps(new_val).encode())
        elif new_val is None:
            _lib.lightui_state_set_null(self._handle, self._name)

    # 便捷方法
    def increment(self, delta=1):
        _lib.lightui_state_increment(self._handle, self._name, float(delta))

    def append(self, item):
        _lib.lightui_state_array_push(self._handle, self._name,
                                       json.dumps(item).encode())

    def watch(self, callback):
        """监听状态变化"""
        @CFUNCTYPE(None, c_char_p, c_char_p, c_void_p)
        def _cb(name, value_json, ud):
            callback(json.loads(value_json.decode()))
        self._watch_cbs.append(_cb)  # 防止 GC 回收
        return _lib.lightui_state_watch(self._handle, self._name, _cb, None)
```

### 4.4 App 类设计

```python
class App:
    def __init__(self, title="LightUI", width=800, height=600, **kwargs):
        _lib.lightui_init()

        if kwargs:
            config = _lib.lightui_default_config()
            config.title = title.encode()
            config.width = width
            config.height = height
            # ... 设置其他 kwargs
            self._handle = _lib.lightui_create_ex(byref(config))
        else:
            self._handle = _lib.lightui_create(title.encode(), width, height)

        self._states = {}
        self._bound_funcs = {}

    def state(self, name, initial=None):
        """创建或获取状态"""
        if name in self._states:
            return self._states[name]

        # 根据类型调用对应的 C API 创建
        _create_state(self._handle, name, initial)

        s = State(self._handle, name)
        self._states[name] = s
        return s

    def bind(self, func):
        """绑定 Python 函数供 JS 调用（装饰器）"""
        name = func.__name__

        @CFUNCTYPE(c_char_p, c_char_p, c_void_p)
        def _callback(args_json, user_data):
            args = json.loads(args_json.decode()) if args_json else None
            if isinstance(args, list):
                result = func(*args)
            elif isinstance(args, dict):
                result = func(**args)
            elif args is not None:
                result = func(args)
            else:
                result = func()
            return json.dumps(result).encode() if result is not None else b"null"

        self._bound_funcs[name] = _callback  # 防止 GC
        _lib.lightui_bind(self._handle, name.encode(), _callback, None)
        return func

    def load_html(self, html):
        _lib.lightui_load_html(self._handle, html.encode())

    def load_file(self, filepath):
        _lib.lightui_load_html_file(self._handle, filepath.encode())

    def eval_js(self, code):
        _lib.lightui_eval_js(self._handle, code.encode())

    def run(self):
        _lib.lightui_run(self._handle)

    def __del__(self):
        if hasattr(self, '_handle') and self._handle:
            _lib.lightui_destroy(self._handle)
```

---

## 5. 对比总结

| 维度 | 旧方案 (pybind11) | 新方案 (C ABI + ctypes) |
|------|-------------------|------------------------|
| **绑定层** | bindings.cpp 1400行 | lightui.h ~100个函数声明 |
| **Python 封装** | app.py 1308行 + AST 魔法 | app.py ~300行，清晰直观 |
| **State 类** | 4 个 Wrapper 各 ~100 行 | 1 个 State 类 ~80 行 |
| **编译依赖** | 每个 Python 版本需重编译 | 编译一次 DLL，所有版本通用 |
| **Go/Rust 支持** | ❌ 无法复用 | ✅ 同一个 DLL |
| **序列化层数** | 5+ 层 | 2 层 |
| **GIL 管理** | 手动 ~30 处 | ctypes 自动处理 |
| **安装方式** | 需要 C++ 编译器 | pip install（预编译 wheel） |
| **调试体验** | C++ 异常 → Python | Python 原生错误栈 |

---

## 6. 实施计划

```
Phase 1: 设计文档 ← 你正在看的这个
Phase 2: 扩展 lightui.h → 新增 ~30 个函数声明
Phase 3: 重写 lightui.cpp → 完整实现（~800行）
Phase 4: CMakeLists → SHARED 库
Phase 5: Python ctypes 绑定 → _ffi.py + _state.py + app.py
Phase 6: 编译 + 运行验证
```

### 6.1 风险和注意事项

1. **事件循环线程模型** — `lightui_run()` 阻塞主线程，Python 回调在主线程执行（无需 GIL 问题）
2. **回调生命周期** — Python `ctypes.CFUNCTYPE` 回调必须保持引用防止 GC
3. **字符串内存** — C API 返回的 `char*` 由调用者通过 `lightui_free()` 释放
4. **向后兼容** — 旧 pybind11 代码保留在 `bindings/python/src/` 作为参考
5. **PyInstaller** — DLL 打包只需在 spec 文件中添加 `lightui.dll` 即可