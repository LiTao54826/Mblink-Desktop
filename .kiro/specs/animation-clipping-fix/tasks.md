# Implementation Plan: Animation Clipping Fix

- [x] 1. Create AnimationBoundsCalculator core component
  - [x] 1.1 Create animation_bounds_calculator.h with AnimationBounds struct and class declaration
    - Define AnimationBounds struct with bounds, offset, and needs_expansion fields
    - Declare static methods for Calculate, CalculateRotationBounds, CalculateTranslationBounds, CalculateScaleBounds
    - _Requirements: 1.1, 1.2, 1.3, 1.4_
  - [x] 1.2 Implement CalculateRotationBounds method
    - Calculate circumscribed circle bounds for rotation
    - Handle transform origin offset
    - Return AnimationBounds with correct offset
    - _Requirements: 3.1, 3.2, 3.3_
  - [x] 1.3 Write property test for rotation bounds
    - **Property 1: Rotation bounds contain all rotated corners**
    - **Validates: Requirements 1.2, 3.1, 3.2**
  - [x] 1.4 Implement CalculateTranslationBounds method
    - Calculate union of all translation positions
    - Handle percentage values relative to element size
    - _Requirements: 4.1, 4.2, 4.3_
  - [x] 1.5 Write property test for translation bounds
    - **Property 2: Translation bounds include all positions**
    - **Validates: Requirements 1.3, 4.1, 4.2, 4.3**
  - [x] 1.6 Implement CalculateScaleBounds method
    - Calculate bounds for maximum scale value
    - Handle transform origin for scale
    - _Requirements: 1.4_
  - [x] 1.7 Write property test for scale bounds
    - **Property 6: Scale bounds accommodate maximum scale**
    - **Validates: Requirements 1.4**

- [x] 2. Implement keyframe transform extraction
  - [x] 2.1 Implement ExtractTransforms method
    - Parse transform strings from keyframes
    - Convert to SkMatrix for each keyframe
    - Handle transform-origin
    - _Requirements: 4.2_
  - [x] 2.2 Implement CalculateUnionBounds method
    - Apply each transform to element corners
    - Calculate union bounding box of all transformed positions
    - _Requirements: 1.1, 3.4, 4.4_
  - [x] 2.3 Write property test for combined transform bounds
    - **Property 3: Combined transform bounds are correct**
    - **Validates: Requirements 1.1, 3.4, 4.4**
  - [x] 2.4 Implement main Calculate method
    - Get keyframes by animation name
    - Extract transforms and calculate union bounds
    - _Requirements: 1.1_

- [x] 3. Checkpoint - Ensure AnimationBoundsCalculator tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. Integrate with LayerTreeBuilder
  - [x] 4.1 Add CalculateAnimationBounds helper method to LayerTreeBuilder
    - Get animation info from RenderObject style
    - Look up keyframes and calculate bounds
    - _Requirements: 1.1, 2.1, 2.2_
  - [x] 4.2 Modify UpdateLayerBounds to use animation bounds
    - Call CalculateAnimationBounds for animated elements
    - Expand layer bounds when animation requires it
    - Store animation bounds in CompositorLayer
    - _Requirements: 1.1, 2.1, 2.2_
  - [x] 4.3 Write property test for dynamic bounds update
    - **Property 5: Dynamic bounds update correctly**
    - **Validates: Requirements 2.1, 2.2, 2.3**

- [x] 5. Modify CompositorLayer for animation bounds storage
  - [x] 5.1 Add animation_bounds_ member to CompositorLayer
    - Add std::optional<AnimationBounds> member
    - Add getter/setter methods
    - _Requirements: 1.1_

- [x] 6. Modify Rasterizer for expanded bounds
  - [x] 6.1 Update RasterizeLayer to handle animation bounds offset
    - Check for animation bounds on layer
    - Apply offset to canvas translation
    - Ensure bitmap allocation uses expanded bounds
    - _Requirements: 5.1, 5.2, 5.3_
  - [x] 6.2 Write property test for rasterization correctness
    - **Property 4: Rasterized content is not clipped**
    - **Validates: Requirements 5.1, 5.2, 5.3, 5.4**

- [x] 7. Checkpoint - Ensure integration tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Update CMakeLists and add to build
  - [x] 8.1 Add animation_bounds_calculator.cpp/h to compositor CMakeLists.txt
    - _Requirements: N/A (build configuration)_

- [x] 9. Final Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
