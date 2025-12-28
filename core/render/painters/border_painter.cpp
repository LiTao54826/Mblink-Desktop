/**
 * @file border_painter.cpp
 * @brief 边框绘制器实现
 * 
 * 从 render_object.cpp 和 box_renderer.cpp 提取的边框绘制逻辑
 */

#include "border_painter.h"
#include "core/render/render_object.h"
#include "core/render/paint.h"
#include "core/render/shapes.h"
#include "core/dom/element.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

BorderPainter::BorderPainter(SkCanvas* canvas)
    : canvas_(canvas) {
}

void BorderPainter::Paint(const Box& box, 
                          const ComputedStyle& style, 
                          const PaintCache& cache,
                          const std::vector<std::shared_ptr<RenderObject>>* children) {
    if (!canvas_ || !cache.has_border) {
        return;
    }

    SkRect border_box = box.GetBorderBox();

    // 使用缓存的圆角标志
    bool has_border_radius = cache.has_border_radius;

    if (has_border_radius) {
        // 有圆角：使用 PaintRoundedBorderAdvanced（支持每边独立属性）
        
        // 准备四边宽度数组 [top, right, bottom, left]
        float border_widths[4] = {
            box.border_top_width,
            box.border_right_width,
            box.border_bottom_width,
            box.border_left_width
        };
        
        // 准备四边样式数组
        CSSBorderStyle border_styles[4] = {
            style.border_top_style != CSSBorderStyle::NONE ? style.border_top_style : style.border.style,
            style.border_right_style != CSSBorderStyle::NONE ? style.border_right_style : style.border.style,
            style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_style : style.border.style,
            style.border_left_style != CSSBorderStyle::NONE ? style.border_left_style : style.border.style
        };
        
        // 准备四边颜色数组
        SkColor border_colors[4] = {
            style.border_top_style != CSSBorderStyle::NONE ? style.border_top_color : style.border.color,
            style.border_right_style != CSSBorderStyle::NONE ? style.border_right_color : style.border.color,
            style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_color : style.border.color,
            style.border_left_style != CSSBorderStyle::NONE ? style.border_left_color : style.border.color
        };
        
        PaintRoundedBorderAdvanced(box, border_widths, border_styles, border_colors, style.border_radius);
        
    } else {
        // 无圆角：使用独立边框渲染逻辑
        
        // 边框绘制时需要向内偏移半个边框宽度
        // 因为 Skia 的线条是以指定坐标为中心绘制的
        float half_left = box.border_left_width / 2.0f;
        float half_right = box.border_right_width / 2.0f;
        float half_top = box.border_top_width / 2.0f;
        float half_bottom = box.border_bottom_width / 2.0f;

        // 检查是否是 fieldset 元素，需要特殊处理上边框
        bool is_fieldset = false;
        float legend_left = 0, legend_right = 0;
        RenderObject* legend_render = nullptr;

        if (children) {
            // 检查是否是 fieldset（通过查找 legend 子元素）
            for (const auto& child : *children) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto child_elem = std::static_pointer_cast<Element>(child_node);
                    if (child_elem->GetTagName() == "legend") {
                        is_fieldset = true;
                        legend_render = child.get();
                        auto& legend_layout = child->GetLayoutInfo();
                        auto& legend_style = child->GetComputedStyle();

                        // 计算 legend 的实际渲染宽度
                        float max_child_right = 0.0f;
                        for (const auto& grandchild : child->GetChildren()) {
                            auto& gc_layout = grandchild->GetLayoutInfo();
                            max_child_right = std::max(max_child_right, gc_layout.x + gc_layout.width);
                        }

                        float legend_padding_right = legend_style.padding.right.ToPx();
                        float legend_border_right = legend_style.border_right_width > 0 ?
                            legend_style.border_right_width : legend_style.border.width.ToPx();

                        legend_left = legend_layout.x;
                        legend_right = legend_layout.x + max_child_right + legend_padding_right + legend_border_right;
                        break;
                    }
                }
            }
        }

        // 渲染左边框
        if (box.border_left_width > 0) {
            CSSBorderStyle left_style = style.border_left_style != CSSBorderStyle::NONE ?
                                        style.border_left_style : style.border.style;
            SkColor left_color = style.border_left_style != CSSBorderStyle::NONE ?
                                 style.border_left_color : style.border.color;
            if (left_style != CSSBorderStyle::NONE) {
                PaintBorderEdge(
                    border_box.left() + half_left, border_box.top(),
                    border_box.left() + half_left, border_box.bottom(),
                    box.border_left_width, left_style, left_color
                );
            }
        }

        // 渲染右边框
        if (box.border_right_width > 0) {
            CSSBorderStyle right_style = style.border_right_style != CSSBorderStyle::NONE ?
                                         style.border_right_style : style.border.style;
            SkColor right_color = style.border_right_style != CSSBorderStyle::NONE ?
                                  style.border_right_color : style.border.color;
            if (right_style != CSSBorderStyle::NONE) {
                PaintBorderEdge(
                    border_box.right() - half_right, border_box.top(),
                    border_box.right() - half_right, border_box.bottom(),
                    box.border_right_width, right_style, right_color
                );
            }
        }

        // 渲染上边框 - fieldset 需要特殊处理（在 legend 位置断开）
        if (box.border_top_width > 0) {
            CSSBorderStyle top_style = style.border_top_style != CSSBorderStyle::NONE ?
                                       style.border_top_style : style.border.style;
            SkColor top_color = style.border_top_style != CSSBorderStyle::NONE ?
                                style.border_top_color : style.border.color;
            if (top_style != CSSBorderStyle::NONE) {
                if (is_fieldset && legend_render) {
                    // fieldset 上边框在 legend 位置断开
                    if (legend_left > border_box.left()) {
                        PaintBorderEdge(
                            border_box.left(), border_box.top() + half_top,
                            legend_left, border_box.top() + half_top,
                            box.border_top_width, top_style, top_color
                        );
                    }
                    if (legend_right < border_box.right()) {
                        PaintBorderEdge(
                            legend_right, border_box.top() + half_top,
                            border_box.right(), border_box.top() + half_top,
                            box.border_top_width, top_style, top_color
                        );
                    }
                } else {
                    // 普通元素：绘制完整上边框
                    PaintBorderEdge(
                        border_box.left(), border_box.top() + half_top,
                        border_box.right(), border_box.top() + half_top,
                        box.border_top_width, top_style, top_color
                    );
                }
            }
        }

        // 渲染下边框
        if (box.border_bottom_width > 0) {
            CSSBorderStyle bottom_style = style.border_bottom_style != CSSBorderStyle::NONE ?
                                          style.border_bottom_style : style.border.style;
            SkColor bottom_color = style.border_bottom_style != CSSBorderStyle::NONE ?
                                   style.border_bottom_color : style.border.color;
            if (bottom_style != CSSBorderStyle::NONE) {
                PaintBorderEdge(
                    border_box.left(), border_box.bottom() - half_bottom,
                    border_box.right(), border_box.bottom() - half_bottom,
                    box.border_bottom_width, bottom_style, bottom_color
                );
            }
        }
    }
}

void BorderPainter::PaintBorderEdge(float x1, float y1, float x2, float y2,
                                    float width, CSSBorderStyle style, SkColor color) {
    if (!canvas_ || style == CSSBorderStyle::NONE || width <= 0) {
        return;
    }

    SkPaint paint;
    paint.setColor(color);
    paint.setStrokeWidth(width);
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    SetBorderStyle(paint, style, width);
    
    if (style == CSSBorderStyle::DOUBLE) {
        // 双线（绘制两条线）
        float third = width / 3.0f;
        SkPaint thin_paint = paint;
        thin_paint.setStrokeWidth(third);
        
        // 计算偏移
        float dx = (x2 - x1 == 0) ? third : 0;
        float dy = (y2 - y1 == 0) ? third : 0;
        
        canvas_->drawLine(x1 - dx, y1 - dy, x2 - dx, y2 - dy, thin_paint);
        canvas_->drawLine(x1 + dx, y1 + dy, x2 + dx, y2 + dy, thin_paint);
        return;
    }
    
    canvas_->drawLine(x1, y1, x2, y2, paint);
}


void BorderPainter::PaintRoundedBorder(const Box& box,
                                       float border_width,
                                       CSSBorderStyle border_style,
                                       SkColor border_color,
                                       const CSSBorderRadius& border_radius) {
    if (!canvas_ || border_width <= 0 || border_style == CSSBorderStyle::NONE) {
        return;
    }

    float widths[4] = {border_width, border_width, border_width, border_width};
    CSSBorderStyle styles[4] = {border_style, border_style, border_style, border_style};
    SkColor colors[4] = {border_color, border_color, border_color, border_color};
    
    PaintRoundedBorderAdvanced(box, widths, styles, colors, border_radius);
}

void BorderPainter::PaintRoundedBorderAdvanced(const Box& box,
                                               const float border_widths[4],
                                               const CSSBorderStyle border_styles[4],
                                               const SkColor border_colors[4],
                                               const CSSBorderRadius& border_radius) {
    if (!canvas_) {
        return;
    }

    // 索引定义：0=top, 1=right, 2=bottom, 3=left
    bool has_top = border_widths[0] > 0 && border_styles[0] != CSSBorderStyle::NONE;
    bool has_right = border_widths[1] > 0 && border_styles[1] != CSSBorderStyle::NONE;
    bool has_bottom = border_widths[2] > 0 && border_styles[2] != CSSBorderStyle::NONE;
    bool has_left = border_widths[3] > 0 && border_styles[3] != CSSBorderStyle::NONE;
    
    if (!has_top && !has_right && !has_bottom && !has_left) {
        return;
    }
    
    // 检查四边属性是否完全相同
    bool all_same = AreAllSidesEqual(border_widths, border_styles, border_colors);
    
    SkRect border_box = box.GetBorderBox();
    
    // 计算 border-radius 百分比的基准尺寸
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

    if (all_same && has_top && has_right && has_bottom && has_left) {
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
        
        SkPaint paint;
        paint.setColor(border_colors[0]);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(width);
        paint.setAntiAlias(true);
        
        SetBorderStyle(paint, border_styles[0], width);
        
        SkPath path;
        path.addRRect(rrect);
        canvas_->drawPath(path, paint);
    } else {
        // 四边属性不同：分段绘制（支持圆角）
        PaintSegmentedBorder(box, border_widths, border_styles, border_colors, 
                             outer_radii, border_box, has_top, has_right, has_bottom, has_left);
    }
}

void BorderPainter::SetBorderStyle(SkPaint& paint, CSSBorderStyle style, float width) {
    switch (style) {
        case CSSBorderStyle::SOLID:
            // 实线，无需特殊处理
            break;
            
        case CSSBorderStyle::DASHED: {
            // 虚线
            float intervals[] = {width * 3, width * 3};
            paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
            break;
        }
            
        case CSSBorderStyle::DOTTED: {
            // 点线
            float intervals[] = {width, width};
            paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
            paint.setStrokeCap(SkPaint::kRound_Cap);
            break;
        }
            
        case CSSBorderStyle::NONE:
        default:
            break;
    }
}

bool BorderPainter::AreAllSidesEqual(const float widths[4],
                                     const CSSBorderStyle styles[4],
                                     const SkColor colors[4]) const {
    return widths[0] == widths[1] && widths[1] == widths[2] && widths[2] == widths[3] &&
           styles[0] == styles[1] && styles[1] == styles[2] && styles[2] == styles[3] &&
           colors[0] == colors[1] && colors[1] == colors[2] && colors[2] == colors[3];
}

SkPath BorderPainter::CreateRoundedPath(const SkRect& rect, 
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

void BorderPainter::CalculateRadii(const CSSBorderRadius& border_radius,
                                   float width, float height,
                                   SkVector out_radii[4]) {
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


void BorderPainter::PaintSegmentedBorder(const Box& box,
                                         const float border_widths[4],
                                         const CSSBorderStyle border_styles[4],
                                         const SkColor border_colors[4],
                                         const SkVector outer_radii[4],
                                         const SkRect& border_box,
                                         bool has_top, bool has_right, 
                                         bool has_bottom, bool has_left) {
    // 1. 计算分割角 (Split Angles)
    // Skia 角度: 0=R, 90=D, 180=L, 270=U
    auto to_deg = [](float rad) { return rad * 180.0f / static_cast<float>(M_PI); };
    
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

    auto draw_side = [&](int side, 
                         int c1_idx, float c1_start, float c1_sweep,
                         int c2_idx, float c2_start, float c2_sweep) {
        CSSBorderStyle style = border_styles[side];
        float width = border_widths[side];
        
        // 对于 dashed/dotted 样式，使用 STROKE 模式沿中线绘制
        bool use_stroke = (style == CSSBorderStyle::DASHED || style == CSSBorderStyle::DOTTED);
        
        if (use_stroke) {
            // STROKE 模式：沿边框中线绘制，支持虚线效果
            SkPath stroke_path;
            
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
            
            SkPaint paint;
            paint.setColor(border_colors[side]);
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(width);
            paint.setAntiAlias(true);
            
            if (style == CSSBorderStyle::DASHED) {
                float intervals[] = {width * 3, width * 3};
                paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
            } else if (style == CSSBorderStyle::DOTTED) {
                float intervals[] = {width, width * 2};
                paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
                paint.setStrokeCap(SkPaint::kRound_Cap);
            }
            
            canvas_->drawPath(stroke_path, paint);
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
            
            SkPaint paint;
            paint.setColor(border_colors[side]);
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas_->drawPath(path, paint);
        }
    };
    
    // Draw Top (0) - Connects TL(0) and TR(1)
    if (has_top) {
        draw_side(0, 0, angle_tl, 270 - angle_tl, 1, 270, angle_tr - 270);
    }
    
    // Draw Right (1) - Connects TR(1) and BR(2)
    if (has_right) {
        draw_side(1, 1, angle_tr, 360 - angle_tr, 2, 0, angle_br);
    }
    
    // Draw Bottom (2) - Connects BR(2) and BL(3)
    if (has_bottom) {
        draw_side(2, 2, angle_br, 90 - angle_br, 3, 90, angle_bl - 90);
    }
    
    // Draw Left (3) - Connects BL(3) and TL(0)
    if (has_left) {
        draw_side(3, 3, angle_bl, 180 - angle_bl, 0, 180, angle_tl - 180);
    }
}

} // namespace lightui
