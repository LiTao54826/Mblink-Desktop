/**
 * @file render_inline.cpp
 * @brief RenderInline 类实现
 *
 * 从 render_object.cpp 提取的内联元素渲染对象实现。
 * 包含 RenderInline::Layout, PositionChildrenOnly, MeasureIntrinsicSize, Paint 方法。
 * 表单元素绘制委托给 FormElementPainter。
 */

#include "render_object.h"
#include "core/render/painters/box_renderer.h"
#include "painters/form_element_painter.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/text.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mbink {


// ========== RenderInline 实现 ==========

void RenderInline::Layout(float parent_width, float parent_height) {
    static bool debug_iflex = std::getenv("DEBUG_INLINE_FLEX") != nullptr;
    auto node = GetNode();
    const auto& style = computed_style_;

    if (debug_iflex) {
        std::string tag = "?";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            if (elem) tag = elem->GetTagName();
        }
    }

    // 检查是否有显式的width/height设置
    float explicit_width = 0;
    float explicit_height = 0;
    bool has_explicit_width = false;
    bool has_explicit_height = false;

    if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO) {
        explicit_width = style.width.ToPx(parent_width, style.font_size);
        has_explicit_width = true;
    }

    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
        explicit_height = style.height.ToPx(parent_height, style.font_size);
        has_explicit_height = true;
    }

    // 内联元素布局：计算所有子元素的总宽度和最大高度
    float total_width = 0;
    float max_height = 0;

    // 父节点进入本轮布局时，子节点尺寸需要参与最新测量，
    // 不能只依赖子节点自身的 NeedsLayout 标志。
    const bool parent_layout_pass = needs_layout_ || child_needs_layout_;

    for (auto& child : children_) {
        if (debug_iflex) {
            std::string ctag = "?";
            auto cnode = child->GetNode();
            if (cnode && cnode->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto celem = std::dynamic_pointer_cast<Element>(cnode);
                if (celem) ctag = celem->GetTagName();
            } else if (cnode && cnode->GetNodeType() == NodeType::TEXT_NODE) {
                ctag = "#text";
            }
        }

        if (child->NeedsLayout() || parent_layout_pass) {
            child->Layout(parent_width, parent_height);
        }
        auto& child_layout = child->GetLayoutInfo();
        if (debug_iflex) {
        }
        total_width += child_layout.width;
        max_height = std::max(max_height, child_layout.height);
    }

    // 计算 padding
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_height, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_height, style.font_size);

    // 设置内联元素的尺寸
    layout_info_.width = has_explicit_width ? explicit_width : (total_width + padding_left + padding_right);
    layout_info_.height = has_explicit_height ? explicit_height : (max_height > 0 ? max_height + padding_top + padding_bottom : 20.0f);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;

    if (debug_iflex) {
    }

    // 计算内容区域宽度
    float content_width = layout_info_.width - padding_left - padding_right;

    // 设置子元素的位置（水平排列，支持 text-align）
    float start_x = padding_left;

    if (style.text_align == "center" && total_width < content_width) {
        start_x = padding_left + (content_width - total_width) / 2.0f;
    } else if (style.text_align == "right" && total_width < content_width) {
        start_x = padding_left + content_width - total_width;
    }

    float current_x = start_x;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        child_layout.x = current_x;
        child_layout.y = padding_top + (layout_info_.height - padding_top - padding_bottom - child_layout.height) / 2.0f;
        current_x += child_layout.width;
    }
}

void RenderInline::PositionChildrenOnly() {
    const auto& style = computed_style_;

    float padding_left = style.padding.left.ToPx(layout_info_.width, style.font_size);
    float padding_top = style.padding.top.ToPx(layout_info_.width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(layout_info_.width, style.font_size);

    float max_height = 0;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        if (child_layout.width == 0 && child_layout.height == 0) {
            child->Layout(layout_info_.width, layout_info_.height);
        }
        max_height = std::max(max_height, child_layout.height);
    }

    float content_height = layout_info_.height - padding_top - padding_bottom;

    // DEBUG: 输出 inline 元素子元素定位信息
    static bool debug_inline = std::getenv("DEBUG_INLINE_POS") != nullptr;

    float current_x = padding_left;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        float child_y = padding_top + (content_height - child_layout.height) / 2.0f;

        if (debug_inline) {
        }

        float new_x = current_x;
        child_layout.x = new_x;
        child_layout.y = child_y;
        current_x += child_layout.width;
    }
}

std::pair<float, float> RenderInline::MeasureIntrinsicSize(float available_width) {
    const auto& style = computed_style_;

    // 调试日志
    static bool debug_inline = std::getenv("DEBUG_INLINE") != nullptr;
    if (debug_inline) {
        auto node = GetNode();
        std::string tag = "?";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            tag = elem->GetTagName();
        }
    }

    float padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float padding_right = style.padding.right.ToPx(available_width, style.font_size);
    float padding_top = style.padding.top.ToPx(available_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(available_width, style.font_size);

    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;

    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(available_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }

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

    if (debug_inline) {
    }

    return {width, height};
}

void RenderInline::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // 跳过零高度元素
    if (layout_info_.height <= 0) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y,
                                          layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(10, 10))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

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

    BoxRenderer renderer(canvas);

    // 渲染背景
    renderer.RenderBackgroundAdvanced(box, styles, &style.border_radius);

    // 渲染边框
    if (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) {
        std::string border_width = std::to_string(style.border.width.value) + "px";
        std::string border_style = "solid";
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        renderer.RenderBorder(box, border_width, border_style, color_str);
    }

    // 绘制 outline
    PaintOutline(canvas);

    // 渲染表单控件 - 使用 FormElementPainter
    auto node = GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);

        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            Box form_box;
            form_box.content_x = 0;
            form_box.content_y = 0;
            form_box.content_width = layout.width;
            form_box.content_height = layout.height;

            FormElementPaintParams params;
            params.font_family = style.font_family;
            params.font_size = style.font_size;
            params.text_color = style.color;
            params.has_focus = element->HasPseudoClass("focus");

            FormElementPainter painter(canvas);
            painter.PaintInputElement(input_element.get(), form_box, params);
        }

        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            Box form_box;
            form_box.content_x = 0;
            form_box.content_y = 0;
            form_box.content_width = layout.width;
            form_box.content_height = layout.height;

            FormElementPaintParams params;
            params.font_family = style.font_family;
            params.font_size = style.font_size;
            params.text_color = style.color;
            params.has_focus = element->HasPseudoClass("focus");

            FormElementPainter painter(canvas);
            painter.PaintTextAreaElement(textarea_element.get(), form_box, params);
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
    std::stable_sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            return a->GetComputedStyle().z_index < b->GetComputedStyle().z_index;
        });

    // 绘制所有子元素
    for (auto& child : sorted_children) {
        // 跳过有独立合成层的子元素
        if (child->HasOwnCompositorLayer()) {
            continue;
        }
        child->Paint(canvas);
    }

    if (needs_clip) {
        canvas->restore();
    }

    canvas->restore();
    needs_paint_ = false;
}

} // namespace mbink
