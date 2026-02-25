# 共享 C 对象方案 — 设计文档 v1.0

> 彻底替代 State/watch 架构，Python/JS 共享同一个 QuickJS C 对象

## 1. 核心思想

```
┌─────────────┐       ┌──────────────────┐       ┌─────────────┐
│  Python 侧   │       │   C / DLL 层      │       │   JS 侧     │
│             │       │                  │       │             │
│ SharedState │──────>│  SharedObject {  │<──────│ globalThis  │
│ __setattr__ │ ctypes│    JSContext*    │  JS   │   .data     │
│ __getattr__ │──────>│    JSValue obj   │<──────│   .count    │
│             │       │  }               │       │   .name     │
└─────────────┘       └──────────────────┘       └─────────────┘
      代理类               C 对象 = 唯一真相源          就是普通变量
```

**关键洞察**：QuickJS 的 `JSValue` 就是 C 结构体。
- JS 变量 = C 对象的 JS 视图
- Python 通过 C API 操作 = 操作同一个 C 对象
- 三方共享同一块内存，零拷贝、零序列化

## 2. 与旧架构对比

| 维度 | State/watch (旧) | eval_js 直接 | **共享 C 对象 (新)** |
|------|-----------------|-------------|---------------------|
| 数据存储 | StateManager C++ map | Python 变量 | **JSValue C 对象** |
| Python→JS | 6层: set→enqueue→apply→notify→watch→DOM | eval_js 字符串拼接 | **C API 直接设属性** |
| JS→Python | host.state.get/set | host.call | **直接读同一个对象** |
| DOM 更新 | watcher 链条 (broken) | eval_js (working) | **C 层自动触发 JS 回调** |
| 状态 | ❌ 已证实不工作 | ✅ 工作 | 🔵 待实现 |
| 代码复杂度 | StateManager ~600行 | 0 | ~200行 C + 80行 Python |

## 3. C API 新增函数

### 3.1 共享对象生命周期

```c
/// 创建共享对象并注册为 JS 全局变量 `globalThis.{name}`
/// @return 不透明指针，失败返回 NULL
LIGHTUI_API void* lightui_shared_create(LightUIHandle handle, const char* name);

/// 销毁共享对象，释放 JSValue 引用
LIGHTUI_API void lightui_shared_destroy(void* shared);
```

### 3.2 类型化写入（Python → C 对象 → JS 可见）

```c
LIGHTUI_API int lightui_shared_set_int(void* shared, const char* key, int64_t value);
LIGHTUI_API int lightui_shared_set_double(void* shared, const char* key, double value);
LIGHTUI_API int lightui_shared_set_string(void* shared, const char* key, const char* value);
LIGHTUI_API int lightui_shared_set_bool(void* shared, const char* key, bool value);
LIGHTUI_API int lightui_shared_set_null(void* shared, const char* key);
LIGHTUI_API int lightui_shared_set_json(void* shared, const char* key, const char* json);
```

每个 set 函数内部：
1. `JS_SetPropertyStr(ctx, obj, key, JS_NewXxx(ctx, value))` — 直接改 JSValue
2. 如果有注册更新器 → 调用 `JS_Call(ctx, updater_func, ...)` 通知 JS 端

### 3.3 类型化读取（Python ← C 对象 ← 与 JS 同一份数据）

```c
LIGHTUI_API int64_t     lightui_shared_get_int(void* shared, const char* key);
LIGHTUI_API double      lightui_shared_get_double(void* shared, const char* key);
LIGHTUI_API const char* lightui_shared_get_string(void* shared, const char* key);
LIGHTUI_API bool        lightui_shared_get_bool(void* shared, const char* key);
LIGHTUI_API int         lightui_shared_get_type(void* shared, const char* key);  // LightUIType
```

### 3.4 通用操作

```c
/// 以 JSON 获取任意属性
LIGHTUI_API const char* lightui_shared_get_json(void* shared, const char* key);

/// 获取所有 key（返回 JSON 数组字符串）
LIGHTUI_API const char* lightui_shared_keys(void* shared);

/// 删除属性
LIGHTUI_API int lightui_shared_delete(void* shared, const char* key);

/// 注册 JS 更新回调函数名（当 Python 写入时自动调用）
LIGHTUI_API int lightui_shared_on_update(void* shared, const char* js_function_name);
```

## 4. C++ 实现核心

### 4.1 SharedObject 结构体

```cpp
struct SharedObject {
    WindowContext* wctx;          // 所属窗口上下文
    JSContext* ctx;               // QuickJS 上下文（快捷引用）
    JSValue js_obj;               // 核心：JS 对象本体 (refcount managed)
    std::string global_name;      // JS 全局变量名
    JSValue updater_func;         // 可选的 JS 更新回调 (JS_UNDEFINED if none)

    // 返回字符串的缓冲区（避免悬空指针）
    std::string str_buffer;
    std::string json_buffer;
    std::string keys_buffer;
};
```



### 4.2 关键实现

```cpp
void* lightui_shared_create(LightUIHandle handle, const char* name) {
    auto wctx = getContext(handle);
    if (!wctx || !wctx->runtime) return nullptr;

    auto* shared = new SharedObject();
    shared->wctx = wctx;
    shared->ctx = wctx->runtime->GetContext();
    shared->global_name = name;
    shared->updater_func = JS_UNDEFINED;

    // 创建 JS 对象
    shared->js_obj = JS_NewObject(shared->ctx);

    // 注册为 JS 全局变量: globalThis.{name} = obj
    JSValue global = JS_GetGlobalObject(shared->ctx);
    JS_SetPropertyStr(shared->ctx, global, name,
                      JS_DupValue(shared->ctx, shared->js_obj));
    JS_FreeValue(shared->ctx, global);

    return shared;
}

int lightui_shared_set_int(void* ptr, const char* key, int64_t value) {
    auto* s = static_cast<SharedObject*>(ptr);
    if (!s || !s->ctx) return LIGHTUI_ERROR_INVALID_HANDLE;

    JS_SetPropertyStr(s->ctx, s->js_obj, key,
                      JS_NewInt64(s->ctx, value));

    // 触发 JS 更新通知
    if (!JS_IsUndefined(s->updater_func)) {
        JSValue args[2] = {
            JS_NewString(s->ctx, key),
            JS_NewInt64(s->ctx, value)
        };
        JSValue ret = JS_Call(s->ctx, s->updater_func,
                              JS_UNDEFINED, 2, args);
        JS_FreeValue(s->ctx, ret);
        JS_FreeValue(s->ctx, args[0]);
        JS_FreeValue(s->ctx, args[1]);
    }
    return LIGHTUI_OK;
}

int64_t lightui_shared_get_int(void* ptr, const char* key) {
    auto* s = static_cast<SharedObject*>(ptr);
    if (!s) return 0;
    JSValue val = JS_GetPropertyStr(s->ctx, s->js_obj, key);
    int64_t result = 0;
    JS_ToInt64(s->ctx, &result, val);
    JS_FreeValue(s->ctx, val);
    return result;  // JS 改过的值 Python 也能读到！
}
```

### 4.3 DOM 自动更新机制

Python 写入 → C 设置 JSValue → C 调用 JS 更新回调 → DOM 更新

```cpp
int lightui_shared_on_update(void* ptr, const char* js_function_name) {
    auto* s = static_cast<SharedObject*>(ptr);
    if (!s) return LIGHTUI_ERROR_INVALID_HANDLE;

    // 获取 JS 全局函数引用
    JSValue global = JS_GetGlobalObject(s->ctx);
    JSValue func = JS_GetPropertyStr(s->ctx, global, js_function_name);
    JS_FreeValue(s->ctx, global);

    if (!JS_IsFunction(s->ctx, func)) {
        JS_FreeValue(s->ctx, func);
        return LIGHTUI_ERROR_NOT_FOUND;
    }

    // 释放旧的更新器
    if (!JS_IsUndefined(s->updater_func)) {
        JS_FreeValue(s->ctx, s->updater_func);
    }
    s->updater_func = func;  // 持有引用
    return LIGHTUI_OK;
}
```

## 5. Python 层设计

### 5.1 ctypes 绑定 (_ffi.py 追加)

```python
# 共享对象 API
lib.lightui_shared_create.argtypes = [c_void_p, c_char_p]
lib.lightui_shared_create.restype = c_void_p

lib.lightui_shared_set_int.argtypes = [c_void_p, c_char_p, c_int64]
lib.lightui_shared_set_int.restype = c_int

lib.lightui_shared_get_int.argtypes = [c_void_p, c_char_p]
lib.lightui_shared_get_int.restype = c_int64
# ... 其他类型同理
```

### 5.2 SharedState 代理类

```python
class SharedState:
    """Python 代理类，操作底层 C/JS 共享对象"""

    def __init__(self, app, name="shared"):
        object.__setattr__(self, '_app', app)
        object.__setattr__(self, '_ptr',
            app._lib.lightui_shared_create(app._handle, name.encode()))
        object.__setattr__(self, '_types', {})

    def __setattr__(self, key, value):
        ptr = object.__getattribute__(self, '_ptr')
        lib = object.__getattribute__(self, '_app')._lib
        types = object.__getattribute__(self, '_types')
        k = key.encode()
        if isinstance(value, bool):
            lib.lightui_shared_set_bool(ptr, k, value)
            types[key] = 'bool'
        elif isinstance(value, int):
            lib.lightui_shared_set_int(ptr, k, value)
            types[key] = 'int'
        elif isinstance(value, float):
            lib.lightui_shared_set_double(ptr, k, value)
            types[key] = 'double'
        elif isinstance(value, str):
            lib.lightui_shared_set_string(ptr, k, value.encode())
            types[key] = 'string'
        elif value is None:
            lib.lightui_shared_set_null(ptr, k)
            types[key] = 'null'
        else:
            import json as _json
            lib.lightui_shared_set_json(ptr, k, _json.dumps(value).encode())
            types[key] = 'json'

    def __getattr__(self, key):
        ptr = object.__getattribute__(self, '_ptr')
        lib = object.__getattribute__(self, '_app')._lib
        types = object.__getattribute__(self, '_types')
        k = key.encode()
        t = types.get(key)
        if t == 'int':    return lib.lightui_shared_get_int(ptr, k)
        elif t == 'double': return lib.lightui_shared_get_double(ptr, k)
        elif t == 'string':
            raw = lib.lightui_shared_get_string(ptr, k)
            return raw.decode() if raw else ""
        elif t == 'bool': return lib.lightui_shared_get_bool(ptr, k)
        else:
            raw = lib.lightui_shared_get_json(ptr, k)
            import json as _json
            return _json.loads(raw.decode()) if raw else None
```

## 6. 使用示例

### 6.1 Counter — 共享 C 对象版

```python
from lightui import App

app = App("Counter", 400, 300)
data = app.shared("data")   # 创建共享 C 对象 → JS globalThis.data
data.count = 0

@app.bind("increment")
def _(args):
    data.count += 1          # 读 + 写同一个 C 对象

@app.bind("decrement")
def _(args):
    data.count -= 1

app.load_html("""
<div id="count">0</div>
<button onclick="host.call('increment')">+1</button>
<button onclick="host.call('decrement')">-1</button>
<script>
function __onDataUpdate(key, value) {
    if (key === 'count')
        document.getElementById('count').textContent = value;
}
</script>
""")
app.run()
```

### 6.2 数据流

```
Python: data.count = 5
  → SharedState.__setattr__("count", 5)
  → ctypes: lightui_shared_set_int(ptr, "count", 5)
  → C++: JS_SetPropertyStr(ctx, obj, "count", JS_NewInt64(ctx, 5))
  → C++: JS_Call(ctx, updater_func, "count", 5)
  → JS: __onDataUpdate("count", 5)
  → DOM: getElementById('count').textContent = 5

JS: data.count  →  直接读 JSValue  →  5
Python: data.count  →  ctypes → JS_GetPropertyStr  →  5
```

## 7. 实现计划

| 阶段 | 内容 | 文件 |
|------|------|------|
| 1 | C API 头文件新增 | `core/api/lightui.h` |
| 2 | C++ 实现 | `core/api/lightui.cpp` |
| 3 | 编译 DLL | `cmake --build` |
| 4 | Python ctypes 绑定 | `bindings/python/lightui/_ffi.py` |
| 5 | Python SharedState 类 | `bindings/python/lightui/shared.py` |
| 6 | 更新 App 类 | `bindings/python/lightui/app_v2.py` |
| 7 | 示例 counter_v4 | `bindings/python/examples/counter_v4.py` |
| 8 | 编译 + 运行验证 | — |

## 8. 待清理

共享 C 对象方案成功后，以下旧代码可移除：
- `_state.py` — State 类（已 broken）
- `lightui.h` 中 `lightui_state_*` 系列函数
- `lightui.cpp` 中 StateManager 相关代码
- `ui.py` 中旧的响应式 JS 代码