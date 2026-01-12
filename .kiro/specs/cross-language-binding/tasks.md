# Implementation Plan: Cross-Language Binding

## Overview

实现 LightUI 跨语言绑定系统，分为 C++ StateManager 核心、C API 封装、JS Host Bridge 三个主要部分。

## Tasks

- [x] 1. 项目结构和基础设施
  - [x] 1.1 创建 core/bridge 目录结构
    - 创建 `core/bridge/` 目录
    - 创建 `core/bridge/CMakeLists.txt`
    - 创建 `core/bridge/README.md`
    - _Requirements: 9.1_

  - [x] 1.2 添加 nlohmann/json 依赖
    - 在 CMakeLists.txt 中添加 nlohmann_json 依赖
    - 验证 json 头文件可用
    - _Requirements: 9.6_

- [x] 2. StateManager 核心实现
  - [x] 2.1 实现状态存储和类型系统
    - 创建 `core/bridge/state_manager.h`
    - 定义 LightUIType 枚举
    - 定义 LightUIError 枚举
    - 定义 StateOp 枚举和 StateOperation 结构
    - 实现 StateManager 类框架
    - _Requirements: 1.1, 1.4, 1.5_

  - [x] 2.2 实现状态创建方法
    - 实现 createNull, createBool, createInt, createDouble, createString
    - 实现 createArray, createObject, createJson
    - 实现名称验证（空名称、重复名称检查）
    - _Requirements: 1.1, 1.2, 1.3_

  - [ ]* 2.3 编写状态创建属性测试
    - **Property 1: State Creation Type Consistency**
    - **Property 2: Duplicate Name Rejection**
    - **Property 3: Invalid Name Rejection**
    - **Validates: Requirements 1.1, 1.2, 1.3, 1.4**

  - [x] 2.4 实现状态读取方法
    - 实现 exists, type, getBool, getInt, getDouble, getString
    - 实现 getJson (深拷贝), getAt, getKey, getLength
    - 实现字符串缓存机制
    - 实现类型不匹配时返回默认值
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8_

  - [ ]* 2.5 编写状态读取属性测试
    - **Property 4: Getter Round-Trip Consistency**
    - **Property 5: Type Mismatch Default Values**
    - **Property 6: JSON Deep Copy Isolation**
    - **Validates: Requirements 2.1, 2.2, 2.4**

- [x] 3. 操作队列和线程安全
  - [x] 3.1 实现操作队列
    - 实现 enqueue 方法（mutex 保护）
    - 实现 processQueue 方法
    - 实现 queueSize 方法
    - _Requirements: 3.1, 3.2, 3.3_

  - [x] 3.2 实现状态写入方法
    - 实现 setNull, setBool, setInt, setDouble, setString, setJson
    - 实现 remove 方法
    - 所有写入操作入队而非直接修改
    - _Requirements: 3.4_

  - [x] 3.3 实现合并模式
    - 实现 setMergeMode 方法
    - 在 enqueue 中实现同名 SET 操作合并
    - _Requirements: 3.5_

  - [ ]* 3.4 编写队列和写入属性测试
    - **Property 7: Queue-Based Write Semantics**
    - **Property 8: Merge Mode Optimization**
    - **Validates: Requirements 3.1, 3.3, 3.5**

- [x] 4. Checkpoint - 核心功能验证
  - 确保所有测试通过，ask the user if questions arise.

- [x] 5. 数组和对象操作
  - [x] 5.1 实现数组操作
    - 实现 arrayPush, arrayPop, arrayShift, arrayUnshift
    - 实现 arrayRemove, arrayClear, arraySet
    - 实现类型检查（非数组返回 TYPE_MISMATCH）
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8_

  - [x] 5.2 实现对象操作
    - 实现 objectSet, objectRemove, objectClear
    - 实现类型检查（非对象返回 TYPE_MISMATCH）
    - _Requirements: 5.1, 5.2, 5.3, 5.4_

  - [ ]* 5.3 编写数组和对象操作属性测试
    - **Property 9: Array Operations Correctness**
    - **Property 10: Object Operations Correctness**
    - **Property 11: Type Mismatch Error Handling**
    - **Validates: Requirements 4.1-4.8, 5.1-5.4**

- [x] 6. 原子操作
  - [x] 6.1 实现数值原子操作
    - 实现 increment, multiply
    - 实现类型检查（非数值返回 TYPE_MISMATCH）
    - _Requirements: 6.1, 6.2, 6.5_

  - [x] 6.2 实现字符串操作
    - 实现 stringAppend, stringPrepend
    - 实现类型检查（非字符串返回 TYPE_MISMATCH）
    - _Requirements: 6.3, 6.4, 6.6_

  - [ ]* 6.3 编写原子操作属性测试
    - **Property 12: Atomic Numeric Operations**
    - **Property 13: String Concatenation Operations**
    - **Validates: Requirements 6.1, 6.2, 6.3, 6.4**

- [x] 7. 监听系统
  - [x] 7.1 实现 Watcher 机制
    - 定义 Watcher 结构和 StateCallback 类型
    - 实现 watch 方法（返回唯一 watch_id）
    - 实现 unwatch 方法
    - 在 processQueue 后触发回调
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

  - [x] 7.2 实现批量模式
    - 实现 batchBegin, batchEnd
    - 在批量模式下累积变更，延迟通知
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_

  - [ ]* 7.3 编写监听系统属性测试
    - **Property 14: Watcher Notification**
    - **Property 15: Batch Mode Deferred Notification**
    - **Validates: Requirements 7.1, 7.2, 7.3, 8.3, 8.4**

- [x] 8. Checkpoint - StateManager 完整验证
  - 确保所有测试通过，ask the user if questions arise.

- [x] 9. C API 封装
  - [x] 9.1 创建 C API 头文件
    - 创建 `core/api/lightui.h`
    - 定义所有 C API 函数签名
    - 定义类型和错误码
    - _Requirements: 9.1, 9.2, 9.3_

  - [x] 9.2 实现 C API
    - 创建 `core/api/lightui.cpp`
    - 实现生命周期函数 (init, cleanup, version)
    - 实现窗口管理函数 (create, destroy, run, stop)
    - 实现状态管理函数（封装 StateManager）
    - 实现 lightui_free, lightui_last_error
    - _Requirements: 9.4, 9.5, 9.6_

  - [ ]* 9.3 编写 C API 单元测试
    - 测试所有 C API 函数
    - 测试错误码返回
    - _Requirements: 9.1-9.6_

- [x] 10. JS Host Bridge
  - [x] 10.1 创建 HostBridge 类
    - 创建 `core/bridge/host_bridge.h`
    - 创建 `core/bridge/host_bridge.cpp`
    - 实现 registerGlobal 注册 host 对象
    - _Requirements: 10.1_

  - [x] 10.2 实现 host.call
    - 实现 bind/unbind 方法
    - 实现 jsCall 静态方法
    - 支持 JSON 参数传递和返回
    - _Requirements: 10.1, 10.5_

  - [x] 10.3 实现 host.state
    - 实现 jsStateGet, jsStateSet, jsStateWatch
    - 连接到 StateManager
    - _Requirements: 10.2, 10.3, 10.4, 10.6_

  - [ ]* 10.4 编写 Host Bridge 集成测试
    - 测试 JS 调用宿主函数
    - 测试 JS 读写共享状态
    - 测试状态变化通知
    - _Requirements: 10.1-10.6_

- [x] 11. useSharedState Hook
  - [x] 11.1 实现 useSharedState Hook
    - 创建 `js/hooks/use_shared_state.js`
    - 实现 [value, setter] 返回
    - 实现自动订阅/取消订阅
    - 实现状态变化触发重渲染
    - _Requirements: 11.1, 11.2, 11.3, 11.4, 11.5_

  - [ ]* 11.2 编写 useSharedState 测试
    - 测试 Hook 基本功能
    - 测试多组件共享状态
    - 测试组件卸载时取消订阅
    - _Requirements: 11.1-11.5_

- [x] 12. Final Checkpoint - 端到端验证
  - 确保所有测试通过
  - 验证 C++ ↔ JS 双向同步
  - ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- 使用 rapidcheck 作为 C++ 属性测试框架
- 测试文件位置：`tests/cpp/state_manager_test.cpp`
- 每个属性测试至少 100 次迭代
