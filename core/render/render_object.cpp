/**
 * @file render_object.cpp
 * @brief 渲染对象实现
 */

#include "render_object.h"
#include "box_renderer.h"
#include "text_renderer.h"
#include "gradient_renderer.h"
#include "shadow_renderer.h"
#include "color.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

namespace lightui {

// 静态成员初始化
float RenderObject::viewport_width_ = 0.0f;
float RenderObject::viewport_height_ = 0.0f;

void RenderObject::SetViewportSize(float width, float height) {
    viewport_width_ = width;
    viewport_height_ = height;
}

bool RenderObject::IsBodyElement() const {
    auto node = GetNode();
    if (node) {
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element) {
            std::string tag = element->GetTagName();
            return (tag == "body" || tag == "BODY");
        }
    }
    return false;
}

float RenderObject::GetEffectiveVisibleWidth() const {
    if (IsBodyElement() && viewport_width_ > 0) {
        return viewport_width_;
    }
    return layout_info_.width;
}

float RenderObject::GetEffectiveVisibleHeight() const {
    if (IsBodyElement() && viewport_height_ > 0) {
        return viewport_height_;
    }

    // 如果设置了 max-height，使用 max-height 作为可见高度
    float max_height_px = computed_style_.max_height.ToPx();
    if (max_height_px > 0 && max_height_px < layout_info_.height) {
        return max_height_px;
    }

    return layout_info_.height;
}

// ========== RenderObject 基类实现 ==========

RenderObject::RenderObject(RenderObjectType type)
    : type_(type)
    , node_()
    , parent_()
    , children_()
    , computed_style_()
    , layout_info_()
    , needs_layout_(true)
    , needs_paint_(true) {
}

void RenderObject::AppendChild(std::shared_ptr<RenderObject> child) {
    if (!child) {
        return;
    }

    // 从原父节点移除
    if (auto old_parent = child->GetParent()) {
        old_parent->RemoveChild(child);
    }

    children_.push_back(child);
    child->SetParent(shared_from_this());

    MarkNeedsLayout();
    MarkNeedsPaint();
}

void RenderObject::RemoveChild(std::shared_ptr<RenderObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->SetParent(nullptr);
        children_.erase(it);
        MarkNeedsLayout();
        MarkNeedsPaint();
    }
}

void RenderObject::RemoveAllChildren() {
    for (auto& child : children_) {
        child->SetParent(nullptr);
    }

    children_.clear();
    MarkNeedsLayout();
    MarkNeedsPaint();
}

void RenderObject::Layout(float parent_width, float parent_height) {
    // 基类默认实现：简单的块布局
    // 子类应该重写此方法实现具体的布局逻辑
    needs_layout_ = false;
}

void RenderObject::Paint(SkCanvas* canvas) {
    // 基类默认实现：什么都不做
    needs_paint_ = false;
}

void RenderObject::ScrollBy(float dx, float dy) {
    float new_x = scroll_x_ + dx;
    float new_y = scroll_y_ + dy;
    ScrollTo(new_x, new_y);
}

void RenderObject::ScrollTo(float x, float y) {
    // 限制滚动范围
    float max_x = GetMaxScrollX();
    float max_y = GetMaxScrollY();

    scroll_x_ = std::max(0.0f, std::min(x, max_x));
    scroll_y_ = std::max(0.0f, std::min(y, max_y));

    MarkNeedsPaint();
}

bool RenderObject::IsScrollable() const {
    const auto& style = computed_style_;

    // 获取独立的 overflow-x 和 overflow-y 值
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");

    if (!allow_h_scroll && !allow_v_scroll) {
        return false;
    }

    // 检查内容是否超出可见区域（对于 body 元素使用视口尺寸）
    float visible_width = GetEffectiveVisibleWidth();
    float visible_height = GetEffectiveVisibleHeight();

    return (allow_h_scroll && content_width_ > visible_width) ||
           (allow_v_scroll && content_height_ > visible_height);
}

float RenderObject::GetMaxScrollX() const {
    const float scrollbar_width = GetScrollbarWidth();
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 检查是否需要垂直滚动条
    bool needs_v_scroll = content_height_ > effective_height;

    // 可用内容宽度需要减去垂直滚动条宽度
    float available_width = effective_width - (needs_v_scroll ? scrollbar_width : 0);
    return std::max(0.0f, content_width_ - available_width);
}

float RenderObject::GetMaxScrollY() const {
    const float scrollbar_width = GetScrollbarWidth();
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 检查是否需要垂直滚动条（用于计算内容区域宽度）
    bool needs_v_scroll = content_height_ > effective_height;
    float available_width = effective_width - (needs_v_scroll ? scrollbar_width : 0);

    // 检查是否需要水平滚动条
    bool needs_h_scroll = content_width_ > available_width;

    // 可用内容高度需要减去水平滚动条高度
    float available_height = effective_height - (needs_h_scroll ? scrollbar_width : 0);
    return std::max(0.0f, content_height_ - available_height);
}

RenderObject::ScrollbarHitArea RenderObject::HitTestScrollbar(float local_x, float local_y) const {
    const auto& style = computed_style_;

    // 获取独立的 overflow-x 和 overflow-y 值
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");

    // 只有设置了 overflow: scroll 或 auto 才有滚动条
    if (!allow_h_scroll && !allow_v_scroll) {
        return ScrollbarHitArea::None;
    }

    const float scrollbar_width = GetScrollbarWidth();

    // 计算可见区域（对于 body 元素使用视口尺寸）
    float visible_width = GetEffectiveVisibleWidth();
    float visible_height = GetEffectiveVisibleHeight();

    // 使用与 Paint 相同的逻辑判断是否需要滚动条
    bool needs_v_scroll = allow_v_scroll && (content_height_ > visible_height || overflow_y == "scroll");
    float content_area_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
    bool needs_h_scroll = allow_h_scroll && (content_width_ > content_area_width || overflow_x == "scroll");

    // 检测垂直滚动条区域（优先检测，因为它更常见）
    if (needs_v_scroll) {
        float track_x = visible_width - scrollbar_width;
        float track_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);

        if (local_x >= track_x && local_x <= visible_width &&
            local_y >= 0 && local_y <= track_height) {
            return ScrollbarHitArea::VerticalTrack;
        }
    }

    // 检测水平滚动条区域
    if (needs_h_scroll) {
        float track_y = visible_height - scrollbar_width;
        float track_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);

        if (local_y >= track_y && local_y <= visible_height &&
            local_x >= 0 && local_x <= track_width) {
            return ScrollbarHitArea::HorizontalTrack;
        }
    }

    return ScrollbarHitArea::None;
}

void RenderObject::StartScrollbarDrag(ScrollbarHitArea area, float mouse_x, float mouse_y) {
    if (area == ScrollbarHitArea::None) {
        return;
    }

    dragging_scrollbar_ = area;

    if (area == ScrollbarHitArea::HorizontalTrack || area == ScrollbarHitArea::HorizontalThumb) {
        drag_start_scroll_ = scroll_x_;
        drag_start_mouse_ = mouse_x;
    } else {
        drag_start_scroll_ = scroll_y_;
        drag_start_mouse_ = mouse_y;
    }

}

void RenderObject::UpdateScrollbarDrag(float mouse_x, float mouse_y) {
    if (dragging_scrollbar_ == ScrollbarHitArea::None) {
        return;
    }

    const float scrollbar_width = GetScrollbarWidth();

    // 对于 body 元素使用视口尺寸
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 使用与 Paint 相同的逻辑判断滚动条
    bool needs_v_scroll = content_height_ > effective_height;
    float content_area_width = effective_width - (needs_v_scroll ? scrollbar_width : 0);
    bool needs_h_scroll = content_width_ > content_area_width;
    float content_area_height = effective_height - (needs_h_scroll ? scrollbar_width : 0);

    if (dragging_scrollbar_ == ScrollbarHitArea::HorizontalTrack ||
        dragging_scrollbar_ == ScrollbarHitArea::HorizontalThumb) {
        // 水平滚动
        float scrollable_width = content_width_ - content_area_width;

        if (scrollable_width > 0 && content_area_width > 0) {
            // 计算滑块可以移动的轨道长度
            float thumb_ratio = content_area_width / content_width_;
            float thumb_width = std::max(30.0f, content_area_width * thumb_ratio);
            float track_length = content_area_width - thumb_width;

            if (track_length > 0) {
                // 鼠标移动距离转换为滚动距离
                float mouse_delta = mouse_x - drag_start_mouse_;
                float scroll_delta = (mouse_delta / track_length) * scrollable_width;

                scroll_x_ = std::max(0.0f, std::min(drag_start_scroll_ + scroll_delta, scrollable_width));
                MarkNeedsPaint();
            }
        }
    } else {
        // 垂直滚动
        float scrollable_height = content_height_ - content_area_height;

        if (scrollable_height > 0 && content_area_height > 0) {
            // 计算滑块可以移动的轨道长度
            float thumb_ratio = content_area_height / content_height_;
            float thumb_height = std::max(30.0f, content_area_height * thumb_ratio);
            float track_length = content_area_height - thumb_height;

            if (track_length > 0) {
                // 鼠标移动距离转换为滚动距离
                float mouse_delta = mouse_y - drag_start_mouse_;
                float scroll_delta = (mouse_delta / track_length) * scrollable_height;

                scroll_y_ = std::max(0.0f, std::min(drag_start_scroll_ + scroll_delta, scrollable_height));
                MarkNeedsPaint();
            }
        }
    }
}

void RenderObject::EndScrollbarDrag() {
    dragging_scrollbar_ = ScrollbarHitArea::None;
    drag_start_scroll_ = 0;
    drag_start_mouse_ = 0;
}

float RenderObject::CalculateContentHeight() const {
    float max_height = 0.0f;

    // 辅助函数：检查是否设置了 overflow 隐藏/滚动
    auto hasOverflowClip = [](const ComputedStyle& s) {
        std::string oy = !s.overflow_y.empty() ? s.overflow_y : s.overflow;
        return oy == "scroll" || oy == "auto" || oy == "hidden";
    };

    for (const auto& child : children_) {
        const auto& child_layout = child->GetLayoutInfo();
        const auto& child_style = child->GetComputedStyle();

        // 获取子元素的位置和高度
        float child_y = child_layout.y;
        float child_height = child_layout.height;

        // 如果子元素也有 overflow: scroll/auto/hidden，使用其布局高度
        // 否则，递归计算其内容高度
        if (!hasOverflowClip(child_style)) {
            // 递归计算子元素的内容高度
            float child_content_height = child->CalculateContentHeight();
            if (child_content_height > child_height) {
                child_height = child_content_height;
            }
        }

        max_height = std::max(max_height, child_y + child_height);
    }

    return max_height;
}

float RenderObject::CalculateContentWidth() const {
    float max_width = 0.0f;

    // 辅助函数：检查是否设置了 overflow 隐藏/滚动
    auto hasOverflowClip = [](const ComputedStyle& s) {
        std::string ox = !s.overflow_x.empty() ? s.overflow_x : s.overflow;
        return ox == "scroll" || ox == "auto" || ox == "hidden";
    };

    for (const auto& child : children_) {
        const auto& child_layout = child->GetLayoutInfo();
        const auto& child_style = child->GetComputedStyle();

        // 获取子元素的位置和宽度
        float child_x = child_layout.x;
        float child_width = child_layout.width;

        // 如果子元素也有 overflow: scroll/auto/hidden，使用其布局宽度
        // 否则，递归计算其内容宽度
        if (!hasOverflowClip(child_style)) {
            // 递归计算子元素的内容宽度
            float child_content_width = child->CalculateContentWidth();
            if (child_content_width > child_width) {
                child_width = child_content_width;
            }
        }

        max_width = std::max(max_width, child_x + child_width);
    }

    return max_width;
}

// ========== RenderBlock 实现 ==========

void RenderBlock::Layout(float parent_width, float parent_height) {
    // 使用传统块布局
    const auto& style = computed_style_;
    
    // 计算宽度
    float width = parent_width;
    if (!style.width.IsAuto()) {
        width = style.width.ToPx(parent_width, style.font_size);
    }
    
    // 应用 min-width 和 max-width
    if (!style.min_width.IsZero()) {
        float min_w = style.min_width.ToPx(parent_width, style.font_size);
        width = std::max(width, min_w);
    }
    if (style.max_width.unit != CSSUnit::NONE) {
        float max_w = style.max_width.ToPx(parent_width, style.font_size);
        width = std::min(width, max_w);
    }
    
    // 计算 padding
    float padding_left = style.padding.left.ToPx(width, style.font_size);
    float padding_right = style.padding.right.ToPx(width, style.font_size);
    float padding_top = style.padding.top.ToPx(width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(width, style.font_size);
    
    // 计算 border
    float border_left = style.border.width.ToPx();
    float border_right = style.border.width.ToPx();
    float border_top = style.border.width.ToPx();
    float border_bottom = style.border.width.ToPx();
    
    // 计算内容区域宽度
    float content_width = width - padding_left - padding_right - border_left - border_right;
    
    // 布局子元素 - 第一遍：计算尺寸
    for (auto& child : children_) {
        if (child->NeedsLayout()) {
            child->Layout(content_width, 0);
        }
    }

    // 布局子元素 - 第二遍：设置位置
    // 支持内联元素水平排列和块级元素垂直排列
    float current_y = 0;
    float current_x = padding_left + border_left;
    float line_height = 0;  // 当前行的高度

    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        auto& child_style = child->GetComputedStyle();

        // 计算子元素的 margin
        float child_margin_top = child_style.margin.top.ToPx(width, child_style.font_size);
        float child_margin_bottom = child_style.margin.bottom.ToPx(width, child_style.font_size);
        float child_margin_left = child_style.margin.left.ToPx(width, child_style.font_size);
        float child_margin_right = child_style.margin.right.ToPx(width, child_style.font_size);

        // 判断是块级还是内联元素
        // 检查渲染对象的实际类型，而不是 display 属性
        bool is_inline = (dynamic_cast<RenderInline*>(child.get()) != nullptr ||
                         dynamic_cast<RenderText*>(child.get()) != nullptr);

        if (is_inline) {
            // 内联元素：水平排列
            float child_width = child_layout.width + child_margin_left + child_margin_right;

            // 检查是否需要换行
            if (current_x + child_width > width - padding_right - border_right && current_x > padding_left + border_left) {
                // 换行
                current_y += line_height;
                current_x = padding_left + border_left;
                line_height = 0;
            }

            // 设置位置
            child_layout.x = current_x + child_margin_left;
            child_layout.y = current_y + padding_top + border_top + child_margin_top;

            // 更新当前X位置和行高
            current_x += child_width;
            line_height = std::max(line_height, child_layout.height + child_margin_top + child_margin_bottom);
        } else {
            // 块级元素：垂直排列
            // 如果当前行有内联元素，先完成当前行
            if (current_x > padding_left + border_left) {
                current_y += line_height;
                current_x = padding_left + border_left;
                line_height = 0;
            }

            // 计算X位置（考虑 text-align）
            float new_x = padding_left + border_left + child_margin_left;

            if (style.text_align == "center") {
                float available_width = content_width - child_margin_left - child_margin_right;
                float child_width = child_layout.width;
                if (child_width < available_width) {
                    new_x = padding_left + border_left + (available_width - child_width) / 2.0f;
                }
            } else if (style.text_align == "right") {
                float available_width = content_width - child_margin_left - child_margin_right;
                float child_width = child_layout.width;
                if (child_width < available_width) {
                    new_x = padding_left + border_left + available_width - child_width - child_margin_right;
                }
            }

            float new_y = current_y + padding_top + border_top + child_margin_top;

            child_layout.x = new_x;
            child_layout.y = new_y;

            // 累加高度
            current_y += child_margin_top + child_layout.height + child_margin_bottom;
        }
    }

    // 如果最后一行有内联元素，完成最后一行
    if (current_x > padding_left + border_left) {
        current_y += line_height;
    }
    
    // 计算高度
    float height = 0;
    if (!style.height.IsAuto()) {
        height = style.height.ToPx(parent_height, style.font_size);
    } else {
        height = current_y + padding_top + padding_bottom + border_top + border_bottom;
    }
    
    // 应用 min-height 和 max-height
    if (!style.min_height.IsZero()) {
        float min_h = style.min_height.ToPx(parent_height, style.font_size);
        height = std::max(height, min_h);
    }
    if (style.max_height.unit != CSSUnit::NONE) {
        float max_h = style.max_height.ToPx(parent_height, style.font_size);
        height = std::min(height, max_h);
    }
    
    // 设置布局信息
    layout_info_.width = width;
    layout_info_.height = height;
    
    layout_info_.content_rect = SkRect::MakeXYWH(
        padding_left + border_left,
        padding_top + border_top,
        content_width,
        current_y
    );
    
    layout_info_.padding_rect = SkRect::MakeXYWH(
        border_left,
        border_top,
        content_width + padding_left + padding_right,
        current_y + padding_top + padding_bottom
    );
    
    layout_info_.border_rect = SkRect::MakeXYWH(
        0, 0, width, height
    );
    
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderBlock::Paint(SkCanvas* canvas) {
    if (!canvas) {
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // DEBUG: 打印每个有 max-height 或 overflow 的元素
    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 创建盒模型
    Box box;

    // 计算 padding
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);

    // 计算 border - 优先使用单边边框宽度，否则使用统一的 border.width
    box.border_top_width = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    box.border_right_width = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    box.border_bottom_width = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();
    box.border_left_width = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();

    // Taffy 返回的是 border-box 尺寸
    // 由于我们已经 translate 到元素左上角（border-box 的左上角）
    // content_x 和 content_y 应该从 border + padding 开始
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.border_left_width - box.border_right_width
                        - box.padding_left - box.padding_right;
    box.content_height = layout.height - box.border_top_width - box.border_bottom_width
                         - box.padding_top - box.padding_bottom;

    // 创建样式映射
    std::unordered_map<std::string, std::string> styles;
    if (!style.background_color.empty()) {
        styles["background-color"] = style.background_color;
    }
    if (!style.background_image.empty()) {
        styles["background-image"] = style.background_image;
    }

    // 渲染器
    BoxRenderer renderer(canvas);

    // 检查是否是 <hr> 元素
    auto node = GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() == "hr") {
            // 绘制水平线
            SkPaint line_paint;
            line_paint.setColor(style.border.color);
            line_paint.setStrokeWidth(style.border.width.ToPx());
            line_paint.setAntiAlias(true);

            float y = layout_info_.y + layout_info_.height / 2;
            canvas->drawLine(
                layout_info_.x, y,
                layout_info_.x + layout_info_.width, y,
                line_paint
            );
            return; // 不绘制其他内容
        }
    }

    // 渲染阴影
    if (!style.box_shadow.empty()) {
        renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);
    }

    // 渲染背景（优先渐变，然后纯色）
    SkRect padding_box = box.GetPaddingBox();
    if (style.background_linear_gradient.has_value()) {
        GradientRenderer::RenderLinearGradient(canvas, padding_box, *style.background_linear_gradient);
    }
    else if (style.background_radial_gradient.has_value()) {
        GradientRenderer::RenderRadialGradient(canvas, padding_box, *style.background_radial_gradient);
    }
    else {
        renderer.RenderBackgroundAdvanced(box, styles, &style.border_radius);
    }

    // 渲染边框 - 支持单边边框
    bool has_any_border = (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) ||
                          (style.border_left_width > 0 && style.border_left_style != CSSBorderStyle::NONE) ||
                          (style.border_right_width > 0 && style.border_right_style != CSSBorderStyle::NONE) ||
                          (style.border_top_width > 0 && style.border_top_style != CSSBorderStyle::NONE) ||
                          (style.border_bottom_width > 0 && style.border_bottom_style != CSSBorderStyle::NONE);



    if (has_any_border) {
        SkRect border_box = box.GetBorderBox();

        // 边框绘制时需要向内偏移半个边框宽度
        // 因为 Skia 的线条是以指定坐标为中心绘制的
        float half_left = box.border_left_width / 2.0f;
        float half_right = box.border_right_width / 2.0f;
        float half_top = box.border_top_width / 2.0f;
        float half_bottom = box.border_bottom_width / 2.0f;

        // 渲染左边框
        if (box.border_left_width > 0) {
            CSSBorderStyle left_style = style.border_left_style != CSSBorderStyle::NONE ?
                                        style.border_left_style : style.border.style;
            SkColor left_color = style.border_left_style != CSSBorderStyle::NONE ?
                                 style.border_left_color : style.border.color;
            if (left_style != CSSBorderStyle::NONE) {
                renderer.RenderBorderEdge(
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
                renderer.RenderBorderEdge(
                    border_box.right() - half_right, border_box.top(),
                    border_box.right() - half_right, border_box.bottom(),
                    box.border_right_width, right_style, right_color
                );
            }
        }

        // 渲染上边框
        if (box.border_top_width > 0) {
            CSSBorderStyle top_style = style.border_top_style != CSSBorderStyle::NONE ?
                                       style.border_top_style : style.border.style;
            SkColor top_color = style.border_top_style != CSSBorderStyle::NONE ?
                                style.border_top_color : style.border.color;
            if (top_style != CSSBorderStyle::NONE) {
                renderer.RenderBorderEdge(
                    border_box.left(), border_box.top() + half_top,
                    border_box.right(), border_box.top() + half_top,
                    box.border_top_width, top_style, top_color
                );
            }
        }

        // 渲染下边框
        if (box.border_bottom_width > 0) {
            CSSBorderStyle bottom_style = style.border_bottom_style != CSSBorderStyle::NONE ?
                                          style.border_bottom_style : style.border.style;
            SkColor bottom_color = style.border_bottom_style != CSSBorderStyle::NONE ?
                                   style.border_bottom_color : style.border.color;
            if (bottom_style != CSSBorderStyle::NONE) {
                renderer.RenderBorderEdge(
                    border_box.left(), border_box.bottom() - half_bottom,
                    border_box.right(), border_box.bottom() - half_bottom,
                    box.border_bottom_width, bottom_style, bottom_color
                );
            }
        }
    }

    // ========== 绘制 outline（焦点指示器）==========
    // outline 不占用布局空间，绘制在边框外部
    if (style.outline_style != "none" && !style.outline_width.IsZero()) {
        float outline_width = style.outline_width.ToPx();
        float outline_offset = style.outline_offset.ToPx();

        // outline 绘制在边框外部
        SkRect outline_rect = SkRect::MakeXYWH(
            -outline_offset - outline_width,
            -outline_offset - outline_width,
            layout.width + 2 * (outline_offset + outline_width),
            layout.height + 2 * (outline_offset + outline_width)
        );

        SkPaint outline_paint;
        outline_paint.setColor(style.outline_color);
        outline_paint.setStyle(SkPaint::kStroke_Style);
        outline_paint.setStrokeWidth(outline_width);
        outline_paint.setAntiAlias(true);

        // 根据 outline_style 设置线条样式
        if (style.outline_style == "dashed") {
            const SkScalar intervals[] = {6.0f, 3.0f};
            outline_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        } else if (style.outline_style == "dotted") {
            const SkScalar intervals[] = {2.0f, 2.0f};
            outline_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        }
        // solid 不需要特殊处理

        // 如果有圆角，outline 也应该有圆角
        if (style.border_radius.top_left.value > 0 || style.border_radius.top_right.value > 0 ||
            style.border_radius.bottom_left.value > 0 || style.border_radius.bottom_right.value > 0) {
            float tl = style.border_radius.top_left.ToPx() + outline_offset + outline_width;
            float tr = style.border_radius.top_right.ToPx() + outline_offset + outline_width;
            float br = style.border_radius.bottom_right.ToPx() + outline_offset + outline_width;
            float bl = style.border_radius.bottom_left.ToPx() + outline_offset + outline_width;

            SkRRect outline_rrect;
            SkVector radii[4] = {{tl, tl}, {tr, tr}, {br, br}, {bl, bl}};
            outline_rrect.setRectRadii(outline_rect, radii);
            canvas->drawRRect(outline_rrect, outline_paint);
        } else {
            canvas->drawRect(outline_rect, outline_paint);
        }
    }

    // 绘制列表项目符号（如果是<li>元素）
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element2 = std::static_pointer_cast<Element>(node);
        if (element2->GetTagName() == "li") {
            // 获取父元素（ul或ol）
            auto parent_node = element2->GetParentNode();
            if (parent_node && parent_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto parent_element = std::static_pointer_cast<Element>(parent_node);
                std::string parent_tag = parent_element->GetTagName();

                if (parent_tag == "ul" || parent_tag == "ol") {
                    // 设置文本样式
                    SkFont font;
                    font.setSize(style.font_size);

                    SkPaint paint;
                    if (!style.color.empty()) {
                        paint.setColor(Color::Parse(style.color));
                    } else {
                        paint.setColor(SK_ColorBLACK);
                    }
                    paint.setAntiAlias(true);

                    // 计算项目符号位置（在padding区域的左侧）
                    float marker_x = box.content_x - 20.0f;  // 在内容左侧20px处
                    float marker_y = box.content_y + style.font_size * 0.8f;  // 第一行文本的基线位置

                    if (parent_tag == "ul") {
                        // 无序列表：绘制圆点
                        float bullet_radius = 3.0f;
                        float bullet_x = marker_x;
                        float bullet_y = marker_y - style.font_size * 0.3f;
                        canvas->drawCircle(bullet_x, bullet_y, bullet_radius, paint);
                    } else if (parent_tag == "ol") {
                        // 有序列表：绘制数字
                        // 计算当前<li>在<ol>中的索引
                        int index = 1;
                        auto siblings = parent_element->GetChildNodes();
                        for (const auto& sibling : siblings) {
                            if (sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
                                auto sibling_elem = std::static_pointer_cast<Element>(sibling);
                                if (sibling_elem->GetTagName() == "li") {
                                    if (sibling_elem == element2) {
                                        break;
                                    }
                                    index++;
                                }
                            }
                        }

                        // 绘制数字
                        std::string marker_text = std::to_string(index) + ".";
                        canvas->drawString(marker_text.c_str(), marker_x - 15.0f, marker_y, font, paint);
                    }
                }
            }
        }
    }

    // 渲染表单控件特定内容
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        // 直接从node进行dynamic_cast，保留类型信息
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            PaintInputElement(canvas, input_element.get(), box);
        }

        // 渲染 textarea 元素
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            PaintTextAreaElement(canvas, textarea_element.get(), box);
        }
    }

    // 应用 overflow 裁剪
    bool needs_clip = false;
    bool needs_scrollbar = false;
    float content_width = 0, content_height = 0;
    const float scrollbar_width = 12.0f;

    // 获取独立的 overflow-x 和 overflow-y 值
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    // 检查任一方向是否有 overflow 属性
    auto isOverflowSet = [](const std::string& v) {
        return v == "hidden" || v == "scroll" || v == "auto";
    };

    if (isOverflowSet(overflow_x) || isOverflowSet(overflow_y)) {
        needs_clip = true;

        // 使用递归方法计算子元素内容的实际尺寸
        content_width = CalculateContentWidth();
        content_height = CalculateContentHeight();

        // 保存内容尺寸用于滚动计算
        content_width_ = content_width;
        content_height_ = content_height;

        // 判断是否需要滚动条
        float effective_width = GetEffectiveVisibleWidth();
        float effective_height = GetEffectiveVisibleHeight();
        float visible_width = effective_width - box.border_left_width - box.border_right_width;
        float visible_height = effective_height - box.border_top_width - box.border_bottom_width;



        bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");
        bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");

        bool needs_v_scroll = allow_v_scroll && (content_height > visible_height || overflow_y == "scroll");

        float content_area_width = visible_width;
        if (needs_v_scroll) {
            content_area_width -= scrollbar_width;
        }

        bool needs_h_scroll = allow_h_scroll && (content_width > content_area_width || overflow_x == "scroll");

        float content_area_height = visible_height;
        if (needs_h_scroll) {
            content_area_height -= scrollbar_width;
            if (allow_v_scroll && !needs_v_scroll && content_height > content_area_height) {
                needs_v_scroll = true;
                content_area_width = visible_width - scrollbar_width;
                needs_h_scroll = allow_h_scroll && content_width > content_area_width;
            }
        }

        needs_scrollbar = needs_h_scroll || needs_v_scroll;

        float clip_width = content_area_width;
        float clip_height = content_area_height;

        SkRect clip_rect = SkRect::MakeXYWH(
            box.border_left_width,
            box.border_top_width,
            clip_width,
            clip_height
        );
        canvas->save();
        canvas->clipRect(clip_rect, SkClipOp::kIntersect, true);

        // 应用滚动偏移
        canvas->translate(-scroll_x_, -scroll_y_);
    }

    // 按 z-index 排序子元素
    std::vector<std::shared_ptr<RenderObject>> sorted_children = children_;
    std::sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            return a->GetComputedStyle().z_index < b->GetComputedStyle().z_index;
        });

    // 浏览器行为：当元素有 border-radius 时，子元素会被裁剪到圆角区域内
    // 即使没有设置 overflow: hidden
    bool has_border_radius = style.border_radius.top_left.value > 0 ||
                             style.border_radius.top_right.value > 0 ||
                             style.border_radius.bottom_left.value > 0 ||
                             style.border_radius.bottom_right.value > 0;

    bool needs_radius_clip = has_border_radius && !needs_clip;
    if (needs_radius_clip) {
        canvas->save();
        SkRect clip_rect = box.GetPaddingBox();
        SkRRect rrect;
        float tl = style.border_radius.top_left.ToPx();
        float tr = style.border_radius.top_right.ToPx();
        float br = style.border_radius.bottom_right.ToPx();
        float bl = style.border_radius.bottom_left.ToPx();
        SkVector radii[4] = {
            {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
        };
        rrect.setRectRadii(clip_rect, radii);
        canvas->clipRRect(rrect, SkClipOp::kIntersect, true);
    }

    // 绘制子元素
    for (auto& child : sorted_children) {
        child->Paint(canvas);
    }

    // 恢复圆角裁剪状态
    if (needs_radius_clip) {
        canvas->restore();
    }

    // 恢复 overflow 裁剪状态和滚动偏移
    if (needs_clip) {
        canvas->restore();
    }

    // 绘制滚动条 (在裁剪区域外绘制)
    if (needs_scrollbar) {
        // 对于 body 元素使用视口尺寸
        float effective_width = GetEffectiveVisibleWidth();
        float effective_height = GetEffectiveVisibleHeight();
        float visible_width = effective_width - box.border_left_width - box.border_right_width;
        float visible_height = effective_height - box.border_top_width - box.border_bottom_width;

        // 获取独立的 overflow-x 和 overflow-y 值
        std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
        std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

        // 判断是否允许显示滚动条
        bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");
        bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");

        // 使用与上面相同的逻辑判断是否需要滚动条
        bool needs_v_scroll = allow_v_scroll && (content_height > visible_height || overflow_y == "scroll");
        float content_area_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
        bool needs_h_scroll = allow_h_scroll && (content_width > content_area_width || overflow_x == "scroll");

        // 如果需要水平滚动条，调整高度并重新检查
        if (needs_h_scroll) {
            float content_area_height = visible_height - scrollbar_width;
            if (allow_v_scroll && !needs_v_scroll && content_height > content_area_height) {
                needs_v_scroll = true;
            }
        }

        const float scrollbar_margin = 2.0f;
        const float corner_radius = 4.0f;

        // 滚动条轨道颜色 (更接近浏览器的浅灰色)
        SkPaint track_paint;
        track_paint.setColor(SkColorSetRGB(241, 241, 241));
        track_paint.setAntiAlias(true);

        // 滚动条滑块颜色 (深灰色)
        SkPaint thumb_paint;
        thumb_paint.setColor(SkColorSetRGB(193, 193, 193));
        thumb_paint.setAntiAlias(true);

        // 滚动条区域尺寸
        float scrollbar_area_width = effective_width;
        float scrollbar_area_height = effective_height;

        // 绘制水平滚动条
        if (needs_h_scroll) {
            float track_x = box.border_left_width;
            float track_y = scrollbar_area_height - box.border_bottom_width - scrollbar_width;
            float track_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);

            // 绘制轨道
            SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, track_width, scrollbar_width);
            canvas->drawRect(track_rect, track_paint);

            // 计算滑块尺寸和位置 - 使用减去滚动条后的可用宽度
            float available_content_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
            float scrollable_width = content_width - available_content_width;
            float thumb_ratio = available_content_width / content_width;
            float thumb_width = std::max(30.0f, (track_width - 2 * scrollbar_margin) * thumb_ratio);
            float available_track = track_width - thumb_width - 2 * scrollbar_margin;
            float scroll_ratio = scrollable_width > 0 ? scroll_x_ / scrollable_width : 0;
            float thumb_x = track_x + scrollbar_margin + available_track * scroll_ratio;

            SkRect thumb_rect = SkRect::MakeXYWH(
                thumb_x,
                track_y + scrollbar_margin,
                thumb_width,
                scrollbar_width - 2 * scrollbar_margin
            );
            canvas->drawRoundRect(thumb_rect, corner_radius, corner_radius, thumb_paint);
        }

        // 绘制垂直滚动条
        if (needs_v_scroll) {
            float track_x = scrollbar_area_width - box.border_right_width - scrollbar_width;
            float track_y = box.border_top_width;
            float track_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);

            // 绘制轨道
            SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height);
            canvas->drawRect(track_rect, track_paint);

            // 计算滑块尺寸和位置 - 使用减去滚动条后的可用高度
            float available_content_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);
            float scrollable_height = content_height - available_content_height;
            float thumb_ratio = available_content_height / content_height;
            float thumb_height = std::max(30.0f, (track_height - 2 * scrollbar_margin) * thumb_ratio);
            float available_track = track_height - thumb_height - 2 * scrollbar_margin;
            float scroll_ratio = scrollable_height > 0 ? scroll_y_ / scrollable_height : 0;
            float thumb_y = track_y + scrollbar_margin + available_track * scroll_ratio;

            SkRect thumb_rect = SkRect::MakeXYWH(
                track_x + scrollbar_margin,
                thumb_y,
                scrollbar_width - 2 * scrollbar_margin,
                thumb_height
            );
            canvas->drawRoundRect(thumb_rect, corner_radius, corner_radius, thumb_paint);
        }

        // 绘制滚动条角落（当两个滚动条都存在时）
        if (needs_h_scroll && needs_v_scroll) {
            float corner_x = scrollbar_area_width - box.border_right_width - scrollbar_width;
            float corner_y = scrollbar_area_height - box.border_bottom_width - scrollbar_width;
            SkRect corner_rect = SkRect::MakeXYWH(corner_x, corner_y, scrollbar_width, scrollbar_width);
            canvas->drawRect(corner_rect, track_paint);
        }
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

void RenderBlock::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    if (!input) return;

    InputType type = input->GetInputType();

    // 处理文本类型的input
    if (type == InputType::Text || type == InputType::Password ||
        type == InputType::Email || type == InputType::Tel ||
        type == InputType::Url || type == InputType::Search ||
        type == InputType::Number) {

        std::string value = input->GetValue();
        std::string display_text = value;

        // 如果是密码类型，显示为星号
        if (type == InputType::Password && !value.empty()) {
            display_text = std::string(value.length(), '*');
        }

        // 如果值为空，显示placeholder
        if (value.empty()) {
            display_text = input->GetPlaceholder();
        }

        if (!display_text.empty()) {
            // 创建字体
            FontDescriptor desc;
            desc.family = computed_style_.font_family;
            desc.size = computed_style_.font_size;
            desc.weight = FontWeight::NORMAL;
            desc.style = FontStyle::NORMAL;

            SkFont font = FontManager::GetInstance().LoadFont(desc);

            // 获取字体度量信息
            SkFontMetrics font_metrics;
            font.getMetrics(&font_metrics);

            // 计算文本位置（左对齐，垂直居中）
            float text_x = box.content_x;
            float text_y = box.content_y + (box.content_height - font_metrics.fDescent + font_metrics.fAscent) / 2 - font_metrics.fAscent;

            // 创建文本渲染器
            TextRenderer text_renderer(canvas);

            // 设置文本颜色
            lightui::Paint text_paint;
            if (value.empty()) {
                // placeholder使用灰色
                text_paint.SetColor(SkColorSetRGB(150, 150, 150));
            } else if (!computed_style_.color.empty()) {
                text_paint.SetColor(lightui::Color::Parse(computed_style_.color));
            } else {
                text_paint.SetColor(SK_ColorBLACK);
            }

            // 绘制文本
            text_renderer.DrawText(display_text, text_x, text_y, font, text_paint);

            // 如果有焦点，绘制选中高亮和光标
            auto element = std::static_pointer_cast<Element>(GetNode());
            if (element && element->HasPseudoClass("focus")) {
                int sel_start = input->GetSelectionStart();
                int sel_end = input->GetSelectionEnd();

                // 绘制选中区域高亮
                if (sel_start != sel_end) {
                    int start_char = std::min(sel_start, sel_end);
                    int end_char = std::max(sel_start, sel_end);

                    // 使用 UTF-8 工具计算字节位置
                    size_t start_byte = utf8::CharPosToBytePos(value, start_char);
                    size_t end_byte = utf8::CharPosToBytePos(value, end_char);

                    std::string text_before_sel = value.substr(0, start_byte);
                    std::string selected_text = value.substr(start_byte, end_byte - start_byte);

                    // 如果是密码类型，使用星号
                    if (type == InputType::Password) {
                        text_before_sel = std::string(start_char, '*');
                        selected_text = std::string(end_char - start_char, '*');
                    }

                    float sel_start_x = text_x;
                    if (start_char > 0) {
                        sel_start_x += font.measureText(text_before_sel.c_str(), text_before_sel.length(), SkTextEncoding::kUTF8);
                    }
                    float sel_width = font.measureText(selected_text.c_str(), selected_text.length(), SkTextEncoding::kUTF8);

                    // 绘制选中背景
                    SkPaint sel_paint;
                    sel_paint.setColor(SkColorSetARGB(128, 51, 153, 255));  // 半透明蓝色
                    sel_paint.setStyle(SkPaint::kFill_Style);

                    canvas->drawRect(SkRect::MakeXYWH(sel_start_x, box.content_y, sel_width, box.content_height), sel_paint);
                }

                // 基于时间的光标闪烁：每500毫秒切换一次
                auto now = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                bool cursor_visible = (ms / 500) % 2 == 0;

                if (cursor_visible) {
                    // 计算光标位置 - 使用 UTF-8 字符位置转换为字节位置
                    int cursor_pos = sel_end;  // 使用 selection_end 作为光标位置
                    size_t cursor_byte_pos = utf8::CharPosToBytePos(value, cursor_pos);
                    std::string text_before_cursor = value.substr(0, cursor_byte_pos);

                    // 如果是密码类型，使用星号计算宽度
                    if (type == InputType::Password) {
                        text_before_cursor = std::string(cursor_pos, '*');
                    }

                    // 测量光标前的文本宽度
                    float cursor_x = text_x;
                    if (cursor_pos > 0) {
                        cursor_x += font.measureText(
                            text_before_cursor.c_str(),
                            text_before_cursor.length(),
                            SkTextEncoding::kUTF8
                        );
                    }

                    // 计算光标的Y坐标（从文本顶部到底部）
                    float cursor_y_top = box.content_y;
                    float cursor_y_bottom = box.content_y + box.content_height;

                    // 绘制光标
                    SkPaint cursor_paint;
                    cursor_paint.setColor(SK_ColorBLACK);
                    cursor_paint.setStrokeWidth(1);
                    cursor_paint.setAntiAlias(true);

                    canvas->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
                }
            }
        }
    }
    // 处理checkbox和radio类型
    else if (type == InputType::Checkbox || type == InputType::Radio) {
        bool checked = input->GetChecked();

        // 无条件绘制标记用于测试
        if (type == InputType::Checkbox) {
            // 绘制勾选标记（✓）
            SkPaint check_paint;
            check_paint.setColor(checked ? SK_ColorBLACK : SkColorSetRGB(200, 200, 200));
            check_paint.setStrokeWidth(2);
            check_paint.setStyle(SkPaint::kStroke_Style);
            check_paint.setAntiAlias(true);

            float cx = box.content_x + box.content_width / 2;
            float cy = box.content_y + box.content_height / 2;

            SkPath check_path;
            check_path.moveTo(cx - 4, cy);
            check_path.lineTo(cx - 1, cy + 3);
            check_path.lineTo(cx + 4, cy - 3);
            canvas->drawPath(check_path, check_paint);
        }
        else if (type == InputType::Radio) {
            // 绘制圆点标记
            SkPaint dot_paint;
            dot_paint.setColor(checked ? SK_ColorBLACK : SkColorSetRGB(200, 200, 200));
            dot_paint.setStyle(SkPaint::kFill_Style);
            dot_paint.setAntiAlias(true);

            float cx = box.content_x + box.content_width / 2;
            float cy = box.content_y + box.content_height / 2;
            float radius = std::min(box.content_width, box.content_height) / 4;

            canvas->drawCircle(cx, cy, radius, dot_paint);
        }
    }
}

void RenderBlock::PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box) {
    if (!textarea) return;

    std::string value = textarea->GetValue();

    if (value.empty()) {
        // 显示placeholder
        value = textarea->GetPlaceholder();
    }

    if (!value.empty()) {
        // 创建字体
        FontDescriptor desc;
        desc.family = computed_style_.font_family;
        desc.size = computed_style_.font_size;
        desc.weight = FontWeight::NORMAL;
        desc.style = FontStyle::NORMAL;

        SkFont font = FontManager::GetInstance().LoadFont(desc);

        // 获取字体度量信息
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        float line_height = -font_metrics.fAscent + font_metrics.fDescent + font_metrics.fLeading;

        // 创建文本渲染器
        TextRenderer text_renderer(canvas);

        // 设置文本颜色
        lightui::Paint text_paint;
        if (textarea->GetValue().empty()) {
            // placeholder使用灰色
            text_paint.SetColor(SkColorSetRGB(150, 150, 150));
        } else if (!computed_style_.color.empty()) {
            text_paint.SetColor(lightui::Color::Parse(computed_style_.color));
        } else {
            text_paint.SetColor(SK_ColorBLACK);
        }

        // 绘制多行文本
        float text_x = box.content_x;
        float text_y = box.content_y - font_metrics.fAscent;

        text_renderer.DrawMultilineText(value, text_x, text_y, box.content_width, line_height, font, text_paint);

        // 如果有焦点且不是placeholder，绘制光标
        if (!textarea->GetValue().empty()) {
            auto element = std::static_pointer_cast<Element>(GetNode());
            if (element && element->HasPseudoClass("focus")) {
                // 简化版本：只在第一行显示光标
                int cursor_pos = textarea->GetSelectionStart();
                std::string text_before_cursor = textarea->GetValue().substr(0, cursor_pos);

                // 找到最后一个换行符的位置
                size_t last_newline = text_before_cursor.rfind('\n');
                std::string current_line_before_cursor;
                float cursor_y = text_y;

                if (last_newline != std::string::npos) {
                    // 光标在某一行中
                    current_line_before_cursor = text_before_cursor.substr(last_newline + 1);
                    // 计算光标所在行（简化：每个\n增加一行）
                    int line_count = std::count(text_before_cursor.begin(), text_before_cursor.end(), '\n');
                    cursor_y += line_count * line_height;
                } else {
                    // 光标在第一行
                    current_line_before_cursor = text_before_cursor;
                }

                // 测量光标前的文本宽度
                float cursor_x = text_x;
                if (!current_line_before_cursor.empty()) {
                    cursor_x += font.measureText(
                        current_line_before_cursor.c_str(),
                        current_line_before_cursor.length(),
                        SkTextEncoding::kUTF8
                    );
                }

                // 绘制光标
                SkPaint cursor_paint;
                cursor_paint.setColor(SK_ColorBLACK);
                cursor_paint.setStrokeWidth(1);
                cursor_paint.setAntiAlias(true);

                canvas->drawLine(cursor_x, cursor_y + font_metrics.fAscent,
                               cursor_x, cursor_y + font_metrics.fDescent, cursor_paint);
            }
        }
    }
}

// ========== RenderInline 实现 ==========

void RenderInline::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 首先检查是否有显式的width/height设置（例如input元素）
    float explicit_width = 0;
    float explicit_height = 0;
    bool has_explicit_width = false;
    bool has_explicit_height = false;

    if (style.width.unit != CSSUnit::NONE) {
        explicit_width = style.width.ToPx(parent_width, style.font_size);
        has_explicit_width = true;
    }

    if (style.height.unit != CSSUnit::NONE) {
        explicit_height = style.height.ToPx(parent_height, style.font_size);
        has_explicit_height = true;
    }

    // 内联元素布局：计算所有子元素的总宽度和最大高度
    float total_width = 0;
    float max_height = 0;

    // 布局所有子元素
    for (auto& child : children_) {
        if (child->NeedsLayout()) {
            child->Layout(parent_width, parent_height);
        }

        auto& child_layout = child->GetLayoutInfo();
        total_width += child_layout.width;
        max_height = std::max(max_height, child_layout.height);
    }

    // 计算 padding
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_height, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_height, style.font_size);

    // 设置内联元素的尺寸
    // 如果有显式宽度/高度，使用显式值；否则使用子元素计算的值 + padding
    layout_info_.width = has_explicit_width ? explicit_width : (total_width + padding_left + padding_right);
    layout_info_.height = has_explicit_height ? explicit_height : (max_height > 0 ? max_height + padding_top + padding_bottom : 20.0f);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;

    // 计算内容区域宽度（不包括 padding）
    float content_width = layout_info_.width - padding_left - padding_right;

    // 设置子元素的位置（水平排列，支持 text-align）
    float start_x = padding_left;

    // 处理 text-align
    if (style.text_align == "center" && total_width < content_width) {
        // 居中对齐：计算起始偏移
        start_x = padding_left + (content_width - total_width) / 2.0f;
    } else if (style.text_align == "right" && total_width < content_width) {
        // 右对齐
        start_x = padding_left + content_width - total_width;
    }

    float current_x = start_x;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        child_layout.x = current_x;
        // 垂直居中：如果父元素高度大于子元素高度，则居中对齐
        child_layout.y = padding_top + (layout_info_.height - padding_top - padding_bottom - child_layout.height) / 2.0f;
        current_x += child_layout.width;
    }
}

std::pair<float, float> RenderInline::MeasureIntrinsicSize(float available_width) {
    const auto& style = computed_style_;

    // 计算 padding
    float padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float padding_right = style.padding.right.ToPx(available_width, style.font_size);
    float padding_top = style.padding.top.ToPx(available_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(available_width, style.font_size);

    // 计算 border（单边边框宽度是 float 类型）
    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;

    // 如果没有单独的边框宽度，使用通用边框
    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(available_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }

    // 检查是否有显式宽高
    bool has_explicit_width = false;
    bool has_explicit_height = false;
    float explicit_width = 0.0f;
    float explicit_height = 0.0f;

    if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO) {
        explicit_width = style.width.ToPx(available_width, style.font_size);
        has_explicit_width = true;
    }

    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
        explicit_height = style.height.ToPx(available_width, style.font_size);
        has_explicit_height = true;
    }

    // 计算子元素的尺寸
    float total_width = 0;
    float max_height = 0;

    for (auto& child : children_) {
        child->Layout(available_width, 0);
        auto& child_layout = child->GetLayoutInfo();
        total_width += child_layout.width;
        max_height = std::max(max_height, child_layout.height);
    }

    float width = has_explicit_width ? explicit_width :
        (total_width + padding_left + padding_right + border_left + border_right);
    float height = has_explicit_height ? explicit_height :
        (max_height > 0 ? max_height + padding_top + padding_bottom + border_top + border_bottom : 20.0f);

    return {width, height};
}

void RenderInline::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 创建盒模型
    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_top_width = style.border.width.ToPx();
    box.border_right_width = style.border.width.ToPx();
    box.border_bottom_width = style.border.width.ToPx();
    box.border_left_width = style.border.width.ToPx();
    // 由于我们已经 translate 到元素左上角（border-box 的左上角）
    // content_x 和 content_y 应该从 border + padding 开始
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.border_left_width - box.border_right_width
                        - box.padding_left - box.padding_right;
    box.content_height = layout.height - box.border_top_width - box.border_bottom_width
                         - box.padding_top - box.padding_bottom;

    // 创建样式映射
    std::unordered_map<std::string, std::string> styles;
    if (!style.background_color.empty()) {
        styles["background-color"] = style.background_color;
    }
    if (!style.background_image.empty()) {
        styles["background-image"] = style.background_image;
    }

    // 渲染器
    BoxRenderer renderer(canvas);

    // 渲染背景
    renderer.RenderBackgroundAdvanced(box, styles, &style.border_radius);

    // 渲染边框
    if (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) {
        std::string border_width = std::to_string(style.border.width.value) + "px";
        std::string border_style = "solid";

        // 将 SkColor 转换为十六进制字符串
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        std::string border_color = color_str;

        renderer.RenderBorder(box, border_width, border_style, border_color);
    }

    // 渲染表单控件特定内容
    auto node = GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        // 直接从node进行dynamic_cast，保留类型信息
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            Box box;
            box.content_x = 0;  // 相对于当前画布
            box.content_y = 0;
            box.content_width = layout.width;
            box.content_height = layout.height;
            PaintInputElement(canvas, input_element.get(), box);
        }

        // 渲染 textarea 元素
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            Box box;
            box.content_x = 0;  // 相对于当前画布
            box.content_y = 0;
            box.content_width = layout.width;
            box.content_height = layout.height;
            PaintTextAreaElement(canvas, textarea_element.get(), box);
        }
    }

    // 应用 overflow 裁剪
    bool needs_clip = false;
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    auto isOverflowSet = [](const std::string& v) {
        return v == "hidden" || v == "scroll" || v == "auto";
    };
    if (isOverflowSet(overflow_x) || isOverflowSet(overflow_y)) {
        needs_clip = true;
        // 裁剪到 padding box (内容区域 + padding)
        SkRect clip_rect = SkRect::MakeXYWH(
            box.border_left_width,
            box.border_top_width,
            layout.width - box.border_left_width - box.border_right_width,
            layout.height - box.border_top_width - box.border_bottom_width
        );
        canvas->save();
        canvas->clipRect(clip_rect, SkClipOp::kIntersect, true);
    }

    // 按 z-index 排序子元素
    std::vector<std::shared_ptr<RenderObject>> sorted_children = children_;
    std::sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            return a->GetComputedStyle().z_index < b->GetComputedStyle().z_index;
        });

    // 绘制所有子元素
    for (auto& child : sorted_children) {
        child->Paint(canvas);
    }

    // 恢复裁剪状态
    if (needs_clip) {
        canvas->restore();
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

void RenderInline::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    if (!input) return;

    InputType type = input->GetInputType();
    std::string value = input->GetValue();

    // 处理文本类型的输入框
    if (type == InputType::Text || type == InputType::Password ||
        type == InputType::Email || type == InputType::Tel ||
        type == InputType::Url || type == InputType::Search ||
        type == InputType::Number) {

        // 如果value为空，显示placeholder
        bool is_placeholder = false;
        if (value.empty()) {
            value = input->GetPlaceholder();
            is_placeholder = true;
        }

        if (!value.empty()) {
            // 创建字体
            FontDescriptor desc;
            desc.family = computed_style_.font_family;
            desc.size = computed_style_.font_size;
            desc.weight = FontWeight::NORMAL;
            desc.style = FontStyle::NORMAL;

            SkFont font = FontManager::GetInstance().LoadFont(desc);

            // 创建文本渲染器
            TextRenderer text_renderer(canvas);

            // 设置文本颜色
            lightui::Paint text_paint;
            if (is_placeholder) {
                // placeholder使用灰色
                text_paint.SetColor(SkColorSetRGB(150, 150, 150));
            } else if (!computed_style_.color.empty()) {
                text_paint.SetColor(lightui::Color::Parse(computed_style_.color));
            } else {
                text_paint.SetColor(SK_ColorBLACK);
            }

            // 计算文本位置（垂直居中）
            SkFontMetrics font_metrics;
            font.getMetrics(&font_metrics);
            float text_height = -font_metrics.fAscent + font_metrics.fDescent;
            float text_y = box.content_y + (box.content_height - text_height) / 2 - font_metrics.fAscent;
            float text_x = box.content_x;

            // 如果是密码类型，显示星号
            std::string display_text = value;
            if (type == InputType::Password && !is_placeholder) {
                display_text = std::string(value.length(), '*');
            }

            // 绘制文本
            text_renderer.DrawText(display_text, text_x, text_y, font, text_paint);

            // 如果有焦点，绘制选中高亮和光标
            auto element = std::static_pointer_cast<Element>(GetNode());
            if (element && element->HasPseudoClass("focus")) {
                int sel_start = input->GetSelectionStart();
                int sel_end = input->GetSelectionEnd();

                // 绘制选中区域高亮
                if (sel_start != sel_end && !is_placeholder) {
                    int start_char = std::min(sel_start, sel_end);
                    int end_char = std::max(sel_start, sel_end);

                    // 使用 UTF-8 工具计算字节位置
                    size_t start_byte = utf8::CharPosToBytePos(value, start_char);
                    size_t end_byte = utf8::CharPosToBytePos(value, end_char);

                    std::string text_before_sel = value.substr(0, start_byte);
                    std::string selected_text = value.substr(start_byte, end_byte - start_byte);

                    // 如果是密码类型，使用星号
                    if (type == InputType::Password) {
                        text_before_sel = std::string(start_char, '*');
                        selected_text = std::string(end_char - start_char, '*');
                    }

                    float sel_start_x = text_x;
                    if (start_char > 0) {
                        sel_start_x += font.measureText(text_before_sel.c_str(), text_before_sel.length(), SkTextEncoding::kUTF8);
                    }
                    float sel_width = font.measureText(selected_text.c_str(), selected_text.length(), SkTextEncoding::kUTF8);

                    // 绘制选中背景
                    SkPaint sel_paint;
                    sel_paint.setColor(SkColorSetARGB(128, 51, 153, 255));  // 半透明蓝色
                    sel_paint.setStyle(SkPaint::kFill_Style);

                    canvas->drawRect(SkRect::MakeXYWH(sel_start_x, box.content_y, sel_width, box.content_height), sel_paint);
                }

                // 基于时间的光标闪烁
                auto now = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                bool cursor_visible = (ms / 500) % 2 == 0;

                if (cursor_visible) {
                    // 计算光标位置 - 使用 UTF-8 字符位置转换为字节位置
                    int cursor_pos = sel_end;
                    size_t cursor_byte_pos = utf8::CharPosToBytePos(value, cursor_pos);
                    std::string text_before_cursor = value.substr(0, cursor_byte_pos);

                    // 如果是密码类型，使用星号计算宽度
                    if (type == InputType::Password) {
                        text_before_cursor = std::string(cursor_pos, '*');
                    }

                    // 测量光标前的文本宽度
                    float cursor_x = text_x;
                    if (cursor_pos > 0) {
                        cursor_x += font.measureText(
                            text_before_cursor.c_str(),
                            text_before_cursor.length(),
                            SkTextEncoding::kUTF8
                        );
                    }

                    // 计算光标的Y坐标（从文本顶部到底部）
                    float cursor_y_top = box.content_y;
                    float cursor_y_bottom = box.content_y + box.content_height;

                    // 绘制光标
                    SkPaint cursor_paint;
                    cursor_paint.setColor(SK_ColorBLACK);
                    cursor_paint.setStrokeWidth(1);
                    cursor_paint.setAntiAlias(true);

                    canvas->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
                }
            }
        }
    }
    // 处理checkbox和radio类型
    else if (type == InputType::Checkbox || type == InputType::Radio) {
        bool checked = input->GetChecked();

        // 只有选中时才绘制标记
        if (checked) {
            if (type == InputType::Checkbox) {
                // 绘制勾选标记（✓）
                SkPaint check_paint;
                check_paint.setColor(SK_ColorBLACK);
                check_paint.setStrokeWidth(2);
                check_paint.setStyle(SkPaint::kStroke_Style);
                check_paint.setAntiAlias(true);

                float cx = box.content_x + box.content_width / 2;
                float cy = box.content_y + box.content_height / 2;

                SkPath check_path;
                check_path.moveTo(cx - 4, cy);
                check_path.lineTo(cx - 1, cy + 3);
                check_path.lineTo(cx + 4, cy - 3);
                canvas->drawPath(check_path, check_paint);
            }
            else if (type == InputType::Radio) {
                // 绘制圆点标记
                SkPaint dot_paint;
                dot_paint.setColor(SK_ColorBLACK);
                dot_paint.setStyle(SkPaint::kFill_Style);
                dot_paint.setAntiAlias(true);

                float cx = box.content_x + box.content_width / 2;
                float cy = box.content_y + box.content_height / 2;
                float radius = std::min(box.content_width, box.content_height) / 4;

                canvas->drawCircle(cx, cy, radius, dot_paint);
            }
        }
    }
}

void RenderInline::PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box) {
    if (!textarea) return;

    std::string value = textarea->GetValue();

    if (value.empty()) {
        // 显示placeholder
        value = textarea->GetPlaceholder();
    }

    if (!value.empty()) {
        // 创建字体
        FontDescriptor desc;
        desc.family = computed_style_.font_family;
        desc.size = computed_style_.font_size;
        desc.weight = FontWeight::NORMAL;
        desc.style = FontStyle::NORMAL;

        SkFont font = FontManager::GetInstance().LoadFont(desc);

        // 获取字体度量信息
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        float line_height = -font_metrics.fAscent + font_metrics.fDescent + font_metrics.fLeading;

        // 创建文本渲染器
        TextRenderer text_renderer(canvas);

        // 设置文本颜色
        lightui::Paint text_paint;
        if (textarea->GetValue().empty()) {
            // placeholder使用灰色
            text_paint.SetColor(SkColorSetRGB(150, 150, 150));
        } else if (!computed_style_.color.empty()) {
            text_paint.SetColor(lightui::Color::Parse(computed_style_.color));
        } else {
            text_paint.SetColor(SK_ColorBLACK);
        }

        // 绘制多行文本
        float text_x = box.content_x;
        float text_y = box.content_y - font_metrics.fAscent;

        text_renderer.DrawMultilineText(value, text_x, text_y, box.content_width, line_height, font, text_paint);

        // 如果有焦点且不是placeholder，绘制光标
        if (!textarea->GetValue().empty()) {
            auto element = std::static_pointer_cast<Element>(GetNode());
            if (element && element->HasPseudoClass("focus")) {
                // 简化版本：只在第一行显示光标
                int cursor_pos = textarea->GetSelectionStart();
                std::string text_before_cursor = textarea->GetValue().substr(0, cursor_pos);

                // 找到最后一个换行符的位置
                size_t last_newline = text_before_cursor.rfind('\n');
                std::string current_line_before_cursor;
                float cursor_y = text_y;

                if (last_newline != std::string::npos) {
                    // 光标在某一行中
                    current_line_before_cursor = text_before_cursor.substr(last_newline + 1);
                    // 计算光标所在行（简化：每个\n增加一行）
                    int line_count = std::count(text_before_cursor.begin(), text_before_cursor.end(), '\n');
                    cursor_y += line_count * line_height;
                } else {
                    // 光标在第一行
                    current_line_before_cursor = text_before_cursor;
                }

                // 测量光标前的文本宽度
                float cursor_x = text_x;
                if (!current_line_before_cursor.empty()) {
                    cursor_x += font.measureText(
                        current_line_before_cursor.c_str(),
                        current_line_before_cursor.length(),
                        SkTextEncoding::kUTF8
                    );
                }

                // 绘制光标
                SkPaint cursor_paint;
                cursor_paint.setColor(SK_ColorBLACK);
                cursor_paint.setStrokeWidth(1);
                cursor_paint.setAntiAlias(true);

                canvas->drawLine(cursor_x, cursor_y + font_metrics.fAscent,
                               cursor_x, cursor_y + font_metrics.fDescent, cursor_paint);
            }
        }
    }
}

// ========== RenderText 实现 ==========

void RenderText::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 创建字体
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? FontWeight::BOLD : FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 创建文本渲染器来测量文本
    TextRenderer text_renderer(nullptr);

    // 检查是否包含换行符
    if (text_.find('\n') != std::string::npos) {
        // 多行文本：分别测量每一行，取最大宽度和累加高度
        std::istringstream iss(text_);
        std::string line;
        float max_width = 0;
        float total_height = 0;
        float line_height = style.line_height * style.font_size;
        int line_count = 0;

        while (std::getline(iss, line)) {
            // 使用支持emoji的文本测量
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            max_width = std::max(max_width, line_width);
            line_count++;
        }

        total_height = line_count * line_height;

        layout_info_.width = max_width;
        layout_info_.height = total_height;
    } else {
        // 单行文本 - 使用支持emoji的测量
        float width = text_renderer.MeasureTextWidthWithEmoji(text_, font);
        auto metrics = text_renderer.MeasureText(text_, font);
        layout_info_.width = width;
        layout_info_.height = metrics.height;
    }

    layout_info_.content_rect = SkRect::MakeWH(layout_info_.width, layout_info_.height);
    layout_info_.is_laid_out = true;

    needs_layout_ = false;
}

void RenderText::Paint(SkCanvas* canvas) {
    if (!canvas || text_.empty()) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 使用 Taffy 计算的位置来支持 text-align
    // layout.x 包含了 text-align 的偏移量
    float text_x = layout.x;
    float text_y = layout.y;

    // 保存画布状态
    canvas->save();
    // 移动到文本位置（支持 text-align 居中等）
    canvas->translate(text_x, text_y);

    // 创建字体
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? FontWeight::BOLD : FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 获取字体度量信息
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    // 计算基线位置：从顶部开始，向下偏移 ascent（ascent 是负值，所以取反）
    float baseline_y = -font_metrics.fAscent;

    // 创建文本渲染器
    TextRenderer text_renderer(canvas);

    // 设置文本样式
    SkColor text_color;
    if (!style.color.empty()) {
        text_color = lightui::Color::Parse(style.color);
    } else {
        // 默认黑色
        text_color = SK_ColorBLACK;
    }

    // Determine which lines to render
    // Priority: 1. wrapped_lines_ (from Taffy measure), 2. explicit newlines, 3. single line
    std::vector<std::string> lines_to_render;

    if (!wrapped_lines_.empty()) {
        // Use wrapped lines from Taffy measure function
        lines_to_render = wrapped_lines_;
    } else if (text_.find('\n') != std::string::npos) {
        // Split by explicit newlines
        std::istringstream iss(text_);
        std::string line;
        while (std::getline(iss, line)) {
            lines_to_render.push_back(line);
        }
    } else {
        // Single line
        lines_to_render.push_back(text_);
    }

    // Render all lines
    float current_y = baseline_y;
    float line_height = style.line_height * style.font_size;

    for (const auto& line : lines_to_render) {
        // Skip empty lines (but still advance y position)
        if (!line.empty()) {
            if (!style.text_shadow.empty()) {
                ShadowRenderer::RenderTextWithShadow(canvas, line, font, 0, current_y, text_color, style.text_shadow);
            } else {
                lightui::Paint text_paint;
                text_paint.SetColor(text_color);
                // 使用支持emoji的文本渲染
                text_renderer.DrawTextWithEmoji(line, 0, current_y, font, text_paint);
            }
        }

        // Move to next line
        current_y += line_height;
    }

    // 绘制文本装饰（下划线、删除线等）- 需要为每一行绘制
    if (style.text_decoration == "underline" || style.text_decoration == "line-through") {
        SkPaint line_paint;
        line_paint.setColor(text_color);
        line_paint.setAntiAlias(true);

        float decoration_current_y = baseline_y;

        for (size_t i = 0; i < lines_to_render.size(); ++i) {
            const auto& line = lines_to_render[i];

            // Calculate line width (use actual text width for each line)
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            if (line_width <= 0) {
                decoration_current_y += line_height;
                continue;
            }

            if (style.text_decoration == "underline") {
                // 下划线：在基线下方
                float underline_y = decoration_current_y + font_metrics.fUnderlinePosition;
                float underline_thickness = font_metrics.fUnderlineThickness;
                if (underline_thickness < 1.0f) underline_thickness = 1.0f;
                line_paint.setStrokeWidth(underline_thickness);
                canvas->drawLine(0, underline_y, line_width, underline_y, line_paint);
            } else if (style.text_decoration == "line-through") {
                // 删除线：在文字中间
                float strikethrough_y = decoration_current_y + font_metrics.fStrikeoutPosition;
                float strikethrough_thickness = font_metrics.fStrikeoutThickness;
                if (strikethrough_thickness < 1.0f) strikethrough_thickness = 1.0f;
                line_paint.setStrokeWidth(strikethrough_thickness);
                canvas->drawLine(0, strikethrough_y, line_width, strikethrough_y, line_paint);
            }

            decoration_current_y += line_height;
        }
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

} // namespace lightui

