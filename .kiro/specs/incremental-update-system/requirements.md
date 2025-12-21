# Requirements Document

## Introduction

本文档定义了 LightUI 渲染引擎增量更新系统重构的需求。当前系统存在双树同步困难、ReplaceChild 时序问题、缺乏原子性、布局树顺序错误等问题。本重构旨在采用延迟同步 + 统一树 + 生命周期的设计，解决这些核心问题，提升系统稳定性和性能。

## Glossary

- **DirtyNodeTracker**: 脏节点追踪器，收集 DOM 变化并延迟到渲染前统一处理
- **RenderTreeSynchronizer**: 渲染树同步器，在渲染前将 DOM 变化同步到渲染树
- **RenderPipeline**: 渲染管线，管理渲染生命周期的各个阶段
- **StructuralChange**: 结构变化记录，包括节点添加、删除、移动、替换
- **RenderLifecycle**: 渲染生命周期阶段枚举
- **AnonymousBlockBox**: 匿名块盒，用于包裹混合内容中的内联元素
- **IFC (Inline Formatting Context)**: 内联格式化上下文

## Requirements

### Requirement 1: 脏节点追踪器

**User Story:** As a developer, I want DOM changes to be collected and deferred until render time, so that intermediate states do not cause inconsistencies.

#### Acceptance Criteria

1. WHEN a node is added to the DOM THEN the DirtyNodeTracker SHALL record the addition with parent reference and index
2. WHEN a node is removed from the DOM THEN the DirtyNodeTracker SHALL record the removal with parent reference and index
3. WHEN a node is replaced via ReplaceChild THEN the DirtyNodeTracker SHALL record it as a single atomic replacement operation
4. WHEN a node's style changes THEN the DirtyNodeTracker SHALL record the style change with property name and old/new values
5. WHEN a text node's content changes THEN the DirtyNodeTracker SHALL record the text change with old and new text
6. WHEN HasPendingChanges is called THEN the DirtyNodeTracker SHALL return true if any changes are pending
7. WHEN Optimize is called THEN the DirtyNodeTracker SHALL merge redundant changes (e.g., add then remove same node)

### Requirement 2: 渲染树同步器

**User Story:** As a developer, I want the render tree to be synchronized with DOM changes atomically before rendering, so that the render tree is always consistent.

#### Acceptance Criteria

1. WHEN Synchronize is called THEN the RenderTreeSynchronizer SHALL process all pending structural changes
2. WHEN Synchronize is called THEN the RenderTreeSynchronizer SHALL process all pending style changes
3. WHEN Synchronize is called THEN the RenderTreeSynchronizer SHALL process all pending text changes
4. WHEN changes exceed a threshold (e.g., >10 changes) THEN the RenderTreeSynchronizer SHALL rebuild affected subtrees instead of incremental updates
5. WHEN a Replaced change is processed THEN the RenderTreeSynchronizer SHALL handle it atomically without intermediate states
6. WHEN synchronization completes THEN the DirtyNodeTracker SHALL be cleared

### Requirement 3: DOM 操作延迟更新

**User Story:** As a developer, I want DOM operations to record changes instead of immediately updating the render tree, so that batch operations are efficient.

#### Acceptance Criteria

1. WHEN AppendChild is called THEN the Node SHALL modify DOM tree and record change to DirtyNodeTracker
2. WHEN RemoveChild is called THEN the Node SHALL record change before modifying DOM tree
3. WHEN ReplaceChild is called THEN the Node SHALL record as atomic replacement operation
4. WHEN InsertBefore is called THEN the Node SHALL record change with correct index
5. WHEN SetTextContent is called on a text node THEN the Node SHALL record text change

### Requirement 4: 渲染管线生命周期

**User Story:** As a developer, I want a clear rendering pipeline with defined lifecycle stages, so that rendering is predictable and debuggable.

#### Acceptance Criteria

1. WHEN ProcessFrame is called THEN the RenderPipeline SHALL execute stages in order: RenderTreeSync → StyleRecalc → Layout → Paint
2. WHEN in RenderTreeSync stage THEN the RenderPipeline SHALL synchronize render tree if changes are pending
3. WHEN in StyleRecalc stage THEN the RenderPipeline SHALL recalculate styles if needed
4. WHEN in Layout stage THEN the RenderPipeline SHALL compute layout for dirty nodes
5. WHEN in Paint stage THEN the RenderPipeline SHALL render the frame
6. WHEN lifecycle stage changes THEN the RenderPipeline SHALL update lifecycle state

### Requirement 5: 子树重建策略

**User Story:** As a developer, I want the system to intelligently choose between incremental updates and subtree rebuilds, so that performance is optimized.

#### Acceptance Criteria

1. WHEN changes count exceeds 10 THEN the RenderTreeSynchronizer SHALL trigger subtree rebuild
2. WHEN a Replaced node has more than 5 children THEN the RenderTreeSynchronizer SHALL trigger subtree rebuild
3. WHEN same parent has more than 3 changes THEN the RenderTreeSynchronizer SHALL trigger subtree rebuild for that parent
4. WHEN incremental update is chosen THEN the RenderTreeSynchronizer SHALL process changes one by one
5. WHEN subtree rebuild is chosen THEN the RenderTreeSynchronizer SHALL rebuild entire affected subtree

### Requirement 6: 向后兼容性

**User Story:** As a developer, I want the refactored system to be backward compatible, so that existing functionality is not broken.

#### Acceptance Criteria

1. WHEN existing DOM operations are called THEN the system SHALL produce the same final render result
2. WHEN existing tests run THEN all tests SHALL pass without modification
3. WHEN Preact demo runs THEN the demo SHALL function correctly with the new system
4. WHEN window resize occurs THEN the system SHALL handle it correctly

### Requirement 7: 窗口实际渲染测试

**User Story:** As a developer, I want to verify the system through actual window rendering tests, so that I can confirm visual correctness.

#### Acceptance Criteria

1. WHEN a test creates a window with DOM elements THEN the window SHALL render correctly
2. WHEN DOM elements are dynamically added THEN the window SHALL update to show new elements
3. WHEN DOM elements are removed THEN the window SHALL update to hide removed elements
4. WHEN ReplaceChild is called THEN the window SHALL show the new element in place of the old one
5. WHEN text content changes THEN the window SHALL display the updated text
6. WHEN styles change THEN the window SHALL reflect the style changes visually

