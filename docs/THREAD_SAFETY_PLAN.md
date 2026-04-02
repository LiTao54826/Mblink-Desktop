# MBink 跨线程安全统一架构 — 详细实施计划

> **目标读者**：执行此计划的 AI 编程助手。请严格按照本文档的步骤、代码位置、数据结构执行，不要自由发挥。

---

## ⚠️ 关键约束（开工前必读，每个 Phase 开始前再读一遍）

1. **不要修改 QuickJS 源码**（`third_party/quickjs/`），它是单线程引擎，不可改为多线程
2. **不要改动 `mbink.h` 中的任何函数签名** — ABI 必须 100% 向后兼容
3. **不要合并 SharedObject 和 StateManager** — 它们面向不同场景，保持独立
4. **不要在 binding 层（Python/Go/Rust）实现线程安全** — 一切在 C++ Core 层解决
5. **不要使用 `std::condition_variable`** — event loop 已提供 tick，不需要额外同步
6. **不要给 `JS_SetPropertyStr` / `JS_GetPropertyStr` 外面套 mutex** — 保护不了 GC
7. **所有 QuickJS API 调用（`JS_*` 函数）只允许在主线程执行**，这是不可违反的铁律
8. **不要删除现有文件或大规模重构** — 这是增量改进，不是重写
9. **不要修改 Python binding 层** (`bindings/python/mbink/shared.py`) — 它调用的 C ABI 不变
10. **每完成一个 Phase 就编译验证**，不要积累到最后

---

## 问题根因

`core/api/mbink.cpp` 中的 `mbink_shared_set_*` 系列函数直接调用 `JS_SetPropertyStr`。
当宿主的 async callback 在 `std::thread` 中调用这些函数时，会从非主线程触碰 QuickJS 内部状态。
QuickJS 的 JSValue/GC/ref_count 全部非线程安全，这是未定义行为，会导致随机崩溃。

**受影响的函数**（全部在 `core/api/mbink.cpp` 中）：

| 函数 | 大约行号 | 问题 |
|------|----------|------|
| `mbink_shared_set_int` | 1671 | 直接调 JS_SetPropertyStr |
| `mbink_shared_set_double` | 1687 | 同上 |
| `mbink_shared_set_string` | 1702 | 同上 |
| `mbink_shared_set_bool` | 1718 | 同上 |
| `mbink_shared_set_null` | 1733 | 同上 |
| `mbink_shared_set_json` | 1746 | 同上 + JS_ParseJSON |
| `mbink_shared_get_int` | 1769 | 直接调 JS_GetPropertyStr |
| `mbink_shared_get_double` | 1779 | 同上 |
| `mbink_shared_get_string` | 1789 | 同上 + JS_ToCString |
| `mbink_shared_get_bool` | 1801 | 同上 |
| `mbink_shared_get_json` | 1810 | 同上 + JS_JSONStringify |
| `mbink_shared_get_type` | 1831 | 同上 |
| `mbink_shared_delete` | 1849 | JS_DeleteProperty |
| `mbink_shared_has` | 1859 | JS_HasProperty |
| `mbink_logview_append` | 1900 | 直接调 element->Append |
| `mbink_logview_clear` | 1912 | 直接调 element->Clear |
| `mbink_terminal_write` | 1944 | 直接调 element->Write |
| `mbink_terminal_clear` | 1951 | 直接调 element->Clear |

---


## 方案总览

**"主线程调度队列 + SharedObject 双存储"**

核心思路：
1. 新增 `MainThreadQueue` 类 — 线程安全 MPSC 队列，任意线程 post，主线程 flush
2. SharedObject 增加 `nlohmann::json data_` 成员 — C++ 侧数据副本，所有读操作读它
3. 写操作检测线程：主线程直接执行（零额外开销），非主线程投递到队列
4. LogView/Terminal 同理：非主线程调用时投递到队列

**不改动的东西**：
- `mbink.h` 的所有函数签名
- `StateManager` 类（它已经是线程安全的）
- Python/Go/Rust binding 层代码
- HostBridge 的 async callback 机制
- QuickJS 源码

---

## 实施步骤（共 6 个 Phase，按顺序执行）

---

### Phase 1: 新增 MainThreadQueue

#### 1.1 创建新文件 `core/bridge/main_thread_queue.h`

这是一个 header-only 文件，完整内容如下：

```cpp
#pragma once
/**
 * @file main_thread_queue.h
 * @brief 主线程调度队列 — 线程安全的 MPSC (Multiple Producer, Single Consumer) 队列
 *
 * 任意线程调用 post() 投递操作，主线程在 event loop 中调用 flush() 执行。
 * 复用 HostBridge 已有的 alive flag 模式保证生命周期安全。
 */

#include <mutex>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

namespace mbink {

class MainThreadQueue {
public:
    MainThreadQueue()
        : mainThreadId_(std::this_thread::get_id())
        , alive_(std::make_shared<std::atomic<bool>>(true)) {}

    ~MainThreadQueue() {
        *alive_ = false;
    }

    MainThreadQueue(const MainThreadQueue&) = delete;
    MainThreadQueue& operator=(const MainThreadQueue&) = delete;

    bool isMainThread() const {
        return std::this_thread::get_id() == mainThreadId_;
    }

    void post(std::function<void()> fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(fn));
    }

    void flush() {
        std::vector<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(batch, queue_);
        }
        for (auto& fn : batch) {
            if (alive_->load()) {
                fn();
            }
        }
    }

    std::shared_ptr<std::atomic<bool>> aliveFlag() const {
        return alive_;
    }

private:
    std::thread::id mainThreadId_;
    std::mutex mutex_;
    std::vector<std::function<void()>> queue_;
    std::shared_ptr<std::atomic<bool>> alive_;
};

} // namespace mbink
```

> **注意**：构造函数记录 `std::this_thread::get_id()` 作为主线程 ID。
> `MainThreadQueue` 作为 `WindowContext` 的成员，在 `mbink_create()` 中构造，
> 而 `mbink_create()` 必须从主线程调用，所以主线程 ID 自然正确。

#### 1.2 在 WindowContext 中添加成员

文件：`core/api/mbink.cpp`

**添加 include**（在文件头部第 11 行附近，与其他 bridge include 放一起）：
```cpp
#include "core/bridge/main_thread_queue.h"
```

**在 WindowContext 结构体中添加成员**（第 140-173 行的结构体内）：

在第 150 行 `std::unique_ptr<mbink::StateManager> stateManager;` 之后添加：
```cpp
    // 主线程调度队列 — 所有跨线程操作通过此队列投递到主线程
    mbink::MainThreadQueue mainThreadQueue;
```

#### 1.3 在 event loop flush 链中添加 flush 调用

文件：`core/api/mbink.cpp`

**位置 A**：`mbink_run()` 函数的 update callback（约第 698-717 行）

当前代码：
```cpp
ctx->eventLoop->SetUpdateCallback([ctx](float dt) {
    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    // 刷新所有 SharedObject 的延迟通知
    for (auto& kv : ctx->sharedObjects) {
        kv.second->flushPendingNotify();
    }
    if (ctx->onUpdateCallback) {
        ctx->onUpdateCallback(dt, ctx->onUpdateUserData);
    }
});
```

改为（在 SharedObject flush 之前插入 MainThreadQueue flush）：
```cpp
ctx->eventLoop->SetUpdateCallback([ctx](float dt) {
    if (ctx->stateManager) {
        ctx->stateManager->processQueue();
    }
    if (ctx->hostBridge) {
        ctx->hostBridge->flushEvents();
        ctx->hostBridge->flushAsyncResults();
    }
    // 刷新主线程调度队列（跨线程投递的 SharedObject/LogView/Terminal 操作）
    ctx->mainThreadQueue.flush();
    // 刷新所有 SharedObject 的延迟通知
    for (auto& kv : ctx->sharedObjects) {
        kv.second->flushPendingNotify();
    }
    if (ctx->onUpdateCallback) {
        ctx->onUpdateCallback(dt, ctx->onUpdateUserData);
    }
});
```

**位置 B**：`mbink_poll_events()` 函数（约第 732-753 行）

同样，在 SharedObject flush 之前添加：
```cpp
    // 刷新主线程调度队列
    ctx->mainThreadQueue.flush();
```

> **⚠️ flush 顺序很重要**：MainThreadQueue.flush() 必须在 SharedObject flushPendingNotify() 之前，
> 因为队列中的操作会设置 `pending_notify_ = true`，需要在同一帧被 flushPendingNotify() 处理。

#### 1.4 Phase 1 验证

- 编译通过即可
- 此时 MainThreadQueue 存在但没有任何代码调用 post()，不影响现有行为
- 运行现有测试/示例，确认无回归


---

### Phase 2: SharedObjectData 增加 C++ 侧数据存储

#### 2.1 添加头文件 include

文件：`core/api/mbink.cpp`

在文件头部（约第 11 行附近，和其他 bridge include 放一起）添加：
```cpp
#include <shared_mutex>
```

> **注意**：`nlohmann/json.hpp` 很可能已通过 `state_manager.h` 间接包含。先检查，如果没有再显式 include。

#### 2.2 修改 SharedObjectData 结构体 — 添加新成员

文件：`core/api/mbink.cpp`
位置：第 59-138 行的 `SharedObjectData` 结构体

在第 66 行 `bool pending_notify_ = false;` 之后添加：

```cpp
    // ===== 线程安全数据存储（Phase 2 新增）=====
    mutable std::shared_mutex dataMutex_;                  // 保护 data_
    nlohmann::json data_ = nlohmann::json::object();       // C++ 侧数据副本（任意线程可安全读）
    mbink::MainThreadQueue* mainQueue_ = nullptr;          // 指向 WindowContext 的主线程队列
```

#### 2.3 在 mbink_shared_create 中初始化 mainQueue_

文件：`core/api/mbink.cpp`
位置：`mbink_shared_create` 函数（约第 1620-1646 行）

在第 1630 行 `shared->name = name;` 之后添加：
```cpp
    shared->mainQueue_ = &ctx->mainThreadQueue;
```

#### 2.4 为 SharedObjectData 添加线程安全方法

在 SharedObjectData 结构体最后的 `};` 之前（第 138 行之前），添加以下方法：

> **⚠️ 注意**：以下代码添加到 `flushBatch()` 方法之后、`};` 之前。
> 不要删除或改动已有方法（refreshUpdater, beginBatch, endBatch, notifyUpdate, flushPendingNotify, flushBatch）。

```cpp
    // ===== Phase 2 新增：线程安全操作方法 =====

    // 线程安全写入：更新 json 副本 + 同步到 JS
    void safeSetProperty(const char* key, const nlohmann::json& value) {
        // 1. 立即更新 C++ 侧存储
        {
            std::unique_lock<std::shared_mutex> lock(dataMutex_);
            data_[key] = value;
        }

        // 2. 同步到 JS
        if (mainQueue_ && mainQueue_->isMainThread()) {
            // 主线程：直接执行
            applyPropertyToJS(key, value);
            lazyRefreshUpdater();
            notifyUpdate(key);
        } else if (mainQueue_) {
            // 非主线程：投递到主线程队列
            std::string keyCopy(key);
            nlohmann::json valueCopy = value;
            auto alive = mainQueue_->aliveFlag();
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, valueCopy, alive]() {
                if (!alive->load()) return;
                self->applyPropertyToJS(keyCopy.c_str(), valueCopy);
                self->lazyRefreshUpdater();
                self->notifyUpdate(keyCopy.c_str());
            });
        }
    }

    // 线程安全删除
    void safeDeleteProperty(const char* key) {
        {
            std::unique_lock<std::shared_mutex> lock(dataMutex_);
            data_.erase(key);
        }

        if (mainQueue_ && mainQueue_->isMainThread()) {
            JSAtom atom = JS_NewAtom(ctx, key);
            JS_DeleteProperty(ctx, js_obj, atom, 0);
            JS_FreeAtom(ctx, atom);
            notifyUpdate(key);
        } else if (mainQueue_) {
            std::string keyCopy(key);
            auto alive = mainQueue_->aliveFlag();
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, alive]() {
                if (!alive->load()) return;
                JSAtom atom = JS_NewAtom(self->ctx, keyCopy.c_str());
                JS_DeleteProperty(self->ctx, self->js_obj, atom, 0);
                JS_FreeAtom(self->ctx, atom);
                self->notifyUpdate(keyCopy.c_str());
            });
        }
    }

    // 线程安全读取（从 json 副本读，不碰 QuickJS）
    nlohmann::json safeGetProperty(const char* key) const {
        std::shared_lock<std::shared_mutex> lock(dataMutex_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            return *it;
        }
        return nlohmann::json(nullptr);
    }

    // 线程安全查询是否存在
    bool safeHasProperty(const char* key) const {
        std::shared_lock<std::shared_mutex> lock(dataMutex_);
        return data_.contains(key);
    }

    // 线程安全获取类型
    int safeGetType(const char* key) const {
        std::shared_lock<std::shared_mutex> lock(dataMutex_);
        auto it = data_.find(key);
        if (it == data_.end() || it->is_null()) return 0;  // MBINK_TYPE_NULL
        if (it->is_boolean()) return 1;                     // MBINK_TYPE_BOOL
        if (it->is_number_integer()) return 2;              // MBINK_TYPE_INT
        if (it->is_number_float()) return 3;                // MBINK_TYPE_DOUBLE
        if (it->is_string()) return 4;                      // MBINK_TYPE_STRING
        if (it->is_array()) return 5;                       // MBINK_TYPE_ARRAY
        if (it->is_object()) return 6;                      // MBINK_TYPE_OBJECT
        return 0;
    }

private:
    // ===== 以下方法只在主线程调用 =====

    // 将 nlohmann::json 写入 QuickJS JSValue
    void applyPropertyToJS(const char* key, const nlohmann::json& value) {
        if (!ctx) return;
        JSValue jsVal;
        if (value.is_null()) {
            jsVal = JS_NULL;
        } else if (value.is_boolean()) {
            jsVal = JS_NewBool(ctx, value.get<bool>());
        } else if (value.is_number_integer()) {
            jsVal = JS_NewInt64(ctx, value.get<int64_t>());
        } else if (value.is_number_float()) {
            jsVal = JS_NewFloat64(ctx, value.get<double>());
        } else if (value.is_string()) {
            jsVal = JS_NewString(ctx, value.get<std::string>().c_str());
        } else {
            // array / object → dump 成 JSON 字符串 → JS_ParseJSON
            auto s = value.dump();
            jsVal = JS_ParseJSON(ctx, s.c_str(), s.size(), "<shared>");
            if (JS_IsException(jsVal)) {
                JSValue exc = JS_GetException(ctx);
                JS_FreeValue(ctx, exc);
                return;
            }
        }
        JS_SetPropertyStr(ctx, js_obj, key, jsVal);
    }

    // 懒刷新 updater 函数缓存
    void lazyRefreshUpdater() {
        if (JS_IsUndefined(updater_func)) {
            JSValue global = JS_GetGlobalObject(ctx);
            updater_func = JS_GetPropertyStr(ctx, global, "__onSharedUpdate");
            JS_FreeValue(ctx, global);
        }
    }
```

> **⚠️ 重要**：
> - `private:` 访问控制标记会影响后面的 `flushBatch()` 等已有方法。你需要确保已有方法在 `private:` 之前。
>   正确的做法是把新增方法中的 `private:` 部分（`applyPropertyToJS` 和 `lazyRefreshUpdater`）放在结构体最后。
>   已有的 `refreshUpdater`、`beginBatch`、`endBatch`、`notifyUpdate`、`flushPendingNotify`、`flushBatch` 保持在前面。
> - 如果 struct 原来没有 `private:`/`public:` 标记，你需要在已有方法前面加 `public:` 保持它们可见。
>   或者直接不用 `private:`，在 `applyPropertyToJS` 和 `lazyRefreshUpdater` 前加注释说明"仅限主线程调用"即可。
>   **推荐后者**：SharedObjectData 是内部结构体，不需要严格的访问控制。去掉 `private:`，改用注释。

#### 2.5 Phase 2 验证

- 编译通过
- 此时新方法存在但尚未被外部函数调用，现有行为不变
- 运行示例确认无回归


---

### Phase 3: 改写所有 mbink_shared_set_* 和 mbink_shared_get_* 函数

> **核心原则**：所有函数改为调用 Phase 2 添加的 `safeSetProperty` / `safeGetProperty` 等方法。
> 不再直接调用任何 `JS_*` 函数。ABI 签名不变。

#### 3.1 改写 Setter 函数

文件：`core/api/mbink.cpp`

**`mbink_shared_set_int`**（约第 1671 行）

当前代码：
```cpp
int mbink_shared_set_int(MBinkSharedHandle shared_handle,
                            const char* key, int64_t value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    JS_SetPropertyStr(shared->ctx, shared->js_obj, key, JS_NewInt64(shared->ctx, value));
    if (JS_IsUndefined(shared->updater_func)) {
        JSValue global = JS_GetGlobalObject(shared->ctx);
        shared->updater_func = JS_GetPropertyStr(shared->ctx, global, "__onSharedUpdate");
        JS_FreeValue(shared->ctx, global);
    }
    shared->notifyUpdate(key);
    return MBINK_OK;
}
```

替换为：
```cpp
int mbink_shared_set_int(MBinkSharedHandle shared_handle,
                            const char* key, int64_t value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}
```

**`mbink_shared_set_double`**（约第 1687 行）— 同样模式：
```cpp
int mbink_shared_set_double(MBinkSharedHandle shared_handle,
                               const char* key, double value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}
```

**`mbink_shared_set_string`**（约第 1702 行）：
```cpp
int mbink_shared_set_string(MBinkSharedHandle shared_handle,
                               const char* key, const char* value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (value) {
        shared->safeSetProperty(key, nlohmann::json(std::string(value)));
    } else {
        shared->safeSetProperty(key, nlohmann::json(nullptr));
    }
    return MBINK_OK;
}
```

**`mbink_shared_set_bool`**（约第 1718 行）：
```cpp
int mbink_shared_set_bool(MBinkSharedHandle shared_handle,
                             const char* key, bool value) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(value));
    return MBINK_OK;
}
```

**`mbink_shared_set_null`**（约第 1733 行）：
```cpp
int mbink_shared_set_null(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeSetProperty(key, nlohmann::json(nullptr));
    return MBINK_OK;
}
```

**`mbink_shared_set_json`**（约第 1746 行）：
```cpp
int mbink_shared_set_json(MBinkSharedHandle shared_handle,
                             const char* key, const char* json_str) {
    if (!shared_handle || !key || !json_str) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto parsed = nlohmann::json::parse(json_str, nullptr, false);
    if (parsed.is_discarded()) {
        return MBINK_ERROR_INVALID_PARAM;
    }
    shared->safeSetProperty(key, parsed);
    return MBINK_OK;
}
```

> **⚠️ set_json 注意事项**：原来用 `JS_ParseJSON` 解析 JSON 字符串，现在改用 `nlohmann::json::parse`。
> 第二个参数 `nullptr` 表示不使用 callback，第三个参数 `false` 表示不抛异常（返回 `discarded` 值表示解析失败）。

#### 3.2 改写 Getter 函数

**`mbink_shared_get_int`**（约第 1769 行）：
```cpp
int64_t mbink_shared_get_int(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number_integer()) return val.get<int64_t>();
    if (val.is_number_float()) return static_cast<int64_t>(val.get<double>());
    return 0;
}
```

**`mbink_shared_get_double`**（约第 1779 行）：
```cpp
double mbink_shared_get_double(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return 0.0;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_number()) return val.get<double>();
    return 0.0;
}
```

**`mbink_shared_get_string`**（约第 1789 行）：
```cpp
const char* mbink_shared_get_string(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_string()) {
        return duplicateString(val.get<std::string>().c_str());
    }
    return nullptr;
}
```

**`mbink_shared_get_bool`**（约第 1801 行）：
```cpp
bool mbink_shared_get_bool(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_boolean()) return val.get<bool>();
    return false;
}
```

**`mbink_shared_get_json`**（约第 1810 行）：
```cpp
const char* mbink_shared_get_json(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return nullptr;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    auto val = shared->safeGetProperty(key);
    if (val.is_null()) return nullptr;
    auto s = val.dump();
    return duplicateString(s.c_str());
}
```


#### 3.3 改写属性查询函数

**`mbink_shared_get_type`**（约第 1831 行）：
```cpp
int mbink_shared_get_type(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_TYPE_NULL;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeGetType(key);
}
```

**`mbink_shared_delete`**（约第 1849 行）：
```cpp
int mbink_shared_delete(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return MBINK_ERROR_INVALID_PARAM;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    shared->safeDeleteProperty(key);
    return MBINK_OK;
}
```

**`mbink_shared_has`**（约第 1859 行）：
```cpp
bool mbink_shared_has(MBinkSharedHandle shared_handle, const char* key) {
    if (!shared_handle || !key) return false;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    return shared->safeHasProperty(key);
}
```

#### 3.4 Batch 操作的线程安全处理

**`mbink_shared_batch_begin`** 和 **`mbink_shared_batch_end`**（约第 1870-1880 行）

batch 操作的语义是"抑制中间通知，批量结束时统一通知"。
`batch_depth` 和 `pending_updates` 只在主线程使用（因为它们控制的是 JS 通知时机）。

对于非主线程调用 batch：
- 写操作本身通过 `safeSetProperty` 已经线程安全
- `batch_depth++/--` 和 `flushBatch` 涉及到 `pending_notify_` 标记，而 `pending_notify_` 会被主线程 `flushPendingNotify()` 读取

**推荐方案**：batch begin/end 投递到主线程执行。

```cpp
void mbink_shared_batch_begin(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (shared->mainQueue_ && shared->mainQueue_->isMainThread()) {
        shared->beginBatch();
    } else if (shared->mainQueue_) {
        auto alive = shared->mainQueue_->aliveFlag();
        SharedObjectData* self = shared;
        shared->mainQueue_->post([self, alive]() {
            if (!alive->load()) return;
            self->beginBatch();
        });
    }
}

void mbink_shared_batch_end(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);
    if (shared->mainQueue_ && shared->mainQueue_->isMainThread()) {
        shared->endBatch();
    } else if (shared->mainQueue_) {
        auto alive = shared->mainQueue_->aliveFlag();
        SharedObjectData* self = shared;
        shared->mainQueue_->post([self, alive]() {
            if (!alive->load()) return;
            self->endBatch();
        });
    }
}
```

> **⚠️ 注意跨线程 batch 的行为差异**：
> 从非主线程调用 `batch_begin` → N 次 `set` → `batch_end` 时：
> - `batch_begin` 和 `batch_end` 是异步投递到主线程的
> - 中间的 `set` 操作各自独立投递 lambda
> - 由于 MPSC 队列保序，执行顺序是：`beginBatch → set1 → set2 → ... → endBatch`
> - **所以跨线程 batch 仍然能正确抑制中间通知**
> - 但要注意：写入 json 副本是立即的（`safeSetProperty` 中第一步），JS 同步和 batch 管理是在主线程 flush 时按序执行的

#### 3.5 Phase 3 验证

- 编译通过
- **这是关键 Phase**：此后所有 SharedObject 的读写都经过线程安全路径
- 测试要点：
  1. 主线程 set → get：应该和以前完全一样
  2. 主线程 set → JS 能看到更新
  3. set_json 解析失败应返回 MBINK_ERROR_INVALID_PARAM
  4. batch begin → 多次 set → batch end → 只触发一次通知
- 此时还没有跨线程测试场景，但主线程路径必须无回归


---

### Phase 4: LogView/Terminal 线程安全

#### 4.1 修改 LogViewHandleData 和 TerminalHandleData

文件：`core/api/mbink.cpp`
位置：第 175-181 行

当前代码：
```cpp
struct LogViewHandleData {
    std::shared_ptr<mbink::HTMLLogViewElement> element;
};

struct TerminalHandleData {
    std::shared_ptr<mbink::HTMLTerminalElement> element;
};
```

替换为：
```cpp
struct LogViewHandleData {
    std::shared_ptr<mbink::HTMLLogViewElement> element;
    mbink::MainThreadQueue* mainQueue = nullptr;  // Phase 4 新增
};

struct TerminalHandleData {
    std::shared_ptr<mbink::HTMLTerminalElement> element;
    mbink::MainThreadQueue* mainQueue = nullptr;  // Phase 4 新增
};
```

#### 4.2 在 get 函数中初始化 mainQueue

**`mbink_logview_get`**（约第 1882 行）

在 `auto* data = new LogViewHandleData();` 之后、`return` 之前添加：
```cpp
    data->mainQueue = &ctx->mainThreadQueue;
```

**`mbink_terminal_get`**（约第 1926 行）

同样，在 `auto* data = new TerminalHandleData();` 之后添加：
```cpp
    data->mainQueue = &ctx->mainThreadQueue;
```

#### 4.3 改写 LogView 操作函数

**`mbink_logview_append`**（约第 1900 行）：

当前：
```cpp
int mbink_logview_append(MBinkLogViewHandle logview_handle,
                         const char* level,
                         const char* source,
                         const char* message) {
    if (!logview_handle || !level || !source || !message) {
        return MBINK_ERROR_INVALID_PARAM;
    }
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    data->element->Append(level, source, message);
    return MBINK_OK;
}
```

替换为：
```cpp
int mbink_logview_append(MBinkLogViewHandle logview_handle,
                         const char* level,
                         const char* source,
                         const char* message) {
    if (!logview_handle || !level || !source || !message) {
        return MBINK_ERROR_INVALID_PARAM;
    }
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Append(level, source, message);
    } else if (data->mainQueue) {
        // 非主线程：复制参数，投递到主线程
        std::string l(level), s(source), m(message);
        auto elem = data->element;  // shared_ptr，安全拷贝
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, l, s, m, alive]() {
            if (!alive->load()) return;
            elem->Append(l.c_str(), s.c_str(), m.c_str());
        });
    }
    return MBINK_OK;
}
```

> **注意**：`data->element` 是 `shared_ptr`，lambda 捕获 `shared_ptr` 副本是安全的。
> 即使 `LogViewHandleData` 被 destroy，`shared_ptr` 引用计数保证 element 存活。

**`mbink_logview_clear`**（约第 1912 行）：

替换为：
```cpp
void mbink_logview_clear(MBinkLogViewHandle logview_handle) {
    if (!logview_handle) return;
    auto* data = reinterpret_cast<LogViewHandleData*>(logview_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Clear();
    } else if (data->mainQueue) {
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, alive]() {
            if (!alive->load()) return;
            elem->Clear();
        });
    }
}
```

#### 4.4 改写 Terminal 操作函数

**`mbink_terminal_write`**（约第 1944 行）：

替换为：
```cpp
int mbink_terminal_write(MBinkTerminalHandle terminal_handle, const char* data_str) {
    if (!terminal_handle || !data_str) return MBINK_ERROR_INVALID_PARAM;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Write(data_str);
    } else if (data->mainQueue) {
        std::string str(data_str);
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, str, alive]() {
            if (!alive->load()) return;
            elem->Write(str.c_str());
        });
    }
    return MBINK_OK;
}
```

**`mbink_terminal_clear`**（约第 1951 行）：

替换为：
```cpp
void mbink_terminal_clear(MBinkTerminalHandle terminal_handle) {
    if (!terminal_handle) return;
    auto* data = reinterpret_cast<TerminalHandleData*>(terminal_handle);
    if (data->mainQueue && data->mainQueue->isMainThread()) {
        data->element->Clear();
    } else if (data->mainQueue) {
        auto elem = data->element;
        auto alive = data->mainQueue->aliveFlag();
        data->mainQueue->post([elem, alive]() {
            if (!alive->load()) return;
            elem->Clear();
        });
    }
}
```

> **⚠️ 其他 Terminal 函数** (`mbink_terminal_execute`, `mbink_terminal_start_shell`, `mbink_terminal_send_input`, `mbink_terminal_resize`, `mbink_terminal_serialize`)：
> - `execute`、`start_shell`、`send_input`、`resize` 也应该做同样的 isMainThread 分支处理
> - `serialize` 返回值需要同步返回，如果从非主线程调用不安全，建议在文档/注释中标注"必须从主线程调用"
> - 或者，如果这些函数在实际使用中不会从非主线程调用，可以暂时不改，只在注释中说明

#### 4.5 Phase 4 验证

- 编译通过
- 从主线程调用 logview_append / terminal_write 应行为不变
- 跨线程调用应不崩溃（操作会延迟到下一帧执行）



---

### Phase 5: 生命周期安全

#### 5.1 问题描述

`mbink_shared_destroy()` 被调用时，可能有已投递但尚未执行的 lambda 仍引用被销毁的 `SharedObjectData*`。
如果 lambda 在 destroy 之后才执行，`self` 指针已是野指针 → 崩溃。

同样的问题存在于 LogView/Terminal 的 destroy 场景。

#### 5.2 SharedObject 生命周期保护

**方案：为每个 SharedObjectData 添加独立的 alive 标志。**

> **为什么不直接 flush？** flush 可以解决"销毁前先执行完队列"的问题，但有两个缺陷：
> 1. `mbink_shared_destroy` 必须在主线程调用（它会调用 `JS_FreeValue`），flush 本身也必须在主线程。
>    虽然可行，但如果未来有人误从非主线程调用 destroy，flush 不会帮上忙。
> 2. flush 会执行队列中**所有**操作（包括其他 SharedObject 的），不够精准。
>
> alive 标志方案更健壮：lambda 执行前检查标志，destroy 只需设标志，无需关心队列状态。

**步骤 A**：在 SharedObjectData 中添加 alive 标志

在 Phase 2 新增的成员之后（`mainQueue_` 之后）添加：
```cpp
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
```

**步骤 B**：修改 safeSetProperty 和 safeDeleteProperty 中的 lambda

将 Phase 2 中 `safeSetProperty` 的非主线程分支改为同时捕获 SharedObject 自己的 alive 标志：

原来：
```cpp
        } else if (mainQueue_) {
            std::string keyCopy(key);
            nlohmann::json valueCopy = value;
            auto alive = mainQueue_->aliveFlag();
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, valueCopy, alive]() {
                if (!alive->load()) return;
                self->applyPropertyToJS(keyCopy.c_str(), valueCopy);
                self->lazyRefreshUpdater();
                self->notifyUpdate(keyCopy.c_str());
            });
        }
```

改为：
```cpp
        } else if (mainQueue_) {
            std::string keyCopy(key);
            nlohmann::json valueCopy = value;
            auto queueAlive = mainQueue_->aliveFlag();  // 窗口级生命周期
            auto objAlive = alive_;                       // 对象级生命周期
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, valueCopy, queueAlive, objAlive]() {
                if (!queueAlive->load() || !objAlive->load()) return;
                self->applyPropertyToJS(keyCopy.c_str(), valueCopy);
                self->lazyRefreshUpdater();
                self->notifyUpdate(keyCopy.c_str());
            });
        }
```

同样修改 `safeDeleteProperty` 的非主线程分支：
```cpp
        } else if (mainQueue_) {
            std::string keyCopy(key);
            auto queueAlive = mainQueue_->aliveFlag();
            auto objAlive = alive_;
            SharedObjectData* self = this;
            mainQueue_->post([self, keyCopy, queueAlive, objAlive]() {
                if (!queueAlive->load() || !objAlive->load()) return;
                JSAtom atom = JS_NewAtom(self->ctx, keyCopy.c_str());
                JS_DeleteProperty(self->ctx, self->js_obj, atom, 0);
                JS_FreeAtom(self->ctx, atom);
                self->notifyUpdate(keyCopy.c_str());
            });
        }
```

同样修改 Phase 3 中 `mbink_shared_batch_begin` 和 `mbink_shared_batch_end` 的 lambda：
```cpp
    // batch_begin 示例
    } else if (shared->mainQueue_) {
        auto queueAlive = shared->mainQueue_->aliveFlag();
        auto objAlive = shared->alive_;
        SharedObjectData* self = shared;
        shared->mainQueue_->post([self, queueAlive, objAlive]() {
            if (!queueAlive->load() || !objAlive->load()) return;
            self->beginBatch();
        });
    }
```

**步骤 C**：修改 `mbink_shared_destroy`

当前代码：
```cpp
void mbink_shared_destroy(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    if (shared->ctx) {
        // 从 globalThis 移除
        JSValue global = JS_GetGlobalObject(shared->ctx);
        JSAtom atom = JS_NewAtom(shared->ctx, shared->name.c_str());
        JS_DeleteProperty(shared->ctx, global, atom, 0);
        JS_FreeAtom(shared->ctx, atom);
        JS_FreeValue(shared->ctx, global);

        // 释放 JS 值
        JS_FreeValue(shared->ctx, shared->js_obj);
        if (!JS_IsUndefined(shared->updater_func)) {
            JS_FreeValue(shared->ctx, shared->updater_func);
        }
    }
    delete shared;
}
```

替换为：
```cpp
void mbink_shared_destroy(MBinkSharedHandle shared_handle) {
    if (!shared_handle) return;
    auto* shared = reinterpret_cast<SharedObjectData*>(shared_handle);

    // 标记对象已死亡 — 队列中的 pending lambda 检查后会跳过
    *(shared->alive_) = false;

    if (shared->ctx) {
        // 从 globalThis 移除
        JSValue global = JS_GetGlobalObject(shared->ctx);
        JSAtom atom = JS_NewAtom(shared->ctx, shared->name.c_str());
        JS_DeleteProperty(shared->ctx, global, atom, 0);
        JS_FreeAtom(shared->ctx, atom);
        JS_FreeValue(shared->ctx, global);

        // 释放 JS 值
        JS_FreeValue(shared->ctx, shared->js_obj);
        if (!JS_IsUndefined(shared->updater_func)) {
            JS_FreeValue(shared->ctx, shared->updater_func);
        }
    }
    delete shared;
}
```

> **⚠️ 重要约束**：`mbink_shared_destroy` **必须从主线程调用**，因为它直接调用 `JS_FreeValue` 等 QuickJS API。
> 这与 `mbink_shared_create` 的约束一致（创建也必须在主线程）。
> 如果有人从非主线程调用 destroy，那是调用方的 bug，不是框架该兜底的。
> 但 alive 标志至少保证了：destroy 后，队列中残留的 lambda 不会访问已释放的内存。



#### 5.3 LogView/Terminal 生命周期保护

LogView 和 Terminal 的情况与 SharedObject 不同：

- Phase 4 中 lambda 捕获的是 `shared_ptr<Element>`，不是裸指针
- 即使 `LogViewHandleData` / `TerminalHandleData` 被 `delete`，element 的 `shared_ptr` 副本仍持有引用计数
- element 本身不会因 handle 销毁而失效（它是 DOM 树的一部分，生命周期由引擎管理）

**所以 LogView/Terminal 不需要额外的 alive 标志。** `shared_ptr` 引用计数 + MainThreadQueue 的 alive 标志已经足够。

但需要注意 `mbink_logview_destroy` 和 `mbink_terminal_destroy` 的当前实现：

查看当前代码，它们应该只是 `delete data`（释放 HandleData 结构体）。
这是安全的：delete 后，队列中的 lambda 持有的是 `shared_ptr<Element>` 副本，不依赖 HandleData。

> **⚠️ 注意**：如果当前 destroy 实现中有调用 element 方法的逻辑（比如 Detach/Cleanup），
> 那也必须确保在主线程调用。检查实际代码确认。

#### 5.4 WindowContext 销毁顺序

`mbink_destroy()` 销毁 WindowContext 时，`MainThreadQueue` 作为成员会在析构函数中设 `*alive_ = false`。
此后即使有残留 lambda（理论上不应该有，因为 event loop 已经停止），也不会执行。

**需要确认的销毁顺序**：
1. Event loop 停止（不再 flush）
2. `MainThreadQueue` 析构 → `*alive_ = false`
3. SharedObjects 销毁
4. HostBridge 销毁
5. Runtime 销毁

> **⚠️ 注意**：当前 `mbink_destroy` 中如果先销毁了 Runtime/JSContext，再执行 MainThreadQueue 中的残留 lambda，
> lambda 会访问已释放的 JSContext → 崩溃。但 `MainThreadQueue` 析构时设了 `alive_ = false`，
> 加上 event loop 已停止不会再 flush，所以这不是问题。
> 关键是 **不要在 `mbink_destroy` 中手动调用 `mainThreadQueue.flush()`**。

#### 5.5 Phase 5 验证

- 编译通过
- 测试场景：创建 SharedObject → 从非主线程 set → 立即 destroy → 不崩溃
- 确认 destroy 后队列中的 lambda 被安静跳过（不执行、不崩溃）


---

### Phase 6: 最终验证与已知限制

#### 6.1 完整验证清单

| # | 验证项 | 预期结果 |
|---|--------|----------|
| 1 | 编译通过，无 warning | ✓ |
| 2 | 主线程 set_int/get_int | 与修改前行为完全一致 |
| 3 | 主线程 set_json → JS 能读到 | ✓ |
| 4 | 主线程 batch begin → N×set → batch end | 只触发一次 JS 通知 |
| 5 | 主线程 logview_append | 立即追加，无延迟 |
| 6 | 主线程 terminal_write | 立即写入 |
| 7 | async callback 中 set_int → 不崩溃 | ✓ 操作被投递到主线程 |
| 8 | async callback 中 get_int → 返回正确值 | ✓ 从 json 副本读 |
| 9 | async callback 中 logview_append → 不崩溃 | ✓ |
| 10 | 多线程并发 set + get 压测 | 不崩溃，数据最终一致 |
| 11 | set_json 传入非法 JSON | 返回 MBINK_ERROR_INVALID_PARAM |
| 12 | destroy SharedObject 后队列中有残留 lambda | lambda 被跳过，不崩溃 |
| 13 | Python binding 无修改，功能正常 | ✓ ABI 兼容 |

#### 6.2 已知限制（必须理解，不是 bug）

1. **跨线程写入有 1 帧延迟**
   - 非主线程调用 `set_int` 后，JS 侧看到的值要等到下一次 event loop flush
   - C++ 侧 json 副本是立即更新的，所以同一线程内 `set → get` 仍然立即可见
   - 这是 MPSC 队列模型的固有特性，不是 bug

2. **跨线程读返回的是 json 快照，不是 live JSValue**
   - `get_string` 返回的是 `nlohmann::json` 中存储的字符串，不是 `JS_ToCString` 的结果
   - 对于基本类型（int/double/bool/string/null），语义完全等价
   - 对于 `get_json`，返回的是 `nlohmann::json::dump()` 而不是 `JS_JSONStringify`
   - 99.9% 的情况输出相同，极端情况下 key 的排列顺序可能不同（nlohmann::json 默认按字典序）

3. **JS 侧直接修改 SharedObject 不会同步到 C++ json 副本**
   - 如果 JS 代码执行 `shared.foo = 123`，C++ 侧 `data_` 不知道这个变更
   - 这是**设计如此**：SharedObject 的写入方向是 宿主→JS，不是双向同步
   - 如果未来需要 JS→宿主同步，那是另一个 feature，不在本次改造范围内

4. **serialize 类函数不支持跨线程调用**
   - `mbink_terminal_serialize` 需要同步返回值，无法走异步队列
   - 必须从主线程调用，否则行为未定义
   - 在注释中标注即可

5. **SharedObject 的 create/destroy 必须在主线程**
   - 它们直接操作 QuickJS（`JS_NewObject`, `JS_FreeValue`）
   - 这与 `mbink_create`/`mbink_destroy` 的约束一致

#### 6.3 性能影响

| 操作 | 修改前 | 修改后 | 差异 |
|------|--------|--------|------|
| 主线程 set | 直接 JS_SetPropertyStr | json赋值 + JS_SetPropertyStr | +1次json赋值（微秒级） |
| 主线程 get | 直接 JS_GetPropertyStr | shared_lock + json读取 | 相当，可能略快（不需要 JS→C 转换） |
| 非主线程 set | ❌ 未定义行为 | json赋值 + lambda入队 | 从"崩溃"变为"正确工作" |
| 非主线程 get | ❌ 未定义行为 | shared_lock + json读取 | 从"崩溃"变为"正确工作" |
| 内存 | 无额外开销 | 每个 SharedObject +1 份 json 副本 | 通常 < 1KB |
| event loop | 无额外 flush | flush 空队列 = 1次 swap 空 vector | 可忽略 |

#### 6.4 绝对不要做的事（最终重申）

1. ❌ 不要修改 `mbink.h` 的函数签名
2. ❌ 不要修改 QuickJS 源码
3. ❌ 不要修改 Python binding
4. ❌ 不要给 `JS_SetPropertyStr` 外面加锁替代本方案
5. ❌ 不要在 `mbink_destroy` 中调用 `mainThreadQueue.flush()`
6. ❌ 不要合并 SharedObject 和 StateManager
7. ❌ 不要在非主线程调用 `mbink_shared_create` / `mbink_shared_destroy`
8. ❌ 不要删除 SharedObjectData 原有的 `refreshUpdater`/`notifyUpdate`/`flushPendingNotify` 方法
9. ❌ 不要让 `safeGetProperty` 走队列（读操作必须同步返回）
10. ❌ 不要引入 `std::condition_variable` 做 wakeup

---

## 修改文件清单

| 文件 | 操作 | Phase |
|------|------|-------|
| `core/bridge/main_thread_queue.h` | **新建** | 1 |
| `core/api/mbink.cpp` | **修改** | 1-5 |

> 只涉及 2 个文件。`mbink.h` 不改。binding 层不改。其他 core 文件不改。

---

## 实施顺序总结

```
Phase 1 → 编译验证 → Phase 2 → 编译验证 → Phase 3 → 编译+功能验证
→ Phase 4 → 编译验证 → Phase 5 → 编译+生命周期验证 → Phase 6（最终验证）
```

每个 Phase 独立可回滚。如果某个 Phase 编译失败，先修好再继续，不要跳过。编译使用cmake --build build --config Release
