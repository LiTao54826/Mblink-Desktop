# Requirements Document

## Introduction

本文档定义了 LightUI 滚动条系统统一化的需求。当前滚动条处理逻辑分散在多个地方，导致 body 元素与普通元素的滚动条行为不一致，以及水平滚动条在不应该出现时错误显示。本次重构旨在统一所有元素的滚动条检测、布局和绘制逻辑，参考 Blink 引擎的 ScrollableArea 架构设计。

## Glossary

- **ScrollableArea**: 可滚动区域，包含滚动条状态、滚动偏移量和内容尺寸信息
- **scrollbar_width**: 滚动条宽度，默认 12px
- **scrollbar_gutter**: 滚动条占用的空间，用于减少内容区域宽度
- **overflow**: CSS overflow 属性，控制内容溢出时的行为（visible/hidden/scroll/auto）
- **content_size**: 内容实际尺寸，用于判断是否需要滚动条
- **visible_size**: 可见区域尺寸，即容器内部可用空间

## Requirements

### Requirement 1

**User Story:** As a developer, I want all elements (including body) to use the same scrollbar detection logic, so that scrollbar behavior is consistent across the application.

#### Acceptance Criteria

1. WHEN an element has overflow:auto and content height exceeds container height THEN the System SHALL display a vertical scrollbar and reduce content area width by scrollbar_width
2. WHEN an element has overflow:auto and content width exceeds (container width - vertical scrollbar width) THEN the System SHALL display a horizontal scrollbar
3. WHEN an element has overflow:scroll THEN the System SHALL always display scrollbars regardless of content size
4. WHEN the body element has overflow:auto THEN the System SHALL use the same scrollbar detection logic as other elements
5. WHEN vertical scrollbar is displayed THEN the System SHALL reduce content layout width by scrollbar_width (12px)

### Requirement 2

**User Story:** As a developer, I want the scrollbar to be displayed inside the element's boundary (Chrome behavior), so that the element's outer dimensions remain unchanged.

#### Acceptance Criteria

1. WHEN a scrollbar is needed THEN the System SHALL display the scrollbar inside the element's border box
2. WHEN calculating content area width THEN the System SHALL subtract scrollbar_gutter from the available width
3. WHEN the element width is set to viewport width THEN the System SHALL maintain that width and reduce content area instead

### Requirement 3

**User Story:** As a developer, I want to remove duplicate scrollbar handling code, so that the codebase is maintainable and consistent.

#### Acceptance Criteria

1. WHEN handling scrollbar detection THEN the System SHALL use a single unified code path for all elements
2. WHEN the body element needs scrollbar detection THEN the System SHALL NOT use special-case handling separate from other elements
3. WHEN computing scrollbar_gutter THEN the System SHALL use the ComputeScrollbarGutter function in block_layout.cpp

### Requirement 4

**User Story:** As a developer, I want to access scroll-related properties via JavaScript API, so that I can programmatically control scrolling behavior.

#### Acceptance Criteria

1. WHEN accessing element.scrollWidth THEN the System SHALL return the total content width including overflow
2. WHEN accessing element.scrollHeight THEN the System SHALL return the total content height including overflow
3. WHEN accessing element.scrollTop THEN the System SHALL return the current vertical scroll offset
4. WHEN accessing element.scrollLeft THEN the System SHALL return the current horizontal scroll offset
5. WHEN setting element.scrollTop THEN the System SHALL update the vertical scroll position within valid bounds
6. WHEN setting element.scrollLeft THEN the System SHALL update the horizontal scroll position within valid bounds

### Requirement 5

**User Story:** As a developer, I want debug logging to be removed from production code, so that the console output is clean.

#### Acceptance Criteria

1. WHEN the application runs THEN the System SHALL NOT output debug printf statements related to scrollbar layout
2. WHEN scrollbar detection occurs THEN the System SHALL NOT call fflush(stdout) for debugging purposes

### Requirement 6

**User Story:** As a developer, I want textarea elements to continue using their existing scrollbar implementation, so that form element behavior is preserved.

#### Acceptance Criteria

1. WHEN a textarea element needs scrollbars THEN the System SHALL use the HTMLTextAreaElement's dedicated scrollbar handling
2. WHEN the unified scrollbar system is applied THEN the System SHALL NOT affect textarea scrollbar behavior
