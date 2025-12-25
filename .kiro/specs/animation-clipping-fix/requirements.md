# Requirements Document: Animation Clipping Fix

## Introduction

本文档定义了修复动画分层裁剪问题的需求。当前实现中，动画元素在以下情况下会被错误裁剪：

1. **旋转裁剪** - 矩形元素旋转时，角会超出原始边界框被裁掉
2. **移动裁剪** - 元素上下/左右移动时，移出原始位置区域会被裁掉

根本原因是层边界在构建时基于元素的静态布局位置计算，没有考虑动画过程中变换的动态变化范围。

## Problem Analysis

### 问题 1: 层边界静态计算

**根因分析:**
- `layer_tree_builder.cpp:UpdateLayerBounds()` 只在层创建时计算边界
- 虽然代码尝试扩展边界以容纳变换后的内容，但只使用当前帧的变换值
- 动画过程中变换值持续变化，但层边界不会更新
- 结果：动画元素在某些帧会超出层边界被裁剪

### 问题 2: 光栅化裁剪

**根因分析:**
- `rasterizer.cpp:RasterizeLayer()` 使用层边界作为位图大小
- 当元素变换超出层边界时，超出部分无法被光栅化
- 增量光栅化 `RasterizeRegion()` 使用 `clipIRect()` 进一步限制绘制区域

### 问题 3: 动画范围未知

**根因分析:**
- 系统不知道动画的完整变换范围（最大旋转角度、最大位移等）
- 无法预先计算足够大的层边界来容纳整个动画过程

## Glossary

- **Layer Bounds**: 合成层的边界矩形，决定了层位图的大小和位置
- **Transform Animation**: CSS transform 属性的动画，包括 rotate、translate、scale 等
- **Clipping**: 裁剪，超出边界的内容被截断不显示
- **Keyframes**: CSS @keyframes 定义的动画关键帧
- **Bounding Box**: 包围盒，能完全包含变换后内容的最小矩形

## Requirements

### Requirement 1: 动画边界预计算

**User Story:** As a developer, I want animated elements to have layer bounds that accommodate the full animation range, so that content is never clipped during animation.

#### Acceptance Criteria

1. WHEN a layer is created for an animated element THEN the system SHALL calculate bounds that encompass all keyframe transforms
2. WHEN an element has rotate animation THEN the layer bounds SHALL be large enough to contain the element at any rotation angle
3. WHEN an element has translate animation THEN the layer bounds SHALL include the full translation range from all keyframes
4. WHEN an element has scale animation THEN the layer bounds SHALL accommodate the maximum scale value

### Requirement 2: 动态边界更新

**User Story:** As a developer, I want layer bounds to update when animation properties change, so that new animations are properly accommodated.

#### Acceptance Criteria

1. WHEN animation keyframes change THEN the system SHALL recalculate layer bounds
2. WHEN animation is added to an element THEN the system SHALL expand layer bounds if needed
3. WHEN animation is removed from an element THEN the system SHALL shrink layer bounds to optimal size
4. WHEN updating bounds THEN the system SHALL preserve existing rasterized content where possible

### Requirement 3: 旋转动画边界计算

**User Story:** As a developer, I want rotation animations to never clip corners, so that rotating rectangles display correctly.

#### Acceptance Criteria

1. WHEN an element has rotation animation THEN the layer bounds SHALL be the circumscribed circle of the element
2. WHEN calculating rotation bounds THEN the system SHALL use the diagonal length as the minimum dimension
3. WHEN rotation center is offset THEN the system SHALL adjust bounds accordingly
4. WHEN rotation is combined with other transforms THEN the system SHALL calculate the combined bounding box

### Requirement 4: 位移动画边界计算

**User Story:** As a developer, I want translation animations to show the full movement range, so that moving elements are never cut off.

#### Acceptance Criteria

1. WHEN an element has translate animation THEN the layer bounds SHALL include start and end positions
2. WHEN parsing keyframes THEN the system SHALL extract all translate values
3. WHEN translate uses percentage values THEN the system SHALL resolve them relative to element size
4. WHEN translate is combined with rotation THEN the system SHALL calculate the combined bounding box

### Requirement 5: 光栅化区域扩展

**User Story:** As a developer, I want the rasterizer to use expanded bounds for animated elements, so that all transformed content is captured.

#### Acceptance Criteria

1. WHEN rasterizing an animated layer THEN the system SHALL use the expanded bounds for bitmap allocation
2. WHEN the layer has transform animation THEN the rasterizer SHALL NOT clip to original element bounds
3. WHEN drawing transformed content THEN the system SHALL offset drawing to account for expanded bounds
4. WHEN compositing THEN the system SHALL position the layer correctly despite expanded bounds

### Requirement 6: 性能优化

**User Story:** As a developer, I want animation bounds calculation to be efficient, so that it does not impact animation performance.

#### Acceptance Criteria

1. WHEN calculating animation bounds THEN the system SHALL cache results until animation changes
2. WHEN animation uses simple transforms THEN the system SHALL use optimized bounds calculation
3. WHEN bounds are unchanged THEN the system SHALL NOT reallocate layer bitmaps
4. WHEN multiple elements animate THEN the system SHALL calculate bounds in parallel where possible

