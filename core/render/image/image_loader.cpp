/**
 * @file image_loader.cpp
 * @brief 图片加载器实现
 */

#include "image_loader.h"
#include "image_cache.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/codec/SkCodec.h"
#include "core/network/http_client.h"
#include "core/utils/async_resource_context.h"
#include "core/utils/background_task_runner.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <deque>
#include <mutex>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

namespace mbink {
namespace {

struct ImageAsyncContextEntry {
    uint64_t owner_token = 0;
    std::weak_ptr<BackgroundTaskRunner> runner;
    std::weak_ptr<AsyncResourceContext> resource_context;
};

struct ImageAsyncContext {
    std::shared_ptr<BackgroundTaskRunner> runner;
    std::shared_ptr<AsyncResourceContext> resource_context;
};

std::mutex g_image_async_context_mutex;
std::deque<ImageAsyncContextEntry> g_image_async_contexts;

std::shared_ptr<BackgroundTaskRunner> FallbackImageRunner() {
    static auto runner = std::make_shared<BackgroundTaskRunner>();
    return runner;
}

ImageAsyncContext SelectImageAsyncContext() {
    std::lock_guard<std::mutex> lock(g_image_async_context_mutex);

    for (auto it = g_image_async_contexts.begin(); it != g_image_async_contexts.end();) {
        if (it->runner.expired() || it->resource_context.expired()) {
            it = g_image_async_contexts.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = g_image_async_contexts.rbegin(); it != g_image_async_contexts.rend(); ++it) {
        auto runner = it->runner.lock();
        auto resource_context = it->resource_context.lock();
        if (runner && resource_context && resource_context->IsAlive()) {
            return {std::move(runner), std::move(resource_context)};
        }
    }

    return {};
}

template <typename Task, typename Drop>
bool PostImageAsyncTask(const ImageAsyncContext& context, Task&& task, Drop&& on_drop) {
    auto runner = context.runner ? context.runner : FallbackImageRunner();
    if (!runner || runner->IsShuttingDown()) {
        on_drop();
        return false;
    }
    if (!runner->Post(std::forward<Task>(task), std::forward<Drop>(on_drop), "ImageLoader async")) {
        on_drop();
        return false;
    }
    return true;
}

bool IsAbsoluteImagePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
#ifdef _WIN32
    return (path.size() >= 2 && path[1] == ':') ||
           (path.size() >= 2 && path[0] == '\\' && path[1] == '\\');
#else
    return path[0] == '/';
#endif
}

std::string ResolveImagePathWithBase(const std::string& path, const std::string& base_path) {
    if (path.empty() ||
        ImageLoader::IsNetworkUrl(path) ||
        ImageLoader::IsDataUrl(path) ||
        ImageLoader::IsExeIconUrl(path) ||
        IsAbsoluteImagePath(path) ||
        base_path.empty()) {
        return path;
    }

    std::string base = base_path;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') {
        base += '/';
    }
    return base + path;
}

sk_sp<SkImage> LoadFromFileWithSnapshot(const std::string& path,
                                        const AsyncResourceContext::Snapshot& snapshot) {
    if (ImageLoader::IsNetworkUrl(path) || ImageLoader::IsDataUrl(path)) {
        return ImageLoader::LoadFromUrl(path);
    }

    if (snapshot.asset_provider) {
        std::vector<uint8_t> asset_data;
        if (snapshot.asset_provider(path, asset_data)) {
            return ImageLoader::LoadFromMemory(asset_data.data(), asset_data.size());
        }
    }

    const std::string resolved_path = ResolveImagePathWithBase(path, snapshot.base_path);
    sk_sp<SkData> data = SkData::MakeFromFileName(resolved_path.c_str());
    if (!data) {
        return nullptr;
    }
    return SkImages::DeferredFromEncodedData(data);
}

ImageLoadResult LoadFromUrlWithSnapshot(const std::string& url,
                                        const AsyncResourceContext::Snapshot& snapshot) {
    ImageLoadResult result;
    if (url.empty()) {
        result.error = "Empty URL";
        return result;
    }

    if (ImageLoader::IsExeIconUrl(url) || ImageLoader::IsNetworkUrl(url) || ImageLoader::IsDataUrl(url)) {
        return ImageLoader::LoadFromUrlWithResult(url);
    }

    const std::string resolved_url = ResolveImagePathWithBase(url, snapshot.base_path);
    sk_sp<SkImage> cached = ImageCache::GetInstance().Get(resolved_url);
    if (cached) {
        result.image = cached;
        result.natural_width = cached->width();
        result.natural_height = cached->height();
        result.success = true;
        return result;
    }

    result.image = LoadFromFileWithSnapshot(resolved_url, snapshot);
    if (result.image) {
        result.natural_width = result.image->width();
        result.natural_height = result.image->height();
        result.success = true;
        ImageCache::GetInstance().Put(url, result.image);
    } else {
        result.error = "Failed to load image from file: " + resolved_url;
    }
    return result;
}

} // namespace

// ========== 静态成员 ==========

AssetProvider ImageLoader::asset_provider_ = nullptr;
std::string ImageLoader::base_path_;

void ImageLoader::SetAssetProvider(AssetProvider provider) {
    asset_provider_ = provider;
}

AssetProvider ImageLoader::GetAssetProvider() {
    return asset_provider_;
}

void ImageLoader::SetBasePath(const std::string& path) {
    base_path_ = path;
}

const std::string& ImageLoader::GetBasePath() {
    return base_path_;
}

void ImageLoader::RegisterAsyncContext(uint64_t owner_token,
                                       std::weak_ptr<BackgroundTaskRunner> runner,
                                       std::weak_ptr<AsyncResourceContext> resource_context) {
    std::lock_guard<std::mutex> lock(g_image_async_context_mutex);
    for (auto& entry : g_image_async_contexts) {
        if (entry.owner_token == owner_token) {
            entry.runner = std::move(runner);
            entry.resource_context = std::move(resource_context);
            return;
        }
    }
    g_image_async_contexts.push_back({owner_token, std::move(runner), std::move(resource_context)});
}

void ImageLoader::UnregisterAsyncContext(uint64_t owner_token) {
    std::lock_guard<std::mutex> lock(g_image_async_context_mutex);
    for (auto it = g_image_async_contexts.begin(); it != g_image_async_contexts.end();) {
        if (it->owner_token == owner_token) {
            it = g_image_async_contexts.erase(it);
        } else {
            ++it;
        }
    }
}

// ========== URL 辅助方法 ==========

bool ImageLoader::IsNetworkUrl(const std::string& url) {
    return url.find("http://") == 0 || url.find("https://") == 0;
}

bool ImageLoader::IsDataUrl(const std::string& url) {
    return url.find("data:") == 0;
}

bool ImageLoader::IsExeIconUrl(const std::string& url) {
    if (url.empty()) {
        return false;
    }

    std::string normalized = url;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    size_t query_pos = normalized.find_first_of("?#");
    if (query_pos != std::string::npos) {
        normalized = normalized.substr(0, query_pos);
    }

    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }

    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return normalized == "app://res.ico";
}

// ========== EXE 图标加载 ==========

#ifdef _WIN32
sk_sp<SkImage> ImageLoader::LoadCurrentExeIcon() {
    // 1. 获取当前 exe 路径
    wchar_t exe_path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return nullptr;
    }

    // 2. 提取大图标 (尝试获取 256x256，回退到系统默认大小)
    HICON hIconLarge = NULL;

    // 先尝试 PrivateExtractIcons 获取高分辨率图标
    UINT icon_count = PrivateExtractIconsW(exe_path, 0, 256, 256, &hIconLarge, NULL, 1, 0);
    if (icon_count == 0 || hIconLarge == NULL) {
        // 回退到 48x48
        icon_count = PrivateExtractIconsW(exe_path, 0, 48, 48, &hIconLarge, NULL, 1, 0);
    }
    if (icon_count == 0 || hIconLarge == NULL) {
        // 最后回退到 ExtractIconExW
        ExtractIconExW(exe_path, 0, &hIconLarge, NULL, 1);
    }

    if (!hIconLarge) {
        return nullptr;
    }

    // 3. 获取图标信息
    ICONINFO icon_info = {0};
    if (!GetIconInfo(hIconLarge, &icon_info)) {
        DestroyIcon(hIconLarge);
        return nullptr;
    }

    // 4. 获取位图尺寸
    BITMAP bmp = {0};
    GetObject(icon_info.hbmColor ? icon_info.hbmColor : icon_info.hbmMask, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    if (width <= 0 || height <= 0) {
        if (icon_info.hbmColor) DeleteObject(icon_info.hbmColor);
        if (icon_info.hbmMask) DeleteObject(icon_info.hbmMask);
        DestroyIcon(hIconLarge);
        return nullptr;
    }

    // 5. 提取 BGRA 像素数据
    HDC hdc = GetDC(NULL);
    HDC mem_dc = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 自顶向下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(width * height * 4);

    if (icon_info.hbmColor) {
        // 有颜色位图，直接获取 BGRA 数据
        GetDIBits(mem_dc, icon_info.hbmColor, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
    }

    // 6. 用 DrawIconEx 绘制到 DIB 上，确保 alpha 通道正确
    // 某些图标的 alpha 通道可能全为 0，需要通过绘制来获取正确的像素
    void* dib_bits = nullptr;
    HBITMAP hDIB = CreateDIBSection(mem_dc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
    if (hDIB && dib_bits) {
        HBITMAP old_bmp = (HBITMAP)SelectObject(mem_dc, hDIB);

        // 先清除为全透明
        memset(dib_bits, 0, width * height * 4);

        // 绘制图标
        DrawIconEx(mem_dc, 0, 0, hIconLarge, width, height, 0, NULL, DI_NORMAL);

        SelectObject(mem_dc, old_bmp);

        // 复制绘制结果
        memcpy(pixels.data(), dib_bits, width * height * 4);
        DeleteObject(hDIB);
    }

    DeleteDC(mem_dc);
    ReleaseDC(NULL, hdc);

    // 7. BGRA -> RGBA 转换（Skia 在某些配置下需要 RGBA）
    // N32 在 Windows 上通常是 BGRA，直接用 kBGRA_8888
    SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    SkPixmap pixmap(info, pixels.data(), width * 4);
    sk_sp<SkImage> image = SkImages::RasterFromPixmapCopy(pixmap);

    // 8. 清理资源
    if (icon_info.hbmColor) DeleteObject(icon_info.hbmColor);
    if (icon_info.hbmMask) DeleteObject(icon_info.hbmMask);
    DestroyIcon(hIconLarge);

    return image;
}
#else
sk_sp<SkImage> ImageLoader::LoadCurrentExeIcon() {
    // 非 Windows 平台暂不支持
    return nullptr;
}
#endif

// ========== Base64 解码 ==========

std::vector<uint8_t> ImageLoader::DecodeBase64(const std::string& encoded) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::vector<uint8_t> decoded;
    
    // 创建解码表
    std::vector<int> decode_table(256, -1);
    for (size_t i = 0; i < base64_chars.size(); ++i) {
        decode_table[static_cast<unsigned char>(base64_chars[i])] = static_cast<int>(i);
    }
    
    int val = 0;
    int bits = -8;
    
    for (unsigned char c : encoded) {
        if (decode_table[c] == -1) {
            if (c == '=') break;  // 填充字符
            continue;  // 跳过空白字符
        }
        
        val = (val << 6) + decode_table[c];
        bits += 6;
        
        if (bits >= 0) {
            decoded.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    
    return decoded;
}

// ========== 网络加载 ==========

bool ImageLoader::FetchFromNetwork(const std::string& url, std::vector<uint8_t>& out_data) {
    HttpClient client;
    HttpRequestOptions options;
    options.timeout_ms = 30000;  // 30秒超时
    
    HttpResponse response = client.Get(url, options);
    
    if (!response.IsOk()) {
        return false;
    }
    
    // 复制响应体到输出
    out_data.assign(response.body.begin(), response.body.end());
    return !out_data.empty();
}

// ========== 同步加载 ==========

sk_sp<SkImage> ImageLoader::LoadFromFile(const std::string& path) {
    // 检查是否是URL
    if (IsNetworkUrl(path) || IsDataUrl(path)) {
        return LoadFromUrl(path);
    }
    
    // 1. 优先从嵌入资源加载
    if (asset_provider_) {
        std::vector<uint8_t> asset_data;
        if (asset_provider_(path, asset_data)) {
            // 从内存加载
            return LoadFromMemory(asset_data.data(), asset_data.size());
        }
    }
    
    // 2. 回退到文件系统
    sk_sp<SkData> data = SkData::MakeFromFileName(path.c_str());
    if (!data) {
        return nullptr;
    }

    return SkImages::DeferredFromEncodedData(data);
}

sk_sp<SkImage> ImageLoader::LoadFromMemory(const void* data, size_t size) {
    if (!data || size == 0) {
        return nullptr;
    }

    sk_sp<SkData> sk_data = SkData::MakeWithCopy(data, size);
    return SkImages::DeferredFromEncodedData(sk_data);
}

sk_sp<SkImage> ImageLoader::LoadFromData(sk_sp<SkData> data) {
    if (!data) {
        return nullptr;
    }

    return SkImages::DeferredFromEncodedData(data);
}

sk_sp<SkImage> ImageLoader::LoadFromUrl(const std::string& url) {
    auto result = LoadFromUrlWithResult(url);
    return result.image;
}

ImageLoadResult ImageLoader::LoadFromUrlWithResult(const std::string& url) {
    ImageLoadResult result;

    if (url.empty()) {
        result.error = "Empty URL";
        return result;
    }

    auto load_app_icon = [&]() {
        result.image = LoadCurrentExeIcon();
        if (result.image) {
            result.natural_width = result.image->width();
            result.natural_height = result.image->height();
            result.success = true;
            ImageCache::GetInstance().Put("app://res.ico", result.image);
        } else {
            result.error = "Failed to extract host app icon";
        }
        return result;
    };

    if (IsExeIconUrl(url)) {
        return load_app_icon();
    }

    // 解析相对路径
    std::string resolved_url = url;
    if (!IsNetworkUrl(url) && !IsDataUrl(url) && !base_path_.empty()) {
        // 检查是否是绝对路径
        bool is_absolute = false;
#ifdef _WIN32
        // Windows: 检查是否以盘符开头 (如 C:\) 或以 \\ 开头
        if (url.length() >= 2 && url[1] == ':') {
            is_absolute = true;
        } else if (url.length() >= 2 && url[0] == '\\' && url[1] == '\\') {
            is_absolute = true;
        }
#else
        // Unix: 检查是否以 / 开头
        if (!url.empty() && url[0] == '/') {
            is_absolute = true;
        }
#endif
        if (!is_absolute) {
            // 构建完整路径
            std::string base = base_path_;
            if (!base.empty() && base.back() != '/' && base.back() != '\\') {
                base += '/';
            }
            resolved_url = base + url;
        }
    }

    if (IsExeIconUrl(resolved_url)) {
        return load_app_icon();
    }

    // 检查缓存
    sk_sp<SkImage> cached = ImageCache::GetInstance().Get(resolved_url);
    if (cached) {
        result.image = cached;
        result.natural_width = cached->width();
        result.natural_height = cached->height();
        result.success = true;
        return result;
    }

    // 处理 data: URL
    if (IsDataUrl(url)) {
        result.image = LoadFromDataUrl(url);
        if (result.image) {
            result.natural_width = result.image->width();
            result.natural_height = result.image->height();
            result.success = true;
            // data: URL 不缓存
        } else {
            result.error = "Failed to decode data URL";
        }
        return result;
    }

    // 处理网络 URL
    if (IsNetworkUrl(url)) {
        std::vector<uint8_t> data;
        if (FetchFromNetwork(url, data)) {
            result.image = LoadFromMemory(data.data(), data.size());
            if (result.image) {
                result.natural_width = result.image->width();
                result.natural_height = result.image->height();
                result.success = true;
                // 添加到缓存
                ImageCache::GetInstance().Put(url, result.image);
            } else {
                result.error = "Failed to decode image data";
            }
        } else {
            result.error = "Failed to fetch image from network";
        }
        return result;
    }

    // 处理本地文件（使用解析后的路径）
    result.image = LoadFromFile(resolved_url);
    if (result.image) {
        result.natural_width = result.image->width();
        result.natural_height = result.image->height();
        result.success = true;
        // 添加到缓存（使用原始URL作为key）
        ImageCache::GetInstance().Put(url, result.image);
    } else {
        result.error = "Failed to load image from file: " + resolved_url;
    }

    return result;
}

sk_sp<SkImage> ImageLoader::LoadFromDataUrl(const std::string& data_url) {
    // 格式: data:[<mediatype>][;base64],<data>
    if (!IsDataUrl(data_url)) {
        return nullptr;
    }
    
    // 查找逗号分隔符
    size_t comma_pos = data_url.find(',');
    if (comma_pos == std::string::npos) {
        return nullptr;
    }
    
    std::string header = data_url.substr(5, comma_pos - 5);  // 跳过 "data:"
    std::string data_part = data_url.substr(comma_pos + 1);
    
    // 检查是否是 base64 编码
    bool is_base64 = header.find("base64") != std::string::npos;
    
    if (is_base64) {
        std::vector<uint8_t> decoded = DecodeBase64(data_part);
        if (!decoded.empty()) {
            return LoadFromMemory(decoded.data(), decoded.size());
        }
    } else {
        // URL 编码的数据（较少见）
        // 简单处理：直接尝试解码
        return LoadFromMemory(data_part.data(), data_part.size());
    }
    
    return nullptr;
}

// ========== 异步加载 ==========

void ImageLoader::LoadFromFileAsync(const std::string& path, ImageLoadCallback callback) {
    if (!callback) {
        return;
    }
    
    // 在新线程中加载图片
    auto context = SelectImageAsyncContext();
    auto snapshot = context.resource_context ? context.resource_context->MakeSnapshot()
                                             : AsyncResourceContext::Snapshot{base_path_, asset_provider_, true};
    PostImageAsyncTask(context, [path, callback, snapshot]() {
        sk_sp<SkImage> image = LoadFromFileWithSnapshot(path, snapshot);
        callback(image);
    }, [callback]() {
        callback(nullptr);
    });
}

void ImageLoader::LoadFromMemoryAsync(const void* data, size_t size, ImageLoadCallback callback) {
    if (!callback || !data || size == 0) {
        return;
    }
    
    // 复制数据到新的缓冲区
    std::vector<uint8_t> buffer(static_cast<const uint8_t*>(data), 
                                static_cast<const uint8_t*>(data) + size);
    
    // 在新线程中加载图片
    auto context = SelectImageAsyncContext();
    PostImageAsyncTask(context, [buffer = std::move(buffer), callback]() {
        sk_sp<SkImage> image = LoadFromMemory(buffer.data(), buffer.size());
        callback(image);
    }, [callback]() {
        callback(nullptr);
    });
}

void ImageLoader::LoadFromUrlAsync(const std::string& url, ImageLoadCallback callback) {
    if (!callback) {
        return;
    }
    
    auto context = SelectImageAsyncContext();
    auto snapshot = context.resource_context ? context.resource_context->MakeSnapshot()
                                             : AsyncResourceContext::Snapshot{base_path_, asset_provider_, true};
    PostImageAsyncTask(context, [url, callback, snapshot]() {
        sk_sp<SkImage> image = LoadFromUrlWithSnapshot(url, snapshot).image;
        callback(image);
    }, [callback]() {
        callback(nullptr);
    });
}

void ImageLoader::LoadFromUrlAsyncWithResult(const std::string& url, ImageLoadResultCallback callback) {
    if (!callback) {
        return;
    }
    
    auto context = SelectImageAsyncContext();
    auto snapshot = context.resource_context ? context.resource_context->MakeSnapshot()
                                             : AsyncResourceContext::Snapshot{base_path_, asset_provider_, true};
    PostImageAsyncTask(context, [url, callback, snapshot]() {
        ImageLoadResult result = LoadFromUrlWithSnapshot(url, snapshot);
        callback(result);
    }, [callback]() {
        ImageLoadResult result;
        result.error = "Background task runner is not available";
        callback(result);
    });
}

// ========== 格式检测 ==========

std::string ImageLoader::DetectFormat(const std::string& path) {
    // 读取文件头
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "unknown";
    }
    
    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    size_t bytes_read = file.gcount();
    file.close();
    
    return DetectFormatFromData(header, bytes_read);
}

std::string ImageLoader::DetectFormatFromData(const void* data, size_t size) {
    if (!data || size < 4) {
        return "unknown";
    }
    
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    
    // PNG: 89 50 4E 47
    if (size >= 4 && bytes[0] == 0x89 && bytes[1] == 0x50 && 
        bytes[2] == 0x4E && bytes[3] == 0x47) {
        return "png";
    }
    
    // JPEG: FF D8 FF
    if (size >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
        return "jpeg";
    }
    
    // WebP: RIFF ... WEBP
    if (size >= 12 && bytes[0] == 'R' && bytes[1] == 'I' && 
        bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'E' && 
        bytes[10] == 'B' && bytes[11] == 'P') {
        return "webp";
    }
    
    // GIF: GIF87a or GIF89a
    if (size >= 6 && bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F') {
        return "gif";
    }
    
    // BMP: BM
    if (size >= 2 && bytes[0] == 'B' && bytes[1] == 'M') {
        return "bmp";
    }
    
    return "unknown";
}

} // namespace mbink

