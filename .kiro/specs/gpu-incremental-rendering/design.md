# Design Document: GPU Incremental Rendering

## Overview

This feature implements GPU incremental rendering using Frame Buffer Objects (FBO) to dramatically reduce GPU usage during partial screen updates. The core idea is to use an FBO as a persistent render target that preserves content between frames, enabling true incremental rendering in GPU mode.

### Current Problem

OpenGL double-buffering swaps front and back buffers on each frame:
- Frame N: Render to back buffer B, display front buffer A
- SwapBuffers: A and B swap roles
- Frame N+1: Render to back buffer A (now back), display front buffer B
- **Problem**: Back buffer A contains Frame N-1 content, not Frame N

This means incremental updates show stale content from 2 frames ago, causing visual artifacts.

### Solution

Use an FBO as a persistent buffer:
1. Create FBO with color attachment (texture)
2. Render incrementally to FBO (content persists between frames)
3. Copy FBO to screen back buffer (simple blit operation)
4. SwapBuffers to display

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Render Pipeline                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │ Dirty Region │───▶│  FBO Render  │───▶│  Screen Blit │  │
│  │  Collection  │    │ (Incremental)│    │   (Full)     │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│         │                   │                   │           │
│         ▼                   ▼                   ▼           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │ RenderObject │    │     FBO      │    │ Back Buffer  │  │
│  │  NeedsPaint  │    │  (Persistent)│    │  (Swapped)   │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. FBOManager Class (New)

Manages FBO lifecycle and operations.

```cpp
class FBOManager {
public:
    FBOManager();
    ~FBOManager();
    
    // Initialize FBO with given dimensions
    bool Initialize(int width, int height, GrDirectContext* gr_context);
    
    // Resize FBO (recreates if dimensions changed)
    bool Resize(int width, int height);
    
    // Get Skia surface for rendering to FBO
    sk_sp<SkSurface> GetSurface() const;
    
    // Bind FBO for rendering
    void Bind();
    
    // Unbind FBO (bind default framebuffer)
    void Unbind();
    
    // Blit FBO content to screen
    void BlitToScreen(int screen_width, int screen_height);
    
    // Clear entire FBO
    void Clear(SkColor color);
    
    // Clear specific region of FBO
    void ClearRegion(const SkRect& region, SkColor color);
    
    // Check if FBO is valid
    bool IsValid() const;
    
private:
    GLuint fbo_id_ = 0;
    GLuint texture_id_ = 0;
    GLuint depth_rb_id_ = 0;  // Optional depth/stencil
    int width_ = 0;
    int height_ = 0;
    sk_sp<SkSurface> surface_;
    GrDirectContext* gr_context_ = nullptr;
};
```

### 2. Window Class Modifications

Add FBO support to existing Window class.

```cpp
class Window {
    // ... existing members ...
    
private:
    // FBO for incremental rendering
    std::unique_ptr<FBOManager> fbo_manager_;
    
    // Flag to enable/disable FBO incremental rendering
    bool use_fbo_incremental_ = true;
    
    // Methods
    void InitFBO();
    void RenderToFBO();
    void BlitFBOToScreen();
};
```

### 3. Render Flow Changes

```cpp
void Window::Render() {
    // ... existing setup ...
    
    if (actual_backend_ == RenderBackend::OPENGL && use_fbo_incremental_) {
        // GPU Incremental Mode
        RenderIncrementalGPU();
    } else if (actual_backend_ == RenderBackend::OPENGL) {
        // GPU Full Mode (fallback)
        RenderFullGPU();
    } else {
        // CPU Mode (unchanged)
        RenderIncrementalCPU();
    }
}

void Window::RenderIncrementalGPU() {
    // 1. Collect dirty regions
    CollectDirtyRectsFromRenderTree(cached_render_tree_.get());
    
    // 2. Get FBO surface
    SkCanvas* fbo_canvas = fbo_manager_->GetSurface()->getCanvas();
    
    // 3. For each dirty region, clear and repaint
    for (const auto& rect : dirty_rects_) {
        fbo_canvas->save();
        fbo_canvas->clipRect(rect.makeOutset(50, 50));
        
        // Clear the dirty region
        SkPaint clear_paint;
        clear_paint.setColor(clear_color);
        fbo_canvas->drawRect(rect.makeOutset(50, 50), clear_paint);
        
        // Repaint (clipped to dirty region)
        cached_render_tree_->Paint(fbo_canvas);
        
        fbo_canvas->restore();
    }
    
    // 4. Flush FBO rendering
    gr_context_->flush();
    
    // 5. Blit FBO to screen
    fbo_manager_->BlitToScreen(width, height);
    
    // 6. Swap buffers
    SDL_GL_SwapWindow(sdl_window_);
}
```

## Data Models

### FBO State

```cpp
struct FBOState {
    GLuint fbo_id;           // OpenGL FBO handle
    GLuint texture_id;       // Color attachment texture
    int width;               // FBO width in pixels
    int height;              // FBO height in pixels
    bool valid;              // Whether FBO is usable
};
```

### Dirty Region Info

```cpp
struct DirtyRegionInfo {
    std::vector<SkRect> regions;  // List of dirty rectangles
    bool force_full_repaint;      // Force full FBO repaint
    SkRect combined_bounds;       // Union of all dirty regions
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Static Scene Skip Rendering
*For any* render state where no render objects have `needs_paint_` set and no animations are active, the render function should return early without performing any draw operations.
**Validates: Requirements 1.3**

### Property 2: Dirty Region Contains Changed Objects
*For any* render object that has been marked as needing paint, the collected dirty region should contain that object's bounding rectangle (with appropriate margin for shadows/outlines).
**Validates: Requirements 1.4, 2.2**

### Property 3: Transform-Aware Dirty Regions
*For any* render object with a CSS transform, the dirty region calculation should use the transformed bounding box, not the original layout bounds.
**Validates: Requirements 3.2**

### Property 4: FBO Resize Consistency
*For any* window resize event, after the resize is processed, the FBO dimensions should match the new window dimensions.
**Validates: Requirements 2.3, 5.2**

### Property 5: Fallback on FBO Failure
*For any* FBO initialization or operation failure, the renderer should fall back to full-screen rendering mode without crashing.
**Validates: Requirements 4.3**

## Error Handling

### FBO Creation Failure
- Log warning message
- Set `use_fbo_incremental_ = false`
- Fall back to full-screen GPU rendering

### FBO Resize Failure
- Attempt to recreate FBO
- If still fails, fall back to full-screen mode
- Log error with dimensions

### OpenGL Errors
- Check `glGetError()` after critical operations
- Log errors with context
- Graceful degradation to full-screen mode

## Testing Strategy

### Dual Testing Approach

This feature requires both unit tests and property-based tests:

1. **Unit Tests**: Verify specific examples and edge cases
   - FBO creation with valid dimensions
   - FBO resize behavior
   - Fallback on failure

2. **Property-Based Tests**: Verify universal properties
   - Use fast-check or similar library
   - Generate random render tree states
   - Verify dirty region properties hold

### Property-Based Testing Library

Use **fast-check** (JavaScript) for property-based testing, as the test infrastructure already uses JavaScript/TypeScript.

### Test Configuration

- Minimum 100 iterations per property test
- Each test tagged with property reference: `**Feature: gpu-incremental-rendering, Property N: description**`

### Test Cases

1. **FBO Lifecycle Tests**
   - Create FBO with various dimensions
   - Resize FBO multiple times
   - Destroy and recreate FBO

2. **Dirty Region Tests**
   - Single dirty object
   - Multiple dirty objects
   - Overlapping dirty regions
   - Objects with transforms

3. **Integration Tests**
   - Animation with FBO rendering
   - Window resize during animation
   - Mode switching (GPU ↔ CPU)

4. **Stress Tests**
   - Rapid resize events
   - Many simultaneous animations
   - Large dirty regions
