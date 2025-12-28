/**
 * @file scrollbar_controller.cpp
 * @brief 滚动条控制器实现
 * 
 * 从 render_object.cpp 提取的滚动条逻辑代码
 */

#include "core/render/scrollbar_controller.h"

#include <algorithm>
#include <cmath>

namespace lightui {

ScrollbarHitArea ScrollbarController::HitTestScrollbar(
    float local_x, float local_y,
    const ScrollbarHitTestParams& params) const {
    
    // 获取 overflow 设置
    bool allow_h_scroll = (params.overflow_x == "scroll" || params.overflow_x == "auto");
    bool allow_v_scroll = (params.overflow_y == "scroll" || params.overflow_y == "auto");

    // 只有设置了 overflow: scroll 或 auto 才有滚动条
    if (!allow_h_scroll && !allow_v_scroll) {
        return ScrollbarHitArea::None;
    }

    // 计算可见区域（减去 border）
    float visible_width = params.visible_width - params.border_left - params.border_right;
    float visible_height = params.visible_height - params.border_top - params.border_bottom;

    // 判断是否需要滚动条
    bool needs_v_scroll = NeedsVerticalScrollbar(params.content_height, visible_height, 
                                                  params.overflow_y);
    float content_area_width = visible_width - (needs_v_scroll ? kScrollbarWidth : 0);
    bool needs_h_scroll = NeedsHorizontalScrollbar(params.content_width, content_area_width,
                                                    params.overflow_x);

    // 检测垂直滚动条区域（优先检测，因为它更常见）
    if (needs_v_scroll) {
        float track_x = params.border_left + visible_width - kScrollbarWidth;
        float track_height = visible_height - (needs_h_scroll ? kScrollbarWidth : 0);

        if (local_x >= track_x && local_x <= params.border_left + visible_width &&
            local_y >= params.border_top && local_y <= params.border_top + track_height) {
            return ScrollbarHitArea::VerticalTrack;
        }
    }

    // 检测水平滚动条区域
    if (needs_h_scroll) {
        float track_y = params.border_top + visible_height - kScrollbarWidth;
        float track_width = visible_width - (needs_v_scroll ? kScrollbarWidth : 0);

        if (local_y >= track_y && local_y <= params.border_top + visible_height &&
            local_x >= params.border_left && local_x <= params.border_left + track_width) {
            return ScrollbarHitArea::HorizontalTrack;
        }
    }

    return ScrollbarHitArea::None;
}

void ScrollbarController::StartDrag(ScrollbarHitArea area, float mouse_x, float mouse_y,
                                     float current_scroll_x, float current_scroll_y) {
    if (area == ScrollbarHitArea::None) {
        return;
    }

    dragging_area_ = area;

    if (area == ScrollbarHitArea::HorizontalTrack || area == ScrollbarHitArea::HorizontalThumb) {
        drag_start_scroll_ = current_scroll_x;
        drag_start_mouse_ = mouse_x;
    } else {
        drag_start_scroll_ = current_scroll_y;
        drag_start_mouse_ = mouse_y;
    }
}

bool ScrollbarController::UpdateDrag(float mouse_x, float mouse_y,
                                       const ScrollbarDragParams& params,
                                       float& out_scroll_x, float& out_scroll_y) {
    if (dragging_area_ == ScrollbarHitArea::None) {
        return false;
    }

    // 计算可见区域（减去 border）
    float visible_width = params.visible_width - params.border_left - params.border_right;
    float visible_height = params.visible_height - params.border_top - params.border_bottom;

    // 判断是否需要滚动条
    bool needs_v_scroll = params.content_height > visible_height;
    float content_area_width = visible_width - (needs_v_scroll ? kScrollbarWidth : 0);
    bool needs_h_scroll = params.content_width > content_area_width;
    float content_area_height = visible_height - (needs_h_scroll ? kScrollbarWidth : 0);

    if (dragging_area_ == ScrollbarHitArea::HorizontalTrack ||
        dragging_area_ == ScrollbarHitArea::HorizontalThumb) {
        // 水平滚动
        float scrollable_width = params.content_width - content_area_width;

        if (scrollable_width > 0 && content_area_width > 0) {
            float thumb_size, track_length;
            CalculateThumbMetrics(content_area_width, params.content_width, 
                                  content_area_width, thumb_size, track_length);

            if (track_length > 0) {
                // 鼠标移动距离转换为滚动距离
                float mouse_delta = mouse_x - drag_start_mouse_;
                float scroll_delta = (mouse_delta / track_length) * scrollable_width;

                out_scroll_x = std::max(0.0f, std::min(drag_start_scroll_ + scroll_delta, 
                                                        scrollable_width));
                return true;
            }
        }
    } else {
        // 垂直滚动
        float scrollable_height = params.content_height - content_area_height;

        if (scrollable_height > 0 && content_area_height > 0) {
            float thumb_size, track_length;
            CalculateThumbMetrics(content_area_height, params.content_height,
                                  content_area_height, thumb_size, track_length);

            if (track_length > 0) {
                // 鼠标移动距离转换为滚动距离
                float mouse_delta = mouse_y - drag_start_mouse_;
                float scroll_delta = (mouse_delta / track_length) * scrollable_height;

                out_scroll_y = std::max(0.0f, std::min(drag_start_scroll_ + scroll_delta,
                                                        scrollable_height));
                return true;
            }
        }
    }

    return false;
}

void ScrollbarController::EndDrag() {
    dragging_area_ = ScrollbarHitArea::None;
    drag_start_scroll_ = 0.0f;
    drag_start_mouse_ = 0.0f;
}

bool ScrollbarController::NeedsHorizontalScrollbar(float content_width, float visible_width,
                                                    const std::string& overflow_x) {
    if (overflow_x == "scroll") {
        return true;
    }
    if (overflow_x == "auto") {
        return content_width > visible_width;
    }
    return false;
}

bool ScrollbarController::NeedsVerticalScrollbar(float content_height, float visible_height,
                                                  const std::string& overflow_y) {
    if (overflow_y == "scroll") {
        return true;
    }
    if (overflow_y == "auto") {
        return content_height > visible_height;
    }
    return false;
}

void ScrollbarController::CalculateThumbMetrics(float track_size, float content_size,
                                                 float visible_size,
                                                 float& out_thumb_size,
                                                 float& out_track_length) const {
    if (content_size <= 0 || visible_size <= 0) {
        out_thumb_size = kMinThumbSize;
        out_track_length = 0.0f;
        return;
    }

    float thumb_ratio = visible_size / content_size;
    out_thumb_size = std::max(kMinThumbSize, track_size * thumb_ratio);
    out_track_length = track_size - out_thumb_size;
}

} // namespace lightui
