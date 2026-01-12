# Design Document: Python Binding

## Overview

本设计实现 LightUI 的 Python 绑定，使用 pybind11 直接编译成 `.pyd` 扩展模块。相比 ctypes 方案，pybind11 提供更好的性能、类型安全和 Python 集成。

架构分层：
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

设计参考：`#[[file:docs/binding_design.md]]`

## Architecture

### 模块结构

```
bindings/python/
├── src/
│   └── bindings.cpp     # pybind11 绑定代码
├── lightui/
│   ├── __init__.py      # 纯 Python 包装层（可选）
│   └── __init__.pyi     # 类型存根文件
├── CMakeLists.txt       # 构建配置
├── setup.py             # pip 安装脚本
└── tests/
    ├── test_app.py
    ├── test_state.py
    └── test_integration.py
```

### 数据流

```
┌─────────────────────────────────────────────────────────────┐
│                    Python Application                       │
│                                                             │
│   import lightui                                            │
│   app = lightui.App("My App", 800, 600)                    │
│   count = app.state("count", 0)  # -> IntState             │
│   users = app.state("users", []) # -> ListState            │
│                                                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                 lightui.pyd (pybind11)                      │
│                                                             │
│   直接调用 C++ StateManager                                 │
│   自动 Python ↔ C++ 类型转换                                │
│   GIL 管理和线程安全                                        │
│                                                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                 StateManager (C++)                          │
│                                                             │
│   Thread-safe operation queue                               │
│   Watcher notification system                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### pybind11 优势

1. **性能** - 直接编译成 C 扩展，无 FFI 开销
2. **类型安全** - 编译时类型检查
3. **自动转换** - Python list/dict ↔ nlohmann::json 自动转换
4. **异常处理** - C++ 异常自动转换为 Python 异常
5. **GIL 管理** - 自动处理全局解释器锁
6. **单文件分发** - 打包成单个 .pyd 文件


## Components and Interfaces

### pybind11 绑定 (bindings.cpp)

```cpp
// bindings/python/src/bindings.cpp

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "core/bridge/state_manager.h"
#include "core/window/window.h"

namespace py = pybind11;
using namespace lightui;

// 自定义异常
class LightUIError : public std::runtime_error {
public:
    int code;
    LightUIError(const std::string& msg, int code) 
        : std::runtime_error(msg), code(code) {}
};

// State 基类包装
class PyState {
protected:
    StateManager* sm_;
    std::string name_;
    
public:
    PyState(StateManager* sm, const std::string& name) : sm_(sm), name_(name) {}
    
    py::object get() {
        return jsonToPython(sm_->getJson(name_));
    }
    
    void set(py::object value) {
        sm_->setJson(name_, pythonToJson(value));
    }
    
    int watch(py::function callback) {
        return sm_->watch(name_, [callback](const std::string& name, const json& value) {
            py::gil_scoped_acquire acquire;
            callback(jsonToPython(value));
        });
    }
    
    void unwatch(int watchId) {
        sm_->unwatch(watchId);
    }
    
private:
    static py::object jsonToPython(const json& j);
    static json pythonToJson(py::object obj);
};

// IntState 特化
class PyIntState : public PyState {
public:
    using PyState::PyState;
    
    int64_t get() { return sm_->getInt(name_); }
    void set(int64_t value) { sm_->setInt(name_, value); }
    void increment(int64_t delta = 1) { sm_->increment(name_, delta); }
    void multiply(double factor) { sm_->multiply(name_, factor); }
};

// StringState 特化
class PyStringState : public PyState {
public:
    using PyState::PyState;
    
    std::string get() { return sm_->getString(name_); }
    void set(const std::string& value) { sm_->setString(name_, value); }
    void append(const std::string& suffix) { sm_->stringAppend(name_, suffix); }
    void prepend(const std::string& prefix) { sm_->stringPrepend(name_, prefix); }
    size_t len() { return sm_->getLength(name_); }
};

// ListState 特化
class PyListState : public PyState {
public:
    using PyState::PyState;
    
    void append(py::object item) { sm_->arrayPush(name_, pythonToJson(item)); }
    void pop() { sm_->arrayPop(name_); }
    void remove(int index) { sm_->arrayRemove(name_, index); }
    void clear() { sm_->arrayClear(name_); }
    size_t len() { return sm_->getLength(name_); }
    
    py::object getitem(int index) {
        return jsonToPython(sm_->getAt(name_, index));
    }
    
    void setitem(int index, py::object value) {
        sm_->arraySet(name_, index, pythonToJson(value));
    }
};

// DictState 特化
class PyDictState : public PyState {
public:
    using PyState::PyState;
    
    void set_key(const std::string& key, py::object value) {
        sm_->objectSet(name_, key, pythonToJson(value));
    }
    void remove_key(const std::string& key) { sm_->objectRemove(name_, key); }
    void clear() { sm_->objectClear(name_); }
    
    py::object getitem(const std::string& key) {
        return jsonToPython(sm_->getKey(name_, key));
    }
    
    bool contains(const std::string& key) {
        return !sm_->getKey(name_, key).is_null();
    }
    
    py::list keys() {
        auto obj = sm_->getJson(name_);
        py::list result;
        if (obj.is_object()) {
            for (auto& [key, _] : obj.items()) {
                result.append(key);
            }
        }
        return result;
    }
};

// App 类
class PyApp {
public:
    PyApp(const std::string& title, int width, int height);
    ~PyApp();
    
    py::object state(const std::string& name, py::object initial);
    void bind(const std::string& name, py::function func);
    void unbind(const std::string& name);
    void load_file(const std::string& path);
    void load_js(const std::string& code);
    void run();
    void stop();
    
    // 批量操作
    void batch_begin() { sm_->batchBegin(); }
    void batch_end() { sm_->batchEnd(); }
    
private:
    std::unique_ptr<Window> window_;
    StateManager* sm_;
    std::unordered_map<std::string, std::shared_ptr<PyState>> states_;
};

// pybind11 模块定义
PYBIND11_MODULE(lightui, m) {
    m.doc() = "LightUI Python bindings";
    
    // 异常
    py::register_exception<LightUIError>(m, "LightUIError");
    
    // State 基类
    py::class_<PyState>(m, "State")
        .def("get", &PyState::get)
        .def("set", &PyState::set)
        .def("watch", &PyState::watch)
        .def("unwatch", &PyState::unwatch);
    
    // IntState
    py::class_<PyIntState, PyState>(m, "IntState")
        .def("get", &PyIntState::get)
        .def("set", &PyIntState::set)
        .def("increment", &PyIntState::increment, py::arg("delta") = 1)
        .def("multiply", &PyIntState::multiply);
    
    // StringState
    py::class_<PyStringState, PyState>(m, "StringState")
        .def("get", &PyStringState::get)
        .def("set", &PyStringState::set)
        .def("append", &PyStringState::append)
        .def("prepend", &PyStringState::prepend)
        .def("__len__", &PyStringState::len);
    
    // ListState
    py::class_<PyListState, PyState>(m, "ListState")
        .def("append", &PyListState::append)
        .def("pop", &PyListState::pop)
        .def("remove", &PyListState::remove)
        .def("clear", &PyListState::clear)
        .def("__len__", &PyListState::len)
        .def("__getitem__", &PyListState::getitem)
        .def("__setitem__", &PyListState::setitem);
    
    // DictState
    py::class_<PyDictState, PyState>(m, "DictState")
        .def("set_key", &PyDictState::set_key)
        .def("remove_key", &PyDictState::remove_key)
        .def("clear", &PyDictState::clear)
        .def("keys", &PyDictState::keys)
        .def("__getitem__", &PyDictState::getitem)
        .def("__setitem__", &PyDictState::set_key)
        .def("__contains__", &PyDictState::contains);
    
    // App 类
    py::class_<PyApp>(m, "App")
        .def(py::init<const std::string&, int, int>(),
             py::arg("title") = "LightUI App",
             py::arg("width") = 800,
             py::arg("height") = 600)
        .def("state", &PyApp::state)
        .def("bind", &PyApp::bind)
        .def("unbind", &PyApp::unbind)
        .def("load_file", &PyApp::load_file)
        .def("load_js", &PyApp::load_js)
        .def("run", &PyApp::run)
        .def("stop", &PyApp::stop)
        .def("batch_begin", &PyApp::batch_begin)
        .def("batch_end", &PyApp::batch_end)
        .def("__enter__", [](PyApp& self) -> PyApp& { return self; })
        .def("__exit__", [](PyApp& self, py::object, py::object, py::object) {});
    
    // 批量操作上下文管理器
    py::class_<BatchContext>(m, "BatchContext")
        .def("__enter__", &BatchContext::enter)
        .def("__exit__", &BatchContext::exit);
    
    m.def("version", []() { return "0.1.0"; });
}
```

### Python 使用示例

```python
import lightui

# 创建应用
app = lightui.App(title="Task Manager", width=800, height=600)

# 状态管理 - 自动类型推断
tasks = app.state("tasks", [])      # -> ListState
progress = app.state("progress", 0)  # -> IntState
status = app.state("status", "idle") # -> StringState

# 读写操作
print(tasks.get())      # []
progress.increment(10)  # 原子操作
status.append("...")    # 字符串追加

# 监听变化
@tasks.watch
def on_tasks_change(new_value):
    print(f"Tasks updated: {len(new_value)}")

# 函数绑定
@app.bind
def get_data():
    return {"items": ["a", "b", "c"]}

# 批量操作
with app.batch():
    tasks.set([{"name": "Task 1"}])
    progress.set(0)

# 运行
app.load_file("ui/app.js")
app.run()
```

### CMakeLists.txt

```cmake
# bindings/python/CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(lightui_python)

# 查找 Python 和 pybind11
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)

# 创建 Python 模块
pybind11_add_module(lightui
    src/bindings.cpp
)

# 链接 LightUI 核心库
target_link_libraries(lightui PRIVATE
    lightui_core
    lightui_bridge
)

# 设置输出目录
set_target_properties(lightui PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
)

# 安装
install(TARGETS lightui
    LIBRARY DESTINATION lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages
)
```

### JSON ↔ Python 转换

```cpp
// 辅助函数：json -> Python object
py::object jsonToPython(const json& j) {
    if (j.is_null()) return py::none();
    if (j.is_boolean()) return py::bool_(j.get<bool>());
    if (j.is_number_integer()) return py::int_(j.get<int64_t>());
    if (j.is_number_float()) return py::float_(j.get<double>());
    if (j.is_string()) return py::str(j.get<std::string>());
    if (j.is_array()) {
        py::list result;
        for (const auto& item : j) {
            result.append(jsonToPython(item));
        }
        return result;
    }
    if (j.is_object()) {
        py::dict result;
        for (auto& [key, value] : j.items()) {
            result[py::str(key)] = jsonToPython(value);
        }
        return result;
    }
    return py::none();
}

// 辅助函数：Python object -> json
json pythonToJson(py::object obj) {
    if (obj.is_none()) return nullptr;
    if (py::isinstance<py::bool_>(obj)) return obj.cast<bool>();
    if (py::isinstance<py::int_>(obj)) return obj.cast<int64_t>();
    if (py::isinstance<py::float_>(obj)) return obj.cast<double>();
    if (py::isinstance<py::str>(obj)) return obj.cast<std::string>();
    if (py::isinstance<py::list>(obj)) {
        json arr = json::array();
        for (auto item : obj) {
            arr.push_back(pythonToJson(py::reinterpret_borrow<py::object>(item)));
        }
        return arr;
    }
    if (py::isinstance<py::dict>(obj)) {
        json dict = json::object();
        for (auto item : obj.cast<py::dict>()) {
            dict[item.first.cast<std::string>()] = 
                pythonToJson(py::reinterpret_borrow<py::object>(item.second));
        }
        return dict;
    }
    return nullptr;
}
```


## Data Models

### LightUIType (Python 映射)

| C++ 类型 | Python 类型 | State 子类 |
|---------|------------|-----------|
| nullptr | None | State |
| bool | bool | State |
| int64_t | int | IntState |
| double | float | State |
| std::string | str | StringState |
| json::array | list | ListState |
| json::object | dict | DictState |

### 回调签名

```python
# 函数绑定回调
HostCallback = Callable[[Any], Any]

# 状态变化回调
StateCallback = Callable[[T], None]
```

### GIL 管理

```cpp
// 在 C++ 回调中获取 GIL
void notifyPythonCallback(py::function callback, const json& value) {
    py::gil_scoped_acquire acquire;  // 获取 GIL
    callback(jsonToPython(value));
}

// 在长时间操作中释放 GIL
void PyApp::run() {
    py::gil_scoped_release release;  // 释放 GIL
    window_->run();  // 事件循环
}
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Type Inference Correctness

*For any* Python value of type T (int/float/str/list/dict/bool/None), calling app.state(name, value) SHALL return the corresponding State subclass (IntState/State[float]/StringState/ListState/DictState/State[bool]/State[None]).

**Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8**

### Property 2: State Round-Trip Consistency

*For any* state value V, calling state.set(V) followed by state.get() after processQueue SHALL return a value equivalent to V.

**Validates: Requirements 4.1, 4.2**

### Property 3: State Proxy Identity

*For any* state name N, calling app.state(N, initial) multiple times SHALL return the same State proxy object.

**Validates: Requirements 3.9**

### Property 4: Thread-Safe Operations

*For any* sequence of concurrent state operations from multiple threads, all operations SHALL complete without data corruption or race conditions.

**Validates: Requirements 4.6, 5.4**

### Property 5: IntState Atomic Operations

*For any* IntState with initial value V, increment(delta) SHALL result in V + delta, and multiply(factor) SHALL result in V * factor.

**Validates: Requirements 5.1, 5.2**

### Property 6: ListState Operations Correctness

*For any* ListState and sequence of list operations (append/pop/insert/remove/clear), the resulting list SHALL match the expected state after applying operations in order.

**Validates: Requirements 6.1, 6.2, 6.3, 6.4, 6.5, 6.6, 6.7, 6.8**

### Property 7: DictState Operations Correctness

*For any* DictState and sequence of dict operations (set_key/remove_key/clear), the resulting dict SHALL match the expected state after applying operations in order.

**Validates: Requirements 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7**

### Property 8: StringState Operations Correctness

*For any* StringState with initial value S, append(suffix) SHALL result in S + suffix, and prepend(prefix) SHALL result in prefix + S.

**Validates: Requirements 8.1, 8.2, 8.3, 8.4**

### Property 9: Batch Mode Deferred Notification

*For any* sequence of state changes within app.batch() context, watchers SHALL NOT be invoked until the context exits, and then all changed states SHALL trigger their watchers exactly once.

**Validates: Requirements 10.2, 10.3, 10.4**

### Property 10: Function Binding JSON Round-Trip

*For any* Python value returned by a bound function, the JSON serialization and deserialization SHALL preserve the value's structure and content.

**Validates: Requirements 9.5**


## Error Handling

### 错误码映射

| C 错误码 | Python 异常 | 说明 |
|---------|------------|------|
| LIGHTUI_OK | (无异常) | 成功 |
| LIGHTUI_ERROR_INVALID_HANDLE | LightUIError | 无效句柄 |
| LIGHTUI_ERROR_INVALID_PARAM | ValueError | 无效参数 |
| LIGHTUI_ERROR_NOT_FOUND | StateNotFoundError | 状态不存在 |
| LIGHTUI_ERROR_TYPE_MISMATCH | StateTypeError | 类型不匹配 |
| LIGHTUI_ERROR_OUT_OF_RANGE | IndexError | 索引越界 |
| LIGHTUI_ERROR_JS_ERROR | LightUIError | JS 执行错误 |

### 错误处理策略

```python
def _check_error(result: int, operation: str):
    """检查 C API 返回值并抛出对应异常"""
    if result == 0:
        return
    
    error_map = {
        -1: (LightUIError, "Invalid handle"),
        -2: (ValueError, "Invalid parameter"),
        -3: (StateNotFoundError, "State not found"),
        -4: (StateTypeError, "Type mismatch"),
        -5: (IndexError, "Index out of range"),
        -6: (LightUIError, "JavaScript error"),
    }
    
    exc_class, message = error_map.get(result, (LightUIError, "Unknown error"))
    raise exc_class(f"{operation}: {message}", code=result)
```

## Testing Strategy

### 单元测试

使用 pytest 进行单元测试：

```python
# tests/test_state.py
import pytest
from lightui import App, IntState, ListState, DictState, StringState

class TestStateTypeInference:
    def test_int_returns_intstate(self):
        with App() as app:
            state = app.state("count", 0)
            assert isinstance(state, IntState)
    
    def test_list_returns_liststate(self):
        with App() as app:
            state = app.state("items", [])
            assert isinstance(state, ListState)
    
    # ... 更多测试

class TestIntState:
    def test_increment(self):
        with App() as app:
            count = app.state("count", 10)
            count.increment(5)
            app._lib.lightui_process_queue(app._handle)
            assert count.get() == 15
    
    def test_multiply(self):
        with App() as app:
            count = app.state("count", 10)
            count.multiply(2)
            app._lib.lightui_process_queue(app._handle)
            assert count.get() == 20
```

### 属性测试

使用 hypothesis 进行属性测试：

```python
# tests/test_properties.py
from hypothesis import given, strategies as st
from lightui import App

@given(st.integers())
def test_int_round_trip(value):
    """Property 2: State Round-Trip Consistency for integers"""
    with App() as app:
        state = app.state("test", 0)
        state.set(value)
        app._lib.lightui_process_queue(app._handle)
        assert state.get() == value

@given(st.lists(st.integers()))
def test_list_round_trip(value):
    """Property 2: State Round-Trip Consistency for lists"""
    with App() as app:
        state = app.state("test", [])
        state.set(value)
        app._lib.lightui_process_queue(app._handle)
        assert state.get() == value

@given(st.integers(), st.integers())
def test_increment_correctness(initial, delta):
    """Property 5: IntState Atomic Operations"""
    with App() as app:
        state = app.state("count", initial)
        state.increment(delta)
        app._lib.lightui_process_queue(app._handle)
        assert state.get() == initial + delta
```

### 集成测试

测试 Python ↔ C++ ↔ JS 三向同步：

```python
# tests/test_integration.py
def test_python_to_js_state_sync():
    """测试 Python 设置状态后 JS 能读取"""
    with App() as app:
        count = app.state("count", 0)
        count.set(42)
        
        # 加载 JS 代码验证
        app.load_js("""
            const value = host.state.get('count');
            if (value !== 42) throw new Error('State not synced');
        """)

def test_js_to_python_state_sync():
    """测试 JS 设置状态后 Python 能读取"""
    with App() as app:
        count = app.state("count", 0)
        
        app.load_js("""
            host.state.set('count', 100);
        """)
        
        app._lib.lightui_process_queue(app._handle)
        assert count.get() == 100
```

### 测试配置

- 属性测试：每个属性至少 100 次迭代
- 使用 hypothesis 作为 Python 属性测试框架
- 测试文件位置：`bindings/python/tests/`
- 运行命令：`pytest bindings/python/tests/ -v`

