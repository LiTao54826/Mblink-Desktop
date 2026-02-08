/**
 * @file display_backend.cpp
 * @brief 显示后端实现
 */

#include "display_backend.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <cstring>
#include <algorithm>  // for std::min

#ifdef _WIN32
// 避免 Windows.h 中的 min/max 宏与 std::min/std::max 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Direct3D 11 头文件
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#endif

// OpenGL 头文件
#ifdef _WIN32
#include <GL/gl.h>
// Windows GL.h 不包含 GL_CLAMP_TO_EDGE，需要手动定义
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace lightui {

// ============================================================================
// DisplayBackend 静态方法
// ============================================================================

std::unique_ptr<DisplayBackend> DisplayBackend::Create(DisplayBackendType type) {
    switch (type) {
#ifdef _WIN32
        case DisplayBackendType::D3D11:
            return std::make_unique<D3D11DisplayBackend>();
        case DisplayBackendType::PAINT_MODE:
            return std::make_unique<PaintModeDisplayBackend>();
        case DisplayBackendType::LAYERED_WINDOW:
            return std::make_unique<LayeredWindowDisplayBackend>();
        case DisplayBackendType::GDI:
            return std::make_unique<GDIDisplayBackend>();
#endif
        case DisplayBackendType::OPENGL:
            return std::make_unique<OpenGLDisplayBackend>();
        case DisplayBackendType::SDL_SURFACE:
            return std::make_unique<SDLSurfaceDisplayBackend>();
        case DisplayBackendType::AUTO:
        default:
            return nullptr;  // 使用 CreateBest
    }
}

// 检测是否在虚拟机中运行
static bool IsRunningInVirtualMachine() {
#ifdef _WIN32
    // 方法1: 检查系统信息中的虚拟机标识
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char value[256] = {0};
        DWORD size = sizeof(value);
        if (RegQueryValueExA(hKey, "0", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            // 检查常见虚拟机标识
            std::string disk_info(value);
            if (disk_info.find("VBOX") != std::string::npos ||
                disk_info.find("VMWARE") != std::string::npos ||
                disk_info.find("QEMU") != std::string::npos ||
                disk_info.find("Virtual") != std::string::npos) {
                RegCloseKey(hKey);
                return true;
            }
        }
        RegCloseKey(hKey);
    }

    // 方法2: 检查 BIOS 信息
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\BIOS",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char value[256] = {0};
        DWORD size = sizeof(value);
        if (RegQueryValueExA(hKey, "SystemProductName", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            std::string product(value);
            if (product.find("VirtualBox") != std::string::npos ||
                product.find("VMware") != std::string::npos ||
                product.find("Virtual Machine") != std::string::npos ||
                product.find("Hyper-V") != std::string::npos) {
                RegCloseKey(hKey);
                return true;
            }
        }
        RegCloseKey(hKey);
    }
#endif
    return false;
}

std::unique_ptr<DisplayBackend> DisplayBackend::CreateBest(SDL_Window* window, int width, int height) {
    // 按优先级尝试各后端

#ifdef _WIN32
    // 检测虚拟机环境
    bool is_vm = IsRunningInVirtualMachine();
    if (is_vm) {
    }

    // 虚拟机环境：优先使用 PaintMode 后端（基于 WM_PAINT，与系统窗口管理器协作更好）
    if (is_vm) {
        auto backend = std::make_unique<PaintModeDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }

    // 1. 物理机上优先使用 Direct3D 11 - 最稳定的无闪烁方案
    {
        auto backend = std::make_unique<D3D11DisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }

    // 2. PaintMode 作为第二选择（如果 D3D11 失败）
    if (!is_vm) {
        auto backend = std::make_unique<PaintModeDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }

    // 3. GDI 作为回退
    {
        auto backend = std::make_unique<GDIDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }
#endif

    // 4. OpenGL 作为跨平台方案
    if (IsOpenGLAvailable()) {
        auto backend = std::make_unique<OpenGLDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }

    // 5. SDL Surface 最终回退
    {
        auto backend = std::make_unique<SDLSurfaceDisplayBackend>();
        if (backend->Initialize(window, width, height)) {
            return backend;
        }
    }

    return nullptr;
}

bool DisplayBackend::IsOpenGLAvailable() {
    // 简单检测：尝试创建临时 OpenGL 上下文
    // 这里先返回 true，实际检测在 Initialize 中进行
    return true;
}

// ============================================================================
// D3D11DisplayBackend 实现 (Windows only)
// ============================================================================

#ifdef _WIN32

// 简单的全屏四边形顶点着色器
static const char* g_d3d11_vertex_shader = R"(
struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}
)";

// 简单的纹理采样像素着色器
static const char* g_d3d11_pixel_shader = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    return tex.Sample(samp, input.uv);
}
)";

// 顶点结构
struct D3D11Vertex {
    float x, y;   // 位置
    float u, v;   // 纹理坐标
};

D3D11DisplayBackend::D3D11DisplayBackend() = default;

D3D11DisplayBackend::~D3D11DisplayBackend() {
    Shutdown();
}

bool D3D11DisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Windows 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (!hwnd_) {
        return false;
    }

    // 创建设备和交换链
    if (!CreateDeviceAndSwapChain()) {
        return false;
    }

    // 创建渲染目标
    if (!CreateRenderTarget()) {
        Shutdown();
        return false;
    }

    // 创建纹理
    if (!CreateTexture(width, height)) {
        Shutdown();
        return false;
    }

    // 创建着色器
    if (!CreateShaders()) {
        Shutdown();
        return false;
    }

    return true;
}

bool D3D11DisplayBackend::CreateDeviceAndSwapChain() {
    HWND hwnd = (HWND)hwnd_;

    // 描述交换链
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = width_;
    scd.BufferDesc.Height = height_;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT create_flags = 0;
#ifdef _DEBUG
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    D3D_FEATURE_LEVEL feature_level;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // 适配器（默认）
        D3D_DRIVER_TYPE_HARDWARE,   // 驱动类型
        nullptr,                    // 软件模块
        create_flags,               // 创建标志
        feature_levels,             // 功能级别
        _countof(feature_levels),
        D3D11_SDK_VERSION,
        &scd,
        &swap_chain,
        &device,
        &feature_level,
        &context
    );

    if (FAILED(hr)) {
        // 尝试不带调试层
        create_flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            create_flags, feature_levels, _countof(feature_levels),
            D3D11_SDK_VERSION, &scd, &swap_chain, &device,
            &feature_level, &context
        );
    }

    if (FAILED(hr)) {
        return false;
    }

    device_ = device;
    device_context_ = context;
    swap_chain_ = swap_chain;

    return true;
}

bool D3D11DisplayBackend::CreateRenderTarget() {
    IDXGISwapChain* swap_chain = (IDXGISwapChain*)swap_chain_;
    ID3D11Device* device = (ID3D11Device*)device_;

    ID3D11Texture2D* back_buffer = nullptr;
    HRESULT hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
    if (FAILED(hr)) {
        return false;
    }

    ID3D11RenderTargetView* rtv = nullptr;
    hr = device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
    back_buffer->Release();

    if (FAILED(hr)) {
        return false;
    }

    render_target_view_ = rtv;
    return true;
}

bool D3D11DisplayBackend::CreateTexture(int width, int height) {
    ID3D11Device* device = (ID3D11Device*)device_;

    // 创建动态纹理用于上传 CPU 渲染的像素
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // BGRA 格式匹配 Skia
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DYNAMIC;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&tex_desc, nullptr, &texture);
    if (FAILED(hr)) {
        return false;
    }

    // 创建着色器资源视图
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, &srv_desc, &srv);
    if (FAILED(hr)) {
        texture->Release();
        return false;
    }

    // 创建采样器
    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    ID3D11SamplerState* sampler = nullptr;
    hr = device->CreateSamplerState(&sampler_desc, &sampler);
    if (FAILED(hr)) {
        texture->Release();
        srv->Release();
        return false;
    }

    texture_ = texture;
    texture_srv_ = srv;
    sampler_state_ = sampler;
    texture_width_ = width;
    texture_height_ = height;

    return true;
}

bool D3D11DisplayBackend::CreateShaders() {
    ID3D11Device* device = (ID3D11Device*)device_;

    // 编译顶点着色器
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    HRESULT hr = D3DCompile(
        g_d3d11_vertex_shader, strlen(g_d3d11_vertex_shader),
        "VS", nullptr, nullptr, "main", "vs_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &vs_blob, &error_blob
    );

    if (FAILED(hr)) {
        if (error_blob) {
            error_blob->Release();
        }
        return false;
    }

    ID3D11VertexShader* vs = nullptr;
    hr = device->CreateVertexShader(
        vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
        nullptr, &vs
    );
    if (FAILED(hr)) {
        vs_blob->Release();
        return false;
    }

    // 创建输入布局
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* input_layout = nullptr;
    hr = device->CreateInputLayout(
        layout, _countof(layout),
        vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
        &input_layout
    );
    vs_blob->Release();

    if (FAILED(hr)) {
        vs->Release();
        return false;
    }

    // 编译像素着色器
    ID3DBlob* ps_blob = nullptr;
    hr = D3DCompile(
        g_d3d11_pixel_shader, strlen(g_d3d11_pixel_shader),
        "PS", nullptr, nullptr, "main", "ps_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &ps_blob, &error_blob
    );

    if (FAILED(hr)) {
        if (error_blob) {
            error_blob->Release();
        }
        vs->Release();
        input_layout->Release();
        return false;
    }

    ID3D11PixelShader* ps = nullptr;
    hr = device->CreatePixelShader(
        ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
        nullptr, &ps
    );
    ps_blob->Release();

    if (FAILED(hr)) {
        vs->Release();
        input_layout->Release();
        return false;
    }

    // 创建顶点缓冲区（全屏四边形）
    D3D11Vertex vertices[] = {
        { -1.0f,  1.0f, 0.0f, 0.0f }, // 左上
        {  1.0f,  1.0f, 1.0f, 0.0f }, // 右上
        { -1.0f, -1.0f, 0.0f, 1.0f }, // 左下
        {  1.0f, -1.0f, 1.0f, 1.0f }, // 右下
    };

    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vb_data = {};
    vb_data.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    hr = device->CreateBuffer(&vb_desc, &vb_data, &vb);
    if (FAILED(hr)) {
        vs->Release();
        ps->Release();
        input_layout->Release();
        return false;
    }

    vertex_shader_ = vs;
    pixel_shader_ = ps;
    input_layout_ = input_layout;
    vertex_buffer_ = vb;

    return true;
}

void D3D11DisplayBackend::ReleaseRenderTarget() {
    if (render_target_view_) {
        ((ID3D11RenderTargetView*)render_target_view_)->Release();
        render_target_view_ = nullptr;
    }
}

void D3D11DisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!device_context_ || !swap_chain_ || !texture_) return;

    ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)device_context_;
    IDXGISwapChain* swap_chain = (IDXGISwapChain*)swap_chain_;

    // 如果尺寸变化，重建纹理
    if (width != texture_width_ || height != texture_height_) {
        if (texture_srv_) ((ID3D11ShaderResourceView*)texture_srv_)->Release();
        if (texture_) ((ID3D11Texture2D*)texture_)->Release();
        texture_ = nullptr;
        texture_srv_ = nullptr;
        CreateTexture(width, height);
    }

    // 上传像素到纹理
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = ctx->Map((ID3D11Texture2D*)texture_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        const uint8_t* src = (const uint8_t*)pixels;
        uint8_t* dst = (uint8_t*)mapped.pData;
        int copy_width = width * 4;
        for (int y = 0; y < height; y++) {
            memcpy(dst, src, copy_width);
            src += stride;
            dst += mapped.RowPitch;
        }
        ctx->Unmap((ID3D11Texture2D*)texture_, 0);
    }

    // 设置渲染状态
    ID3D11RenderTargetView* rtv = (ID3D11RenderTargetView*)render_target_view_;
    ctx->OMSetRenderTargets(1, &rtv, nullptr);

    // 设置视口
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width_;
    viewport.Height = (float)height_;
    viewport.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &viewport);

    // 清屏
    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clear_color);

    // 设置着色器和资源
    ctx->IASetInputLayout((ID3D11InputLayout*)input_layout_);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    UINT stride_vb = sizeof(D3D11Vertex);
    UINT offset = 0;
    ID3D11Buffer* vb = (ID3D11Buffer*)vertex_buffer_;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride_vb, &offset);

    ctx->VSSetShader((ID3D11VertexShader*)vertex_shader_, nullptr, 0);
    ctx->PSSetShader((ID3D11PixelShader*)pixel_shader_, nullptr, 0);

    ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)texture_srv_;
    ctx->PSSetShaderResources(0, 1, &srv);

    ID3D11SamplerState* sampler = (ID3D11SamplerState*)sampler_state_;
    ctx->PSSetSamplers(0, 1, &sampler);

    // 绘制全屏四边形
    ctx->Draw(4, 0);

    // 显示（带 VSync）
    swap_chain->Present(vsync_enabled_ ? 1 : 0, 0);
}

void D3D11DisplayBackend::OnResize(int width, int height) {
    if (width == width_ && height == height_) return;
    if (!swap_chain_ || !device_) return;

    width_ = width;
    height_ = height;

    ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)device_context_;
    IDXGISwapChain* swap_chain = (IDXGISwapChain*)swap_chain_;

    // 释放渲染目标
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ReleaseRenderTarget();

    // 调整交换链大小
    HRESULT hr = swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        return;
    }

    // 重建渲染目标
    CreateRenderTarget();
}

void D3D11DisplayBackend::Shutdown() {
    if (vertex_buffer_) { ((ID3D11Buffer*)vertex_buffer_)->Release(); vertex_buffer_ = nullptr; }
    if (input_layout_) { ((ID3D11InputLayout*)input_layout_)->Release(); input_layout_ = nullptr; }
    if (pixel_shader_) { ((ID3D11PixelShader*)pixel_shader_)->Release(); pixel_shader_ = nullptr; }
    if (vertex_shader_) { ((ID3D11VertexShader*)vertex_shader_)->Release(); vertex_shader_ = nullptr; }
    if (sampler_state_) { ((ID3D11SamplerState*)sampler_state_)->Release(); sampler_state_ = nullptr; }
    if (texture_srv_) { ((ID3D11ShaderResourceView*)texture_srv_)->Release(); texture_srv_ = nullptr; }
    if (texture_) { ((ID3D11Texture2D*)texture_)->Release(); texture_ = nullptr; }
    ReleaseRenderTarget();
    if (swap_chain_) { ((IDXGISwapChain*)swap_chain_)->Release(); swap_chain_ = nullptr; }
    if (device_context_) { ((ID3D11DeviceContext*)device_context_)->Release(); device_context_ = nullptr; }
    if (device_) { ((ID3D11Device*)device_)->Release(); device_ = nullptr; }
}

bool D3D11DisplayBackend::SetVSync(bool enabled) {
    vsync_enabled_ = enabled;
    return true;
}

// ============================================================================
// PaintModeDisplayBackend 实现
// ============================================================================

PaintModeDisplayBackend::PaintModeDisplayBackend() = default;

PaintModeDisplayBackend::~PaintModeDisplayBackend() {
    Shutdown();
}

bool PaintModeDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Windows 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (!hwnd_) {
        return false;
    }

    // 创建离屏缓冲区
    if (!CreateBuffer(width, height)) {
        return false;
    }

    return true;
}

bool PaintModeDisplayBackend::CreateBuffer(int width, int height) {
    // 如果已有缓冲区且尺寸匹配，直接返回
    if (hbitmap_ && bitmap_width_ == width && bitmap_height_ == height) {
        return true;
    }

    // 销毁旧缓冲区
    DestroyBuffer();

    HWND hwnd = (HWND)hwnd_;
    HDC hdc_window = GetDC(hwnd);
    if (!hdc_window) {
        return false;
    }

    // 创建内存 DC
    HDC hdc_mem = CreateCompatibleDC(hdc_window);
    if (!hdc_mem) {
        ReleaseDC(hwnd, hdc_window);
        return false;
    }

    // 创建 DIB 位图
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负数表示自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbitmap = CreateDIBSection(hdc_mem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);

    ReleaseDC(hwnd, hdc_window);

    if (!hbitmap || !bits) {
        DeleteDC(hdc_mem);
        return false;
    }

    // 选择位图到 DC
    HBITMAP old_bitmap = (HBITMAP)SelectObject(hdc_mem, hbitmap);

    hdc_mem_ = hdc_mem;
    hbitmap_ = hbitmap;
    hbitmap_old_ = old_bitmap;
    bitmap_bits_ = bits;
    bitmap_width_ = width;
    bitmap_height_ = height;

    return true;
}

void PaintModeDisplayBackend::DestroyBuffer() {
    if (hdc_mem_) {
        if (hbitmap_old_) {
            SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
            hbitmap_old_ = nullptr;
        }
        DeleteDC((HDC)hdc_mem_);
        hdc_mem_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
    }
    bitmap_bits_ = nullptr;
    bitmap_width_ = 0;
    bitmap_height_ = 0;
}

void PaintModeDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    // 默认实现：复制整个 surface
    PresentPartial(pixels, width, height, stride, 0, 0, width, height);
}

void PaintModeDisplayBackend::PresentPartial(const void* pixels, int width, int height, int stride,
                                              int dirty_x, int dirty_y, int dirty_width, int dirty_height) {
    if (!hwnd_ || !bitmap_bits_) return;

    // 如果尺寸变化，重建缓冲区
    if (width != bitmap_width_ || height != bitmap_height_) {
        if (!CreateBuffer(width, height)) {
            return;
        }
    }

    // 边界检查
    if (dirty_x < 0) dirty_x = 0;
    if (dirty_y < 0) dirty_y = 0;
    if (dirty_x + dirty_width > width) dirty_width = width - dirty_x;
    if (dirty_y + dirty_height > height) dirty_height = height - dirty_y;
    if (dirty_width <= 0 || dirty_height <= 0) return;

    // 只复制脏区域的像素到离屏缓冲区
    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)bitmap_bits_;
    int dst_stride = bitmap_width_ * 4;
    int bytes_per_pixel = 4;

    // 定位到脏区域的起始位置
    src += dirty_y * stride + dirty_x * bytes_per_pixel;
    dst += dirty_y * dst_stride + dirty_x * bytes_per_pixel;

    // 只复制脏区域的行
    int copy_width = dirty_width * bytes_per_pixel;
    for (int y = 0; y < dirty_height; y++) {
        memcpy(dst, src, copy_width);
        src += stride;
        dst += dst_stride;
    }

    // 只更新脏区域到窗口
    HWND hwnd = (HWND)hwnd_;
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        // 使用 GDI 的 BitBlt 只复制脏区域
        BitBlt(hdc, dirty_x, dirty_y, dirty_width, dirty_height,
               (HDC)hdc_mem_, dirty_x, dirty_y, SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }

    has_pending_paint_ = false;
}

void PaintModeDisplayBackend::OnPaint(void* hdc, const void* paint_rect) {
    if (!hdc || !hdc_mem_) return;

    HDC hdc_target = (HDC)hdc;
    HDC hdc_source = (HDC)hdc_mem_;

    // 如果提供了绘制区域，只绘制该区域（优化）
    if (paint_rect) {
        const RECT* rc = (const RECT*)paint_rect;
        BitBlt(hdc_target, rc->left, rc->top,
               rc->right - rc->left, rc->bottom - rc->top,
               hdc_source, rc->left, rc->top, SRCCOPY);
    } else {
        // 绘制整个缓冲区
        BitBlt(hdc_target, 0, 0, bitmap_width_, bitmap_height_,
               hdc_source, 0, 0, SRCCOPY);
    }

    has_pending_paint_ = false;
}

void PaintModeDisplayBackend::OnResize(int width, int height) {
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    // 重建缓冲区
    CreateBuffer(width, height);
}

void PaintModeDisplayBackend::Shutdown() {
    DestroyBuffer();
    hwnd_ = nullptr;
}

#endif // _WIN32

// ============================================================================
// OpenGLDisplayBackend 实现
// ============================================================================

OpenGLDisplayBackend::OpenGLDisplayBackend() = default;

OpenGLDisplayBackend::~OpenGLDisplayBackend() {
    Shutdown();
}

bool OpenGLDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 设置 OpenGL 属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // 创建 OpenGL 上下文
    gl_context_ = SDL_GL_CreateContext(window);
    if (!gl_context_) {
        return false;
    }

    SDL_GL_MakeCurrent(window, (SDL_GLContext)gl_context_);

    // 启用 VSync
    SetVSync(true);

    // 创建纹理
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 预分配纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);

    // 设置正交投影
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    return true;
}

void OpenGLDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!gl_context_ || !texture_id_) return;

    SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);

    // 设置视口
    glViewport(0, 0, width, height);

    // 更新纹理
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // 如果尺寸变化，重新分配纹理
    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

        // 更新投影
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        // 使用 glTexSubImage2D 更高效
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    }

    // 清屏
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 确保纹理已启用
    glEnable(GL_TEXTURE_2D);

    // 绘制全屏四边形
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f((float)width, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f((float)width, (float)height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)height);
    glEnd();

    // 交换缓冲区
    SDL_GL_SwapWindow(window_);
}

void OpenGLDisplayBackend::OnResize(int width, int height) {
    if (!gl_context_) return;

    SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    width_ = width;
    height_ = height;
}

void OpenGLDisplayBackend::Shutdown() {
    if (gl_context_) {
        SDL_GL_MakeCurrent(window_, (SDL_GLContext)gl_context_);
        
        if (texture_id_) {
            glDeleteTextures(1, &texture_id_);
            texture_id_ = 0;
        }

        SDL_GL_DestroyContext((SDL_GLContext)gl_context_);
        gl_context_ = nullptr;
    }
}

bool OpenGLDisplayBackend::SetVSync(bool enabled) {
    vsync_enabled_ = enabled;
    int result = SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    return result == 0;
}

// ============================================================================
// SDLSurfaceDisplayBackend 实现
// ============================================================================

SDLSurfaceDisplayBackend::SDLSurfaceDisplayBackend() = default;

SDLSurfaceDisplayBackend::~SDLSurfaceDisplayBackend() {
    Shutdown();
}

bool SDLSurfaceDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    sdl_surface_ = SDL_GetWindowSurface(window);
    if (!sdl_surface_) {
        return false;
    }

    return true;
}

void SDLSurfaceDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    SDL_Surface* surface = (SDL_Surface*)sdl_surface_;
    if (!surface) {
        sdl_surface_ = SDL_GetWindowSurface(window_);
        surface = (SDL_Surface*)sdl_surface_;
        if (!surface) return;
    }

    // 锁定表面
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    // 复制像素数据
    int copy_height = std::min(height, surface->h);
    int copy_width = std::min(width, surface->w);
    int src_stride = stride;
    int dst_stride = surface->pitch;

    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)surface->pixels;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * dst_stride, src + y * src_stride, copy_width * 4);
    }

    // 解锁表面
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    // 更新窗口
    SDL_UpdateWindowSurface(window_);
}

void SDLSurfaceDisplayBackend::OnResize(int width, int height) {
    width_ = width;
    height_ = height;
    // SDL Surface 会在窗口大小变化时自动更新
    sdl_surface_ = SDL_GetWindowSurface(window_);
}

void SDLSurfaceDisplayBackend::Shutdown() {
    // SDL_Surface 由 SDL 管理，不需要手动释放
    sdl_surface_ = nullptr;
}

// ============================================================================
// Windows 特定后端实现
// ============================================================================

#ifdef _WIN32

// ----------------------------------------------------------------------------
// LayeredWindowDisplayBackend 实现
// ----------------------------------------------------------------------------

LayeredWindowDisplayBackend::LayeredWindowDisplayBackend() = default;

LayeredWindowDisplayBackend::~LayeredWindowDisplayBackend() {
    Shutdown();
}

bool LayeredWindowDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Win32 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                           SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd_) {
        return false;
    }

    HWND hWnd = (HWND)hwnd_;

    // 设置为分层窗口
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

    // 创建兼容 DC 和位图
    HDC hdcScreen = GetDC(NULL);
    hdc_mem_ = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);

    if (!hdc_mem_) {
        return false;
    }

    // 创建 DIB Section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (!hbitmap_) {
        Shutdown();
        return false;
    }

    hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
    bitmap_width_ = width;
    bitmap_height_ = height;

    return true;
}

void LayeredWindowDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!hwnd_ || !hdc_mem_ || !bitmap_bits_) return;

    // 检查是否需要重建位图
    if (width != bitmap_width_ || height != bitmap_height_) {
        OnResize(width, height);
    }

    // 复制像素数据到 DIB
    int copy_height = std::min(height, bitmap_height_);
    int copy_width = std::min(width, bitmap_width_);
    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)bitmap_bits_;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * bitmap_width_ * 4, src + y * stride, copy_width * 4);
    }

    // 使用 UpdateLayeredWindow
    HWND hWnd = (HWND)hwnd_;
    HDC hdcScreen = GetDC(NULL);

    POINT ptDst = {0, 0};
    RECT rc;
    GetWindowRect(hWnd, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    SIZE size = {bitmap_width_, bitmap_height_};
    POINT ptSrc = {0, 0};

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;  // 使用源的 alpha

    // 注意：对于不透明窗口，使用 ULW_OPAQUE 会更快
    // 但 Skia 输出是预乘 alpha，我们使用 AC_SRC_ALPHA
    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &size, (HDC)hdc_mem_, &ptSrc, 0, &blend, ULW_ALPHA);

    ReleaseDC(NULL, hdcScreen);
}

void LayeredWindowDisplayBackend::OnResize(int width, int height) {
    if (width == bitmap_width_ && height == bitmap_height_) return;

    // 释放旧位图
    if (hdc_mem_ && hbitmap_old_) {
        SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
        hbitmap_old_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
        bitmap_bits_ = nullptr;
    }

    // 创建新位图
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (hbitmap_) {
        hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
        bitmap_width_ = width;
        bitmap_height_ = height;
    }

    width_ = width;
    height_ = height;
}

void LayeredWindowDisplayBackend::Shutdown() {
    if (hdc_mem_) {
        if (hbitmap_old_) {
            SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
            hbitmap_old_ = nullptr;
        }
        DeleteDC((HDC)hdc_mem_);
        hdc_mem_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
    }
    bitmap_bits_ = nullptr;

    // 移除分层窗口样式
    if (hwnd_) {
        HWND hWnd = (HWND)hwnd_;
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        SetWindowLong(hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        hwnd_ = nullptr;
    }
}

// ----------------------------------------------------------------------------
// GDIDisplayBackend 实现
// ----------------------------------------------------------------------------

GDIDisplayBackend::GDIDisplayBackend() = default;

GDIDisplayBackend::~GDIDisplayBackend() {
    Shutdown();
}

bool GDIDisplayBackend::Initialize(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // 获取 Win32 窗口句柄
    hwnd_ = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                           SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd_) {
        return false;
    }

    // 创建兼容 DC
    HDC hdcWindow = GetDC((HWND)hwnd_);
    hdc_mem_ = CreateCompatibleDC(hdcWindow);
    ReleaseDC((HWND)hwnd_, hdcWindow);

    if (!hdc_mem_) {
        return false;
    }

    // 创建 DIB Section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示自上而下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (!hbitmap_) {
        Shutdown();
        return false;
    }

    hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
    bitmap_width_ = width;
    bitmap_height_ = height;

    return true;
}

void GDIDisplayBackend::Present(const void* pixels, int width, int height, int stride) {
    if (!hwnd_ || !hdc_mem_ || !bitmap_bits_) return;

    // 检查是否需要重建位图
    if (width != bitmap_width_ || height != bitmap_height_) {
        OnResize(width, height);
    }

    // 复制像素数据到 DIB
    int copy_height = std::min(height, bitmap_height_);
    int copy_width = std::min(width, bitmap_width_);
    const uint8_t* src = (const uint8_t*)pixels;
    uint8_t* dst = (uint8_t*)bitmap_bits_;

    for (int y = 0; y < copy_height; y++) {
        std::memcpy(dst + y * bitmap_width_ * 4, src + y * stride, copy_width * 4);
    }

    // 使用 BitBlt 复制到窗口
    HDC hdcWindow = GetDC((HWND)hwnd_);
    if (hdcWindow) {
        BitBlt(hdcWindow, 0, 0, bitmap_width_, bitmap_height_, (HDC)hdc_mem_, 0, 0, SRCCOPY);
        ReleaseDC((HWND)hwnd_, hdcWindow);
    }

    // 验证整个客户区，阻止 WM_PAINT 消息
    ValidateRect((HWND)hwnd_, NULL);
}

void GDIDisplayBackend::OnResize(int width, int height) {
    if (width == bitmap_width_ && height == bitmap_height_) return;

    // 释放旧位图
    if (hdc_mem_ && hbitmap_old_) {
        SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
        hbitmap_old_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
        bitmap_bits_ = nullptr;
    }

    // 创建新位图
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection((HDC)hdc_mem_, &bmi, DIB_RGB_COLORS, &bitmap_bits_, NULL, 0);
    if (hbitmap_) {
        hbitmap_old_ = SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_);
        bitmap_width_ = width;
        bitmap_height_ = height;
    }

    width_ = width;
    height_ = height;
}

void GDIDisplayBackend::Shutdown() {
    if (hdc_mem_) {
        if (hbitmap_old_) {
            SelectObject((HDC)hdc_mem_, (HBITMAP)hbitmap_old_);
            hbitmap_old_ = nullptr;
        }
        DeleteDC((HDC)hdc_mem_);
        hdc_mem_ = nullptr;
    }
    if (hbitmap_) {
        DeleteObject((HBITMAP)hbitmap_);
        hbitmap_ = nullptr;
    }
    bitmap_bits_ = nullptr;
    hwnd_ = nullptr;
}

#endif // _WIN32

} // namespace lightui

