# Requirements Document

## Introduction

本文档定义了增量布局边界优化系统的需求。当前实现中，DOM 元素的增删操作会触发整棵渲染树的重建（`InvalidateRenderTree()`），这对于滚动容器、固定定位元素等场景造成了不必要的性能开销。

通过识别"布局边界"（Layout Boundary），可以将布局变化限制在边界内部，避免影响外部元素，从而显著提升性能。

### 布局边界类型

以下类型的元素可以作为布局边界：

1. **脱离文档流元素** - `position: fixed/absolute`
2. **滚动容器** - `overflow: scroll/auto` 且有固定尺寸
3. **固定尺寸容器** - 明确指定 `width` 和 `height` 的元素
4. **CSS Containment** - `contain: layout/strict/content`
5. **Flex/Grid 固定项** - `flex: 0 0 <size>` 不参与伸缩的项

### 受影响的组件

1. **WindowDOMObserver** - DOM 变化检测和响应
2. **RenderTreeBuilder** - 渲染树构建
3. **LayoutEngine** - 布局计算
4. **RenderObject** - 布局边界标记

## Glossary

- **Layout Boundary**: 布局边界，内部布局变化不会影响外部元素的节点
- **Out-of-Flow**: 脱离文档流，指 `position: fixed/absolute` 的元素
- **Scroll Container**: 滚动容器，`overflow: scroll/auto` 的元素
- **CSS Containment**: CSS 包含规范，通过 `contain` 属性声明布局隔离
- **Incremental Layout**: 增量布局，只重新计算受影响的子树
- **InvalidateRenderTree**: 标记整棵渲染树需要重建的操作

## Requirements

### Requirement 1

**User Story:** As a developer, I want fixed/absolute positioned elements to not trigger full render tree rebuilds when added or removed, so that modals, toasts, and dropdowns perform efficiently.

#### Acceptance Criteria

1. WHEN a `position: fixed` element is added to DOM THEN the System SHALL create its RenderObject without rebuilding the entire render tree
2. WHEN a `position: absolute` element is added to DOM THEN the System SHALL create its RenderObject without rebuilding the entire render tree
3. WHEN a `position: fixed/absolute` element is removed from DOM THEN the System SHALL remove its RenderObject without rebuilding the entire render tree
4. WHEN a fixed/absolute element is added or removed THEN the System SHALL mark only the affected area for repaint

### Requirement 2

**User Story:** As a developer, I want scroll container children changes to not trigger full render tree rebuilds, so that virtual lists and tables perform efficiently.

#### Acceptance Criteria

1. WHEN a child element is added to a scroll container THEN the System SHALL update only the scroll container's layout
2. WHEN a child element is removed from a scroll container THEN the System SHALL update only the scroll container's layout
3. WHEN scroll container children change THEN the System SHALL update the scroll container's scrollHeight/scrollWidth
4. WHEN scroll container children change THEN the System SHALL NOT affect layout of elements outside the scroll container

### Requirement 3

**User Story:** As a developer, I want fixed-size containers to act as layout boundaries, so that their internal changes don't affect external layout.

#### Acceptance Criteria

1. WHEN a child is added to a container with fixed width and height THEN the System SHALL NOT trigger layout recalculation for ancestors
2. WHEN a child is removed from a container with fixed width and height THEN the System SHALL NOT trigger layout recalculation for ancestors
3. THE System SHALL identify fixed-size containers by checking if both width and height are explicit pixel values or viewport units

### Requirement 4

**User Story:** As a developer, I want CSS Containment (`contain: layout`) to be respected as a layout boundary, so that I can explicitly declare isolation points.

#### Acceptance Criteria

1. WHEN a child is added to an element with `contain: layout` THEN the System SHALL NOT trigger layout recalculation for ancestors
2. WHEN a child is added to an element with `contain: strict` THEN the System SHALL NOT trigger layout recalculation for ancestors
3. WHEN a child is added to an element with `contain: content` THEN the System SHALL NOT trigger layout recalculation for ancestors
4. THE System SHALL parse and store the `contain` CSS property value

### Requirement 5

**User Story:** As a developer, I want the system to correctly identify layout boundaries, so that incremental updates are applied safely.

#### Acceptance Criteria

1. THE RenderObject SHALL provide an `IsLayoutBoundary()` method that returns true for layout boundary elements
2. WHEN DOM changes occur THEN the System SHALL traverse up to find the nearest layout boundary ancestor
3. WHEN a layout boundary is found THEN the System SHALL only invalidate layout from that boundary downward
4. IF no layout boundary is found THEN the System SHALL fall back to full render tree rebuild

### Requirement 6

**User Story:** As a developer, I want the incremental layout system to handle edge cases correctly, so that rendering remains correct.

#### Acceptance Criteria

1. IF a child of a layout boundary has `position: fixed` relative to viewport THEN the System SHALL handle it as a separate layout boundary
2. IF a layout boundary's size depends on children (e.g., `height: auto`) THEN the System SHALL NOT treat it as a layout boundary
3. WHEN multiple nested layout boundaries exist THEN the System SHALL use the innermost applicable boundary
4. WHEN a layout boundary element's style changes to no longer be a boundary THEN the System SHALL trigger appropriate layout recalculation
