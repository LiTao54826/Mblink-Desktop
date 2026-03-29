/**
 * @file scrollbar_painter.cpp
 * @brief 滚动条绘制器实现
 * 
 * 从 render_object.cpp 提取的滚动条绘制逻辑
 */

#include "scrollbar_painter.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkRRect.h"
#include <algorithm>

namespace mbink {

ScrollbarPainter::ScrollbarPainter(SkCanvas* canvas)
    : canvas_(canvas) {
}

void ScrollbarPainter::Paint(const ScrollbarPaintParams& params) {
    // 应用 CSS scrollbar-color 自定义颜色
    if (!params.scrollbar_color_auto) {
        SetThumbColor(params.scrollbar_thumb_color);
        SetTrackColor(params.scrollbar_track_color);
    }

    if (params.needs_h_scroll) {
        PaintHorizontalScrollbar(params);
    }

    if (params.needs_v_scroll) {
        PaintVerticalScrollbar(params);
    }

    // 绘制滚动条角落（当两个滚动条都存在时）
    if (params.needs_h_scroll && params.needs_v_scroll) {
        PaintScrollbarCorner(params);
    }
}

void ScrollbarPainter::PaintHorizontalScrollbar(const ScrollbarPaintParams& params) {
    float track_x = params.border_left;
    float track_y = params.effective_height - params.border_bottom - kScrollbarWidth;
    float track_width = params.visible_width - (params.needs_v_scroll ? kScrollbarWidth : 0);
    
    // 绘制轨道
    SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, track_width, kScrollbarWidth);
    PaintTrack(track_rect);
    
    // 计算滑块尺寸和位置
    float available_content_width = params.visible_width - (params.needs_v_scroll ? kScrollbarWidth : 0);
    float scrollable_width = params.content_width - available_content_width;
    
    float thumb_pos, thumb_size;
    CalculateThumbMetrics(
        track_width - 2 * kScrollbarMargin,
        params.content_width,
        available_content_width,
        params.scroll_x,
        thumb_pos,
        thumb_size);
    
    float thumb_x = track_x + kScrollbarMargin + thumb_pos;
    
    SkRect thumb_rect = SkRect::MakeXYWH(
        thumb_x,
        track_y + kScrollbarMargin,
        thumb_size,
        kScrollbarWidth - 2 * kScrollbarMargin
    );
    PaintThumb(thumb_rect);
}

void ScrollbarPainter::PaintVerticalScrollbar(const ScrollbarPaintParams& params) {
    float track_x = params.effective_width - params.border_right - kScrollbarWidth;
    float track_y = params.border_top;
    float track_height = params.visible_height - (params.needs_h_scroll ? kScrollbarWidth : 0);
    
    // 绘制轨道
    SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, kScrollbarWidth, track_height);
    PaintTrack(track_rect);
    
    // 计算滑块尺寸和位置
    float available_content_height = params.visible_height - (params.needs_h_scroll ? kScrollbarWidth : 0);
    
    float thumb_pos, thumb_size;
    CalculateThumbMetrics(
        track_height - 2 * kScrollbarMargin,
        params.content_height,
        available_content_height,
        params.scroll_y,
        thumb_pos,
        thumb_size);
    
    float thumb_y = track_y + kScrollbarMargin + thumb_pos;
    
    SkRect thumb_rect = SkRect::MakeXYWH(
        track_x + kScrollbarMargin,
        thumb_y,
        kScrollbarWidth - 2 * kScrollbarMargin,
        thumb_size
    );
    PaintThumb(thumb_rect);
}

void ScrollbarPainter::PaintScrollbarCorner(const ScrollbarPaintParams& params) {
    float corner_x = params.effective_width - params.border_right - kScrollbarWidth;
    float corner_y = params.effective_height - params.border_bottom - kScrollbarWidth;
    
    SkRect corner_rect = SkRect::MakeXYWH(corner_x, corner_y, kScrollbarWidth, kScrollbarWidth);
    PaintTrack(corner_rect);  // 角落使用轨道颜色
}

void ScrollbarPainter::PaintTrack(const SkRect& rect) {
    SkPaint paint;
    paint.setColor(track_color_);
    paint.setAntiAlias(true);
    canvas_->drawRect(rect, paint);
}

void ScrollbarPainter::PaintThumb(const SkRect& rect) {
    SkPaint paint;
    paint.setColor(thumb_color_);
    paint.setAntiAlias(true);
    canvas_->drawRoundRect(rect, kCornerRadius, kCornerRadius, paint);
}

void ScrollbarPainter::CalculateThumbMetrics(
    float track_size,
    float content_size,
    float visible_size,
    float scroll_pos,
    float& out_thumb_pos,
    float& out_thumb_size) {
    
    // 计算滑块比例
    float thumb_ratio = visible_size / content_size;
    out_thumb_size = std::max(kMinThumbSize, track_size * thumb_ratio);
    
    // 计算可用轨道空间
    float available_track = track_size - out_thumb_size;
    
    // 计算滚动比例
    float scrollable_size = content_size - visible_size;
    float scroll_ratio = scrollable_size > 0 ? scroll_pos / scrollable_size : 0;
    
    // 计算滑块位置
    out_thumb_pos = available_track * scroll_ratio;
}

ScrollbarPaintParams ScrollbarPainter::CreateParams(
    const RenderObject& render_object,
    const Box& box,
    const ComputedStyle& style) {
    
    ScrollbarPaintParams params;
    
    // 获取有效尺寸
    params.effective_width = render_object.GetEffectiveVisibleWidth();
    params.effective_height = render_object.GetEffectiveVisibleHeight();
    
    // 计算可见区域
    params.visible_width = params.effective_width - box.border_left_width - box.border_right_width;
    params.visible_height = params.effective_height - box.border_top_width - box.border_bottom_width;
    
    // 边框宽度
    params.border_left = box.border_left_width;
    params.border_right = box.border_right_width;
    params.border_top = box.border_top_width;
    params.border_bottom = box.border_bottom_width;
    
    // 内容尺寸
    float content_width = render_object.GetContentWidth();
    float content_height = render_object.GetContentHeight();
    
    if (content_width <= 0) {
        content_width = render_object.CalculateContentWidth();
    }
    if (content_height <= 0) {
        content_height = render_object.CalculateContentHeight();
    }
    
    params.content_width = content_width;
    params.content_height = content_height;
    
    // 滚动位置
    params.scroll_x = render_object.GetScrollX();
    params.scroll_y = render_object.GetScrollY();
    
    // 获取 overflow 设置
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");
    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    
    // 判断是否需要滚动条
    params.needs_v_scroll = allow_v_scroll && 
        (content_height > params.visible_height || overflow_y == "scroll");
    
    float content_area_width = params.visible_width - 
        (params.needs_v_scroll ? kScrollbarWidth : 0);
    
    params.needs_h_scroll = allow_h_scroll && 
        (content_width > content_area_width || overflow_x == "scroll");
    
    // 如果需要水平滚动条，重新检查垂直滚动条
    if (params.needs_h_scroll) {
        float content_area_height = params.visible_height - kScrollbarWidth;
        if (allow_v_scroll && !params.needs_v_scroll && 
            content_height > content_area_height) {
            params.needs_v_scroll = true;
        }
    }
    
    // CSS scrollbar-color 属性
    params.scrollbar_color_auto = style.scrollbar_color_auto;
    params.scrollbar_thumb_color = style.scrollbar_thumb_color;
    params.scrollbar_track_color = style.scrollbar_track_color;

    return params;
}

} // namespace mbink
