# Requirements Document

## Introduction

本文档定义了 MBink 框架的开发者工具（DevTools）功能需求，类似于 Chrome DevTools 的 Elements 面板和样式分析工具。该工具将帮助开发者在运行时检查和调试 DOM 结构、CSS 样式、计算样式以及布局信息，提升 MBink 应用的开发和调试体验。

## Glossary

- **DevTools**: 开发者工具，用于检查和调试应用程序的工具集
- **Inspector**: 检查器，用于查看和修改 DOM 元素及其属性的组件
- **DOM Tree**: 文档对象模型树，表示 HTML 文档的层次结构
- **Computed Style**: 计算样式，元素最终应用的所有 CSS 属性值
- **Inline Style**: 内联样式，直接在元素 style 属性中定义的样式
- **Box Model**: 盒模型，描述元素的 margin、border、padding、content 区域
- **Layout Box**: 布局盒，元素在页面上的位置和尺寸信息
- **Highlight Overlay**: 高亮覆盖层，用于在页面上可视化显示选中元素的边界

## Requirements

### Requirement 1

**User Story:** As a developer, I want to view the DOM tree structure of my application, so that I can understand and navigate the element hierarchy.

#### Acceptance Criteria

1. WHEN the DevTools Inspector is opened THEN the Inspector SHALL display the complete DOM tree starting from the document root
2. WHEN a DOM node has child nodes THEN the Inspector SHALL display an expandable/collapsible indicator for that node
3. WHEN a user clicks on an expand indicator THEN the Inspector SHALL reveal the child nodes of that element
4. WHEN a user clicks on a collapse indicator THEN the Inspector SHALL hide the child nodes of that element
5. WHEN the DOM tree is modified at runtime THEN the Inspector SHALL update the tree view to reflect the changes within 100 milliseconds

### Requirement 2

**User Story:** As a developer, I want to select elements in the DOM tree, so that I can inspect their properties and styles.

#### Acceptance Criteria

1. WHEN a user clicks on an element in the DOM tree view THEN the Inspector SHALL mark that element as selected and highlight it visually
2. WHEN an element is selected THEN the Inspector SHALL display a highlight overlay on the corresponding element in the rendered view
3. WHEN a user hovers over an element in the DOM tree view THEN the Inspector SHALL display a temporary highlight overlay on the corresponding rendered element
4. WHEN a different element is selected THEN the Inspector SHALL remove the highlight from the previously selected element

### Requirement 3

**User Story:** As a developer, I want to view element attributes, so that I can verify the correct configuration of DOM elements.

#### Acceptance Criteria

1. WHEN an element is selected THEN the Inspector SHALL display all attributes of that element in an attributes panel
2. WHEN an element has an id attribute THEN the Inspector SHALL display the id with distinct visual styling
3. WHEN an element has class attributes THEN the Inspector SHALL display all class names
4. WHEN an element has data-* attributes THEN the Inspector SHALL display all dataset attributes
5. WHEN an attribute value changes at runtime THEN the Inspector SHALL update the displayed value within 100 milliseconds

### Requirement 4

**User Story:** As a developer, I want to view inline styles of an element, so that I can see styles directly applied to the element.

#### Acceptance Criteria

1. WHEN an element is selected THEN the Inspector SHALL display all inline styles defined on that element
2. WHEN an inline style property is displayed THEN the Inspector SHALL show both the property name and its value
3. WHEN an element has no inline styles THEN the Inspector SHALL display an empty state message indicating no inline styles exist

### Requirement 5

**User Story:** As a developer, I want to view computed styles of an element, so that I can see the final resolved CSS values.

#### Acceptance Criteria

1. WHEN an element is selected THEN the Inspector SHALL display the computed styles for that element
2. WHEN displaying computed styles THEN the Inspector SHALL show all CSS properties with their resolved values
3. WHEN a computed style value differs from the default value THEN the Inspector SHALL visually distinguish that property
4. WHEN computed styles are displayed THEN the Inspector SHALL group properties by category (layout, typography, colors, etc.)

### Requirement 6

**User Story:** As a developer, I want to view the box model of an element, so that I can understand its layout dimensions.

#### Acceptance Criteria

1. WHEN an element is selected THEN the Inspector SHALL display a visual box model diagram
2. WHEN displaying the box model THEN the Inspector SHALL show margin values for all four sides
3. WHEN displaying the box model THEN the Inspector SHALL show border values for all four sides
4. WHEN displaying the box model THEN the Inspector SHALL show padding values for all four sides
5. WHEN displaying the box model THEN the Inspector SHALL show the content width and height
6. WHEN the element layout changes THEN the Inspector SHALL update the box model display within 100 milliseconds

### Requirement 7

**User Story:** As a developer, I want to modify element styles in real-time, so that I can experiment with different styling options.

#### Acceptance Criteria

1. WHEN a user edits an inline style value in the Inspector THEN the Inspector SHALL apply the change to the element immediately
2. WHEN a user adds a new style property in the Inspector THEN the Inspector SHALL add that property to the element's inline styles
3. WHEN a user removes a style property in the Inspector THEN the Inspector SHALL remove that property from the element's inline styles
4. WHEN a style modification is invalid THEN the Inspector SHALL display an error indicator and preserve the previous valid value

### Requirement 8

**User Story:** As a developer, I want to modify element attributes in real-time, so that I can test different configurations.

#### Acceptance Criteria

1. WHEN a user edits an attribute value in the Inspector THEN the Inspector SHALL apply the change to the element immediately
2. WHEN a user adds a new attribute in the Inspector THEN the Inspector SHALL add that attribute to the element
3. WHEN a user removes an attribute in the Inspector THEN the Inspector SHALL remove that attribute from the element
4. WHEN an attribute modification affects element behavior THEN the Inspector SHALL trigger appropriate DOM updates

### Requirement 9

**User Story:** As a developer, I want to search for elements in the DOM tree, so that I can quickly locate specific elements.

#### Acceptance Criteria

1. WHEN a user enters a CSS selector in the search field THEN the Inspector SHALL highlight all matching elements in the DOM tree
2. WHEN a user enters a text string in the search field THEN the Inspector SHALL highlight elements containing that text in their tag name, id, or class
3. WHEN search results are found THEN the Inspector SHALL display the count of matching elements
4. WHEN no search results are found THEN the Inspector SHALL display a message indicating no matches
5. WHEN multiple results exist THEN the Inspector SHALL provide navigation controls to move between matches

### Requirement 10

**User Story:** As a developer, I want to use a pick element tool, so that I can select elements directly from the rendered view.

#### Acceptance Criteria

1. WHEN the pick element mode is activated THEN the Inspector SHALL highlight elements under the cursor in the rendered view
2. WHEN a user clicks on an element in pick mode THEN the Inspector SHALL select that element in the DOM tree view
3. WHEN a user clicks on an element in pick mode THEN the Inspector SHALL exit pick mode and display the element's properties
4. WHEN pick mode is active THEN the Inspector SHALL display a visual indicator showing the current element under cursor with its tag name and dimensions

### Requirement 11

**User Story:** As a developer, I want the DevTools to be rendered as an embedded panel within the same window, so that I can inspect my application without managing multiple windows.

#### Acceptance Criteria

1. WHEN the DevTools is opened THEN the Inspector SHALL render as an embedded panel within the application window using the existing rendering pipeline
2. WHEN the DevTools panel is displayed THEN the Inspector SHALL support docking to the bottom or right side of the window
3. WHEN the DevTools panel is resized THEN the Inspector SHALL preserve the resize state across sessions
4. WHEN the DevTools is closed THEN the Inspector SHALL restore the full application view
5. WHEN the DevTools panel is active THEN the Inspector SHALL share the same event loop and rendering context as the main application

### Requirement 12

**User Story:** As a developer, I want to toggle DevTools with a keyboard shortcut, so that I can quickly access debugging tools.

#### Acceptance Criteria

1. WHEN a user presses F12 THEN the Inspector SHALL toggle the DevTools panel visibility
2. WHEN a user presses Ctrl+Shift+I (or Cmd+Shift+I on macOS) THEN the Inspector SHALL toggle the DevTools panel visibility
3. WHEN DevTools is opened via keyboard shortcut THEN the Inspector SHALL restore the previous panel state and selected element

### Requirement 13

**User Story:** As a developer, I want to serialize and pretty-print the DOM tree, so that I can export and analyze the structure.

#### Acceptance Criteria

1. WHEN a user requests DOM serialization THEN the Inspector SHALL generate a formatted HTML string representation of the DOM tree
2. WHEN serializing the DOM THEN the Inspector SHALL preserve all attributes and their values
3. WHEN serializing the DOM THEN the Inspector SHALL use proper indentation for nested elements
4. WHEN parsing the serialized output THEN the Inspector SHALL produce an equivalent DOM structure (round-trip consistency)
