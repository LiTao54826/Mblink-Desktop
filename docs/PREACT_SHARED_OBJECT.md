# Preact-First + SharedObject 架构设计文档

## 概述

本文档描述 LightUI Python 绑定的 **Preact-First + SharedObject** 架构，实现了 Python ↔ C ↔ JS 三层零拷贝共享状态。

## 架构图

```
Python 侧                  C 层 (DLL)               JS 侧 (QuickJS)
─────────────────────      ──────────────────────    ──────────────────────
data = app.shared("data")  SharedObjectData {        globalThis.data = {
data.count = 0        ──►    JSContext* ctx            count: 0          }
data.count += 1       ──►    JSValue js_obj  ◄──────  data.count  →  0
                             notifyUpdate()  ──────►  __onSharedUpdate()
                           }                          → Preact re-render

py.increment()  ◄──────────────────────────────────  py.increment()
(Python callback)          HostBridge                (JS 调用)
```

## 核心组件

### 1. C API (`core/api/lightui.h`)

```c
// 创建共享对象，自动注册为 globalThis.<name>
LightUISharedHandle lightui_shared_create(LightUIHandle handle, const char* name);
void lightui_shared_destroy(LightUISharedHandle shared);

// 类型化 setter（自动触发 JS __onSharedUpdate）
int lightui_shared_set_int(LightUISharedHandle, const char* key, int64_t value);
int lightui_shared_set_double(LightUISharedHandle, const char* key, double value);
int lightui_shared_set_string(LightUISharedHandle, const char* key, const char* value);
int lightui_shared_set_bool(LightUISharedHandle, const char* key, bool value);
int lightui_shared_set_null(LightUISharedHandle, const char* key);
int lightui_shared_set_json(LightUISharedHandle, const char* key, const char* json_str);

// 类型化 getter
int64_t     lightui_shared_get_int(LightUISharedHandle, const char* key);
double      lightui_shared_get_double(LightUISharedHandle, const char* key);
const char* lightui_shared_get_string(LightUISharedHandle, const char* key);  // 需 lightui_free()
bool        lightui_shared_get_bool(LightUISharedHandle, const char* key);
const char* lightui_shared_get_json(LightUISharedHandle, const char* key);    // 需 lightui_free()

// 批量更新（抑制中间通知）
void lightui_shared_batch_begin(LightUISharedHandle);
void lightui_shared_batch_end(LightUISharedHandle);
```

### 2. Python 代理类 (`bindings/python/lightui/shared.py`)

```python
data = app.shared("data")   # 创建 SharedState 代理
data.count = 0              # → lightui_shared_set_int (自动类型推断)
data.name = "hello"         # → lightui_shared_set_string
data.flag = True            # → lightui_shared_set_bool
x = data.count              # → lightui_shared_get_int

# 批量更新（只触发一次 __onSharedUpdate）
with data.batch():
    data.x = 1
    data.y = 2
    data.z = 3
```

### 3. App 方法 (`bindings/python/lightui/app_v2.py`)

```python
app = App("Title", 800, 600)
data = app.shared("data")       # 创建共享对象
app.load_html("...")            # 加载 HTML（需包含 <div id="root">）
app.load_preact("ui/app.js")    # 加载 Preact + 用户 JS（同步 eval）
app.run()
```

### 4. JS 侧 (`ui/app.js`)

```js
const { h, render } = Preact;

function App() {
    return h('div', null,
        h('span', null, data.count),          // 直接读取共享对象
        h('button', { onClick: () => py.increment() }, '+1'),  // 调用 Python
    );
}

var _root = document.getElementById('root');
function rerender() { render(h(App), _root); }

globalThis.__onSharedUpdate = function(key) { rerender(); };  // Python 写入时触发
rerender();  // 首次渲染
```

## 关键设计决策

| 决策 | 原因 |
|------|------|
| 用 `getElementById('root')` 而非 `document.body` | QuickJS DOM 不支持 `document.body` 属性 |
| 用 `eval_js` 而非 `eval_module` 加载 app.js | `eval_module` 在 QuickJS 中是异步的（返回 Promise），首次渲染不会执行 |
| SharedObjectData 缓存 `updater_func` | 避免每次 setter 都 `JS_GetPropertyStr`，但需在 `__onSharedUpdate` 定义后刷新 |
| Python 在 `load_html` 之后调用 `shared()` | 确保 DOM 已就绪（虽然 QuickJS runtime 不会被 load_html 重置） |

## 已知限制

- `__onSharedUpdate` 必须在 `app.js` 里定义；如果 Python 在 JS 定义之前修改 shared 对象，通知会被静默忽略（`updater_func` 为 undefined）
- QuickJS GC 在关闭时有 2 个 Element 对象泄漏（不影响运行时）
- `lightui_shared_get_string` / `lightui_shared_get_json` 返回的字符串需要调用 `lightui_free()` 释放（Python 层已自动处理）

## 示例

```
bindings/python/examples/counter_preact/
├── main.py       # Python 入口：创建 App、shared、bind、load_preact
└── ui/
    └── app.js    # Preact UI：读取 data.count，调用 py.increment()
```

运行：
```bash
cd d:/code/C/MBink
python bindings/python/examples/counter_preact/main.py
```

