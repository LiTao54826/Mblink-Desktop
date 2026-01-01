# Requirements Document

## Introduction

本文档定义了 LightUI 统一层系统重构的需求。当前 LightUI 存在两个独立的层系统：
1. **CompositorLayer** (core/compositor/) - 负责 GPU 合成、纹理缓存、动画
2. **LayerManager/Layer** (core/render/layer/) - 负责 z-index 排序、绘制顺序、Hit Testing

这两个系统各自处理绘制顺序，导致职责重叠和潜在的不一致性。参考 Blink 的设计，z-index 排序应该在 PaintLayer 层面统一处理，Compositing 只负责决定哪些层需要 GPU 加速，不改变绘制顺序。

本重构的目标是统一这两个系统，建立清晰的职责分离：
- **PaintLayer** - 统一处理 Stacking Context 和 z-index 排序
- **CompositorLayer** - 只负责 GPU 加速和纹理缓存优化

## Glossary

- **Stacking Context**: CSS 堆叠上下文，决定元素的绘制顺序
- **PaintLayer**: 绘制层，每个需要特殊绘制处理的 RenderObject 都有一个 PaintLayer
- **CompositorLayer**: 合成层，需要 GPU 加速的 PaintLayer 会有对应的 CompositorLayer
- **z-index**: CSS 属性，控制元素在 Stacking Context 中的堆叠顺序
- **Compositing**: 合成，决定哪些 PaintLayer 需要独立的 GPU 层
- **Hit Testing**: 点击测试，确定鼠标/触摸事件命中哪个元素
- **Rasterization**: 光栅化，将矢量图形转换为位图
- **Property Tree**: 属性树，高效管理变换、裁剪、效果等属性

## Requirements

### Requirement 1

**User Story:** As a developer, I want a unified PaintLayer system that handles all stacking context and z-index sorting, so that the rendering order is consistent and predictable.

#### Acceptance Criteria

1. WHEN a RenderObject creates a new stacking context THEN the Unified_Layer_System SHALL create a corresponding PaintLayer for that RenderObject
2. WHEN a PaintLayer has child elements with z-index THEN the Unified_Layer_System SHALL maintain separate lists for positive z-index children (pos_z_order_list) and negative z-index children (neg_z_order_list)
3. WHEN painting a stacking context THEN the Unified_Layer_System SHALL paint in the following order: background, negative z-index children, normal flow, positive z-index children
4. WHEN serializing a PaintLayer tree to a string representation THEN the Unified_Layer_System SHALL produce output that can be parsed back to reconstruct an equivalent tree structure
5. WHEN a PaintLayer tree is serialized and then parsed THEN the Unified_Layer_System SHALL produce a tree with identical structure and z-index ordering

### Requirement 2

**User Story:** As a developer, I want CompositorLayer to focus solely on GPU acceleration and caching, so that compositing decisions don't affect rendering order.

#### Acceptance Criteria

1. WHEN a PaintLayer needs GPU acceleration THEN the Unified_Layer_System SHALL create a CompositorLayer that references the PaintLayer without duplicating z-index sorting logic
2. WHEN compositing layers THEN the Unified_Layer_System SHALL traverse the PaintLayer tree to determine draw order rather than maintaining separate ordering in CompositorLayer
3. WHEN a CompositorLayer is created for GPU acceleration THEN the Unified_Layer_System SHALL preserve the original paint order defined by the PaintLayer hierarchy
4. WHEN serializing compositing decisions THEN the Unified_Layer_System SHALL produce output that can be parsed back to verify GPU acceleration assignments

### Requirement 3

**User Story:** As a developer, I want position:fixed elements to be handled correctly within the unified layer system, so that they render at the correct position and z-order.

#### Acceptance Criteria

1. WHEN an element has position:fixed THEN the Unified_Layer_System SHALL create a PaintLayer that participates in the root stacking context
2. WHEN painting position:fixed elements THEN the Unified_Layer_System SHALL apply viewport-relative positioning while respecting z-index ordering
3. WHEN hit testing position:fixed elements THEN the Unified_Layer_System SHALL correctly identify hits based on viewport coordinates
4. WHEN a position:fixed element has z-index THEN the Unified_Layer_System SHALL sort the element according to its z-index within the root stacking context

### Requirement 4

**User Story:** As a developer, I want efficient hit testing through the unified layer system, so that mouse and touch events are correctly dispatched.

#### Acceptance Criteria

1. WHEN performing hit testing THEN the Unified_Layer_System SHALL traverse PaintLayers in reverse paint order (top to bottom visually)
2. WHEN a hit is detected in a higher z-index layer THEN the Unified_Layer_System SHALL return that element without testing lower layers
3. WHEN hit testing scrollable containers THEN the Unified_Layer_System SHALL account for scroll offsets in coordinate transformation
4. WHEN multiple elements overlap at the same z-index THEN the Unified_Layer_System SHALL return the element that appears later in document order

### Requirement 5

**User Story:** As a developer, I want the LayerManager to be removed or refactored, so that there is a single source of truth for layer management.

#### Acceptance Criteria

1. WHEN the refactoring is complete THEN the Unified_Layer_System SHALL have removed the LayerManager class from core/render/layer/
2. WHEN high z-index elements (overlay, modal) need special handling THEN the Unified_Layer_System SHALL handle them through PaintLayer's stacking context mechanism
3. WHEN the old LayerManager functionality is needed THEN the Unified_Layer_System SHALL provide equivalent functionality through the PaintLayer system
4. WHEN migrating from LayerManager THEN the Unified_Layer_System SHALL maintain backward compatibility for existing z-index threshold behavior (overlay >= 100, modal >= 1000)

### Requirement 6

**User Story:** As a developer, I want clear compositing promotion rules, so that I can predict which elements will get GPU acceleration.

#### Acceptance Criteria

1. WHEN an element has will-change:transform or will-change:opacity THEN the Unified_Layer_System SHALL promote its PaintLayer to have a CompositorLayer
2. WHEN an element has an active CSS transform or opacity animation THEN the Unified_Layer_System SHALL promote its PaintLayer to have a CompositorLayer
3. WHEN an element is a scrollable container with overflow content THEN the Unified_Layer_System SHALL promote its PaintLayer to have a CompositorLayer
4. WHEN an element has position:fixed THEN the Unified_Layer_System SHALL promote its PaintLayer to have a CompositorLayer
5. WHEN compositing promotion occurs THEN the Unified_Layer_System SHALL record the promotion reason for debugging purposes

### Requirement 7

**User Story:** As a developer, I want the unified layer system to support incremental updates, so that only changed portions of the layer tree are rebuilt.

#### Acceptance Criteria

1. WHEN a RenderObject's style changes THEN the Unified_Layer_System SHALL update only the affected PaintLayer and its ancestors
2. WHEN a RenderObject is added or removed THEN the Unified_Layer_System SHALL incrementally update the PaintLayer tree without full rebuild
3. WHEN z-index changes on an element THEN the Unified_Layer_System SHALL re-sort only the affected stacking context's z-order lists
4. WHEN compositing reasons change THEN the Unified_Layer_System SHALL update CompositorLayer assignments without affecting PaintLayer structure

### Requirement 8

**User Story:** As a developer, I want debugging tools for the unified layer system, so that I can diagnose rendering and compositing issues.

#### Acceptance Criteria

1. WHEN debug mode is enabled THEN the Unified_Layer_System SHALL display visual borders around PaintLayers with different colors for different layer types
2. WHEN debug mode is enabled THEN the Unified_Layer_System SHALL log stacking context creation and z-index sorting decisions
3. WHEN inspecting a PaintLayer THEN the Unified_Layer_System SHALL provide information about its stacking context, z-index, compositing status, and associated RenderObject
4. WHEN serializing debug information THEN the Unified_Layer_System SHALL produce human-readable output showing the complete layer hierarchy

