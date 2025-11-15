# QuickJS 缓存机制设计问题全面分析

## 问题现象

**崩溃位置**: `quickjs.c:2197` - `assert(list_empty(&rt->gc_obj_list))`

**崩溃时机**: 
1. 添加第一个 TODO 项成功
2. 在输入框中按任意键时崩溃
3. 崩溃发生在 QuickJS Runtime 销毁时

**错误信息**: `Assertion failed: list_empty(&rt->gc_obj_list)`

这个断言表示：**在 QuickJS Runtime 销毁时，GC 对象列表不为空，说明有 JSValue 对象没有被正确释放**。

---

## 当前缓存机制设计

### 数据结构

```cpp
// dom_bindings.h (line 167-169)
static std::unordered_map<Element*, std::pair<JSContext*, JSValue>> element_cache_;
static std::unordered_map<Text*, std::pair<JSContext*, JSValue>> text_cache_;
static std::unordered_map<Document*, std::pair<JSContext*, JSValue>> document_cache_;
```

**缓存目的**: 防止同一个 C++ 对象被包装成多个不同的 JSValue，确保 JavaScript 中对同一个 DOM 元素的引用是同一个对象。

### 当前实现（有 Bug）

#### 1. WrapElement (line 1730-1762)

```cpp
JSValue DOMBindings::WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    Element* raw_ptr = element.get();
    auto it = element_cache_.find(raw_ptr);
    if (it != element_cache_.end()) {
        // 缓存命中，返回已有的JSValue
        return JS_DupValue(ctx, it->second.second);  // ✅ 正确：增加引用计数
    }

    // 创建新的JSValue
    JSValue obj = JS_NewObjectClass(ctx, element_class_id);
    auto ptr = new std::shared_ptr<Element>(element);
    JS_SetOpaque(obj, ptr);

    // 添加到缓存
    element_cache_[raw_ptr] = std::make_pair(ctx, JS_DupValue(ctx, obj));  // ❌ 问题：缓存持有引用

    return obj;  // 返回原始对象（引用计数=1）
}
```

**引用计数分析**:
- `JS_NewObjectClass` 创建对象，引用计数 = 1
- `JS_DupValue(ctx, obj)` 增加引用计数，引用计数 = 2
- 缓存持有引用计数 = 1
- 返回给 JavaScript 的引用计数 = 1
- **总引用计数 = 2**

#### 2. Finalizer (line 41-48)

```cpp
static void js_element_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque(val, DOMBindings::element_class_id));
    if (ptr) {
        DOMBindings::RemoveFromElementCache(ptr->get());  // ❌ 问题：需要 JSContext
        delete ptr;
    }
}
```

**问题**: Finalizer 只提供 `JSRuntime*`，但 `RemoveFromElementCache` 需要调用 `JS_FreeValue(ctx, ...)`，而 `JS_FreeValue` 需要 `JSContext*`。

#### 3. RemoveFromElementCache (line 1891-1905)

```cpp
void DOMBindings::RemoveFromElementCache(Element* ptr) {
    auto it = element_cache_.find(ptr);
    if (it != element_cache_.end()) {
        JSContext* ctx = it->second.first;
        JSValue cached_val = it->second.second;
        JS_FreeValue(ctx, cached_val);  // ❌ 问题：在 finalizer 中调用时，ctx 可能已失效
        element_cache_.erase(it);
    }
}
```

**问题**: 
1. 在 finalizer 中调用时，存储的 `JSContext*` 可能已经失效
2. 在 Runtime 销毁时，JSContext 可能已经被销毁，导致 `JS_FreeValue` 访问无效内存

---

## 根本问题：循环引用和引用计数不匹配

### 问题 1: 缓存持有引用导致对象无法被 GC

**场景**:
1. `WrapElement` 创建 JSValue，引用计数 = 2（缓存 1 + JavaScript 1）
2. JavaScript 不再引用该对象，引用计数 = 1（只剩缓存）
3. QuickJS GC 发现引用计数 > 0，不会回收对象
4. Finalizer 不会被调用
5. 缓存永远不会被清理
6. **内存泄漏**

### 问题 2: innerHTML 清理缓存时机错误

**场景**:
1. `renderTodos()` 调用 `innerHTML = html`
2. `js_element_set_inner_html` 在设置前清理旧子节点的缓存
3. 清理缓存时调用 `JS_FreeValue`，减少引用计数
4. 但是 **JavaScript 可能还持有对这些元素的引用**（例如事件监听器）
5. 引用计数变为 0，对象被释放
6. JavaScript 访问已释放的对象 → **崩溃**

### 问题 3: Finalizer 无法访问 JSContext

**QuickJS 设计**:
- Finalizer 签名: `void (*finalizer)(JSRuntime *rt, JSValue val)`
- 只提供 `JSRuntime*`，不提供 `JSContext*`
- 这是因为 finalizer 可能在任何 context 的 GC 中被调用

**我们的问题**:
- 缓存存储了 `JSContext*`，但在 finalizer 中无法安全使用
- 在 Runtime 销毁时，所有 JSContext 可能已经被销毁

---

## 正确的设计方案

### 方案 1: 弱引用缓存（推荐）

**核心思想**: 缓存不持有 JSValue 的引用计数，只是一个查找表。

#### 实现

```cpp
// WrapElement
JSValue DOMBindings::WrapElement(JSContext* ctx, std::shared_ptr<Element> element) {
    Element* raw_ptr = element.get();
    auto it = element_cache_.find(raw_ptr);
    if (it != element_cache_.end()) {
        // 缓存命中，返回已有的JSValue
        return JS_DupValue(ctx, it->second.second);  // ✅ 增加引用计数
    }

    // 创建新的JSValue
    JSValue obj = JS_NewObjectClass(ctx, element_class_id);
    auto ptr = new std::shared_ptr<Element>(element);
    JS_SetOpaque(obj, ptr);

    // 添加到缓存（不DupValue，弱引用）
    element_cache_[raw_ptr] = std::make_pair(ctx, obj);  // ✅ 不增加引用计数

    return obj;  // 引用计数 = 1
}

// Finalizer
static void js_element_finalizer(JSRuntime* rt, JSValue val) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque(val, DOMBindings::element_class_id));
    if (ptr) {
        // 直接从缓存移除，不调用 JS_FreeValue
        Element* raw_ptr = ptr->get();
        element_cache_.erase(raw_ptr);  // ✅ 简单移除
        delete ptr;
    }
}

// RemoveFromElementCache（用于 innerHTML 清理）
void DOMBindings::RemoveFromElementCache(Element* ptr) {
    element_cache_.erase(ptr);  // ✅ 简单移除，不调用 JS_FreeValue
}
```

**优点**:
- ✅ 缓存不持有引用，对象可以被正常 GC
- ✅ Finalizer 不需要 JSContext，可以安全清理
- ✅ innerHTML 清理缓存时不会影响引用计数

**缺点**:
- ⚠️ 缓存中可能存在"悬空指针"（指向已被 GC 的 JSValue）
- ⚠️ 需要在缓存命中时检查 JSValue 是否仍然有效

#### 改进：添加有效性检查

QuickJS 没有提供直接的 API 来检查 JSValue 是否有效，但我们可以通过其他方式：

**方案 1A: 依赖 Finalizer 清理**
- Finalizer 被调用时，从缓存中移除
- 缓存命中时，假设 JSValue 仍然有效（因为 finalizer 会清理）
- **问题**: 如果 finalizer 没有被调用（例如 Runtime 销毁时），缓存会有悬空指针

**方案 1B: 在 Cleanup 时清理所有缓存**
- 在 `DOMBindings::Cleanup` 中清理所有缓存
- 不调用 `JS_FreeValue`，因为 Runtime 即将销毁
- **推荐**: 这是最安全的方案

---

### 方案 2: 使用 QuickJS 的 gc_mark 回调

**核心思想**: 让 QuickJS 知道缓存中的引用关系。

```cpp
static void js_element_gc_mark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    auto ptr = static_cast<std::shared_ptr<Element>*>(JS_GetOpaque(val, DOMBindings::element_class_id));
    if (ptr && *ptr) {
        // 标记子节点
        for (const auto& child : (*ptr)->GetChildNodes()) {
            if (auto elem = std::dynamic_pointer_cast<Element>(child)) {
                auto it = element_cache_.find(elem.get());
                if (it != element_cache_.end()) {
                    JS_MarkValue(rt, it->second.second, mark_func);
                }
            }
        }
    }
}
```

**优点**:
- ✅ QuickJS 可以正确追踪引用关系
- ✅ 防止子节点被过早 GC

**缺点**:
- ❌ 复杂度高
- ❌ 需要维护完整的引用图
- ❌ 性能开销大

---

## 推荐方案：弱引用缓存 + Cleanup 清理

### 修改清单

1. **WrapElement/WrapText/WrapDocument**: 缓存时不使用 `JS_DupValue`
2. **Finalizer**: 直接 `erase` 缓存，不调用 `JS_FreeValue`
3. **RemoveFromElementCache**: 直接 `erase` 缓存，不调用 `JS_FreeValue`
4. **Cleanup**: 清理所有缓存，不调用 `JS_FreeValue`（因为 Runtime 即将销毁）

### 关键点

- ✅ 缓存是弱引用，不影响 GC
- ✅ Finalizer 可以安全清理缓存
- ✅ innerHTML 清理缓存不会影响引用计数
- ✅ Cleanup 时不需要 `JS_FreeValue`

---

## 连锁反应分析

### 影响的代码

1. **dom_bindings.cpp**:
   - `WrapElement` (line 1730)
   - `WrapText` (line 1764)
   - `WrapDocument` (line 1793)
   - `RemoveFromElementCache` (line 1891)
   - `RemoveFromTextCache` (line 1907)
   - `RemoveFromDocumentCache` (line 1919)
   - `Cleanup` (line 1694)

2. **Finalizers**:
   - `js_element_finalizer` (line 41)
   - `js_text_finalizer` (需要检查)
   - `js_document_finalizer` (需要检查)

3. **innerHTML 清理逻辑**:
   - `js_element_set_inner_html` (line 802)

### 测试场景

1. ✅ 创建元素，JavaScript 引用，然后释放 → 应该被 GC
2. ✅ innerHTML 替换子节点 → 旧节点应该被 GC
3. ✅ 多次添加 TODO 项 → 不应该崩溃
4. ✅ 程序退出 → 不应该有内存泄漏或断言失败

---

## 下一步行动

1. 修改 `WrapElement/WrapText/WrapDocument`，移除 `JS_DupValue`
2. 修改 `RemoveFromElementCache` 等函数，移除 `JS_FreeValue`
3. 修改 `Cleanup`，移除 `JS_FreeValue`
4. 检查所有 finalizers，确保只调用 `erase`
5. 重新编译测试
6. 验证多次添加 TODO 项不崩溃
7. 验证程序退出不崩溃

