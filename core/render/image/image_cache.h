/**
 * @file image_cache.h
 * @brief 图片缓存管理
 * 
 * 功能：
 * - 图片缓存系统
 * - 避免重复加载
 * - LRU 缓存策略
 * - 内存使用管理
 */

#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include "include/core/SkImage.h"

namespace mbink {

/**
 * @brief 图片缓存项
 */
struct ImageCacheEntry {
    std::string key;        ///< 缓存键
    sk_sp<SkImage> image;   ///< 图片
    size_t size;            ///< 图片大小（字节）
    
    ImageCacheEntry(const std::string& k, sk_sp<SkImage> img, size_t s)
        : key(k), image(img), size(s) {}
};

/**
 * @brief 图片缓存类
 * 
 * 使用 LRU (Least Recently Used) 策略管理图片缓存
 */
class ImageCache {
public:
    /**
     * @brief 获取单例实例
     */
    static ImageCache& GetInstance();
    
    /**
     * @brief 设置最大缓存大小
     * @param max_size 最大缓存大小（字节）
     */
    void SetMaxCacheSize(size_t max_size);
    
    /**
     * @brief 获取最大缓存大小
     * @return 最大缓存大小（字节）
     */
    size_t GetMaxCacheSize() const { return max_cache_size_; }
    
    /**
     * @brief 获取当前缓存大小
     * @return 当前缓存大小（字节）
     */
    size_t GetCurrentCacheSize() const { return current_cache_size_; }
    
    /**
     * @brief 添加图片到缓存
     * @param key 缓存键
     * @param image 图片
     */
    void Put(const std::string& key, sk_sp<SkImage> image);
    
    /**
     * @brief 从缓存获取图片
     * @param key 缓存键
     * @return 图片智能指针，如果不存在返回 nullptr
     */
    sk_sp<SkImage> Get(const std::string& key);
    
    /**
     * @brief 检查缓存中是否存在指定键
     * @param key 缓存键
     * @return true 表示存在
     */
    bool Contains(const std::string& key) const;
    
    /**
     * @brief 从缓存中移除指定键
     * @param key 缓存键
     */
    void Remove(const std::string& key);
    
    /**
     * @brief 清空缓存
     */
    void Clear();
    
    /**
     * @brief 获取缓存项数量
     * @return 缓存项数量
     */
    size_t GetCacheCount() const { return cache_map_.size(); }

private:
    ImageCache();
    ~ImageCache() = default;
    
    // 禁止拷贝和赋值
    ImageCache(const ImageCache&) = delete;
    ImageCache& operator=(const ImageCache&) = delete;
    
    /**
     * @brief 驱逐最少使用的缓存项
     */
    void EvictLRU();
    
    /**
     * @brief 计算图片大小
     */
    size_t CalculateImageSize(sk_sp<SkImage> image) const;

private:
    using CacheList = std::list<ImageCacheEntry>;
    using CacheMap = std::unordered_map<std::string, CacheList::iterator>;
    
    CacheList cache_list_;          ///< LRU 列表（最近使用的在前）
    CacheMap cache_map_;            ///< 键到列表迭代器的映射
    size_t max_cache_size_;         ///< 最大缓存大小（字节）
    size_t current_cache_size_;     ///< 当前缓存大小（字节）
};

} // namespace mbink

