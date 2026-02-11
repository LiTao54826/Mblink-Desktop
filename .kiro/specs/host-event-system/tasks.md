# 实现计划：Host Event System

## 概述

为 HostBridge 添加事件分发系统，分为 C++ 核心层、JS 全局 API、Python 绑定层三个阶段，逐步构建并测试。每个阶段完成后通过测试验证，确保增量正确性。

## 任务

- [x] 1. C++ HostBridge 事件系统核心
  - [x] 1.1 在 host_bridge.h 中添加事件系统数据结构和接口
    - 添加 `EventListener` 结构体（id, eventName, JSValue callback）
    - 添加 `PendingEvent` 结构体（eventName, dataJson）
    - 添加 `AutoWatcher` 结构体（watcherId, listenerCount）
    - 添加成员变量：`listeners_`, `nextListenerId_`, `eventQueue_`, `eventQueueMutex_`, `autoWatchers_`
    - 添加公共方法声明：`on()`, `off()`, `emit()`, `flushEvents()`
    - 添加静态 JS 回调声明：`jsHostOn()`, `jsHostOff()`
    - 在析构函数中添加清理 JSValue 引用的逻辑
    - _Requirements: 1.1, 1.2, 2.1, 6.1_

  - [x] 1.2 在 host_bridge.cpp 中实现事件系统核心逻辑
    - 实现 `on(eventName, callback)`：注册 Listener，JS_DupValue 增引用，检查是否需要创建 auto-watcher，返回 ListenerId
    - 实现 `off(listenerId)`：移除 Listener，JS_FreeValue 减引用，检查是否需要移除 auto-watcher
    - 实现 `emit(eventName, dataJson)`：加锁将事件入队
    - 实现 `flushEvents()`：取出队列所有事件，遍历 listeners_ 按注册顺序调用匹配的回调，捕获 JS 异常
    - 实现 `jsHostOn()` 和 `jsHostOff()` 静态回调，参数校验（非字符串/非函数返回 -1）
    - 在 `registerGlobal()` 中注册 `host.on` 和 `host.off` 到 JS 全局
    - 在析构函数中遍历 listeners_ 调用 JS_FreeValue，清理 autoWatchers_
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 2.1, 2.2, 2.3, 2.4, 3.1, 3.2, 3.3, 3.4, 6.1_

- [x] 2. JS 端事件系统测试
  - [x] 2.1 创建 tests/js/test_host_event.js 基础单元测试
    - 测试 host.on 注册返回唯一 ID
    - 测试 host.off 移除后不再收到事件
    - 测试无效参数（非字符串事件名、非函数回调）
    - 测试重复 off 同一 ID
    - 测试无 Listener 时 emit 不报错
    - 测试向后兼容：py.funcName() 和 host.state.watch/get/set 仍正常
    - _Requirements: 1.1, 1.2, 1.4, 1.5, 2.3, 5.1, 5.2, 5.3_

  - [ ]* 2.2 在 test_host_event.js 中添加属性测试
    - **Property 1: Listener ID 唯一性**
    - **Validates: Requirements 1.1**
    - **Property 3: Listener 调用顺序**
    - **Validates: Requirements 1.3**
    - **Property 4: Emit-Flush-Callback 投递**
    - **Validates: Requirements 2.1, 2.2**
    - **Property 5: Emit 顺序保持**
    - **Validates: Requirements 2.4**

- [x] 3. Checkpoint - 确保 C++ 层和 JS 测试通过
  - 编译验证：`cmake --build build --config Release --target esm_loader`
  - 运行测试：`build\bin\Release\esm_loader.exe tests\js\test_host_event.js -q 5`
  - 确保所有测试通过，如有问题请询问用户

- [x] 4. 状态自动事件集成测试
  - [x] 4.1 在 test_host_event.js 中添加状态自动事件测试
    - 测试 host.on(stateName, cb) 在状态变化后收到新值
    - 测试移除所有 Listener 后状态变化不再触发回调
    - 测试 host.state.watch 和 host.on 同时监听同一状态，互不干扰
    - _Requirements: 3.1, 3.2, 3.3, 5.4_

  - [ ]* 4.2 在 test_host_event.js 中添加状态自动事件属性测试
    - **Property 6: 状态自动事件传递正确值**
    - **Validates: Requirements 3.1, 3.2**
    - **Property 7: Auto-Watcher 清理**
    - **Validates: Requirements 3.3**
    - **Property 9: Watch 与 On 共存独立性**
    - **Validates: Requirements 5.4**
    - **Property 10: 状态队列先于事件队列处理**
    - **Validates: Requirements 6.3**

- [ ] 5. Python 绑定层
  - [ ] 5.1 在 bindings.cpp 的 PyHostBridge 中添加 emit 方法
    - 实现 `emit(eventName, data)`：将 Python 对象序列化为 JSON，调用 HostBridge::emit()
    - 参数校验：不可序列化类型抛出 TypeError，空事件名抛出 ValueError
    - 在 pybind11 模块定义中注册 emit 方法
    - _Requirements: 4.1, 4.2, 4.3_

  - [ ] 5.2 在 app.py 的 App 类中添加 emit 方法
    - 实现 `emit(event, data=None)`：调用 bridge.emit()
    - 参数校验和错误处理
    - _Requirements: 4.1, 4.2, 4.3_

  - [ ] 5.3 创建 bindings/python/tests/test_host_event.py 单元测试
    - 测试 app.emit 基本流程（Python emit → JS callback 收到数据）
    - 测试各种数据类型序列化（None, bool, int, float, str, list, dict）
    - 测试不可序列化类型抛出 TypeError
    - 测试空事件名抛出 ValueError
    - 使用低级 API 测试，不创建 Window
    - _Requirements: 4.1, 4.2, 4.3_

  - [ ]* 5.4 在 test_host_event.py 中添加 hypothesis 属性测试
    - **Property 8: Python Emit 序列化往返**
    - **Validates: Requirements 4.1, 4.2**

- [ ] 6. EventLoop 集成
  - [ ] 6.1 修改 EventLoop 集成代码，在 processQueue 后调用 flushEvents
    - 在 bindings.cpp 的 PyEventLoop::run() 中，Update 回调里 processQueue 之后调用 bridge->flushEvents()
    - 在 PyEventLoop 中添加 HostBridge 指针成员和 setter 方法
    - 在 app.py 中将 bridge 传递给 event_loop
    - _Requirements: 6.2, 6.3_

- [ ] 7. 更新 counter.py 示例
  - [ ] 7.1 更新 counter.py 使用新的事件模式
    - 移除 increment/decrement 的 return 语句
    - 添加 host.on("counter", cb) 监听状态变化自动更新 DOM
    - 保留按钮的 py.increment()/py.decrement() 调用
    - _Requirements: 3.1, 3.2, 5.1_

- [ ] 8. 最终 Checkpoint - 确保所有测试通过
  - 编译验证
  - 运行 JS 测试：test_host_event.js
  - 运行 Python 测试：test_host_event.py
  - 运行现有测试确保向后兼容：test_binding.py, test_new_api.py
  - 确保所有测试通过，如有问题请询问用户

## 备注

- 标记 `*` 的任务为可选，可跳过以加速 MVP
- 每个任务引用了具体的需求编号以便追溯
- Checkpoint 任务确保增量验证
- 属性测试验证设计文档中的通用正确性属性
- 单元测试验证具体示例和边界情况
