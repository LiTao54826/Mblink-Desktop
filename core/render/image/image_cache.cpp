/**
 * @file image_cache.cpp
 * @brief 图片缓存管理实现
 */

#include "image_cache.h"

namespace mblink {

// ========== 构造函数 ==========

ImageCache::ImageCache()
    : cache_list_()
    , cache_map_()
    , max_cache_size_(100 * 1024 * 1024)  // 默认 100MB
    , current_cache_size_(0) {
}

ImageCache& ImageCache::GetInstance() {
    static ImageCache instance;
    return instance;
}

// ========== 缓存大小管理 ==========

void ImageCache::SetMaxCacheSize(size_t max_size) {
    max_cache_size_ = max_size;
    
    // 如果当前缓存超过新的最大值，驱逐多余的项
    while (current_cache_size_ > max_cache_size_ && !cache_list_.empty()) {
        EvictLRU();
    }
}

// ========== 缓存操作 ==========

void ImageCache::Put(const std::string& key, sk_sp<SkImage> image) {
    if (!image) {
        return;
    }
    
    // 如果键已存在，先移除旧的
    if (Contains(key)) {
        Remove(key);
    }
    
    // 计算图片大小
    size_t image_size = CalculateImageSize(image);
    
    // 如果图片太大，不缓存
    if (image_size > max_cache_size_) {
        return;
    }
    
    // 驱逐缓存直到有足够空间
    while (current_cache_size_ + image_size > max_cache_size_ && !cache_list_.empty()) {
        EvictLRU();
    }
    
    // 添加到缓存列表前端（最近使用）
    cache_list_.emplace_front(key, image, image_size);
    cache_map_[key] = cache_list_.begin();
    current_cache_size_ += image_size;
}

sk_sp<SkImage> ImageCache::Get(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        return nullptr;
    }
    
    // 移动到列表前端（标记为最近使用）
    cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
    
    return it->second->image;
}

bool ImageCache::Contains(const std::string& key) const {
    return cache_map_.find(key) != cache_map_.end();
}

void ImageCache::Remove(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        return;
    }
    
    // 更新缓存大小
    current_cache_size_ -= it->second->size;
    
    // 从列表和映射中移除
    cache_list_.erase(it->second);
    cache_map_.erase(it);
}

void ImageCache::Clear() {
    cache_list_.clear();
    cache_map_.clear();
    current_cache_size_ = 0;
}

// ========== 私有辅助方法 ==========

void ImageCache::EvictLRU() {
    if (cache_list_.empty()) {
        return;
    }
    
    // 移除列表末尾的项（最少使用）
    const ImageCacheEntry& entry = cache_list_.back();
    current_cache_size_ -= entry.size;
    cache_map_.erase(entry.key);
    cache_list_.pop_back();
}

size_t ImageCache::CalculateImageSize(sk_sp<SkImage> image) const {
    if (!image) {
        return 0;
    }
    
    // 估算图片大小：宽 * 高 * 4 (RGBA)
    return image->width() * image->height() * 4;
}

} // namespace mblink

