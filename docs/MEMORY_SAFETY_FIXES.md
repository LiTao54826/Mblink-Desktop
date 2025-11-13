# MBink 内存安全修复计划

> **创建日期**: 2025-11-13  
> **优先级**: P0 (紧急 - 阻塞所有开发)  
> **预计时间**: 3-5天

---

## 🚨 发现的严重问题总结

### 问题严重性分级
- 🔴 **P0 严重**: 必定导致内存泄漏或崩溃
- 🟡 **P1 重要**: 可能导致内存泄漏
- 🟢 **P2 优化**: 性能或代码质量问题

---

## 🔴 P0 严重问题

### 问题 1: addEventListener 的 JSValue 永久泄漏

**文件**: `core/dom/dom_bindings.cpp:234-273`  
**严重性**: 🔴 P0 - 每次调用都泄漏  
**影响**: 长时间运行必定崩溃

**问题**:
```cpp
JSValue listener = JS_DupValue(ctx, argv[1]);  // 引用计数 +1
element->AddEventListener(type, [ctx, listener](std::shared_ptr<Event> event) {
    // 使用 listener
});
// ❌ listener 永远不会被 JS_FreeValue 释放
```

**修复方案**:
1. 创建 `JSValueWrapper` 辅助类管理 JSValue 生命周期
2. 使用 `shared_ptr<JSValueWrapper>` 在 lambda 中捕获
3. 实现 `removeEventListener` 支持手动清理

**预计工作量**: 4小时

---

### 问题 2: PreactRenderer 事件监听器悬空指针

**文件**: `core/quickjs/preact_renderer.cpp:237-272`  
**严重性**: 🔴 P0 - 可能导致崩溃  
**影响**: Element 删除后触发事件会崩溃

**问题**:
```cpp
event_handlers_[element].push_back(JS_DupValue(ctx_, handler));  // 引用计数 +1
auto listener = [this, handler](std::shared_ptr<Event> event) {  // ❌ 按值捕获，没有DupValue
    JS_Call(ctx_, handler, ...);  // 可能是悬空指针
};
```

**修复方案**:
1. Lambda 捕获 `shared_ptr<JSValueWrapper>` 而不是裸 JSValue
2. 确保 Element 删除时清理事件监听器

**预计工作量**: 3小时

---

### 问题 3: Timer Queue 的 Task 副本泄漏

**文件**: `core/quickjs/quickjs_runtime.cpp:711-751`  
**严重性**: 🔴 P0 - setInterval 必定泄漏  
**影响**: 使用 setInterval 的应用会持续泄漏

**问题**:
```cpp
Task task = timer_queue_.top();  // 复制 Task
timer_queue_.pop();
// ❌ task 的 JSValue 成员没有被释放

if (active_task.repeat) {
    timer_queue_.push(active_task);  // ❌ 复制 Task，但没有 DupValue
}
```

**修复方案**:
1. 实现 Task 的拷贝构造函数，自动 DupValue
2. 实现 Task 的析构函数，自动 FreeValue
3. 或者 timer_queue_ 存储 Task ID 而不是 Task 副本

**预计工作量**: 4小时

---

## 🟡 P1 重要问题

### 问题 4: WrapElement 重复包装

**文件**: `core/dom/dom_bindings.cpp:574-620`  
**严重性**: 🟡 P1 - 增加内存占用  
**影响**: 同一个 Element 可能有多个 JSValue 表示

**问题**:
```cpp
JSValue DOMBindings::WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    // 每次都创建新的 JSValue 对象
    auto ptr = new std::shared_ptr<Element>(element);  // shared_ptr 引用计数 +1
    JS_SetOpaque(obj, ptr);
    return obj;
}
```

**影响**:
- 内存占用增加（每个包装都有开销）
- 可能导致 JavaScript 中的对象比较失败

**修复方案**:
1. 实现对象缓存机制（Element → JSValue 映射）
2. 使用 WeakMap 避免阻止 GC

**预计工作量**: 6小时

---

## 🟢 P2 优化问题

### 问题 5: 缺少 removeEventListener 实现

**文件**: `core/dom/dom_bindings.cpp`  
**严重性**: 🟢 P2 - 功能缺失  
**影响**: 无法手动清理事件监听器

**修复方案**:
1. 实现 `js_element_remove_event_listener`
2. 返回 listener ID 给 JavaScript
3. 支持通过 ID 移除监听器

**预计工作量**: 3小时

---

## 📋 修复计划

### Phase 1: 创建基础设施 (Day 1, 4小时)

#### Task 1.1: 创建 JSValueWrapper 类
**文件**: `core/quickjs/js_value_wrapper.h/cpp`

```cpp
// js_value_wrapper.h
#pragma once
#include "quickjs.h"
#include <memory>

namespace lightui {

/**
 * @brief RAII 包装器，自动管理 JSValue 生命周期
 */
class JSValueWrapper {
public:
    JSValueWrapper(JSContext* ctx, JSValue value);
    ~JSValueWrapper();
    
    // 禁止拷贝
    JSValueWrapper(const JSValueWrapper&) = delete;
    JSValueWrapper& operator=(const JSValueWrapper&) = delete;
    
    // 允许移动
    JSValueWrapper(JSValueWrapper&& other) noexcept;
    JSValueWrapper& operator=(JSValueWrapper&& other) noexcept;
    
    JSValue Get() const { return value_; }
    JSContext* GetContext() const { return ctx_; }
    
private:
    JSContext* ctx_;
    JSValue value_;
};

} // namespace lightui
```

**验收标准**:
- ✅ 构造函数调用 `JS_DupValue`
- ✅ 析构函数调用 `JS_FreeValue`
- ✅ 移动语义正确实现
- ✅ 单元测试通过

---

#### Task 1.2: 改进 Task 结构
**文件**: `core/quickjs/quickjs_runtime.h`

```cpp
struct Task {
    int id;
    std::shared_ptr<JSValueWrapper> callback;  // 使用 shared_ptr 管理
    std::vector<std::shared_ptr<JSValueWrapper>> args;
    int64_t execute_time;
    bool repeat;
    int64_t interval;
    bool cancelled;
    
    // 拷贝构造函数（自动管理引用计数）
    Task(const Task& other);
    Task& operator=(const Task& other);
    
    bool operator>(const Task& other) const {
        return execute_time > other.execute_time;
    }
};
```

**验收标准**:
- ✅ Task 可以安全拷贝
- ✅ JSValue 引用计数正确管理
- ✅ 单元测试通过

---

### Phase 2: 修复 P0 问题 (Day 2-3, 11小时)

#### Task 2.1: 修复 addEventListener 泄漏 (4小时)
**文件**: `core/dom/dom_bindings.cpp`

**步骤**:
1. 修改 `js_element_add_event_listener` 使用 `JSValueWrapper`
2. 返回 listener ID 给 JavaScript
3. 存储 listener ID 到 Element 的映射

**测试**:
```cpp
// tests/test_memory_leak_addEventListener.cpp
TEST(MemoryLeakTest, AddEventListenerNoLeak) {
    auto runtime = std::make_shared<QuickJSRuntime>();
    auto doc = std::make_shared<Document>();
    
    // 添加 1000 个事件监听器
    for (int i = 0; i < 1000; i++) {
        runtime->Eval(R"(
            const div = document.createElement('div');
            div.addEventListener('click', function() {
                console.log('clicked');
            });
        )");
    }
    
    // 强制 GC
    runtime->RunGC();
    
    // 检查内存使用（应该稳定）
    size_t memory_before = GetMemoryUsage();
    runtime->RunGC();
    size_t memory_after = GetMemoryUsage();
    
    EXPECT_LT(memory_after - memory_before, 1024);  // 增长 < 1KB
}
```

---

#### Task 2.2: 修复 PreactRenderer 事件泄漏 (3小时)
**文件**: `core/quickjs/preact_renderer.cpp`

**步骤**:
1. 修改 `event_handlers_` 存储 `shared_ptr<JSValueWrapper>`
2. Lambda 捕获 `shared_ptr<JSValueWrapper>`
3. 添加 Element 删除时的清理逻辑

**测试**:
```cpp
TEST(MemoryLeakTest, PreactEventListenerNoLeak) {
    // 创建和删除 1000 个带事件监听器的元素
    for (int i = 0; i < 1000; i++) {
        runtime->Eval(R"(
            const div = document.createElement('div');
            renderer.addEventHandler(div, 'click', () => {});
            // div 超出作用域，应该被 GC
        )");
    }
    
    runtime->RunGC();
    
    // 检查内存稳定
    EXPECT_LT(GetMemoryUsage(), initial_memory + 10 * 1024);
}
```

---

#### Task 2.3: 修复 Timer Queue 泄漏 (4小时)
**文件**: `core/quickjs/quickjs_runtime.cpp`

**步骤**:
1. 修改 Task 使用 `shared_ptr<JSValueWrapper>`
2. 实现 Task 的拷贝构造函数
3. 修复 `ProcessTasks` 和 timer 处理逻辑

**测试**:
```cpp
TEST(MemoryLeakTest, SetIntervalNoLeak) {
    runtime->Eval(R"(
        let count = 0;
        const id = setInterval(() => {
            count++;
            if (count >= 100) {
                clearInterval(id);
            }
        }, 10);
    )");
    
    // 运行事件循环直到 interval 清除
    runtime->RunEventLoop(200);
    
    runtime->RunGC();
    
    // 检查内存稳定
    EXPECT_LT(GetMemoryUsage(), initial_memory + 5 * 1024);
}
```

---

### Phase 3: 修复 P1 问题 (Day 4, 6小时)

#### Task 3.1: 实现对象缓存机制 (6小时)
**文件**: `core/dom/dom_bindings.cpp`

**步骤**:
1. 创建 `Element* → JSValue` 的 WeakMap
2. WrapElement 时先查缓存
3. Finalizer 时从缓存移除

**注意**: 需要处理循环引用问题

---

### Phase 4: 实现 P2 功能 (Day 5, 3小时)

#### Task 4.1: 实现 removeEventListener (3小时)
**文件**: `core/dom/dom_bindings.cpp`

---

### Phase 5: 测试和验证 (Day 5, 4小时)

#### Task 5.1: 创建内存泄漏检测测试套件
**文件**: `tests/test_memory_leak.cpp`

**测试场景**:
1. 大量 addEventListener/removeEventListener
2. 大量 setTimeout/setInterval/clearTimeout
3. 大量 DOM 创建/删除
4. Preact 组件创建/销毁
5. 长时间运行测试（1小时）

#### Task 5.2: 压力测试
**文件**: `tests/test_stress.cpp`

**测试场景**:
1. 10000 个事件监听器
2. 1000 个并发 timer
3. 深度嵌套的 DOM 树（1000层）
4. 大量 Preact 组件（1000个）

---

## 📊 时间估算

| Phase | 任务 | 时间 |
|-------|------|------|
| Phase 1 | 基础设施 | 4小时 |
| Phase 2 | P0 问题修复 | 11小时 |
| Phase 3 | P1 问题修复 | 6小时 |
| Phase 4 | P2 功能实现 | 3小时 |
| Phase 5 | 测试验证 | 4小时 |
| **总计** | | **28小时 (3.5天)** |

加上缓冲时间，预计 **4-5天** 完成所有修复。

---

## ✅ 验收标准

### 功能验收
- [ ] 所有 P0 问题修复完成
- [ ] 所有 P1 问题修复完成
- [ ] removeEventListener 实现完成

### 测试验收
- [ ] 内存泄漏测试全部通过
- [ ] 压力测试全部通过
- [ ] 长时间运行测试（1小时）无泄漏
- [ ] 所有现有测试仍然通过

### 性能验收
- [ ] 内存使用稳定（长时间运行增长 < 10MB）
- [ ] 无崩溃或段错误
- [ ] 性能无明显下降（< 5%）

---

## 🚀 执行顺序

1. **Day 1 上午**: Task 1.1 - 创建 JSValueWrapper
2. **Day 1 下午**: Task 1.2 - 改进 Task 结构
3. **Day 2 上午**: Task 2.1 - 修复 addEventListener
4. **Day 2 下午**: Task 2.2 - 修复 PreactRenderer
5. **Day 3 上午**: Task 2.3 - 修复 Timer Queue
6. **Day 3 下午**: 测试 P0 修复
7. **Day 4**: Task 3.1 - 对象缓存机制
8. **Day 5 上午**: Task 4.1 - removeEventListener
9. **Day 5 下午**: Task 5.1-5.2 - 完整测试

---

**下一步**: 开始 Phase 1 - 创建 JSValueWrapper 类

