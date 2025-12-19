/**
 * @file render_object.cpp
 * @brief 渲染对象实现
 */

#include "render_object.h"
#include "render_inline_block.h"
#include "box_renderer.h"
#include "text_renderer.h"
#include "text_transform.h"
#include "gradient_renderer.h"
#include "shadow_renderer.h"
#include "list_marker.h"
#include "color.h"
#include "css_value.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_canvas_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include <atomic>
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

namespace lightui {

// 静态成员初始化
float RenderObject::viewport_width_ = 0.0f;
float RenderObject::viewport_height_ = 0.0f;

// 视口剔除调试统计（用于验证 quickReject 效果）
static std::atomic<int> g_paint_total_calls{0};
static std::atomic<int> g_paint_culled_calls{0};

void RenderObject::ResetPaintStats() {
    g_paint_total_calls = 0;
    g_paint_culled_calls = 0;
}

void RenderObject::PrintPaintStats() {
    int total = g_paint_total_calls.load();
    int culled = g_paint_culled_calls.load();
    int painted = total - culled;
    float cull_rate = total > 0 ? (culled * 100.0f / total) : 0.0f;
    std::cout << "[ViewportCulling] Total: " << total 
              << ", Painted: " << painted 
              << ", Culled: " << culled 
              << " (" << cull_rate << "%)" << std::endl;
}

// 辅助函数：计算浏览器风格的 line-height: normal
// 与 IFCLayout::MeasureTextStatic 中的查找表保持一致
// 使用 Arial 字体的 line-height: normal 值（比率约 1.156）
static float GetBrowserNormalLineHeight(float font_size) {
    int font_size_int = static_cast<int>(font_size + 0.5f);  // 四舍五入到整数
    switch (font_size_int) {
        case 10: return 11.5f;   // ~1.15
        case 11: return 13.0f;   // ~1.18
        case 12: return 14.0f;   // ~1.17
        case 13: return 15.0f;   // ~1.15
        case 14: return 16.0f;   // ~1.14
        case 15: return 17.5f;   // ~1.17
        case 16: return 18.5f;   // ~1.156 (从浏览器测量)
        case 17: return 19.5f;   // ~1.15
        case 18: return 21.0f;   // ~1.17
        case 19: return 22.0f;   // ~1.16
        case 20: return 23.0f;   // ~1.15
        case 22: return 25.5f;   // ~1.16
        case 24: return 28.0f;   // ~1.17
        case 32: return 37.0f;   // h1 (32px -> 37px, 从浏览器测量)
        default:
            // 对于其他字体大小，使用 1.156 倍数并四舍五入到 0.5px
            float line_height = font_size * 1.156f;
            return std::round(line_height * 2.0f) / 2.0f;
    }
}

void RenderObject::SetViewportSize(float width, float height) {
    viewport_width_ = width;
    viewport_height_ = height;
    // 同步更新 ViewportSize（用于 CSS vh/vw 单位计算）
    ViewportSize::Set(width, height);
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

// =========================================================================
// 布局样式转换辅助函数
// =========================================================================

namespace {

/// 将 CSSLength 转换为 Dimension
Dimension ConvertDimension(const CSSLength& value) {
    if (value.IsAuto()) {
        return Dimension::Auto();
    }
    if (value.unit == CSSUnit::PERCENT) {
        return Dimension::Percent(value.value / 100.0f);
    }
    return Dimension::Length(value.ToPx());
}

/// 将 CSSLength 转换为 LengthPercentage
LengthPercentage ConvertLengthFromCSSLength(const CSSLength& value) {
    if (value.unit == CSSUnit::PERCENT) {
        return LengthPercentage::Percent(value.value / 100.0f);
    }
    return LengthPercentage::Length(value.ToPx());
}

/// 将 CSSLength 转换为 LengthPercentageAuto
LengthPercentageAuto ConvertLengthAuto(const CSSLength& value) {
    if (value.IsAuto()) {
        return LengthPercentageAuto::Auto();
    }
    if (value.unit == CSSUnit::PERCENT) {
        return LengthPercentageAuto::Percent(value.value / 100.0f);
    }
    return LengthPercentageAuto::Length(value.ToPx());
}

/// 将 ComputedStyle 转换为布局 Style
Style ConvertComputedStyleToLayoutStyle(const ComputedStyle& computed) {
    Style style;

    // Display
    switch (computed.display) {
        case RenderObjectType::NONE:
            style.display = Display::None;
            break;
        case RenderObjectType::FLEX:
            style.display = Display::Flex;
            break;
        case RenderObjectType::GRID:
            style.display = Display::Grid;
            break;
        default:
            style.display = Display::Block;
            break;
    }

    // Position
    if (computed.position == "absolute") {
        style.position = Position::Absolute;
    } else if (computed.position == "relative") {
        style.position = Position::Relative;
    } else if (computed.position == "fixed") {
        style.position = Position::Fixed;
    } else if (computed.position == "sticky") {
        style.position = Position::Sticky;
    } else {
        style.position = Position::Relative;
    }

    // Box sizing
    if (computed.box_sizing == "border-box") {
        style.box_sizing = BoxSizing::BorderBox;
    } else {
        style.box_sizing = BoxSizing::ContentBox;
    }

    // Size
    style.size.width = ConvertDimension(computed.width);
    style.size.height = ConvertDimension(computed.height);
    style.min_size.width = ConvertDimension(computed.min_width);
    style.min_size.height = ConvertDimension(computed.min_height);
    style.max_size.width = ConvertDimension(computed.max_width);
    style.max_size.height = ConvertDimension(computed.max_height);

    // Padding
    style.padding.left = ConvertLengthFromCSSLength(computed.padding.left);
    style.padding.right = ConvertLengthFromCSSLength(computed.padding.right);
    style.padding.top = ConvertLengthFromCSSLength(computed.padding.top);
    style.padding.bottom = ConvertLengthFromCSSLength(computed.padding.bottom);

    // Margin
    style.margin.left = ConvertLengthAuto(computed.margin.left);
    style.margin.right = ConvertLengthAuto(computed.margin.right);
    style.margin.top = ConvertLengthAuto(computed.margin.top);
    style.margin.bottom = ConvertLengthAuto(computed.margin.bottom);

    // Border widths
    float fallback_border = computed.border.width.ToPx(0, computed.font_size);
    style.border.left = LengthPercentage::Length(computed.border_left_width > 0 ? computed.border_left_width : fallback_border);
    style.border.right = LengthPercentage::Length(computed.border_right_width > 0 ? computed.border_right_width : fallback_border);
    style.border.top = LengthPercentage::Length(computed.border_top_width > 0 ? computed.border_top_width : fallback_border);
    style.border.bottom = LengthPercentage::Length(computed.border_bottom_width > 0 ? computed.border_bottom_width : fallback_border);

    // Inset (for positioned elements)
    style.inset.left = ConvertLengthAuto(computed.left);
    style.inset.right = ConvertLengthAuto(computed.right);
    style.inset.top = ConvertLengthAuto(computed.top);
    style.inset.bottom = ConvertLengthAuto(computed.bottom);

    // Flexbox properties
    style.flex_direction = FlexDirection::Row;
    if (computed.flex_direction == "column") {
        style.flex_direction = FlexDirection::Column;
    } else if (computed.flex_direction == "row-reverse") {
        style.flex_direction = FlexDirection::RowReverse;
    } else if (computed.flex_direction == "column-reverse") {
        style.flex_direction = FlexDirection::ColumnReverse;
    }

    style.flex_wrap = FlexWrap::NoWrap;
    if (computed.flex_wrap == "wrap") {
        style.flex_wrap = FlexWrap::Wrap;
    } else if (computed.flex_wrap == "wrap-reverse") {
        style.flex_wrap = FlexWrap::WrapReverse;
    }

    style.flex_grow = computed.flex_grow;
    style.flex_shrink = computed.flex_shrink;
    style.flex_basis = ConvertDimension(computed.flex_basis);
    style.order = computed.order;

    // Alignment
    if (computed.justify_content == "flex-start") {
        style.justify_content = JustifyContent::FlexStart;
    } else if (computed.justify_content == "start") {
        style.justify_content = JustifyContent::Start;
    } else if (computed.justify_content == "flex-end") {
        style.justify_content = JustifyContent::FlexEnd;
    } else if (computed.justify_content == "end") {
        style.justify_content = JustifyContent::End;
    } else if (computed.justify_content == "center") {
        style.justify_content = JustifyContent::Center;
    } else if (computed.justify_content == "space-between") {
        style.justify_content = JustifyContent::SpaceBetween;
    } else if (computed.justify_content == "space-around") {
        style.justify_content = JustifyContent::SpaceAround;
    } else if (computed.justify_content == "space-evenly") {
        style.justify_content = JustifyContent::SpaceEvenly;
    }

    // align-items
    if (computed.align_items == "flex-start" || computed.align_items == "start") {
        style.align_items = AlignItems::FlexStart;
    } else if (computed.align_items == "flex-end" || computed.align_items == "end") {
        style.align_items = AlignItems::FlexEnd;
    } else if (computed.align_items == "center") {
        style.align_items = AlignItems::Center;
    } else if (computed.align_items == "baseline") {
        style.align_items = AlignItems::Baseline;
    } else if (computed.align_items == "stretch") {
        style.align_items = AlignItems::Stretch;
    }

    // align-self
    if (computed.align_self == "auto") {
        style.align_self = std::nullopt;
    } else if (computed.align_self == "flex-start" || computed.align_self == "start") {
        style.align_self = AlignSelf::FlexStart;
    } else if (computed.align_self == "flex-end" || computed.align_self == "end") {
        style.align_self = AlignSelf::FlexEnd;
    } else if (computed.align_self == "center") {
        style.align_self = AlignSelf::Center;
    } else if (computed.align_self == "baseline") {
        style.align_self = AlignSelf::Baseline;
    } else if (computed.align_self == "stretch") {
        style.align_self = AlignSelf::Stretch;
    }

    // Gap
    style.gap.width = ConvertLengthFromCSSLength(computed.column_gap);
    style.gap.height = ConvertLengthFromCSSLength(computed.row_gap);

    // Aspect ratio
    // Convert ComputedStyle::AspectRatio to layout Style::aspect_ratio
    // Requirements 3.1-3.7: aspect-ratio property support
    if (!computed.aspect_ratio.is_auto && computed.aspect_ratio.HasRatio()) {
        style.aspect_ratio = computed.aspect_ratio.ratio;
    } else {
        style.aspect_ratio = std::nullopt;
    }

    return style;
}

/// 将 CoreStyle 基础属性从 Style 复制到目标
void CopyCoreStyleFrom(CoreStyle& target, const Style& source) {
    target.display = source.display;
    target.box_sizing = source.box_sizing;
    target.position = source.position;
    target.overflow = source.overflow;
    target.scrollbar_width = source.scrollbar_width;
    target.size = source.size;
    target.min_size = source.min_size;
    target.max_size = source.max_size;
    target.padding = source.padding;
    target.border = source.border;
    target.margin = source.margin;
    target.inset = source.inset;
}

} // anonymous namespace

void RenderObject::UpdateLayoutStyle() {
    if (!layout_style_dirty_) {
        return;
    }

    // 转换 ComputedStyle 到布局 Style
    layout_style_ = ConvertComputedStyleToLayoutStyle(computed_style_);

    // 更新 Block 样式
    CopyCoreStyleFrom(block_container_style_, layout_style_);
    CopyCoreStyleFrom(block_item_style_, layout_style_);

    // 更新 Flexbox 样式
    CopyCoreStyleFrom(flex_container_style_, layout_style_);
    flex_container_style_.flex_direction = layout_style_.flex_direction;
    flex_container_style_.flex_wrap = layout_style_.flex_wrap;
    flex_container_style_.align_items = layout_style_.align_items.value_or(AlignItems::Stretch);
    flex_container_style_.align_content = layout_style_.align_content.value_or(AlignContent::Stretch);
    flex_container_style_.justify_content = layout_style_.justify_content;
    flex_container_style_.gap = layout_style_.gap;

    CopyCoreStyleFrom(flex_item_style_, layout_style_);
    flex_item_style_.align_self = layout_style_.align_self;
    flex_item_style_.flex_grow = layout_style_.flex_grow;
    flex_item_style_.flex_shrink = layout_style_.flex_shrink;
    flex_item_style_.flex_basis = layout_style_.flex_basis;
    flex_item_style_.order = layout_style_.order;

    // 更新 Grid 样式
    CopyCoreStyleFrom(grid_container_style_, layout_style_);
    grid_container_style_.align_items = layout_style_.align_items;
    grid_container_style_.justify_items = layout_style_.justify_items;
    grid_container_style_.row_gap = ConvertLengthFromCSSLength(computed_style_.row_gap);
    grid_container_style_.column_gap = ConvertLengthFromCSSLength(computed_style_.column_gap);
    // Note: grid-template-* 需要更复杂的解析，暂时保持默认值

    CopyCoreStyleFrom(grid_item_style_, layout_style_);
    grid_item_style_.align_self = layout_style_.align_self;
    // Note: grid-row/column-* 需要更复杂的解析，暂时保持默认值

    // 清除脏标记
    layout_style_dirty_ = false;
}

void RenderObject::Layout(float parent_width, float parent_height) {
    // 基类默认实现：简单的块布局
    // 子类应该重写此方法实现具体的布局逻辑
    needs_layout_ = false;
}

void RenderObject::UpdatePaintCache() {
    // P1优化：样式预计算缓存
    // 如果缓存有效，直接返回
    if (paint_cache_.valid) {
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 计算 padding（像素值）
    paint_cache_.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    paint_cache_.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    paint_cache_.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    paint_cache_.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);

    // 计算 border 宽度（优先使用单边边框宽度）
    paint_cache_.border_top_width = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    paint_cache_.border_right_width = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    paint_cache_.border_bottom_width = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();
    paint_cache_.border_left_width = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();

    // 计算内容区域偏移
    paint_cache_.content_x = paint_cache_.border_left_width + paint_cache_.padding_left;
    paint_cache_.content_y = paint_cache_.border_top_width + paint_cache_.padding_top;

    // 预解析背景颜色
    if (!style.background_color.empty() && style.background_color != "transparent") {
        paint_cache_.background_color = Color::Parse(style.background_color);
    } else {
        paint_cache_.background_color = SK_ColorTRANSPARENT;
    }

    // 预解析边框颜色
    paint_cache_.border_top_color = style.border_top_style != CSSBorderStyle::NONE ? 
        style.border_top_color : style.border.color;
    paint_cache_.border_right_color = style.border_right_style != CSSBorderStyle::NONE ? 
        style.border_right_color : style.border.color;
    paint_cache_.border_bottom_color = style.border_bottom_style != CSSBorderStyle::NONE ? 
        style.border_bottom_color : style.border.color;
    paint_cache_.border_left_color = style.border_left_style != CSSBorderStyle::NONE ? 
        style.border_left_color : style.border.color;

    // 预计算圆角
    paint_cache_.border_radius_tl = style.border_radius.top_left.ToPx(layout.width);
    paint_cache_.border_radius_tr = style.border_radius.top_right.ToPx(layout.width);
    paint_cache_.border_radius_bl = style.border_radius.bottom_left.ToPx(layout.width);
    paint_cache_.border_radius_br = style.border_radius.bottom_right.ToPx(layout.width);

    // 预计算标志位
    paint_cache_.has_border = (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) ||
                              (paint_cache_.border_left_width > 0 && style.border_left_style != CSSBorderStyle::NONE) ||
                              (paint_cache_.border_right_width > 0 && style.border_right_style != CSSBorderStyle::NONE) ||
                              (paint_cache_.border_top_width > 0 && style.border_top_style != CSSBorderStyle::NONE) ||
                              (paint_cache_.border_bottom_width > 0 && style.border_bottom_style != CSSBorderStyle::NONE);

    paint_cache_.has_border_radius = paint_cache_.border_radius_tl > 0 ||
                                     paint_cache_.border_radius_tr > 0 ||
                                     paint_cache_.border_radius_bl > 0 ||
                                     paint_cache_.border_radius_br > 0;

    paint_cache_.has_box_shadow = !style.box_shadow.empty();
    paint_cache_.has_gradient = style.background_linear_gradient.has_value() || 
                                style.background_radial_gradient.has_value();

    // 标记缓存有效
    paint_cache_.valid = true;
}

void RenderObject::Paint(SkCanvas* canvas) {
    // 基类默认实现：什么都不做
    needs_paint_ = false;
}

void RenderObject::PaintOutline(SkCanvas* canvas) {
    if (!canvas) return;
    
    const auto& style = computed_style_;
    const auto& layout = layout_info_;
    
    // Skip if outline is not visible
    if (style.outline_style == "none" || style.outline_width.IsZero()) {
        return;
    }
    
    float outline_width = style.outline_width.ToPx();
    float outline_offset = style.outline_offset.ToPx();
    
    // Use stroke drawing: line is centered on the rectangle edge
    // To make outline inner edge touch border-box outer edge, offset by half_width
    // So stroke center is at (outline_offset + half_width), inner edge at outline_offset
    float half_width = outline_width / 2.0f;
    SkRect outline_rect = SkRect::MakeXYWH(
        -(outline_offset + half_width),
        -(outline_offset + half_width),
        layout.width + 2 * (outline_offset + half_width),
        layout.height + 2 * (outline_offset + half_width)
    );
    
    SkPaint outline_paint;
    outline_paint.setColor(style.outline_color);
    outline_paint.setStyle(SkPaint::kStroke_Style);
    outline_paint.setStrokeWidth(outline_width);
    outline_paint.setAntiAlias(true);
    
    // Set line style based on outline_style
    if (style.outline_style == "dashed") {
        const SkScalar intervals[] = {6.0f, 3.0f};
        outline_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    } else if (style.outline_style == "dotted") {
        const SkScalar intervals[] = {2.0f, 2.0f};
        outline_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    }
    // solid doesn't need special handling
    
    // If element has border-radius, outline should also have rounded corners
    if (style.border_radius.top_left.value > 0 || style.border_radius.top_right.value > 0 ||
        style.border_radius.bottom_left.value > 0 || style.border_radius.bottom_right.value > 0) {
        // Calculate border-radius percentage base size
        float box_width = outline_rect.width();
        float box_height = outline_rect.height();
        float base_size = std::min(box_width, box_height);
        
        float tl = style.border_radius.top_left.ToPx(base_size) + outline_offset + half_width;
        float tr = style.border_radius.top_right.ToPx(base_size) + outline_offset + half_width;
        float br = style.border_radius.bottom_right.ToPx(base_size) + outline_offset + half_width;
        float bl = style.border_radius.bottom_left.ToPx(base_size) + outline_offset + half_width;
        
        SkRRect outline_rrect;
        SkVector radii[4] = {{tl, tl}, {tr, tr}, {br, br}, {bl, bl}};
        outline_rrect.setRectRadii(outline_rect, radii);
        canvas->drawRRect(outline_rrect, outline_paint);
    } else {
        canvas->drawRect(outline_rect, outline_paint);
    }
}

SkRect RenderObject::GetBoundingRect() const {
    // 使用布局信息计算边界框（文档坐标系，用于脏区域收集）
    const auto& layout = layout_info_;

    // 如果布局信息无效，返回空矩形
    if (!layout.is_laid_out) {
        return SkRect::MakeEmpty();
    }

    // 计算绝对位置（需要累加所有祖先的偏移）
    float abs_x = layout.x;
    float abs_y = layout.y;

    auto parent = parent_.lock();
    while (parent) {
        const auto& parent_layout = parent->GetLayoutInfo();
        abs_x += parent_layout.x;
        abs_y += parent_layout.y;

        // 注意：这里不减去滚动偏移，保持文档坐标系
        // 这样脏区域收集才能正确工作

        parent = parent->GetParent();
    }

    return SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);
}

SkRect RenderObject::GetViewportBoundingRect() const {
    // 使用布局信息计算边界框（视口坐标系，用于元素选择器高亮）
    const auto& layout = layout_info_;

    // 如果布局信息无效，返回空矩形
    if (!layout.is_laid_out) {
        return SkRect::MakeEmpty();
    }

    // 计算绝对位置（需要累加所有祖先的偏移）
    float abs_x = layout.x;
    float abs_y = layout.y;

    auto parent = parent_.lock();
    while (parent) {
        const auto& parent_layout = parent->GetLayoutInfo();
        abs_x += parent_layout.x;
        abs_y += parent_layout.y;

        // 减去父元素的滚动偏移，转换为视口坐标
        abs_x -= parent->GetScrollX();
        abs_y -= parent->GetScrollY();

        parent = parent->GetParent();
    }

    return SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);
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

    float old_scroll_x = scroll_x_;
    float old_scroll_y = scroll_y_;

    scroll_x_ = std::max(0.0f, std::min(x, max_x));
    scroll_y_ = std::max(0.0f, std::min(y, max_y));

    // 只有滚动位置真正改变时才标记重绘
    if (scroll_x_ != old_scroll_x || scroll_y_ != old_scroll_y) {
        MarkNeedsPaint();
        
        // 关键修复：滚动时，标记所有子元素也需要重绘
        // 因为子元素的视觉位置改变了（即使布局位置没变）
        std::function<void(RenderObject*)> mark_children = [&](RenderObject* obj) {
            if (!obj) return;
            obj->MarkNeedsPaint();
            for (const auto& child : obj->GetChildren()) {
                mark_children(child.get());
            }
        };
        mark_children(this);
    }
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

    // 计算 border 宽度 - 与 Paint 中保持一致：优先使用单边边框宽度
    const auto& style = computed_style_;
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 可见区域需要减去 border（与 Paint 保持一致）
    float visible_width = effective_width - border_left - border_right;
    float visible_height = effective_height - border_top - border_bottom;

    // ✅ 修复：始终动态计算内容尺寸，不使用缓存（与GetMaxScrollY保持一致）
    float content_width = CalculateContentWidth();
    float content_height = CalculateContentHeight();

    // 检查是否需要垂直滚动条
    bool needs_v_scroll = content_height > visible_height;

    // 可用内容宽度需要减去垂直滚动条宽度
    float available_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
    return std::max(0.0f, content_width - available_width);
}

float RenderObject::GetMaxScrollY() const {
    const float scrollbar_width = GetScrollbarWidth();
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 计算 border 宽度 - 与 Paint 中保持一致：优先使用单边边框宽度
    const auto& style = computed_style_;
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 可见区域需要减去 border（与 Paint 保持一致）
    float visible_width = effective_width - border_left - border_right;
    float visible_height = effective_height - border_top - border_bottom;

    // ✅ 修复：始终动态计算内容尺寸，不使用缓存
    // 原因：窗口resize后，effective_height会立即更新，但content_height_可能
    //      还是旧值（因为Paint可能被增量渲染跳过），导致滚动范围计算错误
    float content_width = CalculateContentWidth();
    float content_height = CalculateContentHeight();

    // 检查是否需要垂直滚动条（用于计算内容区域宽度）
    bool needs_v_scroll = content_height > visible_height;
    float available_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);

    // 检查是否需要水平滚动条
    bool needs_h_scroll = content_width > available_width;

    // 可用内容高度需要减去水平滚动条高度
    float available_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);
    float max_scroll = std::max(0.0f, content_height - available_height);

    return max_scroll;
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

    // 计算 border 宽度（与 Paint 保持一致）
    float border_left = style.border.width.ToPx();
    float border_right = style.border.width.ToPx();
    float border_top = style.border.width.ToPx();
    float border_bottom = style.border.width.ToPx();

    // 计算可见区域（对于 body 元素使用视口尺寸）- 减去 border
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();
    float visible_width = effective_width - border_left - border_right;
    float visible_height = effective_height - border_top - border_bottom;

    // 如果 content_width_/height_ 还没初始化（首次渲染前），动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

    // 使用与 Paint 相同的逻辑判断是否需要滚动条
    bool needs_v_scroll = allow_v_scroll && (content_height > visible_height || overflow_y == "scroll");
    float content_area_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
    bool needs_h_scroll = allow_h_scroll && (content_width > content_area_width || overflow_x == "scroll");

    // 检测垂直滚动条区域（优先检测，因为它更常见）
    // 注意：local_x/local_y 是相对于元素的坐标，需要考虑 border
    if (needs_v_scroll) {
        float track_x = border_left + visible_width - scrollbar_width;
        float track_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);

        if (local_x >= track_x && local_x <= border_left + visible_width &&
            local_y >= border_top && local_y <= border_top + track_height) {
            return ScrollbarHitArea::VerticalTrack;
        }
    }

    // 检测水平滚动条区域
    if (needs_h_scroll) {
        float track_y = border_top + visible_height - scrollbar_width;
        float track_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);

        if (local_y >= track_y && local_y <= border_top + visible_height &&
            local_x >= border_left && local_x <= border_left + track_width) {
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

    // 计算 border 宽度（与 Paint 保持一致）
    float border_left = computed_style_.border.width.ToPx();
    float border_right = computed_style_.border.width.ToPx();
    float border_top = computed_style_.border.width.ToPx();
    float border_bottom = computed_style_.border.width.ToPx();

    // 对于 body 元素使用视口尺寸
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 可见区域减去 border（与 Paint 保持一致）
    float visible_width = effective_width - border_left - border_right;
    float visible_height = effective_height - border_top - border_bottom;

    // 如果 content_width_/height_ 还没初始化，动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

    // 使用与 Paint 相同的逻辑判断滚动条
    bool needs_v_scroll = content_height > visible_height;
    float content_area_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
    bool needs_h_scroll = content_width > content_area_width;
    float content_area_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);

    if (dragging_scrollbar_ == ScrollbarHitArea::HorizontalTrack ||
        dragging_scrollbar_ == ScrollbarHitArea::HorizontalThumb) {
        // 水平滚动
        float scrollable_width = content_width - content_area_width;

        if (scrollable_width > 0 && content_area_width > 0) {
            // 计算滑块可以移动的轨道长度
            float thumb_ratio = content_area_width / content_width;
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
        float scrollable_height = content_height - content_area_height;

        if (scrollable_height > 0 && content_area_height > 0) {
            // 计算滑块可以移动的轨道长度
            float thumb_ratio = content_area_height / content_height;
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

        // ✅ 修复：不再加 margin-bottom，因为 child_y 在 Layout 阶段已经包含了之前元素的 margin
        // 内容高度 = 子元素位置 + 子元素高度（不加margin，避免重复计算）
        max_height = std::max(max_height, child_y + child_height);
    }

    // ✅ 修复：为最后一个子元素添加 margin-bottom（这部分在可滚动区域内应该被看到）
    // 并且为 body 元素添加其自身的 margin-bottom
    if (!children_.empty()) {
        const auto& last_child = children_.back();
        const auto& last_child_style = last_child->GetComputedStyle();
        // 最后一个子元素的 margin-bottom 不会与后续元素 collapse，需要计入内容高度
        float last_margin_bottom = last_child_style.margin.bottom.ToPx(layout_info_.width, last_child_style.font_size);
        max_height += last_margin_bottom;
    }

    // For body element, add body's own margin-bottom to content height
    // because it's part of the scrollable content
    if (IsBodyElement()) {
        // 注意：body 的 margin-top 已经在第一个子元素的 child_y 中体现了
        // 只需要加上 margin-bottom
        float body_margin_bottom = computed_style_.margin.bottom.ToPx(viewport_height_, computed_style_.font_size);
        max_height += body_margin_bottom;
    }

    // ✅ 修复：添加容器自身的 padding-bottom
    // 原因：子元素的 y 坐标已经包含了 padding-top (从 padding-top 开始布局)
    //      但内容高度需要延伸到 padding-bottom 的底部，这样滚动时才能看到完整的底部留白
    // 注意：不需要加 padding-top，因为子元素的 child_y 已经是相对于 content area 的
    //      (content area 从 border + padding-top 开始)
    float padding_bottom = computed_style_.padding.bottom.ToPx(layout_info_.width, computed_style_.font_size);
    max_height += padding_bottom;

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

    // ✅ 修复：添加容器自身的 padding-right（与 CalculateContentHeight 保持一致）
    float padding_right = computed_style_.padding.right.ToPx(layout_info_.width, computed_style_.font_size);
    max_width += padding_right;

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
    
    // 计算 border - 优先使用单边边框宽度，否则使用统一的 border.width（与 Paint 保持一致）
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();
    
    // 计算内容区域宽度
    float content_width = width - padding_left - padding_right - border_left - border_right;
    
    // 布局子元素 - 第一遍：计算尺寸
    for (auto& child : children_) {
        if (child->NeedsLayout()) {
            // 检查是否是 legend 元素，需要特殊处理宽度
            auto child_node = child->GetNode();
            bool is_legend = false;
            if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto child_elem = std::static_pointer_cast<Element>(child_node);
                is_legend = (child_elem->GetTagName() == "legend");
            }

            if (is_legend) {
                // legend 的宽度应该是 fit-content（自适应内容）
                // 用大宽度布局，让文本不换行
                child->Layout(10000, 0);

                // 找到子元素的最右边位置
                float max_right = 0.0f;
                for (const auto& grandchild : child->GetChildren()) {
                    auto& gc_layout = grandchild->GetLayoutInfo();
                    max_right = std::max(max_right, gc_layout.x + gc_layout.width);
                }

                auto& child_style = child->GetComputedStyle();
                float legend_padding_right = child_style.padding.right.ToPx();
                float legend_border_right = child_style.border_right_width > 0 ? child_style.border_right_width : child_style.border.width.ToPx();

                // 设置 legend 的布局宽度 = 子元素右边缘 + 右侧 padding + 右侧 border
                child->GetLayoutInfo().width = max_right + legend_padding_right + legend_border_right;
            } else {
                child->Layout(content_width, 0);
            }
        }
    }

    // 布局子元素 - 第二遍：设置位置
    // 支持内联元素水平排列和块级元素垂直排列
    float current_y = 0;
    float current_x = padding_left + border_left;
    float line_height = 0;  // 当前行的高度

    // 检查是否是 fieldset 元素
    bool is_fieldset = false;
    auto this_node = GetNode();
    if (this_node && this_node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto this_elem = std::static_pointer_cast<Element>(this_node);
        is_fieldset = (this_elem->GetTagName() == "fieldset");
    }

    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        auto& child_style = child->GetComputedStyle();

        // 计算子元素的 margin
        float child_margin_top = child_style.margin.top.ToPx(width, child_style.font_size);
        float child_margin_bottom = child_style.margin.bottom.ToPx(width, child_style.font_size);
        float child_margin_left = child_style.margin.left.ToPx(width, child_style.font_size);
        float child_margin_right = child_style.margin.right.ToPx(width, child_style.font_size);

        // 检查是否是 legend 元素（fieldset 的子元素）
        bool is_legend = false;
        if (is_fieldset) {
            auto child_node = child->GetNode();
            if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto child_elem = std::static_pointer_cast<Element>(child_node);
                is_legend = (child_elem->GetTagName() == "legend");
            }
        }

        // 判断是块级还是内联元素
        // 检查渲染对象的实际类型，而不是 display 属性
        // RenderInlineBlock 也应该被视为内联元素参与行内流
        bool is_inline = (dynamic_cast<RenderInline*>(child.get()) != nullptr ||
                         dynamic_cast<RenderText*>(child.get()) != nullptr ||
                         dynamic_cast<RenderInlineBlock*>(child.get()) != nullptr);

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
            // ✅ 修复：Inline元素的垂直margin不应该参与行高计算（CSS规范）
            // 这样可以避免inline元素后的block元素margin被错误累加
            line_height = std::max(line_height, child_layout.height);
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

            // 对于 legend 元素，使用特殊的 y 坐标计算
            // 根据浏览器行为，legend 的 y 坐标（从 getBoundingClientRect 获取）
            // 与 fieldset 的 y 坐标相同，即 legend.y = 0（相对于 fieldset）
            if (is_legend) {
                new_y = 0;
            }

            child_layout.x = new_x;
            child_layout.y = new_y;

            // 累加高度（legend 不占用正常流的高度，因为它在边框上）
            if (!is_legend) {
                current_y += child_margin_top + child_layout.height + child_margin_bottom;
            }
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

    // 统计：每次 Paint 调用
    extern std::atomic<int> g_paint_total_calls;
    extern std::atomic<int> g_paint_culled_calls;
    g_paint_total_calls++;

    // Enterprise-Grade Optimization: View Culling
    // Check if the object is visible in the current clip rect.
    // layout_info_ contains coordinates relative to the parent.
    // The canvas CTM is currently set to the parent's generic coordinate space.
    // So paint_rect matches the CTM directly.
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    
    // Aggressive culling: Skip if completely outside the clip.
    // Note: This relies on Skia's quickReject which accounts for the current transform (CTM) and clip.
    // We add a safety margin (50px) to account for shadows, outlines, or minor overflows.
    // For large overflows (overflow: visible), strictly speaking we shouldn't cull, 
    // but in practice large offscreen content is rare in well-designed apps.
    if (canvas->quickReject(paint_rect.makeOutset(50, 50))) {
        g_paint_culled_calls++;  // 统计：被剔除的调用
        needs_paint_ = false;
        return;
    }

    // P1优化：更新绘制缓存（如果无效则重新计算）
    UpdatePaintCache();

    const auto& style = computed_style_;
    const auto& layout = layout_info_;
    const auto& cache = paint_cache_;  // 使用缓存的值

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 应用 CSS transform
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        SkRect element_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(element_rect, style.transform_origin);
        canvas->concat(transform_matrix);
    }

    // 应用 CSS clip-path
    if (style.clip_path.has_value() && !style.clip_path->IsNone()) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPath clip_path = style.clip_path->ToSkPath(bounds);
        canvas->clipPath(clip_path, true);  // true = anti-alias
    }

    // 创建盒模型 - 使用缓存的值
    Box box;

    // 使用缓存的 padding 值
    box.padding_left = cache.padding_left;
    box.padding_right = cache.padding_right;
    box.padding_top = cache.padding_top;
    box.padding_bottom = cache.padding_bottom;

    // 使用缓存的 border 宽度
    box.border_top_width = cache.border_top_width;
    box.border_right_width = cache.border_right_width;
    box.border_bottom_width = cache.border_bottom_width;
    box.border_left_width = cache.border_left_width;

    // 使用缓存的内容区域偏移
    box.content_x = cache.content_x;
    box.content_y = cache.content_y;
    box.content_width = layout.width - cache.border_left_width - cache.border_right_width
                        - cache.padding_left - cache.padding_right;
    box.content_height = layout.height - cache.border_top_width - cache.border_bottom_width
                         - cache.padding_top - cache.padding_bottom;

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
            // 绘制水平线（使用相对坐标，因为已经 translate 过了）
            SkPaint line_paint;
            line_paint.setColor(style.border.color);
            line_paint.setStrokeWidth(style.border.width.ToPx());
            line_paint.setAntiAlias(true);

            float y = layout.height / 2;
            canvas->drawLine(0, y, layout.width, y, line_paint);
            canvas->restore(); // 恢复 canvas 状态
            return; // 不绘制其他内容
        }
        
        // 检查是否是 <canvas> 元素
        if (element->GetTagName() == "canvas") {
            auto canvas_element = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
            if (canvas_element) {
                auto context_2d = canvas_element->GetContext2D();
                if (context_2d) {
                    // 获取Canvas的Surface并绘制到屏幕
                    auto* surface = context_2d->GetSurface();
                    if (surface) {
                        auto image = surface->makeImageSnapshot();
                        if (image) {
                            // 绘制Canvas内容到content区域
                            SkRect dest_rect = SkRect::MakeXYWH(
                                box.content_x, box.content_y,
                                box.content_width, box.content_height
                            );
                            canvas->drawImageRect(image, dest_rect, SkSamplingOptions());
                        }
                    }
                }
            }
            // Canvas元素绘制完Surface后继续正常绘制背景边框等
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

    // 渲染边框 - 使用缓存的标志位
    if (cache.has_border) {
        SkRect border_box = box.GetBorderBox();

        // 使用缓存的圆角标志
        bool has_border_radius = cache.has_border_radius;

        if (has_border_radius) {
            // 有圆角：使用 RenderRoundedBorderAdvanced（支持每边独立属性）
            
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
            
            renderer.RenderRoundedBorderAdvanced(box, border_widths, border_styles, border_colors, style.border_radius);
            
        } else {
            // 无圆角：使用原有的独立边框渲染逻辑
            
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

            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                if (element->GetTagName() == "fieldset") {
                    is_fieldset = true;
                    // 查找 legend 子元素的渲染对象
                    for (auto& child : children_) {
                        auto child_node = child->GetNode();
                        if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto child_elem = std::static_pointer_cast<Element>(child_node);
                            if (child_elem->GetTagName() == "legend") {
                                legend_render = child.get();
                                auto& legend_layout = child->GetLayoutInfo();
                                auto& legend_style = child->GetComputedStyle();

                                // 计算 legend 的实际渲染宽度
                                // 遍历 legend 的子元素，找到最右边的位置
                                float max_child_right = 0.0f;
                                for (const auto& grandchild : child->GetChildren()) {
                                    auto& gc_layout = grandchild->GetLayoutInfo();
                                    max_child_right = std::max(max_child_right, gc_layout.x + gc_layout.width);
                                }

                                float legend_padding_right = legend_style.padding.right.ToPx();
                                float legend_border_right = legend_style.border_right_width > 0 ?
                                    legend_style.border_right_width : legend_style.border.width.ToPx();

                                // legend_left 是 legend 的左边缘（相对于 fieldset border-box）
                                legend_left = legend_layout.x;
                                // legend_right 是 legend 的右边缘
                                // = legend_left + 子元素最右边位置 + 右侧 padding + 右侧 border
                                legend_right = legend_layout.x + max_child_right + legend_padding_right + legend_border_right;
                                break;
                            }
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

            // 渲染上边框 - fieldset 需要特殊处理（在 legend 位置断开）
            if (box.border_top_width > 0) {
                CSSBorderStyle top_style = style.border_top_style != CSSBorderStyle::NONE ?
                                           style.border_top_style : style.border.style;
                SkColor top_color = style.border_top_style != CSSBorderStyle::NONE ?
                                    style.border_top_color : style.border.color;
                if (top_style != CSSBorderStyle::NONE) {
                    if (is_fieldset && legend_render) {
                        // fieldset 上边框在 legend 位置断开
                        // 绘制 legend 左边的部分
                        if (legend_left > border_box.left()) {
                            renderer.RenderBorderEdge(
                                border_box.left(), border_box.top() + half_top,
                                legend_left, border_box.top() + half_top,
                                box.border_top_width, top_style, top_color
                            );
                        }
                        // 绘制 legend 右边的部分
                        if (legend_right < border_box.right()) {
                            renderer.RenderBorderEdge(
                                legend_right, border_box.top() + half_top,
                                border_box.right(), border_box.top() + half_top,
                                box.border_top_width, top_style, top_color
                            );
                        }
                    } else {
                        // 普通元素：绘制完整上边框
                        renderer.RenderBorderEdge(
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
                    renderer.RenderBorderEdge(
                        border_box.left(), border_box.bottom() - half_bottom,
                        border_box.right(), border_box.bottom() - half_bottom,
                        box.border_bottom_width, bottom_style, bottom_color
                    );
                }
            }
        }
    }

    // ========== 绘制 outline（焦点指示器）==========
    // outline 不占用布局空间，紧贴边框外边缘绘制（符合浏览器行为）
    PaintOutline(canvas);

    // 绘制列表项目符号（如果是<li>元素）
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element2 = std::static_pointer_cast<Element>(node);
        if (element2->GetTagName() == "li") {
            // 获取父元素（ul或ol）
            auto parent_node = element2->GetParentNode();
            std::string parent_tag = "";

            // 向上查找最近的 ul 或 ol 祖先
            auto ancestor = parent_node;
            std::shared_ptr<Element> list_element = nullptr;
            while (ancestor) {
                if (ancestor->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto ancestor_elem = std::static_pointer_cast<Element>(ancestor);
                    std::string tag = ancestor_elem->GetTagName();
                    if (tag == "ul" || tag == "ol") {
                        parent_tag = tag;
                        list_element = ancestor_elem;
                        break;
                    }
                }
                ancestor = ancestor->GetParentNode();
            }

            if (!parent_tag.empty() && list_element) {
                auto parent_element = list_element;

                // 计算当前<li>在列表中的索引
                int item_index = 1;

                // 检查 <li> 是否有 value 属性
                std::string value_attr = element2->GetAttribute("value");
                if (!value_attr.empty()) {
                    try {
                        item_index = std::stoi(value_attr);
                    } catch (...) {
                        // 忽略解析错误
                    }
                } else {
                    // 获取 ol 的 start 属性
                    std::string start_attr = parent_element->GetAttribute("start");
                    int start_index = 1;
                    if (!start_attr.empty()) {
                        try {
                            start_index = std::stoi(start_attr);
                        } catch (...) {}
                    }

                    // 计算当前<li>在列表中的位置
                    int position = 0;
                    auto siblings = parent_element->GetChildNodes();
                    for (const auto& sibling : siblings) {
                        if (sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto sibling_elem = std::static_pointer_cast<Element>(sibling);
                            if (sibling_elem->GetTagName() == "li") {
                                if (sibling_elem == element2) {
                                    break;
                                }
                                // 检查前面的 li 是否有 value 属性
                                std::string prev_value = sibling_elem->GetAttribute("value");
                                if (!prev_value.empty()) {
                                    try {
                                        start_index = std::stoi(prev_value) + 1;
                                        position = 0;
                                    } catch (...) {}
                                }
                                position++;
                            }
                        }
                    }
                    item_index = start_index + position;
                }

                // Use the new PaintListMarker function
                PaintListMarker(canvas, style, layout, box, item_index, parent_tag);
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

        // ✅ 修复：窗口或内容尺寸变化后，重新限制滚动位置
        // 场景1：用户滚动到底部后，窗口变高，此时max_scroll变小，
        //       需要自动调整scroll_y_以保持在有效范围内
        // 场景2：窗口最大化后不再需要滚动条，需要重置滚动位置
        float max_scroll_x = GetMaxScrollX();
        float max_scroll_y = GetMaxScrollY();
        
        // 只在超出范围时调整（避免不必要的重绘标记）
        if (scroll_x_ > max_scroll_x || scroll_y_ > max_scroll_y) {
            scroll_x_ = std::max(0.0f, std::min(scroll_x_, max_scroll_x));
            scroll_y_ = std::max(0.0f, std::min(scroll_y_, max_scroll_y));
            // 注意：这里不调用MarkNeedsPaint()，因为我们已经在Paint中了
        }

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

    // 检查是否是 fieldset 元素
    bool is_fieldset_element = false;
    RenderObject* legend_child = nullptr;
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::static_pointer_cast<Element>(node);
        is_fieldset_element = (elem->GetTagName() == "fieldset");

        // 查找 legend 子元素
        if (is_fieldset_element) {
            for (auto& child : children_) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto child_elem = std::static_pointer_cast<Element>(child_node);
                    if (child_elem->GetTagName() == "legend") {
                        legend_child = child.get();
                        break;
                    }
                }
            }
        }
    }

    // 浏览器行为：当元素有 border-radius 时，子元素会被裁剪到圆角区域内
    // 即使没有设置 overflow: hidden
    // 但是 fieldset 的 legend 不应该被裁剪
    bool has_border_radius = style.border_radius.top_left.value > 0 ||
                             style.border_radius.top_right.value > 0 ||
                             style.border_radius.bottom_left.value > 0 ||
                             style.border_radius.bottom_right.value > 0;

    bool needs_radius_clip = has_border_radius && !needs_clip && !is_fieldset_element;
    if (needs_radius_clip) {
        canvas->save();
        SkRect clip_rect = box.GetPaddingBox();
        
        // 修复：计算 border-radius 百分比的基准尺寸
        float box_width = clip_rect.width();
        float box_height = clip_rect.height();
        float base_size = std::min(box_width, box_height);
        
        SkRRect rrect;
        float tl = style.border_radius.top_left.ToPx(base_size);
        float tr = style.border_radius.top_right.ToPx(base_size);
        float br = style.border_radius.bottom_right.ToPx(base_size);
        float bl = style.border_radius.bottom_left.ToPx(base_size);
        SkVector radii[4] = {
            {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
        };
        rrect.setRectRadii(clip_rect, radii);
        canvas->clipRRect(rrect, SkClipOp::kIntersect, true);
    }

    // 对于 fieldset，先在裁剪区域外绘制 legend
    // legend 的布局 y 坐标是 0（与 fieldset 的 y 坐标相同，用于 getBoundingClientRect）
    // 但是绘制时需要将 legend 居中于 fieldset 的上边框线
    if (is_fieldset_element && legend_child) {
        auto& legend_layout = legend_child->GetLayoutInfo();
        float original_y = legend_layout.y;

        // 计算绘制时的 y 坐标：legend 的垂直中心应该在 border_top / 2 的位置
        float legend_half_height = legend_layout.height / 2.0f;
        float paint_y = box.border_top_width / 2.0f - legend_half_height;

        // 临时修改位置进行渲染
        legend_layout.y = paint_y;
        legend_child->Paint(canvas);

        // 恢复原始位置（保持布局一致性）
        legend_layout.y = original_y;
    }

    // 绘制其他子元素（fieldset 的非 legend 子元素需要裁剪）
    if (is_fieldset_element && has_border_radius) {
        canvas->save();
        SkRect clip_rect = box.GetPaddingBox();
        
        // 修复：计算 border-radius 百分比的基准尺寸
        float box_width = clip_rect.width();
        float box_height = clip_rect.height();
        float base_size = std::min(box_width, box_height);
        
        SkRRect rrect;
        float tl = style.border_radius.top_left.ToPx(base_size);
        float tr = style.border_radius.top_right.ToPx(base_size);
        float br = style.border_radius.bottom_right.ToPx(base_size);
        float bl = style.border_radius.bottom_left.ToPx(base_size);
        SkVector radii[4] = {
            {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
        };
        rrect.setRectRadii(clip_rect, radii);
        canvas->clipRRect(rrect, SkClipOp::kIntersect, true);
    }

    for (auto& child : sorted_children) {
        // 跳过已经绘制的 legend
        if (is_fieldset_element && child.get() == legend_child) {
            continue;
        }
        child->Paint(canvas);
    }

    // 恢复 fieldset 的圆角裁剪状态
    if (is_fieldset_element && has_border_radius) {
        canvas->restore();
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
        // For body element, scrollbar should be drawn relative to viewport, not body
        // Save current transform and adjust for body's margin
        bool is_body = IsBodyElement();
        if (is_body) {
            canvas->save();
            // Translate back by body's position to draw scrollbar relative to viewport
            canvas->translate(-layout_info_.x, -layout_info_.y);
        }
        
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
        
        // Restore transform for body element
        if (is_body) {
            canvas->restore();
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
        float cx = box.content_x + box.content_width / 2;
        float cy = box.content_y + box.content_height / 2;

        if (type == InputType::Checkbox) {
            // 绘制 checkbox 方框边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);

            float size = std::min(box.content_width, box.content_height);
            float half = size / 2;
            SkRect checkbox_rect = SkRect::MakeXYWH(cx - half, cy - half, size, size);

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            canvas->drawRoundRect(checkbox_rect, 2, 2, bg_paint);

            // 边框
            canvas->drawRoundRect(checkbox_rect, 2, 2, border_paint);

            // 如果选中，绘制勾选标记
            if (checked) {
                SkPaint check_paint;
                check_paint.setColor(SK_ColorBLACK);
                check_paint.setStrokeWidth(2);
                check_paint.setStyle(SkPaint::kStroke_Style);
                check_paint.setAntiAlias(true);

                SkPath check_path;
                check_path.moveTo(cx - 4, cy);
                check_path.lineTo(cx - 1, cy + 3);
                check_path.lineTo(cx + 4, cy - 3);
                canvas->drawPath(check_path, check_paint);
            }
        }
        else if (type == InputType::Radio) {
            // 绘制 radio 外圆环
            float radius = std::min(box.content_width, box.content_height) / 2;

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            bg_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, bg_paint);

            // 边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, border_paint);

            // 如果选中，绘制内圆点
            if (checked) {
                SkPaint dot_paint;
                dot_paint.setColor(SK_ColorBLACK);
                dot_paint.setStyle(SkPaint::kFill_Style);
                dot_paint.setAntiAlias(true);

                float inner_radius = radius / 2;
                canvas->drawCircle(cx, cy, inner_radius, dot_paint);
            }
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
    // 🔍 调试：开始布局
    auto node = GetNode();
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

    // Enterprise-Grade Optimization: View Culling
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(10, 10))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 应用 CSS clip-path
    if (style.clip_path.has_value() && !style.clip_path->IsNone()) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPath clip_path = style.clip_path->ToSkPath(bounds);
        canvas->clipPath(clip_path, true);
    }

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

    // ========== 绘制 outline（焦点指示器）==========
    // outline 不占用布局空间，紧贴边框外边缘绘制（符合浏览器行为）
    PaintOutline(canvas);

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
        float cx = box.content_x + box.content_width / 2;
        float cy = box.content_y + box.content_height / 2;

        if (type == InputType::Checkbox) {
            // 绘制 checkbox 方框边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);

            float size = std::min(box.content_width, box.content_height);
            float half = size / 2;
            SkRect checkbox_rect = SkRect::MakeXYWH(cx - half, cy - half, size, size);

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            canvas->drawRoundRect(checkbox_rect, 2, 2, bg_paint);

            // 边框
            canvas->drawRoundRect(checkbox_rect, 2, 2, border_paint);

            // 如果选中，绘制勾选标记
            if (checked) {
                SkPaint check_paint;
                check_paint.setColor(SK_ColorBLACK);
                check_paint.setStrokeWidth(2);
                check_paint.setStyle(SkPaint::kStroke_Style);
                check_paint.setAntiAlias(true);

                SkPath check_path;
                check_path.moveTo(cx - 4, cy);
                check_path.lineTo(cx - 1, cy + 3);
                check_path.lineTo(cx + 4, cy - 3);
                canvas->drawPath(check_path, check_paint);
            }
        }
        else if (type == InputType::Radio) {
            // 绘制 radio 外圆环
            float radius = std::min(box.content_width, box.content_height) / 2;

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            bg_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, bg_paint);

            // 边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, border_paint);

            // 如果选中，绘制内圆点
            if (checked) {
                SkPaint dot_paint;
                dot_paint.setColor(SK_ColorBLACK);
                dot_paint.setStyle(SkPaint::kFill_Style);
                dot_paint.setAntiAlias(true);

                float inner_radius = radius / 2;
                canvas->drawCircle(cx, cy, inner_radius, dot_paint);
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

    // 计算 line-height
    // 如果 style.line_height 是默认值 1.2，使用浏览器风格的 line-height: normal
    // 否则使用用户指定的 line-height 倍数
    float line_height;
    if (std::abs(style.line_height - 1.2f) < 0.001f) {
        // 使用浏览器风格的 line-height: normal
        line_height = GetBrowserNormalLineHeight(style.font_size);
    } else {
        // 用户指定了具体的 line-height
        line_height = style.line_height * style.font_size;
    }

    // 检查是否包含换行符
    if (text_.find('\n') != std::string::npos) {
        // 多行文本：分别测量每一行，取最大宽度和累加高度
        std::istringstream iss(text_);
        std::string line;
        float max_width = 0;
        float total_height = 0;
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
    } else if (parent_width > 0) {
        // 有可用宽度限制，检查是否需要换行
        float text_width = text_renderer.MeasureTextWidthWithEmoji(text_, font);

        // 使用小容差值来避免浮点数精度问题
        // 当 text_width 和 parent_width 非常接近时，不应该换行
        const float epsilon = 0.01f;
        bool needs_wrap = text_width > parent_width + epsilon;

        if (needs_wrap) {
            // 文本超出可用宽度，需要换行
            std::vector<std::string> lines = text_renderer.WrapText(text_, parent_width, font);

            float max_width = 0;
            for (const auto& line : lines) {
                float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                max_width = std::max(max_width, line_width);
            }

            layout_info_.width = max_width;
            layout_info_.height = lines.size() * line_height;
        } else {
            // 文本不需要换行
            layout_info_.width = text_width;
            layout_info_.height = line_height;
        }
    } else {
        // 没有宽度限制，单行文本
        float width = text_renderer.MeasureTextWidthWithEmoji(text_, font);
        layout_info_.width = width;
        layout_info_.height = line_height;
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

    // Viewport Culling: Skip text outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(10, 10))) {
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

    // 计算 Skia 测量的精确文本高度
    float skia_text_height = -font_metrics.fAscent + font_metrics.fDescent;

    // 计算 CSS line-height
    float css_line_height = style.line_height * style.font_size;

    // 计算 half-leading：当 CSS line-height 大于文本高度时，
    // 额外空间应该平均分配在文本上下
    float half_leading = 0.0f;
    if (css_line_height > skia_text_height) {
        half_leading = (css_line_height - skia_text_height) / 2.0f;
    }

    // 计算基线位置：从顶部开始，加上 half-leading，再加上 ascent
    float baseline_y = half_leading + (-font_metrics.fAscent);

    // 处理 vertical-align
    if (style.vertical_align == "super") {
        // 上标：向上偏移 (约为字体大小的 0.4 倍)
        baseline_y -= style.font_size * 0.4f;
    } else if (style.vertical_align == "sub") {
        // 下标：向下偏移 (约为字体大小的 0.2 倍)
        baseline_y += style.font_size * 0.2f;
    }

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

    // Check for text-overflow: ellipsis from parent container
    bool use_ellipsis = false;
    float available_width = 0.0f;
    auto parent = GetParent();
    if (parent) {
        const auto& parent_style = parent->GetComputedStyle();
        if (parent_style.text_overflow == "ellipsis") {
            use_ellipsis = true;
            // Get parent's content width (layout width minus padding)
            const auto& parent_layout = parent->GetLayoutInfo();
            float padding_left = parent_style.padding.left.ToPx(parent_layout.width, parent_style.font_size);
            float padding_right = parent_style.padding.right.ToPx(parent_layout.width, parent_style.font_size);
            available_width = parent_layout.width - padding_left - padding_right;
        }
    }

    // Render all lines
    float current_y = baseline_y;
    float line_height = style.line_height * style.font_size;

    for (const auto& line : lines_to_render) {
        // Skip empty lines (but still advance y position)
        if (!line.empty()) {
            std::string text_to_render = line;

            // Apply text-transform (CSS text-transform property)
            // This transforms text for rendering only, DOM content remains unchanged
            if (!style.text_transform.empty() && style.text_transform != "none") {
                text_to_render = TransformText(text_to_render, style.text_transform);
            }

            // Apply text-overflow: ellipsis if needed
            if (use_ellipsis && available_width > 0) {
                float text_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                if (text_width > available_width) {
                    // Need to truncate and add ellipsis
                    const std::string ellipsis = "...";
                    float ellipsis_width = font.measureText(ellipsis.c_str(), ellipsis.length(), SkTextEncoding::kUTF8);
                    float target_width = available_width - ellipsis_width;

                    if (target_width > 0) {
                        // Binary search to find the right truncation point
                        std::string truncated;
                        size_t len = line.length();
                        size_t low = 0, high = len;

                        while (low < high) {
                            size_t mid = (low + high + 1) / 2;
                            // Handle UTF-8: find valid character boundary
                            size_t char_end = mid;
                            while (char_end > 0 && char_end < len && (line[char_end] & 0xC0) == 0x80) {
                                char_end--;
                            }
                            std::string test = line.substr(0, char_end);
                            float test_width = text_renderer.MeasureTextWidthWithEmoji(test, font);
                            if (test_width <= target_width) {
                                low = mid;
                                truncated = test;
                            } else {
                                high = mid - 1;
                            }
                        }

                        text_to_render = truncated + ellipsis;
                    } else {
                        // Not enough space even for ellipsis, just show ellipsis
                        text_to_render = ellipsis;
                    }
                }
            }

            if (!style.text_shadow.empty()) {
                ShadowRenderer::RenderTextWithShadow(canvas, text_to_render, font, 0, current_y, text_color, style.text_shadow);
            } else {
                lightui::Paint text_paint;
                text_paint.SetColor(text_color);
                // 使用支持emoji的文本渲染
                text_renderer.DrawTextWithEmoji(text_to_render, 0, current_y, font, text_paint);
            }
        }

        // Move to next line
        current_y += line_height;
    }

    // 绘制文本装饰（下划线、删除线等）- 需要为每一行绘制
    // 支持格式: "underline", "line-through", "underline dotted", "underline dashed"
    bool has_underline = style.text_decoration.find("underline") != std::string::npos;
    bool has_line_through = style.text_decoration.find("line-through") != std::string::npos;
    bool is_dotted = style.text_decoration.find("dotted") != std::string::npos;
    bool is_dashed = style.text_decoration.find("dashed") != std::string::npos;

    if (has_underline || has_line_through) {
        SkPaint line_paint;
        line_paint.setColor(text_color);
        line_paint.setAntiAlias(true);

        // 设置线条样式
        if (is_dotted) {
            const SkScalar intervals[] = {2.0f, 2.0f};
            line_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        } else if (is_dashed) {
            const SkScalar intervals[] = {4.0f, 2.0f};
            line_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        }

        float decoration_current_y = baseline_y;

        for (size_t i = 0; i < lines_to_render.size(); ++i) {
            const auto& line = lines_to_render[i];

            // Calculate line width (use actual text width for each line)
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            if (line_width <= 0) {
                decoration_current_y += line_height;
                continue;
            }

            if (has_underline) {
                // 下划线：在基线下方
                float underline_y = decoration_current_y + font_metrics.fUnderlinePosition;
                float underline_thickness = font_metrics.fUnderlineThickness;
                if (underline_thickness < 1.0f) underline_thickness = 1.0f;
                line_paint.setStrokeWidth(underline_thickness);
                canvas->drawLine(0, underline_y, line_width, underline_y, line_paint);
            }

            if (has_line_through) {
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

// ========== RenderTable 实现 ==========

void RenderTable::CollectColumnStyles() {
    column_background_colors_.clear();

    // 遍历子元素查找 colgroup 和 col
    for (auto& child : children_) {
        // 检查是否是 colgroup（通过检查 DOM 元素的标签名）
        auto node = child->GetNode();
        if (!node) continue;

        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) continue;

        std::string tag_name = element->GetTagName();
        std::transform(tag_name.begin(), tag_name.end(), tag_name.begin(), ::tolower);

        if (tag_name == "colgroup") {
            // colgroup 的背景色（作为默认值应用到其下的 col）
            std::string colgroup_bg = child->GetComputedStyle().background_color;

            // 遍历 colgroup 中的 col 元素
            for (auto& col_child : child->GetChildren()) {
                auto col_node = col_child->GetNode();
                if (!col_node) continue;

                auto col_element = std::dynamic_pointer_cast<Element>(col_node);
                if (!col_element) continue;

                std::string col_tag = col_element->GetTagName();
                std::transform(col_tag.begin(), col_tag.end(), col_tag.begin(), ::tolower);

                if (col_tag == "col") {
                    // 获取 col 的背景色，如果没有则使用 colgroup 的
                    std::string col_bg = col_child->GetComputedStyle().background_color;
                    if (col_bg.empty() || col_bg == "transparent") {
                        col_bg = colgroup_bg;
                    }

                    // 检查 span 属性
                    int span = 1;
                    std::string span_str = col_element->GetAttribute("span");
                    if (!span_str.empty()) {
                        try {
                            span = std::stoi(span_str);
                            if (span < 1) span = 1;
                        } catch (...) {
                            span = 1;
                        }
                    }

                    // 为每个跨越的列添加背景色
                    for (int i = 0; i < span; ++i) {
                        column_background_colors_.push_back(col_bg);
                    }
                }
            }
        }
        else if (tag_name == "col") {
            // 直接的 col 元素（没有 colgroup 包裹）
            std::string col_bg = child->GetComputedStyle().background_color;

            // 检查 span 属性
            int span = 1;
            std::string span_str = element->GetAttribute("span");
            if (!span_str.empty()) {
                try {
                    span = std::stoi(span_str);
                    if (span < 1) span = 1;
                } catch (...) {
                    span = 1;
                }
            }

            // 为每个跨越的列添加背景色
            for (int i = 0; i < span; ++i) {
                column_background_colors_.push_back(col_bg);
            }
        }
    }
}

void RenderTable::CalculateColumnWidths(float available_width) {
    // 收集所有行中的单元格来确定列数和宽度
    std::vector<float> min_widths;
    std::vector<float> preferred_widths;
    size_t max_columns = 0;

    // 辅助函数：计算一行的实际列数（考虑 colspan）
    auto count_row_columns = [](std::shared_ptr<RenderObject> row) -> size_t {
        size_t col_count = 0;
        for (auto& cell : row->GetChildren()) {
            int col_span = 1;
            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                col_span = table_cell->GetColSpan();
                if (col_span < 1) col_span = 1;
            }
            col_count += col_span;
        }
        return col_count;
    };

    // 遍历所有子元素（可能是 thead, tbody, tfoot 或直接的 tr）
    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();

        // 处理行组（thead, tbody, tfoot）
        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            for (auto& row : child->GetChildren()) {
                if (row->GetType() == RenderObjectType::TABLE_ROW) {
                    max_columns = std::max(max_columns, count_row_columns(row));
                }
            }
        }
        // 处理直接的行（tr）
        else if (child_type == RenderObjectType::TABLE_ROW) {
            max_columns = std::max(max_columns, count_row_columns(child));
        }
    }

    if (max_columns == 0) {
        column_widths_.clear();
        return;
    }

    // 初始化列宽度数组
    min_widths.resize(max_columns, 0.0f);
    preferred_widths.resize(max_columns, 0.0f);

    // 存储 colspan 单元格信息，用于后续处理
    struct ColspanCellInfo {
        size_t start_col;
        int col_span;
        float min_width;
        float preferred_width;
    };
    std::vector<ColspanCellInfo> colspan_cells;

    // 第一遍：处理 colspan=1 的单元格，收集 colspan>1 的单元格信息
    auto process_row_pass1 = [&](std::shared_ptr<RenderObject> row) {
        auto& cells = row->GetChildren();
        size_t logical_col = 0;
        for (size_t i = 0; i < cells.size() && logical_col < max_columns; ++i) {
            auto& cell = cells[i];
            const auto& cell_style = cell->GetComputedStyle();

            int col_span = 1;
            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                col_span = table_cell->GetColSpan();
                if (col_span < 1) col_span = 1;
            }

            float cell_padding_left = cell_style.padding.left.ToPx(available_width, cell_style.font_size);
            float cell_padding_right = cell_style.padding.right.ToPx(available_width, cell_style.font_size);
            float cell_border_left = cell_style.border.width.ToPx();
            float cell_border_right = cell_style.border.width.ToPx();
            float cell_extra = cell_padding_left + cell_padding_right + cell_border_left + cell_border_right;

            float content_min_width = 0;
            float content_preferred_width = 0;

            for (auto& cell_child : cell->GetChildren()) {
                cell_child->Layout(10000.0f, 0);
                auto& child_layout = cell_child->GetLayoutInfo();
                content_min_width = std::max(content_min_width, child_layout.width);
                content_preferred_width = std::max(content_preferred_width, child_layout.width);
            }

            if (cell_style.width.unit == CSSUnit::PX) {
                content_preferred_width = std::max(content_preferred_width, cell_style.width.value);
            }

            if (col_span == 1) {
                // colspan=1 的单元格直接设置列宽
                min_widths[logical_col] = std::max(min_widths[logical_col], content_min_width + cell_extra);
                preferred_widths[logical_col] = std::max(preferred_widths[logical_col], content_preferred_width + cell_extra);
            } else {
                // colspan>1 的单元格，记录下来后续处理
                colspan_cells.push_back({logical_col, col_span, content_min_width + cell_extra, content_preferred_width + cell_extra});
            }

            logical_col += col_span;
        }
    };

    // 遍历所有行（第一遍）
    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();

        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            for (auto& row : child->GetChildren()) {
                if (row->GetType() == RenderObjectType::TABLE_ROW) {
                    process_row_pass1(row);
                }
            }
        }
        else if (child_type == RenderObjectType::TABLE_ROW) {
            process_row_pass1(child);
        }
    }

    // 第二遍：处理 colspan>1 的单元格
    // 只有当 colspan 单元格的宽度超过其跨越列的总宽度时，才需要扩展列宽
    for (const auto& info : colspan_cells) {
        // 计算当前跨越列的总宽度
        float current_total_min = 0;
        float current_total_pref = 0;
        for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
            current_total_min += min_widths[info.start_col + j];
            current_total_pref += preferred_widths[info.start_col + j];
        }

        // 如果 colspan 单元格需要更多宽度，按比例分配额外宽度
        if (info.min_width > current_total_min) {
            float extra = info.min_width - current_total_min;
            float extra_per_col = extra / info.col_span;
            for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
                min_widths[info.start_col + j] += extra_per_col;
            }
        }
        if (info.preferred_width > current_total_pref) {
            float extra = info.preferred_width - current_total_pref;
            float extra_per_col = extra / info.col_span;
            for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
                preferred_widths[info.start_col + j] += extra_per_col;
            }
        }
    }

    // 计算表格边框和间距
    const auto& style = computed_style_;
    bool is_collapse = (style.border_collapse == "collapse");

    // 在 collapse 模式下，表格边框与单元格边框合并，不占用额外空间
    float table_border_left = is_collapse ? 0 : style.border.width.ToPx();
    float table_border_right = is_collapse ? 0 : style.border.width.ToPx();
    float table_padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float table_padding_right = style.padding.right.ToPx(available_width, style.font_size);

    // 计算实际可用于列的宽度
    float table_extra = table_border_left + table_border_right + table_padding_left + table_padding_right;
    float available_for_columns = available_width - table_extra;

    // 计算总最小宽度和总首选宽度
    float total_min_width = 0;
    float total_preferred_width = 0;
    for (size_t i = 0; i < max_columns; ++i) {
        total_min_width += min_widths[i];
        total_preferred_width += preferred_widths[i];
    }

    // 分配列宽度
    column_widths_.resize(max_columns);

    // 检查表格是否有显式宽度设置
    bool has_explicit_width = (style.width.unit == CSSUnit::PX || style.width.unit == CSSUnit::PERCENT);

    if (has_explicit_width) {
        // 表格有显式宽度，需要根据可用空间分配列宽
        if (total_preferred_width <= available_for_columns) {
            // 有足够空间，使用首选宽度并按比例分配剩余空间
            // 浏览器行为：按照内容宽度的比例分配剩余空间，而不是平均分配
            float extra_space = available_for_columns - total_preferred_width;
            for (size_t i = 0; i < max_columns; ++i) {
                // 按首选宽度的比例分配剩余空间
                float ratio = (total_preferred_width > 0) ? (preferred_widths[i] / total_preferred_width) : (1.0f / max_columns);
                column_widths_[i] = preferred_widths[i] + extra_space * ratio;
            }
        }
        else if (total_min_width <= available_for_columns) {
            // 空间不足以容纳首选宽度，但可以容纳最小宽度
            // 按比例分配剩余空间
            float extra_space = available_for_columns - total_min_width;
            float total_extra_needed = total_preferred_width - total_min_width;

            for (size_t i = 0; i < max_columns; ++i) {
                float extra_needed = preferred_widths[i] - min_widths[i];
                float extra = (total_extra_needed > 0) ? (extra_needed / total_extra_needed * extra_space) : 0;
                column_widths_[i] = min_widths[i] + extra;
            }
        }
        else {
            // 空间不足以容纳最小宽度，使用最小宽度
            column_widths_ = min_widths;
        }
    } else {
        // 表格宽度为 auto，使用内容的自然宽度（首选宽度）
        // 不进行拉伸，保持紧凑
        for (size_t i = 0; i < max_columns; ++i) {
            column_widths_[i] = preferred_widths[i];
        }
    }
}

void RenderTable::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 检查是否有显式宽度
    bool has_explicit_width = (style.width.unit == CSSUnit::PX || style.width.unit == CSSUnit::PERCENT);

    // 检查 border-collapse 模式
    bool is_collapse = (style.border_collapse == "collapse");

    // 获取 border-spacing（仅在 separate 模式下有效）
    // CSS 默认值是 2px（实际上浏览器默认是 0，但为了视觉效果使用 2px）
    float border_spacing = 0;
    if (!is_collapse) {
        // 在 separate 模式下，使用显式设置的值
        // 如果设置了非零值，使用它；否则使用默认值 2px
        if (style.border_spacing.value > 0) {
            border_spacing = style.border_spacing.ToPx();
        } else {
            border_spacing = 2.0f;  // 默认间距
        }
    }


    // 边框和padding
    float border_width = style.border.width.ToPx();
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);

    // 在 collapse 模式下，表格边框会与单元格边框合并
    // 所以表格本身不需要额外的边框空间
    float effective_border = is_collapse ? 0 : border_width;

    // 收集 colgroup/col 的样式（背景色等）
    CollectColumnStyles();

    float width;
    if (has_explicit_width) {
        // 有显式宽度，使用指定的宽度
        if (style.width.unit == CSSUnit::PX) {
            width = style.width.value;
        } else {
            width = style.width.value / 100.0f * parent_width;
        }
        // 计算列宽度（会拉伸以填满表格宽度）
        CalculateColumnWidths(width);
    } else {
        // 宽度为 auto，先计算内容需要的宽度
        // 传入一个大值来获取自然宽度
        CalculateColumnWidths(parent_width);

        // 计算所有列的总宽度
        float total_column_width = 0;
        for (float col_w : column_widths_) {
            total_column_width += col_w;
        }

        // 添加 border-spacing（separate 模式下列之间的间距）
        size_t num_columns = column_widths_.size();
        float total_spacing = is_collapse ? 0 : (border_spacing * (num_columns + 1));

        // 表格宽度 = 列宽度 + spacing + padding + border
        width = total_column_width + total_spacing + padding_left + padding_right + effective_border * 2;

        // 确保不超过父容器宽度
        width = std::min(width, parent_width);
    }

    // 布局表格内容
    float current_y = padding_top + effective_border + (is_collapse ? 0 : border_spacing);
    float content_width = width - effective_border * 2 - padding_left - padding_right;

    // 先布局 caption（在表格内容之前）
    for (auto& child : children_) {
        if (child->GetType() == RenderObjectType::TABLE_CAPTION) {
            child->Layout(content_width, 0);
            auto& child_layout = child->GetLayoutInfo();
            child_layout.x = padding_left + effective_border;
            child_layout.y = current_y;
            current_y += child_layout.height;
        }
    }

    // 收集所有行（用于 rowspan 处理）
    std::vector<std::shared_ptr<RenderTableRow>> all_rows;
    std::vector<std::shared_ptr<RenderObject>> row_groups;  // 记录行组以便后续设置布局

    auto collect_rows = [&](std::shared_ptr<RenderObject> container) {
        for (auto& child : container->GetChildren()) {
            if (child->GetType() == RenderObjectType::TABLE_ROW) {
                auto table_row = std::dynamic_pointer_cast<RenderTableRow>(child);
                if (table_row) {
                    all_rows.push_back(table_row);
                }
            }
        }
    };

    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();
        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            row_groups.push_back(child);
            collect_rows(child);
        } else if (child_type == RenderObjectType::TABLE_ROW) {
            auto table_row = std::dynamic_pointer_cast<RenderTableRow>(child);
            if (table_row) {
                all_rows.push_back(table_row);
            }
        }
    }

    // 跟踪 rowspan 占据的单元格
    std::vector<RenderTableRow::RowspanCell> active_rowspans;

    // 记录 rowspan 单元格信息，用于后续更新高度
    struct RowspanInfo {
        std::shared_ptr<RenderTableCell> cell;
        size_t start_row;  // 起始行索引
        int row_span;      // 跨越的行数
    };
    std::vector<RowspanInfo> rowspan_cells;

    // 第一遍：布局所有行，收集 rowspan 信息
    for (size_t row_idx = 0; row_idx < all_rows.size(); ++row_idx) {
        auto& table_row = all_rows[row_idx];

        // 设置行的基本属性
        table_row->SetColumnWidths(column_widths_);
        table_row->SetBorderSpacing(border_spacing);
        table_row->SetBorderCollapse(is_collapse);
        table_row->SetRowspanOccupiedCols(active_rowspans);

        // 布局行
        table_row->Layout(content_width, 0);
        auto& row_layout = table_row->GetLayoutInfo();
        row_layout.y = current_y;
        current_y += row_layout.height;

        // 更新 active_rowspans：处理当前行的单元格
        // 1. 减少现有 rowspan 的剩余行数，移除已完成的
        for (auto it = active_rowspans.begin(); it != active_rowspans.end(); ) {
            it->remaining_rows--;
            if (it->remaining_rows <= 0) {
                it = active_rowspans.erase(it);
            } else {
                ++it;
            }
        }

        // 2. 添加当前行中新的 rowspan 单元格
        size_t logical_col = 0;
        for (auto& cell : table_row->GetChildren()) {
            // 跳过被 rowspan 占据的列
            while (true) {
                bool occupied = false;
                for (const auto& rs : active_rowspans) {
                    if (rs.col_index == logical_col) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied) break;
                logical_col++;
            }

            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                int col_span = table_cell->GetColSpan();
                int row_span = table_cell->GetRowSpan();
                if (col_span < 1) col_span = 1;
                if (row_span < 1) row_span = 1;

                // 如果有 rowspan > 1，记录它
                if (row_span > 1) {
                    // 记录用于后续更新高度
                    RowspanInfo info;
                    info.cell = table_cell;
                    info.start_row = row_idx;
                    info.row_span = row_span;
                    rowspan_cells.push_back(info);

                    for (int c = 0; c < col_span; ++c) {
                        RenderTableRow::RowspanCell rs_cell;
                        rs_cell.col_index = logical_col + c;
                        rs_cell.remaining_rows = row_span - 1;
                        rs_cell.cell = std::dynamic_pointer_cast<RenderObject>(table_cell);
                        active_rowspans.push_back(rs_cell);
                    }
                }

                logical_col += col_span;
            } else {
                logical_col++;
            }
        }

        // 在 separate 模式下，行之间添加 border-spacing
        if (!is_collapse) {
            current_y += border_spacing;
        }
    }

    // 第二遍：更新 rowspan 单元格的高度并重新布局以应用垂直居中
    for (const auto& info : rowspan_cells) {
        size_t end_row = std::min(info.start_row + info.row_span, all_rows.size());
        if (end_row <= info.start_row) continue;

        // 计算跨越行的总高度
        float start_y = all_rows[info.start_row]->GetLayoutInfo().y;
        float end_y = all_rows[end_row - 1]->GetLayoutInfo().y +
                      all_rows[end_row - 1]->GetLayoutInfo().height;
        float total_height = end_y - start_y;

        // 添加行间距（如果有的话）
        if (!is_collapse && info.row_span > 1) {
            total_height += border_spacing * (info.row_span - 1);
        }

        // 获取单元格当前布局信息
        auto& cell_layout = info.cell->GetLayoutInfo();
        float cell_width = cell_layout.width;
        float cell_x = cell_layout.x;
        float cell_y = cell_layout.y;

        // 重新布局单元格，传入目标高度以应用垂直居中
        info.cell->Layout(cell_width, total_height);

        // 恢复位置（Layout 不设置 x/y）
        cell_layout.x = cell_x;
        cell_layout.y = cell_y;
        cell_layout.height = total_height;
    }

    // 设置独立行的 x 坐标
    for (auto& child : children_) {
        if (child->GetType() == RenderObjectType::TABLE_ROW) {
            auto& child_layout = child->GetLayoutInfo();
            child_layout.x = padding_left + effective_border + (is_collapse ? 0 : border_spacing);
        }
    }

    // 设置行组的布局信息
    for (auto& group : row_groups) {
        float min_y = std::numeric_limits<float>::max();
        float max_y = 0;
        for (auto& row : group->GetChildren()) {
            if (row->GetType() == RenderObjectType::TABLE_ROW) {
                auto& row_layout = row->GetLayoutInfo();
                min_y = std::min(min_y, row_layout.y);
                max_y = std::max(max_y, row_layout.y + row_layout.height);
                // 设置行在行组内的相对位置
                row_layout.x = 0;
            }
        }
        auto& group_layout = group->GetLayoutInfo();
        group_layout.x = padding_left + effective_border + (is_collapse ? 0 : border_spacing);
        group_layout.y = min_y;
        group_layout.width = content_width;
        group_layout.height = max_y - min_y;
        group_layout.is_laid_out = true;

        // 调整行在行组内的 y 坐标（相对于行组）
        for (auto& row : group->GetChildren()) {
            if (row->GetType() == RenderObjectType::TABLE_ROW) {
                auto& row_layout = row->GetLayoutInfo();
                row_layout.y -= min_y;
            }
        }
    }

    // 计算表格高度
    float height = current_y + padding_bottom + effective_border;

    // 应用显式高度（如果有）
    if (style.height.unit == CSSUnit::PX) {
        height = std::max(height, style.height.value);
    }

    // 设置布局信息
    layout_info_.width = width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTable::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Enterprise-Grade Optimization: View Culling
    // Tables can be large, so culling is important.
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(50, 50))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 检查 border-collapse 模式
    bool is_collapse = (style.border_collapse == "collapse");

    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 获取边框宽度
    float border_w = style.border.width.ToPx();

    // 创建盒模型
    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_top_width = border_w;
    box.border_right_width = border_w;
    box.border_bottom_width = border_w;
    box.border_left_width = border_w;
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.padding_left - box.padding_right - box.border_left_width - box.border_right_width;
    box.content_height = layout.height - box.padding_top - box.padding_bottom - box.border_top_width - box.border_bottom_width;

    BoxRenderer renderer(canvas);

    // 渲染背景
    SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    // 渲染子元素（先渲染子元素，这样表格边框可以覆盖在上面）
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    // 渲染表格边框
    // 在 collapse 模式下，表格边框画在最外层（覆盖单元格边框）
    // 在 separate 模式下，表格边框也正常画
    if (style.border.style != CSSBorderStyle::NONE && border_w > 0) {
        char width_str[32];
        snprintf(width_str, sizeof(width_str), "%.0fpx", border_w);
        std::string border_width = width_str;
        std::string border_style = "solid";
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        std::string border_color = color_str;
        renderer.RenderBorder(box, border_width, border_style, border_color);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableRowGroup 实现 ==========

void RenderTableRowGroup::Layout(float parent_width, float parent_height) {
    // 行组的布局由父表格处理
    // 这里只标记为已布局
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableRowGroup::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling: Skip row groups outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 渲染背景（如果有）
    const auto& style = computed_style_;
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    // 渲染子元素（行）
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableRow 实现 ==========

void RenderTableRow::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 计算行的padding和border
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);
    float border_width = style.border.width.ToPx();

    // 获取 border-spacing（在 separate 模式下单元格之间的间距）
    float spacing = border_collapse_ ? 0 : border_spacing_;

    // 辅助函数：检查某列是否被 rowspan 占据
    auto is_col_occupied = [this](size_t col) -> bool {
        for (const auto& rs : rowspan_occupied_cols_) {
            if (rs.col_index == col) {
                return true;
            }
        }
        return false;
    };

    // 辅助函数：获取某列被 rowspan 占据的宽度
    auto get_occupied_width = [this, &spacing](size_t col) -> float {
        if (col < column_widths_.size()) {
            float w = column_widths_[col];
            if (!border_collapse_) {
                w += spacing;
            }
            return w;
        }
        return 0;
    };

    // 布局每个单元格
    float current_x = 0;  // 在 separate 模式下，第一个单元格前的间距由表格处理
    float max_height = 0;

    auto& cells = children_;
    size_t logical_col = 0;  // 跟踪逻辑列索引（考虑 colspan 和 rowspan）

    for (size_t i = 0; i < cells.size(); ++i) {
        auto& cell = cells[i];

        // 跳过被 rowspan 占据的列
        while (is_col_occupied(logical_col)) {
            current_x += get_occupied_width(logical_col);
            logical_col++;
        }

        // 获取当前单元格的 colspan
        int col_span = 1;
        auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
        if (table_cell) {
            col_span = table_cell->GetColSpan();
            if (col_span < 1) col_span = 1;
            // 设置单元格的列索引（用于应用 col 的背景色）
            table_cell->SetColumnIndex(logical_col);
        }

        // 计算单元格宽度：从 logical_col 开始，跨越 col_span 列
        float cell_width = 0;
        int actual_cols = 0;
        for (int j = 0; j < col_span && (logical_col + j) < column_widths_.size(); ++j) {
            // 跳过被 rowspan 占据的列
            size_t target_col = logical_col + j;
            while (is_col_occupied(target_col) && target_col < column_widths_.size()) {
                target_col++;
            }
            if (target_col >= column_widths_.size()) break;

            cell_width += column_widths_[target_col];
            actual_cols++;
            // 在 separate 模式下，合并的列之间也有间距（除了第一列）
            if (actual_cols > 1 && !border_collapse_) {
                cell_width += spacing;
            }
        }

        // 如果没有列宽度信息，使用默认值
        if (cell_width == 0) {
            cell_width = 100.0f;
        }

        // 布局单元格
        cell->Layout(cell_width, parent_height);
        auto& cell_layout = cell->GetLayoutInfo();

        // 设置单元格位置
        cell_layout.x = current_x;
        cell_layout.y = 0;
        cell_layout.width = cell_width;

        current_x += cell_width;
        // 在 separate 模式下，单元格之间添加间距
        if (!border_collapse_) {
            current_x += spacing;
        }
        max_height = std::max(max_height, cell_layout.height);

        // 更新逻辑列索引
        logical_col += col_span;
    }

    // 跳过行末尾被 rowspan 占据的列（用于计算正确的行宽度）
    while (is_col_occupied(logical_col) && logical_col < column_widths_.size()) {
        current_x += get_occupied_width(logical_col);
        logical_col++;
    }

    // 统一所有单元格的高度
    for (auto& cell : cells) {
        auto& cell_layout = cell->GetLayoutInfo();
        cell_layout.height = max_height;
    }

    // 设置行的布局信息
    layout_info_.width = current_x;
    layout_info_.height = max_height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableRow::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling: Skip rows outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 渲染背景（如果有）
    const auto& style = computed_style_;
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    // 渲染单元格
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableCell 实现 ==========

void RenderTableCell::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 计算padding和border
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);
    float border_width = style.border.width.ToPx();

    // 计算内容区域宽度
    float content_width = parent_width - padding_left - padding_right - border_width * 2;

    // 布局子元素，先计算内容总高度
    float content_height = 0;
    for (auto& child : children_) {
        child->Layout(content_width, 0);
        content_height += child->GetLayoutInfo().height;
    }

    // 计算单元格自然高度（如果没有 parent_height 约束）
    float natural_height = content_height + padding_top + padding_bottom + border_width * 2;

    // 使用较大的高度（考虑行高统一或 rowspan）
    float height = std::max(natural_height, parent_height);

    // 计算垂直对齐偏移（根据 vertical-align 属性）
    float available_height = height - padding_top - padding_bottom - border_width * 2;
    float vertical_offset = 0;

    const std::string& v_align = style.vertical_align;
    if (v_align == "middle") {
        // 垂直居中
        vertical_offset = (available_height - content_height) / 2;
    } else if (v_align == "bottom") {
        // 底部对齐
        vertical_offset = available_height - content_height;
    } else {
        // top, baseline 或其他值：顶部对齐
        vertical_offset = 0;
    }
    if (vertical_offset < 0) vertical_offset = 0;

    // 设置子元素位置（应用垂直居中和水平对齐）
    float current_y = padding_top + border_width + vertical_offset;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();

        // 计算水平对齐偏移（根据 text-align 属性）
        float horizontal_offset = 0;
        const std::string& t_align = style.text_align;
        if (t_align == "center") {
            horizontal_offset = (content_width - child_layout.width) / 2;
        } else if (t_align == "right") {
            horizontal_offset = content_width - child_layout.width;
        }
        if (horizontal_offset < 0) horizontal_offset = 0;

        child_layout.x = padding_left + border_width + horizontal_offset;
        child_layout.y = current_y;
        current_y += child_layout.height;
    }

    // 设置布局信息
    layout_info_.width = parent_width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableCell::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling: Skip cells outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 获取边框宽度
    float border_w = style.border.width.ToPx();

    // 创建盒模型
    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_top_width = border_w;
    box.border_right_width = border_w;
    box.border_bottom_width = border_w;
    box.border_left_width = border_w;
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.padding_left - box.padding_right - box.border_left_width - box.border_right_width;
    box.content_height = layout.height - box.padding_top - box.padding_bottom - box.border_top_width - box.border_bottom_width;

    BoxRenderer renderer(canvas);

    // 渲染背景
    SkRect bounds = SkRect::MakeWH(layout.width, layout.height);

    // 首先检查 col 的背景色（优先级低于单元格自身的背景色）
    std::string effective_bg_color;

    // 查找父表格来获取列的背景色
    auto parent = GetParent();
    while (parent) {
        if (parent->GetType() == RenderObjectType::TABLE) {
            auto table = std::dynamic_pointer_cast<RenderTable>(parent);
            if (table) {
                std::string col_bg = table->GetColumnBackgroundColor(column_index_);
                if (!col_bg.empty() && col_bg != "transparent") {
                    effective_bg_color = col_bg;
                }
            }
            break;
        }
        parent = parent->GetParent();
    }

    // 单元格自身的背景色优先级更高
    if (!style.background_color.empty() && style.background_color != "transparent") {
        effective_bg_color = style.background_color;
    }

    // 绘制背景
    if (!effective_bg_color.empty()) {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(effective_bg_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    // 渲染边框
    if (style.border.style != CSSBorderStyle::NONE && border_w > 0) {
        char width_str[32];
        snprintf(width_str, sizeof(width_str), "%.0fpx", border_w);
        std::string border_width = width_str;
        std::string border_style = "solid";
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        std::string border_color = color_str;
        renderer.RenderBorder(box, border_width, border_style, border_color);
    }

    // 渲染子元素
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableCaption 实现 ==========

void RenderTableCaption::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 计算padding
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);

    // 计算内容区域宽度
    float content_width = parent_width - padding_left - padding_right;

    // 布局子元素
    float content_height = 0;
    for (auto& child : children_) {
        child->Layout(content_width, 0);
        auto& child_layout = child->GetLayoutInfo();

        // 根据 text-align 计算水平偏移
        float horizontal_offset = 0;
        const std::string& t_align = style.text_align;
        if (t_align == "center") {
            horizontal_offset = (content_width - child_layout.width) / 2;
        } else if (t_align == "right") {
            horizontal_offset = content_width - child_layout.width;
        }
        // left 或其他值：horizontal_offset = 0

        child_layout.x = padding_left + horizontal_offset;
        child_layout.y = padding_top + content_height;
        content_height += child_layout.height;
    }

    // 计算标题高度
    float height = content_height + padding_top + padding_bottom;

    // 设置布局信息
    layout_info_.width = parent_width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableCaption::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling: Skip captions outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 渲染背景
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    // 渲染子元素
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

} // namespace lightui

