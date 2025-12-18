# Implementation Plan

## CSS Media and Layout Properties (Phase 2)

- [x] 1. Extend ComputedStyle with new CSS properties







  - [x] 1.1 Add object-fit, object-position fields to ComputedStyle


    - Add `std::string object_fit = "fill"` field
    - Add `std::string object_position = "50% 50%"` field
    - Location: `core/render/render_object.h`
    - _Requirements: 1.1-1.6, 2.1-2.5_


  - [x] 1.2 Add aspect-ratio field to ComputedStyle


    - Add `AspectRatio aspect_ratio` struct with `is_auto` and `ratio` fields
    - Location: `core/render/render_object.h`
    - _Requirements: 3.1-3.7_



  - [x] 1.3 Add list-style fields to ComputedStyle

    - Add `std::string list_style_type = "disc"` field
    - Add `std::string list_style_position = "outside"` field
    - Add `std::string list_style_image` field
    - Location: `core/render/render_object.h`
    - _Requirements: 4.1-4.15_

- [x] 2. Implement object-fit and object-position parsing






  - [x] 2.1 Add object-fit parsing in StyleResolver



    - Parse values: fill, contain, cover, none, scale-down
    - Location: `core/render/style_resolver.cpp`
    - _Requirements: 1.1-1.6_


  - [x] 2.2 Add object-position parsing in StyleResolver


    - Parse keyword values: center, top, bottom, left, right, and combinations
    - Parse percentage values: 50% 50%
    - Parse length values: 10px 20px
    - Parse mixed values: center 10px
    - _Requirements: 2.1-2.5_

  - [x] 2.3 Write property test for object-fit/position parsing



    - **Property 1: Object Fit Contain Preserves Aspect Ratio**
    - **Property 3: Object Position Centers by Default**
    - **Validates: Requirements 1.2, 2.5**

- [x] 3. Implement object-fit and object-position rendering




  - [x] 3.1 Add CalculateObjectFit helper function



    - Calculate source and destination rectangles based on object-fit
    - Handle all five object-fit values
    - Location: `core/render/render_object.cpp` or new `image_fit.cpp`
    - _Requirements: 1.1-1.5_


  - [x] 3.2 Add ParseObjectPosition helper function

    - Parse position string to x/y offsets
    - Handle keywords, percentages, and lengths
    - _Requirements: 2.1-2.4_

  - [x] 3.3 Apply object-fit/position in RenderImage::Paint



    - Use CalculateObjectFit to get source/dest rects
    - Draw image with calculated rectangles
    - Location: `core/render/render_object.cpp` (RenderInlineBlock or image rendering)
    - _Requirements: 1.1-1.6, 2.1-2.5_


  - [x] 3.4 Write property test for object-fit rendering


    - **Property 2: Object Fit Cover Fills Container**
    - **Validates: Requirements 1.3**

- [x] 4. Implement aspect-ratio parsing and layout






  - [x] 4.1 Add aspect-ratio parsing in StyleResolver



    - Parse ratio values: 16 / 9, 4/3, 1
    - Parse auto keyword
    - Parse auto with ratio: auto 16 / 9
    - Location: `core/render/style_resolver.cpp`
    - _Requirements: 3.1-3.3_

  - [x] 4.2 Apply aspect-ratio in layout calculation



    - Calculate missing dimension when only one is specified
    - Ignore aspect-ratio when both dimensions are set
    - Location: `core/render/render_object.cpp` (UpdateLayoutStyle) or `core/layout/native_layout_engine.cpp`
    - _Requirements: 3.4-3.7_



  - [x] 4.3 Write property tests for aspect-ratio

    - **Property 4: Aspect Ratio Calculates Missing Dimension**
    - **Property 5: Aspect Ratio Ignored When Both Dimensions Set**
    - **Validates: Requirements 3.4-3.6**

- [x] 5. Checkpoint - Ensure object-fit and aspect-ratio tests pass






  - Ensure all tests pass, ask the user if questions arise.


- [x] 6. Implement list-style parsing





  - [x] 6.1 Add list-style-type parsing in StyleResolver



    - Parse values: disc, circle, square, decimal, decimal-leading-zero, lower-roman, upper-roman, lower-alpha, upper-alpha, none
    - Add to inheritable_properties_ set
    - Location: `core/render/style_resolver.cpp`
    - _Requirements: 4.1-4.10_


  - [x] 6.2 Add list-style-position parsing in StyleResolver


    - Parse values: inside, outside
    - Add to inheritable_properties_ set
    - _Requirements: 4.11-4.12_


  - [x] 6.3 Add list-style-image parsing in StyleResolver


    - Parse url(...) values
    - Add to inheritable_properties_ set
    - _Requirements: 4.13_

  - [x] 6.4 Add list-style shorthand parsing



    - Parse combined values in any order
    - _Requirements: 4.14_

- [x] 7. Implement list marker rendering






  - [x] 7.1 Add Roman numeral conversion utilities



    - Implement ToLowerRoman and ToUpperRoman functions
    - Location: `core/render/list_marker.cpp` (new file) or `core/utils/`
    - _Requirements: 4.6, 4.7_


  - [x] 7.2 Add PaintListMarker method


    - Draw disc, circle, square markers
    - Draw decimal, roman, alpha markers
    - Handle list-style-position (inside/outside)
    - Location: `core/render/render_object.cpp`
    - _Requirements: 4.1-4.12_


  - [x] 7.3 Integrate list marker rendering in RenderBlock::Paint


    - Call PaintListMarker for <li> elements
    - Track item index within parent list
    - _Requirements: 4.1-4.12_


  - [x] 7.4 Add list-style-image support


    - Load and render custom marker images
    - Fall back to list-style-type if image fails
    - _Requirements: 4.13_


  - [x] 7.5 Write property tests for list-style


    - **Property 6: List Style Type Inheritance**
    - **Property 7: List Style None Hides Marker**
    - **Property 8: Decimal List Markers Sequential**
    - **Validates: Requirements 4.4, 4.10, 4.15**

- [x] 8. Final Checkpoint - Ensure all tests pass






  - Ensure all tests pass, ask the user if questions arise.
