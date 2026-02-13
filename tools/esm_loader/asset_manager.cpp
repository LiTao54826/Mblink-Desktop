/**
 * @file asset_manager.cpp
 * @brief 嵌入资源管理器实现
 */

#include "asset_manager.h"
#include "payload.h"
#include <algorithm>

namespace lightui {

AssetManager& AssetManager::Instance() {
    static AssetManager instance;
    return instance;
}

void AssetManager::Initialize(const std::vector<uint8_t>& assets_data,
                               const std::vector<mbink::AssetInfo>& assets_index) {
    Clear();
    data_ = assets_data;
    
    for (const auto& info : assets_index) {
        index_[info.name] = {info.offset, info.size};
    }
}

void AssetManager::Clear() {
    data_.clear();
    index_.clear();
}

bool AssetManager::HasAsset(const std::string& path) const {
    std::string normalized = path;
    
    // 规范化路径
    if (!path.empty() && path[0] == '/') {
        normalized = path.substr(1);
    } else if (path.size() > 2 && path[0] == '.' && path[1] == '/') {
        normalized = path.substr(2);
    }
    
    // 将反斜杠转换为正斜杠
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }
    
    if (index_.find(normalized) != index_.end()) return true;
    if (index_.find(path) != index_.end()) return true;
    
    return false;
}

bool AssetManager::GetAsset(const std::string& path, std::vector<uint8_t>& out_data) const {
    std::string normalized = path;
    
    // 规范化路径
    if (!path.empty() && path[0] == '/') {
        normalized = path.substr(1);
    } else if (path.size() > 2 && path[0] == '.' && path[1] == '/') {
        normalized = path.substr(2);
    }
    
    // 将反斜杠转换为正斜杠
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }
    
    auto it = index_.find(normalized);
    if (it == index_.end()) {
        // 尝试原始路径
        it = index_.find(path);
        if (it == index_.end()) return false;
    }
    
    const auto& entry = it->second;
    if (entry.offset + entry.size > data_.size()) return false;
    
    out_data.assign(
        data_.begin() + entry.offset,
        data_.begin() + entry.offset + entry.size
    );
    return true;
}

std::string AssetManager::GetMimeType(const std::string& path) {
    // 获取扩展名
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // 常见 MIME 类型
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "webp") return "image/webp";
    if (ext == "bmp") return "image/bmp";
    
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "xml") return "application/xml";
    if (ext == "txt") return "text/plain";
    
    if (ext == "ttf") return "font/ttf";
    if (ext == "otf") return "font/otf";
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "eot") return "application/vnd.ms-fontobject";
    
    if (ext == "mp3") return "audio/mpeg";
    if (ext == "wav") return "audio/wav";
    if (ext == "ogg") return "audio/ogg";
    
    if (ext == "mp4") return "video/mp4";
    if (ext == "webm") return "video/webm";
    
    if (ext == "pdf") return "application/pdf";
    if (ext == "zip") return "application/zip";
    
    return "application/octet-stream";
}

std::string AssetManager::Base64Encode(const uint8_t* data, size_t length) {
    static const char* chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
  result.reserve((length + 2) / 3 * 4);
    
    for (size_t i = 0; i < length; i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < length) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < length) n |= static_cast<uint32_t>(data[i + 2]);
        
        result += chars[(n >> 18) & 0x3F];
        result += chars[(n >> 12) & 0x3F];
        result += (i + 1 < length) ? chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < length) ? chars[n & 0x3F] : '=';
    }
    
    return result;
}

std::string AssetManager::GetAssetDataUrl(const std::string& path) const {
    std::vector<uint8_t> data;
    if (!GetAsset(path, data)) return "";
    
    std::string mime = GetMimeType(path);
    std::string base64 = Base64Encode(data.data(), data.size());
    
    return "data:" + mime + ";base64," + base64;
}

std::vector<std::string> AssetManager::GetAssetPaths() const {
    std::vector<std::string> paths;
    paths.reserve(index_.size());
    for (const auto& pair : index_) {
        paths.push_back(pair.first);
    }
    return paths;
}

}  // namespace lightui

