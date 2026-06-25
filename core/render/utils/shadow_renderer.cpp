/**
 * @file shadow_renderer.cpp
 * @brief CSS 阴影渲染器实现
 */

#include "shadow_renderer.h"
#include "core/render/text/text_renderer.h"
#include "core/render/utils/color.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkPath.h"
#include "include/core/SkClipOp.h"
#include <algorithm>
#include <cstdio>

namespace mblink {

void ShadowRenderer::RenderBoxShadow(SkCanvas* canvas,
                                    const SkRect& rect,
                                    const std::vector<CSSBoxShadow>& shadows,
                                    float border_radius) {
    if (shadows.empty()) {
        return;
    }

    // 从后往前渲染（最后的阴影在最底层）
    for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
        const auto& shadow = *it;
        
        if (shadow.inset) {
            RenderInsetShadow(canvas, rect, shadow, border_radius);
        } else {
            RenderOutsetShadow(canvas, rect, shadow, border_radius);
        }
    }
}

void ShadowRenderer::RenderOutsetShadow(SkCanvas* canvas,
                                       const SkRect& rect,
                                       const CSSBoxShadow& shadow,
                                       float border_radius) {
    // 1. 计算阴影矩形
    SkRect shadow_rect = rect;
    shadow_rect.offset(shadow.offset_x, shadow.offset_y);
    
    // 应用扩展半径
    if (shadow.spread_radius != 0) {
        shadow_rect.outset(shadow.spread_radius, shadow.spread_radius);
    }
    
    // 2. 创建画笔
    SkPaint paint;
    paint.setColor(shadow.color);
    paint.setAntiAlias(true);

    // 3. 应用模糊滤镜
    if (shadow.blur_radius > 0) {
        // CSS blur-radius 转换为 Skia sigma
        // 根据测试，使用 blur_radius 作为 sigma 值效果最接近浏览器
        // 注意：不同浏览器的实现可能略有差异
        float sigma = shadow.blur_radius;
        paint.setMaskFilter(CreateBlurFilter(sigma));
    }
    
    // 4. 绘制阴影
    if (border_radius > 0) {
        // 圆角矩形阴影
        // 注意：扩展半径也会影响圆角
        float adjusted_radius = std::max(0.0f, border_radius + shadow.spread_radius);
        canvas->drawRoundRect(shadow_rect, adjusted_radius, adjusted_radius, paint);
    } else {
        // 普通矩形阴影
        canvas->drawRect(shadow_rect, paint);
    }
}

void ShadowRenderer::RenderInsetShadow(SkCanvas* canvas,
                                      const SkRect& rect,
                                      const CSSBoxShadow& shadow,
                                      float border_radius) {
    // 内阴影的实现：
    // 1. 保存 canvas 状态
    // 2. 裁剪到元素区域
    // 3. 绘制反向阴影
    
    canvas->save();
    
    // 1. 裁剪到元素区域
    if (border_radius > 0) {
        SkPath clip_path;
        clip_path.addRoundRect(rect, border_radius, border_radius);
        canvas->clipPath(clip_path, SkClipOp::kIntersect, true);
    } else {
        canvas->clipRect(rect, SkClipOp::kIntersect, true);
    }
    
    // 2. 计算阴影矩形（反向偏移）
    SkRect shadow_rect = rect;
    shadow_rect.offset(-shadow.offset_x, -shadow.offset_y);
    
    // 应用扩展半径（内阴影是向内扩展）
    if (shadow.spread_radius != 0) {
        shadow_rect.inset(shadow.spread_radius, shadow.spread_radius);
    }
    
    // 3. 创建画笔
    SkPaint paint;
    paint.setColor(shadow.color);
    paint.setAntiAlias(true);
    
    // 4. 应用模糊滤镜
    if (shadow.blur_radius > 0) {
        // CSS blur-radius 转换为 Skia sigma
        // 根据测试，使用 blur_radius 作为 sigma 值效果最接近浏览器
        // 注意：不同浏览器的实现可能略有差异
        float sigma = shadow.blur_radius;
        paint.setMaskFilter(CreateBlurFilter(sigma));
    }
    
    // 5. 绘制阴影
    // 为了创建内阴影效果，我们需要绘制一个大的外部矩形，
    // 然后在中间挖空一个洞
    SkPath shadow_path;
    
    // 外部大矩形（足够大以覆盖模糊区域）
    SkRect outer_rect = rect;
    float blur_extent = shadow.blur_radius * 3.0f; // 模糊范围
    outer_rect.outset(blur_extent, blur_extent);
    shadow_path.addRect(outer_rect);
    
    // 内部矩形（挖空）
    if (border_radius > 0) {
        float adjusted_radius = std::max(0.0f, border_radius - shadow.spread_radius);
        shadow_path.addRoundRect(shadow_rect, adjusted_radius, adjusted_radius, 
                                SkPathDirection::kCCW);
    } else {
        shadow_path.addRect(shadow_rect, SkPathDirection::kCCW);
    }
    
    shadow_path.setFillType(SkPathFillType::kEvenOdd);
    
    // 6. 绘制路径
    canvas->drawPath(shadow_path, paint);
    
    canvas->restore();
}

void ShadowRenderer::RenderTextWithShadow(SkCanvas* canvas,
                                         const std::string& text,
                                         const SkFont& font,
                                         float x, float y,
                                         SkColor text_color,
                                         const std::vector<CSSTextShadow>& shadows,
                                         TextRenderer& text_renderer) {
    if (text.empty()) {
        return;
    }

    // 1. 先渲染阴影（从后往前）
    for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
        const auto& shadow = *it;

        // 创建阴影画笔
        Paint shadow_paint;
        shadow_paint.SetColor(shadow.color);

        // 应用模糊
        if (shadow.blur_radius > 0) {
            // CSS blur-radius 转换为 Skia sigma
            // 根据测试，使用 blur_radius 作为 sigma 值效果最接近浏览器
            // 注意：不同浏览器的实现可能略有差异
            float sigma = shadow.blur_radius;
            shadow_paint.GetSkPaint().setMaskFilter(CreateBlurFilter(sigma));
        }

        // 使用TextRenderer绘制阴影文本（支持emoji）
        // 直接在偏移后的坐标绘制，避免使用translate影响canvas状态
        text_renderer.DrawTextWithEmoji(text, x + shadow.offset_x, y + shadow.offset_y, font, shadow_paint);
    }

    // 2. 再渲染文本本身
    Paint text_paint;
    text_paint.SetColor(text_color);
    text_renderer.DrawTextWithEmoji(text, x, y, font, text_paint);
}

sk_sp<SkMaskFilter> ShadowRenderer::CreateBlurFilter(float blur_radius) {
    // 使用 Skia 的高斯模糊
    // kNormal_SkBlurStyle 是标准的模糊样式
    return SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur_radius);
}

} // namespace mblink

