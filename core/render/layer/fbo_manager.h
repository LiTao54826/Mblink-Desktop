/**
 * @file fbo_manager.h
 * @brief FBO (Frame Buffer Object) Manager for GPU incremental rendering
 *
 * This class manages an OpenGL FBO that serves as a persistent render target,
 * enabling true incremental rendering in GPU mode. The FBO content persists
 * between frames, allowing partial updates without the artifacts caused by
 * OpenGL double-buffering.
 */

#pragma once

#include <memory>
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkRect.h"
#include "include/gpu/ganesh/GrDirectContext.h"

// 前向声明 OpenGL 类型
typedef unsigned int GLuint;
typedef unsigned int GLenum;

namespace mbink {

/**
 * @brief Manages FBO lifecycle and operations for GPU incremental rendering
 *
 * The FBOManager creates and maintains an off-screen framebuffer that:
 * 1. Preserves content between frames (unlike double-buffered back buffer)
 * 2. Allows incremental updates to specific dirty regions
 * 3. Can be blitted to the screen back buffer for display
 */
class FBOManager {
public:
    FBOManager();
    ~FBOManager();

    // Disable copy
    FBOManager(const FBOManager&) = delete;
    FBOManager& operator=(const FBOManager&) = delete;

    /**
     * @brief Initialize FBO with given dimensions
     * @param width FBO width in pixels
     * @param height FBO height in pixels
     * @param gr_context Skia GPU context for creating surface
     * @return true if initialization succeeded
     */
    bool Initialize(int width, int height, GrDirectContext* gr_context);

    /**
     * @brief Resize FBO (recreates if dimensions changed)
     * @param width New width in pixels
     * @param height New height in pixels
     * @return true if resize succeeded
     */
    bool Resize(int width, int height);

    /**
     * @brief Get Skia surface for rendering to FBO
     * @return Skia surface, or nullptr if FBO is invalid
     */
    sk_sp<SkSurface> GetSurface() const { return surface_; }

    /**
     * @brief Get canvas for rendering to FBO
     * @return Skia canvas, or nullptr if FBO is invalid
     */
    SkCanvas* GetCanvas() const { return surface_ ? surface_->getCanvas() : nullptr; }

    /**
     * @brief Bind FBO for rendering
     */
    void Bind();

    /**
     * @brief Unbind FBO (bind default framebuffer)
     */
    void Unbind();

    /**
     * @brief Blit FBO content to screen back buffer
     * @param screen_width Screen width in pixels
     * @param screen_height Screen height in pixels
     */
    void BlitToScreen(int screen_width, int screen_height);

    /**
     * @brief Clear entire FBO with specified color
     * @param color Clear color
     */
    void Clear(SkColor color);

    /**
     * @brief Clear specific region of FBO
     * @param region Region to clear
     * @param color Clear color
     */
    void ClearRegion(const SkRect& region, SkColor color);

    /**
     * @brief Check if FBO is valid and ready for use
     * @return true if FBO is valid
     */
    bool IsValid() const { return valid_; }

    /**
     * @brief Get FBO width
     * @return Width in pixels
     */
    int GetWidth() const { return width_; }

    /**
     * @brief Get FBO height
     * @return Height in pixels
     */
    int GetHeight() const { return height_; }

    /**
     * @brief Flush any pending Skia operations to the FBO
     */
    void Flush();

private:
    /**
     * @brief Create OpenGL FBO and texture
     * @return true if creation succeeded
     */
    bool CreateFBO();

    /**
     * @brief Create Skia surface wrapping the FBO
     * @return true if creation succeeded
     */
    bool CreateSkiaSurface();

    /**
     * @brief Destroy FBO and release resources
     */
    void Destroy();

    /**
     * @brief Check for OpenGL errors
     * @param operation Description of the operation for logging
     * @return true if no errors
     */
    bool CheckGLError(const char* operation);

private:
    GLuint fbo_id_ = 0;           ///< OpenGL FBO handle
    GLuint texture_id_ = 0;       ///< Color attachment texture
    GLuint depth_stencil_rb_ = 0; ///< Depth/stencil renderbuffer (optional)
    
    int width_ = 0;               ///< FBO width in pixels
    int height_ = 0;              ///< FBO height in pixels
    bool valid_ = false;          ///< Whether FBO is usable
    
    sk_sp<SkSurface> surface_;    ///< Skia surface for rendering
    GrDirectContext* gr_context_ = nullptr; ///< Skia GPU context (not owned)
};

} // namespace mbink
