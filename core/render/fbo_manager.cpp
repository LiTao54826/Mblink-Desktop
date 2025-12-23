/**
 * @file fbo_manager.cpp
 * @brief FBO Manager implementation for GPU incremental rendering
 */

#include "fbo_manager.h"
#include <iostream>
#include <SDL3/SDL.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
// OpenGL FBO 扩展函数类型定义
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

// OpenGL 常量定义
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// 全局函数指针
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
static PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = nullptr;

static bool g_gl_extensions_loaded = false;

static bool LoadGLExtensions() {
    if (g_gl_extensions_loaded) return true;
    
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress("glRenderbufferStorage");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBlitFramebuffer");
    
    if (!glGenFramebuffers || !glDeleteFramebuffers || !glBindFramebuffer ||
        !glCheckFramebufferStatus || !glFramebufferTexture2D ||
        !glGenRenderbuffers || !glDeleteRenderbuffers || !glBindRenderbuffer ||
        !glRenderbufferStorage || !glFramebufferRenderbuffer || !glBlitFramebuffer) {
        std::cerr << "[FBOManager] Failed to load OpenGL FBO extensions" << std::endl;
        return false;
    }
    
    g_gl_extensions_loaded = true;
    std::cout << "[FBOManager] OpenGL FBO extensions loaded successfully" << std::endl;
    return true;
}

#else
// 非 Windows 平台，假设 OpenGL 扩展已经可用
#include <GL/gl.h>
#include <GL/glext.h>
static bool LoadGLExtensions() { return true; }
#endif

#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/core/SkColorSpace.h"

namespace lightui {

FBOManager::FBOManager() = default;

FBOManager::~FBOManager() {
    Destroy();
}

bool FBOManager::Initialize(int width, int height, GrDirectContext* gr_context) {
    if (width <= 0 || height <= 0) {
        std::cerr << "[FBOManager] Invalid dimensions: " << width << "x" << height << std::endl;
        return false;
    }

    if (!gr_context) {
        std::cerr << "[FBOManager] Invalid GrDirectContext" << std::endl;
        return false;
    }

    // 加载 OpenGL 扩展
    if (!LoadGLExtensions()) {
        std::cerr << "[FBOManager] Failed to load OpenGL extensions" << std::endl;
        return false;
    }

    // Store parameters
    width_ = width;
    height_ = height;
    gr_context_ = gr_context;

    // Create FBO
    if (!CreateFBO()) {
        std::cerr << "[FBOManager] Failed to create FBO" << std::endl;
        Destroy();
        return false;
    }

    // Create Skia surface
    if (!CreateSkiaSurface()) {
        std::cerr << "[FBOManager] Failed to create Skia surface" << std::endl;
        Destroy();
        return false;
    }

    valid_ = true;
    std::cout << "[FBOManager] Initialized " << width << "x" << height << " FBO" << std::endl;
    return true;
}

bool FBOManager::CreateFBO() {
    // Generate FBO
    glGenFramebuffers(1, &fbo_id_);
    if (!CheckGLError("glGenFramebuffers")) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    if (!CheckGLError("glBindFramebuffer")) {
        return false;
    }

    // Generate texture for color attachment
    glGenTextures(1, &texture_id_);
    if (!CheckGLError("glGenTextures")) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    if (!CheckGLError("glBindTexture")) {
        return false;
    }

    // Allocate texture storage (RGBA8)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (!CheckGLError("glTexImage2D")) {
        return false;
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);
    if (!CheckGLError("glFramebufferTexture2D")) {
        return false;
    }

    // Create depth/stencil renderbuffer (needed for some Skia operations)
    glGenRenderbuffers(1, &depth_stencil_rb_);
    if (!CheckGLError("glGenRenderbuffers")) {
        return false;
    }

    glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_rb_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    if (!CheckGLError("glRenderbufferStorage")) {
        return false;
    }

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_rb_);
    if (!CheckGLError("glFramebufferRenderbuffer")) {
        return false;
    }

    // Check FBO completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FBOManager] FBO incomplete, status: 0x" << std::hex << status << std::dec << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return true;
}

bool FBOManager::CreateSkiaSurface() {
    if (!gr_context_ || fbo_id_ == 0 || texture_id_ == 0) {
        return false;
    }

    // Create GrBackendRenderTarget wrapping our FBO
    GrGLFramebufferInfo fb_info;
    fb_info.fFBOID = fbo_id_;
    fb_info.fFormat = GL_RGBA8;

    auto backend_rt = GrBackendRenderTargets::MakeGL(width_, height_, 0, 8, fb_info);

    // Create Skia surface from backend render target
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        gr_context_,
        backend_rt,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr  // surface props
    );

    if (!surface_) {
        std::cerr << "[FBOManager] Failed to create Skia surface from FBO" << std::endl;
        return false;
    }

    return true;
}

void FBOManager::Destroy() {
    // Release Skia surface first
    surface_.reset();

    // Delete OpenGL resources
    if (depth_stencil_rb_ != 0) {
        glDeleteRenderbuffers(1, &depth_stencil_rb_);
        depth_stencil_rb_ = 0;
    }

    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }

    if (fbo_id_ != 0) {
        glDeleteFramebuffers(1, &fbo_id_);
        fbo_id_ = 0;
    }

    width_ = 0;
    height_ = 0;
    valid_ = false;
}

bool FBOManager::Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        std::cerr << "[FBOManager] Invalid resize dimensions: " << width << "x" << height << std::endl;
        return false;
    }

    // No change needed
    if (width == width_ && height == height_ && valid_) {
        return true;
    }

    std::cout << "[FBOManager] Resizing from " << width_ << "x" << height_ 
              << " to " << width << "x" << height << std::endl;

    // Store context
    GrDirectContext* ctx = gr_context_;

    // Destroy old resources
    Destroy();

    // Reinitialize with new dimensions
    return Initialize(width, height, ctx);
}

void FBOManager::Bind() {
    if (fbo_id_ != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    }
}

void FBOManager::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBOManager::BlitToScreen(int screen_width, int screen_height) {
    if (!valid_ || fbo_id_ == 0) {
        return;
    }

    // Bind FBO as read framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_id_);
    
    // Bind default framebuffer (screen) as draw framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Blit from FBO to screen
    // Note: FBO and screen may have different sizes, use GL_LINEAR for scaling
    glBlitFramebuffer(
        0, 0, width_, height_,           // Source rect (FBO)
        0, 0, screen_width, screen_height, // Dest rect (screen)
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
    );

    if (!CheckGLError("glBlitFramebuffer")) {
        std::cerr << "[FBOManager] BlitToScreen failed" << std::endl;
    }

    // Restore default framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBOManager::Clear(SkColor color) {
    if (!surface_) {
        return;
    }

    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        canvas->clear(color);
    }
}

void FBOManager::ClearRegion(const SkRect& region, SkColor color) {
    if (!surface_) {
        return;
    }

    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        canvas->save();
        canvas->clipRect(region);
        canvas->clear(color);
        canvas->restore();
    }
}

void FBOManager::Flush() {
    if (gr_context_) {
        gr_context_->flush();
    }
}

bool FBOManager::CheckGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[FBOManager] OpenGL error after " << operation 
                  << ": 0x" << std::hex << error << std::dec << std::endl;
        return false;
    }
    return true;
}

} // namespace lightui
