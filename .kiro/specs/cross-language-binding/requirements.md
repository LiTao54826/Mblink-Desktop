# Requirements Document

## Introduction

LightUI 跨语言绑定系统，实现 JS 负责 UI 渲染、宿主语言（Python/Rust/Go）负责数据和业务逻辑的架构。核心是一个线程安全的状态管理器，支持多语言间的双向数据同步。

## Glossary

- **StateManager**: C++ 层的状态管理器，是所有状态数据的唯一存储位置
- **State**: 一个命名的状态值，支持 null/bool/int/double/string/array/object 类型
- **Operation_Queue**: 线程安全的操作队列，用于收集来自任意线程的状态修改请求
- **Watcher**: 状态变化监听器，当状态变更时触发回调
- **Host_Bridge**: JS 端的 host 对象，提供调用宿主语言函数和访问共享状态的能力
- **C_API**: 对外暴露的 C 语言接口层，供各语言绑定使用
- **Batch_Mode**: 批量操作模式，多个状态变更只触发一次通知
- **Merge_Mode**: 操作合并模式，连续的同名 SET 操作只保留最后一次

## Requirements

### Requirement 1: 状态创建与类型管理

**User Story:** As a developer, I want to create typed state values, so that I can store and manage application data with type safety.

#### Acceptance Criteria

1. THE StateManager SHALL support creating states of types: null, bool, int64, double, string, array, object
2. WHEN a state is created with a name that already exists, THEN THE StateManager SHALL return LIGHTUI_ERR_ALREADY_EXISTS
3. WHEN a state is created with an empty or invalid name, THEN THE StateManager SHALL return LIGHTUI_ERR_INVALID_NAME
4. THE StateManager SHALL provide a type query function that returns the current type of a named state
5. WHEN querying a non-existent state's type, THEN THE StateManager SHALL return LIGHTUI_TYPE_NULL

### Requirement 2: 状态读取

**User Story:** As a developer, I want to read state values with type-specific accessors, so that I can efficiently retrieve data without unnecessary serialization.

#### Acceptance Criteria

1. THE StateManager SHALL provide type-specific getter functions: getBool, getInt, getDouble, getString, getJson
2. WHEN reading a state with mismatched type, THEN THE StateManager SHALL return a default value (false/0/0.0/""/null)
3. THE getString function SHALL return a reference to an internal cache, avoiding memory allocation
4. THE getJson function SHALL return a deep copy of the state value
5. THE StateManager SHALL provide getAt(name, index) for array element access
6. THE StateManager SHALL provide getKey(name, key) for object property access
7. WHEN accessing an array with out-of-range index, THEN THE StateManager SHALL return null
8. WHEN accessing an object with non-existent key, THEN THE StateManager SHALL return null

### Requirement 3: 状态写入与线程安全

**User Story:** As a developer, I want to modify state values from any thread safely, so that I can update UI state from background tasks without race conditions.

#### Acceptance Criteria

1. THE StateManager SHALL enqueue all write operations instead of applying them immediately
2. THE Operation_Queue SHALL be protected by a mutex for thread-safe access
3. WHEN processQueue is called, THEN THE StateManager SHALL apply all pending operations in order
4. THE StateManager SHALL provide type-specific setter functions: setNull, setBool, setInt, setDouble, setString, setJson
5. WHEN Merge_Mode is enabled and multiple SET operations target the same state, THEN THE StateManager SHALL keep only the last operation
6. THE enqueue operation SHALL return immediately without blocking the caller thread

### Requirement 4: 数组操作

**User Story:** As a developer, I want to perform common array operations on array states, so that I can efficiently manipulate list data.

#### Acceptance Criteria

1. THE StateManager SHALL provide arrayPush to append an item to an array state
2. THE StateManager SHALL provide arrayPop to remove the last item from an array state
3. THE StateManager SHALL provide arrayShift to remove the first item from an array state
4. THE StateManager SHALL provide arrayUnshift to prepend an item to an array state
5. THE StateManager SHALL provide arrayRemove(index) to remove an item at a specific position
6. THE StateManager SHALL provide arrayClear to remove all items from an array state
7. THE StateManager SHALL provide arraySet(index, value) to update an item at a specific position
8. WHEN an array operation is performed on a non-array state, THEN THE StateManager SHALL return LIGHTUI_ERR_TYPE_MISMATCH

### Requirement 5: 对象操作

**User Story:** As a developer, I want to perform common object operations on object states, so that I can efficiently manipulate dictionary data.

#### Acceptance Criteria

1. THE StateManager SHALL provide objectSet(key, value) to set a property on an object state
2. THE StateManager SHALL provide objectRemove(key) to remove a property from an object state
3. THE StateManager SHALL provide objectClear to remove all properties from an object state
4. WHEN an object operation is performed on a non-object state, THEN THE StateManager SHALL return LIGHTUI_ERR_TYPE_MISMATCH

### Requirement 6: 数值与字符串原子操作

**User Story:** As a developer, I want atomic increment/multiply operations for numbers and append/prepend for strings, so that I can safely update counters and build strings from multiple threads.

#### Acceptance Criteria

1. THE StateManager SHALL provide increment(delta) to atomically add a value to a numeric state
2. THE StateManager SHALL provide multiply(factor) to atomically multiply a numeric state
3. THE StateManager SHALL provide stringAppend(suffix) to append text to a string state
4. THE StateManager SHALL provide stringPrepend(prefix) to prepend text to a string state
5. WHEN increment/multiply is performed on a non-numeric state, THEN THE StateManager SHALL return LIGHTUI_ERR_TYPE_MISMATCH
6. WHEN stringAppend/stringPrepend is performed on a non-string state, THEN THE StateManager SHALL return LIGHTUI_ERR_TYPE_MISMATCH

### Requirement 7: 状态监听

**User Story:** As a developer, I want to watch for state changes, so that I can react to data updates and synchronize UI.

#### Acceptance Criteria

1. THE StateManager SHALL provide watch(name, callback) that returns a unique watch_id
2. WHEN a watched state changes, THEN THE StateManager SHALL invoke all registered callbacks with the new value
3. THE StateManager SHALL provide unwatch(watch_id) to remove a specific watcher
4. WHEN unwatch is called with an invalid watch_id, THEN THE StateManager SHALL ignore the call without error
5. THE callbacks SHALL be invoked on the main thread after processQueue completes

### Requirement 8: 批量操作

**User Story:** As a developer, I want to batch multiple state changes into a single notification, so that I can avoid unnecessary UI updates during complex operations.

#### Acceptance Criteria

1. THE StateManager SHALL provide batchBegin() to enter batch mode
2. THE StateManager SHALL provide batchEnd() to exit batch mode and trigger notifications
3. WHILE in Batch_Mode, THE StateManager SHALL accumulate changed state names without triggering callbacks
4. WHEN batchEnd is called, THEN THE StateManager SHALL notify watchers for all accumulated changes
5. IF batchEnd is called without a matching batchBegin, THEN THE StateManager SHALL ignore the call

### Requirement 9: C API 封装

**User Story:** As a language binding developer, I want a stable C API, so that I can create bindings for Python/Rust/Go.

#### Acceptance Criteria

1. THE C_API SHALL expose all StateManager functionality through C-compatible functions
2. THE C_API SHALL use LightUIHandle as an opaque pointer to the window/state context
3. THE C_API SHALL return LightUIError codes for all operations that can fail
4. THE C_API SHALL provide lightui_free() for releasing memory allocated by the library
5. THE C_API SHALL provide lightui_last_error() to retrieve the last error message
6. THE C_API SHALL use JSON strings for complex type serialization/deserialization

### Requirement 10: JS Host Bridge

**User Story:** As a JS developer, I want to access shared state and call host functions from JavaScript, so that I can build reactive UIs that communicate with the backend.

#### Acceptance Criteria

1. THE Host_Bridge SHALL expose host.call(name, args) for synchronous function invocation
2. THE Host_Bridge SHALL expose host.state.get(name) to read a state value
3. THE Host_Bridge SHALL expose host.state.set(name, value) to write a state value
4. THE Host_Bridge SHALL expose host.state.watch(name, callback) to observe state changes
5. WHEN host.call is invoked, THEN THE Host_Bridge SHALL block until the host function returns
6. WHEN host.state.set is invoked, THEN THE Host_Bridge SHALL enqueue the operation and return immediately

### Requirement 11: useSharedState Hook

**User Story:** As a React/Preact developer, I want a useSharedState hook, so that I can bind UI components to shared state with automatic re-rendering.

#### Acceptance Criteria

1. THE useSharedState hook SHALL return a [value, setter] tuple
2. WHEN the underlying state changes, THEN THE useSharedState hook SHALL trigger a component re-render
3. WHEN the setter function is called, THEN THE useSharedState hook SHALL update the shared state
4. THE useSharedState hook SHALL automatically subscribe on mount and unsubscribe on unmount
5. WHEN multiple components use the same state, THEN all components SHALL re-render when the state changes
