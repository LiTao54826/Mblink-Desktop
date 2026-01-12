# Design Document: Cross-Language Binding

## Overview

本设计实现 LightUI 的跨语言绑定系统，核心是一个线程安全的 StateManager，作为 C++/JS/Python 之间的数据桥梁。

架构分层：
```
宿主语言 (Python/Rust/Go)
        ↓
    C API 层 (lightui.h)
        ↓
  StateManager (C++)  ←→  Host Bridge (JS)
        ↓
    QuickJS Runtime
```

设计参考：`#[[file:docs/binding_design.md]]`

## Architecture

### 数据流

```
┌─────────────────────────────────────────────────────────────┐
│                    StateManager                             │
│                                                             │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              State Store (唯一数据源)                │  │
│   │         std::unordered_map<string, json>            │  │
│   └─────────────────────────────────────────────────────┘  │
│                            │                                │
│   ┌────────────────────────▼────────────────────────────┐  │
│   │            Operation Queue (线程安全)                │  │
│   │              std::deque<StateOperation>             │  │
│   │              protected by std::mutex                │  │
│   └─────────────────────────────────────────────────────┘  │
│                            │                                │
│                   processQueue()                            │
│                            │                                │
│   ┌────────────────────────▼────────────────────────────┐  │
│   │              Watcher Notification                   │  │
│   │         std::vector<Watcher> watchers_              │  │
│   └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 线程模型

- **读操作**：使用 `shared_mutex` 的共享锁，允许多线程并发读取
- **写操作**：入队到 Operation Queue，立即返回
- **队列处理**：主线程调用 `processQueue()`，应用所有待处理操作
- **回调触发**：在 `processQueue()` 完成后，主线程触发所有 watcher 回调

## Components and Interfaces

### StateManager

```cpp
namespace lightui {

class StateManager {
public:
    // 创建
    int createNull(const std::string& name);
    int createBool(const std::string& name, bool value);
    int createInt(const std::string& name, int64_t value);
    int createDouble(const std::string& name, double value);
    int createString(const std::string& name, const std::string& value);
    int createArray(const std::string& name);
    int createObject(const std::string& name);
    int createJson(const std::string& name, const json& value);
    
    // 读取
    bool exists(const std::string& name) const;
    LightUIType type(const std::string& name) const;
    bool getBool(const std::string& name) const;
    int64_t getInt(const std::string& name) const;
    double getDouble(const std::string& name) const;
    const std::string& getString(const std::string& name) const;
    json getJson(const std::string& name) const;
    json getAt(const std::string& name, int index) const;
    json getKey(const std::string& name, const std::string& key) const;
    size_t getLength(const std::string& name) const;
    
    // 写入（入队）
    int setNull(const std::string& name);
    int setBool(const std::string& name, bool value);
    int setInt(const std::string& name, int64_t value);
    int setDouble(const std::string& name, double value);
    int setString(const std::string& name, const std::string& value);
    int setJson(const std::string& name, const json& value);
    int remove(const std::string& name);
    
    // 数组操作（入队）
    int arrayPush(const std::string& name, const json& item);
    int arrayPop(const std::string& name);
    int arrayShift(const std::string& name);
    int arrayUnshift(const std::string& name, const json& item);
    int arrayRemove(const std::string& name, int index);
    int arrayClear(const std::string& name);
    int arraySet(const std::string& name, int index, const json& item);
    
    // 对象操作（入队）
    int objectSet(const std::string& name, const std::string& key, const json& value);
    int objectRemove(const std::string& name, const std::string& key);
    int objectClear(const std::string& name);
    
    // 数值操作（入队）
    int increment(const std::string& name, double delta);
    int multiply(const std::string& name, double factor);
    
    // 字符串操作（入队）
    int stringAppend(const std::string& name, const std::string& suffix);
    int stringPrepend(const std::string& name, const std::string& prefix);
    
    // 监听
    int watch(const std::string& name, StateCallback callback);
    void unwatch(int watchId);
    
    // 批量/队列
    void batchBegin();
    void batchEnd();
    void setMergeMode(bool enable);
    void processQueue();
    size_t queueSize() const;

private:
    std::unordered_map<std::string, json> states_;
    mutable std::shared_mutex statesMutex_;
    
    std::deque<StateOperation> opQueue_;
    mutable std::mutex queueMutex_;
    bool mergeMode_ = true;
    
    std::vector<Watcher> watchers_;
    int nextWatcherId_ = 0;
    std::mutex watchersMutex_;
    
    bool batchMode_ = false;
    std::set<std::string> batchChanges_;
    
    mutable std::unordered_map<std::string, std::string> stringCache_;
};

}
```

### StateOperation

```cpp
enum class StateOp {
    SET, DELETE,
    INCREMENT, MULTIPLY,
    ARRAY_PUSH, ARRAY_POP, ARRAY_SHIFT, ARRAY_UNSHIFT,
    ARRAY_REMOVE, ARRAY_CLEAR, ARRAY_SET,
    OBJECT_SET, OBJECT_REMOVE, OBJECT_CLEAR,
    STRING_APPEND, STRING_PREPEND
};

struct StateOperation {
    StateOp op;
    std::string name;
    json value;
    std::string key;    // for object operations
    int index = -1;     // for array operations
};
```

### HostBridge (JS 端)

```cpp
class HostBridge {
public:
    HostBridge(JSContext* ctx, StateManager* stateManager);
    
    // 注册 host 对象到 JS 全局
    void registerGlobal();
    
    // 绑定宿主函数
    void bind(const std::string& name, HostCallback callback, void* userData);
    void unbind(const std::string& name);

private:
    JSContext* ctx_;
    StateManager* stateManager_;
    std::unordered_map<std::string, HostFunction> functions_;
    
    // JS 方法实现
    static JSValue jsCall(JSContext* ctx, JSValueConst thisVal, 
                          int argc, JSValueConst* argv, void* opaque);
    static JSValue jsStateGet(JSContext* ctx, JSValueConst thisVal,
                              int argc, JSValueConst* argv, void* opaque);
    static JSValue jsStateSet(JSContext* ctx, JSValueConst thisVal,
                              int argc, JSValueConst* argv, void* opaque);
    static JSValue jsStateWatch(JSContext* ctx, JSValueConst thisVal,
                                int argc, JSValueConst* argv, void* opaque);
};
```

## Data Models

### LightUIType

```cpp
enum LightUIType {
    LIGHTUI_TYPE_NULL = 0,
    LIGHTUI_TYPE_BOOL,
    LIGHTUI_TYPE_INT,
    LIGHTUI_TYPE_DOUBLE,
    LIGHTUI_TYPE_STRING,
    LIGHTUI_TYPE_ARRAY,
    LIGHTUI_TYPE_OBJECT
};
```

### LightUIError

```cpp
enum LightUIError {
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
};
```

### Watcher

```cpp
struct Watcher {
    int id;
    std::string name;
    StateCallback callback;
};

using StateCallback = std::function<void(const std::string& name, const json& value)>;
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: State Creation Type Consistency

*For any* state created with a specific type (bool/int/double/string/array/object), querying its type SHALL return the corresponding LightUIType value.

**Validates: Requirements 1.1, 1.4**

### Property 2: Duplicate Name Rejection

*For any* state name that already exists in the StateManager, attempting to create another state with the same name SHALL return LIGHTUI_ERR_ALREADY_EXISTS.

**Validates: Requirements 1.2**

### Property 3: Invalid Name Rejection

*For any* empty string or string containing only whitespace, attempting to create a state with that name SHALL return LIGHTUI_ERR_INVALID_NAME.

**Validates: Requirements 1.3**

### Property 4: Getter Round-Trip Consistency

*For any* state value set via a type-specific setter, reading it back with the corresponding getter after processQueue SHALL return an equivalent value.

**Validates: Requirements 2.1, 3.4**

### Property 5: Type Mismatch Default Values

*For any* state of type T, reading it with a getter for type U (where T ≠ U) SHALL return the default value for type U (false/0/0.0/""/null).

**Validates: Requirements 2.2**

### Property 6: JSON Deep Copy Isolation

*For any* state value, the json object returned by getJson SHALL be independent of the internal state—modifying the returned json SHALL NOT affect the stored state.

**Validates: Requirements 2.4**

### Property 7: Queue-Based Write Semantics

*For any* write operation (set/array/object/numeric/string), the state SHALL NOT change until processQueue is called.

**Validates: Requirements 3.1, 3.3**

### Property 8: Merge Mode Optimization

*For any* sequence of SET operations on the same state name with mergeMode enabled, only the last value SHALL be applied after processQueue.

**Validates: Requirements 3.5**

### Property 9: Array Operations Correctness

*For any* array state and sequence of array operations (push/pop/shift/unshift/remove/clear/set), the resulting array SHALL match the expected state after applying operations in order.

**Validates: Requirements 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7**

### Property 10: Object Operations Correctness

*For any* object state and sequence of object operations (set/remove/clear), the resulting object SHALL match the expected state after applying operations in order.

**Validates: Requirements 5.1, 5.2, 5.3**

### Property 11: Type Mismatch Error Handling

*For any* operation that requires a specific type (array ops on non-array, object ops on non-object, numeric ops on non-numeric, string ops on non-string), the operation SHALL return LIGHTUI_ERR_TYPE_MISMATCH.

**Validates: Requirements 4.8, 5.4, 6.5, 6.6**

### Property 12: Atomic Numeric Operations

*For any* numeric state with initial value V, increment(delta) SHALL result in V + delta, and multiply(factor) SHALL result in V * factor.

**Validates: Requirements 6.1, 6.2**

### Property 13: String Concatenation Operations

*For any* string state with initial value S, stringAppend(suffix) SHALL result in S + suffix, and stringPrepend(prefix) SHALL result in prefix + S.

**Validates: Requirements 6.3, 6.4**

### Property 14: Watcher Notification

*For any* watched state, when the state changes via processQueue, all registered watchers SHALL be invoked with the new value.

**Validates: Requirements 7.1, 7.2, 7.3**

### Property 15: Batch Mode Deferred Notification

*For any* sequence of state changes within batchBegin/batchEnd, watchers SHALL NOT be invoked until batchEnd is called, and then all changed states SHALL trigger their watchers exactly once.

**Validates: Requirements 8.3, 8.4**

## Error Handling

### Error Codes

| Error Code | Meaning | Recovery |
|------------|---------|----------|
| LIGHTUI_OK | Success | N/A |
| LIGHTUI_ERR_INVALID_HANDLE | Null or invalid handle | Check handle before use |
| LIGHTUI_ERR_NOT_FOUND | State name not found | Create state first |
| LIGHTUI_ERR_TYPE_MISMATCH | Operation incompatible with state type | Check type before operation |
| LIGHTUI_ERR_INDEX_OUT_OF_RANGE | Array index out of bounds | Validate index |
| LIGHTUI_ERR_INVALID_JSON | JSON parse error | Validate JSON string |
| LIGHTUI_ERR_ALREADY_EXISTS | State name already exists | Use different name or check exists() |
| LIGHTUI_ERR_INVALID_NAME | Empty or invalid state name | Use non-empty alphanumeric name |
| LIGHTUI_ERR_QUEUE_FULL | Operation queue overflow | Process queue or increase limit |

### Error Retrieval

```cpp
// C API
const char* lightui_last_error(void);  // Returns last error message

// C++ API
std::string StateManager::lastError() const;
```

## Testing Strategy

### Unit Tests

单元测试覆盖：
- StateManager 各类型创建/读取/写入
- 数组操作边界条件
- 对象操作边界条件
- 错误码返回
- Watcher 注册/注销

### Property-Based Tests

使用 [rapidcheck](https://github.com/emil-e/rapidcheck) 进行属性测试：

```cpp
#include <rapidcheck.h>

// Property 1: State Creation Type Consistency
RC_GTEST_PROP(StateManager, TypeConsistency, (int typeIndex, std::string name)) {
    // Generate valid name
    RC_PRE(!name.empty() && name.find_first_not_of(" \t\n") != std::string::npos);
    
    StateManager sm;
    LightUIType expectedType;
    
    switch (typeIndex % 7) {
        case 0: sm.createNull(name); expectedType = LIGHTUI_TYPE_NULL; break;
        case 1: sm.createBool(name, true); expectedType = LIGHTUI_TYPE_BOOL; break;
        case 2: sm.createInt(name, 42); expectedType = LIGHTUI_TYPE_INT; break;
        case 3: sm.createDouble(name, 3.14); expectedType = LIGHTUI_TYPE_DOUBLE; break;
        case 4: sm.createString(name, "test"); expectedType = LIGHTUI_TYPE_STRING; break;
        case 5: sm.createArray(name); expectedType = LIGHTUI_TYPE_ARRAY; break;
        case 6: sm.createObject(name); expectedType = LIGHTUI_TYPE_OBJECT; break;
    }
    
    RC_ASSERT(sm.type(name) == expectedType);
}
```

### Integration Tests

集成测试覆盖：
- C API 完整流程
- JS Host Bridge 双向通信
- useSharedState Hook 响应式更新
- 多线程并发写入

### Test Configuration

- 属性测试：每个属性至少 100 次迭代
- 使用 rapidcheck 作为 C++ 属性测试框架
- 测试文件位置：`tests/cpp/state_manager_test.cpp`
