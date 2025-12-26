# Implementation Plan

- [x] 1. Create DragEvent class



  - [x] 1.1 Create `core/dom/drag_event.h` with DragEvent class inheriting from MouseEvent

    - Add `dataTransfer_` member variable
    - Add `GetDataTransfer()` method
    - Define event type constants (DRAG_START, DRAG, DRAG_END, DRAG_ENTER, DRAG_LEAVE, DRAG_OVER, DROP)
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 3.1_

  - [x] 1.2 Create `core/dom/drag_event.cpp` with DragEvent implementation

    - Implement constructor with DataTransfer parameter
    - Implement GetDataTransfer() method
    - _Requirements: 3.1, 8.1, 8.2, 8.3, 8.4_
  - [ ]* 1.3 Write property test for DragEvent properties
    - **Property 8: DragEvent contains all required properties**
    - **Validates: Requirements 8.1, 8.2, 8.3, 8.4**

- [x] 2. Enhance DragManager for HTML5 draggable attribute


  - [x] 2.1 Add `IsElementDraggable()` method to DragManager


    - Check `draggable` attribute first (takes precedence)
    - Fall back to `drag` attribute for backward compatibility
    - Handle default behavior for different element types
    - _Requirements: 1.1, 1.2, 1.3, 7.5_
  - [x] 2.2 Modify `GetDragMode()` to support `draggable` attribute

    - Map `draggable="true"` to DragMode::DragDrop
    - Map `draggable="false"` to DragMode::None
    - _Requirements: 1.1, 1.2, 1.4_
  - [ ]* 2.3 Write property test for draggable attribute parsing
    - **Property 1: Draggable attribute parsing**
    - **Validates: Requirements 1.1, 1.2**
  - [ ]* 2.4 Write property test for attribute precedence
    - **Property 7: Attribute precedence**
    - **Validates: Requirements 7.5**

- [x] 3. Implement drag threshold detection

  - [x] 3.1 Add DragState enum and state management to DragManager

    - Add `DragState` enum (None, Detecting, Dragging)
    - Add `drag_state_` member variable
    - Add `detect_start_x_`, `detect_start_y_` for threshold calculation
    - _Requirements: 6.1_
  - [x] 3.2 Implement `CheckDragThreshold()` method

    - Calculate distance from detection start position
    - Return true if distance >= DRAG_THRESHOLD (4 pixels)
    - _Requirements: 6.2, 6.3_
  - [x] 3.3 Modify `StartDragDetection()` to enter Detecting state

    - Store initial mouse position
    - Set state to Detecting instead of immediately starting drag
    - _Requirements: 6.1_
  - [x] 3.4 Modify `UpdateDrag()` to check threshold before starting

    - Check threshold in Detecting state
    - Transition to Dragging state and dispatch dragstart when threshold reached
    - _Requirements: 6.3_
  - [ ]* 3.5 Write property test for drag threshold behavior
    - **Property 6: Drag threshold behavior**
    - **Validates: Requirements 6.2, 6.3**

- [x] 4. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Implement standard HTML5 drag events
  - [x] 5.1 Add `dragenter` event dispatch in `UpdateDragHoverChain()`

    - Dispatch dragenter when element enters hover chain
    - Replace or supplement existing dragover logic
    - _Requirements: 2.4_

  - [x] 5.2 Add `dragleave` event dispatch in `UpdateDragHoverChain()`
    - Dispatch dragleave when element leaves hover chain
    - Replace or supplement existing dragout logic
    - _Requirements: 2.5_

  - [x] 5.3 Rename `dragdrop` to `drop` in `EndDrag()`
    - Change event type from "dragdrop" to "drop"
    - Maintain backward compatibility by also dispatching "dragdrop" if needed
    - _Requirements: 2.7_
  - [x] 5.4 Update `SendDragEvents()` to use DragEvent instead of MouseEvent

    - Create DragEvent with DataTransfer
    - Pass DataTransfer to all drag events
    - _Requirements: 3.1_

- [x] 6. Enhance DataTransfer for full W3C compliance



  - [x] 6.1 Verify and enhance `SetData()` / `GetData()` methods

    - Ensure format normalization (lowercase)
    - Handle "text" -> "text/plain" and "url" -> "text/uri-list" aliases
    - _Requirements: 3.2, 3.3_

  - [x] 6.2 Enhance `GetTypes()` to return proper format list
    - Return vector of all stored formats
    - Maintain insertion order if possible
    - _Requirements: 3.5_
  - [ ]* 6.3 Write property test for DataTransfer round-trip
    - **Property 2: DataTransfer setData/getData round-trip**
    - **Validates: Requirements 3.2, 3.3**
  - [ ]* 6.4 Write property test for DataTransfer types
    - **Property 3: DataTransfer types contains all formats**
    - **Validates: Requirements 3.5**



- [x] 7. Implement effect compatibility logic

  - [x] 7.1 Add effect compatibility checking in DragManager

    - Create `IsEffectCompatible(effectAllowed, dropEffect)` helper
    - Define compatibility rules per W3C spec
    - _Requirements: 4.3_
  - [x] 7.2 Enforce effect compatibility in `UpdateDrag()`

    - Check compatibility when dropEffect is set
    - Set dropEffect to "none" if incompatible
    - _Requirements: 4.3_
  - [x] 7.3 Handle drop cancellation when dropEffect is "none"

    - Skip drop event dispatch if dropEffect is "none"
    - Still dispatch dragend to source
    - _Requirements: 4.4_
  - [ ]* 7.4 Write property test for effect compatibility
    - **Property 4: Effect compatibility enforcement**
    - **Validates: Requirements 4.3**

- [x] 8. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Verify event propagation for drag events

  - [x] 9.1 Ensure DragEvent uses standard event propagation

    - Verify capture phase propagation works
    - Verify bubble phase propagation works
    - _Requirements: 5.1, 5.2, 5.3_
  - [x] 9.2 Implement preventDefault handling for dragover

    - Track if preventDefault was called on dragover
    - Only allow drop if preventDefault was called
    - _Requirements: 5.5_
  - [ ]* 9.3 Write property test for event propagation order
    - **Property 5: Event propagation order**
    - **Validates: Requirements 5.1, 5.2, 5.3**

- [x] 10. Create JavaScript DataTransfer binding
  - [x] 10.1 Create `bindings/quickjs/js_data_transfer.h` header
    - Declare JSDataTransfer class
    - Declare Register() and all accessor methods
    - _Requirements: 9.1_
  - [x] 10.2 Create `bindings/quickjs/js_data_transfer.cpp` implementation
    - Implement setData/getData/clearData bindings
    - Implement types property getter
    - Implement effectAllowed/dropEffect getters and setters
    - _Requirements: 9.2, 9.3, 9.4, 9.5_
  - [x] 10.3 Register DataTransfer in QuickJS runtime
    - Add JSDataTransfer::Register() call in runtime initialization
    - Ensure DragEvent.dataTransfer returns JS DataTransfer object
    - _Requirements: 9.1_
  - [ ]* 10.4 Write property test for JS DataTransfer round-trip
    - **Property 9: JavaScript DataTransfer round-trip**
    - **Validates: Requirements 9.2, 9.3**

- [x] 11. Update EventLoop integration


  - [x] 11.1 Modify `HandleMouseEventForDOM()` to use new DragManager API

    - Call `StartDragDetection()` on mousedown for draggable elements
    - Call `CheckDragThreshold()` and `UpdateDrag()` on mousemove
    - Call `EndDrag()` on mouseup
    - _Requirements: 6.1, 6.4_
  - [x] 11.2 Handle click fallback when threshold not reached

    - Dispatch click event if mouseup before threshold
    - Reset drag detection state
    - _Requirements: 6.4_

- [x] 12. Update CMakeLists.txt


  - [x] 12.1 Add new source files to core/dom CMakeLists.txt

    - Add drag_event.h and drag_event.cpp
    - _Requirements: N/A (build configuration)_
  - [x] 12.2 Add new source files to bindings/quickjs CMakeLists.txt

    - Add js_data_transfer.h and js_data_transfer.cpp
    - _Requirements: N/A (build configuration)_

- [x] 13. Final Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
