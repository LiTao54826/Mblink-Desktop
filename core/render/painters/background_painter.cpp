/**
 * @file background_painter.cpp
 * @brief 背景绘制器实现
 * 
 * 从 render_object.cpp 和 box_renderer.cpp 提取的背景绘制逻辑
 */

#include "background_painter.h"
#include "core/render/objects/render_object.h"
#include "core/render/utils/color.h"
#include "core/render/utils/gradient_renderer.h"
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
    SkRect padding_box = box.GetPaddingBox();

    // 优先级：多层渐变 > 单个渐变 > 径向渐变 > 纯色背景
    if (!style.background_linear_gradients.empty()) {
        // 多层线性渐变（CSS规范：列表中第一个在最上层，最后一个在最下层）
        // 因此需要反向绘制：先绘制最后一个（底层），最后绘制第一个（顶层）
        for (int i = static_cast<int>(style.background_linear_gradients.size()) - 1; i >= 0; i--) {
            const auto& gradient = style.background_linear_gradients[i];

            // 获取对应的 background-size（如果有）
            CSSBackgroundSize bg_size;
            if (!style.background_sizes.empty()) {
                // 如果 size 数量少于 gradient，则循环使用
                size_t size_index = i % style.background_sizes.size();
                bg_size = style.background_sizes[size_index];
            } else if (style.background_size.type != CSSBackgroundSize::Type::AUTO) {
                // 如果没有多个 size，但有单个 size，使用单个 size（向后兼容）
                bg_size = style.background_size;
            } else {
            }

            if (bg_size.type == CSSBackgroundSize::Type::LENGTH) {
            }

            // 使用 background-size 调整渲染区域
            SkRect render_rect = padding_box;
            if (bg_size.type == CSSBackgroundSize::Type::LENGTH) {
                if (!bg_size.width.IsAuto() && !bg_size.height.IsAuto()) {
                    float width = bg_size.width.ToPx(padding_box.width());
                    float height = bg_size.height.ToPx(padding_box.height());


                    // 创建平铺效果：使用 shader 的平铺模式
                    // 先绘制一个小的渐变单元，然后通过 shader 平铺
                    SkRect tile_rect = SkRect::MakeXYWH(padding_box.left(), padding_box.top(), width, height);

                    // 提取颜色和位置
                    std::vector<SkColor> colors;
                    std::vector<SkScalar> positions;
                    for (const auto& stop : gradient.stops) {
                        colors.push_back(stop.color);
                        positions.push_back(stop.position);
                    }

                    if (colors.size() >= 2) {
                        // 计算渐变方向（使用相对坐标系统，从 (0,0) 到 (width, height)）
                        float angle_rad = (gradient.angle - 90.0f) * M_PI / 180.0f;

                        float dx = std::cos(angle_rad);
                        float dy = std::sin(angle_rad);

                        // 对于平铺背景，渐变应该在一个 tile 单元内完成
                        // 起点和终点基于 tile 的尺寸
                        // CSS 渐变角度：0deg=向上, 90deg=向右, 180deg=向下(默认), 270deg=向左
                        SkPoint pts[2];

                        if (std::abs(gradient.angle) < 0.01f) {
                            // 0度：向上（从下到上）
                            pts[0] = SkPoint::Make(0, height);
                            pts[1] = SkPoint::Make(0, 0);
                        } else if (std::abs(gradient.angle - 180.0f) < 0.01f) {
                            // 180度：向下（从上到下）- CSS 默认方向
                            pts[0] = SkPoint::Make(0, 0);
                            pts[1] = SkPoint::Make(0, height);
                        } else if (std::abs(gradient.angle - 90.0f) < 0.01f) {
                            // 90度：向右（从左到右）
                            pts[0] = SkPoint::Make(0, 0);
                            pts[1] = SkPoint::Make(width, 0);
                        } else if (std::abs(gradient.angle - 270.0f) < 0.01f) {
                            // 270度：向左（从右到左）
                            pts[0] = SkPoint::Make(width, 0);
                            pts[1] = SkPoint::Make(0, 0);
                        } else {
                            // 其他角度
                            float halfWidth = width / 2.0f;
                            float halfHeight = height / 2.0f;
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

                            pts[0] = SkPoint::Make(halfWidth - dx * distance, halfHeight - dy * distance);
                            pts[1] = SkPoint::Make(halfWidth + dx * distance, halfHeight + dy * distance);
                        }

                        // 创建平铺的渐变 shader（使用局部矩阵进行平铺）
                        sk_sp<SkShader> gradient_shader = SkGradientShader::MakeLinear(
                            pts, colors.data(), positions.data(),
                            static_cast<int>(colors.size()), SkTileMode::kRepeat);

                        // 创建矩阵变换，将 shader 平移到正确的位置
                        SkMatrix matrix;
                        matrix.setTranslate(padding_box.left(), padding_box.top());

                        sk_sp<SkShader> shader = gradient_shader->makeWithLocalMatrix(matrix);

                        SkPaint paint;
                        paint.setShader(shader);
                        paint.setAntiAlias(true);

                        canvas_->drawRect(padding_box, paint);
                    }

                    continue;
                }
            }

            // 默认渲染（无特殊 size 或 auto）
            GradientRenderer::RenderLinearGradient(canvas_, padding_box, gradient);
        }
    } else if (style.background_linear_gradient.has_value()) {
        // 单个线性渐变（向后兼容）
        GradientRenderer::RenderLinearGradient(canvas_, padding_box,
                                               *style.background_linear_gradient);
    } else if (style.background_radial_gradient.has_value()) {
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
