# DOM API 性能分析和优化指南

**日期**: 2025-11-09
**版本**: Phase 2.2

---

## 📊 性能基准测试结果

### 节点创建性能

| 操作 | 数量 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| 创建 Element | 10,000 | 2,996 | 299.6 |
| 创建 Text 节点 | 10,000 | 2,274 | 227.4 |

**分析**: 
- Element 创建速度约为 **3.3M ops/sec**
- Text 节点创建速度约为 **4.4M ops/sec**
- Text 节点创建比 Element 快约 24%（因为 Element 需要初始化更多数据结构）

---

### DOM 树构建性能

| 操作 | 规模 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| 深度树 | 100 层 | 173 | 1,730 |
| 宽度树 | 1,000 子节点 | 602 | 602 |
| 复杂网格 | 100x10 | 999 | 999 |

**分析**:
- 深度树构建非常快（100 层仅需 173μs）
- 宽度树构建也很高效（1,000 个子节点仅需 602μs）
- 复杂网格（1,000 个节点）构建仅需 1ms

---

### 属性操作性能

| 操作 | 数量 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| SetAttribute | 10,000 | 5,211 | 521.1 |
| GetAttribute | 10,000 | 2,232 | 223.2 |

**分析**:
- SetAttribute 速度约为 **1.9M ops/sec**
- GetAttribute 速度约为 **4.5M ops/sec**
- 读取比写入快约 2.3 倍（符合预期）
- 使用 `std::unordered_map` 提供了良好的性能

---

### 查询性能

| 操作 | 数量 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| GetElementById | 1,000 | 146 | 146 |
| QuerySelector (ID) | 100 | 1,944 | 19,440 |
| QuerySelector (class) | 100 | 320 | 3,200 |
| QuerySelectorAll | 100 items | 65 | 650 |

**分析**:
- **GetElementById 非常快**（146ns/op），因为使用了 hash map 缓存
- QuerySelector (ID) 比 GetElementById 慢约 **133 倍**（因为需要遍历树）
- QuerySelector (class) 比 ID 查询快约 6 倍（因为类选择器匹配更简单）
- QuerySelectorAll 收集 100 个元素仅需 65μs

**优化建议**:
- ✅ **已优化**: ID 查询使用 hash map 缓存
- ⚠️ **可优化**: QuerySelector 可以检测 ID 选择器并使用 GetElementById
- ⚠️ **可优化**: 可以缓存常用的查询结果

---

### 事件系统性能

| 操作 | 数量 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| AddEventListener | 1,000 | 163 | 163 |
| DispatchEvent | 1,000 | 686 | 686 |
| Event Bubbling (深度 10) | 100 | 379 | 3,790 |

**分析**:
- AddEventListener 速度约为 **6.1M ops/sec**（非常快）
- DispatchEvent 速度约为 **1.5M ops/sec**
- 事件冒泡（深度 10）每次约 3.8μs（包含 10 个监听器调用）

**优化建议**:
- ✅ **已优化**: 使用 `std::unordered_map<std::string, std::vector<EventListener>>` 存储监听器
- ✅ **已优化**: 事件类型作为 key，避免遍历所有监听器

---

### 克隆性能

| 操作 | 规模 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| 浅克隆 | 1,000 | 683 | 683 |
| 深克隆 | 100 个节点树 | 13,181 | 131,810 |

**分析**:
- 浅克隆速度约为 **1.5M ops/sec**
- 深克隆 100 个节点的树需要 131μs（约 1.3μs/节点）

---

### innerHTML 性能

| 操作 | 规模 | 时间 (μs) | 平均时间 (ns/op) |
|------|------|-----------|------------------|
| GetInnerHTML | 100 个节点树 | 8,727 | 87,270 |

**分析**:
- 生成 100 个节点的 HTML 字符串需要 87μs（约 870ns/节点）
- 主要开销在字符串拼接

**优化建议**:
- ⚠️ **可优化**: 使用 `std::ostringstream` 或预分配字符串缓冲区

---

## 🎯 已实现的优化

### 1. **ID 映射缓存** ✅
- **位置**: `core/dom/document.h`
- **实现**: 使用 `std::unordered_map<std::string, std::weak_ptr<Element>>` 缓存 ID 到元素的映射
- **效果**: GetElementById 查询时间从 O(n) 降低到 O(1)
- **性能提升**: 约 **133 倍**（相比 QuerySelector）

### 2. **事件监听器 Hash Map** ✅
- **位置**: `core/dom/element.h`
- **实现**: 使用 `std::unordered_map<std::string, std::vector<EventListener>>` 存储监听器
- **效果**: 按事件类型分组，避免遍历所有监听器
- **性能提升**: 事件分发时间从 O(n) 降低到 O(k)，其中 k 是该事件类型的监听器数量

### 3. **属性 Hash Map** ✅
- **位置**: `core/dom/element.h`
- **实现**: 使用 `std::unordered_map<std::string, std::string>` 存储属性
- **效果**: 属性查询时间从 O(n) 降低到 O(1)
- **性能提升**: GetAttribute 速度约 **4.5M ops/sec**

### 4. **脏标记优化** ✅
- **位置**: `core/dom/node.h`
- **实现**: 使用 `is_dirty_` 标记跟踪节点变化
- **效果**: 避免不必要的重新布局/渲染
- **性能提升**: 减少不必要的计算

---

## 💡 潜在优化建议

### 1. **QuerySelector ID 优化** ⚠️

**问题**: QuerySelector("#id") 比 GetElementById 慢 133 倍

**解决方案**:
```cpp
std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    // 检测 ID 选择器
    if (selector.size() > 1 && selector[0] == '#') {
        std::string id = selector.substr(1);
        // 使用 Document::GetElementById
        if (auto doc = GetOwnerDocument()) {
            return doc->GetElementById(id);
        }
    }
    
    // 原有的遍历逻辑
    // ...
}
```

**预期效果**: QuerySelector("#id") 性能提升 **100+ 倍**

---

### 2. **innerHTML 字符串优化** ⚠️

**问题**: 字符串拼接开销较大

**解决方案**:
```cpp
std::string Element::GetInnerHTML() const {
    std::ostringstream oss;
    oss.str().reserve(1024);  // 预分配缓冲区
    
    for (const auto& child : child_nodes_) {
        // 使用 ostringstream
        oss << child->ToString();
    }
    
    return oss.str();
}
```

**预期效果**: innerHTML 生成速度提升 **20-30%**

---

### 3. **查询结果缓存** ⚠️

**问题**: 重复查询相同的选择器会重复遍历树

**解决方案**:
```cpp
class Element {
private:
    // 查询缓存
    mutable std::unordered_map<std::string, std::weak_ptr<Element>> query_cache_;
    
public:
    std::shared_ptr<Element> QuerySelector(const std::string& selector) {
        // 检查缓存
        auto it = query_cache_.find(selector);
        if (it != query_cache_.end()) {
            if (auto cached = it->second.lock()) {
                return cached;
            }
        }
        
        // 执行查询
        auto result = QuerySelectorImpl(selector);
        
        // 缓存结果
        if (result) {
            query_cache_[selector] = result;
        }
        
        return result;
    }
    
    void InvalidateQueryCache() {
        query_cache_.clear();
    }
};
```

**注意**: 需要在 DOM 树修改时清除缓存

**预期效果**: 重复查询速度提升 **10-100 倍**

---

### 4. **内存池优化** ⚠️

**问题**: 频繁创建/销毁节点会导致内存碎片

**解决方案**:
```cpp
class NodePool {
public:
    template<typename T, typename... Args>
    std::shared_ptr<T> Allocate(Args&&... args) {
        // 从池中分配
        if (!free_list_.empty()) {
            auto ptr = free_list_.back();
            free_list_.pop_back();
            new (ptr) T(std::forward<Args>(args)...);
            return std::shared_ptr<T>(ptr, [this](T* p) {
                p->~T();
                free_list_.push_back(p);
            });
        }
        
        // 分配新内存
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
private:
    std::vector<void*> free_list_;
};
```

**预期效果**: 节点创建速度提升 **20-50%**，减少内存碎片

---

## 📈 性能总结

### 当前性能等级

| 操作类型 | 性能等级 | 说明 |
|---------|---------|------|
| 节点创建 | ⭐⭐⭐⭐ | 3-4M ops/sec，非常快 |
| 树构建 | ⭐⭐⭐⭐⭐ | 1000 节点仅需 1ms，优秀 |
| 属性操作 | ⭐⭐⭐⭐ | 2-5M ops/sec，很快 |
| ID 查询 | ⭐⭐⭐⭐⭐ | 146ns/op，极快 |
| 选择器查询 | ⭐⭐⭐ | 可优化空间较大 |
| 事件系统 | ⭐⭐⭐⭐ | 1-6M ops/sec，很快 |
| 克隆 | ⭐⭐⭐⭐ | 1.3μs/节点，快 |
| innerHTML | ⭐⭐⭐ | 870ns/节点，可优化 |

### 总体评价

✅ **优秀**:
- 核心 DOM 操作性能优秀
- 已实现关键优化（ID 缓存、Hash Map）
- 满足大多数实际应用需求

⚠️ **可改进**:
- QuerySelector 可以进一步优化
- innerHTML 生成可以优化
- 可以添加查询缓存

---

## 🚀 使用建议

### 1. **优先使用 GetElementById**
```javascript
// ✅ 推荐（极快）
const elem = document.getElementById('myId');

// ❌ 避免（慢 133 倍）
const elem = document.querySelector('#myId');
```

### 2. **批量操作时减少 DOM 访问**
```javascript
// ❌ 避免
for (let i = 0; i < 1000; i++) {
    const elem = document.createElement('div');
    container.appendChild(elem);  // 每次都触发脏标记
}

// ✅ 推荐
const fragment = document.createDocumentFragment();
for (let i = 0; i < 1000; i++) {
    const elem = document.createElement('div');
    fragment.appendChild(elem);
}
container.appendChild(fragment);  // 只触发一次脏标记
```

### 3. **缓存查询结果**
```javascript
// ❌ 避免
for (let i = 0; i < 100; i++) {
    const elem = document.querySelector('.item');  // 重复查询
    // ...
}

// ✅ 推荐
const elem = document.querySelector('.item');  // 查询一次
for (let i = 0; i < 100; i++) {
    // 使用缓存的结果
    // ...
}
```

### 4. **使用事件委托**
```javascript
// ❌ 避免（为每个元素添加监听器）
const items = document.querySelectorAll('.item');
items.forEach(item => {
    item.addEventListener('click', handler);
});

// ✅ 推荐（只在父元素添加一个监听器）
container.addEventListener('click', (e) => {
    if (e.target.matches('.item')) {
        handler(e);
    }
});
```

---

## 📊 基准测试运行方法

```bash
# 编译基准测试
cmake --build build --target benchmark_dom -j4

# 运行基准测试
./build/bin/benchmark_dom.exe
```

---

## 📝 结论

Phase 2.2 DOM API 的性能已经达到了生产级别：

✅ **核心操作性能优秀**（百万级 ops/sec）
✅ **关键优化已实现**（ID 缓存、Hash Map）
✅ **满足实际应用需求**

未来可以根据实际使用情况进行进一步优化。

