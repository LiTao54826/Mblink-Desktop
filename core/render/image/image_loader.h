/**
 * @file image_loader.h
 * @brief 图片加载器
 * 
 * 功能：
 * - 从文件加载图片
 * - 从内存加载图片
 * - 从网络URL加载图片
 * - 支持 PNG, JPEG, WebP, GIF, BMP 格式
 * - 异步加载机制
 * - 支持嵌入资源加载
 * - 支持 data: URL
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include "include/core/SkImage.h"
#include "include/core/SkData.h"

namespace mbink {

/**
 * @brief 图片加载结果
 */
struct ImageLoadResult {
    sk_sp<SkImage> image;       ///< 加载的图片
    int natural_width = 0;      ///< 原始宽度
    int natural_height = 0;     ///< 原始高度
    std::string error;          ///< 错误信息
    bool success = false;       ///< 是否成功
    
    ImageLoadResult() = default;
    ImageLoadResult(sk_sp<SkImage> img) 
        : image(img)
        , natural_width(img ? img->width() : 0)
        , natural_height(img ? img->height() : 0)
        , success(img != nullptr) {}
};

/**
 * @brief 图片加载回调
 */
using ImageLoadCallback = std::function<void(sk_sp<SkImage>)>;

/**
 * @brief 图片加载结果回调（带详细信息）
 */
using ImageLoadResultCallback = std::function<void(const ImageLoadResult&)>;

/**
 * @brief 资源提供者回调 - 用于从嵌入资源加载
 * @param path 资源路径
 * @param out_data 输出数据
 * @return 是否找到资源
 */
using AssetProvider = std::function<bool(const std::string& path, std::vector<uint8_t>& out_data)>;

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
    
    // ========== 基础路径 ==========
    
    /**
     * @brief 设置基础路径（用于解析相对路径）
     */
    static void SetBasePath(const std::string& path);
    
    /**
     * @brief 获取基础路径
     */
    static const std::string& GetBasePath();
    
    // ========== 资源提供者 ==========
    
    /**
     * @brief 设置资源提供者（用于嵌入资源）
     */
    static void SetAssetProvider(AssetProvider provider);
    
    /**
     * @brief 获取资源提供者
     */
    static AssetProvider GetAssetProvider();
    
    // ========== 同步加载 ==========
    
    /**
     * @brief 从文件加载图片（优先检查嵌入资源）
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
    
    /**
     * @brief 从URL加载图片（支持 http://, https://, file://, data:）
     * @param url 图片URL
     * @return SkImage 智能指针
     */
    static sk_sp<SkImage> LoadFromUrl(const std::string& url);
    
    /**
     * @brief 从URL加载图片（带详细结果）
     * @param url 图片URL
     * @return ImageLoadResult 加载结果
     */
    static ImageLoadResult LoadFromUrlWithResult(const std::string& url);
    
    /**
     * @brief 从 data: URL 加载图片
     * @param data_url data: URL 字符串
     * @return SkImage 智能指针
     */
    static sk_sp<SkImage> LoadFromDataUrl(const std::string& data_url);
    
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
    
    /**
     * @brief 异步从URL加载图片
     * @param url 图片URL
     * @param callback 加载完成回调
     */
    static void LoadFromUrlAsync(const std::string& url, ImageLoadCallback callback);
    
    /**
     * @brief 异步从URL加载图片（带详细结果）
     * @param url 图片URL
     * @param callback 加载完成回调
     */
    static void LoadFromUrlAsyncWithResult(const std::string& url, ImageLoadResultCallback callback);
    
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
    
    // ========== URL 辅助方法 ==========
    
    /**
     * @brief 检查是否是网络URL
     * @param url URL字符串
     * @return true 如果是 http:// 或 https://
     */
    static bool IsNetworkUrl(const std::string& url);
    
    /**
     * @brief 检查是否是 data: URL
     * @param url URL字符串
     * @return true 如果是 data: URL
     */
    static bool IsDataUrl(const std::string& url);

    /**
     * @brief 检查是否是宿主应用图标特殊路径 (app://res.ico)
     * @param url URL字符串
     * @return true 如果是宿主应用图标路径
     */
    static bool IsExeIconUrl(const std::string& url);

    /**
     * @brief 加载当前运行的 exe 文件的图标
     * @return SkImage 智能指针，非 Windows 平台返回 nullptr
     */
    static sk_sp<SkImage> LoadCurrentExeIcon();

private:
    static AssetProvider asset_provider_;
    static std::string base_path_;
    
    /**
     * @brief 从网络加载图片数据
     * @param url 网络URL
     * @param out_data 输出数据
     * @return 是否成功
     */
    static bool FetchFromNetwork(const std::string& url, std::vector<uint8_t>& out_data);
    
    /**
     * @brief 解码 Base64 字符串
     * @param encoded Base64 编码的字符串
     * @return 解码后的数据
     */
    static std::vector<uint8_t> DecodeBase64(const std::string& encoded);
};

} // namespace mbink

