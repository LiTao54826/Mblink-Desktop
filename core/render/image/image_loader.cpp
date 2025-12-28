/**
 * @file image_loader.cpp
 * @brief 图片加载器实现
 */

#include "image_loader.h"
#include "image_cache.h"
#include "include/core/SkImage.h"
#include "include/codec/SkCodec.h"
#include "core/network/http_client.h"
#include <thread>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace lightui {

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

// ========== URL 辅助方法 ==========

bool ImageLoader::IsNetworkUrl(const std::string& url) {
    return url.find("http://") == 0 || url.find("https://") == 0;
}

bool ImageLoader::IsDataUrl(const std::string& url) {
    return url.find("data:") == 0;
}

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
    std::thread([path, callback]() {
        sk_sp<SkImage> image = LoadFromFile(path);
        callback(image);
    }).detach();
}

void ImageLoader::LoadFromMemoryAsync(const void* data, size_t size, ImageLoadCallback callback) {
    if (!callback || !data || size == 0) {
        return;
    }
    
    // 复制数据到新的缓冲区
    std::vector<uint8_t> buffer(static_cast<const uint8_t*>(data), 
                                static_cast<const uint8_t*>(data) + size);
    
    // 在新线程中加载图片
    std::thread([buffer = std::move(buffer), callback]() {
        sk_sp<SkImage> image = LoadFromMemory(buffer.data(), buffer.size());
        callback(image);
    }).detach();
}

void ImageLoader::LoadFromUrlAsync(const std::string& url, ImageLoadCallback callback) {
    if (!callback) {
        return;
    }
    
    std::thread([url, callback]() {
        sk_sp<SkImage> image = LoadFromUrl(url);
        callback(image);
    }).detach();
}

void ImageLoader::LoadFromUrlAsyncWithResult(const std::string& url, ImageLoadResultCallback callback) {
    if (!callback) {
        return;
    }
    
    std::thread([url, callback]() {
        ImageLoadResult result = LoadFromUrlWithResult(url);
        callback(result);
    }).detach();
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

} // namespace lightui

