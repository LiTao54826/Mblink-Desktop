# Requirements Document

## Introduction

LightUI Python 绑定系统，基于已实现的 C API 和 StateManager，为 Python 开发者提供简洁的 API 来构建桌面应用。Python 负责数据获取和业务逻辑，JS 负责 UI 渲染，通过共享状态实现双向同步。

## Glossary

- **App**: Python 端的应用类，封装窗口创建、状态管理和事件循环
- **State**: 状态代理对象，提供类型安全的读写操作
- **IntState**: 整数状态特化类，支持 increment/multiply 原子操作
- **ListState**: 列表状态特化类，支持 append/pop/remove 等数组操作
- **DictState**: 字典状态特化类，支持 set_key/remove_key 等对象操作
- **StringState**: 字符串状态特化类，支持 append/prepend 操作
- **StateManager_Wrapper**: Python 对 C API StateManager 的封装
- **pybind11**: C++ 库，用于创建 Python 扩展模块
- **GIL**: Global Interpreter Lock，Python 全局解释器锁
- **Callback**: Python 函数，可被 JS 端通过 host.call 调用
- **Watcher**: 状态变化监听器，当状态变更时触发 Python 回调

## Requirements

### Requirement 1: Python 模块结构

**User Story:** As a Python developer, I want to import lightui as a standard Python package, so that I can easily integrate it into my projects.

#### Acceptance Criteria

1. THE Python_Binding SHALL be importable as `import lightui`
2. THE lightui module SHALL expose App class as the main entry point
3. THE lightui module SHALL expose State, IntState, ListState, DictState, StringState classes
4. THE lightui module SHALL be compiled as a .pyd extension using pybind11
5. WHEN the module fails to load, THEN THE Python interpreter SHALL raise ImportError with clear message

### Requirement 2: App 类生命周期

**User Story:** As a Python developer, I want to create and manage application windows, so that I can build desktop GUI applications.

#### Acceptance Criteria

1. THE App class SHALL accept title, width, height parameters in constructor
2. WHEN App is instantiated, THEN THE App SHALL call lightui_init and lightui_create
3. THE App SHALL provide load_file(path) method to load JS UI files
4. THE App SHALL provide load_js(code) method to load JS code directly
5. THE App SHALL provide run() method to start the event loop
6. THE App SHALL provide stop() method to stop the event loop
7. WHEN App is garbage collected, THEN THE App SHALL call lightui_destroy and lightui_cleanup
8. THE App SHALL support context manager protocol (with statement)

### Requirement 3: 状态创建与类型推断

**User Story:** As a Python developer, I want to create typed states with automatic type inference, so that I can work with states naturally.

#### Acceptance Criteria

1. THE App.state(name, initial_value) method SHALL create a state and return appropriate State subclass
2. WHEN initial_value is int, THEN THE method SHALL return IntState
3. WHEN initial_value is float, THEN THE method SHALL return State[float]
4. WHEN initial_value is str, THEN THE method SHALL return StringState
5. WHEN initial_value is list, THEN THE method SHALL return ListState
6. WHEN initial_value is dict, THEN THE method SHALL return DictState
7. WHEN initial_value is bool, THEN THE method SHALL return State[bool]
8. WHEN initial_value is None, THEN THE method SHALL return State[None]
9. WHEN state with same name already exists, THEN THE method SHALL return existing State proxy

### Requirement 4: State 基类操作

**User Story:** As a Python developer, I want to read and write state values with a simple API, so that I can manage application data easily.

#### Acceptance Criteria

1. THE State.get() method SHALL return current value as Python object
2. THE State.set(value) method SHALL enqueue a SET operation (thread-safe)
3. THE State.watch(callback) method SHALL register a watcher and return watch_id
4. THE State.unwatch(watch_id) method SHALL remove the watcher
5. THE State.watch decorator SHALL support @state.watch syntax for registering callbacks
6. WHEN State.set is called from any thread, THEN THE operation SHALL be thread-safe

### Requirement 5: IntState 数值操作

**User Story:** As a Python developer, I want atomic increment and multiply operations for counters, so that I can safely update numeric values from multiple threads.

#### Acceptance Criteria

1. THE IntState.increment(delta=1) method SHALL atomically add delta to the value
2. THE IntState.multiply(factor) method SHALL atomically multiply the value by factor
3. THE IntState SHALL inherit all State base class methods
4. WHEN increment/multiply is called, THEN THE operation SHALL be enqueued (thread-safe)

### Requirement 6: ListState 数组操作

**User Story:** As a Python developer, I want list-like operations on array states, so that I can manipulate collections naturally.

#### Acceptance Criteria

1. THE ListState.append(item) method SHALL add item to end of array
2. THE ListState.pop() method SHALL remove and return last item
3. THE ListState.insert(index, item) method SHALL insert item at index
4. THE ListState.remove(index) method SHALL remove item at index
5. THE ListState.clear() method SHALL remove all items
6. THE ListState.__len__() method SHALL return array length
7. THE ListState.__getitem__(index) method SHALL return item at index
8. THE ListState.__setitem__(index, value) method SHALL set item at index
9. THE ListState SHALL inherit all State base class methods

### Requirement 7: DictState 对象操作

**User Story:** As a Python developer, I want dict-like operations on object states, so that I can manipulate key-value data naturally.

#### Acceptance Criteria

1. THE DictState.set_key(key, value) method SHALL set a property
2. THE DictState.remove_key(key) method SHALL remove a property
3. THE DictState.clear() method SHALL remove all properties
4. THE DictState.__getitem__(key) method SHALL return value for key
5. THE DictState.__setitem__(key, value) method SHALL set value for key
6. THE DictState.__contains__(key) method SHALL return True if key exists
7. THE DictState.keys() method SHALL return list of keys
8. THE DictState SHALL inherit all State base class methods

### Requirement 8: StringState 字符串操作

**User Story:** As a Python developer, I want string manipulation operations on string states, so that I can build strings efficiently.

#### Acceptance Criteria

1. THE StringState.append(suffix) method SHALL append text to end
2. THE StringState.prepend(prefix) method SHALL prepend text to beginning
3. THE StringState.__len__() method SHALL return string length
4. THE StringState.__add__(other) method SHALL return concatenated string (non-mutating)
5. THE StringState SHALL inherit all State base class methods

### Requirement 9: 函数绑定

**User Story:** As a Python developer, I want to expose Python functions to JavaScript, so that JS UI can call backend logic.

#### Acceptance Criteria

1. THE App.bind(name, func) method SHALL register a Python function callable from JS
2. THE @app.bind decorator SHALL support @app.bind syntax for function registration
3. WHEN JS calls host.call(name, args), THEN THE bound Python function SHALL be invoked
4. THE bound function SHALL receive args as Python dict/list
5. THE bound function return value SHALL be serialized to JSON and returned to JS
6. IF bound function raises exception, THEN THE error message SHALL be returned to JS
7. THE App.unbind(name) method SHALL remove a bound function

### Requirement 10: 批量操作

**User Story:** As a Python developer, I want to batch multiple state changes, so that I can avoid unnecessary UI updates.

#### Acceptance Criteria

1. THE App.batch() method SHALL return a context manager for batch operations
2. WHILE inside batch context, THE StateManager SHALL accumulate changes without notifying
3. WHEN exiting batch context, THEN THE StateManager SHALL notify all watchers once
4. THE batch context SHALL support nested batches (only outermost triggers notification)

### Requirement 11: 错误处理

**User Story:** As a Python developer, I want clear error messages when operations fail, so that I can debug issues easily.

#### Acceptance Criteria

1. WHEN C API returns error code, THEN THE Python binding SHALL raise LightUIError exception
2. THE LightUIError SHALL include error code and descriptive message
3. WHEN state operation fails due to type mismatch, THEN THE binding SHALL raise TypeError
4. WHEN state name is invalid, THEN THE binding SHALL raise ValueError
5. WHEN accessing non-existent state, THEN THE binding SHALL raise KeyError

