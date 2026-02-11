# 需求文档：Host Event System

## 简介

为 LightUI 框架的 HostBridge 补齐"宿主主动推送 JS"通道，实现类似 Electron `ipcRenderer.on` / Tauri `listen` 的标准事件监听模式。当前架构中 Python → JS 方向缺乏主动推送能力，JS 端只能通过 `py.funcName()` 同步调用获取返回值来更新 DOM，导致冗余的"既赋值又返回"模式。本功能通过在 HostBridge 层添加事件注册/分发机制，并与 StateManager 的 watch 机制集成，使状态变化能自动触发 JS 回调，实现声明式的响应式 UI 更新。

## 术语表

- **HostBridge**：C++ 层的 JS ↔ 宿主语言通信桥接组件，负责在 JS 全局注册 `host` 对象
- **StateManager**：C++ 层的线程安全状态存储组件，支持 watch/unwatch 监听状态变化
- **EventQueue**：事件队列，存储待分发的事件，由 EventLoop 每帧 flush
- **EventLoop**：主事件循环，每帧调用 Update/Render，负责处理 StateManager 队列和定时器回调
- **Listener**：JS 端通过 `host.on()` 注册的事件回调函数
- **ListenerId**：每个 Listener 的唯一整数标识，用于 `host.off()` 取消注册
- **processQueue**：StateManager 的队列处理方法，在 EventLoop 每帧 Update 和 jsPyCall 返回后调用
- **App_Python**：Python 高级 API 层的 App 类（`lightui/app.py`）

## 需求

### 需求 1：JS 端事件监听注册与取消

**用户故事：** 作为前端开发者，我希望在 JS 端通过标准事件监听 API 注册和取消事件回调，以便响应来自宿主层的事件推送。

#### 验收标准

1. WHEN JS 代码调用 `host.on(eventName, callback)` 时，THE HostBridge SHALL 注册该 callback 为指定事件名的 Listener，并返回一个唯一的 ListenerId
2. WHEN JS 代码调用 `host.off(listenerId)` 时，THE HostBridge SHALL 移除对应的 Listener，使其不再接收后续事件
3. WHEN 同一事件名注册多个 Listener 时，THE HostBridge SHALL 按注册顺序依次调用所有 Listener
4. WHEN 传入无效参数（eventName 非字符串、callback 非函数、listenerId 不存在）时，THE HostBridge SHALL 忽略该调用且不产生副作用
5. WHEN 一个 Listener 被 `host.off()` 移除后再次对同一 ListenerId 调用 `host.off()` 时，THE HostBridge SHALL 忽略该重复调用

### 需求 2：C++ 层事件分发

**用户故事：** 作为框架开发者，我希望 C++ 层能够向 JS 端分发事件，以便宿主语言能主动推送数据给前端。

#### 验收标准

1. WHEN C++ 代码调用 `HostBridge::emit(eventName, data)` 时，THE HostBridge SHALL 将事件放入 EventQueue
2. WHEN EventLoop 执行每帧 Update 时，THE HostBridge SHALL flush EventQueue 中的所有待处理事件，在 JS 上下文中依次调用对应的 Listener
3. WHEN emit 的事件名没有任何已注册的 Listener 时，THE HostBridge SHALL 丢弃该事件且不产生错误
4. WHEN 同一帧内多次 emit 同一事件名时，THE HostBridge SHALL 按 emit 顺序依次分发每个事件（不合并）

### 需求 3：状态变化自动事件

**用户故事：** 作为前端开发者，我希望当 Python 端修改共享状态后，JS 端能自动收到通知并更新 UI，无需 Python 函数返回值。

#### 验收标准

1. WHEN JS 代码调用 `host.on(stateName, callback)` 且 stateName 对应一个已存在的 StateManager 状态时，THE HostBridge SHALL 自动为该状态注册一个 StateManager watcher，使状态变化时自动 emit 以 stateName 为事件名的事件
2. WHEN 状态值发生变化并触发自动事件时，THE HostBridge SHALL 将新的状态值作为事件数据传递给 Listener callback
3. WHEN 某个状态名的所有 Listener 都被 `host.off()` 移除后，THE HostBridge SHALL 自动取消对应的 StateManager watcher
4. WHEN 状态名与自定义事件名相同时，THE HostBridge SHALL 同时触发自动状态事件和手动 emit 的事件，两者共享同一组 Listener

### 需求 4：Python 端事件发送 API

**用户故事：** 作为 Python 开发者，我希望通过简洁的 API 从 Python 端向 JS 端发送自定义事件，以便在后台逻辑完成后通知前端。

#### 验收标准

1. WHEN Python 代码调用 `app.emit(eventName, data)` 时，THE App_Python SHALL 将事件传递到 C++ HostBridge 的 EventQueue
2. WHEN data 参数为 Python 基本类型（None、bool、int、float、str、list、dict）时，THE App_Python SHALL 将其正确序列化为 JSON 并传递给 JS 端
3. IF data 参数为不可序列化的类型时，THEN THE App_Python SHALL 抛出 TypeError 异常并附带描述性错误信息

### 需求 5：向后兼容

**用户故事：** 作为现有 LightUI 用户，我希望新的事件系统不破坏现有的 `py.funcName()` 调用模式和 `host.state.watch()` 功能，以便平滑升级。

#### 验收标准

1. THE HostBridge SHALL 保留现有的 `py.funcName(args)` 同步调用机制，使其返回值行为不变
2. THE HostBridge SHALL 保留现有的 `host.state.watch(name, callback)` API，使其行为不变
3. THE HostBridge SHALL 保留现有的 `host.state.get(name)` 和 `host.state.set(name, value)` API，使其行为不变
4. WHEN 同时使用 `host.state.watch()` 和 `host.on()` 监听同一状态时，THE HostBridge SHALL 分别触发两种回调，互不干扰

### 需求 6：线程安全与事件队列

**用户故事：** 作为框架开发者，我希望事件队列在多线程环境下安全工作，以便 Python 后台线程能安全地发送事件。

#### 验收标准

1. WHEN 多个线程同时调用 emit 时，THE EventQueue SHALL 使用互斥锁保护队列操作，确保不丢失事件且不产生数据竞争
2. THE EventQueue SHALL 仅在主线程（EventLoop Update）中 flush 事件并执行 JS 回调，确保 JS 上下文的单线程安全
3. WHEN EventQueue 中有待处理事件时，THE EventLoop SHALL 在每帧 Update 阶段先处理 StateManager 队列，再 flush EventQueue
