# Design Document: Incremental Layout Optimization

## Overview

本设计文档描述了 LightUI 渲染引擎增量布局优化的技术方案。核心思想是引入**内容版本号机制**，让布局缓存能够自动感知内容变化，从而实现真正的增量布局——只重新计算受影响的节点。

### 设计目标

1. **性能提升**：单节点更新时，布局时间降低 90%+
2. **自动缓存失效**：无需手动清除缓存，版本号自动管理
3. **精确影响范围**：根据容器类型智能判断布局传播范围
4. **向后兼容**：不破坏现有功能和测试

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Window / Renderer                            │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                    NativeLayoutEngine                        │    │
│  │  ┌─────────────────────────────────────────────────────┐    │    │
│  │  │  LayoutNode (per node)                               │    │    │
│  │  │  - content_version: uint64_t                         │    │    │
│  │  │  - cache: Cache (with version checking)              │    │    │
│  │  │  - needs_layout: bool                                │    │    │
│  │  │  - layout_scope: LayoutScope (NEW)                   │    │    │
│  │  └─────────────────────────────────────────────────────┘    │    │
│  │                           │                                  │    │
│  │                           ▼                                  │    │
│  │  ┌─────────────────────────────────────────────────────┐    │    │
│  │  │  Cache (enhanced)                                    │    │    │
│  │  │  - content_version in CacheEntry                     │    │    │
│  │  │  - Get() checks version match                        │    │    │
│  │  │  - Store() saves current version                     │    │    │
│  │  └─────────────────────────────────────────────────────┘    │    │
│  │                           │                                  │    │
│  │                           ▼                                  │    │
│  │  ┌─────────────────────────────────────────────────────┐    │    │
│  │  │  IFCLayout (enhanced)                                │    │    │
│  │  │  - Uses content_version for cache validity           │    │    │
│  │  │  - Per-container independent caching                 │    │    │
│  │  └─────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. ContentVersionManager (新增)

负责管理内容版本号的生成和传播。

```cpp
/**
 * @brief 内容版本号管理器
 * 
 * 使用全局递增计数器生成唯一版本号，
 * 确保任何内容变化都能被检测到。
 */
class ContentVersionManager {
public:
    /// 获取单例实例
    static ContentVersionManager& GetInstance();
    
    /// 生成新的版本号（全局递增）
    uint64_t GenerateVersion();
    
    /// 获取当前版本号（不递增）
    uint64_t GetCurrentVersion() const;
    
private:
    std::atomic<uint64_t> version_counter_{0};
};
```

### 2. LayoutNode (增强)

在现有 LayoutNode 结构中添加版本号和布局范围字段。

```cpp
struct LayoutNode {
    // ... 现有字段 ...
    
    /// 内容版本号 - 内容变化时递增
    uint64_t content_version = 0;
    
    /// 布局范围 - 决定布局变化的传播范围
    enum class LayoutScope {
        SELF_ONLY,      // 只影响自身（固定尺寸容器）
        SUBTREE,        // 影响子树（auto 尺寸容器）
        SIBLINGS,       // 影响兄弟（flex/grid 子元素）
        ANCESTORS       // 影响祖先（尺寸变化向上传播）
    };
    LayoutScope layout_scope = LayoutScope::SUBTREE;
    
    /// 上次布局时的测量宽度（用于检测尺寸变化）
    float last_measured_width = 0.0f;
    float last_measured_height = 0.0f;
};
```

### 3. Cache (增强)

修改缓存结构，添加版本号检查。

```cpp
template<typename T>
struct CacheEntry {
    Size<std::optional<float>> known_dimensions;
    Size<AvailableSpace> available_space;
    uint64_t content_version;  // 新增：内容版本号
    T content;
};

class Cache {
public:
    /// 尝试获取缓存结果（增加版本号参数）
    std::optional<LayoutOutput> Get(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space,
        RunMode run_mode,
        uint64_t content_version  // 新增参数
    ) const;
    
    /// 存储计算结果（增加版本号参数）
    void Store(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space,
        RunMode run_mode,
        uint64_t content_version,  // 新增参数
        const LayoutOutput& layout_output
    );
    
    // ... 其他方法保持不变 ...
};
```

### 4. NativeLayoutEngine (增强)

修改布局引擎的核心方法。

```cpp
class NativeLayoutEngine {
public:
    // ... 现有方法 ...
    
    /// 增量布局（优化版）
    bool ComputeIncrementalLayout(float available_width, float available_height);
    
    /// 更新节点内容版本
    void UpdateContentVersion(RenderObject* render_obj);
    
    /// 判断布局变化的影响范围
    LayoutScope DetermineLayoutScope(LayoutNode* node) const;
    
private:
    /// 内部布局计算（不清除缓存）
    void ComputeLayoutInternal(float available_width, float available_height);
    
    /// 智能脏标记传播
    void PropagateLayoutDirty(NodeId node_id, LayoutScope scope);
};
```

### 5. IFCLayout (增强)

修改 IFC 布局的缓存机制。

```cpp
class IFCLayout {
public:
    // ... 现有方法 ...
    
private:
    struct LayoutCache {
        float available_width = 0.0f;
        uint64_t content_version = 0;  // 替换 content_version 计算方式
        // ... 其他字段 ...
    };
    
    /// 检查缓存有效性（使用版本号）
    bool IsCacheValid(RenderObject* container, float available_width, 
                      uint64_t content_version) const;
};
```

## Data Models

### 版本号更新触发点

| 触发事件 | 版本号更新 | 影响范围 |
|---------|-----------|---------|
| 文本内容变化 | 文本节点 + 祖先 IFC 容器 | 根据容器类型判断 |
| 子节点增删 | 父节点 | SUBTREE 或 ANCESTORS |
| 样式变化（布局相关） | 当前节点 | 根据样式类型判断 |
| 样式变化（仅绘制） | 不更新 | SELF_ONLY |

### 布局范围判断规则

```cpp
LayoutScope DetermineLayoutScope(LayoutNode* node) const {
    if (!node || !node->render_obj) return LayoutScope::SUBTREE;
    
    const auto& style = node->render_obj->GetComputedStyle();
    
    // 绝对/固定定位：只影响自身子树
    if (style.position == "absolute" || style.position == "fixed") {
        return LayoutScope::SELF_ONLY;
    }
    
    // 固定尺寸：只影响子树
    if (HasFixedSize(node)) {
        return LayoutScope::SUBTREE;
    }
    
    // Flex/Grid 子元素：可能影响兄弟
    if (IsFlexOrGridChild(node)) {
        return LayoutScope::SIBLINGS;
    }
    
    // Auto 尺寸：可能影响祖先
    return LayoutScope::ANCESTORS;
}

bool HasFixedSize(LayoutNode* node) const {
    const auto& style = node->style;
    bool width_fixed = style.size.width.type == Dimension::Type::Length;
    bool height_fixed = style.size.height.type == Dimension::Type::Length;
    return width_fixed && height_fixed;
}
```


## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 内容版本号递增一致性

*For any* LayoutNode, when its content changes (text, children, or layout-affecting style), the content_version SHALL be strictly greater than its previous value.

**Validates: Requirements 1.1, 1.2, 1.3**

### Property 2: 缓存版本匹配

*For any* cache lookup with matching dimensions and available_space, if the content_version differs from the cached entry's version, the Cache SHALL return a cache miss.

**Validates: Requirements 1.4, 1.5**

### Property 3: 增量布局缓存保留

*For any* incremental layout operation, clean nodes (nodes not marked dirty) SHALL retain their cached layout results and not be recomputed.

**Validates: Requirements 2.1, 2.2, 2.4**

### Property 4: 脏标记向上传播

*For any* node marked as needing layout, all its ancestors up to the root (or until an already-dirty ancestor) SHALL also be marked as needing layout.

**Validates: Requirements 4.1, 4.2**

### Property 5: 固定尺寸容器隔离

*For any* container with fixed width and height, text changes within it SHALL NOT propagate layout dirty marks to its ancestors.

**Validates: Requirements 3.2, 3.1.3**

### Property 6: Auto 尺寸容器传播

*For any* container with auto width, if the measured content width changes after a text update, the layout dirty mark SHALL propagate to ancestors.

**Validates: Requirements 3.3, 3.1.4**

### Property 7: 布局结果等价性

*For any* layout tree, the result of ComputeLayout (full layout) SHALL be identical to the result of ComputeIncrementalLayout when all nodes are marked dirty.

**Validates: Requirements 6.1**

### Property 8: IFC 缓存独立性

*For any* two distinct IFC containers, invalidating one container's cache SHALL NOT affect the other container's cache validity.

**Validates: Requirements 3.4, 3.5**

### Property 9: 增量布局性能

*For any* layout tree with N nodes where only K nodes are dirty (K << N), the number of layout computations SHALL be O(K) rather than O(N).

**Validates: Requirements 5.1, 5.2, 5.4**

## Error Handling

### 版本号溢出

使用 `uint64_t` 存储版本号，理论上可以支持 18 quintillion 次更新。即使每秒更新 1000 次，也需要 5.8 亿年才会溢出。因此不需要特殊处理。

### 缓存不一致

如果检测到缓存状态异常（如版本号回退），应该：
1. 记录警告日志
2. 清除该节点的缓存
3. 强制重新计算

### 循环依赖

布局计算中可能出现循环依赖（如百分比宽度依赖父容器，父容器又依赖子内容）。现有的 Taffy 算法已经处理了这种情况，版本号机制不会引入新的循环问题。

## Testing Strategy

### 单元测试

1. **ContentVersionManager 测试**
   - 版本号递增
   - 多线程安全性

2. **Cache 版本检查测试**
   - 版本匹配时返回缓存
   - 版本不匹配时返回 miss
   - 边界条件（空缓存、版本为 0）

3. **LayoutScope 判断测试**
   - 固定尺寸容器
   - Auto 尺寸容器
   - Flex/Grid 子元素
   - 绝对定位元素

### 属性测试 (Property-Based Testing)

使用 [rapidcheck](https://github.com/emil-e/rapidcheck) 库进行属性测试。

每个属性测试配置运行 100 次迭代。

```cpp
// 示例：Property 1 测试
RC_GTEST_PROP(IncrementalLayout, ContentVersionIncrement, ()) {
    // **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
    // **Validates: Requirements 1.1, 1.2, 1.3**
    
    auto node = createRandomLayoutNode();
    uint64_t old_version = node->content_version;
    
    // 随机触发内容变化
    auto change_type = *rc::gen::element(
        ChangeType::TEXT, ChangeType::CHILDREN, ChangeType::STYLE);
    applyChange(node, change_type);
    
    RC_ASSERT(node->content_version > old_version);
}
```

### 集成测试

1. **时钟更新场景**
   - 验证只有时钟文本节点被重新布局
   - 验证其他节点使用缓存

2. **列表项添加场景**
   - 验证新项触发布局
   - 验证现有项使用缓存

3. **窗口 resize 场景**
   - 验证全量布局正确执行
   - 验证缓存被正确清除

### 性能测试

1. **基准测试**：100 节点树，单节点更新
   - 目标：增量布局时间 < 全量布局时间的 10%

2. **压力测试**：1000 节点树，每秒 60 次更新
   - 目标：CPU 使用率 < 10%

## Implementation Notes

### 阶段 1：基础设施

1. 实现 `ContentVersionManager`
2. 修改 `Cache` 结构添加版本号
3. 修改 `LayoutNode` 添加版本号字段

### 阶段 2：版本号更新

1. 在 `RenderText::SetText()` 中触发版本更新
2. 在 `RenderObject::AddChild/RemoveChild()` 中触发版本更新
3. 在 `UpdateStyle()` 中判断是否需要更新版本

### 阶段 3：增量布局优化

1. 修改 `ComputeIncrementalLayout()` 不调用 `ComputeLayout()`
2. 实现 `ComputeLayoutInternal()` 内部方法
3. 修改 `ComputeNodeLayout()` 传递版本号给缓存

### 阶段 4：IFC 集成

1. 修改 `IFCLayout::IsCacheValid()` 使用版本号
2. 移除 `GetContentVersion()` 中的哈希计算
3. 确保 IFC 缓存独立性

### 阶段 5：测试和验证

1. 编写属性测试
2. 运行现有测试确保兼容性
3. 性能基准测试
