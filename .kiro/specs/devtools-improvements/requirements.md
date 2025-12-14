# Requirements Document

## Introduction

本文档定义了 MBink DevTools 的改进需求，针对现有实现中发现的三个问题进行修复和增强：计算样式显示不完整、盒模型数值缺失、以及盒模型缺少鼠标悬停高亮效果。这些改进将提升 DevTools 的实用性和用户体验。

## Glossary

- **ComputedStylesView**: 计算样式视图，显示元素最终应用的所有 CSS 属性值的组件
- **BoxModelView**: 盒模型视图，可视化显示元素的 margin、border、padding、content 区域的组件
- **RenderObject**: 渲染对象，包含元素布局和样式信息的内部数据结构
- **ComputedStyle**: 计算样式，从 RenderObject 获取的元素最终样式值
- **HoveredArea**: 悬停区域，用户鼠标当前悬停的盒模型区域（margin/border/padding/content）

## Requirements

### Requirement 1

**User Story:** As a developer, I want to see the actual computed style values for an element, so that I can understand the real CSS values being applied.

#### Acceptance Criteria

1. WHEN an element is selected in DevTools THEN the ComputedStylesView SHALL display the actual computed values from RenderObject::GetComputedStyle() instead of hardcoded placeholder values
2. WHEN displaying computed styles THEN the ComputedStylesView SHALL show the correct value for each CSS property including width, height, display, position, color, background-color, font-size, and other standard properties
3. WHEN a computed style property has a non-default value THEN the ComputedStylesView SHALL visually distinguish that property from properties with default values
4. WHEN the selected element changes THEN the ComputedStylesView SHALL refresh to show the new element's computed styles within 100 milliseconds

### Requirement 2

**User Story:** As a developer, I want to see all box model values (margin, border, padding, content), so that I can understand the complete layout dimensions of an element.

#### Acceptance Criteria

1. WHEN an element is selected THEN the BoxModelView SHALL display numeric values for all four margin sides (top, right, bottom, left)
2. WHEN an element is selected THEN the BoxModelView SHALL display numeric values for all four border sides (top, right, bottom, left)
3. WHEN an element is selected THEN the BoxModelView SHALL display numeric values for all four padding sides (top, right, bottom, left)
4. WHEN an element is selected THEN the BoxModelView SHALL display the content width and height in the center of the box model diagram
5. WHEN a box model value is zero THEN the BoxModelView SHALL display "0" instead of hiding the value

### Requirement 3

**User Story:** As a developer, I want visual feedback when hovering over different box model regions, so that I can clearly identify which area (margin, border, padding, content) I am inspecting.

#### Acceptance Criteria

1. WHEN the mouse cursor hovers over the margin region in BoxModelView THEN the BoxModelView SHALL highlight the margin area with increased opacity or distinct color
2. WHEN the mouse cursor hovers over the border region in BoxModelView THEN the BoxModelView SHALL highlight the border area with increased opacity or distinct color
3. WHEN the mouse cursor hovers over the padding region in BoxModelView THEN the BoxModelView SHALL highlight the padding area with increased opacity or distinct color
4. WHEN the mouse cursor hovers over the content region in BoxModelView THEN the BoxModelView SHALL highlight the content area with increased opacity or distinct color
5. WHEN the mouse cursor leaves a box model region THEN the BoxModelView SHALL restore the region to its default visual state
6. WHEN a region is highlighted THEN the BoxModelView SHALL also highlight the corresponding area on the selected element in the main application view

