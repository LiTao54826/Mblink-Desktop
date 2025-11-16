/**
 * @file box_renderer.cpp
 * @brief CSS 盒模型渲染器实现
 */

#include "box_renderer.h"
#include "color.h"
#include "image/image_loader.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkMaskFilter.h"
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
    
    // 绘制四条边
    // 上边框
    if (box.border_top_width > 0) {
        RenderBorderEdge(
            border_box.left(), border_box.top(),
            border_box.right(), border_box.top(),
            box.border_top_width, border.style, border.color
        );
    }
    
    // 右边框
    if (box.border_right_width > 0) {
        RenderBorderEdge(
            border_box.right(), border_box.top(),
            border_box.right(), border_box.bottom(),
            box.border_right_width, border.style, border.color
        );
    }
    
    // 下边框
    if (box.border_bottom_width > 0) {
        RenderBorderEdge(
            border_box.left(), border_box.bottom(),
            border_box.right(), border_box.bottom(),
            box.border_bottom_width, border.style, border.color
        );
    }
    
    // 左边框
    if (box.border_left_width > 0) {
        RenderBorderEdge(
            border_box.left(), border_box.top(),
            border_box.left(), border_box.bottom(),
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
    SkRect padding_box = box.GetPaddingBox();

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
        float tl = border_radius->top_left.ToPx();
        float tr = border_radius->top_right.ToPx();
        float br = border_radius->bottom_right.ToPx();
        float bl = border_radius->bottom_left.ToPx();

        // Debug: Print border-radius values and rect
        static int debug_radius_count = 0;
        if (debug_radius_count < 20) {
            std::cout << "[RenderBackgroundAdvanced] Rect: ("
                      << padding_box.left() << ", " << padding_box.top() << ", "
                      << padding_box.right() << ", " << padding_box.bottom() << ") "
                      << "size=" << padding_box.width() << "x" << padding_box.height() << std::endl;
            std::cout << "[RenderBackgroundAdvanced] border-radius: "
                      << "tl=" << tl << ", tr=" << tr << ", br=" << br << ", bl=" << bl << std::endl;
            debug_radius_count++;
        }

        SkVector radii[4] = {
            {tl, tl},  // top-left
            {tr, tr},  // top-right
            {br, br},  // bottom-right
            {bl, bl}   // bottom-left
        };
        rrect.setRectRadii(padding_box, radii);
        path.addRRect(rrect);
    } else {
        path.addRect(padding_box);
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
            pts[0] = SkPoint::Make(padding_box.centerX() - cos(angle_rad) * padding_box.width() / 2,
                                  padding_box.centerY() - sin(angle_rad) * padding_box.height() / 2);
            pts[1] = SkPoint::Make(padding_box.centerX() + cos(angle_rad) * padding_box.width() / 2,
                                  padding_box.centerY() + sin(angle_rad) * padding_box.height() / 2);

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
                padding_box.left() + padding_box.width() * gradient->center_x,
                padding_box.top() + padding_box.height() * gradient->center_y
            );

            float radius = std::max(padding_box.width(), padding_box.height()) / 2.0f;

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

        // 加载图片
        auto image = ImageLoader::LoadFromFile(url);

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
                float scale = std::max(padding_box.width() / img_width,
                                      padding_box.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::CONTAIN) {
                float scale = std::min(padding_box.width() / img_width,
                                      padding_box.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::LENGTH) {
                if (!bg_size.width.IsAuto()) {
                    img_width = bg_size.width.ToPx(padding_box.width());
                }
                if (!bg_size.height.IsAuto()) {
                    img_height = bg_size.height.ToPx(padding_box.height());
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

            SkMatrix matrix = SkMatrix::Translate(padding_box.left(), padding_box.top());
            matrix.postScale(img_width / image->width(), img_height / image->height(),
                           padding_box.left(), padding_box.top());

            sk_sp<SkShader> shader = image->makeShader(tile_x, tile_y, SkSamplingOptions(), matrix);
            paint.GetSkPaint().setShader(shader);
            has_background = true;
        }
    }
    // 4. 纯色背景
    else if (bg_color_it != styles.end() && !bg_color_it->second.empty() &&
             bg_color_it->second != "transparent") {
        SkColor parsed_color = Color::Parse(bg_color_it->second);

        // Debug: Print color parsing
        static int debug_color_count = 0;
        if (debug_color_count < 20) {
            std::cout << "[RenderBackgroundAdvanced] background-color: \"" << bg_color_it->second
                      << "\" -> ARGB("
                      << SkColorGetA(parsed_color) << ", "
                      << SkColorGetR(parsed_color) << ", "
                      << SkColorGetG(parsed_color) << ", "
                      << SkColorGetB(parsed_color) << ")" << std::endl;
            debug_color_count++;
        }

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
    SkRRect rrect;
    SkVector radii[4] = {
        {border_radius.top_left.ToPx(), border_radius.top_left.ToPx()},
        {border_radius.top_right.ToPx(), border_radius.top_right.ToPx()},
        {border_radius.bottom_right.ToPx(), border_radius.bottom_right.ToPx()},
        {border_radius.bottom_left.ToPx(), border_radius.bottom_left.ToPx()}
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

    for (const auto& shadow : shadows) {
        // 创建路径
        SkPath path;
        if (border_radius) {
            SkRRect rrect;
            SkVector radii[4] = {
                {border_radius->top_left.ToPx(), border_radius->top_left.ToPx()},
                {border_radius->top_right.ToPx(), border_radius->top_right.ToPx()},
                {border_radius->bottom_right.ToPx(), border_radius->bottom_right.ToPx()},
                {border_radius->bottom_left.ToPx(), border_radius->bottom_left.ToPx()}
            };
            rrect.setRectRadii(border_box, radii);
            path.addRRect(rrect);
        } else {
            path.addRect(border_box);
        }

        Paint paint;
        paint.SetColor(shadow.color);

        if (!shadow.inset) {
            // 外阴影：使用模糊滤镜
            if (shadow.blur_radius > 0) {
                sk_sp<SkImageFilter> blur = SkImageFilters::Blur(
                    shadow.blur_radius / 2.0f, shadow.blur_radius / 2.0f, nullptr);
                paint.GetSkPaint().setImageFilter(blur);
            }

            // 应用偏移
            canvas_->save();
            canvas_->translate(shadow.offset_x, shadow.offset_y);

            // 应用扩展
            if (shadow.spread_radius != 0) {
                SkRect expanded = border_box;
                expanded.outset(shadow.spread_radius, shadow.spread_radius);
                SkPath expanded_path;
                if (border_radius) {
                    SkRRect rrect;
                    SkVector radii[4] = {
                        {border_radius->top_left.ToPx() + shadow.spread_radius,
                         border_radius->top_left.ToPx() + shadow.spread_radius},
                        {border_radius->top_right.ToPx() + shadow.spread_radius,
                         border_radius->top_right.ToPx() + shadow.spread_radius},
                        {border_radius->bottom_right.ToPx() + shadow.spread_radius,
                         border_radius->bottom_right.ToPx() + shadow.spread_radius},
                        {border_radius->bottom_left.ToPx() + shadow.spread_radius,
                         border_radius->bottom_left.ToPx() + shadow.spread_radius}
                    };
                    rrect.setRectRadii(expanded, radii);
                    expanded_path.addRRect(rrect);
                } else {
                    expanded_path.addRect(expanded);
                }
                canvas_->drawPath(expanded_path, paint.GetSkPaint());
            } else {
                canvas_->drawPath(path, paint.GetSkPaint());
            }

            canvas_->restore();
        } else {
            // 内阴影：使用裁剪和反向绘制
            // 简化实现：暂不支持内阴影
        }
    }
}

} // namespace lightui

