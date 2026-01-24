# Requirements Document

## Introduction

本文档分析 LightUI 框架中原生 JavaScript 操作 UI 时的增量更新机制，识别每种 DOM 操作的处理逻辑、已知问题和缺失的功能。目标是建立一个全面的增量更新系统分析，为后续修复和优化提供依据。

## Glossary

- **DOM_Tree**: Document Object Model 树，表示 HTML 文档的层次结构
- **Render_Tree**: 渲染树，由 RenderObject 组成，用于布局和绘制
- **Layout_Engine**: 布局引擎，计算元素的位置和尺寸
- **DirtyNodeTracker**: 脏节点追踪器，记录 DOM 变化以便延迟处理
- **WindowDOMObserver**: 窗口 DOM 观察者，监听 DOM 变化并触发更新
- **StyleResolver**: 样式解析器，计算元素的最终样式
- **Incremental_Update**: 增量更新，只更新变化的部分而非全量重建
- **Full_Rebuild**: 全量重建，重新构建整个渲染树

## Requirements

### Requirement 1: DOM 结构操作分析

**User Story:** As a developer, I want to understand how each DOM structure operation triggers incremental updates, so that I can identify bugs and optimize performance.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the behavior of `appendChild` operation including:
   - DOM tree modification logic
   - DirtyNodeTracker recording
   - WindowDOMObserver notification
   - RenderObject creation timing
   - Layout boundary detection

2. THE Analysis_Document SHALL document the behavior of `removeChild` operation including:
   - DOM tree modification logic
   - DirtyNodeTracker recording
   - WindowDOMObserver notification
   - RenderObject removal timing
   - Layer cleanup for fixed/absolute elements

3. THE Analysis_Document SHALL document the behavior of `insertBefore` operation including:
   - Index calculation logic
   - Insertion position accuracy
   - Sibling relationship updates

4. THE Analysis_Document SHALL document the behavior of `replaceChild` operation including:
   - Atomic replacement handling
   - Time sequence of remove and add notifications
   - RenderObject replacement logic

5. THE Analysis_Document SHALL document the behavior of `innerHTML` and `outerHTML` operations including:
   - Lexbor parsing integration
   - Batch update handling
   - Full rebuild triggering conditions

### Requirement 2: 属性操作分析

**User Story:** As a developer, I want to understand how attribute changes trigger incremental updates, so that I can fix CSS class switching issues.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the behavior of `setAttribute` operation including:
   - Attribute storage update
   - Observer notification
   - Style recalculation triggering
   - Special handling for `id`, `class`, `style` attributes

2. THE Analysis_Document SHALL document the behavior of `className` property changes including:
   - CSS class parsing
   - Style resolution
   - **Known Issue**: display property changes not triggering render tree rebuild

3. THE Analysis_Document SHALL document the behavior of `classList` operations including:
   - `add()`, `remove()`, `toggle()` methods
   - DOMTokenList synchronization

4. THE Analysis_Document SHALL identify the bug where CSS class changes affecting `display` property do not trigger proper render tree updates

### Requirement 3: 样式操作分析

**User Story:** As a developer, I want to understand how style changes trigger incremental updates, so that I can optimize rendering performance.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the behavior of `element.style.xxx` property changes including:
   - Inline style storage
   - Style attribute synchronization
   - Observer notification
   - Layout vs paint-only property distinction

2. THE Analysis_Document SHALL document the special handling of `display` property changes including:
   - `display: none` to visible transition (RenderObject creation)
   - Visible to `display: none` transition (RenderObject removal)
   - Full render tree rebuild triggering

3. THE Analysis_Document SHALL document paint-only properties that do not trigger layout:
   - `color`, `background-color`, `opacity`, etc.
   - Dirty rectangle marking
   - Incremental repaint

4. THE Analysis_Document SHALL document layout-affecting properties:
   - `width`, `height`, `margin`, `padding`, etc.
   - Layout invalidation propagation
   - Cache clearing

### Requirement 4: 文本内容操作分析

**User Story:** As a developer, I want to understand how text content changes trigger incremental updates, so that I can ensure efficient text rendering.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the behavior of `textContent` property changes including:
   - Single Text node optimization
   - Text node reuse vs recreation
   - OnTextChanged notification
   - RenderText update

2. THE Analysis_Document SHALL document the behavior of `innerText` property changes including:
   - Whitespace handling
   - Line break normalization

3. THE Analysis_Document SHALL document Text node `data` property changes including:
   - Direct text update
   - Layout recalculation
   - Dirty rectangle marking

### Requirement 5: 伪类状态变化分析

**User Story:** As a developer, I want to understand how pseudo-class changes trigger incremental updates, so that I can ensure proper hover and focus effects.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the behavior of `:hover` pseudo-class changes including:
   - Style recalculation
   - Animation triggering
   - Performance optimization (HasHoverRules check)

2. THE Analysis_Document SHALL document the behavior of `:focus` pseudo-class changes including:
   - FocusManager integration
   - Caret rendering
   - Text input activation

3. THE Analysis_Document SHALL document the behavior of other pseudo-classes including:
   - `:active`, `:checked`, `:disabled`
   - Paint-only updates

### Requirement 6: 批量操作分析

**User Story:** As a developer, I want to understand how batch DOM operations are handled, so that I can optimize complex UI updates.

#### Acceptance Criteria

1. THE Analysis_Document SHALL document the batch update mechanism including:
   - `Document::BeginBatch()` and `EndBatch()` API
   - IsInBatch check in observers
   - OnSubtreeModified notification

2. THE Analysis_Document SHALL document the incremental vs full rebuild decision logic including:
   - Change area calculation
   - Viewport percentage threshold (50%)
   - Structural change count threshold

3. THE Analysis_Document SHALL document DirtyNodeTracker optimization including:
   - Change merging
   - Redundant operation elimination

### Requirement 7: 已知问题汇总

**User Story:** As a developer, I want a comprehensive list of known incremental update issues, so that I can prioritize fixes.

#### Acceptance Criteria

1. THE Analysis_Document SHALL list all known issues with:
   - Problem description
   - Reproduction steps
   - Root cause analysis
   - Affected code paths
   - Proposed fix direction

2. THE Analysis_Document SHALL include the CSS class change issue:
   - `className` changes not triggering display property updates
   - Missing `InvalidateRenderTree()` call in `OnAttributeChanged`

3. THE Analysis_Document SHALL include the mouse wheel scrolling issue:
   - Cannot scroll to container bottom
   - Scrollbar drag works correctly

### Requirement 8: 代码路径文档

**User Story:** As a developer, I want to understand the complete code path for each operation, so that I can debug issues effectively.

#### Acceptance Criteria

1. THE Analysis_Document SHALL provide code path diagrams for:
   - DOM operation → DirtyNodeTracker → WindowDOMObserver → RenderTree update
   - Style change → StyleResolver → ComputedStyle update → Layout/Paint

2. THE Analysis_Document SHALL identify key files and functions:
   - `core/dom/node.cpp` - DOM operations
   - `core/dom/element.cpp` - Attribute and style operations
   - `core/window/window_dom_observer.cpp` - Observer callbacks
   - `core/render/css/style_resolver.cpp` - Style resolution
   - `core/layout/layout_engine.cpp` - Layout calculation

3. THE Analysis_Document SHALL document the render pipeline phases:
   - DOM modification
   - Style recalculation
   - Render tree synchronization
   - Layout calculation
   - Paint
