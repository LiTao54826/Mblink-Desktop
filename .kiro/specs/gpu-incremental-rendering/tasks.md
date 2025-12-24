# Implementation Plan

- [-] 1. Create FBOManager class


  - [x] 1.1 Create FBOManager header file with class declaration

    - Define FBOManager class in `core/render/fbo_manager.h`
    - Include OpenGL headers and Skia headers
    - Declare Initialize, Resize, GetSurface, Bind, Unbind, BlitToScreen, Clear methods
    - _Requirements: 2.1_

  - [x] 1.2 Implement FBOManager initialization and destruction

    - Create FBO with glGenFramebuffers
    - Create color texture attachment with glGenTextures
    - Attach texture to FBO with glFramebufferTexture2D
    - Check FBO completeness with glCheckFramebufferStatus
    - Create Skia surface wrapping the FBO
    - _Requirements: 2.1, 4.3_

  - [x] 1.3 Implement FBO resize functionality

    - Delete old texture and FBO if dimensions changed
    - Recreate with new dimensions
    - Recreate Skia surface

    - _Requirements: 2.3, 5.2_

  - [x] 1.4 Implement BlitToScreen using glBlitFramebuffer
    - Bind FBO as read framebuffer
    - Bind default framebuffer (0) as draw framebuffer
    - Call glBlitFramebuffer with GL_COLOR_BUFFER_BIT
    - _Requirements: 3.4_
  - [x] 1.5 Write unit tests for FBOManager

    - Test FBO creation with valid dimensions
    - Test FBO resize
    - Test fallback on failure
    - _Requirements: 4.3_

- [-] 2. Integrate FBOManager into Window class


  - [x] 2.1 Add FBOManager member and initialization

    - Add `std::unique_ptr<FBOManager> fbo_manager_` to Window
    - Add `bool use_fbo_incremental_` flag
    - Initialize FBO after OpenGL context creation
    - _Requirements: 2.1_

  - [x] 2.2 Implement RenderIncrementalGPU method

    - Collect dirty regions from render tree
    - Get FBO canvas from FBOManager
    - For each dirty region: clip, clear, repaint
    - Flush Skia context
    - Call BlitToScreen
    - Call SDL_GL_SwapWindow
    - _Requirements: 1.4, 2.2_
  - [ ] 2.3 Write property test for dirty region collection
    - **Property 2: Dirty Region Contains Changed Objects**
    - **Validates: Requirements 1.4, 2.2**


  - [x] 2.4 Update Render method to use FBO incremental mode
    - Check if GPU mode and FBO is valid
    - Call RenderIncrementalGPU instead of full repaint
    - Keep fallback to full repaint if FBO invalid
    - _Requirements: 2.1, 4.3_



- [-] 3. Handle window resize with FBO
  - [x] 3.1 Update OnResize to resize FBO
    - Call fbo_manager_->Resize with new dimensions
    - Clear FBO after resize


    - Set force_full_repaint flag
    - _Requirements: 5.1, 5.2_
  - [x] 3.2 Clear both FBO and screen buffers on resize

    - Clear FBO with background color
    - Clear screen back buffer
    - Swap and clear again for double buffer
    - _Requirements: 5.1_
  - [ ] 3.3 Write property test for FBO resize consistency
    - **Property 4: FBO Resize Consistency**
    - **Validates: Requirements 2.3, 5.2**

- [ ] 4. Implement transform-aware dirty regions
  - [ ] 4.1 Update dirty region collection to use transformed bounds
    - Check if render object has transform
    - Use GetBoundingRect which already calculates transformed bounds
    - Ensure margin accounts for transform expansion
    - _Requirements: 3.2_
  - [ ] 4.2 Write property test for transform-aware dirty regions
    - **Property 3: Transform-Aware Dirty Regions**
    - **Validates: Requirements 3.2**

- [ ] 5. Implement fallback and error handling
  - [ ] 5.1 Add fallback to full-screen rendering on FBO failure
    - Check FBO validity before incremental render
    - If invalid, use full-screen GPU render path
    - Log warning when falling back
    - _Requirements: 4.3_
  - [ ] 5.2 Add debug logging for dirty regions
    - Check LIGHTUI_DEBUG_RENDER environment variable
    - Log dirty region count and bounds
    - Optionally draw debug overlay showing dirty regions
    - _Requirements: 4.1, 4.2_
  - [ ] 5.3 Write property test for fallback behavior
    - **Property 5: Fallback on FBO Failure**
    - **Validates: Requirements 4.3**

- [ ] 6. Optimize static scene detection
  - [ ] 6.1 Ensure render skips when no changes
    - Check needs_repaint_, has_active_animations, dirty_rects_
    - Return early if all are false/empty
    - Don't call BlitToScreen if nothing changed
    - _Requirements: 1.3_
  - [ ] 6.2 Write property test for static scene skip
    - **Property 1: Static Scene Skip Rendering**
    - **Validates: Requirements 1.3**

- [ ] 7. Checkpoint - Make sure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 8. Integration testing and performance validation
  - [ ] 8.1 Test with test_single_animation.js
    - Run animation test
    - Verify GPU usage is reduced
    - Check for visual artifacts
    - _Requirements: 1.1_
  - [ ] 8.2 Test with test_css_animation.js (6 animations)
    - Run multi-animation test
    - Verify GPU usage is acceptable
    - Check all animations render correctly
    - _Requirements: 1.2_
  - [ ] 8.3 Test window resize during animation
    - Resize window while animations running
    - Verify no black areas or flickering
    - Verify FBO recreated correctly
    - _Requirements: 5.1, 5.2, 5.3_

- [ ] 9. Final Checkpoint - Make sure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
