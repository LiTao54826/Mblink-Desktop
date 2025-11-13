# MBink 内存安全审查报告

> **创建日期**: 2025-11-13  
> **审查范围**: JavaScript绑定层、事件系统、Preact渲染器  
> **优先级**: P0 (紧急)  
> **状态**: 进行中 🔄

---

## 🎯 审查目标

在继续开发前，全面审查和修复JavaScript绑定层的内存泄漏和崩溃问题，确保核心基础设施稳定可靠。

### 已知问题
- ⚠️ JavaScript操作渲染时出现内存泄漏导致崩溃闪退
- ⚠️ 事件监听器可能未正确清理
- ⚠️ JSValue生命周期管理不明确

---

## 📋 审查清单

### ✅ 审查完成
- [ ] JavaScript绑定层 (0%)
- [ ] 事件系统 (0%)
- [ ] Preact渲染器 (0%)
- [ ] 内存泄漏检测工具 (0%)

---

## 🔍 Task 0.1: JavaScript绑定层内存泄漏审查

### 审查范围
1. **DOM绑定** (`core/dom/dom_bindings.cpp`)
2. **QuickJS Runtime** (`core/quickjs/quickjs_runtime.cpp`)
3. **Preact绑定** (`core/quickjs/preact_bindings.cpp`)
4. **Window绑定** (`core/quickjs/window_bindings.cpp`)

---

### 🔴 关键问题 1: addEventListener 的 JSValue 泄漏

**位置**: `core/dom/dom_bindings.cpp:234-273`

**问题代码**:
```cpp
// Element.addEventListener(type, listener)
static JSValue js_element_add_event_listener(JSContext* ctx, JSValueConst this_val, 
                                             int argc, JSValueConst* argv) {
    // ...
    
    // 保存 listener 的引用
    JSValue listener = JS_DupValue(ctx, argv[1]);  // ✅ 增加引用计数
    
    // 创建 C++ lambda 包装 JS 函数
    element->AddEventListener(type, [ctx, listener](std::shared_ptr<Event> event) {
        JSValue event_obj = DOMBindings::WrapEvent(ctx, event);
        JSValue ret = JS_Call(ctx, listener, JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx, event_obj);  // ✅ 释放event_obj
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);  // ✅ 释放返回值
    });
    
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}
```

**问题分析**:
1. ❌ **严重内存泄漏**: `listener` 被 `JS_DupValue` 增加引用计数，但**永远不会被释放**
2. ❌ **Lambda捕获问题**: Lambda捕获了 `listener`，但Element析构时不会调用 `JS_FreeValue(ctx, listener)`
3. ❌ **无法移除监听器**: 没有实现 `removeEventListener`，无法手动清理

**调用栈分析**:
```
JavaScript: element.addEventListener('click', handler)
    ↓
js_element_add_event_listener (dom_bindings.cpp:235)
    ↓ JS_DupValue(ctx, argv[1])  // 引用计数 +1
    ↓
Element::AddEventListener (element.cpp:285)
    ↓ 存储 lambda [ctx, listener]
    ↓
event_listeners_[type].emplace_back(...)
    ↓
【问题】: listener 的 JSValue 永远不会被 JS_FreeValue 释放
```

**影响**:
- 每次调用 `addEventListener` 都会泄漏一个 JSValue
- 长时间运行的应用会不断积累泄漏
- 最终导致内存耗尽和崩溃

**修复方案**:
```cpp
// 方案1: 使用 shared_ptr 管理 JSValue 生命周期
struct JSValueWrapper {
    JSContext* ctx;
    JSValue value;
    
    JSValueWrapper(JSContext* c, JSValue v) : ctx(c), value(JS_DupValue(c, v)) {}
    ~JSValueWrapper() { JS_FreeValue(ctx, value); }
    
    JSValueWrapper(const JSValueWrapper&) = delete;
    JSValueWrapper& operator=(const JSValueWrapper&) = delete;
};

// 修改后的代码
static JSValue js_element_add_event_listener(JSContext* ctx, JSValueConst this_val, 
                                             int argc, JSValueConst* argv) {
    // ...
    
    // 使用 shared_ptr 管理 JSValue 生命周期
    auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[1]);
    
    element->AddEventListener(type, [ctx, listener_wrapper](std::shared_ptr<Event> event) {
        JSValue event_obj = DOMBindings::WrapEvent(ctx, event);
        JSValue ret = JS_Call(ctx, listener_wrapper->value, JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx, event_obj);
        if (JS_IsException(ret)) {
            js_std_dump_error(ctx);
        }
        JS_FreeValue(ctx, ret);
    });
    // listener_wrapper 会在 lambda 销毁时自动释放
    
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}
```

---

### 🔴 关键问题 2: Element/Text/Document Finalizer 可能的双重释放

**位置**: `core/dom/dom_bindings.cpp:308-313, 368-373`

**问题代码**:
```cpp
// Element finalizer
static void js_element_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque(val, DOMBindings::element_class_id));
    if (ptr) {
        delete ptr;  // ⚠️ 删除 shared_ptr 指针
    }
}

// Text finalizer
static void js_text_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Text>*>(JS_GetOpaque(val, DOMBindings::text_class_id));
    if (ptr) {
        delete ptr;  // ⚠️ 删除 shared_ptr 指针
    }
}
```

**问题分析**:
1. ✅ **基本正确**: 使用 `delete` 删除 `new` 创建的 `shared_ptr*`
2. ⚠️ **潜在问题**: 如果同一个 C++ 对象被包装多次，可能导致 shared_ptr 引用计数混乱
3. ⚠️ **缺少空指针检查**: 虽然有 `if (ptr)`，但没有检查 `shared_ptr` 本身是否有效

**调用栈分析**:
```
JavaScript GC 触发
    ↓
QuickJS 调用 finalizer
    ↓
js_element_finalizer (dom_bindings.cpp:308)
    ↓
delete ptr;  // 删除 shared_ptr*
    ↓
shared_ptr 析构
    ↓
如果引用计数为0，删除 Element 对象
```

**潜在风险**:
```cpp
// 场景1: 同一个Element被包装两次
auto element = std::make_shared<Element>("div");
JSValue obj1 = DOMBindings::WrapElement(ctx, element);  // new shared_ptr(element)
JSValue obj2 = DOMBindings::WrapElement(ctx, element);  // new shared_ptr(element)

// GC时:
// obj1 finalizer: delete ptr1 -> element引用计数 -1
// obj2 finalizer: delete ptr2 -> element引用计数 -1
// ✅ 正确: element 会在引用计数为0时删除
```

**结论**: 当前实现基本正确，但需要确保不会重复包装同一对象。

---

### 🔴 关键问题 3: Timer 回调的 JSValue 泄漏

**位置**: `core/quickjs/quickjs_runtime.cpp:611-638`

**问题代码**:
```cpp
JSValue QuickJSRuntime::ClearTimer(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    // ...
    
    auto it = runtime->active_timers_.find(timer_id);
    if (it != runtime->active_timers_.end()) {
        // Free callback and arguments
        JS_FreeValue(ctx, it->second.callback);  // ✅ 释放callback
        for (auto& arg : it->second.args) {
            JS_FreeValue(ctx, arg);  // ✅ 释放参数
        }
        runtime->active_timers_.erase(it);
    }
    
    return JS_UNDEFINED;
}
```

**问题分析**:
1. ✅ **clearTimeout/clearInterval 正确**: 会释放 callback 和 args
2. ❌ **setTimeout 执行后未清理**: 需要检查 `ProcessTasks` 是否释放

**需要检查**: `ProcessTasks` 方法

**检查结果** (`core/quickjs/quickjs_runtime.cpp:641-666`):
```cpp
void QuickJSRuntime::ProcessTasks() {
    while (!task_queue_.empty()) {
        Task task = task_queue_.front();
        task_queue_.pop();

        if (!task.cancelled) {
            // Call the callback
            JSValue result = JS_Call(ctx_, task.callback, JS_UNDEFINED,
                                    task.args.size(), task.args.data());
            // ...
            JS_FreeValue(ctx_, result);
        }

        // Free callback and arguments
        JS_FreeValue(ctx_, task.callback);  // ✅ 正确释放
        for (auto& arg : task.args) {
            JS_FreeValue(ctx_, arg);  // ✅ 正确释放
        }
    }
}
```

**结论**: ✅ **ProcessTasks 正确释放了 callback 和 args**

**但是**: ⚠️ **Timer 处理有问题** (`quickjs_runtime.cpp:711-751`):
```cpp
// 处理过期的timer
while (!timer_queue_.empty()) {
    Task task = timer_queue_.top();
    timer_queue_.pop();

    auto it = active_timers_.find(task.id);
    if (it == active_timers_.end()) {
        continue;  // ❌ 问题：从timer_queue_弹出的task没有释放JSValue
    }

    Task& active_task = it->second;

    // 执行回调
    JSValue result = JS_Call(ctx_, active_task.callback, ...);
    JS_FreeValue(ctx_, result);

    if (active_task.repeat) {
        active_task.execute_time = now + active_task.interval;
        timer_queue_.push(active_task);  // ⚠️ 复制Task，但JSValue没有DupValue
    } else {
        // 一次性timer，清理
        JS_FreeValue(ctx_, active_task.callback);  // ✅ 正确释放
        for (auto& arg : active_task.args) {
            JS_FreeValue(ctx_, arg);  // ✅ 正确释放
        }
        active_timers_.erase(it);
    }
}
```

**问题分析**:
1. ❌ **timer_queue_ 中的 Task 副本未释放**: `timer_queue_.pop()` 弹出的 task 是副本，其 JSValue 成员没有被释放
2. ⚠️ **setInterval 的 Task 复制问题**: `timer_queue_.push(active_task)` 复制 Task，但没有 `JS_DupValue`

**修复方案**:
```cpp
// 方案1: Task 使用 shared_ptr 管理 JSValue
// 方案2: timer_queue_ 存储 Task ID 而不是 Task 副本
// 方案3: 实现 Task 的拷贝构造函数，自动 DupValue
```

---

### 🟡 关键问题 4: WrapElement/WrapText/WrapDocument 重复包装

**位置**: `core/dom/dom_bindings.cpp:574-620`

**问题代码**:
```cpp
JSValue DOMBindings::WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    if (!element) {
        return JS_NULL;
    }

    JSValue obj = JS_NewObjectClass(ctx, element_class_id);
    if (JS_IsException(obj)) {
        return obj;
    }

    auto ptr = new std::shared_ptr<Element>(element);  // ⚠️ 每次都创建新的 shared_ptr*
    JS_SetOpaque(obj, ptr);

    return obj;
}
```

**问题分析**:
1. ⚠️ **重复包装**: 每次调用都创建新的 JSValue 对象
2. ⚠️ **无缓存机制**: 同一个 Element 可能有多个 JSValue 表示
3. ⚠️ **引用计数增加**: 每次包装都会增加 shared_ptr 引用计数

**影响**:
```javascript
// JavaScript 代码
const div = document.createElement('div');
const child1 = div.appendChild(someElement);  // WrapElement 调用1
const child2 = div.firstChild;                // WrapElement 调用2
// child1 和 child2 是不同的 JSValue，但指向同一个 C++ Element
```

**修复建议**:
- 实现对象缓存机制（Element → JSValue 映射）
- 或者接受当前行为（多个 JSValue 指向同一个 C++ 对象是安全的）

---

## 🔍 Task 0.2: 事件系统内存安全审查

### 审查范围
1. **事件监听器注册/移除** (`core/dom/element.cpp`)
2. **事件对象生命周期** (`core/dom/event.cpp`)
3. **事件分发** (`core/event/event_system.cpp`)

---

### 🔴 关键问题 5: PreactRenderer 事件监听器泄漏

**位置**: `core/quickjs/preact_renderer.cpp:237-272`

**问题代码**:
```cpp
void PreactRenderer::AddEventListener(std::shared_ptr<Element> element,
                                      const std::string& event_name,
                                      JSValue handler) {
    if (!JS_IsFunction(ctx_, handler)) {
        return;
    }

    // 保存handler引用（防止GC）
    event_handlers_[element].push_back(JS_DupValue(ctx_, handler));  // ✅ DupValue

    // 创建C++事件监听器
    auto listener = [this, handler](std::shared_ptr<Event> event) {  // ⚠️ 捕获handler
        // 创建简单的事件对象
        JSValue event_obj = JS_NewObject(ctx_);
        // ...
        JSValue result = JS_Call(ctx_, handler, JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx_, event_obj);
        // ...
        JS_FreeValue(ctx_, result);
    };

    element->AddEventListener(event_name, listener);  // ❌ 问题：lambda捕获了handler
}
```

**问题分析**:
1. ✅ **event_handlers_ 正确管理**: `JS_DupValue` 增加引用计数，析构函数会释放
2. ❌ **Lambda 捕获问题**: Lambda 按值捕获 `handler`，但这个 JSValue 没有被 DupValue
3. ❌ **双重引用**: `event_handlers_[element]` 和 lambda 都持有 handler，但只有前者增加了引用计数

**调用栈分析**:
```
PreactRenderer::AddEventListener
    ↓
event_handlers_[element].push_back(JS_DupValue(ctx_, handler))  // 引用计数 +1
    ↓
Lambda [this, handler] 捕获 handler  // ⚠️ 按值捕获，没有增加引用计数
    ↓
Element::AddEventListener(event_name, listener)
    ↓
【问题】: Lambda 中的 handler 可能在 event_handlers_ 清理后变成悬空指针
```

**潜在崩溃场景**:
```cpp
// 场景1: Element 被删除，但事件监听器还在
auto element = document->CreateElement("button");
renderer->AddEventListener(element, "click", handler);
// ...
element = nullptr;  // Element 被删除
// event_handlers_[element] 被清理，handler 被 JS_FreeValue
// 但 Element 的 event_listeners_ 中的 lambda 还持有 handler
// 下次点击事件触发时，lambda 使用悬空的 handler -> 崩溃！
```

**修复方案**:
```cpp
// 方案1: Lambda 捕获 shared_ptr<JSValueWrapper>
void PreactRenderer::AddEventListener(std::shared_ptr<Element> element,
                                      const std::string& event_name,
                                      JSValue handler) {
    if (!JS_IsFunction(ctx_, handler)) {
        return;
    }

    // 使用 shared_ptr 管理 JSValue 生命周期
    auto handler_wrapper = std::make_shared<JSValueWrapper>(ctx_, handler);
    event_handlers_[element].push_back(handler_wrapper);

    // Lambda 捕获 shared_ptr
    auto listener = [this, handler_wrapper](std::shared_ptr<Event> event) {
        JSValue event_obj = JS_NewObject(ctx_);
        // ...
        JSValue result = JS_Call(ctx_, handler_wrapper->value, JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx_, event_obj);
        // ...
        JS_FreeValue(ctx_, result);
    };

    element->AddEventListener(event_name, listener);
}
```

---

### ✅ 正确实现: PreactRenderer 析构函数

**位置**: `core/quickjs/preact_renderer.cpp:23-35`

```cpp
PreactRenderer::~PreactRenderer() {
    // 清理事件处理器
    for (auto& [element, handlers] : event_handlers_) {
        for (auto& handler : handlers) {
            JS_FreeValue(ctx_, handler);  // ✅ 正确释放
        }
    }
    event_handlers_.clear();

    if (!JS_IsUndefined(current_component_)) {
        JS_FreeValue(ctx_, current_component_);  // ✅ 正确释放
    }
}
```

**结论**: ✅ 析构函数正确释放了所有 JSValue

---

### 待审查项目
- [x] PreactRenderer::AddEventListener 内存管理 - **发现严重问题**
- [ ] Element::AddEventListener 内存管理
- [ ] Element::RemoveEventListener 是否正确清理
- [ ] Event 对象的 shared_ptr 引用
- [ ] 事件监听器的循环引用检测

---

## 🔍 Task 0.3: Preact渲染器内存安全审查

### 审查范围
1. **PreactRenderer** (`core/quickjs/preact_renderer.cpp`)
2. **PreactBindings** (`core/quickjs/preact_bindings.cpp`)
3. **Virtual DOM 到 Real DOM 映射**

### 待审查项目
- [ ] VNode 的 JSValue 生命周期
- [ ] 组件卸载时的清理
- [ ] Props 和 Children 的内存管理

---

## 🔍 Task 0.4: 内存泄漏检测工具和测试

### 计划创建的工具
1. **内存泄漏检测测试** (`tests/test_memory_leak.cpp`)
2. **压力测试** (`tests/test_stress.cpp`)
3. **长时间运行测试** (`tests/test_long_running.cpp`)

---

## 📊 审查进度

| 任务 | 状态 | 发现问题 | 已修复 |
|------|------|---------|--------|
| Task 0.1: JavaScript绑定层 | 🔄 进行中 | 4 | 0 |
| Task 0.2: 事件系统 | ⏳ 待开始 | - | - |
| Task 0.3: Preact渲染器 | ⏳ 待开始 | - | - |
| Task 0.4: 检测工具 | ⏳ 待开始 | - | - |

---

**下一步**: 继续深入检查 QuickJS Runtime 的 Timer 实现

