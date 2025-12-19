# Design Document: Viewport Units and Absolute Positioning Fix

## Overview

This design addresses two related layout issues in MBink:

1. **Viewport Units (vh, vw, vmin, vmax)**: While the CSS parser recognizes these units, the layout engine doesn't properly resolve them during layout computation. The `CSSLength::ToPx()` method calls `GetViewportHeight()/GetViewportWidth()` but these values may not be properly propagated to all layout contexts.

2. **Absolute Positioning with bottom/right**: The block layout engine has the code for handling absolute positioning with inset properties, but there may be issues with how the containing block dimensions are passed or how the inset values are resolved.

## Architecture

### Current Flow

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│  CSS Parser     │────▶│  Style Resolver  │────▶│  Layout Engine  │
│  (lexbor)       │     │  (CSSLength)     │     │  (Taffy/Native) │
└─────────────────┘     └──────────────────┘     └─────────────────┘
                               │
                               ▼
                        ┌──────────────────┐
                        │  ViewportSize    │
                        │  (static class)  │
                        └──────────────────┘
```

### Problem Analysis

1. **Viewport Units Issue**:
   - `ViewportSize::Set()` is called in `RenderObject::SetViewportSize()`
   - `CSSLength::ToPx()` uses `GetViewportHeight()/GetViewportWidth()` 
   - However, the native layout engine (`NativeLayoutEngine`) may be resolving lengths before viewport size is set, or using a different resolution path

2. **Absolute Positioning Issue**:
   - `block_layout.cpp::PerformAbsoluteLayoutOnAbsoluteChildren()` correctly handles left/right/top/bottom
   - The issue may be in how `area_size` is computed - it should be the containing block's padding box dimensions
   - For elements with `position: absolute` and no positioned ancestor, the containing block should be the viewport

### Proposed Solution

#### Phase 1: Fix Viewport Unit Resolution

1. **Ensure ViewportSize is set before layout**:
   - Verify `RenderObject::SetViewportSize()` is called before any layout computation
   - Add viewport size to layout context passed through the layout tree

2. **Update CSSLength resolution in layout engine**:
   - Modify `ConvertLength()` in `native_layout_engine.cpp` to handle vh/vw/vmin/vmax
   - Pass viewport dimensions through the layout input structure

3. **Add viewport-aware length resolution**:
   ```cpp
   // In native_layout_engine.cpp
   static LengthPercentage ConvertLengthWithViewport(
       const CSSLength& css_length,
       float viewport_width,
       float viewport_height
   ) {
       switch (css_length.unit) {
           case CSSUnit::VH:
               return LengthPercentage::Length(css_length.value * viewport_height / 100.0f);
           case CSSUnit::VW:
               return LengthPercentage::Length(css_length.value * viewport_width / 100.0f);
           case CSSUnit::VMIN:
               return LengthPercentage::Length(css_length.value * std::min(viewport_width, viewport_height) / 100.0f);
           case CSSUnit::VMAX:
               return LengthPercentage::Length(css_length.value * std::max(viewport_width, viewport_height) / 100.0f);
           // ... existing cases
       }
   }
   ```

#### Phase 2: Fix Absolute Positioning

1. **Verify containing block calculation**:
   - For `position: absolute`, containing block is nearest positioned ancestor's padding box
   - For `position: fixed`, containing block is the viewport
   - If no positioned ancestor, use initial containing block (viewport)

2. **Fix area_size computation in block layout**:
   - Ensure `area_size` passed to `PerformAbsoluteLayoutOnAbsoluteChildren()` is correct
   - For root-level absolute elements, use viewport dimensions

3. **Add debug logging** (temporary):
   - Log inset resolution values
   - Log containing block dimensions
   - Log final computed positions

## Components and Interfaces

### Modified Files

1. **core/layout/native_layout_engine.cpp**
   - Add viewport dimensions to `CreateNode()` style conversion
   - Update `ConvertLength()` to handle viewport units
   - Pass viewport size through layout context

2. **core/layout/native_layout_engine.h**
   - Add viewport size members to layout context if needed

3. **core/layout/block_layout.cpp**
   - Verify `PerformAbsoluteLayoutOnAbsoluteChildren()` receives correct area_size
   - Add handling for viewport as containing block

4. **core/render/css_value.cpp**
   - Verify `ViewportSize` is properly initialized before use

5. **core/render/render_object.cpp**
   - Ensure `SetViewportSize()` is called at the right time

### New Structures

```cpp
// Layout context with viewport info
struct LayoutContext {
    float viewport_width;
    float viewport_height;
    // ... existing fields
};
```

## Data Models

### CSSUnit Enum (existing)
```cpp
enum class CSSUnit {
    PX,
    PERCENT,
    EM,
    REM,
    VW,      // viewport width
    VH,      // viewport height
    VMIN,    // min(vw, vh)
    VMAX,    // max(vw, vh)
    AUTO,
    NONE
};
```

### ViewportSize (existing static class)
```cpp
class ViewportSize {
public:
    static void Set(float width, float height);
    static float GetWidth();
    static float GetHeight();
private:
    static float width_;
    static float height_;
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Viewport height unit resolution
*For any* viewport height H and vh value V, the computed pixel value SHALL equal H * V / 100
**Validates: Requirements 1.1, 1.2**

### Property 2: Viewport width unit resolution  
*For any* viewport width W and vw value V, the computed pixel value SHALL equal W * V / 100
**Validates: Requirements 2.1, 2.2**

### Property 3: Viewport unit recalculation on resize
*For any* element using viewport units, when viewport dimensions change, the computed pixel values SHALL update to reflect the new viewport size
**Validates: Requirements 1.3, 2.3**

### Property 4: Viewport units are viewport-relative regardless of nesting
*For any* nested element using vh/vw units, the computed value SHALL be relative to the root viewport, not the parent element's dimensions
**Validates: Requirements 1.4**

### Property 5: vmin/vmax resolution
*For any* viewport with dimensions W and H, vmin SHALL resolve relative to min(W,H) and vmax SHALL resolve relative to max(W,H)
**Validates: Requirements 3.1, 3.2**

### Property 6: Absolute positioning with bottom inset
*For any* absolutely positioned element with bottom: B in a containing block of height H, the element's bottom edge SHALL be at position (H - B) from the containing block's top
**Validates: Requirements 4.1, 4.5**

### Property 7: Absolute positioning with right inset
*For any* absolutely positioned element with right: R in a containing block of width W, the element's right edge SHALL be at position (W - R) from the containing block's left
**Validates: Requirements 4.2**

### Property 8: Absolute positioning stretch with top+bottom
*For any* absolutely positioned element with top: T and bottom: B but no explicit height, in a containing block of height H, the element's height SHALL equal H - T - B - margins
**Validates: Requirements 4.3**

### Property 9: Absolute positioning stretch with left+right
*For any* absolutely positioned element with left: L and right: R but no explicit width, in a containing block of width W, the element's width SHALL equal W - L - R - margins
**Validates: Requirements 4.4**

### Property 10: Consistent absolute positioning across layout modes
*For any* absolutely positioned element, the inset properties SHALL produce the same positioning result whether the parent is a block or flex container
**Validates: Requirements 6.1, 6.2**

## Error Handling

1. **Invalid viewport size**: If viewport dimensions are 0 or negative, viewport units should resolve to 0
2. **Missing viewport size**: If `ViewportSize` is not initialized, log a warning and use fallback dimensions (e.g., 800x600)
3. **Conflicting inset values**: If both left+right or top+bottom are set with explicit width/height, the explicit dimension takes precedence

## Testing Strategy

### Unit Tests

1. Test `CSSLength::ToPx()` with various viewport unit values
2. Test `ConvertLength()` with vh/vw/vmin/vmax
3. Test absolute positioning calculations with various inset combinations

### Property-Based Tests

The property-based testing library to use: **Google Test with custom generators**

Each property test will:
- Generate random viewport dimensions (100-2000 px)
- Generate random CSS values (0-200 for viewport units)
- Generate random element configurations
- Verify the computed values match expected formulas

Property tests should run a minimum of 100 iterations each.

Test file location: `tests/property/layout/test_viewport_units_properties.cpp`

Each property-based test MUST be tagged with a comment referencing the correctness property:
```cpp
// **Feature: viewport-units-and-absolute-positioning, Property 1: Viewport height unit resolution**
TEST_F(ViewportUnitsPropertyTest, VhResolution) { ... }
```
