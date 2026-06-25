/**
 * @file display_backend.h
 * @brief 显示后端抽象接口
 * 
 * 将渲染（Skia）和显示分离，支持多种显示方式：
 * - OpenGL 纹理显示（利用 GPU VSync，无闪烁）
 * - Windows Layered Window（Win32 原生无闪烁 API）
 * - GDI 双缓冲（回退方案）
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

// 前向声明
struct SDL_Window;

namespace mblink {

/**
 * @brief 显示后端类型
 */
enum class DisplayBackendType {
    AUTO,           ///< 自动选择最佳后端
    D3D11,          ///< Direct3D 11 (仅 Windows，推荐)
    PAINT_MODE,     ///< WM_PAINT 模式 (仅 Windows，虚拟机友好)
    OPENGL,         ///< OpenGL 纹理显示
    LAYERED_WINDOW, ///< Windows Layered Window (仅 Windows)
    GDI,            ///< GDI 双缓冲 (仅 Windows)
    SDL_SURFACE     ///< SDL Surface (跨平台回退)
};

/**
 * @brief 显示后端抽象基类
 * 
 * 负责将像素数据显示到窗口，与渲染引擎（Skia）解耦
 */
class DisplayBackend {
public:
    virtual ~DisplayBackend() = default;

    /**
     * @brief 初始化显示后端
     * @param window SDL 窗口句柄
     * @param width 窗口宽度
     * @param height 窗口高度
     * @return 是否初始化成功
     */
    virtual bool Initialize(SDL_Window* window, int width, int height) = 0;

    /**
     * @brief 将像素数据显示到窗口
     * @param pixels 像素数据指针 (BGRA 格式, 预乘 alpha)
     * @param width 图像宽度
     * @param height 图像高度
     * @param stride 每行字节数（通常为 width * 4）
     */
    virtual void Present(const void* pixels, int width, int height, int stride) = 0;

    /**
     * @brief 将像素数据的脏区域显示到窗口（优化版本）
     * @param pixels 像素数据指针 (BGRA 格式, 预乘 alpha)
     * @param width 图像宽度
     * @param height 图像高度
     * @param stride 每行字节数（通常为 width * 4）
     * @param dirty_x 脏区域左上角 X
     * @param dirty_y 脏区域左上角 Y
     * @param dirty_width 脏区域宽度
     * @param dirty_height 脏区域高度
     * 
     * 默认实现调用 Present() 复制整个 surface。
     * 子类可以重写此方法以实现局部更新优化。
     */
    virtual void PresentPartial(const void* pixels, int width, int height, int stride,
                                int /*dirty_x*/, int /*dirty_y*/, int /*dirty_width*/, int /*dirty_height*/) {
        // 默认实现：复制整个 surface
        Present(pixels, width, height, stride);
    }

    /**
     * @brief 处理窗口大小变化
     * @param width 新宽度
     * @param height 新高度
     */
    virtual void OnResize(int width, int height) = 0;

    /**
     * @brief 释放资源
     */
    virtual void Shutdown() = 0;

    /**
     * @brief 获取后端类型
     */
    virtual DisplayBackendType GetType() const = 0;

    /**
     * @brief 获取后端名称（用于调试）
     */
    virtual const char* GetName() const = 0;

    /**
     * @brief 检查后端是否支持 VSync
     */
    virtual bool SupportsVSync() const { return false; }

    /**
     * @brief 设置 VSync
     * @param enabled 是否启用
     * @return 是否设置成功
     */
    virtual bool SetVSync(bool /*enabled*/) { return false; }

    /**
     * @brief 创建指定类型的显示后端
     * @param type 后端类型
     * @return 显示后端实例，失败返回 nullptr
     */
    static std::unique_ptr<DisplayBackend> Create(DisplayBackendType type);

    /**
     * @brief 按优先级自动选择最佳显示后端
     * @param window SDL 窗口句柄
     * @param width 窗口宽度
     * @param height 窗口高度
     * @return 显示后端实例，失败返回 nullptr
     */
    static std::unique_ptr<DisplayBackend> CreateBest(SDL_Window* window, int width, int height);

    /**
     * @brief 检测 OpenGL 是否可用
     * @return 是否可用
     */
    static bool IsOpenGLAvailable();

protected:
    SDL_Window* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

/**
 * @brief OpenGL 显示后端
 * 
 * 使用 OpenGL 纹理显示像素数据，利用 GPU VSync 实现无闪烁
 */
class OpenGLDisplayBackend : public DisplayBackend {
public:
    OpenGLDisplayBackend();
    ~OpenGLDisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::OPENGL; }
    const char* GetName() const override { return "OpenGL"; }
    bool SupportsVSync() const override { return true; }
    bool SetVSync(bool enabled) override;

private:
    void* gl_context_ = nullptr;  // SDL_GLContext
    uint32_t texture_id_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t shader_program_ = 0;
    bool vsync_enabled_ = true;
};

#ifdef _WIN32
/**
 * @brief Direct3D 11 显示后端
 *
 * 使用 D3D11 纹理显示像素数据，利用 DXGI VSync 实现无闪烁
 * 这是 Windows 上推荐的 CPU 渲染显示方案
 */
class D3D11DisplayBackend : public DisplayBackend {
public:
    D3D11DisplayBackend();
    ~D3D11DisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::D3D11; }
    const char* GetName() const override { return "Direct3D 11"; }
    bool SupportsVSync() const override { return true; }
    bool SetVSync(bool enabled) override;

private:
    bool CreateDeviceAndSwapChain();
    bool CreateRenderTarget();
    bool CreateTexture(int width, int height);
    bool CreateShaders();
    void ReleaseRenderTarget();

    void* hwnd_ = nullptr;              // HWND
    void* device_ = nullptr;            // ID3D11Device*
    void* device_context_ = nullptr;    // ID3D11DeviceContext*
    void* swap_chain_ = nullptr;        // IDXGISwapChain*
    void* render_target_view_ = nullptr;// ID3D11RenderTargetView*
    void* texture_ = nullptr;           // ID3D11Texture2D* (动态纹理)
    void* texture_srv_ = nullptr;       // ID3D11ShaderResourceView*
    void* sampler_state_ = nullptr;     // ID3D11SamplerState*
    void* vertex_shader_ = nullptr;     // ID3D11VertexShader*
    void* pixel_shader_ = nullptr;      // ID3D11PixelShader*
    void* vertex_buffer_ = nullptr;     // ID3D11Buffer*
    void* input_layout_ = nullptr;      // ID3D11InputLayout*
    int texture_width_ = 0;
    int texture_height_ = 0;
    bool vsync_enabled_ = true;
};

/**
 * @brief Windows Layered Window 显示后端
 *
 * 使用 Win32 UpdateLayeredWindow API 实现无闪烁显示
 */
class LayeredWindowDisplayBackend : public DisplayBackend {
public:
    LayeredWindowDisplayBackend();
    ~LayeredWindowDisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::LAYERED_WINDOW; }
    const char* GetName() const override { return "LayeredWindow"; }

private:
    void* hwnd_ = nullptr;       // HWND
    void* hdc_mem_ = nullptr;    // HDC
    void* hbitmap_ = nullptr;    // HBITMAP
    void* hbitmap_old_ = nullptr;
    void* bitmap_bits_ = nullptr; // 直接访问位图像素
    int bitmap_width_ = 0;
    int bitmap_height_ = 0;
};

/**
 * @brief GDI 显示后端
 *
 * 使用 GDI 双缓冲显示，作为最终回退方案
 */
class GDIDisplayBackend : public DisplayBackend {
public:
    GDIDisplayBackend();
    ~GDIDisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::GDI; }
    const char* GetName() const override { return "GDI"; }

private:
    void* hwnd_ = nullptr;       // HWND
    void* hdc_mem_ = nullptr;    // HDC
    void* hbitmap_ = nullptr;    // HBITMAP
    void* hbitmap_old_ = nullptr;
    void* bitmap_bits_ = nullptr;
    int bitmap_width_ = 0;
    int bitmap_height_ = 0;
};

/**
 * @brief WM_PAINT 模式显示后端
 *
 * 基于 Windows 标准绘制模型，适合虚拟机环境：
 * 1. 渲染时只更新离屏缓冲区并调用 InvalidateRect()
 * 2. 在 WM_PAINT 消息中使用 BeginPaint/EndPaint 绘制
 * 3. 让系统 DWM 控制显示时机，避免与窗口管理器冲突
 */
class PaintModeDisplayBackend : public DisplayBackend {
public:
    PaintModeDisplayBackend();
    ~PaintModeDisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void PresentPartial(const void* pixels, int width, int height, int stride,
                        int dirty_x, int dirty_y, int dirty_width, int dirty_height) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::PAINT_MODE; }
    const char* GetName() const override { return "PaintMode"; }

    /**
     * @brief 在 WM_PAINT 中调用此方法进行绘制
     * @param hdc 绘制设备上下文 (从 BeginPaint 获取)
     * @param paint_rect 需要绘制的区域 (从 PAINTSTRUCT 获取)
     */
    void OnPaint(void* hdc, const void* paint_rect);

    /**
     * @brief 获取当前缓冲区的内存 DC，用于 WM_PAINT 中的 BitBlt
     * @return 内存 DC 句柄
     */
    void* GetMemoryDC() const { return hdc_mem_; }

    /**
     * @brief 检查是否有待绘制的内容
     */
    bool HasPendingPaint() const { return has_pending_paint_; }

private:
    bool CreateBuffer(int width, int height);
    void DestroyBuffer();

    bool buffer_has_full_content_ = false;
    void* hwnd_ = nullptr;           // HWND
    void* hdc_mem_ = nullptr;        // HDC (内存设备上下文)
    void* hbitmap_ = nullptr;        // HBITMAP (DIB 位图)
    void* hbitmap_old_ = nullptr;    // 旧位图句柄
    void* bitmap_bits_ = nullptr;    // 直接访问位图像素的指针
    int bitmap_width_ = 0;
    int bitmap_height_ = 0;
    bool has_pending_paint_ = false; // 是否有待绘制的内容
};

#endif // _WIN32

/**
 * @brief SDL Surface 显示后端
 * 
 * 使用 SDL_UpdateWindowSurface，跨平台但可能闪烁
 */
class SDLSurfaceDisplayBackend : public DisplayBackend {
public:
    SDLSurfaceDisplayBackend();
    ~SDLSurfaceDisplayBackend() override;

    bool Initialize(SDL_Window* window, int width, int height) override;
    void Present(const void* pixels, int width, int height, int stride) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;

    DisplayBackendType GetType() const override { return DisplayBackendType::SDL_SURFACE; }
    const char* GetName() const override { return "SDL_Surface"; }

private:
    void* sdl_surface_ = nullptr;  // SDL_Surface*
};

} // namespace mblink

