/**
 * @file image_renderer.cpp
 * @brief 图片渲染器实现
 */

#include "image_renderer.h"

namespace lightui {

// ========== 构造函数 ==========

ImageRenderer::ImageRenderer(SkCanvas* canvas)
    : canvas_(canvas)
    , cache_enabled_(true) {
}

// ========== 图片绘制 ==========

void ImageRenderer::DrawImage(sk_sp<SkImage> image, float x, float y, const Paint* paint) {
    if (!canvas_ || !image) return;
    
    SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    
    if (paint) {
        canvas_->drawImage(image, x, y, sampling, &paint->GetSkPaint());
    } else {
        canvas_->drawImage(image, x, y, sampling);
    }
}

void ImageRenderer::DrawImage(sk_sp<SkImage> image, float x, float y, float width, float height,
                             const Paint* paint) {
    if (!canvas_ || !image) return;
    
    SkRect dst = SkRect::MakeXYWH(x, y, width, height);
    SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    
    if (paint) {
        canvas_->drawImageRect(image, dst, sampling, &paint->GetSkPaint());
    } else {
        canvas_->drawImageRect(image, dst, sampling);
    }
}

void ImageRenderer::DrawImage(sk_sp<SkImage> image, const SkRect& src, const SkRect& dst,
                             const Paint* paint) {
    if (!canvas_ || !image) return;
    
    SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    
    if (paint) {
        canvas_->drawImageRect(image, src, dst, sampling, &paint->GetSkPaint(), 
                              SkCanvas::kStrict_SrcRectConstraint);
    } else {
        canvas_->drawImageRect(image, src, dst, sampling, nullptr,
                              SkCanvas::kStrict_SrcRectConstraint);
    }
}

void ImageRenderer::DrawImageFromFile(const std::string& path, float x, float y, const Paint* paint) {
    sk_sp<SkImage> image = LoadImageWithCache(path);
    if (image) {
        DrawImage(image, x, y, paint);
    }
}

void ImageRenderer::DrawImageFromFile(const std::string& path, float x, float y, 
                                     float width, float height, const Paint* paint) {
    sk_sp<SkImage> image = LoadImageWithCache(path);
    if (image) {
        DrawImage(image, x, y, width, height, paint);
    }
}

// ========== 图片变换 ==========

void ImageRenderer::DrawRotatedImage(sk_sp<SkImage> image, float x, float y, float degrees,
                                    const Paint* paint) {
    if (!canvas_ || !image) return;
    
    canvas_->save();
    
    // 移动到旋转中心
    float cx = x + image->width() / 2.0f;
    float cy = y + image->height() / 2.0f;
    canvas_->translate(cx, cy);
    canvas_->rotate(degrees);
    canvas_->translate(-cx, -cy);
    
    // 绘制图片
    DrawImage(image, x, y, paint);
    
    canvas_->restore();
}

void ImageRenderer::DrawImageWithAlpha(sk_sp<SkImage> image, float x, float y, float alpha) {
    if (!canvas_ || !image) return;
    
    Paint paint;
    paint.SetAlpha(static_cast<int>(alpha * 255.0f));
    DrawImage(image, x, y, &paint);
}

// ========== 缓存管理 ==========

void ImageRenderer::ClearCache() {
    ImageCache::GetInstance().Clear();
}

// ========== 私有辅助方法 ==========

sk_sp<SkImage> ImageRenderer::LoadImageWithCache(const std::string& path) {
    if (cache_enabled_) {
        // 尝试从缓存获取
        sk_sp<SkImage> cached_image = ImageCache::GetInstance().Get(path);
        if (cached_image) {
            return cached_image;
        }
        
        // 从文件加载
        sk_sp<SkImage> image = ImageLoader::LoadFromFile(path);
        if (image) {
            // 添加到缓存
            ImageCache::GetInstance().Put(path, image);
        }
        return image;
    } else {
        // 直接从文件加载
        return ImageLoader::LoadFromFile(path);
    }
}

} // namespace lightui

