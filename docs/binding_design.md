# LightUI 跨语言绑定设计

## 核心理念

**JS 负责 UI，宿主语言负责数据和业务逻辑**

```
┌────────────────────────────────────────────────────────────┐
│                      宿主语言层                             │
│              Python / Rust / Go / Node.js                  │
│                                                            │
│  • 数据获取（数据库、API、文件）                            │
│  • 业务逻辑（计算、AI、处理）                               │
│  • 应用配置（窗口、生命周期）                               │
└──────────────────────┬─────────────────────────────────────┘
                       │
                       ▼
┌────────────────────────────────────────────────────────────┐
│                     C API 层                               │
│                   lightui.h                                │
│                                                            │
│  • 窗口管理                                                │
│  • 函数绑定                                                │
│  • 状态管理（线程安全）                                     │
└──────────────────────┬─────────────────────────────────────┘
                       │
                       ▼
┌────────────────────────────────────────────────────────────┐
│                   LightUI Core (C++)                       │
│                                                            │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐              │
│  │ QuickJS  │ ─ │   DOM    │ ─ │  Skia    │              │
│  │ (UI逻辑) │   │ (元素树) │   │ (渲染)   │              │
│  └──────────┘   └──────────┘   └──────────┘              │
└────────────────────────────────────────────────────────────┘
```

## 统一数据源架构

**数据只存一份在 C++ 层，避免内存重复**

```
┌─────────────────────────────────────────────────────────────┐
│                    C++ StateManager                         │
│                                                             │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              State Store (唯一数据源)                │  │
│   │                                                     │  │
│   │   users: [...]    count: 42    config: {...}       │  │
│   │                                                     │  │
│   └─────────────────────────────────────────────────────┘  │
│                            │                                │
│   ┌────────────────────────▼────────────────────────────┐  │
│   │            Thread-Safe Operation Queue              │  │
│   │                                                     │  │
│   │   [SET users] [APPEND logs] [INCREMENT count] ...  │  │
│   │                                                     │  │
│   │   任意线程入队 ──────► 主线程处理                    │  │
│   └─────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              Change Notifier                        │  │
│   │                                                     │  │
│   │      Python Callbacks    JavaScript Callbacks       │  │
│   │            ▼                     ▼                  │  │
│   │      执行回调函数           触发 UI 更新            │  │
│   └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**关键点：**
- Python/JS 都通过 Proxy 访问 C++ 层的数据
- 写操作入队，主线程统一处理
- 数据变更后立即通知所有订阅者


## 线程安全机制

```
  Python Thread 1       Python Thread 2       Main Thread
       │                     │                    │
  state.set(...)        state.append(...)        │
       │                     │                    │
       └─────────┬───────────┘                    │
                 ▼                                │
        ┌────────────────┐                        │
        │ Operation Queue│ ◄─ 线程安全入队        │
        │  (mutex保护)   │                        │
        └───────┬────────┘                        │
                │                                 │
                │         lightui_run() ──────────┤
                │         或 poll_events()        │
                ▼                                 ▼
        ┌────────────────────────────────────────────┐
        │           主线程批量处理队列                │
        │                                            │
        │  1. 取出所有操作                           │
        │  2. 依次应用到 State Store                 │
        │  3. 通知 Python 回调                       │
        │  4. 通知 JS 更新 UI                        │
        └────────────────────────────────────────────┘
```

**用户无需关心线程安全，直接调用即可：**

```python
# 在任意线程中都可以安全调用
def background_task():
    for i in range(100):
        progress.set(i)           # 线程安全
        logs.append(f"Step {i}")  # 线程安全
```

## C API

```c
// ========== 类型定义 ==========
typedef struct LightUIWindow* LightUIHandle;
typedef char* (*LightUICallback)(const char* args_json, void* user_data);
typedef void (*LightUIStateCallback)(const char* name, const char* value_json, void* user_data);

// 状态值类型
typedef enum {
    LIGHTUI_TYPE_NULL = 0,
    LIGHTUI_TYPE_BOOL,
    LIGHTUI_TYPE_INT,
    LIGHTUI_TYPE_DOUBLE,
    LIGHTUI_TYPE_STRING,
    LIGHTUI_TYPE_ARRAY,
    LIGHTUI_TYPE_OBJECT
} LightUIType;

// 错误码
typedef enum {
    LIGHTUI_OK = 0,
    LIGHTUI_ERR_INVALID_HANDLE = -1,
    LIGHTUI_ERR_NOT_FOUND = -2,
    LIGHTUI_ERR_TYPE_MISMATCH = -3,
    LIGHTUI_ERR_INDEX_OUT_OF_RANGE = -4,
    LIGHTUI_ERR_INVALID_JSON = -5,
    LIGHTUI_ERR_ALREADY_EXISTS = -6,
    LIGHTUI_ERR_INVALID_NAME = -7,
    LIGHTUI_ERR_QUEUE_FULL = -8,
    LIGHTUI_ERR_UNKNOWN = -99
} LightUIError;

// ========== 生命周期 ==========
int lightui_init(void);
void lightui_cleanup(void);
const char* lightui_version(void);

// ========== 窗口管理 ==========
LightUIHandle lightui_create(const char* title, int width, int height);
void lightui_destroy(LightUIHandle handle);
void lightui_run(LightUIHandle handle);
void lightui_stop(LightUIHandle handle);

// ========== UI 加载 ==========
int lightui_load_js(LightUIHandle handle, const char* js_code);
int lightui_load_file(LightUIHandle handle, const char* filepath);

// ========== 函数绑定 ==========
int lightui_bind(LightUIHandle handle, const char* name, LightUICallback cb, void* user_data);
void lightui_unbind(LightUIHandle handle, const char* name);

// ========== 状态创建（按类型） ==========
int lightui_state_create_null(LightUIHandle handle, const char* name);
int lightui_state_create_bool(LightUIHandle handle, const char* name, bool value);
int lightui_state_create_int(LightUIHandle handle, const char* name, int64_t value);
int lightui_state_create_double(LightUIHandle handle, const char* name, double value);
int lightui_state_create_string(LightUIHandle handle, const char* name, const char* value);
int lightui_state_create_array(LightUIHandle handle, const char* name);   // 空数组
int lightui_state_create_object(LightUIHandle handle, const char* name);  // 空对象
int lightui_state_create_json(LightUIHandle handle, const char* name, const char* json);

// ========== 状态读取（按类型，无需 free） ==========
LightUIType lightui_state_type(LightUIHandle handle, const char* name);
bool lightui_state_get_bool(LightUIHandle handle, const char* name);
int64_t lightui_state_get_int(LightUIHandle handle, const char* name);
double lightui_state_get_double(LightUIHandle handle, const char* name);
const char* lightui_state_get_string(LightUIHandle handle, const char* name);  // 内部缓存，无需 free
int lightui_state_get_length(LightUIHandle handle, const char* name);  // 数组/字符串长度

// 复杂类型读取（需要 free）
char* lightui_state_get_json(LightUIHandle handle, const char* name);
char* lightui_state_get_at(LightUIHandle handle, const char* name, int index);      // 数组元素
char* lightui_state_get_key(LightUIHandle handle, const char* name, const char* key); // 对象属性

// ========== 状态写入（按类型，线程安全） ==========
int lightui_state_set_null(LightUIHandle handle, const char* name);
int lightui_state_set_bool(LightUIHandle handle, const char* name, bool value);
int lightui_state_set_int(LightUIHandle handle, const char* name, int64_t value);
int lightui_state_set_double(LightUIHandle handle, const char* name, double value);
int lightui_state_set_string(LightUIHandle handle, const char* name, const char* value);
int lightui_state_set_json(LightUIHandle handle, const char* name, const char* json);

// ========== 数组操作（线程安全） ==========
int lightui_state_array_push(LightUIHandle handle, const char* name, const char* item_json);
int lightui_state_array_push_int(LightUIHandle handle, const char* name, int64_t value);
int lightui_state_array_push_double(LightUIHandle handle, const char* name, double value);
int lightui_state_array_push_string(LightUIHandle handle, const char* name, const char* value);
int lightui_state_array_pop(LightUIHandle handle, const char* name);
int lightui_state_array_shift(LightUIHandle handle, const char* name);
int lightui_state_array_unshift(LightUIHandle handle, const char* name, const char* item_json);
int lightui_state_array_remove(LightUIHandle handle, const char* name, int index);
int lightui_state_array_clear(LightUIHandle handle, const char* name);
int lightui_state_array_set(LightUIHandle handle, const char* name, int index, const char* item_json);

// ========== 对象操作（线程安全） ==========
int lightui_state_object_set(LightUIHandle handle, const char* name, const char* key, const char* value_json);
int lightui_state_object_set_int(LightUIHandle handle, const char* name, const char* key, int64_t value);
int lightui_state_object_set_double(LightUIHandle handle, const char* name, const char* key, double value);
int lightui_state_object_set_string(LightUIHandle handle, const char* name, const char* key, const char* value);
int lightui_state_object_set_bool(LightUIHandle handle, const char* name, const char* key, bool value);
int lightui_state_object_remove(LightUIHandle handle, const char* name, const char* key);
int lightui_state_object_clear(LightUIHandle handle, const char* name);

// ========== 数值操作（线程安全，原子） ==========
int lightui_state_increment(LightUIHandle handle, const char* name, double delta);
int lightui_state_multiply(LightUIHandle handle, const char* name, double factor);

// ========== 字符串操作（线程安全） ==========
int lightui_state_string_append(LightUIHandle handle, const char* name, const char* suffix);
int lightui_state_string_prepend(LightUIHandle handle, const char* name, const char* prefix);

// ========== 监听 ==========
int lightui_state_watch(LightUIHandle handle, const char* name, LightUIStateCallback cb, void* user_data);
void lightui_state_unwatch(LightUIHandle handle, int watch_id);

// ========== 批量操作 ==========
void lightui_state_batch_begin(LightUIHandle handle);
void lightui_state_batch_end(LightUIHandle handle);

// ========== 队列优化 ==========
void lightui_state_set_merge_mode(LightUIHandle handle, bool enable);  // 合并同名操作
int lightui_process_queue(LightUIHandle handle);  // 手动处理队列

// ========== 工具 ==========
void lightui_free(void* ptr);
const char* lightui_last_error(void);
bool lightui_state_exists(LightUIHandle handle, const char* name);
void lightui_state_delete(LightUIHandle handle, const char* name);
```

### 性能优化说明

1. **直接类型接口**：简单类型（bool/int/double/string）直接传值，避免 JSON 序列化开销
2. **操作合并**：启用 `merge_mode` 后，连续的同名 SET 操作只保留最后一次
3. **内部字符串缓存**：`get_string` 返回内部缓存指针，无需手动释放
4. **原子数值操作**：`increment/multiply` 是原子操作，适合计数器场景


## Python API

```python
import lightui

# 创建应用
app = lightui.App(title="My App", width=800, height=600)

# ========== 状态管理 ==========
# state() 返回 State 代理对象，而非原始值
# State 对象提供类型安全的读写方法
users = app.state("users", [])      # State[list]
count = app.state("count", 0)       # State[int]
config = app.state("config", {"theme": "light"})  # State[dict]

# 读取 - 返回当前值的拷贝
print(users.get())    # -> [...]
print(count.get())    # -> 0

# 写入（线程安全，操作入队后立即返回）
users.set([{"name": "Alice"}, {"name": "Bob"}])
users.append({"name": "Charlie"})
count.set(10)
count.increment(1)

# 监听变化（回调在主线程的下一帧触发）
@users.watch
def on_users_change(new_value):
    print(f"Users: {len(new_value)}")

# 批量更新（只触发一次通知）
with app.batch():
    users.set([...])
    count.set(0)

# ========== 函数绑定 ==========
@app.bind
def get_data():
    return {"items": db.query_all()}

@app.bind
def save_item(item):
    db.insert(item)
    return {"success": True}

# ========== 运行 ==========
app.load_file("ui/app.js")
app.run()
```

### State 类型定义

```python
from typing import TypeVar, Generic, Callable, List, Dict, Any

T = TypeVar('T')

class State(Generic[T]):
    """状态代理对象，提供线程安全的读写操作"""
    
    def get(self) -> T:
        """获取当前值的拷贝"""
        ...
    
    def set(self, value: T) -> None:
        """设置新值（入队，立即返回）"""
        ...
    
    def watch(self, callback: Callable[[T], None]) -> int:
        """监听变化，返回 watch_id"""
        ...
    
    def unwatch(self, watch_id: int) -> None:
        """取消监听"""
        ...

class IntState(State[int]):
    """整数状态，支持原子操作"""
    def increment(self, delta: int = 1) -> None: ...
    def multiply(self, factor: float) -> None: ...

class ListState(State[List[Any]]):
    """列表状态，支持数组操作"""
    def append(self, item: Any) -> None: ...
    def pop(self) -> None: ...
    def remove(self, index: int) -> None: ...
    def clear(self) -> None: ...

class DictState(State[Dict[str, Any]]):
    """字典状态，支持对象操作"""
    def set_key(self, key: str, value: Any) -> None: ...
    def remove_key(self, key: str) -> None: ...
    def clear(self) -> None: ...

class StringState(State[str]):
    """字符串状态，支持字符串操作"""
    def append(self, suffix: str) -> None: ...
    def prepend(self, prefix: str) -> None: ...
```

## JavaScript API

```javascript
// ========== 调用宿主语言函数 ==========
// 同步调用，阻塞等待结果
const data = host.call('get_data');
const result = host.call('save_item', {name: 'test'});

// ========== 共享状态 ==========
// 获取（返回当前值的拷贝）
const users = host.state.get('users');

// 设置（入队后立即返回，Python 回调在下一帧触发）
host.state.set('users', [...users, newUser]);

// 监听（Python 端修改时，在下一帧触发回调）
host.state.watch('users', (newValue) => {
    console.log('Users updated:', newValue);
});

// ========== React/Preact Hook ==========
// useSharedState 返回 [value, setter] 元组
// - value: 当前状态值，状态变化时自动触发重渲染
// - setter: 设置函数，调用后入队并立即返回
function UserList() {
    const [users, setUsers] = useSharedState('users');
    const [count, setCount] = useSharedState('count');
    
    return h('div', [
        h('ul', users.map(u => h('li', u.name))),
        h('button', { 
            onClick: () => setUsers([...users, {name: 'New'}]) 
        }, 'Add'),
        h('p', `Count: ${count}`),
        h('button', { onClick: () => setCount(count + 1) }, '+1')
    ]);
}
```

### 同步机制说明

```
  Python Thread              Main Thread (Event Loop)
       │                              │
  state.set(value)                    │
       │                              │
       ├─► 入队操作 ─────────────────►│
       │   (立即返回)                 │
       │                              ▼
       │                     ┌────────────────┐
       │                     │ processQueue() │
       │                     │  (每帧调用)    │
       │                     └───────┬────────┘
       │                             │
       │                             ▼
       │                     ┌────────────────┐
       │                     │ 应用状态变更    │
       │                     └───────┬────────┘
       │                             │
       │                             ├─► 通知 Python 回调
       │                             │
       │                             └─► 通知 JS 监听器
       │                                  └─► 触发 React 重渲染
```


## 完整示例

### Python 端

```python
import lightui
import threading
import time

app = lightui.App(title="Task Manager", width=800, height=600)

# 共享状态
tasks = app.state("tasks", [])
progress = app.state("progress", 0)
status = app.state("status", "idle")

# 监听 JS 端的操作
@tasks.watch
def on_tasks_change(new_tasks):
    # JS 添加任务时，保存到数据库
    db.save_tasks(new_tasks)

# 绑定函数供 JS 调用
@app.bind
def load_tasks():
    data = db.query_tasks()
    tasks.set(data)
    return data

@app.bind
def start_processing():
    # 启动后台线程处理
    threading.Thread(target=process_tasks, daemon=True).start()
    return {"started": True}

def process_tasks():
    status.set("processing")
    task_list = tasks.get()
    
    for i, task in enumerate(task_list):
        time.sleep(0.5)  # 模拟处理
        progress.set(int((i + 1) / len(task_list) * 100))
    
    status.set("completed")

app.load_file("ui/tasks.js")
app.run()
```

### JavaScript 端

```javascript
// ui/tasks.js
const { h, render } = preact;
const { useSharedState } = lightui;

function TaskManager() {
    const [tasks, setTasks] = useSharedState('tasks');
    const [progress] = useSharedState('progress');
    const [status] = useSharedState('status');
    
    const addTask = () => {
        const name = prompt('Task name:');
        if (name) {
            setTasks([...tasks, { id: Date.now(), name, done: false }]);
        }
    };
    
    const startProcess = () => {
        host.call('start_processing');
    };
    
    return h('div', { class: 'container' }, [
        h('h1', 'Task Manager'),
        
        h('div', { class: 'toolbar' }, [
            h('button', { onClick: addTask }, 'Add Task'),
            h('button', { onClick: startProcess, disabled: status === 'processing' }, 
                status === 'processing' ? 'Processing...' : 'Start'),
        ]),
        
        status === 'processing' && h('div', { class: 'progress' }, [
            h('div', { class: 'bar', style: `width: ${progress}%` }),
            h('span', `${progress}%`)
        ]),
        
        h('ul', { class: 'tasks' },
            tasks.map(task => 
                h('li', { key: task.id, class: task.done ? 'done' : '' }, task.name)
            )
        )
    ]);
}

render(h(TaskManager), document.body);
```


## C++ StateManager 实现

```cpp
// core/bridge/state_manager.h

#include <string>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <variant>
#include <vector>
#include <set>

namespace lightui {

// 使用 nlohmann::json 作为内部存储，天然支持递归结构
#include "nlohmann/json.hpp"
using json = nlohmann::json;

enum class StateOp {
    // 通用
    SET,
    DELETE,
    
    // 数值
    INCREMENT,
    MULTIPLY,
    
    // 数组
    ARRAY_PUSH,
    ARRAY_POP,
    ARRAY_SHIFT,
    ARRAY_UNSHIFT,
    ARRAY_REMOVE,
    ARRAY_CLEAR,
    ARRAY_SET,
    
    // 对象
    OBJECT_SET,
    OBJECT_REMOVE,
    OBJECT_CLEAR,
    
    // 字符串
    STRING_APPEND,
    STRING_PREPEND,
};

struct StateOperation {
    StateOp op;
    std::string name;
    json value;
    std::string key;    // 用于对象操作
    int index = -1;     // 用于数组操作
};

using StateCallback = std::function<void(const std::string& name, const json& value)>;

class StateManager {
public:
    StateManager();
    ~StateManager();
    
    // ========== 创建 ==========
    void createNull(const std::string& name);
    void createBool(const std::string& name, bool value);
    void createInt(const std::string& name, int64_t value);
    void createDouble(const std::string& name, double value);
    void createString(const std::string& name, const std::string& value);
    void createArray(const std::string& name);
    void createObject(const std::string& name);
    void createJson(const std::string& name, const json& value);
    
    // ========== 读取（线程安全） ==========
    bool exists(const std::string& name) const;
    json::value_t type(const std::string& name) const;
    
    bool getBool(const std::string& name) const;
    int64_t getInt(const std::string& name) const;
    double getDouble(const std::string& name) const;
    const std::string& getString(const std::string& name) const;
    size_t getLength(const std::string& name) const;
    json getJson(const std::string& name) const;
    json getAt(const std::string& name, int index) const;
    json getKey(const std::string& name, const std::string& key) const;
    
    // ========== 写入（入队） ==========
    void setNull(const std::string& name);
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int64_t value);
    void setDouble(const std::string& name, double value);
    void setString(const std::string& name, const std::string& value);
    void setJson(const std::string& name, const json& value);
    void remove(const std::string& name);
    
    // ========== 数组操作（入队） ==========
    void arrayPush(const std::string& name, const json& item);
    void arrayPop(const std::string& name);
    void arrayShift(const std::string& name);
    void arrayUnshift(const std::string& name, const json& item);
    void arrayRemove(const std::string& name, int index);
    void arrayClear(const std::string& name);
    void arraySet(const std::string& name, int index, const json& item);
    
    // ========== 对象操作（入队） ==========
    void objectSet(const std::string& name, const std::string& key, const json& value);
    void objectRemove(const std::string& name, const std::string& key);
    void objectClear(const std::string& name);
    
    // ========== 数值操作（入队） ==========
    void increment(const std::string& name, double delta);
    void multiply(const std::string& name, double factor);
    
    // ========== 字符串操作（入队） ==========
    void stringAppend(const std::string& name, const std::string& suffix);
    void stringPrepend(const std::string& name, const std::string& prefix);
    
    // ========== 监听 ==========
    int watch(const std::string& name, StateCallback callback);
    void unwatch(int watchId);
    
    // ========== 批量/队列 ==========
    void batchBegin();
    void batchEnd();
    void setMergeMode(bool enable);
    void processQueue();
    size_t queueSize() const;
    
private:
    void enqueue(StateOperation op);
    void applyOp(const StateOperation& op);
    void notify(const std::string& name);
    
    // 状态存储
    std::unordered_map<std::string, json> states_;
    mutable std::shared_mutex statesMutex_;
    
    // 字符串缓存（用于 getString 返回引用）
    mutable std::unordered_map<std::string, std::string> stringCache_;
    
    // 操作队列
    std::deque<StateOperation> opQueue_;
    mutable std::mutex queueMutex_;
    bool mergeMode_ = true;  // 默认开启合并
    
    // 监听器
    struct Watcher {
        int id;
        std::string name;
        StateCallback callback;
    };
    std::vector<Watcher> watchers_;
    int nextWatcherId_ = 0;
    std::mutex watchersMutex_;
    
    // 批量模式
    bool batchMode_ = false;
    std::set<std::string> batchChanges_;
};

} // namespace lightui
```

### 队列合并优化

```cpp
void StateManager::enqueue(StateOperation op) {
    std::lock_guard lock(queueMutex_);
    
    if (mergeMode_ && op.op == StateOp::SET) {
        // 查找并替换同名的 SET 操作
        for (auto& existing : opQueue_) {
            if (existing.name == op.name && existing.op == StateOp::SET) {
                existing.value = std::move(op.value);
                return;
            }
        }
    }
    
    opQueue_.push_back(std::move(op));
}

void StateManager::processQueue() {
    std::deque<StateOperation> ops;
    {
        std::lock_guard lock(queueMutex_);
        std::swap(ops, opQueue_);
    }
    
    std::set<std::string> changed;
    for (auto& op : ops) {
        applyOp(op);
        changed.insert(op.name);
    }
    
    if (!batchMode_) {
        for (const auto& name : changed) {
            notify(name);
        }
    } else {
        batchChanges_.insert(changed.begin(), changed.end());
    }
}
```

## 项目结构

```
lightui/
├── core/
│   ├── api/
│   │   ├── lightui.h          # C API 头文件
│   │   └── lightui.cpp        # C API 实现
│   ├── bridge/
│   │   ├── state_manager.h    # 状态管理器
│   │   ├── state_manager.cpp
│   │   ├── host_bridge.h      # JS host 对象
│   │   └── host_bridge.cpp
│   └── ...
│
├── bindings/
│   ├── python/
│   │   ├── lightui/
│   │   │   ├── __init__.py
│   │   │   ├── app.py         # App 类
│   │   │   └── state.py       # State 类
│   │   ├── src/
│   │   │   └── bindings.cpp   # pybind11 绑定
│   │   └── setup.py
│   ├── rust/
│   └── go/
│
├── components/                 # 内置 JS 组件库
│   ├── src/
│   │   ├── button.js
│   │   ├── input.js
│   │   └── index.js
│   └── build/
│       └── components.bc      # 预编译字节码
│
└── examples/
    └── python/
        ├── hello.py
        ├── dashboard.py
        └── ui/
```

## 实现路线图

### Phase 1: C++ 核心
- [ ] StateManager 实现
- [ ] C API 实现
- [ ] JS host 对象绑定

### Phase 2: JS 状态集成
- [ ] useSharedState Hook 实现
- [ ] 状态变更触发重渲染
- [ ] 端到端验证（C++ ↔ JS 双向同步）

### Phase 3: Python 绑定
- [ ] pybind11 封装
- [ ] App 类
- [ ] State 类（含类型特化）
- [ ] 装饰器支持
- [ ] 端到端验证（Python ↔ C++ ↔ JS 三向同步）

### Phase 4: 组件库
- [ ] 基础组件 (Button, Input, Select)
- [ ] 布局组件 (Row, Column, Grid)
- [ ] 数据组件 (Table, List)
- [ ] 反馈组件 (Modal, Toast)

### Phase 5: 其他语言
- [ ] Rust 绑定
- [ ] Go 绑定
