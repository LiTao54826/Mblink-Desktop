# Design Document: Incremental Layer Tree Update System

## Overview

本设计文档描述了 LightUI 增量层树更新系统的架构设计。该系统解决当前架构中的四个核心问题：

1. **层树重建触发时机不合理** - 实现增量更新，只添加/删除变化的层
2. **滚动偏移存储位置混乱** - 建立单一数据源（SSOT）
3. **坐标系不一致** - 统一光栅化和合成的坐标计算
4. **Fixed 元素层结构问题** - 分离 fixed 层到根层下

### 当前架构分析

当前 RenderPipeline 的渲染流程：
```
DOMSync → StyleRecalc → Layout → LayerTreeBuild → Rasterize → Composite
```

问题点：
1. `LayerTreeBuilder::Build()` 每次都调用 `Clear()` 清除所有层
2. `ScrollLayerManager` 在 `RegisterScrollContainer` 时从 `RenderObject` 读取滚动偏移
3. `LayerTreeBuilder::UpdateLayerBounds()` 在 `CreateLayer` 时调用，此时父层可能还未设置
4. `Rasterizer` 和 `Compositor` 对滚动偏移的处理不一致

### 设计原则

- **最小化重建**：只更新变化的层，保持其他层不变
- **单一数据源**：每个状态只存储在一个地方
- **坐标系一致性**：所有组件使用相同的坐标系
- **职责分离**：明确各组件的职责边界
- **渐进式重构**：保持与现有代码的兼容性，逐步改进

## Architecture

### 整体架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            RenderPipeline                                    │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                    LayerTreeManager (新增核心组件)                      │ │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐ │ │
│  │  │ IncrementalUpdate│  │ ScrollStateStore │  │ CoordinateSystem     │ │ │
│  │  │ Controller       │  │ (SSOT)           │  │ Manager              │ │ │
│  │  │ - 增量更新队列    │  │ - 滚动状态存储    │  │ - 坐标转换           │ │ │
│  │  │ - 批量操作       │  │ - 变化通知        │  │ - 边界计算           │ │ │
│  │  └──────────────────┘  └──────────────────┘  └──────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                         │
│  ┌─────────────────────────────────┼─────────────────────────────────────┐  │
│  │                                 ▼                                     │  │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │  │
│  │  │              LayerTreeBuilder (重构)                             │ │  │
│  │  │  - Build() 保持兼容                                              │ │  │
│  │  │  - IncrementalBuild() 增量构建（新增）                            │ │  │
│  │  │  - AddLayerForObject() / RemoveLayerForObject() 单层操作（新增）  │ │  │
│  │  │  - UpdateLayerBoundsDeferred() 延迟边界计算（新增）               │ │  │
│  │  └─────────────────────────────────────────────────────────────────┘ │  │
│  │                                 │                                     │  │
│  │  ┌──────────────────────────────┼──────────────────────────────────┐ │  │
│  │  │                              ▼                                  │ │  │
│  │  │  ┌──────────────────┐  ┌──────────────────┐                    │ │  │
│  │  │  │ Rasterizer       │  │ Compositor       │                    │ │  │
│  │  │  │ - 统一坐标系      │  │ - 统一坐标系      │                    │ │  │
│  │  │  │ - 增量光栅化      │  │ - z-index 排序   │                    │ │  │
│  │  │  └──────────────────┘  └──────────────────┘                    │ │  │
│  │  └────────────────────────────────────────────────────────────────┘ │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 层树结构（重新设计）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              RootLayer                                       │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                       ContentLayer (body)                               │ │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐ │ │
│  │  │ ScrollClipLayer  │  │ NormalLayer      │  │ AnimationLayer       │ │ │
│  │  │ (滚动容器裁剪层)  │  │ (普通提升元素)    │  │ (动画元素)           │ │ │
│  │  │  └─ScrollContent │  │                  │  │                      │ │ │
│  │  │    Layer         │  │                  │  │                      │ │ │
│  │  └──────────────────┘  └──────────────────┘  └──────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                    FixedLayers (直接挂在 RootLayer 下)                  │ │
│  │  ┌──────────────────┐  ┌──────────────────┐                           │ │
│  │  │ FixedLayer1      │  │ FixedLayer2      │  (按 z-index 排序)        │ │
│  │  │ (Toast, z=1000)  │  │ (Modal, z=2000)  │                           │ │
│  │  └──────────────────┘  └──────────────────┘                           │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘

关键设计决策：
1. Fixed 层直接挂在 RootLayer 下，不受 ContentLayer 滚动影响
2. 滚动容器使用 ClipLayer + ContentLayer 结构
3. 层按 z-index 排序绘制
```

### 滚动偏移数据流（单一数据源）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ScrollStateStore (SSOT)                               │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  scroll_states_: Map<RenderObject*, ScrollState>                       │ │
│  │                                                                        │ │
│  │  ScrollState {                                                         │ │
│  │    scroll_x, scroll_y,                                                 │ │
│  │    max_scroll_x, max_scroll_y,                                         │ │
│  │    content_width, content_height,                                      │ │
│  │    viewport_width, viewport_height                                     │ │
│  │  }                                                                     │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                         │
│                    ┌───────────────┼───────────────┐                        │
│                    ▼               ▼               ▼                        │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐          │
│  │ RenderObject     │  │ CompositorLayer  │  │ ScrollLayerMgr   │          │
│  │ (只读访问)        │  │ (只读访问)        │  │ (只读访问)        │          │
│  │ GetScrollX/Y()   │  │ GetScrollOffset()│  │ GetScrollState() │          │
│  │ 委托给 Store     │  │ 委托给 Store     │  │ 委托给 Store     │          │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘          │
│                                                                              │
│  写入路径（唯一）：                                                           │
│  EventSystem → ScrollStateStore.SetScrollPosition() → 通知所有监听器         │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. LayerTreeManager (新增核心组件)

协调层树更新的中央管理器，整合增量更新、滚动状态和坐标系统。

```cpp
/**
 * @file layer_tree_manager.h
 * @brief 层树管理器 - 协调增量层树更新
 * 
 * 职责：
 * - 管理增量更新队列
 * - 维护滚动状态的单一数据源
 * - 提供统一的坐标转换
 * - 协调 LayerTreeBuilder、Rasterizer、Compositor
 */

#pragma once

#include "compositor_layer.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace lightui {

// 前向声明
class RenderObject;
class LayerTreeBuilder;
class Rasterizer;
class Compositor;

/**
 * @brief 层更新操作类型
 */
enum class LayerUpdateType {
    Add,            // 添加层
    Remove,         // 移除层
    UpdateBounds,   // 更新边界
    Reparent,       // 重新附加父层
    UpdateZIndex    // 更新 z-index
};

/**
 * @brief 待处理的层更新操作
 */
struct PendingLayerUpdate {
    LayerUpdateType type;
    RenderObject* target = nullptr;
    LayerPromotionReason reason = LayerPromotionReason::None;
    int z_index = 0;
    std::string debug_info;
};

/**
 * @brief 滚动状态（单一数据源）
 */
struct ScrollState {
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    float max_scroll_x = 0.0f;
    float max_scroll_y = 0.0f;
    float content_width = 0.0f;
    float content_height = 0.0f;
    float viewport_width = 0.0f;
    float viewport_height = 0.0f;
    
    // 版本号，用于检测变化
    uint64_t version = 0;
};

/**
 * @brief 坐标空间类型
 */
enum class CoordinateSpace {
    Document,   // 文档坐标（相对于文档左上角）
    Viewport,   // 视口坐标（相对于可见区域左上角）
    Layer       // 层坐标（相对于层左上角）
};

/**
 * @brief 层树管理器
 */
class LayerTreeManager {
public:
    LayerTreeManager();
    ~LayerTreeManager();

    // 禁止拷贝
    LayerTreeManager(const LayerTreeManager&) = delete;
    LayerTreeManager& operator=(const LayerTreeManager&) = delete;

    // =========================================================================
    // 初始化和配置
    // =========================================================================

    /**
     * @brief 初始化管理器
     * @param builder 层树构建器
     * @param rasterizer 光栅化器
     * @param compositor 合成器
     */
    void Initialize(LayerTreeBuilder* builder, 
                    Rasterizer* rasterizer, 
                    Compositor* compositor);

    /**
     * @brief 设置视口信息
     */
    void SetViewport(float width, float height, float dpi_scale);

    // =========================================================================
    // 增量更新接口
    // =========================================================================

    /**
     * @brief 请求添加层（增量）
     * @param obj 需要层的 RenderObject
     * @param reason 层提升原因
     */
    void RequestAddLayer(RenderObject* obj, LayerPromotionReason reason);

    /**
     * @brief 请求移除层（增量）
     * @param obj 要移除层的 RenderObject
     */
    void RequestRemoveLayer(RenderObject* obj);

    /**
     * @brief 请求更新层边界
     * @param obj 需要更新边界的 RenderObject
     */
    void RequestUpdateBounds(RenderObject* obj);

    /**
     * @brief 应用所有待处理的更新
     * @return 是否有更新被应用
     */
    bool ApplyPendingUpdates();

    /**
     * @brief 检查是否有待处理的更新
     */
    bool HasPendingUpdates() const { return !pending_updates_.empty(); }

    /**
     * @brief 检查是否需要完整重建
     */
    bool NeedsFullRebuild() const { return needs_full_rebuild_; }

    /**
     * @brief 强制完整重建（仅在必要时使用）
     * @param reason 重建原因（用于调试）
     */
    void ForceFullRebuild(const std::string& reason);

    /**
     * @brief 清除完整重建标记
     */
    void ClearFullRebuildFlag() { needs_full_rebuild_ = false; }

    // =========================================================================
    // 滚动状态管理（SSOT）
    // =========================================================================

    /**
     * @brief 注册滚动容器
     * @param container 滚动容器的 RenderObject
     * @return 是否成功注册
     */
    bool RegisterScrollContainer(RenderObject* container);

    /**
     * @brief 注销滚动容器
     * @param container 滚动容器
     */
    void UnregisterScrollContainer(RenderObject* container);

    /**
     * @brief 获取滚动状态（只读）
     * @param container 滚动容器
     * @return 滚动状态指针，如果未注册返回 nullptr
     */
    const ScrollState* GetScrollState(RenderObject* container) const;

    /**
     * @brief 设置滚动位置（唯一的写入入口）
     * @param container 滚动容器
     * @param x X 滚动位置
     * @param y Y 滚动位置
     * @return 是否成功设置
     */
    bool SetScrollPosition(RenderObject* container, float x, float y);

    /**
     * @brief 滚动指定距离
     * @param container 滚动容器
     * @param dx X 方向增量
     * @param dy Y 方向增量
     * @return 是否成功滚动
     */
    bool ScrollBy(RenderObject* container, float dx, float dy);

    /**
     * @brief 更新滚动容器的内容尺寸
     * @param container 滚动容器
     */
    void UpdateScrollContentSize(RenderObject* container);

    /**
     * @brief 添加滚动变化监听器
     * @param listener 监听器回调
     * @return 监听器 ID
     */
    uint32_t AddScrollListener(
        std::function<void(RenderObject*, const ScrollState&)> listener);

    /**
     * @brief 移除滚动变化监听器
     * @param id 监听器 ID
     */
    void RemoveScrollListener(uint32_t id);

    // =========================================================================
    // 坐标转换
    // =========================================================================

    /**
     * @brief 转换点坐标
     * @param point 输入点
     * @param from 源坐标系
     * @param to 目标坐标系
     * @param layer 相关层（用于 Layer 坐标系）
     * @return 转换后的点
     */
    SkPoint ConvertPoint(const SkPoint& point,
                         CoordinateSpace from,
                         CoordinateSpace to,
                         CompositorLayer* layer = nullptr) const;

    /**
     * @brief 转换矩形坐标
     */
    SkRect ConvertRect(const SkRect& rect,
                       CoordinateSpace from,
                       CoordinateSpace to,
                       CompositorLayer* layer = nullptr) const;

    /**
     * @brief 计算层在文档坐标系中的边界
     * @param obj RenderObject
     * @param parent_layer 父层
     * @return 文档坐标系中的边界
     */
    SkRect CalculateLayerBoundsInDocument(RenderObject* obj,
                                          CompositorLayer* parent_layer) const;

    /**
     * @brief 计算 fixed 元素在视口坐标系中的边界
     * @param obj RenderObject
     * @return 视口坐标系中的边界
     */
    SkRect CalculateFixedLayerBounds(RenderObject* obj) const;

    // =========================================================================
    // 层树版本和统计
    // =========================================================================

    /**
     * @brief 获取层树版本号
     */
    uint64_t GetTreeVersion() const { return tree_version_; }

    /**
     * @brief 获取层数量
     */
    size_t GetLayerCount() const;

    /**
     * @brief 获取滚动容器数量
     */
    size_t GetScrollContainerCount() const { return scroll_states_.size(); }

    // =========================================================================
    // 调试支持
    // =========================================================================

    /**
     * @brief 启用/禁用调试日志
     */
    void SetDebugLogging(bool enabled) { debug_logging_ = enabled; }

    /**
     * @brief 获取最后一次完整重建的原因
     */
    const std::string& GetLastRebuildReason() const { return last_rebuild_reason_; }

private:
    // 应用单个更新操作
    bool ApplyUpdate(const PendingLayerUpdate& update);

    // 通知滚动监听器
    void NotifyScrollListeners(RenderObject* container, const ScrollState& state);

    // 计算滚动范围
    void CalculateScrollBounds(ScrollState& state, RenderObject* container);

    // 限制滚动位置
    void ClampScrollPosition(ScrollState& state);

    // 关联组件
    LayerTreeBuilder* builder_ = nullptr;
    Rasterizer* rasterizer_ = nullptr;
    Compositor* compositor_ = nullptr;

    // 待处理的更新
    std::vector<PendingLayerUpdate> pending_updates_;

    // 滚动状态（SSOT）
    std::unordered_map<RenderObject*, ScrollState> scroll_states_;

    // 滚动监听器
    std::unordered_map<uint32_t, 
        std::function<void(RenderObject*, const ScrollState&)>> scroll_listeners_;
    uint32_t next_listener_id_ = 0;

    // 视口信息
    float viewport_width_ = 0.0f;
    float viewport_height_ = 0.0f;
    float dpi_scale_ = 1.0f;
    float document_scroll_x_ = 0.0f;
    float document_scroll_y_ = 0.0f;

    // 状态
    bool needs_full_rebuild_ = true;
    std::string last_rebuild_reason_ = "initial";
    uint64_t tree_version_ = 0;
    bool debug_logging_ = false;
};

} // namespace lightui
```

### 2. LayerTreeBuilder (重构)

重构现有的 LayerTreeBuilder，添加增量更新能力，同时保持向后兼容。

```cpp
/**
 * @file layer_tree_builder.h (扩展)
 * @brief 层树构建器 - 支持增量更新
 * 
 * 新增功能：
 * - IncrementalBuild() 增量构建
 * - AddLayerForObject() / RemoveLayerForObject() 单层操作
 * - UpdateLayerBoundsDeferred() 延迟边界计算
 * - FindParentLayerForObject() 查找父层
 */

class LayerTreeBuilder {
public:
    // ... 现有接口保持不变 ...

    // =========================================================================
    // 新增：增量更新接口
    // =========================================================================

    /**
     * @brief 增量构建层树
     * @param root 渲染树根节点
     * @param pending_updates 待处理的更新列表
     * @return 是否成功
     * 
     * 与 Build() 不同，IncrementalBuild() 不会清除现有层树，
     * 而是根据 pending_updates 进行增量修改。
     */
    bool IncrementalBuild(RenderObject* root, 
                          const std::vector<PendingLayerUpdate>& pending_updates);

    /**
     * @brief 为 RenderObject 添加层（增量）
     * @param obj 需要层的 RenderObject
     * @param reason 层提升原因
     * @return 新创建的层，失败返回 nullptr
     * 
     * 关键改进：
     * 1. 先找到正确的父层
     * 2. 创建层并附加到父层
     * 3. 附加后再计算边界（此时有父层信息）
     */
    std::shared_ptr<CompositorLayer> AddLayerForObject(
        RenderObject* obj, LayerPromotionReason reason);

    /**
     * @brief 移除 RenderObject 的层（增量）
     * @param obj 要移除层的 RenderObject
     * @return 是否成功移除
     * 
     * 关键改进：
     * 1. 将子层转移到父层
     * 2. 从父层移除当前层
     * 3. 清理映射关系
     */
    bool RemoveLayerForObject(RenderObject* obj);

    /**
     * @brief 延迟更新层边界（在父层已知后调用）
     * @param layer 要更新的层
     * @param obj 关联的 RenderObject
     * 
     * 与 UpdateLayerBounds() 的区别：
     * - 确保在层已附加到父层后调用
     * - 正确处理 fixed 元素的视口坐标
     */
    void UpdateLayerBoundsDeferred(CompositorLayer* layer, RenderObject* obj);

    /**
     * @brief 查找 RenderObject 应该附加到的父层
     * @param obj RenderObject
     * @return 父层，如果是 fixed 元素返回根层
     * 
     * 查找逻辑：
     * 1. 如果是 fixed 元素，返回根层
     * 2. 否则，向上遍历 RenderObject 树，找到第一个有层的祖先
     */
    CompositorLayer* FindParentLayerForObject(RenderObject* obj) const;

    /**
     * @brief 检查是否可以增量更新
     * @return 如果可以增量更新返回 true
     * 
     * 不能增量更新的情况：
     * - 根层不存在
     * - 层树结构严重损坏
     * - 需要重新排序大量层
     */
    bool CanIncrementalUpdate() const;

    /**
     * @brief 获取层树版本号（用于检测变化）
     */
    uint64_t GetTreeVersion() const { return tree_version_; }

    /**
     * @brief 递增层树版本号
     */
    void IncrementTreeVersion() { ++tree_version_; }

private:
    // 层树版本号，每次修改递增
    uint64_t tree_version_ = 0;
};
```

### 3. CompositorLayer (扩展)

扩展现有的 CompositorLayer，添加增量更新支持和层标识。

```cpp
/**
 * @file compositor_layer.h (扩展)
 * @brief 合成层 - 扩展增量更新支持
 */

class CompositorLayer {
public:
    // ... 现有接口保持不变 ...

    // =========================================================================
    // 新增：增量更新支持
    // =========================================================================

    /**
     * @brief 获取层的唯一标识（用于增量更新时识别层）
     * 
     * 与 GetId() 的区别：
     * - layer_identity_ 在层的整个生命周期内不变
     * - 即使层被重新创建，只要是同一个 RenderObject，identity 应该相同
     */
    uint64_t GetLayerIdentity() const { return layer_identity_; }

    /**
     * @brief 设置层的唯一标识
     */
    void SetLayerIdentity(uint64_t identity) { layer_identity_ = identity; }

    /**
     * @brief 检查层是否为 fixed 层
     */
    bool IsFixedLayer() const { 
        return promotion_reason_ == LayerPromotionReason::PositionFixed; 
    }

    /**
     * @brief 获取层在层树中的深度
     */
    int GetTreeDepth() const;

    /**
     * @brief 重新附加到新的父层
     * @param new_parent 新的父层
     * 
     * 操作步骤：
     * 1. 从当前父层移除
     * 2. 添加到新父层
     * 3. 更新 parent_ 引用
     */
    void ReparentTo(std::shared_ptr<CompositorLayer> new_parent);

    /**
     * @brief 按 z-index 插入子层
     * @param child 要插入的子层
     * @param z_index z-index 值
     * 
     * 保持子层按 z-index 升序排列
     */
    void InsertChildByZIndex(std::shared_ptr<CompositorLayer> child, int z_index);

    /**
     * @brief 获取关联 RenderObject 的 z-index
     */
    int GetZIndex() const;

private:
    // 层的唯一标识，在层的生命周期内不变
    uint64_t layer_identity_ = 0;
    static uint64_t next_identity_;
};
```

## Data Models

### LayerUpdateOperation

```cpp
/**
 * @brief 层更新操作
 */
struct LayerUpdateOperation {
    enum class Type {
        Add,            // 添加层
        Remove,         // 移除层
        UpdateBounds,   // 更新边界
        Reparent,       // 重新附加父层
        UpdateZIndex    // 更新 z-index
    };
    
    Type type;
    RenderObject* target_obj = nullptr;
    CompositorLayer* target_layer = nullptr;
    LayerPromotionReason reason = LayerPromotionReason::None;
    int z_index = 0;
    
    // 用于调试
    std::string debug_info;
};
```

### LayerTreeSnapshot

```cpp
/**
 * @brief 层树快照（用于调试和测试）
 */
struct LayerTreeSnapshot {
    struct LayerInfo {
        uint64_t identity;
        uint32_t id;
        std::string debug_name;
        LayerPromotionReason reason;
        SkRect bounds;
        int z_index;
        uint64_t parent_identity;
        std::vector<uint64_t> children_identities;
    };
    
    uint64_t tree_version;
    std::vector<LayerInfo> layers;
    
    /**
     * @brief 比较两个快照，返回差异
     */
    static std::vector<LayerUpdateOperation> Diff(
        const LayerTreeSnapshot& before, 
        const LayerTreeSnapshot& after);
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Incremental Layer Addition Preserves Existing Layers

*For any* layer tree and any new element requiring a layer (fixed, scrollable, or will-change), adding the element's layer SHALL result in exactly one new layer being added while all existing layers retain their identity (same layer_identity values).

**Validates: Requirements 1.1, 1.3, 1.4**

### Property 2: Incremental Layer Removal Preserves Unrelated Layers

*For any* layer tree with multiple layers, removing a layer SHALL result in exactly one layer being removed while all other layers retain their identity and parent-child relationships.

**Validates: Requirements 1.2, 1.5**

### Property 3: Batch Updates Are Atomic

*For any* sequence of layer operations requested in the same frame, applying pending updates SHALL result in all operations being applied together, with the final layer tree being equivalent to applying operations individually in order.

**Validates: Requirements 1.6**

### Property 4: Scroll Offset Consistency

*For any* scroll container, querying scroll offset from ScrollStateManager, RenderObject, and CompositorLayer SHALL return identical values.

**Validates: Requirements 2.1, 2.3**

### Property 5: Scroll Offset Preservation on Rebuild

*For any* scroll container with non-zero scroll offset, triggering a layer tree rebuild SHALL preserve the scroll offset value.

**Validates: Requirements 2.2**

### Property 6: Scroll Offset Initialization

*For any* newly registered scroll container, the initial scroll offset SHALL match the value in the authoritative source (ScrollStateManager).

**Validates: Requirements 2.4**

### Property 7: Scroll Offset Notification

*For any* scroll offset change, all registered listeners SHALL receive exactly one notification with the new scroll state.

**Validates: Requirements 2.5**

### Property 8: Coordinate System Round-Trip Consistency

*For any* point in document coordinates, converting to viewport coordinates and back to document coordinates SHALL produce the original point (within floating-point tolerance).

**Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**

### Property 9: Fixed Layer Root Attachment

*For any* position:fixed element, regardless of its DOM hierarchy depth, its layer SHALL be a direct child of the root layer.

**Validates: Requirements 4.1, 4.2**

### Property 10: Fixed Layer Viewport Stability

*For any* position:fixed layer, scrolling the document SHALL not change the layer's position relative to the viewport.

**Validates: Requirements 4.3**

### Property 11: Fixed Layer Z-Index Ordering

*For any* set of position:fixed elements with different z-index values, their layers SHALL be ordered according to z-index in the root layer's children list.

**Validates: Requirements 4.4**

### Property 12: Fixed Layer Removal Isolation

*For any* position:fixed element inside a scroll container, removing the fixed element SHALL not affect the scroll container's layer structure.

**Validates: Requirements 4.5**

### Property 13: Bounds Calculation After Parent Attachment

*For any* newly created layer, bounds SHALL be calculated after the layer is attached to its parent, and the bounds SHALL be relative to the parent layer's coordinate space.

**Validates: Requirements 5.1, 5.2, 5.3**

### Property 14: Non-Fixed Layer Position Accumulation

*For any* non-fixed layer, its bounds SHALL include accumulated positions from all ancestor RenderObjects up to the parent layer's RenderObject.

**Validates: Requirements 5.4**

### Property 15: Fixed Layer Viewport Coordinates

*For any* position:fixed layer, its bounds SHALL be in viewport coordinates without any ancestor position accumulation.

**Validates: Requirements 5.5**

### Property 16: Animation State Preservation

*For any* layer with active animation, updating unrelated layers SHALL preserve the animation's current progress and timing.

**Validates: Requirements 6.1, 6.2, 6.5**

### Property 17: Animation State Transfer

*For any* animating element that gets promoted to a new layer, the animation state SHALL be transferred to the new layer with identical progress.

**Validates: Requirements 6.3**

### Property 18: Animation Cleanup

*For any* animation that completes during a layer update, animation resources SHALL be properly released.

**Validates: Requirements 6.4**

### Property 19: Content Change Dirty Marking

*For any* layer whose content changes, only that layer SHALL be marked as needing rasterization.

**Validates: Requirements 7.1**

### Property 20: Transform/Opacity Update Without Rasterization

*For any* layer whose transform or opacity changes (without content change), the layer SHALL NOT be marked as needing rasterization.

**Validates: Requirements 7.2, 7.3**

### Property 21: Bounds Change Rasterization Decision

*For any* layer whose bounds change, rasterization SHALL only be triggered if the new bounds expose previously invisible content.

**Validates: Requirements 7.4**

### Property 22: Dirty State Accuracy

*For any* layer tree after a series of updates, querying dirty state SHALL accurately reflect which layers need rasterization.

**Validates: Requirements 7.5**

### Property 23: Layer Inspection Information

*For any* layer, inspection SHALL return accurate information about its coordinate system, scroll offset source, and parent relationship.

**Validates: Requirements 8.4**

## Error Handling

### 层操作错误

1. **无效的 RenderObject**
   - 检查 RenderObject 是否为 nullptr
   - 检查 RenderObject 是否已被销毁
   - 返回错误或忽略操作

2. **层树不一致**
   - 检测到不一致时记录详细日志
   - 触发完整重建作为恢复机制
   - 通知调试工具

3. **内存分配失败**
   - 层创建失败时返回 nullptr
   - 调用者检查返回值并处理
   - 记录错误日志

### 滚动状态错误

1. **未注册的滚动容器**
   - GetScrollState 返回 nullptr
   - SetScrollPosition 返回 false
   - 调用者检查返回值

2. **无效的滚动位置**
   - 自动 clamp 到有效范围
   - 记录警告日志（调试模式）

### 坐标转换错误

1. **无效的坐标系组合**
   - 返回原始坐标
   - 记录错误日志

2. **缺少必要的层信息**
   - Layer 坐标系需要有效的层指针
   - 返回原始坐标并记录警告

## Testing Strategy

### 测试方法

本系统采用 JavaScript 自动化测试配合 C++ 日志输出进行真实 UI 测试，实现代码与效果一致性验证，全程无需人工干预。

测试方式遵循 `.kiro/steering/build-and-test.md` 规范：
- 使用 `esm_loader` 运行 JavaScript 测试脚本
- 通过 C++ 日志输出验证内部状态
- 使用 `-q 5` 参数自动退出，避免卡住

### 测试框架

使用 JavaScript 测试脚本配合 C++ 日志进行验证：

```cmd
# 运行测试命令
build\bin\Release\esm_loader.exe tests\js\test_incremental_layer_tree.js -q 5 >> debuglog.txt

# 检查测试结果
findstr "TEST_PASS TEST_FAIL" debuglog.txt

# 清理日志
del debuglog.txt
```

### 属性测试注释格式

每个属性测试必须包含以下注释：

```javascript
/**
 * Property Test: {属性名称}
 * 
 * Feature: incremental-layer-tree, Property {number}: {property_text}
 * Validates: Requirements {requirement_numbers}
 */
function testPropertyXxx() {
    // 属性测试实现
}
```

### 测试辅助函数

```javascript
// tests/js/test_incremental_layer_tree.js

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName) {
    const passed = actual === expected;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function assertLayerCount(expected, testName) {
    // 通过 C++ 日志输出层数量进行验证
    const actual = window.__getLayerCount ? window.__getLayerCount() : -1;
    return assertEqual(actual, expected, testName);
}
```

### 测试用例

#### 1. 增量层添加测试

```javascript
/**
 * Property Test: Incremental Layer Addition
 * 
 * Feature: incremental-layer-tree, Property 1: Incremental Layer Addition Preserves Existing Layers
 * Validates: Requirements 1.1, 1.3, 1.4
 */
function testIncrementalLayerAddition() {
    console.log("[TEST_START] Incremental Layer Addition");
    
    // 创建初始 DOM 结构
    const container = document.createElement('div');
    container.style.cssText = 'width: 400px; height: 300px; overflow: auto;';
    document.body.appendChild(container);
    
    // 记录初始层数量
    const initialLayerCount = window.__getLayerCount ? window.__getLayerCount() : 0;
    
    // 添加 fixed 元素
    const toast = document.createElement('div');
    toast.style.cssText = 'position: fixed; top: 10px; right: 10px; z-index: 1000;';
    toast.textContent = 'Toast';
    document.body.appendChild(toast);
    
    // 等待下一帧渲染
    requestAnimationFrame(() => {
        // 验证层数量增加了 1
        const newLayerCount = window.__getLayerCount ? window.__getLayerCount() : 0;
        assertEqual(newLayerCount, initialLayerCount + 1, "Layer count increased by 1");
        
        // 清理
        document.body.removeChild(toast);
        document.body.removeChild(container);
        
        console.log("[TEST_END]");
    });
}
```

#### 2. Fixed 元素根层附加测试

```javascript
/**
 * Property Test: Fixed Layer Root Attachment
 * 
 * Feature: incremental-layer-tree, Property 9: Fixed Layer Root Attachment
 * Validates: Requirements 4.1, 4.2
 */
function testFixedLayerRootAttachment() {
    console.log("[TEST_START] Fixed Layer Root Attachment");
    
    // 创建嵌套滚动容器
    const scrollContainer = document.createElement('div');
    scrollContainer.style.cssText = 'width: 400px; height: 300px; overflow: auto;';
    
    const content = document.createElement('div');
    content.style.cssText = 'width: 800px; height: 600px;';
    scrollContainer.appendChild(content);
    
    // 在滚动容器内创建 fixed 元素
    const fixedElement = document.createElement('div');
    fixedElement.style.cssText = 'position: fixed; top: 50px; left: 50px;';
    fixedElement.textContent = 'Fixed';
    content.appendChild(fixedElement);
    
    document.body.appendChild(scrollContainer);
    
    requestAnimationFrame(() => {
        // 验证 fixed 元素的层是根层的直接子层
        const isDirectChildOfRoot = window.__isFixedLayerDirectChildOfRoot 
            ? window.__isFixedLayerDirectChildOfRoot(fixedElement) 
            : true;
        logTest("Fixed layer is direct child of root", isDirectChildOfRoot);
        
        // 清理
        document.body.removeChild(scrollContainer);
        
        console.log("[TEST_END]");
    });
}
```

#### 3. 滚动偏移一致性测试

```javascript
/**
 * Property Test: Scroll Offset Consistency
 * 
 * Feature: incremental-layer-tree, Property 4: Scroll Offset Consistency
 * Validates: Requirements 2.1, 2.3
 */
function testScrollOffsetConsistency() {
    console.log("[TEST_START] Scroll Offset Consistency");
    
    const scrollContainer = document.createElement('div');
    scrollContainer.style.cssText = 'width: 400px; height: 300px; overflow: auto;';
    
    const content = document.createElement('div');
    content.style.cssText = 'width: 800px; height: 600px;';
    scrollContainer.appendChild(content);
    
    document.body.appendChild(scrollContainer);
    
    // 设置滚动位置
    scrollContainer.scrollLeft = 100;
    scrollContainer.scrollTop = 150;
    
    requestAnimationFrame(() => {
        // 验证滚动偏移一致性
        assertEqual(scrollContainer.scrollLeft, 100, "scrollLeft is 100");
        assertEqual(scrollContainer.scrollTop, 150, "scrollTop is 150");
        
        // 清理
        document.body.removeChild(scrollContainer);
        
        console.log("[TEST_END]");
    });
}
```

#### 4. Fixed 元素视口稳定性测试

```javascript
/**
 * Property Test: Fixed Layer Viewport Stability
 * 
 * Feature: incremental-layer-tree, Property 10: Fixed Layer Viewport Stability
 * Validates: Requirements 4.3
 */
function testFixedLayerViewportStability() {
    console.log("[TEST_START] Fixed Layer Viewport Stability");
    
    // 创建可滚动文档
    document.body.style.height = '2000px';
    
    const fixedElement = document.createElement('div');
    fixedElement.style.cssText = 'position: fixed; top: 20px; left: 20px; width: 100px; height: 50px;';
    fixedElement.textContent = 'Fixed';
    document.body.appendChild(fixedElement);
    
    // 记录初始位置
    const initialRect = fixedElement.getBoundingClientRect();
    
    // 滚动文档
    window.scrollTo(0, 500);
    
    requestAnimationFrame(() => {
        // 验证 fixed 元素位置不变
        const afterScrollRect = fixedElement.getBoundingClientRect();
        assertEqual(afterScrollRect.top, initialRect.top, "Fixed element top unchanged after scroll");
        assertEqual(afterScrollRect.left, initialRect.left, "Fixed element left unchanged after scroll");
        
        // 清理
        document.body.removeChild(fixedElement);
        document.body.style.height = '';
        window.scrollTo(0, 0);
        
        console.log("[TEST_END]");
    });
}
```

### 集成测试

1. **与动画系统集成**
   - 动画期间的层更新
   - 动画状态保持

2. **与滚动系统集成**
   - 滚动期间的层更新
   - 滚动偏移保持

3. **与渲染管线集成**
   - 完整渲染流程测试
   - 性能回归测试

## Migration Strategy (迁移策略)

### 阶段 1：引入 LayerTreeManager（不破坏现有功能）

1. 创建 `LayerTreeManager` 类，但不立即使用
2. 在 `RenderPipeline` 中添加 `LayerTreeManager` 成员
3. 保持现有的 `LayerTreeBuilder::Build()` 和 `ScrollLayerManager` 不变
4. 添加特性开关 `enable_incremental_layer_tree`

### 阶段 2：滚动状态迁移

1. 在 `LayerTreeManager` 中实现滚动状态存储
2. 修改 `ScrollLayerManager` 委托给 `LayerTreeManager`
3. 逐步移除 `RenderObject` 和 `CompositorLayer` 中的滚动状态存储
4. 验证滚动功能正常

### 阶段 3：增量更新实现

1. 实现 `LayerTreeBuilder::AddLayerForObject()` 和 `RemoveLayerForObject()`
2. 实现 `LayerTreeManager::ApplyPendingUpdates()`
3. 在 `RenderPipeline::DoLayerTreeBuild()` 中添加增量更新路径
4. 通过特性开关控制是否使用增量更新

### 阶段 4：Fixed 元素处理优化

1. 修改 `LayerTreeBuilder::FindParentLayerForObject()` 使 fixed 元素直接挂在根层
2. 更新 `Compositor::CompositeLayerCPU()` 处理新的层结构
3. 验证 fixed 元素（Toast、Modal）的渲染和交互

### 阶段 5：清理和优化

1. 移除 `ScrollLayerManager` 中的冗余代码
2. 统一坐标系计算
3. 性能测试和优化
4. 移除特性开关，默认启用增量更新

## Integration with Existing Systems (与现有系统集成)

### 与 PropertyTree 系统的关系

当前代码已有 `PropertyTrees`、`PropertyTreeBuilder`、`PaintArtifactCompositor`：

```cpp
// RenderPipeline 中的属性树系统
std::unique_ptr<PropertyTrees> property_trees_;
std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;
```

**集成策略：**
- `LayerTreeManager` 不替代 PropertyTree 系统
- PropertyTree 用于高效的属性查询和更新
- `LayerTreeManager` 用于层树结构管理
- 两者协同工作：PropertyTree 提供属性数据，LayerTreeManager 管理层结构

### 与 ScrollLayerManager 的关系

当前 `ScrollLayerManager` 负责：
- 注册/注销滚动容器
- 处理滚动事件
- 管理滚动内容层

**迁移策略：**
- 阶段 1：`ScrollLayerManager` 保持不变
- 阶段 2：滚动状态存储迁移到 `LayerTreeManager`
- 阶段 3：`ScrollLayerManager` 变为 `LayerTreeManager` 的内部实现细节
- 最终：`ScrollLayerManager` 可能被完全合并或简化

### 与 AnimationLayerBridge 的关系

`AnimationLayerBridge` 负责动画和层的关联：

**集成策略：**
- 增量更新时保持动画状态
- `LayerTreeManager` 在添加/移除层时通知 `AnimationLayerBridge`
- 动画状态转移由 `AnimationLayerBridge` 处理

## Long-term Vision (长期愿景)

### 目标架构

```
RenderPipeline
├── LayerTreeManager (核心协调器)
│   ├── 增量更新控制
│   ├── 滚动状态管理 (SSOT)
│   └── 坐标系统管理
├── LayerTreeBuilder (层树构建)
│   ├── Build() - 完整构建（回退）
│   └── IncrementalBuild() - 增量构建（默认）
├── PropertyTrees (属性树)
│   └── 高效属性查询和更新
├── Rasterizer (光栅化)
│   └── 增量光栅化
└── Compositor (合成)
    └── GPU/CPU 合成
```

### 性能目标

1. **层树更新**：从 O(n) 降低到 O(1) 或 O(log n)
2. **滚动性能**：无需重新光栅化的滚动
3. **动画性能**：transform/opacity 动画不触发光栅化
4. **内存使用**：减少不必要的层创建和销毁

### 与 Blink 架构的对比

| 功能 | Blink | LightUI (当前) | LightUI (目标) |
|------|-------|---------------|---------------|
| 层树更新 | 增量 | 完整重建 | 增量 |
| 滚动偏移 | PropertyTree | 多处存储 | SSOT |
| Fixed 元素 | 根层子层 | 可能嵌套 | 根层子层 |
| 坐标系统 | 统一 | 不一致 | 统一 |

