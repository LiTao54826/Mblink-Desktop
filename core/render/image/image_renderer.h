/**
 * @file image_renderer.h
 * @brief 图片渲染器
 * 
 * 功能：
 * - 图片绘制
 * - 图片缩放
 * - 图片裁剪
 * - 图片旋转
 * - 图片透明度
 */

#pragma once

#include <string>
#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkRect.h"
#include "include/core/SkSamplingOptions.h"
#include "core/render/utils/paint.h"
#include "image_loader.h"
#include "image_cache.h"

namespace mblink {

/**
 * @brief 图片渲染器类
 */
class ImageRenderer {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布指针
     */
    explicit ImageRenderer(SkCanvas* canvas);
    
    /**
     * @brief 析构函数
     */
    ~ImageRenderer() = default;
    
    // ========== 图片绘制 ==========
    
    /**
     * @brief 绘制图片
     * @param image 图片
     * @param x X 坐标
     * @param y Y 坐标
     * @param paint 画笔（可选）
     */
    void DrawImage(sk_sp<SkImage> image, float x, float y, const Paint* paint = nullptr);
    
    /**
     * @brief 绘制图片（指定大小）
     * @param image 图片
     * @param x X 坐标
     * @param y Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔（可选）
     */
    void DrawImage(sk_sp<SkImage> image, float x, float y, float width, float height, 
                   const Paint* paint = nullptr);
    
    /**
     * @brief 绘制图片（指定源矩形和目标矩形）
     * @param image 图片
     * @param src 源矩形（图片中的区域）
     * @param dst 目标矩形（画布中的区域）
     * @param paint 画笔（可选）
     */
    void DrawImage(sk_sp<SkImage> image, const SkRect& src, const SkRect& dst, 
                   const Paint* paint = nullptr);
    
    /**
     * @brief 从文件绘制图片
     * @param path 图片文件路径
     * @param x X 坐标
     * @param y Y 坐标
     * @param paint 画笔（可选）
     */
    void DrawImageFromFile(const std::string& path, float x, float y, const Paint* paint = nullptr);
    
    /**
     * @brief 从文件绘制图片（指定大小）
     * @param path 图片文件路径
     * @param x X 坐标
     * @param y Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param paint 画笔（可选）
     */
    void DrawImageFromFile(const std::string& path, float x, float y, float width, float height,
                          const Paint* paint = nullptr);
    
    // ========== 图片变换 ==========
    
    /**
     * @brief 绘制旋转的图片
     * @param image 图片
     * @param x X 坐标
     * @param y Y 坐标
     * @param degrees 旋转角度（度）
     * @param paint 画笔（可选）
     */
    void DrawRotatedImage(sk_sp<SkImage> image, float x, float y, float degrees,
                         const Paint* paint = nullptr);
    
    /**
     * @brief 绘制带透明度的图片
     * @param image 图片
     * @param x X 坐标
     * @param y Y 坐标
     * @param alpha 透明度 (0.0-1.0)
     */
    void DrawImageWithAlpha(sk_sp<SkImage> image, float x, float y, float alpha);
    
    // ========== 缓存管理 ==========
    
    /**
     * @brief 启用/禁用图片缓存
     * @param enable true 启用，false 禁用
     */
    void SetCacheEnabled(bool enable) { cache_enabled_ = enable; }
    
    /**
     * @brief 获取缓存启用状态
     * @return true 表示启用
     */
    bool IsCacheEnabled() const { return cache_enabled_; }
    
    /**
     * @brief 清除图片缓存
     */
    void ClearCache();
    
    // ========== 画布访问 ==========
    
    /**
     * @brief 获取画布指针
     * @return SkCanvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }

private:
    /**
     * @brief 从缓存或文件加载图片
     */
    sk_sp<SkImage> LoadImageWithCache(const std::string& path);

private:
    SkCanvas* canvas_;          ///< Skia 画布指针
    bool cache_enabled_;        ///< 是否启用缓存
};

} // namespace mblink

