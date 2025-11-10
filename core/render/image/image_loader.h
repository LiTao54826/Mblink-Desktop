/**
 * @file image_loader.h
 * @brief 图片加载器
 * 
 * 功能：
 * - 从文件加载图片
 * - 从内存加载图片
 * - 支持 PNG, JPEG, WebP 格式
 * - 异步加载机制
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include "include/core/SkImage.h"
#include "include/core/SkData.h"

namespace lightui {

/**
 * @brief 图片加载回调
 */
using ImageLoadCallback = std::function<void(sk_sp<SkImage>)>;

/**
 * @brief 图片加载器类
 */
class ImageLoader {
public:
    /**
     * @brief 构造函数
     */
    ImageLoader() = default;
    
    /**
     * @brief 析构函数
     */
    ~ImageLoader() = default;
    
    // ========== 同步加载 ==========
    
    /**
     * @brief 从文件加载图片
     * @param path 图片文件路径
     * @return SkImage 智能指针
     */
    static sk_sp<SkImage> LoadFromFile(const std::string& path);
    
    /**
     * @brief 从内存加载图片
     * @param data 图片数据
     * @param size 数据大小
     * @return SkImage 智能指针
     */
    static sk_sp<SkImage> LoadFromMemory(const void* data, size_t size);
    
    /**
     * @brief 从 SkData 加载图片
     * @param data SkData 智能指针
     * @return SkImage 智能指针
     */
    static sk_sp<SkImage> LoadFromData(sk_sp<SkData> data);
    
    // ========== 异步加载 ==========
    
    /**
     * @brief 异步从文件加载图片
     * @param path 图片文件路径
     * @param callback 加载完成回调
     */
    static void LoadFromFileAsync(const std::string& path, ImageLoadCallback callback);
    
    /**
     * @brief 异步从内存加载图片
     * @param data 图片数据
     * @param size 数据大小
     * @param callback 加载完成回调
     */
    static void LoadFromMemoryAsync(const void* data, size_t size, ImageLoadCallback callback);
    
    // ========== 格式检测 ==========
    
    /**
     * @brief 检测图片格式
     * @param path 图片文件路径
     * @return 格式字符串 (png, jpeg, webp, etc.)
     */
    static std::string DetectFormat(const std::string& path);
    
    /**
     * @brief 检测图片格式（从数据）
     * @param data 图片数据
     * @param size 数据大小
     * @return 格式字符串
     */
    static std::string DetectFormatFromData(const void* data, size_t size);
};

} // namespace lightui

