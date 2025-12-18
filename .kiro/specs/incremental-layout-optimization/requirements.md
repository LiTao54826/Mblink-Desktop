# Requirements Document

## Introduction

本文档定义了 LightUI 渲染引擎增量布局优化的需求。当前系统在处理局部 DOM 变化（如文本更新）时会触发全量布局重算，导致性能问题。本优化旨在实现真正的增量布局，只重新计算受影响的节点，大幅提升动态内容更新的性能。

## Glossary

- **LayoutNode**: 布局树中的节点，包含布局缓存和样式信息
- **LayoutCache**: 存储布局计算结果的缓存，基于输入参数匹配
- **ContentVersion**: 内容版本号，用于标识节点内容是否发生变化
- **IFC (Inline Formatting Context)**: 内联格式化上下文，处理文本和内联元素的布局
- **DirtyNode**: 需要重新布局的脏节点
- **NativeLayoutEngine**: 原生布局引擎，负责整体布局计算

## Requirements

### Requirement 1: 内容版本号机制

**User Story:** As a developer, I want the layout system to track content changes via version numbers, so that the cache can automatically invalidate when content changes without manual cache clearing.

#### Acceptance Criteria

1. WHEN a text node's content changes THEN the LayoutNode SHALL increment its content_version
2. WHEN a node's children are added or removed THEN the LayoutNode SHALL increment its content_version
3. WHEN a node's layout-affecting style changes THEN the LayoutNode SHALL increment its content_version
4. WHEN the cache checks for a hit THEN the Cache SHALL compare content_version in addition to dimensions and available_space
5. WHEN content_version mismatches THEN the Cache SHALL return cache miss and trigger recomputation

### Requirement 2: 增量布局路径优化

**User Story:** As a developer, I want incremental layout to only recompute dirty nodes, so that small DOM changes do not trigger full layout recalculation.

#### Acceptance Criteria

1. WHEN ComputeIncrementalLayout is called THEN the NativeLayoutEngine SHALL only clear cache for dirty nodes
2. WHEN ComputeIncrementalLayout executes layout THEN the NativeLayoutEngine SHALL skip the global cache clear in ComputeLayout
3. WHEN a dirty node's ancestors are traversed THEN the NativeLayoutEngine SHALL propagate dirty marks upward to the root
4. WHEN clean nodes are encountered during layout THEN the NativeLayoutEngine SHALL return cached results immediately
5. WHEN scrollbar detection is needed THEN the NativeLayoutEngine SHALL handle it without clearing all caches

### Requirement 3: IFC 布局缓存集成

**User Story:** As a developer, I want IFC layout cache to integrate with the content version system, so that text changes only invalidate affected IFC containers when necessary.

#### Acceptance Criteria

1. WHEN IFC layout checks cache validity THEN the IFCLayout SHALL use content_version instead of text hash computation
2. WHEN a text node changes and the container has fixed width THEN the IFCLayout SHALL invalidate only that container's cache without propagating to ancestors
3. WHEN a text node changes and the container has auto/fit-content width THEN the IFCLayout SHALL propagate dirty marks to ancestors because container size may change
4. WHEN IFC cache is valid THEN the IFCLayout SHALL skip text measurement and line breaking
5. WHEN multiple IFC containers exist THEN the IFCLayout SHALL maintain independent cache validity per container

### Requirement 3.1: 文本变化影响范围判断

**User Story:** As a developer, I want the system to intelligently determine the impact scope of text changes, so that unnecessary layout recalculations are avoided.

#### Acceptance Criteria

1. WHEN text content changes but measured width remains within container bounds THEN the NativeLayoutEngine SHALL only trigger repaint without full layout
2. WHEN text content changes and measured width exceeds container bounds THEN the NativeLayoutEngine SHALL trigger line-breaking recalculation
3. WHEN text content changes in a fixed-width container THEN the NativeLayoutEngine SHALL not propagate layout changes to parent containers
4. WHEN text content changes in an auto-width container THEN the NativeLayoutEngine SHALL propagate layout changes to parent containers if text width changes
5. WHEN text content changes in a flex/grid child with flex-grow or percentage width THEN the NativeLayoutEngine SHALL evaluate whether sibling layouts are affected

### Requirement 4: 脏标记传播机制

**User Story:** As a developer, I want dirty marks to propagate correctly through the layout tree, so that parent containers are aware of child changes.

#### Acceptance Criteria

1. WHEN a leaf node is marked dirty THEN the MarkNeedsLayout SHALL propagate dirty marks to all ancestors
2. WHEN a dirty mark reaches an already-dirty ancestor THEN the MarkNeedsLayout SHALL stop propagation
3. WHEN an absolute/fixed positioned element changes THEN the MarkNeedsLayout SHALL still propagate to ancestors for correct containing block handling
4. WHEN Flexbox/Grid children change size THEN the parent container SHALL be marked dirty to recalculate sibling layouts

### Requirement 5: 性能验证

**User Story:** As a developer, I want measurable performance improvements, so that I can verify the optimization is effective.

#### Acceptance Criteria

1. WHEN a single text node updates in a 100-node tree THEN the layout time SHALL be less than 10% of full layout time
2. WHEN incremental layout runs THEN the number of MeasureText calls SHALL be proportional to dirty nodes only
3. WHEN the clock demo updates every second THEN the CPU usage SHALL remain below 5% during idle periods
4. WHEN profiling incremental layout THEN the cache hit rate for clean nodes SHALL be above 95%

### Requirement 6: 向后兼容性

**User Story:** As a developer, I want the optimization to be backward compatible, so that existing functionality is not broken.

#### Acceptance Criteria

1. WHEN full layout is requested THEN the ComputeLayout SHALL produce identical results to the current implementation
2. WHEN BuildLayoutTree is called THEN the NativeLayoutEngine SHALL reset all content versions appropriately
3. WHEN window resize occurs THEN the NativeLayoutEngine SHALL trigger full layout with proper cache invalidation
4. WHEN existing tests run THEN all layout-related tests SHALL pass without modification
