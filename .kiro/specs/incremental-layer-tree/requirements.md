# Requirements Document

## Introduction

本文档定义了 LightUI 增量层树更新系统的需求。当前架构存在以下问题：

1. **层树重建触发时机不合理**：当创建 position: fixed 元素（如 Toast）时，会触发整个层树重建（needs_layer_tree_rebuild_ = true），导致所有层被重新创建、动画层状态丢失、滚动偏移需要重新恢复。

2. **滚动偏移存储位置混乱**：滚动偏移存储在多个地方（RenderObject::scroll_x_/scroll_y_、CompositorLayer::scroll_offset_、ScrollContainerInfo::scroll_x/scroll_y），层树重建时需要从多个地方同步，容易出错。

3. **层的 bounds 计算时机问题**：在 BuildRecursive 中，CreateLayer 时 bounds 计算没有父层信息，导致相对位置计算可能不正确。

4. **光栅化和合成的坐标系不一致**：光栅化时 transform_offset 基于 RenderObject 父链计算，合成时滚动偏移基于 CompositorLayer 父链应用，两个计算路径不一致。

本重构的目标是：
- 实现增量层树更新，只添加/删除变化的层
- 建立滚动偏移的单一数据源（Single Source of Truth）
- 统一坐标系，确保光栅化和合成使用一致的坐标计算
- 分离 fixed 层，使其直接挂在根层下，不参与滚动容器的层结构

## Glossary

- **CompositorLayer**: 合成层，包含 CPU 位图和 GPU 纹理，用于分层合成
- **LayerTreeBuilder**: 层树构建器，从渲染树构建合成层树
- **RenderObject**: 渲染对象，DOM 到渲染树的桥梁
- **ScrollLayerManager**: 滚动层管理器，管理滚动容器和固定元素的层
- **Layer Promotion**: 层提升，将元素提升为独立合成层的过程
- **Rasterization**: 光栅化，将渲染对象绘制到合成层的 CPU 位图
- **Compositing**: 合成，将多个层合成到屏幕
- **Document Coordinates**: 文档坐标，相对于文档左上角的坐标
- **Viewport Coordinates**: 视口坐标，相对于可见视口左上角的坐标
- **Single Source of Truth (SSOT)**: 单一数据源，数据只存储在一个地方

## Requirements

### Requirement 1

**User Story:** As a developer, I want incremental layer tree updates, so that adding or removing elements does not trigger a full layer tree rebuild.

#### Acceptance Criteria

1. WHEN a position:fixed element is created THEN the Incremental_Layer_Tree_System SHALL add a new layer to the root layer without rebuilding the entire layer tree
2. WHEN a position:fixed element is removed THEN the Incremental_Layer_Tree_System SHALL remove only the corresponding layer without affecting other layers
3. WHEN a scrollable container is created THEN the Incremental_Layer_Tree_System SHALL add a new scroll content layer without rebuilding unrelated layers
4. WHEN an element gains will-change:transform THEN the Incremental_Layer_Tree_System SHALL promote the element to a new layer without rebuilding sibling layers
5. WHEN an element loses will-change:transform THEN the Incremental_Layer_Tree_System SHALL demote the element by removing its layer and merging content back to parent layer
6. WHEN multiple layer changes occur in the same frame THEN the Incremental_Layer_Tree_System SHALL batch the changes and apply them in a single update pass

### Requirement 2

**User Story:** As a developer, I want a single source of truth for scroll offsets, so that scroll state is consistent across all components.

#### Acceptance Criteria

1. WHEN a scroll event occurs THEN the Incremental_Layer_Tree_System SHALL update the scroll offset in exactly one authoritative location
2. WHEN the layer tree is rebuilt THEN the Incremental_Layer_Tree_System SHALL preserve scroll offsets from the authoritative source without requiring synchronization from multiple locations
3. WHEN querying scroll offset THEN the Incremental_Layer_Tree_System SHALL return the value from the single authoritative source
4. WHEN a scrollable container is registered THEN the Incremental_Layer_Tree_System SHALL initialize scroll offset from the authoritative source
5. WHEN scroll offset changes THEN the Incremental_Layer_Tree_System SHALL notify all dependent components through a single update path

### Requirement 3

**User Story:** As a developer, I want consistent coordinate systems between rasterization and compositing, so that elements are positioned correctly.

#### Acceptance Criteria

1. WHEN calculating layer bounds THEN the Incremental_Layer_Tree_System SHALL use document coordinates consistently
2. WHEN rasterizing a layer THEN the Incremental_Layer_Tree_System SHALL apply transform offsets using the same coordinate system as compositing
3. WHEN compositing layers THEN the Incremental_Layer_Tree_System SHALL apply scroll offsets at a single well-defined point in the pipeline
4. WHEN a layer has both transform and scroll offset THEN the Incremental_Layer_Tree_System SHALL apply them in a consistent order across rasterization and compositing
5. WHEN converting between document and viewport coordinates THEN the Incremental_Layer_Tree_System SHALL use a single conversion function to ensure consistency

### Requirement 4

**User Story:** As a developer, I want position:fixed elements to be handled as direct children of the root layer, so that they are not affected by scroll container layer structure.

#### Acceptance Criteria

1. WHEN a position:fixed element is created THEN the Incremental_Layer_Tree_System SHALL attach its layer directly to the root layer regardless of DOM hierarchy
2. WHEN a position:fixed element is inside a scrollable container THEN the Incremental_Layer_Tree_System SHALL ensure the fixed layer is not a child of the scroll content layer
3. WHEN scrolling occurs THEN the Incremental_Layer_Tree_System SHALL keep position:fixed layers stationary relative to the viewport
4. WHEN a position:fixed element has z-index THEN the Incremental_Layer_Tree_System SHALL respect the z-index ordering among fixed layers and other root-level layers
5. WHEN a position:fixed element is removed THEN the Incremental_Layer_Tree_System SHALL remove only its layer without affecting the scroll container structure

### Requirement 5

**User Story:** As a developer, I want layer bounds to be calculated correctly with parent layer information, so that relative positioning is accurate.

#### Acceptance Criteria

1. WHEN creating a new layer THEN the Incremental_Layer_Tree_System SHALL calculate bounds after the layer is attached to its parent
2. WHEN a layer's parent changes THEN the Incremental_Layer_Tree_System SHALL recalculate the layer's bounds relative to the new parent
3. WHEN a parent layer's bounds change THEN the Incremental_Layer_Tree_System SHALL update child layer bounds that depend on parent position
4. WHEN calculating bounds for a non-fixed layer THEN the Incremental_Layer_Tree_System SHALL accumulate positions from ancestor RenderObjects up to the parent layer's RenderObject
5. WHEN calculating bounds for a fixed layer THEN the Incremental_Layer_Tree_System SHALL use viewport coordinates directly without accumulating ancestor positions

### Requirement 6

**User Story:** As a developer, I want animation layer state to be preserved during layer tree updates, so that animations continue smoothly.

#### Acceptance Criteria

1. WHEN a layer tree update occurs THEN the Incremental_Layer_Tree_System SHALL preserve animation state for layers that are not directly affected
2. WHEN a layer with active animation is updated THEN the Incremental_Layer_Tree_System SHALL maintain the animation's current progress and timing
3. WHEN a new layer is created for an animating element THEN the Incremental_Layer_Tree_System SHALL transfer animation state from any previous layer
4. WHEN an animation completes during a layer update THEN the Incremental_Layer_Tree_System SHALL properly clean up animation resources
5. WHEN multiple animations are active THEN the Incremental_Layer_Tree_System SHALL preserve all animation states independently

### Requirement 7

**User Story:** As a developer, I want efficient dirty tracking for layer updates, so that only necessary layers are rasterized.

#### Acceptance Criteria

1. WHEN a layer's content changes THEN the Incremental_Layer_Tree_System SHALL mark only that layer as needing rasterization
2. WHEN a layer's transform changes THEN the Incremental_Layer_Tree_System SHALL update the transform without triggering rasterization
3. WHEN a layer's opacity changes THEN the Incremental_Layer_Tree_System SHALL update the opacity without triggering rasterization
4. WHEN a layer's bounds change THEN the Incremental_Layer_Tree_System SHALL determine if rasterization is needed based on content visibility
5. WHEN querying dirty state THEN the Incremental_Layer_Tree_System SHALL provide accurate information about which layers need updates

### Requirement 8

**User Story:** As a developer, I want debugging tools for layer tree updates, so that I can diagnose issues with incremental updates.

#### Acceptance Criteria

1. WHEN debug mode is enabled THEN the Incremental_Layer_Tree_System SHALL log all layer additions and removals with reasons
2. WHEN debug mode is enabled THEN the Incremental_Layer_Tree_System SHALL log scroll offset changes and their source
3. WHEN debug mode is enabled THEN the Incremental_Layer_Tree_System SHALL visualize layer bounds and coordinate systems
4. WHEN inspecting a layer THEN the Incremental_Layer_Tree_System SHALL provide information about its coordinate system, scroll offset source, and parent relationship
5. WHEN a full rebuild is triggered THEN the Incremental_Layer_Tree_System SHALL log the reason and affected layers

