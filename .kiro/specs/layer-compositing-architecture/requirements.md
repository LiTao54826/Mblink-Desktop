# Requirements Document

## Introduction

本文档定义了 LightUI 渲染引擎的分层合成架构重构需求。当前架构使用 GPU 直接渲染，导致增量更新困难、滚动时必须全量重绘。新架构采用业界标准的 CPU 光栅化 + GPU 合成模式，为后续的分层合成、增量更新打下基础。

## Glossary

- **Layer（层）**: 独立的渲染单元，拥有自己的位图缓存，可以独立更新和变换
- **Compositing（合成）**: 将多个层的纹理按正确顺序混合到最终画面的过程
- **Rasterization（光栅化）**: 将矢量图形转换为像素位图的过程
- **Tile（瓦片）**: 层的子区域，用于细粒度的增量更新
- **Display List**: 绘制命令的序列化表示，可以重放执行
- **Compositor Thread**: 专门负责合成的线程，独立于主线程
- **GPU Texture**: GPU 显存中的纹理，用于硬件加速合成

## Requirements

### Requirement 1: 渲染架构分离

**User Story:** As a developer, I want the rendering architecture to separate rasterization from compositing, so that each layer can be updated independently without affecting others.

#### Acceptance Criteria

1. WHEN the system initializes THEN the Renderer SHALL create a CPU-based Skia surface for rasterization
2. WHEN a layer needs to be drawn THEN the system SHALL rasterize content to a CPU bitmap first
3. WHEN compositing occurs THEN the system SHALL upload dirty layer bitmaps to GPU textures
4. WHEN displaying the final frame THEN the system SHALL use GPU to composite all layer textures
5. WHEN GPU is unavailable THEN the system SHALL fall back to CPU-only compositing using SDL software rendering

### Requirement 2: 层管理系统

**User Story:** As a developer, I want a layer management system that automatically creates and manages compositing layers, so that elements with specific properties can be rendered independently.

#### Acceptance Criteria

1. WHEN an element has `will-change: transform` or `will-change: opacity` THEN the system SHALL create a dedicated compositing layer for that element
2. WHEN an element has `position: fixed` THEN the system SHALL create a dedicated compositing layer
3. WHEN an element has CSS animation on transform or opacity THEN the system SHALL promote the element to its own layer
4. WHEN a scrollable container has overflow content THEN the system SHALL create a separate layer for the scrollable content
5. WHEN a layer is no longer needed THEN the system SHALL release the layer resources and merge content back to parent layer
6. WHEN querying layer count THEN the system SHALL return the current number of active compositing layers

### Requirement 3: 增量光栅化

**User Story:** As a developer, I want the system to only re-rasterize changed portions of a layer, so that rendering performance is optimized.

#### Acceptance Criteria

1. WHEN an element's visual properties change THEN the system SHALL mark only the affected region as dirty within its layer
2. WHEN rasterizing a layer THEN the system SHALL only redraw the dirty regions, preserving unchanged pixels
3. WHEN a layer has no dirty regions THEN the system SHALL skip rasterization entirely for that layer
4. WHEN dirty regions are collected THEN the system SHALL merge overlapping regions to reduce draw calls
5. WHEN a layer is scrolled THEN the system SHALL reuse existing pixels and only rasterize newly visible content

### Requirement 4: GPU 纹理管理

**User Story:** As a developer, I want efficient GPU texture management, so that memory usage is optimized and texture uploads are minimized.

#### Acceptance Criteria

1. WHEN a layer bitmap changes THEN the system SHALL upload only the changed region to the GPU texture
2. WHEN a layer is created THEN the system SHALL allocate a GPU texture matching the layer size
3. WHEN a layer is resized THEN the system SHALL reallocate the GPU texture to match the new size
4. WHEN a layer is destroyed THEN the system SHALL release the associated GPU texture
5. WHEN GPU memory is limited THEN the system SHALL prioritize visible layers and evict off-screen layer textures

### Requirement 5: 合成优化

**User Story:** As a developer, I want the compositor to efficiently blend layers, so that the final frame is rendered with minimal GPU overhead.

#### Acceptance Criteria

1. WHEN compositing layers THEN the system SHALL render layers in correct z-order using GPU blending
2. WHEN a layer has transform animation THEN the system SHALL apply the transform matrix during compositing without re-rasterizing
3. WHEN a layer has opacity animation THEN the system SHALL apply opacity during compositing without re-rasterizing
4. WHEN layers overlap THEN the system SHALL use GPU alpha blending for correct transparency
5. WHEN no layers have changed THEN the system SHALL skip compositing and reuse the previous frame

### Requirement 6: 滚动优化

**User Story:** As a developer, I want scrolling to be smooth and efficient, so that users experience fluid interactions.

#### Acceptance Criteria

1. WHEN the user scrolls a container THEN the system SHALL move the content layer's position without re-rasterizing
2. WHEN scrolling reveals new content THEN the system SHALL rasterize only the newly visible region
3. WHEN scrolling hides content THEN the system SHALL preserve the hidden content in the layer for potential reuse
4. WHEN scroll position changes THEN the system SHALL update the compositor within 16ms to maintain 60fps
5. WHEN a fixed-position element exists THEN the system SHALL keep it stationary during scroll by compositing it separately

### Requirement 7: 动画性能

**User Story:** As a developer, I want animations to run smoothly at 60fps, so that the UI feels responsive and polished.

#### Acceptance Criteria

1. WHEN a transform animation runs THEN the system SHALL update only the layer's transform matrix each frame
2. WHEN an opacity animation runs THEN the system SHALL update only the layer's opacity value each frame
3. WHEN multiple animations run simultaneously THEN the system SHALL batch compositor updates to minimize GPU calls
4. WHEN an animation completes THEN the system SHALL optionally demote the layer back to its parent if no longer needed
5. WHEN animation frame time exceeds 16ms THEN the system SHALL log a warning for performance debugging

### Requirement 8: 向后兼容

**User Story:** As a developer, I want the new architecture to be backward compatible, so that existing applications continue to work without modification.

#### Acceptance Criteria

1. WHEN an application uses the existing rendering API THEN the system SHALL produce identical visual output
2. WHEN the system falls back to CPU-only mode THEN the visual output SHALL match GPU-accelerated mode
3. WHEN layer promotion is disabled THEN the system SHALL render all content in a single root layer
4. WHEN debugging is enabled THEN the system SHALL provide layer boundary visualization
5. WHEN performance profiling is enabled THEN the system SHALL report per-layer rasterization and compositing times
