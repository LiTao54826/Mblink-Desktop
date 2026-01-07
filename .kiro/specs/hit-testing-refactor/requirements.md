# Requirements Document: Hit Testing System Refactor

## Introduction

This document defines the requirements for refactoring the hit testing system in LightUI. The current implementation has scattered hit testing logic across multiple classes (`HitTesting`, `PaintLayer::HitTest`, `HitTestRecursiveInternal`) with inconsistent coordinate handling, leading to failures when testing elements with high z-index that are promoted to independent compositor layers.

The refactor aims to create a unified hit testing system inspired by Blink's approach, with proper coordinate transformation tracking and consistent handling of all element types (normal, fixed, high z-index, scrolled content).

## Glossary

- **Viewport_Coordinates**: Screen coordinates relative to the window's visible area (0,0 at top-left of window). Mouse events arrive in these coordinates.
- **Document_Coordinates**: Coordinates relative to the document's origin. For scrolled content, document coordinates = viewport coordinates + scroll offset.
- **Layer_Local_Coordinates**: Coordinates relative to a specific layer's origin (its layout.x, layout.y position).
- **Hit_Testing_System**: The unified system responsible for determining which DOM element is at a given viewport coordinate.
- **Transform_State**: An object that tracks accumulated coordinate transformations as hit testing traverses the layer tree.
- **Stacking_Context**: A CSS concept where elements with certain properties (z-index, opacity, transform, etc.) create a new layer ordering context.
- **Independent_Layer**: A compositor layer that is rendered separately from its parent, typically for performance (fixed positioning, high z-index, animations).
- **PaintLayer**: The unified layer class that manages stacking context, z-order, compositing, and hit testing for a RenderObject.

## Requirements

### Requirement 1: Unified Hit Testing Entry Point

**User Story:** As a developer, I want a single entry point for all hit testing operations, so that coordinate handling is consistent regardless of element type.

#### Acceptance Criteria

1. THE Hit_Testing_System SHALL provide a single public method `HitTest(viewport_x, viewport_y)` as the entry point for all hit testing operations
2. WHEN hit testing is invoked, THE Hit_Testing_System SHALL create a Transform_State object to track coordinate transformations
3. THE Hit_Testing_System SHALL NOT have multiple parallel hit testing code paths for different element types
4. WHEN the mouse event dispatcher receives a mouse event, THE Mouse_Event_Dispatcher SHALL call only the unified hit testing entry point

### Requirement 2: Coordinate Transformation Tracking

**User Story:** As a developer, I want coordinate transformations to be tracked explicitly during hit testing traversal, so that I can correctly map viewport coordinates to any element's local coordinates.

#### Acceptance Criteria

1. THE Transform_State SHALL track the accumulated transformation matrix from viewport to current layer
2. WHEN traversing into a child layer, THE Hit_Testing_System SHALL update the Transform_State with the child's offset and any CSS transforms
3. WHEN traversing into a scrollable container, THE Hit_Testing_System SHALL apply the scroll offset to the Transform_State
4. WHEN testing a fixed-positioned element, THE Hit_Testing_System SHALL reset the Transform_State to viewport coordinates (ignoring parent scroll offsets)
5. THE Transform_State SHALL provide a method to map a viewport point to the current layer's local coordinates
6. THE Transform_State SHALL provide a method to map a local point back to viewport coordinates

### Requirement 3: Z-Order Traversal

**User Story:** As a developer, I want hit testing to traverse elements in correct z-order (highest to lowest), so that visually topmost elements are hit first.

#### Acceptance Criteria

1. WHEN hit testing a stacking context, THE Hit_Testing_System SHALL test positive z-index children first (highest to lowest)
2. WHEN hit testing a stacking context, THE Hit_Testing_System SHALL test the stacking context's own content after positive z-index children
3. WHEN hit testing a stacking context, THE Hit_Testing_System SHALL test negative z-index children last (highest to lowest)
4. WHEN multiple elements overlap at the same z-index, THE Hit_Testing_System SHALL test them in reverse document order (later elements first)
5. THE Hit_Testing_System SHALL NOT special-case high z-index elements separately from normal z-order traversal

### Requirement 4: Fixed Position Element Handling

**User Story:** As a developer, I want fixed-positioned elements to be hit tested correctly regardless of page scroll position, so that fixed headers, modals, and overlays respond to clicks.

#### Acceptance Criteria

1. WHEN hit testing a fixed-positioned element, THE Hit_Testing_System SHALL use viewport coordinates directly (layout.x, layout.y are already viewport-relative)
2. WHEN hit testing a fixed-positioned element, THE Hit_Testing_System SHALL ignore accumulated scroll offsets from ancestor elements
3. WHEN a fixed-positioned element contains scrollable content, THE Hit_Testing_System SHALL apply only that element's own scroll offset to its children
4. THE Hit_Testing_System SHALL test fixed-positioned elements at their correct z-order position (not as a special pass)

### Requirement 5: High Z-Index Element Handling

**User Story:** As a developer, I want elements with high z-index (like dropdowns) to be hit tested correctly even when promoted to independent compositor layers, so that clicking dropdown options works.

#### Acceptance Criteria

1. WHEN an element has z-index >= 100 and position absolute/fixed, THE Hit_Testing_System SHALL hit test it at its correct z-order position
2. WHEN a high z-index element is promoted to an independent compositor layer, THE Hit_Testing_System SHALL still use the PaintLayer tree for hit testing (not compositor layer bounds)
3. WHEN hit testing a high z-index element, THE Hit_Testing_System SHALL correctly transform viewport coordinates to the element's local coordinates
4. IF a high z-index element is inside a scrollable container, THEN THE Hit_Testing_System SHALL apply the container's scroll offset when calculating the element's position

### Requirement 6: Scrollable Container Handling

**User Story:** As a developer, I want hit testing to work correctly inside scrollable containers, so that clicking on scrolled content hits the correct element.

#### Acceptance Criteria

1. WHEN hit testing inside a scrollable container, THE Hit_Testing_System SHALL apply the container's scroll offset to child coordinates
2. WHEN a scrollable container has overflow:hidden, THE Hit_Testing_System SHALL clip hit testing to the container's visible bounds
3. WHEN nested scrollable containers exist, THE Hit_Testing_System SHALL accumulate scroll offsets correctly
4. THE Hit_Testing_System SHALL NOT apply scroll offset to fixed-positioned children of scrollable containers

### Requirement 7: Overflow Clipping

**User Story:** As a developer, I want hit testing to respect overflow:hidden clipping, so that elements visually clipped by their parent are not hit.

#### Acceptance Criteria

1. WHEN an element has overflow:hidden, THE Hit_Testing_System SHALL clip hit testing to the element's bounds
2. WHEN a child element extends beyond its overflow:hidden parent, THE Hit_Testing_System SHALL NOT hit the clipped portion
3. WHEN an element is promoted to an independent layer (fixed, high z-index), THE Hit_Testing_System SHALL NOT clip it to ancestor overflow:hidden bounds
4. IF the hit point is outside the clipping bounds, THEN THE Hit_Testing_System SHALL skip testing that subtree entirely

### Requirement 8: Pointer Events Property

**User Story:** As a developer, I want the pointer-events CSS property to be respected, so that elements with pointer-events:none pass through clicks to elements below.

#### Acceptance Criteria

1. WHEN an element has pointer-events:none, THE Hit_Testing_System SHALL NOT return that element as a hit target
2. WHEN an element has pointer-events:none, THE Hit_Testing_System SHALL still test its children (unless they also have pointer-events:none)
3. WHEN a parent has pointer-events:none but a child has pointer-events:auto, THE Hit_Testing_System SHALL return the child as a hit target

### Requirement 9: Hit Test Result

**User Story:** As a developer, I want hit test results to include all necessary information for event handling, so that events can be dispatched correctly.

#### Acceptance Criteria

1. THE HitTestResult SHALL contain the hit Element (DOM node)
2. THE HitTestResult SHALL contain the hit RenderObject
3. THE HitTestResult SHALL contain the local coordinates (relative to the hit element)
4. THE HitTestResult SHALL provide a method to check if a valid element was hit
5. WHEN no element is hit, THE HitTestResult SHALL have a null element and render_object

### Requirement 10: CSS Transform Support

**User Story:** As a developer, I want hit testing to work correctly on elements with CSS transforms, so that rotated, scaled, or translated elements respond to clicks at their visual position.

#### Acceptance Criteria

1. WHEN an element has a CSS transform, THE Hit_Testing_System SHALL apply the inverse transform to map viewport coordinates to the element's local coordinates
2. WHEN a transform is not invertible (e.g., scale(0)), THE Hit_Testing_System SHALL skip hit testing that element
3. WHEN nested transforms exist, THE Transform_State SHALL accumulate transforms correctly (pre-concatenation)
4. WHEN an element has preserve-3d, THE Hit_Testing_System SHALL handle 3D depth sorting for overlapping elements
5. THE Transform_State SHALL support flattening when exiting a 3D context

### Requirement 11: Clip Path Support

**User Story:** As a developer, I want hit testing to respect clip-path CSS property, so that elements clipped by complex shapes only respond to clicks within the visible area.

#### Acceptance Criteria

1. WHEN an element has clip-path, THE Hit_Testing_System SHALL test if the hit point is inside the clip path
2. IF the hit point is outside the clip-path, THEN THE Hit_Testing_System SHALL NOT return that element as a hit target
3. THE Hit_Testing_System SHALL support basic clip-path shapes (circle, ellipse, polygon, inset)

### Requirement 12: Border Radius Clipping

**User Story:** As a developer, I want hit testing to respect border-radius clipping, so that rounded corners don't respond to clicks outside the visible rounded area.

#### Acceptance Criteria

1. WHEN an element has border-radius and overflow:hidden, THE Hit_Testing_System SHALL test if the hit point is inside the rounded bounds
2. IF the hit point is in the corner area outside the border-radius curve, THEN THE Hit_Testing_System SHALL NOT return that element as a hit target
3. WHEN ancestor elements have border-radius clipping, THE Hit_Testing_System SHALL check all ancestor clips

### Requirement 13: Rect-Based Hit Testing

**User Story:** As a developer, I want to support rect-based hit testing (not just point-based), so that touch events with larger hit areas work correctly.

#### Acceptance Criteria

1. THE Hit_Testing_System SHALL support both point-based and rect-based hit testing
2. WHEN rect-based hit testing is used, THE Hit_Testing_System SHALL return all elements that intersect the hit rect
3. THE Transform_State SHALL track both the hit point and the hit quad/rect through transformations

### Requirement 14: Performance

**User Story:** As a developer, I want hit testing to be efficient, so that mouse interactions remain responsive.

#### Acceptance Criteria

1. THE Hit_Testing_System SHALL use early-out bounds checking to skip subtrees that cannot contain the hit point
2. THE Hit_Testing_System SHALL cache stacking context and z-order information when possible
3. THE Hit_Testing_System SHALL NOT traverse the entire render tree when the hit point is clearly outside document bounds
4. WHEN z-order lists are dirty, THE Hit_Testing_System SHALL rebuild them before hit testing
5. THE Hit_Testing_System SHALL avoid redundant coordinate transformations by reusing Transform_State

### Requirement 15: Animated Element Hit Testing

**User Story:** As a developer, I want hit testing to work correctly on elements with active CSS animations, so that clicking on moving or transforming elements hits them at their current visual position.

#### Acceptance Criteria

1. WHEN an element has an active transform animation, THE Hit_Testing_System SHALL use the current animated transform value for hit testing
2. WHEN an element has an active opacity animation, THE Hit_Testing_System SHALL still hit test the element (opacity does not affect hit testing)
3. WHEN an element is promoted to an independent layer for animation, THE Hit_Testing_System SHALL still use the PaintLayer tree for hit testing (not compositor layer bounds)
4. THE Hit_Testing_System SHALL query the current animation state to get the current transform value
5. WHEN animation bounds are expanded for rotation/scale animations, THE Hit_Testing_System SHALL use the actual element bounds (not expanded bounds) for hit testing

### Requirement 16: Debug Support

**User Story:** As a developer, I want to debug hit testing issues, so that I can diagnose why clicks are not hitting expected elements.

#### Acceptance Criteria

1. THE Hit_Testing_System SHALL provide a debug mode that logs the hit testing traversal path
2. THE Hit_Testing_System SHALL provide a method to dump the current z-order list for a stacking context
3. WHEN debug mode is enabled, THE Hit_Testing_System SHALL log coordinate transformations at each step


### Requirement 17: DevTools Integration

**User Story:** As a developer using DevTools, I want to inspect hit testing behavior, so that I can understand why elements are or are not being hit.

#### Acceptance Criteria

1. WHEN DevTools element picker is active, THE Hit_Testing_System SHALL highlight the element under the cursor
2. WHEN DevTools element picker is active, THE Hit_Testing_System SHALL return the correct element for inspection
3. THE Hit_Testing_System SHALL provide an API for DevTools to query hit test results at any coordinate
4. WHEN an element is selected in DevTools, THE Hit_Testing_System SHALL support highlighting that element's hit test bounds
5. THE Hit_Testing_System SHALL expose z-order information for DevTools to display stacking context hierarchy
6. WHEN DevTools requests hit test information, THE Hit_Testing_System SHALL return additional metadata (z-index, stacking context, transform state)

### Requirement 18: DevTools Element Inspection

**User Story:** As a developer using DevTools, I want to see why an element was or was not hit, so that I can debug layout and stacking issues.

#### Acceptance Criteria

1. THE Hit_Testing_System SHALL provide a method to explain why a specific element was not hit at a given coordinate
2. THE explanation SHALL include: bounds check result, overflow clipping, pointer-events value, z-order position
3. WHEN an element is clipped by overflow:hidden, THE explanation SHALL identify the clipping ancestor
4. WHEN an element is behind another element in z-order, THE explanation SHALL identify the occluding element
5. THE Hit_Testing_System SHALL support a "hit test all" mode that returns all elements at a coordinate (not just the topmost)
