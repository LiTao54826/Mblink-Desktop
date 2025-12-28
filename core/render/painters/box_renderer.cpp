/**
 * @file box_renderer.cpp
 * @brief CSS 盒模型渲染器实现
 */

#include "box_renderer.h"
#include "../color.h"
#include "../image/image_loader.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkShader.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

BoxRenderer::BoxRenderer(SkCanvas* canvas)
    : canvas_(canvas)
    , shapes_(canvas) {
}

void BoxRenderer::RenderBox(const Box& box, const std::unordered_map<std::string, std::string>& styles) {
    // 1. 渲染背景颜色
    auto bg_it = styles.find("background-color");
    if (bg_it != styles.end() && !bg_it->second.empty()) {
        RenderBackground(box, bg_it->second);
    }
    
    // 2. 渲染边框
    auto border_width_it = styles.find("border-width");
    auto border_style_it = styles.find("border-style");
    auto border_color_it = styles.find("border-color");
    
    std::string border_width = (border_width_it != styles.end()) ? border_width_it->second : "";
    std::string border_style = (border_style_it != styles.end()) ? border_style_it->second : "";
    std::string border_color = (border_color_it != styles.end()) ? border_color_it->second : "";
    
    if (!border_width.empty() || !border_style.empty() || !border_color.empty()) {
        RenderBorder(box, border_width, border_style, border_color);
    }
}

void BoxRenderer::RenderBackground(const Box& box, const std::string& background_color) {
    if (background_color.empty() || background_color == "transparent") {
        return;
    }
    
    // 解析颜色
    SkColor color = CSSValue::ParseColor(background_color);
    
    // 创建画笔
    Paint paint;
    paint.SetColor(color);
    paint.SetStyle(PaintStyle::FILL);
    paint.SetAntiAlias(true);
    
    // 绘制背景（使用 padding box）
    SkRect padding_box = box.GetPaddingBox();
    shapes_.FillRect(padding_box.x(), padding_box.y(), 
                     padding_box.width(), padding_box.height(), paint);
}

void BoxRenderer::RenderBorder(const Box& box, 
                                const std::string& border_width,
                                const std::string& border_style,
                                const std::string& border_color) {
    // 解析边框属性
    CSSBorder border = CSSValue::ParseBorder(
        border_width.empty() ? "1px" : border_width,
        border_style.empty() ? "solid" : border_style,
        border_color.empty() ? "black" : border_color
    );
    
    if (border.style == CSSBorderStyle::NONE || border.width.IsZero()) {
        return;
    }
    
    // 获取边框盒子
    SkRect border_box = box.GetBorderBox();
    SkRect padding_box = box.GetPaddingBox();
    
    float width = border.width.ToPx();
    
    // 绘制四条边 - 坐标向内偏移 width/2，确保边框完全在边界内
    // Skia 的 drawLine 是居中描边，偏移后描边外边缘与边界对齐
    float half_width = width / 2.0f;
    
    // 上边框
    if (box.border_top_width > 0) {
        float top_half = box.border_top_width / 2.0f;
        RenderBorderEdge(
            border_box.left() + top_half, border_box.top() + top_half,
            border_box.right() - top_half, border_box.top() + top_half,
            box.border_top_width, border.style, border.color
        );
    }
    
    // 右边框
    if (box.border_right_width > 0) {
        float right_half = box.border_right_width / 2.0f;
        RenderBorderEdge(
            border_box.right() - right_half, border_box.top() + right_half,
            border_box.right() - right_half, border_box.bottom() - right_half,
            box.border_right_width, border.style, border.color
        );
    }
    
    // 下边框
    if (box.border_bottom_width > 0) {
        float bottom_half = box.border_bottom_width / 2.0f;
        RenderBorderEdge(
            border_box.left() + bottom_half, border_box.bottom() - bottom_half,
            border_box.right() - bottom_half, border_box.bottom() - bottom_half,
            box.border_bottom_width, border.style, border.color
        );
    }
    
    // 左边框
    if (box.border_left_width > 0) {
        float left_half = box.border_left_width / 2.0f;
        RenderBorderEdge(
            border_box.left() + left_half, border_box.top() + left_half,
            border_box.left() + left_half, border_box.bottom() - left_half,
            box.border_left_width, border.style, border.color
        );
    }
}

void BoxRenderer::RenderBorderEdge(float x1, float y1, float x2, float y2,
                                    float width, CSSBorderStyle style, SkColor color) {
    Paint paint;
    paint.SetColor(color);
    paint.SetStrokeWidth(width);
    paint.SetAntiAlias(true);
    
    // 根据样式设置路径效果
    switch (style) {
        case CSSBorderStyle::SOLID:
            // 实线，无需特殊处理
            break;
            
        case CSSBorderStyle::DASHED: {
            // 虚线
            float intervals[] = {width * 3, width * 3};
            paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
            break;
        }
            
        case CSSBorderStyle::DOTTED: {
            // 点线
            float intervals[] = {width, width};
            paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
            paint.SetStrokeCap(StrokeCap::ROUND);
            break;
        }
            
        case CSSBorderStyle::DOUBLE: {
            // 双线（绘制两条线）
            float third = width / 3.0f;
            Paint thin_paint = paint;
            thin_paint.SetStrokeWidth(third);
            
            // 计算偏移
            float dx = (x2 - x1 == 0) ? third : 0;
            float dy = (y2 - y1 == 0) ? third : 0;
            
            shapes_.DrawLine(x1 - dx, y1 - dy, x2 - dx, y2 - dy, thin_paint);
            shapes_.DrawLine(x1 + dx, y1 + dy, x2 + dx, y2 + dy, thin_paint);
            return;
        }
            
        case CSSBorderStyle::NONE:
        default:
            return;
    }
    
    shapes_.DrawLine(x1, y1, x2, y2, paint);
}

Box BoxRenderer::ComputeBox(const std::unordered_map<std::string, std::string>& styles,
                             float x, float y, float width, float height,
                             float parent_width, float font_size) {
    Box box;
    
    // 设置内容区域
    box.content_x = x;
    box.content_y = y;
    box.content_width = width;
    box.content_height = height;
    
    // 解析 padding
    auto padding_it = styles.find("padding");
    if (padding_it != styles.end()) {
        CSSEdges padding = CSSValue::ParseEdges(padding_it->second);
        box.padding_top = padding.top.ToPx(parent_width, font_size);
        box.padding_right = padding.right.ToPx(parent_width, font_size);
        box.padding_bottom = padding.bottom.ToPx(parent_width, font_size);
        box.padding_left = padding.left.ToPx(parent_width, font_size);
    }
    
    // 解析单独的 padding 属性
    auto padding_top_it = styles.find("padding-top");
    if (padding_top_it != styles.end()) {
        box.padding_top = CSSValue::ParseLength(padding_top_it->second).ToPx(parent_width, font_size);
    }
    
    auto padding_right_it = styles.find("padding-right");
    if (padding_right_it != styles.end()) {
        box.padding_right = CSSValue::ParseLength(padding_right_it->second).ToPx(parent_width, font_size);
    }
    
    auto padding_bottom_it = styles.find("padding-bottom");
    if (padding_bottom_it != styles.end()) {
        box.padding_bottom = CSSValue::ParseLength(padding_bottom_it->second).ToPx(parent_width, font_size);
    }
    
    auto padding_left_it = styles.find("padding-left");
    if (padding_left_it != styles.end()) {
        box.padding_left = CSSValue::ParseLength(padding_left_it->second).ToPx(parent_width, font_size);
    }
    
    // 解析 border-width
    auto border_width_it = styles.find("border-width");
    if (border_width_it != styles.end()) {
        CSSEdges border = CSSValue::ParseEdges(border_width_it->second);
        box.border_top_width = border.top.ToPx(parent_width, font_size);
        box.border_right_width = border.right.ToPx(parent_width, font_size);
        box.border_bottom_width = border.bottom.ToPx(parent_width, font_size);
        box.border_left_width = border.left.ToPx(parent_width, font_size);
    }
    
    // 解析单独的 border-width 属性
    auto border_top_width_it = styles.find("border-top-width");
    if (border_top_width_it != styles.end()) {
        box.border_top_width = CSSValue::ParseLength(border_top_width_it->second).ToPx(parent_width, font_size);
    }
    
    auto border_right_width_it = styles.find("border-right-width");
    if (border_right_width_it != styles.end()) {
        box.border_right_width = CSSValue::ParseLength(border_right_width_it->second).ToPx(parent_width, font_size);
    }
    
    auto border_bottom_width_it = styles.find("border-bottom-width");
    if (border_bottom_width_it != styles.end()) {
        box.border_bottom_width = CSSValue::ParseLength(border_bottom_width_it->second).ToPx(parent_width, font_size);
    }
    
    auto border_left_width_it = styles.find("border-left-width");
    if (border_left_width_it != styles.end()) {
        box.border_left_width = CSSValue::ParseLength(border_left_width_it->second).ToPx(parent_width, font_size);
    }
    
    // 解析 margin
    auto margin_it = styles.find("margin");
    if (margin_it != styles.end()) {
        CSSEdges margin = CSSValue::ParseEdges(margin_it->second);
        box.margin_top = margin.top.ToPx(parent_width, font_size);
        box.margin_right = margin.right.ToPx(parent_width, font_size);
        box.margin_bottom = margin.bottom.ToPx(parent_width, font_size);
        box.margin_left = margin.left.ToPx(parent_width, font_size);
    }
    
    // 解析单独的 margin 属性
    auto margin_top_it = styles.find("margin-top");
    if (margin_top_it != styles.end()) {
        box.margin_top = CSSValue::ParseLength(margin_top_it->second).ToPx(parent_width, font_size);
    }
    
    auto margin_right_it = styles.find("margin-right");
    if (margin_right_it != styles.end()) {
        box.margin_right = CSSValue::ParseLength(margin_right_it->second).ToPx(parent_width, font_size);
    }
    
    auto margin_bottom_it = styles.find("margin-bottom");
    if (margin_bottom_it != styles.end()) {
        box.margin_bottom = CSSValue::ParseLength(margin_bottom_it->second).ToPx(parent_width, font_size);
    }
    
    auto margin_left_it = styles.find("margin-left");
    if (margin_left_it != styles.end()) {
        box.margin_left = CSSValue::ParseLength(margin_left_it->second).ToPx(parent_width, font_size);
    }
    
    return box;
}

// ========== 高级样式渲染 ==========

void BoxRenderer::RenderBackgroundAdvanced(const Box& box,
                                          const std::unordered_map<std::string, std::string>& styles,
                                          const CSSBorderRadius* border_radius) {
    // 使用 border_box 绘制背景（符合 CSS 规范 background-clip: border-box 默认值）
    // 背景会延伸到边框外边缘，被边框覆盖
    SkRect border_box_rect = box.GetBorderBox();

    // 创建路径（支持圆角）
    SkPath path;
    bool has_radius = border_radius && (
        border_radius->top_left.value > 0 ||
        border_radius->top_right.value > 0 ||
        border_radius->bottom_right.value > 0 ||
        border_radius->bottom_left.value > 0
    );

    if (has_radius) {
        SkRRect rrect;
        // 修复：border-radius 百分比值应该相对于元素尺寸计算
        // 水平半径相对于宽度，垂直半径相对于高度
        float width = border_box_rect.width();
        float height = border_box_rect.height();
        
        float tl = border_radius->top_left.ToPx(std::min(width, height));
        float tr = border_radius->top_right.ToPx(std::min(width, height));
        float br = border_radius->bottom_right.ToPx(std::min(width, height));
        float bl = border_radius->bottom_left.ToPx(std::min(width, height));

        SkVector radii[4] = {
            {tl, tl},  // top-left
            {tr, tr},  // top-right
            {br, br},  // bottom-right
            {bl, bl}   // bottom-left
        };
        rrect.setRectRadii(border_box_rect, radii);
        path.addRRect(rrect);
    } else {
        path.addRect(border_box_rect);
    }

    Paint paint;
    bool has_background = false;  // 标记是否有有效的背景

    // 检查背景类型
    auto bg_it = styles.find("background");
    auto bg_color_it = styles.find("background-color");
    auto bg_image_it = styles.find("background-image");

    // 1. 线性渐变
    if (bg_it != styles.end() && bg_it->second.find("linear-gradient") != std::string::npos) {
        auto gradient = CSSValue::ParseLinearGradient(bg_it->second);
        if (gradient.has_value()) {
            std::vector<SkColor> colors;
            std::vector<SkScalar> positions;

            for (const auto& stop : gradient->stops) {
                colors.push_back(stop.color);
                positions.push_back(stop.position);
            }

            // 计算渐变方向
            float angle_rad = gradient->angle * M_PI / 180.0f;
            SkPoint pts[2];
            pts[0] = SkPoint::Make(border_box_rect.centerX() - cos(angle_rad) * border_box_rect.width() / 2,
                                  border_box_rect.centerY() - sin(angle_rad) * border_box_rect.height() / 2);
            pts[1] = SkPoint::Make(border_box_rect.centerX() + cos(angle_rad) * border_box_rect.width() / 2,
                                  border_box_rect.centerY() + sin(angle_rad) * border_box_rect.height() / 2);

            sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
                pts, colors.data(), positions.data(), colors.size(), SkTileMode::kClamp);
            paint.GetSkPaint().setShader(shader);
            has_background = true;
        }
    }
    // 2. 径向渐变
    else if (bg_it != styles.end() && bg_it->second.find("radial-gradient") != std::string::npos) {
        auto gradient = CSSValue::ParseRadialGradient(bg_it->second);
        if (gradient.has_value()) {
            std::vector<SkColor> colors;
            std::vector<SkScalar> positions;

            for (const auto& stop : gradient->stops) {
                colors.push_back(stop.color);
                positions.push_back(stop.position);
            }

            SkPoint center = SkPoint::Make(
                border_box_rect.left() + border_box_rect.width() * gradient->center_x,
                border_box_rect.top() + border_box_rect.height() * gradient->center_y
            );

            float radius = std::max(border_box_rect.width(), border_box_rect.height()) / 2.0f;

            sk_sp<SkShader> shader = SkGradientShader::MakeRadial(
                center, radius, colors.data(), positions.data(), colors.size(), SkTileMode::kClamp);
            paint.GetSkPaint().setShader(shader);
            has_background = true;
        }
    }
    // 3. 背景图片
    else if (bg_image_it != styles.end() && bg_image_it->second != "none") {
        std::string url = bg_image_it->second;
        // 移除 url() 包装
        if (url.find("url(") == 0) {
            url = url.substr(4, url.length() - 5);
            // 移除引号
            if (!url.empty() && (url.front() == '"' || url.front() == '\'')) {
                url = url.substr(1, url.length() - 2);
            }
        }

        // 加载图片（支持网络URL）
        auto image = ImageLoader::LoadFromUrl(url);

        if (image) {
            // 解析 background-repeat
            CSSBackgroundRepeat repeat = CSSBackgroundRepeat::REPEAT;
            auto repeat_it = styles.find("background-repeat");
            if (repeat_it != styles.end()) {
                repeat = CSSValue::ParseBackgroundRepeat(repeat_it->second);
            }

            // 解析 background-size
            CSSBackgroundSize bg_size;
            auto size_it = styles.find("background-size");
            if (size_it != styles.end()) {
                bg_size = CSSValue::ParseBackgroundSize(size_it->second);
            }

            // 计算图片尺寸
            float img_width = image->width();
            float img_height = image->height();

            if (bg_size.type == CSSBackgroundSize::Type::COVER) {
                float scale = std::max(border_box_rect.width() / img_width,
                                      border_box_rect.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::CONTAIN) {
                float scale = std::min(border_box_rect.width() / img_width,
                                      border_box_rect.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::LENGTH) {
                if (!bg_size.width.IsAuto()) {
                    img_width = bg_size.width.ToPx(border_box_rect.width());
                }
                if (!bg_size.height.IsAuto()) {
                    img_height = bg_size.height.ToPx(border_box_rect.height());
                }
            }

            // 设置平铺模式
            SkTileMode tile_x = SkTileMode::kRepeat;
            SkTileMode tile_y = SkTileMode::kRepeat;

            if (repeat == CSSBackgroundRepeat::NO_REPEAT) {
                tile_x = tile_y = SkTileMode::kDecal;
            } else if (repeat == CSSBackgroundRepeat::REPEAT_X) {
                tile_y = SkTileMode::kDecal;
            } else if (repeat == CSSBackgroundRepeat::REPEAT_Y) {
                tile_x = SkTileMode::kDecal;
            }

            SkMatrix matrix = SkMatrix::Translate(border_box_rect.left(), border_box_rect.top());
            matrix.postScale(img_width / image->width(), img_height / image->height(),
                           border_box_rect.left(), border_box_rect.top());

            sk_sp<SkShader> shader = image->makeShader(tile_x, tile_y, SkSamplingOptions(), matrix);
            paint.GetSkPaint().setShader(shader);
            has_background = true;
        }
    }
    // 4. 纯色背景
    else if (bg_color_it != styles.end() && !bg_color_it->second.empty() &&
             bg_color_it->second != "transparent") {
        SkColor parsed_color = Color::Parse(bg_color_it->second);

        paint.SetColor(parsed_color);
        has_background = true;
    }

    // 只有在有有效背景时才绘制
    if (has_background) {
        canvas_->drawPath(path, paint.GetSkPaint());
    }
}

void BoxRenderer::RenderRoundedBorder(const Box& box,
                                     const std::string& border_width,
                                     const std::string& border_style,
                                     const std::string& border_color,
                                     const CSSBorderRadius& border_radius) {
    CSSBorder border = CSSValue::ParseBorder(border_width, border_style, border_color);

    if (border.style == CSSBorderStyle::NONE || border.width.IsZero()) {
        return;
    }

    SkRect border_box = box.GetBorderBox();
    float width = border.width.ToPx();

    // 创建圆角矩形路径
    // 修复：border-radius 百分比值应该相对于元素尺寸计算
    float box_width = border_box.width();
    float box_height = border_box.height();
    float base_size = std::min(box_width, box_height);
    
    SkRRect rrect;
    SkVector radii[4] = {
        {border_radius.top_left.ToPx(base_size), border_radius.top_left.ToPx(base_size)},
        {border_radius.top_right.ToPx(base_size), border_radius.top_right.ToPx(base_size)},
        {border_radius.bottom_right.ToPx(base_size), border_radius.bottom_right.ToPx(base_size)},
        {border_radius.bottom_left.ToPx(base_size), border_radius.bottom_left.ToPx(base_size)}
    };
    rrect.setRectRadii(border_box, radii);

    Paint paint;
    paint.SetColor(border.color);
    paint.SetStyle(PaintStyle::STROKE);
    paint.SetStrokeWidth(width);

    // 设置边框样式
    if (border.style == CSSBorderStyle::DASHED) {
        float intervals[] = {width * 3, width * 3};
        paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    } else if (border.style == CSSBorderStyle::DOTTED) {
        float intervals[] = {width, width};
        paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    }

    SkPath path;
    path.addRRect(rrect);
    canvas_->drawPath(path, paint.GetSkPaint());
}

void BoxRenderer::RenderBoxShadow(const Box& box,
                                 const std::vector<CSSBoxShadow>& shadows,
                                 const CSSBorderRadius* border_radius) {
    if (shadows.empty()) {
        return;
    }

    SkRect border_box = box.GetBorderBox();
    
    // 修复：计算 border-radius 百分比的基准尺寸
    float box_width = border_box.width();
    float box_height = border_box.height();
    float base_size = std::min(box_width, box_height);

    for (const auto& shadow : shadows) {
        // 创建路径
        SkPath path;
        if (border_radius) {
            SkRRect rrect;
            SkVector radii[4] = {
                {border_radius->top_left.ToPx(base_size), border_radius->top_left.ToPx(base_size)},
                {border_radius->top_right.ToPx(base_size), border_radius->top_right.ToPx(base_size)},
                {border_radius->bottom_right.ToPx(base_size), border_radius->bottom_right.ToPx(base_size)},
                {border_radius->bottom_left.ToPx(base_size), border_radius->bottom_left.ToPx(base_size)}
            };
            rrect.setRectRadii(border_box, radii);
            path.addRRect(rrect);
        } else {
            path.addRect(border_box);
        }

        Paint paint;
        paint.SetColor(shadow.color);

        if (!shadow.inset) {
            // 外阴影：使用 MaskFilter（比 ImageFilter 快 10-50 倍）
            // respectCTM=false 避免随变换缩放模糊，进一步提升性能
            if (shadow.blur_radius > 0) {
                float sigma = shadow.blur_radius / 2.0f;
                paint.GetSkPaint().setMaskFilter(
                    SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, sigma, false));
            }

            // 计算阴影矩形（应用偏移和扩展）
            SkRect shadow_rect = border_box;
            shadow_rect.offset(shadow.offset_x, shadow.offset_y);
            if (shadow.spread_radius != 0) {
                shadow_rect.outset(shadow.spread_radius, shadow.spread_radius);
            }

            // 使用 drawRRect 替代 drawPath（对于圆角矩形更快）
            if (border_radius) {
                float spread = shadow.spread_radius;
                SkRRect rrect;
                SkVector radii[4] = {
                    {border_radius->top_left.ToPx(base_size) + spread,
                     border_radius->top_left.ToPx(base_size) + spread},
                    {border_radius->top_right.ToPx(base_size) + spread,
                     border_radius->top_right.ToPx(base_size) + spread},
                    {border_radius->bottom_right.ToPx(base_size) + spread,
                     border_radius->bottom_right.ToPx(base_size) + spread},
                    {border_radius->bottom_left.ToPx(base_size) + spread,
                     border_radius->bottom_left.ToPx(base_size) + spread}
                };
                rrect.setRectRadii(shadow_rect, radii);
                canvas_->drawRRect(rrect, paint.GetSkPaint());
            } else {
                canvas_->drawRect(shadow_rect, paint.GetSkPaint());
            }
        } else {
            // 内阴影：使用裁剪和反向绘制
            // 简化实现：暂不支持内阴影
        }
    }
}

void BoxRenderer::RenderRoundedBorderAdvanced(const Box& box,
                                               const float border_widths[4],
                                               const CSSBorderStyle border_styles[4],
                                               const SkColor border_colors[4],
                                               const CSSBorderRadius& border_radius) {
    // 索引定义：0=top, 1=right, 2=bottom, 3=left
    
    bool has_top = border_widths[0] > 0 && border_styles[0] != CSSBorderStyle::NONE;
    bool has_right = border_widths[1] > 0 && border_styles[1] != CSSBorderStyle::NONE;
    bool has_bottom = border_widths[2] > 0 && border_styles[2] != CSSBorderStyle::NONE;
    bool has_left = border_widths[3] > 0 && border_styles[3] != CSSBorderStyle::NONE;
    
    if (!has_top && !has_right && !has_bottom && !has_left) {
        return;
    }
    
    // 检查四边属性是否完全相同
    bool all_same = has_top && has_right && has_bottom && has_left &&
                    border_widths[0] == border_widths[1] &&
                    border_widths[1] == border_widths[2] &&
                    border_widths[2] == border_widths[3] &&
                    border_styles[0] == border_styles[1] &&
                    border_styles[1] == border_styles[2] &&
                    border_styles[2] == border_styles[3] &&
                    border_colors[0] == border_colors[1] &&
                    border_colors[1] == border_colors[2] &&
                    border_colors[2] == border_colors[3];
    
    SkRect border_box = box.GetBorderBox();
    
    // 修复：计算 border-radius 百分比的基准尺寸
    float box_width = border_box.width();
    float box_height = border_box.height();
    float base_size = std::min(box_width, box_height);
    
    // 准备圆角半径
    SkVector outer_radii[4] = {
        {border_radius.top_left.ToPx(base_size), border_radius.top_left.ToPx(base_size)},     // TL
        {border_radius.top_right.ToPx(base_size), border_radius.top_right.ToPx(base_size)},   // TR
        {border_radius.bottom_right.ToPx(base_size), border_radius.bottom_right.ToPx(base_size)}, // BR
        {border_radius.bottom_left.ToPx(base_size), border_radius.bottom_left.ToPx(base_size)}    // BL
    };

    if (all_same) {
        // 四边属性相同：使用 SkRRect 绘制完整圆角矩形
        float width = border_widths[0];
        float half_width = width / 2.0f;
        
        // 向内收缩半个边框宽度，使描边外边缘与元素边界对齐
        SkRect inset_box = border_box.makeInset(half_width, half_width);
        
        // 圆角半径也需要相应减少
        SkVector inset_radii[4] = {
            {std::max(0.0f, outer_radii[0].fX - half_width), std::max(0.0f, outer_radii[0].fY - half_width)},
            {std::max(0.0f, outer_radii[1].fX - half_width), std::max(0.0f, outer_radii[1].fY - half_width)},
            {std::max(0.0f, outer_radii[2].fX - half_width), std::max(0.0f, outer_radii[2].fY - half_width)},
            {std::max(0.0f, outer_radii[3].fX - half_width), std::max(0.0f, outer_radii[3].fY - half_width)}
        };
        
        SkRRect rrect;
        rrect.setRectRadii(inset_box, inset_radii);
        
        Paint paint;
        paint.SetColor(border_colors[0]);
        paint.SetStyle(PaintStyle::STROKE);
        paint.SetStrokeWidth(width);
        paint.SetAntiAlias(true);
        
        if (border_styles[0] == CSSBorderStyle::DASHED) {
            float intervals[] = {width * 3, width * 3};
            paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        } else if (border_styles[0] == CSSBorderStyle::DOTTED) {
            float intervals[] = {width, width};
            paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        }
        
        SkPath path;
        path.addRRect(rrect);
        canvas_->drawPath(path, paint.GetSkPaint());
    } else {
        // 四边属性不同：分段绘制（支持圆角）
        
        // 1. 计算分割角 (Split Angles)
        // Skia 角度: 0=R, 90=D, 180=L, 270=U
        auto to_deg = [](float rad) { return rad * 180.0f / M_PI; };
        
        // TL Corner (180 -> 270)
        float angle_tl; 
        if (border_widths[3] == 0 && border_widths[0] == 0) angle_tl = 225;
        else if (border_widths[3] == 0) angle_tl = 180; // Top takes all
        else if (border_widths[0] == 0) angle_tl = 270; // Left takes all
        else angle_tl = 180 + to_deg(atan2(border_widths[0], border_widths[3]));

        // TR Corner (270 -> 360/0)
        float angle_tr;
        if (border_widths[0] == 0 && border_widths[1] == 0) angle_tr = 315;
        else if (border_widths[0] == 0) angle_tr = 270; // Right takes all
        else if (border_widths[1] == 0) angle_tr = 360; // Top takes all
        else angle_tr = 270 + to_deg(atan2(border_widths[1], border_widths[0]));

        // BR Corner (0 -> 90)
        float angle_br;
        if (border_widths[1] == 0 && border_widths[2] == 0) angle_br = 45;
        else if (border_widths[1] == 0) angle_br = 0;   // Bottom takes all
        else if (border_widths[2] == 0) angle_br = 90;  // Right takes all
        else angle_br = to_deg(atan2(border_widths[2], border_widths[1]));

        // BL Corner (90 -> 180)
        float angle_bl;
        if (border_widths[2] == 0 && border_widths[3] == 0) angle_bl = 135;
        else if (border_widths[2] == 0) angle_bl = 90;  // Left takes all
        else if (border_widths[3] == 0) angle_bl = 180; // Bottom takes all
        else angle_bl = 90 + to_deg(atan2(border_widths[3], border_widths[2]));

        // 2. 准备外圆和内圆的 Rect (Ovals for arcTo)
        // Outer Ovals
        SkRect outer_rects[4]; // TL, TR, BR, BL
        outer_rects[0] = SkRect::MakeXYWH(border_box.left(), border_box.top(), outer_radii[0].fX * 2, outer_radii[0].fY * 2);
        outer_rects[1] = SkRect::MakeXYWH(border_box.right() - outer_radii[1].fX * 2, border_box.top(), outer_radii[1].fX * 2, outer_radii[1].fY * 2);
        outer_rects[2] = SkRect::MakeXYWH(border_box.right() - outer_radii[2].fX * 2, border_box.bottom() - outer_radii[2].fY * 2, outer_radii[2].fX * 2, outer_radii[2].fY * 2);
        outer_rects[3] = SkRect::MakeXYWH(border_box.left(), border_box.bottom() - outer_radii[3].fY * 2, outer_radii[3].fX * 2, outer_radii[3].fY * 2);

        // Inner Radii & Rects
        SkVector inner_radii[4];
        SkRect inner_rects[4];
        
        // Inner TL
        inner_radii[0].fX = std::max(0.0f, outer_radii[0].fX - border_widths[3]); // - Left
        inner_radii[0].fY = std::max(0.0f, outer_radii[0].fY - border_widths[0]); // - Top
        inner_rects[0] = SkRect::MakeXYWH(border_box.left() + border_widths[3], border_box.top() + border_widths[0], inner_radii[0].fX * 2, inner_radii[0].fY * 2);
        
        // Inner TR
        inner_radii[1].fX = std::max(0.0f, outer_radii[1].fX - border_widths[1]); // - Right
        inner_radii[1].fY = std::max(0.0f, outer_radii[1].fY - border_widths[0]); // - Top
        inner_rects[1] = SkRect::MakeXYWH(border_box.right() - border_widths[1] - inner_radii[1].fX * 2, border_box.top() + border_widths[0], inner_radii[1].fX * 2, inner_radii[1].fY * 2);
        
        // Inner BR
        inner_radii[2].fX = std::max(0.0f, outer_radii[2].fX - border_widths[1]); // - Right
        inner_radii[2].fY = std::max(0.0f, outer_radii[2].fY - border_widths[2]); // - Bottom
        inner_rects[2] = SkRect::MakeXYWH(border_box.right() - border_widths[1] - inner_radii[2].fX * 2, border_box.bottom() - border_widths[2] - inner_radii[2].fY * 2, inner_radii[2].fX * 2, inner_radii[2].fY * 2);
        
        // Inner BL
        inner_radii[3].fX = std::max(0.0f, outer_radii[3].fX - border_widths[3]); // - Left
        inner_radii[3].fY = std::max(0.0f, outer_radii[3].fY - border_widths[2]); // - Bottom
        inner_rects[3] = SkRect::MakeXYWH(border_box.left() + border_widths[3], border_box.bottom() - border_widths[2] - inner_radii[3].fY * 2, inner_radii[3].fX * 2, inner_radii[3].fY * 2);

        // 辅助函数：计算角度对应的点坐标（用于零半径角的直线连接）
        auto point_on_rect = [&](const SkRect& rect, float angle) -> SkPoint {
            float rad = angle * M_PI / 180.0f;
            float cx = rect.centerX();
            float cy = rect.centerY();
            float rx = rect.width() / 2.0f;
            float ry = rect.height() / 2.0f;
            return SkPoint::Make(cx + rx * cos(rad), cy + ry * sin(rad));
        };

        // 辅助函数：检查半径是否为零
        auto is_zero_radius = [](const SkVector& radii) -> bool {
            return radii.fX <= 0.001f && radii.fY <= 0.001f;
        };

        // 辅助函数：安全 arcTo - 如果半径为零则使用 lineTo
        auto safe_arc_to = [&](SkPath& path, const SkRect& oval, float start, float sweep, bool force_move, const SkVector& radii) {
            if (is_zero_radius(radii)) {
                // 零半径：直接使用 lineTo 到矩形角点
                SkPoint corner = SkPoint::Make(oval.centerX(), oval.centerY());
                if (force_move) {
                    path.moveTo(corner);
                } else {
                    path.lineTo(corner);
                }
            } else {
                path.arcTo(oval, start, sweep, force_move);
            }
        };

        auto draw_side = [&](int side, float start_angle, float end_angle, 
                             int c1_idx, float c1_start, float c1_sweep,
                             int c2_idx, float c2_start, float c2_sweep) {
            CSSBorderStyle style = border_styles[side];
            float width = border_widths[side];
            
            // 对于 dashed/dotted 样式，使用 STROKE 模式沿中线绘制
            bool use_stroke = (style == CSSBorderStyle::DASHED || style == CSSBorderStyle::DOTTED);
            
            if (use_stroke) {
                // STROKE 模式：沿边框中线绘制，支持虚线效果
                SkPath stroke_path;
                float half_width = width / 2.0f;
                
                // 计算中线矩形（介于外边和内边之间）
                SkRect mid_rects[4];
                SkVector mid_radii[4];
                for (int i = 0; i < 4; i++) {
                    float inset_x = (i == 1 || i == 2) ? border_widths[1] / 2.0f : border_widths[3] / 2.0f;
                    float inset_y = (i == 0 || i == 1) ? border_widths[0] / 2.0f : border_widths[2] / 2.0f;
                    mid_radii[i].fX = std::max(0.0f, outer_radii[i].fX - inset_x);
                    mid_radii[i].fY = std::max(0.0f, outer_radii[i].fY - inset_y);
                    
                    float left = outer_rects[i].left() + (i == 0 || i == 3 ? inset_x : 0);
                    float top = outer_rects[i].top() + (i == 0 || i == 1 ? inset_y : 0);
                    float right = outer_rects[i].right() - (i == 1 || i == 2 ? inset_x : 0);
                    float bottom = outer_rects[i].bottom() - (i == 2 || i == 3 ? inset_y : 0);
                    mid_rects[i] = SkRect::MakeLTRB(left, top, right, bottom);
                }
                
                // 沿中线绘制弧和直线
                safe_arc_to(stroke_path, mid_rects[c1_idx], c1_start, c1_sweep, true, mid_radii[c1_idx]);
                safe_arc_to(stroke_path, mid_rects[c2_idx], c2_start, c2_sweep, false, mid_radii[c2_idx]);
                
                Paint paint;
                paint.SetColor(border_colors[side]);
                paint.SetStyle(PaintStyle::STROKE);
                paint.SetStrokeWidth(width);
                paint.SetAntiAlias(true);
                
                if (style == CSSBorderStyle::DASHED) {
                    float intervals[] = {width * 3, width * 3};
                    paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
                } else if (style == CSSBorderStyle::DOTTED) {
                    float intervals[] = {width, width * 2};
                    paint.GetSkPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
                    paint.SetStrokeCap(StrokeCap::ROUND);
                }
                
                canvas_->drawPath(stroke_path, paint.GetSkPaint());
            } else {
                // FILL 模式：原有逻辑，绘制填充形状
                SkPath path;
                
                // Outer C1 - 使用安全版本处理零半径
                safe_arc_to(path, outer_rects[c1_idx], c1_start, c1_sweep, true, outer_radii[c1_idx]);
                
                // Outer C2 (auto connects line)
                safe_arc_to(path, outer_rects[c2_idx], c2_start, c2_sweep, false, outer_radii[c2_idx]);
                
                // Inner C2 (Reverse)
                safe_arc_to(path, inner_rects[c2_idx], c2_start + c2_sweep, -c2_sweep, false, inner_radii[c2_idx]);
                
                // Inner C1 (Reverse)
                safe_arc_to(path, inner_rects[c1_idx], c1_start + c1_sweep, -c1_sweep, false, inner_radii[c1_idx]);
                
                path.close();
                
                Paint paint;
                paint.SetColor(border_colors[side]);
                paint.SetStyle(PaintStyle::FILL);
                paint.SetAntiAlias(true);
                canvas_->drawPath(path, paint.GetSkPaint());
            }
        };
        
        // Draw Top (0) - Connects TL(0) and TR(1)
        if (has_top) {
            // TL: angle_tl -> 270. TR: 270 -> angle_tr.
            draw_side(0, 0, 0, 
                      0, angle_tl, 270 - angle_tl,
                      1, 270, angle_tr - 270);
        }
        
        // Draw Right (1) - Connects TR(1) and BR(2)
        if (has_right) {
            // TR: angle_tr -> 360. BR: 0 -> angle_br.
            draw_side(1, 0, 0,
                      1, angle_tr, 360 - angle_tr,
                      2, 0, angle_br);
        }
        
        // Draw Bottom (2) - Connects BR(2) and BL(3)
        if (has_bottom) {
            // BR: angle_br -> 90. BL: 90 -> angle_bl.
            draw_side(2, 0, 0,
                      2, angle_br, 90 - angle_br,
                      3, 90, angle_bl - 90);
        }
        
        // Draw Left (3) - Connects BL(3) and TL(0)
        if (has_left) {
            // BL: angle_bl -> 180. TL: 180 -> angle_tl.
            draw_side(3, 0, 0,
                      3, angle_bl, 180 - angle_bl,
                      0, 180, angle_tl - 180);
        }
    }
}

} // namespace lightui

