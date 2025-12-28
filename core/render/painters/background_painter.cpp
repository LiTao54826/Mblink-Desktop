/**
 * @file background_painter.cpp
 * @brief 背景绘制器实现
 * 
 * 从 render_object.cpp 和 box_renderer.cpp 提取的背景绘制逻辑
 */

#include "background_painter.h"
#include "core/render/render_object.h"
#include "core/render/color.h"
#include "core/render/gradient_renderer.h"
#include "core/render/image/image_loader.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/core/SkShader.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkGradientShader.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

BackgroundPainter::BackgroundPainter(SkCanvas* canvas)
    : canvas_(canvas) {
}

void BackgroundPainter::Paint(const Box& box, 
                              const ComputedStyle& style, 
                              const PaintCache& cache) {
    if (!canvas_) {
        return;
    }

    // 使用 border_box 绘制背景（符合 CSS 规范 background-clip: border-box 默认值）
    SkRect border_box = box.GetBorderBox();

    // 优先级：线性渐变 > 径向渐变 > 纯色背景
    if (style.background_linear_gradient.has_value()) {
        // 使用 padding_box 绘制渐变（与原有逻辑一致）
        SkRect padding_box = box.GetPaddingBox();
        GradientRenderer::RenderLinearGradient(canvas_, padding_box, 
                                               *style.background_linear_gradient);
    } else if (style.background_radial_gradient.has_value()) {
        SkRect padding_box = box.GetPaddingBox();
        GradientRenderer::RenderRadialGradient(canvas_, padding_box, 
                                               *style.background_radial_gradient);
    } else {
        // 使用样式映射绘制背景（支持图片和纯色）
        std::unordered_map<std::string, std::string> styles;
        if (!style.background_color.empty()) {
            styles["background-color"] = style.background_color;
        }
        if (!style.background_image.empty()) {
            styles["background-image"] = style.background_image;
        }
        PaintWithStyles(box, styles, &style.border_radius);
    }
}

void BackgroundPainter::PaintSolidColor(const SkRect& rect, 
                                        SkColor color,
                                        const CSSBorderRadius* border_radius) {
    if (!canvas_ || color == SK_ColorTRANSPARENT) {
        return;
    }

    SkPaint paint;
    paint.setColor(color);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);

    if (border_radius) {
        SkVector radii[4];
        CalculateRadii(*border_radius, rect.width(), rect.height(), radii);
        
        bool has_radius = radii[0].fX > 0 || radii[1].fX > 0 || 
                          radii[2].fX > 0 || radii[3].fX > 0;
        
        if (has_radius) {
            SkRRect rrect;
            rrect.setRectRadii(rect, radii);
            canvas_->drawRRect(rrect, paint);
            return;
        }
    }

    canvas_->drawRect(rect, paint);
}

void BackgroundPainter::PaintLinearGradient(const SkRect& rect,
                                            const CSSLinearGradient& gradient,
                                            const CSSBorderRadius* border_radius) {
    if (!canvas_ || gradient.stops.empty()) {
        return;
    }

    // 提取颜色和位置
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    for (const auto& stop : gradient.stops) {
        colors.push_back(stop.color);
        positions.push_back(stop.position);
    }

    if (colors.size() < 2) {
        return;
    }

    // 计算渐变方向
    float angle_rad = (gradient.angle - 90.0f) * M_PI / 180.0f;
    float centerX = rect.centerX();
    float centerY = rect.centerY();
    float halfWidth = rect.width() / 2.0f;
    float halfHeight = rect.height() / 2.0f;

    float dx = std::cos(angle_rad);
    float dy = std::sin(angle_rad);

    float distance;
    if (std::abs(dx) < 0.0001f) {
        distance = halfHeight;
    } else if (std::abs(dy) < 0.0001f) {
        distance = halfWidth;
    } else {
        float distX = std::abs(halfWidth / dx);
        float distY = std::abs(halfHeight / dy);
        distance = std::min(distX, distY);
    }

    SkPoint pts[2];
    pts[0] = SkPoint::Make(centerX - dx * distance, centerY - dy * distance);
    pts[1] = SkPoint::Make(centerX + dx * distance, centerY + dy * distance);

    sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
        pts, colors.data(), positions.data(), 
        static_cast<int>(colors.size()), SkTileMode::kClamp);

    SkPaint paint;
    paint.setShader(shader);
    paint.setAntiAlias(true);

    if (border_radius) {
        SkPath path = CreateRoundedPath(rect, *border_radius);
        canvas_->drawPath(path, paint);
    } else {
        canvas_->drawRect(rect, paint);
    }
}

void BackgroundPainter::PaintRadialGradient(const SkRect& rect,
                                            const CSSRadialGradient& gradient,
                                            const CSSBorderRadius* border_radius) {
    if (!canvas_ || gradient.stops.empty()) {
        return;
    }

    // 提取颜色和位置
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    for (const auto& stop : gradient.stops) {
        colors.push_back(stop.color);
        positions.push_back(stop.position);
    }

    if (colors.size() < 2) {
        return;
    }

    // 计算中心点
    SkPoint center = SkPoint::Make(
        rect.left() + rect.width() * gradient.center_x,
        rect.top() + rect.height() * gradient.center_y
    );

    // 计算半径
    float radius;
    if (gradient.is_circle) {
        radius = std::min(rect.width(), rect.height()) / 2.0f;
    } else {
        radius = std::sqrt(rect.width() * rect.width() + 
                          rect.height() * rect.height()) / 2.0f;
    }

    sk_sp<SkShader> shader = SkGradientShader::MakeRadial(
        center, radius, colors.data(), positions.data(),
        static_cast<int>(colors.size()), SkTileMode::kClamp);

    SkPaint paint;
    paint.setShader(shader);
    paint.setAntiAlias(true);

    if (border_radius) {
        SkPath path = CreateRoundedPath(rect, *border_radius);
        canvas_->drawPath(path, paint);
    } else {
        canvas_->drawRect(rect, paint);
    }
}

void BackgroundPainter::PaintBackgroundImage(const SkRect& rect,
                                             const std::string& image_url,
                                             CSSBackgroundRepeat repeat,
                                             const CSSBackgroundSize& size,
                                             const CSSBorderRadius* border_radius) {
    if (!canvas_ || image_url.empty() || image_url == "none") {
        return;
    }

    std::string url = image_url;
    // 移除 url() 包装
    if (url.find("url(") == 0) {
        url = url.substr(4, url.length() - 5);
        // 移除引号
        if (!url.empty() && (url.front() == '"' || url.front() == '\'')) {
            url = url.substr(1, url.length() - 2);
        }
    }

    // 加载图片
    auto image = ImageLoader::LoadFromUrl(url);
    if (!image) {
        return;
    }

    // 计算图片尺寸
    float img_width = static_cast<float>(image->width());
    float img_height = static_cast<float>(image->height());

    if (size.type == CSSBackgroundSize::Type::COVER) {
        float scale = std::max(rect.width() / img_width, rect.height() / img_height);
        img_width *= scale;
        img_height *= scale;
    } else if (size.type == CSSBackgroundSize::Type::CONTAIN) {
        float scale = std::min(rect.width() / img_width, rect.height() / img_height);
        img_width *= scale;
        img_height *= scale;
    } else if (size.type == CSSBackgroundSize::Type::LENGTH) {
        if (!size.width.IsAuto()) {
            img_width = size.width.ToPx(rect.width());
        }
        if (!size.height.IsAuto()) {
            img_height = size.height.ToPx(rect.height());
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

    SkMatrix matrix = SkMatrix::Translate(rect.left(), rect.top());
    matrix.postScale(img_width / image->width(), img_height / image->height(),
                     rect.left(), rect.top());

    sk_sp<SkShader> shader = image->makeShader(tile_x, tile_y, 
                                                SkSamplingOptions(), matrix);

    SkPaint paint;
    paint.setShader(shader);
    paint.setAntiAlias(true);

    if (border_radius) {
        SkPath path = CreateRoundedPath(rect, *border_radius);
        canvas_->drawPath(path, paint);
    } else {
        canvas_->drawRect(rect, paint);
    }
}

void BackgroundPainter::PaintWithStyles(const Box& box,
                                        const std::unordered_map<std::string, std::string>& styles,
                                        const CSSBorderRadius* border_radius) {
    if (!canvas_) {
        return;
    }

    // 使用 border_box 绘制背景
    SkRect border_box = box.GetBorderBox();

    // 创建路径（支持圆角）
    SkPath path;
    bool has_radius = border_radius && (
        border_radius->top_left.value > 0 ||
        border_radius->top_right.value > 0 ||
        border_radius->bottom_right.value > 0 ||
        border_radius->bottom_left.value > 0
    );

    if (has_radius) {
        SkVector radii[4];
        CalculateRadii(*border_radius, border_box.width(), border_box.height(), radii);
        
        SkRRect rrect;
        rrect.setRectRadii(border_box, radii);
        path.addRRect(rrect);
    } else {
        path.addRect(border_box);
    }

    SkPaint paint;
    bool has_background = false;

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

            float angle_rad = gradient->angle * M_PI / 180.0f;
            SkPoint pts[2];
            pts[0] = SkPoint::Make(
                border_box.centerX() - cos(angle_rad) * border_box.width() / 2,
                border_box.centerY() - sin(angle_rad) * border_box.height() / 2);
            pts[1] = SkPoint::Make(
                border_box.centerX() + cos(angle_rad) * border_box.width() / 2,
                border_box.centerY() + sin(angle_rad) * border_box.height() / 2);

            sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
                pts, colors.data(), positions.data(), 
                static_cast<int>(colors.size()), SkTileMode::kClamp);
            paint.setShader(shader);
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
                border_box.left() + border_box.width() * gradient->center_x,
                border_box.top() + border_box.height() * gradient->center_y
            );

            float radius = std::max(border_box.width(), border_box.height()) / 2.0f;

            sk_sp<SkShader> shader = SkGradientShader::MakeRadial(
                center, radius, colors.data(), positions.data(),
                static_cast<int>(colors.size()), SkTileMode::kClamp);
            paint.setShader(shader);
            has_background = true;
        }
    }
    // 3. 背景图片
    else if (bg_image_it != styles.end() && bg_image_it->second != "none") {
        std::string url = bg_image_it->second;
        // 移除 url() 包装
        if (url.find("url(") == 0) {
            url = url.substr(4, url.length() - 5);
            if (!url.empty() && (url.front() == '"' || url.front() == '\'')) {
                url = url.substr(1, url.length() - 2);
            }
        }

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
            float img_width = static_cast<float>(image->width());
            float img_height = static_cast<float>(image->height());

            if (bg_size.type == CSSBackgroundSize::Type::COVER) {
                float scale = std::max(border_box.width() / img_width,
                                      border_box.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::CONTAIN) {
                float scale = std::min(border_box.width() / img_width,
                                      border_box.height() / img_height);
                img_width *= scale;
                img_height *= scale;
            } else if (bg_size.type == CSSBackgroundSize::Type::LENGTH) {
                if (!bg_size.width.IsAuto()) {
                    img_width = bg_size.width.ToPx(border_box.width());
                }
                if (!bg_size.height.IsAuto()) {
                    img_height = bg_size.height.ToPx(border_box.height());
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

            SkMatrix matrix = SkMatrix::Translate(border_box.left(), border_box.top());
            matrix.postScale(img_width / image->width(), img_height / image->height(),
                           border_box.left(), border_box.top());

            sk_sp<SkShader> shader = image->makeShader(tile_x, tile_y, 
                                                        SkSamplingOptions(), matrix);
            paint.setShader(shader);
            has_background = true;
        }
    }
    // 4. 纯色背景
    else if (bg_color_it != styles.end() && !bg_color_it->second.empty() &&
             bg_color_it->second != "transparent") {
        SkColor parsed_color = Color::Parse(bg_color_it->second);
        paint.setColor(parsed_color);
        has_background = true;
    }

    // 只有在有有效背景时才绘制
    if (has_background) {
        canvas_->drawPath(path, paint);
    }
}

SkPath BackgroundPainter::CreateRoundedPath(const SkRect& rect, 
                                            const CSSBorderRadius& border_radius) {
    SkPath path;
    SkVector radii[4];
    CalculateRadii(border_radius, rect.width(), rect.height(), radii);

    bool has_radius = radii[0].fX > 0 || radii[1].fX > 0 || 
                      radii[2].fX > 0 || radii[3].fX > 0;

    if (has_radius) {
        SkRRect rrect;
        rrect.setRectRadii(rect, radii);
        path.addRRect(rrect);
    } else {
        path.addRect(rect);
    }

    return path;
}

void BackgroundPainter::CalculateRadii(const CSSBorderRadius& border_radius,
                                       float width, float height,
                                       SkVector out_radii[4]) {
    // border-radius 百分比值应该相对于元素尺寸计算
    float base_size = std::min(width, height);
    
    float tl = border_radius.top_left.ToPx(base_size);
    float tr = border_radius.top_right.ToPx(base_size);
    float br = border_radius.bottom_right.ToPx(base_size);
    float bl = border_radius.bottom_left.ToPx(base_size);

    out_radii[0] = {tl, tl};  // top-left
    out_radii[1] = {tr, tr};  // top-right
    out_radii[2] = {br, br};  // bottom-right
    out_radii[3] = {bl, bl};  // bottom-left
}

} // namespace lightui
