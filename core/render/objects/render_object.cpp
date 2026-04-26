/**
 * @file render_object.cpp
 * @brief 渲染对象实现
 *
 * @note 大文件说明 (4197 行)
 * 本文件包含 RenderObject 类的完整实现，是渲染系统的核心组件。
 * 文件较大的原因：
 * 1. RenderObject 是所有渲染对象的基类，包含布局、绘制、滚动等核心功能
 * 2. 包含复杂的 CSS 属性处理逻辑（边框、背景、阴影、变换等）
 * 3. 包含滚动条渲染和交互逻辑
 * 4. 包含增量更新和缓存管理逻辑
 *
 * 已完成重构：
 * - RenderTable 系列实现已迁移到 render_table.cpp
 *
 * 计划重构：
 * - 提取 RenderBlock 到 render_block.cpp
 * - 提取 RenderInline 到 render_inline.cpp
 * - 提取 RenderText 到 render_text.cpp
 * 参见: .kiro/specs/render-object-refactoring/tasks.md
 */

#include "render_object.h"
#include "render_inline_block.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/text_transform.h"
#include "core/render/utils/gradient_renderer.h"
#include "core/render/utils/shadow_renderer.h"
#include "scrollbar_controller.h"
#include "list_marker.h"
#include "core/render/layer/paint_layer.h"
#include "core/render/utils/color.h"
#include "core/render/css/css_value.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/dom/selection/selection.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_canvas_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include "core/compositor/compositor_layer.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#undef min
#undef max
#undef ERROR
#endif
#include "include/core/SkPathEffect.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkDashPathEffect.h"

namespace mbink {

namespace {
std::atomic<size_t> g_render_object_live_count{0};
}

// 静态成员初始化
float RenderObject::viewport_width_ = 0.0f;

void RenderObject::SetComputedStyle(const ComputedStyle& style) {
    computed_style_ = style;
    paint_cache_.valid = false;
    boundary_cache_valid_ = false;
}

void RenderObject::MarkNeedsLayout(bool propagate_to_parent) {
    needs_layout_ = true;
    content_width_ = 0.0f;
    content_height_ = 0.0f;
    if (propagate_to_parent) {
        auto parent = parent_.lock();
        if (parent) {
            parent->MarkNeedsLayout(true);
        }
        MarkAncestorsWithChildNeedsLayout();
    }
}


float RenderObject::viewport_height_ = 0.0f;
bool RenderObject::cursor_visible_ = true;

// 视口剔除调试统计（用于验证 quickReject 效果）
std::atomic<int> g_paint_total_calls{0};
std::atomic<int> g_paint_culled_calls{0};

void RenderObject::ResetPaintStats() {
    g_paint_total_calls = 0;
    g_paint_culled_calls = 0;
}

void RenderObject::PrintPaintStats() {
    int total = g_paint_total_calls.load();
    int culled = g_paint_culled_calls.load();
    int painted = total - culled;
    float cull_rate = total > 0 ? (culled * 100.0f / total) : 0.0f;
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

    // CSS 的 max-height 默认作用于 content-box。
    // Paint/scrollbar/clip 路径这里需要返回 border-box 可见高度，
    // 否则会把 padding 区也错误地挤进 max-height 里，导致可见行数偏多、
    // 半行距被裁掉，视觉上像“没有行间距”。
    float max_height_px = computed_style_.max_height.ToPx();
    if (max_height_px > 0) {
        float visible_height = max_height_px;

        if (computed_style_.box_sizing != "border-box") {
            float reference_width = layout_info_.width > 0 ? layout_info_.width : 0.0f;
            float padding_top = computed_style_.padding.top.ToPx(reference_width, computed_style_.font_size);
            float padding_bottom = computed_style_.padding.bottom.ToPx(reference_width, computed_style_.font_size);
            float border_top = computed_style_.border_top_width > 0 ?
                computed_style_.border_top_width : computed_style_.border.width.ToPx(reference_width, computed_style_.font_size);
            float border_bottom = computed_style_.border_bottom_width > 0 ?
                computed_style_.border_bottom_width : computed_style_.border.width.ToPx(reference_width, computed_style_.font_size);
            visible_height += padding_top + padding_bottom + border_top + border_bottom;
        }

        if (visible_height < layout_info_.height) {
            return visible_height;
        }
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
    g_render_object_live_count.fetch_add(1, std::memory_order_relaxed);
}

RenderObject::~RenderObject() {
    g_render_object_live_count.fetch_sub(1, std::memory_order_relaxed);
}

size_t RenderObject::GetLiveObjectCount() {
    return g_render_object_live_count.load(std::memory_order_relaxed);
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

    // 同步更新 PaintLayer 树
    if (child->NeedsPaintLayer()) {
        PaintLayer* child_layer = child->EnsurePaintLayer();
        if (child_layer && paint_layer_) {
            paint_layer_->AddChild(child_layer);
        }
    }

    MarkNeedsLayout();
    MarkNeedsPaint();
}

void RenderObject::RemoveChild(std::shared_ptr<RenderObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        // 同步更新 PaintLayer 树
        PaintLayer* child_layer = (*it)->GetPaintLayer();
        if (child_layer && paint_layer_) {
            paint_layer_->RemoveChild(child_layer);
        }

        (*it)->SetParent(nullptr);
        children_.erase(it);
        MarkNeedsLayout();
        MarkNeedsPaint();
    }
}

void RenderObject::RemoveAllChildren() {
    for (auto& child : children_) {
        // 同步更新 PaintLayer 树
        PaintLayer* child_layer = child->GetPaintLayer();
        if (child_layer && paint_layer_) {
            paint_layer_->RemoveChild(child_layer);
        }

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

    // 转换 ComputedStyle 到布局 Style（统一样式存储）
    layout_style_ = ConvertComputedStyleToLayoutStyle(computed_style_);

    // 更新 Grid 特有样式（这些属性不在统一的 Style 中）
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

    SkRect base_rect = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);

    // 关键修复：如果元素有 transform，需要计算变换后的边界框
    // 这确保脏区域能正确覆盖变换后的渲染区域
    const auto& style = computed_style_;
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        // 创建以元素中心为原点的局部矩形
        SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);

        // 获取变换矩阵
        SkMatrix transform_matrix = style.transform->ToSkMatrix(local_rect, style.transform_origin);

        // 变换四个角点，计算包围盒
        SkPoint corners[4] = {
            {0, 0},
            {layout.width, 0},
            {layout.width, layout.height},
            {0, layout.height}
        };
        transform_matrix.mapPoints(corners, 4);

        // 计算变换后的边界框
        float min_x = corners[0].x(), max_x = corners[0].x();
        float min_y = corners[0].y(), max_y = corners[0].y();
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x());
            max_x = std::max(max_x, corners[i].x());
            min_y = std::min(min_y, corners[i].y());
            max_y = std::max(max_y, corners[i].y());
        }

        // 转换回文档坐标
        return SkRect::MakeLTRB(
            abs_x + min_x,
            abs_y + min_y,
            abs_x + max_x,
            abs_y + max_y
        );
    }

    return base_rect;
}

SkRect RenderObject::GetBoundingRectRelativeTo(const RenderObject* ancestor) const {
    // 计算相对于指定祖先 RenderObject 的边界矩形。
    // 坐标累加逻辑与 LayerTreeBuilder::UpdateLayerBounds 中 rel_x/rel_y 保持一致，
    // 从而保证 CollectDirtyRectsForLayer 的脏区域坐标与层 bounds 坐标系统一。
    const auto& layout = layout_info_;

    if (!layout.is_laid_out) {
        return SkRect::MakeEmpty();
    }

    // 如果没有指定祖先，回退到完整的文档坐标
    if (!ancestor) {
        return GetBoundingRect();
    }

    // 累加 layout.x/y 直到到达指定的祖先 RenderObject（不包含祖先本身的 layout 偏移）
    float rel_x = layout.x;
    float rel_y = layout.y;

    auto parent = parent_.lock();
    while (parent && parent.get() != ancestor) {
        const auto& parent_layout = parent->GetLayoutInfo();
        rel_x += parent_layout.x;
        rel_y += parent_layout.y;
        parent = parent->GetParent();
    }

    SkRect base_rect = SkRect::MakeXYWH(rel_x, rel_y, layout.width, layout.height);

    // 如果元素有 transform，计算变换后的边界框
    const auto& style = computed_style_;
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(local_rect, style.transform_origin);

        SkPoint corners[4] = {
            {0, 0},
            {layout.width, 0},
            {layout.width, layout.height},
            {0, layout.height}
        };
        transform_matrix.mapPoints(corners, 4);

        float min_x = corners[0].x(), max_x = corners[0].x();
        float min_y = corners[0].y(), max_y = corners[0].y();
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x());
            max_x = std::max(max_x, corners[i].x());
            min_y = std::min(min_y, corners[i].y());
            max_y = std::max(max_y, corners[i].y());
        }

        return SkRect::MakeLTRB(
            rel_x + min_x,
            rel_y + min_y,
            rel_x + max_x,
            rel_y + max_y
        );
    }

    return base_rect;
}

SkRect RenderObject::GetViewportBoundingRect() const {
    // 使用布局信息计算边界框（视口坐标系，用于元素选择器高亮）
    const auto& layout = layout_info_;

    // 如果布局信息无效，返回空矩形
    if (!layout.is_laid_out) {
        return SkRect::MakeEmpty();
    }

    // 检查是否是 position: fixed 元素
    const auto& style = computed_style_;
    bool is_fixed = (style.position == "fixed");

    // 计算绝对位置
    float abs_x = layout.x;
    float abs_y = layout.y;

    // 对于 fixed 元素，layout.x/y 已经是视口绝对坐标，不需要累加父元素偏移
    // 对于非 fixed 元素，需要累加所有祖先的偏移并减去滚动
    if (!is_fixed) {
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
    }

    SkRect base_rect = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);

    // 关键修复：如果元素有 transform，需要计算变换后的边界框
    // 这确保脏区域能正确覆盖变换后的渲染区域
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        // 创建以元素中心为原点的局部矩形
        SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);

        // 获取变换矩阵
        SkMatrix transform_matrix = style.transform->ToSkMatrix(local_rect, style.transform_origin);

        // 变换四个角点，计算包围盒
        SkPoint corners[4] = {
            {0, 0},
            {layout.width, 0},
            {layout.width, layout.height},
            {0, layout.height}
        };
        transform_matrix.mapPoints(corners, 4);

        // 计算变换后的边界框
        float min_x = corners[0].x(), max_x = corners[0].x();
        float min_y = corners[0].y(), max_y = corners[0].y();
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x());
            max_x = std::max(max_x, corners[i].x());
            min_y = std::min(min_y, corners[i].y());
            max_y = std::max(max_y, corners[i].y());
        }

        // 转换回视口坐标
        return SkRect::MakeLTRB(
            abs_x + min_x,
            abs_y + min_y,
            abs_x + max_x,
            abs_y + max_y
        );
    }

    return base_rect;
}

void RenderObject::MarkNeedsPaint() {
    // 🐛 hover bug 调试日志
    static bool debug_hover = std::getenv("MBINK_DEBUG_HOVER_BUG") != nullptr;
    if (debug_hover) {
        std::string tag = "unknown";
        std::string id = "";
        auto node_locked = node_.lock();
        if (node_locked) {
            if (node_locked->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::dynamic_pointer_cast<Element>(node_locked);
                if (elem) {
                    tag = elem->GetTagName();
                    id = elem->GetAttribute("id");
                }
            } else if (node_locked->GetNodeType() == NodeType::TEXT_NODE) {
                tag = "#text";
            }
        }
    }

    needs_paint_ = true;
    MarkAncestorsWithChildNeedsPaint();
}

void RenderObject::MarkAncestorsWithChildNeedsPaint() {
    // 向上传播 child_needs_paint_ 标志到所有祖先节点
    // 这是增量绘制优化的关键：允许跳过不需要重绘的子树
    auto parent = parent_.lock();
    while (parent) {
        // 如果祖先已经标记了 child_needs_paint_，则无需继续
        // 因为更上层的祖先也已经被标记过了
        if (parent->child_needs_paint_) {
            break;
        }
        parent->child_needs_paint_ = true;
        parent = parent->GetParent();
    }
}

void RenderObject::MarkAncestorsWithChildNeedsLayout() {
    // 向上传播 child_needs_layout_ 标志到所有祖先节点
    // 这是增量布局优化的关键：允许跳过不需要布局的子树
    auto parent = parent_.lock();
    while (parent) {
        // 如果祖先已经标记了 child_needs_layout_，则无需继续
        if (parent->child_needs_layout_) {
            break;
        }
        parent->child_needs_layout_ = true;
        parent = parent->GetParent();
    }
}

void RenderObject::ScrollBy(float dx, float dy) {
    float new_x = scroll_x_ + dx;
    float new_y = scroll_y_ + dy;
    ScrollTo(new_x, new_y);
}

void RenderObject::ScrollTo(float x, float y) {
    // 🔍 DEBUG: 增量更新问题调试
    static bool debug_scroll = std::getenv("DEBUG_INCREMENTAL_PAINT") != nullptr;

    // 限制滚动范围
    float max_x = GetMaxScrollX();
    float max_y = GetMaxScrollY();

    float old_scroll_x = scroll_x_;
    float old_scroll_y = scroll_y_;

    scroll_x_ = std::max(0.0f, std::min(x, max_x));
    scroll_y_ = std::max(0.0f, std::min(y, max_y));

    // 只有滚动位置真正改变时才标记重绘
    if (scroll_x_ != old_scroll_x || scroll_y_ != old_scroll_y) {
        if (debug_scroll) {
            auto node = GetNode();
            std::string tag_name = "?";
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::static_pointer_cast<Element>(node);
                tag_name = elem->GetTagName();
            }
        }

        MarkNeedsPaint();

        // 关键修复：滚动时使所有子孙元素的 ViewportBounds 缓存失效
        // 因为子元素的视口坐标改变了
        InvalidateDescendantViewportBounds();

        // 关键修复：滚动时，标记所有子元素也需要重绘
        // 因为子元素的视觉位置改变了（即使布局位置没变）
        std::function<void(RenderObject*, int)> mark_children = [&](RenderObject* obj, int depth) {
            if (!obj) return;
            obj->MarkNeedsPaint();

            if (debug_scroll && depth <= 2) {
                auto child_node = obj->GetNode();
                std::string child_tag = "?";
                std::string child_type = "?";
                if (child_node) {
                    if (child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto elem = std::static_pointer_cast<Element>(child_node);
                        child_tag = elem->GetTagName();
                    } else if (child_node->GetNodeType() == NodeType::TEXT_NODE) {
                        child_tag = "TEXT";
                        auto text_node = std::static_pointer_cast<Text>(child_node);
                        child_type = text_node->GetData().substr(0, 20);
                    }
                }
            }

            for (const auto& child : obj->GetChildren()) {
                mark_children(child.get(), depth + 1);
            }
        };
        mark_children(this, 0);

        if (debug_scroll) {
        }
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
    bool scrollable = (allow_h_scroll && content_width_ > visible_width) ||
                      (allow_v_scroll && content_height_ > visible_height);

    return scrollable;
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

    // 使用缓存的内容尺寸（在 Paint 中已计算并缓存）
    // 如果缓存无效（首次调用或布局后），则动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

    // 检查是否需要垂直滚动条
    bool needs_v_scroll = content_height > visible_height;

    // 可用内容宽度需要减去垂直滚动条宽度
    float available_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
    float max_scroll = std::max(0.0f, content_width - available_width);

    return max_scroll;
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

    // 使用缓存的内容尺寸（在 Paint 中已计算并缓存）
    // 如果缓存无效（首次调用或布局后），则动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

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

float RenderObject::GetScrollWidth() const {
    // **Feature: unified-scrollbar-system**
    // **Validates: Requirements 4.1**
    // 返回内容总宽度，包括溢出部分

    // 使用缓存的内容尺寸（在 Paint 中已计算并缓存）
    // 如果缓存无效（首次调用或布局后），则动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();

    // scrollWidth 至少等于元素的可见宽度
    float visible_width = GetEffectiveVisibleWidth();

    // 计算 border 宽度
    const auto& style = computed_style_;
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();

    // 可见内容区域宽度（不包括 border）
    float client_width = visible_width - border_left - border_right;

    return std::max(content_width, client_width);
}

float RenderObject::GetScrollHeight() const {
    // **Feature: unified-scrollbar-system**
    // **Validates: Requirements 4.2**
    // 返回内容总高度，包括溢出部分

    // 使用缓存的内容尺寸（在 Paint 中已计算并缓存）
    // 如果缓存无效（首次调用或布局后），则动态计算
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

    // scrollHeight 至少等于元素的可见高度
    float visible_height = GetEffectiveVisibleHeight();

    // 计算 border 宽度
    const auto& style = computed_style_;
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 可见内容区域高度（不包括 border）
    float client_height = visible_height - border_top - border_bottom;

    return std::max(content_height, client_height);
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
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

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

    scrollbar_controller_.StartDrag(area, mouse_x, mouse_y, scroll_x_, scroll_y_);
}

bool RenderObject::UpdateScrollbarDrag(float mouse_x, float mouse_y, float& out_scroll_x, float& out_scroll_y) {
    if (!scrollbar_controller_.IsDragging()) {
        return false;
    }

    // 计算 border 宽度
    float border_left = computed_style_.border_left_width > 0 ? computed_style_.border_left_width : computed_style_.border.width.ToPx();
    float border_right = computed_style_.border_right_width > 0 ? computed_style_.border_right_width : computed_style_.border.width.ToPx();
    float border_top = computed_style_.border_top_width > 0 ? computed_style_.border_top_width : computed_style_.border.width.ToPx();
    float border_bottom = computed_style_.border_bottom_width > 0 ? computed_style_.border_bottom_width : computed_style_.border.width.ToPx();

    // 对于 body 元素使用视口尺寸
    float effective_width = GetEffectiveVisibleWidth();
    float effective_height = GetEffectiveVisibleHeight();

    // 如果 content_width_/height_ 还没初始化，动态计算
    float content_width = content_width_ > 0 ? content_width_ : CalculateContentWidth();
    float content_height = content_height_ > 0 ? content_height_ : CalculateContentHeight();

    // 构建拖动参数
    ScrollbarDragParams params;
    params.visible_width = effective_width;
    params.visible_height = effective_height;
    params.content_width = content_width;
    params.content_height = content_height;
    params.border_left = border_left;
    params.border_right = border_right;
    params.border_top = border_top;
    params.border_bottom = border_bottom;

    out_scroll_x = scroll_x_;
    out_scroll_y = scroll_y_;
    return scrollbar_controller_.UpdateDrag(mouse_x, mouse_y, params, out_scroll_x, out_scroll_y);
}

void RenderObject::UpdateScrollbarDrag(float mouse_x, float mouse_y) {
    float new_scroll_x = scroll_x_;
    float new_scroll_y = scroll_y_;

    if (UpdateScrollbarDrag(mouse_x, mouse_y, new_scroll_x, new_scroll_y)) {
        // 关键修复：使用 ScrollTo 而不是直接设置 scroll_x_/scroll_y_
        // ScrollTo 会调用 InvalidateDescendantViewportBounds()，
        // 确保子元素的 ViewportBounds 缓存被正确失效，
        // 这样 hit testing 才能正确计算滚动后的坐标
        ScrollTo(new_scroll_x, new_scroll_y);
    }
}

void RenderObject::EndScrollbarDrag() {
    scrollbar_controller_.EndDrag();
}

float RenderObject::CalculateContentHeight() const {
    // 使用迭代方式代替递归，避免深层嵌套时栈溢出
    // 辅助函数：检查是否设置了 overflow 隐藏/滚动
    auto hasOverflowClip = [](const ComputedStyle& s) {
        std::string oy = !s.overflow_y.empty() ? s.overflow_y : s.overflow;
        return oy == "scroll" || oy == "auto" || oy == "hidden";
    };

    // 辅助函数：检查是否是 out-of-flow 定位（fixed 或 absolute）
    // 这些元素脱离文档流，不应该参与父元素的 content_size 计算
    auto isOutOfFlow = [](const ComputedStyle& s) {
        return s.position == "fixed" || s.position == "absolute";
    };

    // 使用栈来模拟递归：存储 (RenderObject*, 累计偏移Y)
    struct StackItem {
        const RenderObject* obj;
        float offset_y;  // 从根到此节点的累计Y偏移
    };

    std::vector<StackItem> stack;
    float global_max_height = 0.0f;

    // 初始化：将所有直接子元素加入栈（跳过 out-of-flow 元素）
    for (const auto& child : children_) {
        const auto& child_style = child->GetComputedStyle();
        bool child_out_of_flow = isOutOfFlow(child_style);
        if (!child_out_of_flow) {
            stack.push_back({child.get(), 0.0f});
        }
    }

    while (!stack.empty()) {
        StackItem item = stack.back();
        stack.pop_back();

        const RenderObject* obj = item.obj;
        if (!obj) continue;

        const auto& obj_layout = obj->GetLayoutInfo();
        const auto& obj_style = obj->GetComputedStyle();

        float obj_y = item.offset_y + obj_layout.y;
        float obj_height = obj_layout.height;
        bool clip = hasOverflowClip(obj_style);

        // 更新全局最大高度
        global_max_height = std::max(global_max_height, obj_y + obj_height);

        // 如果没有 overflow clip，继续遍历子元素
        if (!clip) {
            const auto& obj_children = obj->GetChildren();
            for (const auto& grandchild : obj_children) {
                const auto& grandchild_style = grandchild->GetComputedStyle();
                bool grandchild_out_of_flow = isOutOfFlow(grandchild_style);
                if (!grandchild_out_of_flow) {
                    // 子元素的偏移 = 当前元素的绝对Y位置
                    stack.push_back({grandchild.get(), obj_y});
                }
            }
        }
    }

    // 处理最后一个子元素的 margin-bottom
    float last_margin_bottom = 0.0f;
    if (!children_.empty()) {
        const auto& last_child = children_.back();
        const auto& last_child_style = last_child->GetComputedStyle();
        last_margin_bottom = last_child_style.margin.bottom.ToPx(layout_info_.width, last_child_style.font_size);
        global_max_height += last_margin_bottom;
    }

    // For body element, add body's own margin-bottom
    float body_margin_bottom = 0.0f;
    if (IsBodyElement()) {
        body_margin_bottom = computed_style_.margin.bottom.ToPx(viewport_height_, computed_style_.font_size);
        global_max_height += body_margin_bottom;
    }

    // 添加容器自身的 padding-bottom
    float padding_bottom = computed_style_.padding.bottom.ToPx(layout_info_.width, computed_style_.font_size);
    global_max_height += padding_bottom;

    return global_max_height;
}

float RenderObject::CalculateContentWidth() const {
    // 使用迭代方式代替递归，避免深层嵌套时栈溢出
    auto hasOverflowClip = [](const ComputedStyle& s) {
        std::string ox = !s.overflow_x.empty() ? s.overflow_x : s.overflow;
        return ox == "scroll" || ox == "auto" || ox == "hidden";
    };

    // 辅助函数：检查是否是 out-of-flow 定位（fixed 或 absolute）
    // 这些元素脱离文档流，不应该参与父元素的 content_size 计算
    auto isOutOfFlow = [](const ComputedStyle& s) {
        return s.position == "fixed" || s.position == "absolute";
    };

    // 使用栈来模拟递归：存储 (RenderObject*, 累计偏移X)
    struct StackItem {
        const RenderObject* obj;
        float offset_x;  // 从根到此节点的累计X偏移
    };

    std::vector<StackItem> stack;
    float global_max_width = 0.0f;

    // 初始化：将所有直接子元素加入栈（跳过 out-of-flow 元素）
    for (const auto& child : children_) {
        const auto& child_style = child->GetComputedStyle();
        bool child_out_of_flow = isOutOfFlow(child_style);
        if (!child_out_of_flow) {
            stack.push_back({child.get(), 0.0f});
        }
    }

    while (!stack.empty()) {
        StackItem item = stack.back();
        stack.pop_back();

        const RenderObject* obj = item.obj;
        if (!obj) continue;

        const auto& obj_layout = obj->GetLayoutInfo();
        const auto& obj_style = obj->GetComputedStyle();

        float obj_x = item.offset_x + obj_layout.x;
        float obj_width = obj_layout.width;
        bool clip = hasOverflowClip(obj_style);

        // 更新全局最大宽度
        global_max_width = std::max(global_max_width, obj_x + obj_width);

        // 如果没有 overflow clip，继续遍历子元素
        if (!clip) {
            const auto& obj_children = obj->GetChildren();
            for (const auto& grandchild : obj_children) {
                const auto& grandchild_style = grandchild->GetComputedStyle();
                bool grandchild_out_of_flow = isOutOfFlow(grandchild_style);
                if (!grandchild_out_of_flow) {
                    // 子元素的偏移 = 当前元素的绝对X位置
                    stack.push_back({grandchild.get(), obj_x});
                }
            }
        }
    }

    // 添加容器自身的 padding-right
    float padding_right = computed_style_.padding.right.ToPx(layout_info_.width, computed_style_.font_size);
    global_max_width += padding_right;

    return global_max_width;
}


// 全局计时统计（非 static，供其他文件使用）
std::atomic<long long> g_paint_bg_time{0};
std::atomic<long long> g_paint_border_time{0};
std::atomic<long long> g_paint_children_time{0};
std::atomic<long long> g_paint_shadow_time{0};
std::atomic<long long> g_paint_scrollbar_time{0};

void RenderObject::PrintPaintTimingStats() {
}

void RenderObject::ResetPaintTimingStats() {
    g_paint_bg_time = 0;
    g_paint_border_time = 0;
    g_paint_children_time = 0;
    g_paint_shadow_time = 0;
    g_paint_scrollbar_time = 0;
}

// ========== LayerInfo 实现 ==========

LayerInfo::LayerInfo()
    : compositor_layer()
    , promotion_reason(LayerPromotionReason::None)
    , force_own_layer(false) {
}

// ========== ViewportBounds 实现 ==========

bool ViewportBounds::Contains(float viewport_x, float viewport_y) const {
    if (!valid) {
        return false;
    }

    // 如果有变换，使用变换后的包围盒
    if (has_transform) {
        return transformed_bounds.contains(viewport_x, viewport_y);
    }

    // 普通边界检查
    return viewport_x >= x && viewport_x < x + width &&
           viewport_y >= y && viewport_y < y + height;
}

SkPoint ViewportBounds::ToLocalCoordinates(float viewport_x, float viewport_y) const {
    if (!valid) {
        return SkPoint::Make(std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::quiet_NaN());
    }

    // 先转换为元素坐标系（相对于元素左上角）
    float local_x = viewport_x - x;
    float local_y = viewport_y - y;

    // 如果有变换且可逆，应用逆变换
    if (has_transform && transform_invertible) {
        SkMatrix inverse;
        if (transform.invert(&inverse)) {
            SkPoint pt = SkPoint::Make(local_x, local_y);
            inverse.mapPoints(&pt, 1);
            return pt;
        }
    }

    return SkPoint::Make(local_x, local_y);
}

// ========== 命中测试优化：视口坐标缓存 ==========

void RenderObject::UpdateViewportBounds() {
    const auto& layout = layout_info_;
    const auto& style = computed_style_;

    // 未布局的元素，缓存无效
    if (!layout.is_laid_out) {
        viewport_bounds_.valid = false;
        return;
    }

    // =========================================================================
    // 规则 1：position: fixed 元素
    // =========================================================================
    // Fixed 元素相对于视口定位，layout.x/y 已经是视口坐标
    // 不受任何祖先的滚动影响
    if (style.position == "fixed") {
        viewport_bounds_.x = layout.x;
        viewport_bounds_.y = layout.y;
        viewport_bounds_.width = layout.width;
        viewport_bounds_.height = layout.height;
        viewport_bounds_.valid = true;
        ApplyTransformToViewportBounds();
        return;
    }

    // =========================================================================
    // 规则 2：position: absolute 元素
    // =========================================================================
    // Absolute 元素的 layout.x/y 是相对于直接父元素的坐标
    // 需要累加所有祖先的偏移直到视口
    if (style.position == "absolute") {
        float abs_x = layout.x;
        float abs_y = layout.y;

        // 累加所有祖先的偏移（与普通元素相同的逻辑）
        auto parent = parent_.lock();
        while (parent) {
            if (!parent->GetViewportBounds().valid) {
                parent->UpdateViewportBounds();
            }

            const auto& parent_style = parent->GetComputedStyle();
            const auto& parent_bounds = parent->GetViewportBounds();

            // 遇到 fixed 祖先，使用其缓存的视口坐标
            if (parent_style.position == "fixed") {
                if (parent_bounds.valid) {
                    abs_x += parent_bounds.x;
                    abs_y += parent_bounds.y;

                    // 祖先 transform 对后代位置的影响（如 toast 容器 translateX）
                    if (parent_bounds.has_transform) {
                        abs_x += (parent_bounds.transformed_bounds.x() - parent_bounds.x);
                        abs_y += (parent_bounds.transformed_bounds.y() - parent_bounds.y);
                    }
                }
                break;
            }

            const auto& parent_layout = parent->GetLayoutInfo();
            abs_x += parent_layout.x;
            abs_y += parent_layout.y;

            // 减去父元素的滚动偏移
            abs_x -= parent->GetScrollX();
            abs_y -= parent->GetScrollY();

            // 祖先 transform 对后代位置的影响（平移/旋转/缩放产生的包围盒偏移）
            if (parent_bounds.valid && parent_bounds.has_transform) {
                abs_x += (parent_bounds.transformed_bounds.x() - parent_bounds.x);
                abs_y += (parent_bounds.transformed_bounds.y() - parent_bounds.y);
            }

            parent = parent->GetParent();
        }

        viewport_bounds_.x = abs_x;
        viewport_bounds_.y = abs_y;
        viewport_bounds_.width = layout.width;
        viewport_bounds_.height = layout.height;
        viewport_bounds_.valid = true;
        ApplyTransformToViewportBounds();
        return;
    }

    // =========================================================================
    // 规则 3：position: static/relative 元素（普通流）
    // =========================================================================
    // 普通元素需要累加所有祖先的偏移，并减去滚动偏移
    // relative 元素的 layout.x/y 已经包含了 top/left 偏移
    float abs_x = layout.x;
    float abs_y = layout.y;

    auto parent = parent_.lock();
    while (parent) {
        if (!parent->GetViewportBounds().valid) {
            parent->UpdateViewportBounds();
        }

        const auto& parent_style = parent->GetComputedStyle();
        const auto& parent_bounds = parent->GetViewportBounds();

        // 遇到 fixed 祖先，使用其缓存的视口坐标
        if (parent_style.position == "fixed") {
            if (parent_bounds.valid) {
                abs_x += parent_bounds.x;
                abs_y += parent_bounds.y;

                // 祖先 transform 对后代位置的影响（如 toast 容器 translateX）
                if (parent_bounds.has_transform) {
                    abs_x += (parent_bounds.transformed_bounds.x() - parent_bounds.x);
                    abs_y += (parent_bounds.transformed_bounds.y() - parent_bounds.y);
                }
            }
            break;
        }

        const auto& parent_layout = parent->GetLayoutInfo();
        abs_x += parent_layout.x;
        abs_y += parent_layout.y;

        // 减去父元素的滚动偏移
        abs_x -= parent->GetScrollX();
        abs_y -= parent->GetScrollY();

        // 祖先 transform 对后代位置的影响（平移/旋转/缩放产生的包围盒偏移）
        if (parent_bounds.valid && parent_bounds.has_transform) {
            abs_x += (parent_bounds.transformed_bounds.x() - parent_bounds.x);
            abs_y += (parent_bounds.transformed_bounds.y() - parent_bounds.y);
        }

        parent = parent->GetParent();
    }

    viewport_bounds_.x = abs_x;
    viewport_bounds_.y = abs_y;
    viewport_bounds_.width = layout.width;
    viewport_bounds_.height = layout.height;
    viewport_bounds_.valid = true;

    ApplyTransformToViewportBounds();
}

void RenderObject::ApplyTransformToViewportBounds() {
    const auto& style = computed_style_;

    if (!style.transform.has_value() || style.transform->IsEmpty()) {
        viewport_bounds_.has_transform = false;
        return;
    }

    // 计算变换矩阵
    SkRect local_rect = SkRect::MakeWH(viewport_bounds_.width, viewport_bounds_.height);
    SkMatrix transform = style.transform->ToSkMatrix(local_rect, style.transform_origin);

    // 检查是否可逆
    SkMatrix inverse;
    viewport_bounds_.transform_invertible = transform.invert(&inverse);
    viewport_bounds_.transform = transform;

    // 变换四个角点，计算包围盒
    SkPoint corners[4] = {
        {0, 0},
        {viewport_bounds_.width, 0},
        {viewport_bounds_.width, viewport_bounds_.height},
        {0, viewport_bounds_.height}
    };
    transform.mapPoints(corners, 4);

    float min_x = corners[0].x(), max_x = corners[0].x();
    float min_y = corners[0].y(), max_y = corners[0].y();
    for (int i = 1; i < 4; ++i) {
        min_x = std::min(min_x, corners[i].x());
        max_x = std::max(max_x, corners[i].x());
        min_y = std::min(min_y, corners[i].y());
        max_y = std::max(max_y, corners[i].y());
    }

    viewport_bounds_.transformed_bounds = SkRect::MakeLTRB(
        viewport_bounds_.x + min_x,
        viewport_bounds_.y + min_y,
        viewport_bounds_.x + max_x,
        viewport_bounds_.y + max_y
    );
    viewport_bounds_.has_transform = true;
}

void RenderObject::InvalidateDescendantViewportBounds() {
    for (auto& child : children_) {
        child->InvalidateViewportBounds();
        child->InvalidateDescendantViewportBounds();
    }
}

std::shared_ptr<RenderObject> RenderObject::FindContainingBlock() const {
    auto parent = parent_.lock();
    while (parent) {
        const auto& parent_style = parent->GetComputedStyle();
        // 定位祖先：position 是 relative/absolute/fixed/sticky，或者有 transform/filter/perspective
        // 注意：空字符串和 "static" 都不是定位祖先
        bool is_positioned = !parent_style.position.empty() &&
                             parent_style.position != "static";
        bool has_transform = parent_style.transform.has_value() &&
                             !parent_style.transform->IsEmpty();
        bool has_filter = parent_style.filter.has_value();

        if (is_positioned || has_transform || has_filter) {
            return parent;
        }
        parent = parent->GetParent();
    }
    return nullptr;  // 没有定位祖先，使用初始包含块
}

bool RenderObject::IsScrollContainer() const {
    const auto& style = computed_style_;
    return style.overflow == "auto" ||
           style.overflow == "scroll" ||
           style.overflow_x == "auto" ||
           style.overflow_x == "scroll" ||
           style.overflow_y == "auto" ||
           style.overflow_y == "scroll";
}

std::shared_ptr<CompositorLayer> RenderObject::GetCompositorLayer() const {
    return layer_info_.compositor_layer.lock();
}

void RenderObject::SetCompositorLayer(std::shared_ptr<CompositorLayer> layer) {
    layer_info_.compositor_layer = layer;

    // 🐛 修复：对于 fixed 元素，计算并设置 shadow_extent
    if (layer && layer->GetPromotionReason() == LayerPromotionReason::PositionFixed) {
        // 优化：分别计算四个方向的阴影扩展范围
        CompositorLayer::ShadowExtent extent;

        if (!computed_style_.box_shadow.empty()) {
            for (const auto& shadow : computed_style_.box_shadow) {
                if (!shadow.inset) {  // 只考虑外阴影
                    // 阴影向左扩展 = blur + spread - offset_x
                    // 如果 offset_x > 0（向右偏移），左边扩展减少
                    float left = shadow.blur_radius + shadow.spread_radius - shadow.offset_x;

                    // 阴影向右扩展 = blur + spread + offset_x
                    // 如果 offset_x > 0（向右偏移），右边扩展增加
                    float right = shadow.blur_radius + shadow.spread_radius + shadow.offset_x;

                    // 阴影向上扩展 = blur + spread - offset_y
                    // 如果 offset_y > 0（向下偏移），上边扩展减少
                    float top = shadow.blur_radius + shadow.spread_radius - shadow.offset_y;

                    // 阴影向下扩展 = blur + spread + offset_y
                    // 如果 offset_y > 0（向下偏移），下边扩展增加
                    float bottom = shadow.blur_radius + shadow.spread_radius + shadow.offset_y;

                    // 取所有阴影的最大扩展（支持多个阴影）
                    extent.left = std::max(extent.left, std::max(0.0f, left));
                    extent.right = std::max(extent.right, std::max(0.0f, right));
                    extent.top = std::max(extent.top, std::max(0.0f, top));
                    extent.bottom = std::max(extent.bottom, std::max(0.0f, bottom));
                }
            }
        }

        // 设置 shadow_extent
        if (extent.HasExtent()) {
            layer->SetShadowExtent(extent);
        }
    }
}

bool RenderObject::HasOwnCompositorLayer() const {
    return !layer_info_.compositor_layer.expired();
}

// =========================================================================
// PaintLayer 支持
// =========================================================================

PaintLayer* RenderObject::EnsurePaintLayer() {
    if (!paint_layer_) {
        if (NeedsPaintLayer()) {
            paint_layer_ = std::make_unique<PaintLayer>(this);
        }
    }
    return paint_layer_.get();
}

bool RenderObject::NeedsPaintLayer() const {
    const auto& style = computed_style_;

    // 根元素总是需要 PaintLayer
    if (!parent_.lock()) {
        return true;
    }

    // position: absolute/relative/fixed/sticky 且 z-index != 0
    bool has_position = (style.position == "absolute" ||
                         style.position == "relative" ||
                         style.position == "fixed" ||
                         style.position == "sticky");
    if (has_position && style.z_index != 0) {
        return true;
    }

    // position: fixed 总是需要 PaintLayer
    if (style.position == "fixed") {
        return true;
    }

    // opacity < 1
    if (style.opacity < 1.0f) {
        return true;
    }

    // transform != none
    if (style.transform.has_value()) {
        return true;
    }

    // filter != none
    if (style.filter.has_value()) {
        return true;
    }

    // will-change: transform/opacity
    if (!style.will_change.empty()) {
        if (style.will_change.find("transform") != std::string::npos ||
            style.will_change.find("opacity") != std::string::npos) {
            return true;
        }
    }

    // 可滚动容器
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    if (overflow_y == "scroll" || overflow_y == "auto") {
        return true;
    }

    return false;
}

// =========================================================================
// 属性树状态方法实现
// =========================================================================

bool RenderObject::NeedsTransformNode() const {
    const auto& style = computed_style_;

    // 有 transform 属性
    if (style.transform.has_value()) {
        return true;
    }

    // 有定位偏移
    if (style.position == "relative" || style.position == "absolute" ||
        style.position == "fixed") {
        if (layout_info_.x != 0 || layout_info_.y != 0) {
            return true;
        }
    }

    // 有 will-change: transform
    if (style.will_change.find("transform") != std::string::npos) {
        return true;
    }

    // 有活动的 transform 动画
    for (const auto& anim : style.animations) {
        if (anim.name.find("transform") != std::string::npos ||
            anim.name.find("move") != std::string::npos ||
            anim.name.find("slide") != std::string::npos ||
            anim.name.find("rotate") != std::string::npos ||
            anim.name.find("scale") != std::string::npos) {
            return true;
        }
    }

    for (const auto& trans : style.transitions) {
        if (trans.property == "transform" || trans.property == "all") {
            return true;
        }
    }

    return false;
}

bool RenderObject::NeedsClipNode() const {
    const auto& style = computed_style_;

    // overflow: hidden/scroll/auto
    if (style.overflow == "hidden" || style.overflow == "scroll" ||
        style.overflow == "auto") {
        return true;
    }
    if (style.overflow_x == "hidden" || style.overflow_x == "scroll" ||
        style.overflow_x == "auto") {
        return true;
    }
    if (style.overflow_y == "hidden" || style.overflow_y == "scroll" ||
        style.overflow_y == "auto") {
        return true;
    }

    // 有 clip-path
    if (style.clip_path.has_value()) {
        return true;
    }

    return false;
}

bool RenderObject::NeedsEffectNode() const {
    const auto& style = computed_style_;

    // opacity < 1
    if (style.opacity < 1.0f) {
        return true;
    }

    // 有 filter
    if (style.filter.has_value()) {
        return true;
    }

    // 有 backdrop-filter
    if (style.backdrop_filter.has_value()) {
        return true;
    }

    // 有 will-change: opacity
    if (style.will_change.find("opacity") != std::string::npos) {
        return true;
    }

    // 有活动的 opacity 动画
    for (const auto& anim : style.animations) {
        if (anim.name.find("opacity") != std::string::npos ||
            anim.name.find("fade") != std::string::npos) {
            return true;
        }
    }

    for (const auto& trans : style.transitions) {
        if (trans.property == "opacity" || trans.property == "all") {
            return true;
        }
    }

    return false;
}

bool RenderObject::NeedsScrollNode() const {
    const auto& style = computed_style_;

    // overflow: scroll/auto
    if (style.overflow == "scroll" || style.overflow == "auto") {
        return true;
    }
    if (style.overflow_x == "scroll" || style.overflow_x == "auto") {
        return true;
    }
    if (style.overflow_y == "scroll" || style.overflow_y == "auto") {
        return true;
    }

    return false;
}

bool RenderObject::CanDirectlyUpdateTransform() const {
    // 如果有独立层，可以直接更新 transform
    if (HasOwnCompositorLayer()) {
        return true;
    }

    // 如果有 will-change: transform，可以直接更新
    if (computed_style_.will_change.find("transform") != std::string::npos) {
        return true;
    }

    return false;
}

bool RenderObject::CanDirectlyUpdateOpacity() const {
    // 如果有独立层，可以直接更新 opacity
    if (HasOwnCompositorLayer()) {
        return true;
    }

    // 如果有 will-change: opacity，可以直接更新
    if (computed_style_.will_change.find("opacity") != std::string::npos) {
        return true;
    }

    return false;
}

// ============================================================================
// 布局边界支持（增量布局优化）
// ============================================================================

bool RenderObject::IsLayoutBoundary() const {
    if (!boundary_cache_valid_) {
        const_cast<RenderObject*>(this)->UpdateLayoutBoundaryCache();
    }
    return cached_boundary_type_ != 0;  // 0 = None
}

int RenderObject::GetLayoutBoundaryType() const {
    if (!boundary_cache_valid_) {
        const_cast<RenderObject*>(this)->UpdateLayoutBoundaryCache();
    }
    return cached_boundary_type_;
}

void RenderObject::UpdateLayoutBoundaryCache() {
    const auto& style = computed_style_;

    // 1. 脱离文档流 - 最强的布局边界
    if (style.position == "fixed" || style.position == "absolute") {
        cached_boundary_type_ = 1;  // OutOfFlow
        boundary_cache_valid_ = true;
        return;
    }

    // 2. CSS Containment
    if (style.HasLayoutContainment()) {
        cached_boundary_type_ = 4;  // CSSContainment
        boundary_cache_valid_ = true;
        return;
    }

    // 检查是否有固定尺寸
    bool width_fixed = (style.width.unit == CSSUnit::PX ||
                        style.width.unit == CSSUnit::VW ||
                        style.width.unit == CSSUnit::VH ||
                        style.width.unit == CSSUnit::VMIN ||
                        style.width.unit == CSSUnit::VMAX);

    bool height_fixed = (style.height.unit == CSSUnit::PX ||
                         style.height.unit == CSSUnit::VW ||
                         style.height.unit == CSSUnit::VH ||
                         style.height.unit == CSSUnit::VMIN ||
                         style.height.unit == CSSUnit::VMAX);

    bool has_fixed_size = width_fixed && height_fixed;

    // 3. 滚动容器 + 固定尺寸
    bool is_scroll_container = (style.overflow_x == "scroll" || style.overflow_x == "auto" ||
                                style.overflow_y == "scroll" || style.overflow_y == "auto" ||
                                style.overflow == "scroll" || style.overflow == "auto");

    if (is_scroll_container && has_fixed_size) {
        cached_boundary_type_ = 2;  // ScrollContainer
        boundary_cache_valid_ = true;
        return;
    }

    // 4. 固定尺寸容器
    if (has_fixed_size) {
        cached_boundary_type_ = 3;  // FixedSize
        boundary_cache_valid_ = true;
        return;
    }

    // 5. Flex 固定项
    if (style.flex_grow == 0.0f && style.flex_shrink == 0.0f &&
        style.flex_basis.unit != CSSUnit::AUTO) {
        auto parent = parent_.lock();
        if (parent && parent->GetComputedStyle().display == RenderObjectType::FLEX) {
            cached_boundary_type_ = 5;  // FlexFixed
            boundary_cache_valid_ = true;
            return;
        }
    }

    cached_boundary_type_ = 0;  // None
    boundary_cache_valid_ = true;
}

} // namespace mbink

