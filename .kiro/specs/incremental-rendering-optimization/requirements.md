# Requirements Document

## Introduction

本文档定义了 LightUI 渲染引擎增量渲染优化的需求。当前实现存在严重的性能问题：即使是简单的文本更新（如计数器数字变化），也会触发全量渲染树重建和全量布局计算，导致 CPU 占用超过 10%。

参考 Chromium Blink 引擎的增量更新机制，本优化旨在实现精细化的脏标记系统和增量样式/布局更新，使简单的 DOM 变化只触发局部更新。

## Glossary

- **StyleChangeType**: 样式变化类型枚举，区分无变化、局部变化、子树变化
- **ChildNeedsStyleRecalc**: 子节点需要样式重算的标志，用于精确追踪脏子树
- **MarkAncestorsWithChildNeedsStyleRecalc**: 向上标记祖先链的方法，只设置 ChildNeedsStyleRecalc 标志
- **LocalStyleChange**: 只影响当前节点的样式变化
- **SubtreeStyleChange**: 影响整个子树的样式变化
- **IncrementalStyleRecalc**: 增量样式重算，只处理标记为脏的节点
- **IncrementalLayout**: 增量布局，只重新计算受影响的节点
- **RenderObject**: 渲染对象，DOM 节点的渲染表示
- **DirtyNodeTracker**: 脏节点追踪器，收集 DOM 变化

## Requirements

### Requirement 1

**User Story:** As a developer, I want text content changes to only trigger local updates, so that simple UI updates like counters don't cause full page re-renders.

#### Acceptance Criteria

1. WHEN a Text node's content changes THEN the system SHALL mark only the Text node and its parent RenderObject as needing layout
2. WHEN a Text node's content changes THEN the system SHALL NOT trigger full render tree rebuild
3. WHEN a Text node's content changes THEN the system SHALL only repaint the affected region
4. WHEN multiple Text nodes change in the same frame THEN the system SHALL batch the updates and process them together

### Requirement 2

**User Story:** As a developer, I want a fine-grained dirty marking system, so that style and layout changes are processed efficiently.

#### Acceptance Criteria

1. WHEN a node's style changes locally THEN the system SHALL mark it with LocalStyleChange type
2. WHEN a node's style change affects descendants THEN the system SHALL mark it with SubtreeStyleChange type
3. WHEN a node is marked dirty THEN the system SHALL mark ancestors with ChildNeedsStyleRecalc flag only
4. WHEN traversing for style recalc THEN the system SHALL skip subtrees without ChildNeedsStyleRecalc flag
5. WHEN clearing dirty flags THEN the system SHALL clear both the node's StyleChangeType and ChildNeedsStyleRecalc flag

### Requirement 3

**User Story:** As a developer, I want incremental style recalculation, so that only dirty nodes are processed during style updates.

#### Acceptance Criteria

1. WHEN performing style recalc THEN the system SHALL only visit nodes with NeedsStyleRecalc or ChildNeedsStyleRecalc flags
2. WHEN a node has LocalStyleChange THEN the system SHALL recalculate only that node's style
3. WHEN a node has SubtreeStyleChange THEN the system SHALL recalculate styles for the entire subtree
4. WHEN a clean subtree is encountered THEN the system SHALL skip it entirely without traversal
5. WHEN style recalc completes THEN the system SHALL clear all style dirty flags on processed nodes

### Requirement 4

**User Story:** As a developer, I want incremental layout updates, so that layout changes don't require full tree traversal.

#### Acceptance Criteria

1. WHEN a node's layout changes THEN the system SHALL mark only affected ancestors as needing layout
2. WHEN performing layout THEN the system SHALL skip subtrees that don't need layout
3. WHEN a node's size changes THEN the system SHALL propagate layout invalidation to ancestors
4. WHEN a node's position changes but size remains THEN the system SHALL NOT invalidate sibling layouts
5. WHEN layout completes THEN the system SHALL clear all layout dirty flags on processed nodes

### Requirement 5

**User Story:** As a developer, I want DOM structural changes to be handled incrementally when possible, so that adding or removing nodes doesn't always cause full rebuilds.

#### Acceptance Criteria

1. WHEN a single node is added THEN the system SHALL create only that node's RenderObject and insert it
2. WHEN a single node is removed THEN the system SHALL remove only that node's RenderObject
3. WHEN a node is replaced THEN the system SHALL perform atomic replacement without intermediate states
4. WHEN multiple structural changes occur THEN the system SHALL batch and optimize them before processing
5. WHEN structural changes are localized THEN the system SHALL NOT rebuild the entire render tree

### Requirement 6

**User Story:** As a developer, I want paint-only changes to skip layout entirely, so that visual-only updates are fast.

#### Acceptance Criteria

1. WHEN a paint-only property changes (color, background-color, opacity) THEN the system SHALL NOT trigger layout
2. WHEN a paint-only property changes THEN the system SHALL only mark the node for repaint
3. WHEN repainting THEN the system SHALL use the cached layout information
4. WHEN multiple paint-only changes occur THEN the system SHALL merge dirty regions for efficient repaint

### Requirement 7

**User Story:** As a developer, I want the system to provide performance metrics, so that I can verify the optimization effectiveness.

#### Acceptance Criteria

1. WHEN a frame is rendered THEN the system SHALL track the number of nodes visited for style recalc
2. WHEN a frame is rendered THEN the system SHALL track the number of nodes visited for layout
3. WHEN a frame is rendered THEN the system SHALL track the dirty region area as percentage of viewport
4. WHEN incremental update is used THEN the system SHALL log the optimization ratio compared to full update
