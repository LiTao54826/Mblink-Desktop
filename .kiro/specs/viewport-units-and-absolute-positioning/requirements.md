# Requirements Document

## Introduction

This document specifies the requirements for fixing viewport units (vh, vw, vmin, vmax) and absolute positioning (bottom, right) support in the MBink layout engine. Currently, while the CSS parser recognizes these values, the layout engine does not correctly compute them, causing layout issues in applications that rely on full-viewport layouts or absolute positioning with bottom/right constraints.

## Glossary

- **Viewport**: The visible area of the application window where content is rendered
- **Viewport Units**: CSS length units relative to viewport dimensions (vh, vw, vmin, vmax)
- **vh**: 1% of viewport height
- **vw**: 1% of viewport width
- **vmin**: 1% of the smaller dimension (min of vh, vw)
- **vmax**: 1% of the larger dimension (max of vh, vw)
- **Absolute Positioning**: CSS position mode where element is positioned relative to its containing block
- **Inset Properties**: CSS properties (top, right, bottom, left) that define positioned element offsets
- **Containing Block**: The reference box used for positioning calculations

## Requirements

### Requirement 1

**User Story:** As a developer, I want to use viewport height units (vh) for element sizing, so that I can create full-height layouts that adapt to window size.

#### Acceptance Criteria

1. WHEN a developer sets `height: 100vh` on an element THEN the Layout_Engine SHALL compute the height as equal to the viewport height in pixels
2. WHEN a developer sets `height: 50vh` on an element THEN the Layout_Engine SHALL compute the height as 50% of the viewport height
3. WHEN the viewport is resized THEN the Layout_Engine SHALL recalculate vh-based dimensions using the new viewport height
4. WHEN vh units are used in nested elements THEN the Layout_Engine SHALL resolve vh relative to the root viewport, not the parent element

### Requirement 2

**User Story:** As a developer, I want to use viewport width units (vw) for element sizing, so that I can create responsive layouts based on window width.

#### Acceptance Criteria

1. WHEN a developer sets `width: 100vw` on an element THEN the Layout_Engine SHALL compute the width as equal to the viewport width in pixels
2. WHEN a developer sets `width: 50vw` on an element THEN the Layout_Engine SHALL compute the width as 50% of the viewport width
3. WHEN the viewport is resized THEN the Layout_Engine SHALL recalculate vw-based dimensions using the new viewport width

### Requirement 3

**User Story:** As a developer, I want to use vmin and vmax units, so that I can create layouts that scale based on the smaller or larger viewport dimension.

#### Acceptance Criteria

1. WHEN a developer uses vmin units THEN the Layout_Engine SHALL compute the value as a percentage of the smaller viewport dimension
2. WHEN a developer uses vmax units THEN the Layout_Engine SHALL compute the value as a percentage of the larger viewport dimension
3. WHEN viewport dimensions change THEN the Layout_Engine SHALL recalculate vmin/vmax values accordingly

### Requirement 4

**User Story:** As a developer, I want to position elements using bottom and right inset properties with absolute positioning, so that I can anchor elements to the bottom-right of their containing block.

#### Acceptance Criteria

1. WHEN an absolutely positioned element has `bottom: 0` set THEN the Layout_Engine SHALL position the element's bottom edge at the bottom of its containing block
2. WHEN an absolutely positioned element has `right: 0` set THEN the Layout_Engine SHALL position the element's right edge at the right of its containing block
3. WHEN an absolutely positioned element has both `top` and `bottom` set without explicit height THEN the Layout_Engine SHALL stretch the element to fill the vertical space between the inset values
4. WHEN an absolutely positioned element has both `left` and `right` set without explicit width THEN the Layout_Engine SHALL stretch the element to fill the horizontal space between the inset values
5. WHEN an absolutely positioned element has `bottom` set with a pixel value THEN the Layout_Engine SHALL offset the element's bottom edge by that value from the containing block's bottom

### Requirement 5

**User Story:** As a developer, I want viewport units to work correctly in the Taffy layout engine integration, so that flexbox and grid layouts can use viewport-relative sizing.

#### Acceptance Criteria

1. WHEN viewport units are used in flex item dimensions THEN the Layout_Engine SHALL resolve them to pixel values before passing to Taffy
2. WHEN viewport units are used in grid track definitions THEN the Layout_Engine SHALL resolve them to pixel values
3. WHEN the viewport size changes THEN the Layout_Engine SHALL trigger a re-layout for elements using viewport units

### Requirement 6

**User Story:** As a developer, I want consistent behavior between block layout and flex layout for absolute positioning, so that my layouts work predictably regardless of display mode.

#### Acceptance Criteria

1. WHEN an absolutely positioned child is inside a block container THEN the Layout_Engine SHALL apply inset properties correctly
2. WHEN an absolutely positioned child is inside a flex container THEN the Layout_Engine SHALL apply inset properties correctly
3. WHEN bottom/right inset values use percentage units THEN the Layout_Engine SHALL resolve them relative to the containing block dimensions
