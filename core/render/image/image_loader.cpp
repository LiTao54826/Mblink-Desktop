/**
 * @file image_loader.cpp
 * @brief 图片加载器实现
 */

#include "image_loader.h"
#include "include/core/SkImage.h"
#include "include/codec/SkCodec.h"
#include <thread>
#include <fstream>

namespace lightui {

// ========== 同步加载 ==========

sk_sp<SkImage> ImageLoader::LoadFromFile(const std::string& path) {
    // 使用 Skia 的图片解码器
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

