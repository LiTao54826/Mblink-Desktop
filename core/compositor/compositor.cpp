/**
 * @file compositor.cpp
 * @brief GPU 合成器实现
 */

#include "compositor.h"
#include "core/render/objects/render_object.h"
#include "core/dom/element.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImageInfo.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

// OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

// OpenGL 类型定义
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// OpenGL 扩展函数指针类型
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX3FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

// OpenGL 常量
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

namespace mbink {

inline bool IsAnimFinalDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_FINAL") != nullptr);
    return enabled;
}

inline bool ShouldLogAnimFinalLayer(const CompositorLayer* layer) {
    if (!layer) {
        return false;
    }
    RenderObject* obj = layer->GetRenderObject();
    return obj != nullptr;
}

inline bool LogTransformChanged(int layer_id, float opacity, const SkMatrix& transform) {
    struct LastValue {
        bool initialized = false;
        float opacity = 0.0f;
        float m00 = 0.0f;
        float m01 = 0.0f;
        float m02 = 0.0f;
        float m10 = 0.0f;
        float m11 = 0.0f;
        float m12 = 0.0f;
    };

    static std::unordered_map<int, LastValue> last_values;
    LastValue current;
    current.initialized = true;
    current.opacity = opacity;
    current.m00 = transform[SkMatrix::kMScaleX];
    current.m01 = transform[SkMatrix::kMSkewX];
    current.m02 = transform[SkMatrix::kMTransX];
    current.m10 = transform[SkMatrix::kMSkewY];
    current.m11 = transform[SkMatrix::kMScaleY];
    current.m12 = transform[SkMatrix::kMTransY];

    auto it = last_values.find(layer_id);
    if (it != last_values.end()) {
        const LastValue& prev = it->second;
        const float eps = 0.0001f;
        bool same = std::fabs(prev.opacity - current.opacity) < eps &&
                    std::fabs(prev.m00 - current.m00) < eps &&
                    std::fabs(prev.m01 - current.m01) < eps &&
                    std::fabs(prev.m02 - current.m02) < eps &&
                    std::fabs(prev.m10 - current.m10) < eps &&
                    std::fabs(prev.m11 - current.m11) < eps &&
                    std::fabs(prev.m12 - current.m12) < eps;
        if (same) {
            return false;
        }
    }

    last_values[layer_id] = current;
    return true;
}

inline void LogAnimFinalConsume(const char* path, CompositorLayer* layer, const SkMatrix& transform) {
    if (!IsAnimFinalDebugEnabled() || !ShouldLogAnimFinalLayer(layer)) {
        return;
    }

    const int layer_id = layer->GetId();
    const float opacity = layer->GetOpacity();
    if (!LogTransformChanged(layer_id, opacity, transform)) {
        return;
    }

    std::cout << "[ANIM_FINAL_CONSUME] path=" << path
              << " layer_id=" << layer_id
              << " opacity=" << opacity
              << " m00=" << transform[SkMatrix::kMScaleX]
              << " m01=" << transform[SkMatrix::kMSkewX]
              << " m02=" << transform[SkMatrix::kMTransX]
              << " m10=" << transform[SkMatrix::kMSkewY]
              << " m11=" << transform[SkMatrix::kMScaleY]
              << " m12=" << transform[SkMatrix::kMTransY]
              << "\n";
}

inline bool IsAnimDrawDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_DRAW") != nullptr) || IsAnimFinalDebugEnabled();
    return enabled;
}

inline bool ShouldLogAnimDrawChanged(int layer_id,
                                     bool skip_self_draw,
                                     bool bitmap_null,
                                     bool drew_bitmap,
                                     float m02,
                                     float m12) {
    struct LastDraw {
        bool initialized = false;
        bool skip_self_draw = false;
        bool bitmap_null = true;
        bool drew_bitmap = false;
        float m02 = 0.0f;
        float m12 = 0.0f;
    };

    static std::unordered_map<int, LastDraw> last_draw;
    LastDraw current;
    current.initialized = true;
    current.skip_self_draw = skip_self_draw;
    current.bitmap_null = bitmap_null;
    current.drew_bitmap = drew_bitmap;
    current.m02 = m02;
    current.m12 = m12;

    auto it = last_draw.find(layer_id);
    if (it != last_draw.end()) {
        const LastDraw& prev = it->second;
        const float eps = 0.0001f;
        bool same = (prev.skip_self_draw == current.skip_self_draw) &&
                    (prev.bitmap_null == current.bitmap_null) &&
                    (prev.drew_bitmap == current.drew_bitmap) &&
                    (std::fabs(prev.m02 - current.m02) < eps) &&
                    (std::fabs(prev.m12 - current.m12) < eps);
        if (same) {
            return false;
        }
    }

    last_draw[layer_id] = current;
    return true;
}

inline void LogAnimDrawCPU(CompositorLayer* layer,
                           bool skip_self_draw,
                           bool bitmap_null,
                           bool drew_bitmap,
                           const SkRect& bounds,
                           const SkMatrix& transform) {
    if (!IsAnimDrawDebugEnabled() || !ShouldLogAnimFinalLayer(layer)) {
        return;
    }

    const int layer_id = layer->GetId();
    const float m02 = transform[SkMatrix::kMTransX];
    const float m12 = transform[SkMatrix::kMTransY];
    if (!ShouldLogAnimDrawChanged(layer_id, skip_self_draw, bitmap_null, drew_bitmap, m02, m12)) {
        return;
    }

    std::cout << "[ANIM_DRAW_CPU]"
              << " layer_id=" << layer_id
              << " skip_self_draw=" << (skip_self_draw ? 1 : 0)
              << " bitmap_null=" << (bitmap_null ? 1 : 0)
              << " drew_bitmap=" << (drew_bitmap ? 1 : 0)
              << " bounds=" << bounds.left() << "," << bounds.top() << ","
              << bounds.width() << "x" << bounds.height()
              << " m02=" << m02
              << " m12=" << m12
              << "\n";
}


// 静态 OpenGL 函数指针
static PFNGLATTACHSHADERPROC glAttachShader_ptr = nullptr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ptr = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShader_ptr = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgram_ptr = nullptr;
static PFNGLCREATESHADERPROC glCreateShader_ptr = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgram_ptr = nullptr;
static PFNGLDELETESHADERPROC glDeleteShader_ptr = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ptr = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ptr = nullptr;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ptr = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ptr = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgram_ptr = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSource_ptr = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgram_ptr = nullptr;
static PFNGLUNIFORM1FPROC glUniform1f_ptr = nullptr;
static PFNGLUNIFORM1IPROC glUniform1i_ptr = nullptr;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv_ptr = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_ptr = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer_ptr = nullptr;
static PFNGLBUFFERDATAPROC glBufferData_ptr = nullptr;
static PFNGLGENBUFFERSPROC glGenBuffers_ptr = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers_ptr = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderiv_ptr = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ptr = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramiv_ptr = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ptr = nullptr;
static PFNGLACTIVETEXTUREPROC glActiveTexture_ptr = nullptr;

// 加载 OpenGL 扩展函数
static bool LoadGLExtensions() {
#ifdef _WIN32
    #define LOAD_GL_FUNC(name) name##_ptr = (decltype(name##_ptr))wglGetProcAddress(#name)
#else
    #define LOAD_GL_FUNC(name) name##_ptr = (decltype(name##_ptr))glXGetProcAddress((const GLubyte*)#name)
#endif

    LOAD_GL_FUNC(glAttachShader);
    LOAD_GL_FUNC(glBindVertexArray);
    LOAD_GL_FUNC(glCompileShader);
    LOAD_GL_FUNC(glCreateProgram);
    LOAD_GL_FUNC(glCreateShader);
    LOAD_GL_FUNC(glDeleteProgram);
    LOAD_GL_FUNC(glDeleteShader);
    LOAD_GL_FUNC(glDeleteVertexArrays);
    LOAD_GL_FUNC(glEnableVertexAttribArray);
    LOAD_GL_FUNC(glGenVertexArrays);
    LOAD_GL_FUNC(glGetUniformLocation);
    LOAD_GL_FUNC(glLinkProgram);
    LOAD_GL_FUNC(glShaderSource);
    LOAD_GL_FUNC(glUseProgram);
    LOAD_GL_FUNC(glUniform1f);
    LOAD_GL_FUNC(glUniform1i);
    LOAD_GL_FUNC(glUniformMatrix3fv);
    LOAD_GL_FUNC(glVertexAttribPointer);
    LOAD_GL_FUNC(glBindBuffer);
    LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glGenBuffers);
    LOAD_GL_FUNC(glDeleteBuffers);
    LOAD_GL_FUNC(glGetShaderiv);
    LOAD_GL_FUNC(glGetShaderInfoLog);
    LOAD_GL_FUNC(glGetProgramiv);
    LOAD_GL_FUNC(glGetProgramInfoLog);
    LOAD_GL_FUNC(glActiveTexture);

    #undef LOAD_GL_FUNC

    // 检查关键函数是否加载成功
    return glCreateProgram_ptr != nullptr &&
           glCreateShader_ptr != nullptr &&
           glGenVertexArrays_ptr != nullptr;
}

// 顶点着色器源码
static const char* vertex_shader_source = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat3 uTransform;

void main() {
    vec3 pos = uTransform * vec3(aPos, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// 片段着色器源码
static const char* fragment_shader_source = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uOpacity;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    FragColor = vec4(texColor.rgb, texColor.a * uOpacity);
}
)";

Compositor::Compositor() = default;

Compositor::~Compositor() {
    Shutdown();
}

// =========================================================================
// 初始化
// =========================================================================

bool Compositor::Initialize(int width, int height) {
    if (initialized_) {
        return true;
    }

    viewport_width_ = width;
    viewport_height_ = height;

    // 尝试初始化 GPU
    if (LoadGLExtensions() && InitializeShaders() && InitializeBuffers()) {
        use_gpu_ = true;
        stats_.using_gpu = true;
    } else {
        // 回退到 CPU 模式
        use_gpu_ = false;
        stats_.using_gpu = false;

        // 创建 CPU Surface
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        cpu_surface_ = SkSurfaces::Raster(info);
    }

    initialized_ = true;
    return true;
}

void Compositor::Resize(int width, int height) {
    if (width == viewport_width_ && height == viewport_height_) {
        return;
    }

    viewport_width_ = width;
    viewport_height_ = height;

    if (!use_gpu_ && cpu_surface_) {
        // 重新创建 CPU Surface
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        cpu_surface_ = SkSurfaces::Raster(info);
    }

    // 视口变化，合成缓存失效
    cache_valid_ = false;
    needs_composite_ = true;
}

void Compositor::Shutdown() {
    if (!initialized_) {
        return;
    }

    if (use_gpu_) {
        // 释放 GPU 资源
        if (shader_program_ != 0 && glDeleteProgram_ptr) {
            glDeleteProgram_ptr(shader_program_);
            shader_program_ = 0;
        }
        if (vao_ != 0 && glDeleteVertexArrays_ptr) {
            glDeleteVertexArrays_ptr(1, &vao_);
            vao_ = 0;
        }
        if (vbo_ != 0 && glDeleteBuffers_ptr) {
            glDeleteBuffers_ptr(1, &vbo_);
            vbo_ = 0;
        }
    }

    cpu_surface_.reset();
    initialized_ = false;
}

bool Compositor::InitializeShaders() {
    if (!glCreateShader_ptr || !glCreateProgram_ptr) {
        return false;
    }

    // 创建顶点着色器
    GLuint vertex_shader = glCreateShader_ptr(GL_VERTEX_SHADER);
    glShaderSource_ptr(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader_ptr(vertex_shader);

    // 检查编译错误
    GLint success;
    glGetShaderiv_ptr(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader_ptr(vertex_shader);
        return false;
    }

    // 创建片段着色器
    GLuint fragment_shader = glCreateShader_ptr(GL_FRAGMENT_SHADER);
    glShaderSource_ptr(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader_ptr(fragment_shader);

    glGetShaderiv_ptr(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader_ptr(vertex_shader);
        glDeleteShader_ptr(fragment_shader);
        return false;
    }

    // 创建着色器程序
    shader_program_ = glCreateProgram_ptr();
    glAttachShader_ptr(shader_program_, vertex_shader);
    glAttachShader_ptr(shader_program_, fragment_shader);
    glLinkProgram_ptr(shader_program_);

    glGetProgramiv_ptr(shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteShader_ptr(vertex_shader);
        glDeleteShader_ptr(fragment_shader);
        glDeleteProgram_ptr(shader_program_);
        shader_program_ = 0;
        return false;
    }

    // 删除着色器（已链接到程序）
    glDeleteShader_ptr(vertex_shader);
    glDeleteShader_ptr(fragment_shader);

    // 获取 uniform 位置
    uniform_transform_ = glGetUniformLocation_ptr(shader_program_, "uTransform");
    uniform_opacity_ = glGetUniformLocation_ptr(shader_program_, "uOpacity");
    uniform_texture_ = glGetUniformLocation_ptr(shader_program_, "uTexture");

    return true;
}

bool Compositor::InitializeBuffers() {
    if (!glGenVertexArrays_ptr || !glGenBuffers_ptr) {
        return false;
    }

    // 四边形顶点数据（位置 + 纹理坐标）
    float vertices[] = {
        // 位置      // 纹理坐标
        0.0f, 0.0f,  0.0f, 0.0f,  // 左下
        1.0f, 0.0f,  1.0f, 0.0f,  // 右下
        1.0f, 1.0f,  1.0f, 1.0f,  // 右上
        0.0f, 0.0f,  0.0f, 0.0f,  // 左下
        1.0f, 1.0f,  1.0f, 1.0f,  // 右上
        0.0f, 1.0f,  0.0f, 1.0f   // 左上
    };

    glGenVertexArrays_ptr(1, &vao_);
    glGenBuffers_ptr(1, &vbo_);

    glBindVertexArray_ptr(vao_);

    glBindBuffer_ptr(GL_ARRAY_BUFFER, vbo_);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray_ptr(0);

    // 纹理坐标属性
    glVertexAttribPointer_ptr(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray_ptr(1);

    glBindVertexArray_ptr(0);

    return true;
}

// =========================================================================
// 合成
// =========================================================================

bool Compositor::Composite(CompositorLayer* root) {
    if (!initialized_ || !root) {
        return false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 帧跳过优化
    if (frame_skip_enabled_ && !needs_composite_ && !HasChanges(root)) {
        stats_.frames_skipped++;
        current_frame_info_.frame_skipped = true;
        return true;
    }

    // 上传脏纹理
    int textures_uploaded = UploadDirtyTextures(root);
    stats_.textures_uploaded += textures_uploaded;

    if (use_gpu_) {
        // GPU 合成
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram_ptr(shader_program_);
        glBindVertexArray_ptr(vao_);

        CompositeLayerGPU(root, SkMatrix::I());

        glBindVertexArray_ptr(0);
        glUseProgram_ptr(0);
    } else if (cpu_surface_) {
        // CPU 合成
        SkCanvas* canvas = cpu_surface_->getCanvas();
        canvas->clear(SK_ColorTRANSPARENT);
        CompositeLayerCPU(root, canvas, SkMatrix::I());
    }

    needs_composite_ = false;

    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    stats_.frames_composited++;
    stats_.layers_composited += CountLayers(root);
    stats_.composite_time_ms += duration.count() / 1000.0;

    current_frame_info_.composite_time_ms = duration.count() / 1000.0;
    current_frame_info_.layers_composited = CountLayers(root);

    return true;
}

bool Compositor::CompositeToCanvas(CompositorLayer* root, SkCanvas* canvas) {
    if (!root || !canvas) {
        return false;
    }

    // 注意：CompositeToCanvas 不使用帧跳过优化！
    // 原因：调用方（window.cpp）每帧都会 canvas->clear(clear_color) 清除画布，
    // 如果跳过合成，canvas 上就只有背景色，导致画面闪烁空白帧。
    // 帧跳过优化仅适用于 GPU 路径的 Composite() 函数（有独立 framebuffer）。
    //
    // 同时不使用中间 SkBitmap 缓存，因为目标 canvas 可能带有 DPI 缩放矩阵，
    // 中间 bitmap 会丢失缩放信息导致文字模糊和子像素渲染失效。

    // 直接在目标 canvas 上合成（保留 canvas 的 DPI 缩放和子像素渲染能力）
    CompositeLayerCPU(root, canvas, SkMatrix::I());

    needs_composite_ = false;

    return true;
}

bool Compositor::NeedsComposite(CompositorLayer* root) const {
    return needs_composite_ || HasChanges(root);
}

// =========================================================================
// 帧管理
// =========================================================================

void Compositor::BeginFrame() {
    current_frame_info_ = CompositorFrameInfo();
    frame_start_time_ = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

CompositorFrameInfo Compositor::EndFrame() {
    double now = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    current_frame_info_.total_time_ms = now - frame_start_time_;
    last_frame_info_ = current_frame_info_;

    return last_frame_info_;
}

// =========================================================================
// 配置
// =========================================================================

void Compositor::SetGPUEnabled(bool enabled) {
    if (enabled && !use_gpu_) {
        // 尝试启用 GPU
        if (LoadGLExtensions() && InitializeShaders() && InitializeBuffers()) {
            use_gpu_ = true;
            stats_.using_gpu = true;
            cpu_surface_.reset();
        }
    } else if (!enabled && use_gpu_) {
        // 禁用 GPU，切换到 CPU
        use_gpu_ = false;
        stats_.using_gpu = false;

        // 创建 CPU Surface
        if (viewport_width_ > 0 && viewport_height_ > 0) {
            SkImageInfo info = SkImageInfo::MakeN32Premul(viewport_width_, viewport_height_);
            cpu_surface_ = SkSurfaces::Raster(info);
        }
    }

    needs_composite_ = true;
}

// =========================================================================
// GPU 合成
// =========================================================================

void Compositor::CompositeLayerGPU(CompositorLayer* layer, const SkMatrix& parent_transform) {
    if (!layer) {
        return;
    }

    // 计算层变换
    SkMatrix layer_transform = parent_transform;

    // 应用层位置
    const SkRect& bounds = layer->GetBounds();
    layer_transform.preTranslate(bounds.left(), bounds.top());

    // 应用层自身变换
    layer_transform.preConcat(layer->GetTransform());

    // 渲染当前层（不应用滚动偏移，滚动偏移只影响子层）
    if (layer->HasTexture()) {
        RenderTexturedQuad(layer, layer_transform);
    }

    // 关键修复：滚动偏移应该只应用到子层的绘制上
    // 当滚动容器滚动时，其子层（内容）应该相应移动
    const SkPoint& scroll = layer->GetScrollOffset();
    SkMatrix child_transform = layer_transform;
    if (scroll.fX != 0 || scroll.fY != 0) {
        child_transform.preTranslate(-scroll.fX, -scroll.fY);
    }

    // 递归渲染子层（使用包含滚动偏移的变换）
    for (const auto& child : layer->GetChildren()) {
        CompositeLayerGPU(child.get(), child_transform);
    }
}

void Compositor::RenderTexturedQuad(CompositorLayer* layer, const SkMatrix& transform) {
    if (!layer || !layer->HasTexture()) {
        return;
    }

    // 构建变换矩阵（转换到 NDC 坐标）
    SkMatrix ndc_transform;

    // 缩放到层大小
    const SkRect& bounds = layer->GetBounds();
    ndc_transform.setScale(bounds.width(), bounds.height());

    // 应用层变换
    ndc_transform.postConcat(transform);

    // 转换到 NDC（-1 到 1）
    ndc_transform.postScale(2.0f / viewport_width_, -2.0f / viewport_height_);
    ndc_transform.postTranslate(-1.0f, 1.0f);

    // 转换为 3x3 矩阵数组
    float matrix[9];
    for (int i = 0; i < 9; i++) {
        matrix[i] = ndc_transform[i];
    }

    // 设置 uniform
    glUniformMatrix3fv_ptr(uniform_transform_, 1, GL_FALSE, matrix);
    glUniform1f_ptr(uniform_opacity_, layer->GetOpacity());

    LogAnimFinalConsume("gpu", layer, transform);

    // 绑定纹理
    glActiveTexture_ptr(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, layer->GetTextureId());
    glUniform1i_ptr(uniform_texture_, 0);

    // 绘制四边形
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// =========================================================================
// CPU 合成
// =========================================================================

// 辅助函数：获取层的 z-index
static int GetLayerZIndex(CompositorLayer* layer) {
    if (!layer) return 0;
    RenderObject* obj = layer->GetRenderObject();
    if (!obj) return 0;
    return obj->GetComputedStyle().z_index;
}

void Compositor::CompositeLayerCPU(CompositorLayer* layer, SkCanvas* canvas, const SkMatrix& parent_transform) {
    if (!layer || !canvas) {
        return;
    }

    const SkRect& bounds = layer->GetBounds();

    // 关键修复：对于空边界的层（如空的 Toast 容器），跳过自身绘制但仍需绘制子层
    // 因为子层可能有有效内容（如动画层）
    bool skip_self_draw = bounds.isEmpty();

    canvas->save();

    // 构建当前层的完整变换（与 GPU 路径保持一致）
    // 1) 继承父层变换
    // 2) 应用当前层位置
    // 3) 应用当前层自身变换（transform 动画）
    SkMatrix layer_transform = parent_transform;
    layer_transform.preTranslate(bounds.left(), bounds.top());
    layer_transform.preConcat(layer->GetTransform());

    LogAnimFinalConsume("cpu", layer, layer_transform);

    // 获取当前层的滚动偏移
    const SkPoint& scroll = layer->GetScrollOffset();
    bool has_scroll = (scroll.fX != 0 || scroll.fY != 0);

    // 计算传递给子层的累积变换
    // 滚动偏移只影响子层（内容），不影响当前层位图
    SkMatrix child_transform = layer_transform;
    if (has_scroll) {
        child_transform.preTranslate(-scroll.fX, -scroll.fY);
    }

    // 应用当前层变换到画布
    if (!skip_self_draw) {
        canvas->concat(layer_transform);
    }

    bool bitmap_null = true;
    bool drew_bitmap = false;

    // 绘制层位图（不应用滚动偏移，因为位图内容是静态的）
    // 滚动偏移只影响子层的位置
    if (!skip_self_draw) {
        const SkBitmap& bitmap = layer->GetBitmap();
        bitmap_null = bitmap.isNull();
        if (!bitmap_null) {
            SkPaint paint;
            paint.setAlpha(static_cast<int>(layer->GetOpacity() * 255));

            // 🐛 修复：如果有 shadow_extent，需要调整绘制位置
            // bitmap 比元素大，需要向左上偏移 shadow_extent
            // 优化：使用四个方向独立的扩展值
            const auto& shadow_extent = layer->GetShadowExtent();
            float draw_offset_x = -shadow_extent.left;
            float draw_offset_y = -shadow_extent.top;

            // 位图是物理像素大小，需要缩放回逻辑像素大小绘制
            float dpi_scale = layer->GetDpiScale();
            if (dpi_scale != 1.0f) {
                canvas->save();
                canvas->scale(1.0f / dpi_scale, 1.0f / dpi_scale);
                // 应用 DPI 缩放到偏移
                canvas->drawImage(bitmap.asImage(),
                                  draw_offset_x * dpi_scale,
                                  draw_offset_y * dpi_scale,
                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                canvas->restore();
            } else {
                canvas->drawImage(bitmap.asImage(), draw_offset_x, draw_offset_y, SkSamplingOptions(), &paint);
            }
            drew_bitmap = true;
        }

        // 绘制层边界（调试）
        if (show_layer_borders_) {
            DrawLayerBorder(layer, canvas);
        }
    }

    LogAnimDrawCPU(layer, skip_self_draw, bitmap_null, drew_bitmap, bounds, layer_transform);

    // 关键修复：按 z-index 排序子层后再绘制
    // 这确保高 z-index 的元素（如 Modal）绘制在低 z-index 元素之上
    const auto& children = layer->GetChildren();
    std::vector<CompositorLayer*> sorted_children;
    sorted_children.reserve(children.size());
    for (const auto& child : children) {
        sorted_children.push_back(child.get());
    }

    // 按 z-index 升序稳定排序（低 z-index 先绘制；同 z-index 保持原文档顺序）
    std::stable_sort(sorted_children.begin(), sorted_children.end(),
        [](CompositorLayer* a, CompositorLayer* b) {
            return GetLayerZIndex(a) < GetLayerZIndex(b);
        });

    // 关键修复：在递归调用子层之前，先恢复 canvas 到干净状态
    // 这样子层不会继承当前层的 canvas 变换
    canvas->restore();

    // 递归绘制排序后的子层
    for (CompositorLayer* child : sorted_children) {
        // 关键修复：position: fixed 元素不应该受到父层滚动偏移的影响
        // fixed 元素的位置是相对于视口的，不随滚动变化
        bool child_is_fixed = (child->GetPromotionReason() == LayerPromotionReason::PositionFixed);

        if (child_is_fixed) {
            // fixed 元素：不应用任何滚动偏移，重置变换
            CompositeLayerCPU(child, canvas, SkMatrix::I());
        } else {
            // 非 fixed 元素：传递累积的平移偏移
            CompositeLayerCPU(child, canvas, child_transform);
        }
    }
}

// =========================================================================
// 辅助方法
// =========================================================================

int Compositor::UploadDirtyTextures(CompositorLayer* root) {
    if (!root) {
        return 0;
    }

    int count = 0;

    // 上传当前层
    if (root->IsTextureDirty()) {
        if (root->UploadDirtyRegions()) {
            count++;
        }
    }

    // 递归处理子层
    for (const auto& child : root->GetChildren()) {
        count += UploadDirtyTextures(child.get());
    }

    return count;
}

bool Compositor::HasChanges(CompositorLayer* root) const {
    if (!root) {
        return false;
    }

    // 检查当前层
    if (root->HasDirtyRegions() || root->IsTextureDirty()) {
        return true;
    }

    // 递归检查子层
    for (const auto& child : root->GetChildren()) {
        if (HasChanges(child.get())) {
            return true;
        }
    }

    return false;
}

void Compositor::DrawLayerBorder(CompositorLayer* layer, SkCanvas* canvas) {
    if (!layer || !canvas) {
        return;
    }

    const SkRect& bounds = layer->GetBounds();
    SkRect border_rect = SkRect::MakeWH(bounds.width(), bounds.height());

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);

    // 根据层提升原因选择颜色
    switch (layer->GetPromotionReason()) {
        case LayerPromotionReason::RootLayer:
            paint.setColor(SK_ColorGREEN);
            break;
        case LayerPromotionReason::PositionFixed:
            paint.setColor(SK_ColorBLUE);
            break;
        case LayerPromotionReason::TransformAnimation:
        case LayerPromotionReason::OpacityAnimation:
            paint.setColor(SK_ColorYELLOW);
            break;
        case LayerPromotionReason::ScrollableContent:
            paint.setColor(SK_ColorCYAN);
            break;
        default:
            paint.setColor(SK_ColorRED);
            break;
    }

    canvas->drawRect(border_rect, paint);
}

int Compositor::CountLayers(CompositorLayer* root) const {
    if (!root) {
        return 0;
    }

    int count = 1;
    for (const auto& child : root->GetChildren()) {
        count += CountLayers(child.get());
    }

    return count;
}

} // namespace mbink
