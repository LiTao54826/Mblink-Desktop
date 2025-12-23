/**
 * @file render_inline_block.cpp
 * @brief Inline-Block渲染对象实现
 */

#include "render_inline_block.h"
#include "box_renderer.h"
#include "text_renderer.h"
#include "gradient_renderer.h"
#include "color.h"
#include "text/font_manager.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_select_element.h"
#include "core/dom/html_form_controls.h"
#include "core/dom/html_canvas_element.h"
#include "core/dom/html_image_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include "image/image_fit.h"
#include "image/image_loader.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

namespace lightui {

void RenderInlineBlock::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // Check if dimensions are already set by external layout engine (e.g., flex/grid).
    // When is_laid_out is true and we have valid dimensions, we only need to
    // position children, not recalculate our own dimensions.
    // This is important because flex layout may stretch elements (align-items: stretch),
    // and we need to preserve the externally calculated dimensions.
    bool dimensions_externally_set = layout_info_.is_laid_out &&
                                     layout_info_.width > 0 &&
                                     layout_info_.height > 0;

    // 计算 padding (use layout_info_.width if externally set, otherwise parent_width)
    float reference_width = dimensions_externally_set ? layout_info_.width : parent_width;
    float reference_height = dimensions_externally_set ? layout_info_.height : parent_height;

    float padding_left = style.padding.left.ToPx(reference_width, style.font_size);
    float padding_right = style.padding.right.ToPx(reference_width, style.font_size);
    float padding_top = style.padding.top.ToPx(reference_height, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(reference_height, style.font_size);

    // 计算 border（提前计算，用于 box-sizing 调整）
    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    if (border_left == 0 && border_right == 0) {
        float border_width = style.border.width.ToPx(reference_width, style.font_size);
        border_left = border_right = border_width;
    }

    // 1. 计算宽度 (only if not externally set)
    if (!dimensions_externally_set) {
        if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO) {
            // 显式设置了宽度
            float specified_width = style.width.ToPx(parent_width, style.font_size);

            // 根据 box-sizing 调整宽度
            // content-box: width 只是内容宽度，需要加上 padding 和 border
            // border-box: width 包含 padding 和 border
            if (style.box_sizing == "content-box") {
                layout_info_.width = specified_width + padding_left + padding_right + border_left + border_right;
            } else {
                // border-box
                layout_info_.width = specified_width;
            }
        } else {
            // 使用shrink-to-fit算法（包含 padding）
            layout_info_.width = CalculateShrinkToFitWidth(parent_width);
        }
    }

    // 2. 布局子元素（考虑 padding 和 border）
    float content_height = 0;
    float total_child_width = 0;
    float content_width = layout_info_.width - padding_left - padding_right - border_left - border_right;

    // 首先布局所有子元素并计算总宽度和高度
    // Note: Always layout children to ensure they have valid dimensions.
    // When user specifies fixed width/height on inline-block (e.g., button),
    // MeasureIntrinsicSize won't layout children, leaving their dimensions as 0.
    // We need to layout them here to get correct dimensions for text-align calculation.
    for (auto& child : children_) {
        child->Layout(content_width, reference_height);

        auto& child_layout = child->GetLayoutInfo();
        total_child_width += child_layout.width;
        content_height = std::max(content_height, child_layout.height);
    }

    // 3. 计算高度（包含 padding 和 border）(only if not externally set)
    // 计算 border
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;
    if (border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(reference_width, style.font_size);
        border_top = border_bottom = border_width;
    }

    if (!dimensions_externally_set) {
        if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
            // 显式设置了高度
            float specified_height = style.height.ToPx(parent_height, style.font_size);

            // 根据 box-sizing 调整高度
            if (style.box_sizing == "content-box") {
                layout_info_.height = specified_height + padding_top + padding_bottom + border_top + border_bottom;
            } else {
                // border-box
                layout_info_.height = specified_height;
            }
        } else {
            if (content_height > 0) {
                // 有子元素内容
                layout_info_.height = content_height + padding_top + padding_bottom + border_top + border_bottom;
            } else {
                // 没有子元素（如 input, select 元素），基于 font-size 计算
                // 检查是否是 text/password 类型的 input 元素
                bool is_text_input = false;
                auto node = GetNode();
                if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto element = std::static_pointer_cast<Element>(node);
                    if (element->GetTagName() == "input") {
                        std::string type = element->GetAttribute("type");
                        is_text_input = (type.empty() || type == "text" || type == "password" || type == "number");
                    }
                }

                // Chrome text input 元素: content_height ≈ font-size * 0.85 (约 13.5px for 16px font)
                // 其他元素: content_height ≈ font-size * 1.2 (line-height: normal)
                float content_line_height = is_text_input ? (style.font_size * 0.85f) : (style.font_size * 1.2f);
                layout_info_.height = content_line_height + padding_top + padding_bottom + border_top + border_bottom;
            }
        }
    }

    // 4. 设置子元素位置，支持 text-align
    // Calculate content area for positioning children
    float content_area_height = layout_info_.height - padding_top - padding_bottom - border_top - border_bottom;
    float start_x = padding_left + border_left;

    // 处理 text-align
    if (style.text_align == "center" && total_child_width < content_width) {
        // 居中对齐
        start_x = padding_left + border_left + (content_width - total_child_width) / 2.0f;
    } else if (style.text_align == "right" && total_child_width < content_width) {
        // 右对齐
        start_x = padding_left + border_left + content_width - total_child_width;
    }

    float current_x = start_x;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        child_layout.x = current_x;
        // 垂直居中：如果内容高度小于内容区域高度，则居中
        // 这对于按钮等元素的文字垂直居中很重要
        if (child_layout.height < content_area_height) {
            child_layout.y = padding_top + border_top + (content_area_height - child_layout.height) / 2.0f;
        } else {
            child_layout.y = padding_top + border_top;
        }
        current_x += child_layout.width;
    }

    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

float RenderInlineBlock::CalculateShrinkToFitWidth(float available_width) {
    // CSS shrink-to-fit 算法：
    // 1. 如果 available_width >= preferred_width，使用 preferred_width
    // 2. 如果 available_width < preferred_width，使用 max(preferred_min, available_width)
    float preferred = CalculatePreferredWidth();

    if (available_width >= preferred) {
        return preferred;
    }

    float preferred_min = CalculatePreferredMinimumWidth();
    return std::max(preferred_min, available_width);
}

float RenderInlineBlock::CalculatePreferredMinimumWidth() {
    // 计算内容不换行的最小宽度
    const auto& style = computed_style_;
    float padding_left = style.padding.left.ToPx();
    float padding_right = style.padding.right.ToPx();
    float border_width = style.border.width.ToPx();

    float content_width = 0;

    // 特殊处理：Select元素需要根据option内容计算宽度
    auto node = GetNode();
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() == "select") {
            // 找到最长的option文本
            std::string longest_text = "";

            // 辅助函数：从option元素获取文本
            auto getOptionText = [](const std::shared_ptr<Element>& option) -> std::string {
                const auto& option_children = option->GetChildNodes();
                for (const auto& text_node : option_children) {
                    if (text_node->GetNodeType() == NodeType::TEXT_NODE) {
                        auto text = std::static_pointer_cast<Text>(text_node);
                        return text->GetData();
                    }
                }
                return "";
            };

            // 递归查找所有option元素（支持optgroup）
            std::function<void(const std::vector<std::shared_ptr<Node>>&)> findLongestOption;
            findLongestOption = [&](const std::vector<std::shared_ptr<Node>>& nodes) {
                for (const auto& child : nodes) {
                    if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto elem = std::static_pointer_cast<Element>(child);
                        std::string tag = elem->GetTagName();

                        if (tag == "option") {
                            std::string option_text = getOptionText(elem);
                            if (option_text.length() > longest_text.length()) {
                                longest_text = option_text;
                            }
                        } else if (tag == "optgroup") {
                            // 递归搜索optgroup内的option
                            findLongestOption(elem->GetChildNodes());
                        }
                    }
                }
            };

            findLongestOption(element->GetChildNodes());

            if (!longest_text.empty()) {
                // 使用字体测量实际宽度（支持CJK字符）
                FontDescriptor desc;
                desc.family = style.font_family;
                desc.size = style.font_size;
                desc.weight = FontWeight::NORMAL;
                desc.style = FontStyle::NORMAL;

                SkFont font = FontManager::GetInstance().LoadFont(desc);
                // 使用支持CJK/Emoji的测量方法
                content_width = TextRenderer::MeasureMixedTextWidth(longest_text, font);

                // 添加下拉箭头区域宽度 (Chrome 约 16px)
                const float dropdown_arrow_width = 16.0f;
                content_width += dropdown_arrow_width;
            }
        }
    }

    // 对于其他元素，使用子元素计算或使用字体测量
    if (content_width == 0) {
        for (auto& child : children_) {
            if (child->GetType() == RenderObjectType::TEXT) {
                auto text_child = std::static_pointer_cast<RenderText>(child);
                std::string text = text_child->GetText();

                // 使用实际字体测量宽度（支持CJK/Emoji字符）
                FontDescriptor desc;
                desc.family = style.font_family;
                desc.size = style.font_size;
                desc.weight = FontWeight::NORMAL;
                desc.style = FontStyle::NORMAL;

                SkFont font = FontManager::GetInstance().LoadFont(desc);
                // 使用支持CJK/Emoji的测量方法
                float text_width = TextRenderer::MeasureMixedTextWidth(text, font);
                content_width = std::max(content_width, text_width);
            }
        }
    }

    // 总宽度 = 内容宽度 + padding + border
    return content_width + padding_left + padding_right + border_width * 2;
}

float RenderInlineBlock::CalculatePreferredWidth() {
    // 计算内容自然布局的宽度
    // 对于简单的inline-block元素，preferred width = preferred minimum width
    return CalculatePreferredMinimumWidth();
}

std::pair<float, float> RenderInlineBlock::MeasureIntrinsicSize(float available_width) {
    const auto& style = computed_style_;

    // 计算 padding
    float padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float padding_right = style.padding.right.ToPx(available_width, style.font_size);
    float padding_top = style.padding.top.ToPx(available_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(available_width, style.font_size);

    // 计算 border
    float border_left = style.border_left_width;
    float border_right = style.border_right_width;
    float border_top = style.border_top_width;
    float border_bottom = style.border_bottom_width;

    // 如果没有单独的边框宽度，使用通用边框
    if (border_left == 0 && border_right == 0 && border_top == 0 && border_bottom == 0) {
        float border_width = style.border.width.ToPx(available_width, style.font_size);
        border_left = border_right = border_top = border_bottom = border_width;
    }

    // 计算宽度
    float width;
    if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO) {
        // 显式设置了宽度
        float specified_width = style.width.ToPx(available_width, style.font_size);

        // 根据 box-sizing 调整宽度
        if (style.box_sizing == "content-box") {
            width = specified_width + padding_left + padding_right + border_left + border_right;
        } else {
            // border-box
            width = specified_width;
        }
    } else {
        // 使用 shrink-to-fit 算法
        width = CalculateShrinkToFitWidth(available_width);
    }

    // 计算高度
    float height;
    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
        // 显式设置了高度
        float specified_height = style.height.ToPx(available_width, style.font_size);

        // 根据 box-sizing 调整高度
        if (style.box_sizing == "content-box") {
            height = specified_height + padding_top + padding_bottom + border_top + border_bottom;
        } else {
            // border-box
            height = specified_height;
        }
    } else {
        // 根据内容计算高度
        float content_height = 0;
        float content_width = width - padding_left - padding_right - border_left - border_right;

        // 布局子元素以获取高度
        for (auto& child : children_) {
            child->Layout(content_width, 0);
            auto& child_layout = child->GetLayoutInfo();
            content_height = std::max(content_height, child_layout.height);
        }

        if (content_height > 0) {
            // 有子元素内容
            height = content_height + padding_top + padding_bottom + border_top + border_bottom;
        } else {
            // 没有子元素（如 input, select 元素），基于 font-size 计算
            // 检查是否是 text/password 类型的 input 元素
            bool is_text_input = false;
            auto node = GetNode();
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                if (element->GetTagName() == "input") {
                    std::string type = element->GetAttribute("type");
                    is_text_input = (type.empty() || type == "text" || type == "password" || type == "number");
                }
            }

            // Chrome text input 元素: content_height ≈ font-size * 0.85 (约 13.5px for 16px font)
            // 其他元素: content_height ≈ font-size * 1.2 (line-height: normal)
            float content_line_height = is_text_input ? (style.font_size * 0.85f) : (style.font_size * 1.2f);
            height = content_line_height + padding_top + padding_bottom + border_top + border_bottom;
        }
    }

    return {width, height};
}

void RenderInlineBlock::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling: Skip inline-block elements outside clip region
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(50, 50))) {
        needs_paint_ = false;
        return;
    }

    auto node = GetNode();
    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 应用 CSS opacity（使用 saveLayerAlpha 实现透明度）
    bool has_opacity = style.opacity < 1.0f;
    if (has_opacity) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        int alpha = static_cast<int>(style.opacity * 255);
        canvas->saveLayerAlpha(&bounds, alpha);
    }

    // 应用 CSS transform
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(bounds, style.transform_origin);
        canvas->concat(transform_matrix);
    }

    // 应用 CSS clip-path
    if (style.clip_path.has_value() && !style.clip_path->IsNone()) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPath clip_path = style.clip_path->ToSkPath(bounds);
        canvas->clipPath(clip_path, true);
    }

    // 创建盒模型
    Box box;
    // 设置 padding 和 border 值
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_left_width = style.border.width.ToPx();
    box.border_right_width = style.border.width.ToPx();
    box.border_top_width = style.border.width.ToPx();
    box.border_bottom_width = style.border.width.ToPx();

    // content_x 和 content_y 从 border + padding 开始
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    // content_width 和 content_height 是去掉 border 和 padding 后的尺寸
    box.content_width = layout.width - box.border_left_width - box.border_right_width - box.padding_left - box.padding_right;
    box.content_height = layout.height - box.border_top_width - box.border_bottom_width - box.padding_top - box.padding_bottom;

    // 渲染器
    BoxRenderer renderer(canvas);

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
    else if (!style.background_color.empty()) {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = style.background_color;
        renderer.RenderBackgroundAdvanced(box, styles, &style.border_radius);
    }


    // 渲染边框 - 支持单边边框
    bool has_any_border = (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) ||
                          (style.border_left_width > 0 && style.border_left_style != CSSBorderStyle::NONE) ||
                          (style.border_right_width > 0 && style.border_right_style != CSSBorderStyle::NONE) ||
                          (style.border_top_width > 0 && style.border_top_style != CSSBorderStyle::NONE) ||
                          (style.border_bottom_width > 0 && style.border_bottom_style != CSSBorderStyle::NONE);

    if (has_any_border) {
        // 检查是否有圆角
        bool has_border_radius = !style.border_radius.top_left.IsZero() ||
                                 !style.border_radius.top_right.IsZero() ||
                                 !style.border_radius.bottom_right.IsZero() ||
                                 !style.border_radius.bottom_left.IsZero();

        if (has_border_radius) {
            // 有圆角：使用 RenderRoundedBorderAdvanced（支持每边独立属性）
            
            // 准备四边宽度数组 [top, right, bottom, left]
            float border_widths[4] = {
                style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx(),
                style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx(),
                style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx(),
                style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx()
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
            // 无圆角：使用普通边框渲染
            std::string border_width = std::to_string(style.border.width.value) + "px";
            std::string border_style = "solid";
            char color_str[8];
            snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                     SkColorGetR(style.border.color),
                     SkColorGetG(style.border.color),
                     SkColorGetB(style.border.color));
            renderer.RenderBorder(box, border_width, border_style, std::string(color_str));
        }
    }

    // ========== 绘制 outline（焦点指示器）==========
    // outline 不占用布局空间，紧贴边框外边缘绘制（符合浏览器行为）
    if (style.outline_style != "none" && !style.outline_width.IsZero()) {
        float outline_width = style.outline_width.ToPx();
        float outline_offset = style.outline_offset.ToPx();

        // 使用 stroke 绘制时，线条以矩形边缘为中心
        // 要让 outline 内边缘紧贴 border-box 外边缘，需要向外偏移 half_width
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
            float tl = style.border_radius.top_left.ToPx() + outline_offset + half_width;
            float tr = style.border_radius.top_right.ToPx() + outline_offset + half_width;
            float br = style.border_radius.bottom_right.ToPx() + outline_offset + half_width;
            float bl = style.border_radius.bottom_left.ToPx() + outline_offset + half_width;

            SkRRect outline_rrect;
            SkVector radii[4] = {{tl, tl}, {tr, tr}, {br, br}, {bl, bl}};
            outline_rrect.setRectRadii(outline_rect, radii);
            canvas->drawRRect(outline_rrect, outline_paint);
        } else {
            canvas->drawRect(outline_rect, outline_paint);
        }
    }

    // 渲染表单控件特定内容
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        std::string tag_name = element->GetTagName();

        // Input元素
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            PaintInputElement(canvas, input_element.get(), box);
        }

        // Textarea元素
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            PaintTextAreaElement(canvas, textarea_element.get(), box);
            // Textarea元素不绘制子元素（文本内容由value管理）
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }

        // Select元素
        if (element->GetTagName() == "select") {
            PaintSelectElement(canvas, element.get(), box);
            // Select元素不绘制子元素（option元素由PaintSelectElement处理）
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }

        // Progress元素
        auto progress_element = std::dynamic_pointer_cast<HTMLProgressElement>(node);
        if (progress_element) {
            PaintProgressElement(canvas, progress_element.get(), box);
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }

        // Meter元素
        auto meter_element = std::dynamic_pointer_cast<HTMLMeterElement>(node);
        if (meter_element) {
            PaintMeterElement(canvas, meter_element.get(), box);
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }
        
        // Canvas元素 - 将Canvas内部surface内容绘制到窗口画布
        auto canvas_element = std::dynamic_pointer_cast<HTMLCanvasElement>(node);
        if (canvas_element) {
            auto ctx2d = canvas_element->GetContext2D();
            if (ctx2d) {
                SkSurface* surface = ctx2d->GetSurface();
                if (surface) {
                    sk_sp<SkImage> image = surface->makeImageSnapshot();
                    if (image) {
                        // 使用canvas元素的实际尺寸而不是布局尺寸
                        // 因为JS可能在渲染树构建后修改了width/height属性
                        float canvas_width = static_cast<float>(canvas_element->GetWidth());
                        float canvas_height = static_cast<float>(canvas_element->GetHeight());
                        
                        SkRect dst = SkRect::MakeXYWH(
                            box.content_x,
                            box.content_y,
                            canvas_width,
                            canvas_height
                        );
                        canvas->drawImageRect(image, dst, SkSamplingOptions());
                    }
                }
            }
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }
        
        // Image元素 - 使用object-fit和object-position渲染图片
        auto image_element = std::dynamic_pointer_cast<HTMLImageElement>(node);
        if (image_element) {
            // 优先使用已加载的图片
            sk_sp<SkImage> image = image_element->GetSkImage();
            
            // 如果图片未加载，尝试从URL加载
            if (!image) {
                std::string src = image_element->GetSrc();
                if (!src.empty()) {
                    // 使用 ImageLoader 加载（支持网络URL）
                    image = ImageLoader::LoadFromUrl(src);
                    if (image) {
                        // 缓存到元素中
                        image_element->SetSkImage(image);
                    }
                }
            }
            
            if (image) {
                // 获取图片原始尺寸
                float image_width = static_cast<float>(image->width());
                float image_height = static_cast<float>(image->height());
                
                // 获取容器区域（content box）
                SkRect container_rect = SkRect::MakeXYWH(
                    box.content_x,
                    box.content_y,
                    box.content_width,
                    box.content_height
                );
                
                // 使用object-fit和object-position计算源和目标矩形
                ObjectFitResult fit_result = CalculateObjectFit(
                    image_width,
                    image_height,
                    container_rect,
                    style.object_fit,
                    style.object_position
                );
                
                // 绘制图片
                if (!fit_result.src_rect.isEmpty() && !fit_result.dst_rect.isEmpty()) {
                    SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
                    canvas->drawImageRect(image, fit_result.src_rect, fit_result.dst_rect, 
                                         sampling, nullptr, SkCanvas::kStrict_SrcRectConstraint);
                }
            }
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }
    }

    // 绘制子元素
    // 注意：对于 inline-block 元素，子元素的绘制不依赖 NeedsPaint 标志
    // 因为子元素不在 Taffy 树中，它们的重绘状态可能没有正确同步
    for (auto& child : children_) {
        child->Paint(canvas);
    }

    // 恢复 opacity layer（如果有）
    if (has_opacity) {
        canvas->restore();
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

void RenderInlineBlock::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    if (!input) return;

    InputType type = input->GetInputType();
    std::string value = input->GetValue();
    
    // 调试：输出 input 的值
    static int debug_count = 0;
    if (++debug_count <= 10) {
        std::cout << "[PaintInputElement] value='" << value << "', placeholder='" << input->GetPlaceholder() << "'" << std::endl;
    }

    // 处理文本类型的输入框
    if (type == InputType::Text || type == InputType::Password ||
        type == InputType::Email || type == InputType::Tel ||
        type == InputType::Url || type == InputType::Search ||
        type == InputType::Number) {

        // 创建字体（光标绘制也需要）
        FontDescriptor desc;
        desc.family = computed_style_.font_family;
        desc.size = computed_style_.font_size;
        desc.weight = FontWeight::NORMAL;
        desc.style = FontStyle::NORMAL;
        SkFont font = FontManager::GetInstance().LoadFont(desc);

        // input[number] 的 spinner 宽度
        const float spinner_width = (type == InputType::Number) ? 16.0f : 0.0f;

        // 计算文本位置
        // 注意：box.content_x/y 已经包含了 border + padding 的偏移
        // box.content_width/height 是 content 区域的尺寸（不包括 padding）
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        float text_height = -font_metrics.fAscent + font_metrics.fDescent;
        float text_y = box.content_y + (box.content_height - text_height) / 2 - font_metrics.fAscent;
        float text_x = box.content_x;

        // 文本可用宽度（减去 spinner 宽度）
        float text_available_width = box.content_width - spinner_width;

        // 如果value为空，显示placeholder
        std::string original_value = value;  // 保存原始值用于光标计算
        bool is_placeholder = false;
        if (value.empty()) {
            value = input->GetPlaceholder();
            is_placeholder = true;
        }

        // 裁剪文本区域（防止文本溢出）
        canvas->save();
        SkRect text_clip_rect = SkRect::MakeXYWH(
            text_x,
            box.content_y,
            text_available_width,
            box.content_height
        );
        canvas->clipRect(text_clip_rect);

        // 绘制文本（如果有内容）
        if (!value.empty()) {
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

            // 如果是密码类型，显示星号
            std::string display_text = value;
            if (type == InputType::Password && !is_placeholder) {
                display_text = std::string(value.length(), '*');
            }

            // 绘制文本（使用支持CJK的方法，解决中文placeholder乱码问题）
            text_renderer.DrawTextWithEmoji(display_text, text_x, text_y, font, text_paint);
        }

        // 如果有焦点，绘制选中高亮和光标
        auto element = std::static_pointer_cast<Element>(GetNode());
        bool has_focus = element && element->HasPseudoClass("focus");

        if (has_focus) {
            int sel_start = input->GetSelectionStart();
            int sel_end = input->GetSelectionEnd();

            // 绘制选中区域高亮
            if (sel_start != sel_end && !original_value.empty()) {
                int start_char = std::min(sel_start, sel_end);
                int end_char = std::max(sel_start, sel_end);

                // 使用 UTF-8 工具计算字节位置
                size_t start_byte = utf8::CharPosToBytePos(original_value, start_char);
                size_t end_byte = utf8::CharPosToBytePos(original_value, end_char);

                std::string text_before_sel = original_value.substr(0, start_byte);
                std::string selected_text = original_value.substr(start_byte, end_byte - start_byte);

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

                // box.content_y 和 box.content_height 已经是 content 区域
                float sel_y_top = box.content_y;
                float sel_height = box.content_height;
                canvas->drawRect(SkRect::MakeXYWH(sel_start_x, sel_y_top, sel_width, sel_height), sel_paint);
            }

            // 基于时间的光标闪烁：每500毫秒切换一次
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            bool cursor_visible = (ms / 500) % 2 == 0;

            if (cursor_visible) {
                // 计算光标位置 - 使用 UTF-8 字符位置
                int cursor_pos = sel_end;
                size_t cursor_byte_pos = utf8::CharPosToBytePos(original_value, cursor_pos);
                std::string text_before_cursor = original_value.substr(0, cursor_byte_pos);

                // 如果是密码类型，使用星号计算宽度
                if (type == InputType::Password) {
                    text_before_cursor = std::string(cursor_pos, '*');
                }

                // 测量光标前的文本宽度
                float cursor_x = text_x;
                if (!text_before_cursor.empty()) {
                    cursor_x += font.measureText(
                        text_before_cursor.c_str(),
                        text_before_cursor.length(),
                        SkTextEncoding::kUTF8
                    );
                }

                // 计算光标的Y坐标（基于字体度量，垂直居中）
                float font_height = font_metrics.fDescent - font_metrics.fAscent;
                float cursor_y_top = box.content_y + (box.content_height - font_height) / 2;
                float cursor_y_bottom = cursor_y_top + font_height;

                // 绘制光标
                SkPaint cursor_paint;
                cursor_paint.setColor(SK_ColorBLACK);
                cursor_paint.setStrokeWidth(1.5f);
                cursor_paint.setAntiAlias(true);

                canvas->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
            }
        }

        // 恢复裁剪
        canvas->restore();

        // 绘制 input[number] 的 spinner 箭头
        // 浏览器标准行为：hover 或 focus 时显示
        if (type == InputType::Number && spinner_width > 0) {
            auto element = std::static_pointer_cast<Element>(GetNode());
            bool is_hovered = element && element->HasPseudoClass("hover");
            bool is_focused = element && element->HasPseudoClass("focus");

            if (is_hovered || is_focused) {
                float spinner_x = box.content_x + box.content_width - spinner_width;
                float spinner_y = box.content_y;
                float spinner_height = box.content_height;
                float half_height = spinner_height / 2;

                // 绘制 spinner 背景分隔线
                SkPaint line_paint;
                line_paint.setColor(SkColorSetRGB(200, 200, 200));
                line_paint.setStrokeWidth(1);
                line_paint.setAntiAlias(true);
                canvas->drawLine(spinner_x, spinner_y, spinner_x, spinner_y + spinner_height, line_paint);
                canvas->drawLine(spinner_x, spinner_y + half_height, spinner_x + spinner_width, spinner_y + half_height, line_paint);

                // 箭头大小
                float arrow_size = 4.0f;
                float arrow_cx = spinner_x + spinner_width / 2;

                // 绘制上箭头 (▲)
                float up_arrow_cy = spinner_y + half_height / 2;
                SkPath up_arrow;
                up_arrow.moveTo(arrow_cx, up_arrow_cy - arrow_size / 2);
                up_arrow.lineTo(arrow_cx - arrow_size, up_arrow_cy + arrow_size / 2);
                up_arrow.lineTo(arrow_cx + arrow_size, up_arrow_cy + arrow_size / 2);
                up_arrow.close();

                SkPaint arrow_paint;
                arrow_paint.setColor(SkColorSetRGB(80, 80, 80));
                arrow_paint.setStyle(SkPaint::kFill_Style);
                arrow_paint.setAntiAlias(true);
                canvas->drawPath(up_arrow, arrow_paint);

                // 绘制下箭头 (▼)
                float down_arrow_cy = spinner_y + half_height + half_height / 2;
                SkPath down_arrow;
                down_arrow.moveTo(arrow_cx, down_arrow_cy + arrow_size / 2);
                down_arrow.lineTo(arrow_cx - arrow_size, down_arrow_cy - arrow_size / 2);
                down_arrow.lineTo(arrow_cx + arrow_size, down_arrow_cy - arrow_size / 2);
                down_arrow.close();
                canvas->drawPath(down_arrow, arrow_paint);
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
    // 处理 range 类型（滑动条）
    else if (type == InputType::Range) {
        // 获取 min, max, value
        double min_val = input->GetMin();
        double max_val = input->GetMax();
        double cur_val = input->GetValueAsNumber();

        // 计算滑块位置
        double range = max_val - min_val;
        double position = (range > 0) ? ((cur_val - min_val) / range) : 0.5;
        position = std::clamp(position, 0.0, 1.0);

        // 轨道参数 - Chrome 默认轨道高度约 8px
        float track_height = 8.0f;
        float track_y = box.content_y + (box.content_height - track_height) / 2.0f;
        float track_radius = track_height / 2.0f;

        // 绘制轨道背景
        SkPaint track_paint;
        track_paint.setColor(SkColorSetRGB(200, 200, 200));
        track_paint.setStyle(SkPaint::kFill_Style);
        track_paint.setAntiAlias(true);

        SkRRect track_rrect;
        track_rrect.setRectXY(
            SkRect::MakeXYWH(box.content_x, track_y, box.content_width, track_height),
            track_radius, track_radius
        );
        canvas->drawRRect(track_rrect, track_paint);

        // 绘制已填充部分（蓝色）
        float filled_width = box.content_width * static_cast<float>(position);
        if (filled_width > 0) {
            SkPaint filled_paint;
            filled_paint.setColor(SkColorSetRGB(0, 120, 215));  // Windows 蓝色
            filled_paint.setStyle(SkPaint::kFill_Style);
            filled_paint.setAntiAlias(true);

            SkRRect filled_rrect;
            filled_rrect.setRectXY(
                SkRect::MakeXYWH(box.content_x, track_y, filled_width, track_height),
                track_radius, track_radius
            );
            canvas->drawRRect(filled_rrect, filled_paint);
        }

        // 绘制滑块（圆形）
        float thumb_radius = 7.0f;
        float thumb_x = box.content_x + box.content_width * static_cast<float>(position);
        float thumb_y = box.content_y + box.content_height / 2.0f;

        // 滑块阴影
        SkPaint shadow_paint;
        shadow_paint.setColor(SkColorSetARGB(40, 0, 0, 0));
        shadow_paint.setStyle(SkPaint::kFill_Style);
        shadow_paint.setAntiAlias(true);
        canvas->drawCircle(thumb_x, thumb_y + 1, thumb_radius, shadow_paint);

        // 滑块背景
        SkPaint thumb_paint;
        thumb_paint.setColor(SK_ColorWHITE);
        thumb_paint.setStyle(SkPaint::kFill_Style);
        thumb_paint.setAntiAlias(true);
        canvas->drawCircle(thumb_x, thumb_y, thumb_radius, thumb_paint);

        // 滑块边框
        SkPaint thumb_border;
        thumb_border.setColor(SkColorSetRGB(180, 180, 180));
        thumb_border.setStyle(SkPaint::kStroke_Style);
        thumb_border.setStrokeWidth(1);
        thumb_border.setAntiAlias(true);
        canvas->drawCircle(thumb_x, thumb_y, thumb_radius, thumb_border);
    }
}

void RenderInlineBlock::PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box) {
    if (!textarea) return;

    std::string value = textarea->GetValue();
    bool is_placeholder = false;

    if (value.empty()) {
        // 显示placeholder
        value = textarea->GetPlaceholder();
        is_placeholder = true;
    }

    // 创建字体
    FontDescriptor desc;
    desc.family = computed_style_.font_family;
    desc.size = computed_style_.font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 获取字体度量
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    // 行高 = ascent + descent，再加一点行间距
    float line_height = -font_metrics.fAscent + font_metrics.fDescent;
    if (font_metrics.fLeading > 0) {
        line_height += font_metrics.fLeading;
    } else {
        // 如果没有 leading，添加一点额外间距（通常是字体大小的20%）
        line_height += computed_style_.font_size * 0.2f;
    }

    // 注意：box.content_x/y 已经包含了 border + padding 的偏移
    // box.content_width/height 已经是去掉 border 和 padding 后的尺寸
    // 滚动条应该在 padding 区域内，贴着 border

    // 获取滚动偏移量
    float scroll_top = textarea->GetScrollTop();
    float scroll_left = textarea->GetScrollLeft();

    // padding 区域的尺寸（包含 content + padding，不包含 border）
    float padding_box_width = box.content_width + box.padding_left + box.padding_right;
    float padding_box_height = box.content_height + box.padding_top + box.padding_bottom;
    // padding 区域的起始位置
    float padding_box_x = box.border_left_width;
    float padding_box_y = box.border_top_width;

    // 计算内容尺寸以确定是否需要滚动条
    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
    float content_height = textarea->GetContentHeight(line_height);
    float max_line_width = textarea->GetMaxLineWidth(font);

    // 判断是否需要滚动条（用 content 区域判断）
    bool need_v_scrollbar = content_height > box.content_height;
    bool need_h_scrollbar = max_line_width > box.content_width;

    // 如果需要滚动条，文本可见区域减去滚动条占用的空间
    float visible_height = box.content_height - (need_h_scrollbar ? scrollbar_width : 0);
    float visible_width = box.content_width - (need_v_scrollbar ? scrollbar_width : 0);

    // 重新检查是否需要另一个滚动条（因为可见区域减小了）
    if (!need_v_scrollbar && content_height > visible_height) {
        need_v_scrollbar = true;
        visible_width = box.content_width - scrollbar_width;
    }
    if (!need_h_scrollbar && max_line_width > visible_width) {
        need_h_scrollbar = true;
        visible_height = box.content_height - scrollbar_width;
    }

    // 文本起始位置：content 区域的左上角
    float text_x = box.content_x - scroll_left;  // 应用横向滚动偏移
    float text_y = box.content_y - font_metrics.fAscent - scroll_top;  // 应用垂直滚动偏移

    // 计算文本可用宽度
    float text_available_width = visible_width;

    // 裁剪文本区域（防止文本溢出到滚动条区域）
    canvas->save();
    SkRect text_clip_rect = SkRect::MakeXYWH(
        box.content_x,
        box.content_y,
        visible_width,
        visible_height
    );
    canvas->clipRect(text_clip_rect);

    if (!value.empty()) {
        // 创建文本渲染器
        TextRenderer text_renderer(canvas);

        // 设置文本颜色
        lightui::Paint text_paint;
        if (is_placeholder) {
            text_paint.SetColor(SkColorSetRGB(150, 150, 150));
        } else if (!computed_style_.color.empty()) {
            text_paint.SetColor(lightui::Color::Parse(computed_style_.color));
        } else {
            text_paint.SetColor(SK_ColorBLACK);
        }

        // 绘制多行文本 - textarea 只按换行符分割，不自动换行（长行可横向滚动）
        std::istringstream stream(value);
        std::string line;
        float current_y = text_y;
        while (std::getline(stream, line)) {
            text_renderer.DrawTextWithEmoji(line, text_x, current_y, font, text_paint);
            current_y += line_height;
        }
    }

    // 如果有焦点，绘制选中高亮和光标
    auto element = std::static_pointer_cast<Element>(GetNode());
    if (element && element->HasPseudoClass("focus")) {
        // 只在光标位置变化时才自动滚动到光标位置
        if (textarea->NeedsScrollToCursor()) {
            textarea->EnsureCursorVisible(line_height, visible_height, visible_width, font);
            textarea->ResetScrollToCursor();
            // 重新获取滚动偏移量（可能已经更新）
            scroll_top = textarea->GetScrollTop();
            scroll_left = textarea->GetScrollLeft();
            text_x = box.content_x - scroll_left;
            text_y = box.content_y - font_metrics.fAscent - scroll_top;
        }

        std::string actual_value = textarea->GetValue();
        int sel_start = textarea->GetSelectionStart();
        int sel_end = textarea->GetSelectionEnd();

        // 绘制选中高亮
        if (sel_start != sel_end) {
            int start = std::min(sel_start, sel_end);
            int end = std::max(sel_start, sel_end);

            // 选中高亮颜色
            SkPaint selection_paint;
            selection_paint.setColor(SkColorSetARGB(128, 66, 133, 244));  // 半透明蓝色
            selection_paint.setStyle(SkPaint::kFill_Style);

            // 将文本分割成行来绘制选中区域
            std::vector<std::string> lines;
            std::istringstream stream(actual_value);
            std::string line;
            while (std::getline(stream, line)) {
                lines.push_back(line);
            }
            if (actual_value.empty() || (!actual_value.empty() && actual_value.back() == '\n')) {
                lines.push_back("");
            }

            int char_offset = 0;
            float current_y = text_y;

            for (size_t line_idx = 0; line_idx < lines.size(); line_idx++) {
                const std::string& current_line = lines[line_idx];
                int line_char_count = static_cast<int>(utf8::CharCount(current_line));
                int line_start = char_offset;
                int line_end = char_offset + line_char_count;

                // 检查选中区域是否与此行重叠
                if (end > line_start && start < line_end + 1) {
                    int sel_start_in_line = std::max(0, start - line_start);
                    int sel_end_in_line = std::min(line_char_count, end - line_start);

                    // 计算选中区域的 x 坐标 - 使用支持 CJK/Emoji 的测量方法
                    TextRenderer temp_renderer(canvas);
                    float sel_x_start = text_x;
                    float sel_x_end = text_x;

                    if (sel_start_in_line > 0) {
                        std::string before_sel = utf8::SubstrByChar(current_line, 0, sel_start_in_line);
                        sel_x_start += temp_renderer.MeasureTextWidthWithEmoji(before_sel, font);
                    }

                    if (sel_end_in_line > 0) {
                        std::string to_sel_end = utf8::SubstrByChar(current_line, 0, sel_end_in_line);
                        sel_x_end += temp_renderer.MeasureTextWidthWithEmoji(to_sel_end, font);
                    }

                    // 如果选中包含换行符，只高亮到行尾实际字符位置，不扩展到整行宽度
                    if (end > line_end && sel_end_in_line == line_char_count) {
                        // 选中区域已经到行尾，不再额外扩展
                        // sel_x_end 保持为实际文本宽度
                    }

                    // 绘制选中矩形
                    SkRect sel_rect = SkRect::MakeXYWH(
                        sel_x_start,
                        current_y + font_metrics.fAscent,
                        sel_x_end - sel_x_start,
                        -font_metrics.fAscent + font_metrics.fDescent
                    );
                    canvas->drawRect(sel_rect, selection_paint);
                }

                char_offset = line_end + 1;  // +1 for newline
                current_y += line_height;
            }
        }

        // 绘制光标（基于时间的闪烁）
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        bool cursor_visible = (ms / 500) % 2 == 0;

        if (cursor_visible) {
            // 计算光标位置 - 使用 sel_end 作为光标位置
            int cursor_pos = sel_end;
            size_t cursor_byte_pos = utf8::CharPosToBytePos(actual_value, cursor_pos);
            std::string text_before_cursor = actual_value.substr(0, cursor_byte_pos);

            // 找到最后一个换行符的位置
            size_t last_newline = text_before_cursor.rfind('\n');
            std::string current_line_before_cursor;
            float cursor_y = text_y;

            if (last_newline != std::string::npos) {
                // 光标在某一行中
                current_line_before_cursor = text_before_cursor.substr(last_newline + 1);
                // 计算光标所在行（每个\n增加一行）
                int line_count = std::count(text_before_cursor.begin(), text_before_cursor.end(), '\n');
                cursor_y += line_count * line_height;
            } else {
                // 光标在第一行
                current_line_before_cursor = text_before_cursor;
            }

            // 测量光标前的文本宽度 - 使用支持 CJK/Emoji 的测量方法
            float cursor_x = text_x;
            if (!current_line_before_cursor.empty()) {
                TextRenderer temp_renderer(canvas);
                cursor_x += temp_renderer.MeasureTextWidthWithEmoji(current_line_before_cursor, font);
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

    // 恢复裁剪
    canvas->restore();

    // ========== 绘制滚动条 ==========
    // 滚动条应该在 padding 区域内，贴着 border 内侧
    // scrollbar_width 已在前面定义
    const float scrollbar_min_size = 20.0f;
    const SkColor scrollbar_track_color = SkColorSetARGB(30, 0, 0, 0);
    const SkColor scrollbar_thumb_color = SkColorSetARGB(128, 100, 100, 100);
    // content_height, max_line_width, need_v_scrollbar, need_h_scrollbar 已在前面计算

    // 绘制垂直滚动条 - 贴紧右边框（在 padding 区域的右边缘）
    if (need_v_scrollbar) {
        // 滚动条 x 位置：padding 区域右边缘减去滚动条宽度
        float track_x = padding_box_x + padding_box_width - scrollbar_width;
        float track_y = padding_box_y;
        float track_height = need_h_scrollbar ? (padding_box_height - scrollbar_width) : padding_box_height;

        // 绘制滚动条轨道
        SkPaint track_paint;
        track_paint.setColor(scrollbar_track_color);
        track_paint.setAntiAlias(true);
        SkRRect track_rrect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height),
            scrollbar_width / 2, scrollbar_width / 2
        );
        canvas->drawRRect(track_rrect, track_paint);

        // 计算滚动条滑块尺寸和位置
        float thumb_ratio = visible_height / content_height;
        float thumb_height = std::max(scrollbar_min_size, track_height * thumb_ratio);
        float max_scroll = content_height - visible_height;
        float scroll_ratio = max_scroll > 0 ? (scroll_top / max_scroll) : 0;
        float thumb_y = track_y + scroll_ratio * (track_height - thumb_height);

        // 绘制滚动条滑块
        SkPaint thumb_paint;
        thumb_paint.setColor(scrollbar_thumb_color);
        thumb_paint.setAntiAlias(true);
        SkRRect thumb_rrect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(track_x, thumb_y, scrollbar_width, thumb_height),
            scrollbar_width / 2, scrollbar_width / 2
        );
        canvas->drawRRect(thumb_rrect, thumb_paint);
    }

    // 绘制水平滚动条 - 贴紧下边框（在 padding 区域的下边缘）
    if (need_h_scrollbar) {
        float track_x = padding_box_x;
        // 滚动条 y 位置：padding 区域下边缘减去滚动条宽度
        float track_y = padding_box_y + padding_box_height - scrollbar_width;
        float track_width = need_v_scrollbar ? (padding_box_width - scrollbar_width) : padding_box_width;

        // 绘制滚动条轨道
        SkPaint track_paint;
        track_paint.setColor(scrollbar_track_color);
        track_paint.setAntiAlias(true);
        SkRRect track_rrect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(track_x, track_y, track_width, scrollbar_width),
            scrollbar_width / 2, scrollbar_width / 2
        );
        canvas->drawRRect(track_rrect, track_paint);

        // 计算滚动条滑块尺寸和位置
        float thumb_ratio = visible_width / max_line_width;
        float thumb_width = std::max(scrollbar_min_size, track_width * thumb_ratio);
        float max_scroll = max_line_width - visible_width;
        float scroll_ratio = max_scroll > 0 ? (scroll_left / max_scroll) : 0;
        float thumb_x = track_x + scroll_ratio * (track_width - thumb_width);

        // 绘制滚动条滑块
        SkPaint thumb_paint;
        thumb_paint.setColor(scrollbar_thumb_color);
        thumb_paint.setAntiAlias(true);
        SkRRect thumb_rrect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(thumb_x, track_y, thumb_width, scrollbar_width),
            scrollbar_width / 2, scrollbar_width / 2
        );
        canvas->drawRRect(thumb_rrect, thumb_paint);
    }
}

void RenderInlineBlock::PaintSelectElement(SkCanvas* canvas, Element* select, const Box& box) {
    if (!select) return;

    // 获取 HTMLSelectElement
    auto select_element = dynamic_cast<HTMLSelectElement*>(select);
    if (!select_element) return;

    // 辅助函数：从option元素获取文本
    auto getOptionText = [](const std::shared_ptr<Element>& option) -> std::string {
        const auto& option_children = option->GetChildNodes();
        for (const auto& text_node : option_children) {
            if (text_node->GetNodeType() == NodeType::TEXT_NODE) {
                auto text = std::static_pointer_cast<Text>(text_node);
                return text->GetData();
            }
        }
        return "";
    };

    // 获取当前选中的文本
    std::string selected_text = "";
    auto options = select_element->GetOptions();
    long selected_index = select_element->GetSelectedIndex();
    if (selected_index >= 0 && selected_index < static_cast<long>(options.size())) {
        selected_text = getOptionText(options[selected_index]);
    } else if (!options.empty()) {
        selected_text = getOptionText(options[0]);
    }

    // 渲染选中的文本
    FontDescriptor desc;
    desc.family = computed_style_.font_family;
    desc.size = computed_style_.font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 获取字体度量
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    float line_height = -font_metrics.fAscent + font_metrics.fDescent;

    // Chrome select 的布局:
    // - 文字从 content_x 开始（box.content_x 已经是内容区域起点）
    // - 箭头区域大约 16px，在内容区域右侧
    const float arrow_area_width = 16.0f;

    if (!selected_text.empty()) {
        // 计算文本位置（垂直居中）
        float text_height = line_height;
        // content_x 已经是内容区域的起点，不需要再加 padding
        float text_x = box.content_x;
        float text_y = box.content_y + (box.content_height - text_height) / 2.0f - font_metrics.fAscent;

        // 计算文本可用宽度（减去箭头区域）
        float text_available_width = box.content_width - arrow_area_width;

        // 裁剪文本区域
        canvas->save();
        SkRect text_clip_rect = SkRect::MakeXYWH(text_x, box.content_y, text_available_width, box.content_height);
        canvas->clipRect(text_clip_rect);

        TextRenderer text_renderer(canvas);
        lightui::Paint text_paint;
        text_paint.SetColor(SK_ColorBLACK);
        text_renderer.DrawTextWithEmoji(selected_text, text_x, text_y, font, text_paint);

        canvas->restore();
    }

    // 绘制下拉箭头（Chrome 风格的 V 形箭头）
    SkPaint arrow_paint;
    arrow_paint.setColor(SkColorSetRGB(80, 80, 80));
    arrow_paint.setStyle(SkPaint::kStroke_Style);
    arrow_paint.setStrokeWidth(1.5f);
    arrow_paint.setAntiAlias(true);
    arrow_paint.setStrokeCap(SkPaint::kRound_Cap);
    arrow_paint.setStrokeJoin(SkPaint::kRound_Join);

    float arrow_width = 6.0f;
    float arrow_height = 4.0f;

    // 获取 layout 信息，箭头右边缘距离右边框 2px
    const auto& layout = GetLayoutInfo();
    float border_right = box.border_right_width;
    // 箭头右边缘 = 元素宽度 - 边框 - 2px
    // 箭头中心 = 箭头右边缘 - 箭头半宽
    float arrow_right_edge = layout.width - border_right - 2.0f;
    float arrow_center_x = arrow_right_edge - arrow_width / 2.0f;
    float arrow_center_y = box.content_y + box.content_height / 2.0f;

    SkPath arrow_path;
    arrow_path.moveTo(arrow_center_x - arrow_width / 2.0f, arrow_center_y - arrow_height / 2.0f);
    arrow_path.lineTo(arrow_center_x, arrow_center_y + arrow_height / 2.0f);
    arrow_path.lineTo(arrow_center_x + arrow_width / 2.0f, arrow_center_y - arrow_height / 2.0f);
    canvas->drawPath(arrow_path, arrow_paint);
}

void RenderInlineBlock::PaintProgressElement(SkCanvas* canvas, HTMLProgressElement* progress, const Box& box) {
    if (!progress) return;

    double value = progress->GetValue();
    double max = progress->GetMax();
    double position = (max > 0) ? (value / max) : 0.0;
    position = std::clamp(position, 0.0, 1.0);

    // 背景轨道
    SkPaint track_paint;
    track_paint.setColor(SkColorSetRGB(230, 230, 230));
    track_paint.setStyle(SkPaint::kFill_Style);
    track_paint.setAntiAlias(true);

    float track_height = box.content_height * 0.6f;
    float track_y = box.content_y + (box.content_height - track_height) / 2.0f;
    float corner_radius = track_height / 2.0f;

    SkRRect track_rrect;
    track_rrect.setRectXY(
        SkRect::MakeXYWH(box.content_x, track_y, box.content_width, track_height),
        corner_radius, corner_radius
    );
    canvas->drawRRect(track_rrect, track_paint);

    // 进度条填充 - 使用浏览器默认蓝色（Chrome/Edge: rgb(30, 144, 255)）
    if (position > 0) {
        SkPaint progress_paint;
        progress_paint.setColor(SkColorSetRGB(30, 144, 255));  // 浏览器默认蓝色
        progress_paint.setStyle(SkPaint::kFill_Style);
        progress_paint.setAntiAlias(true);

        float progress_width = box.content_width * static_cast<float>(position);
        SkRRect progress_rrect;
        progress_rrect.setRectXY(
            SkRect::MakeXYWH(box.content_x, track_y, progress_width, track_height),
            corner_radius, corner_radius
        );
        canvas->drawRRect(progress_rrect, progress_paint);
    }
}

void RenderInlineBlock::PaintMeterElement(SkCanvas* canvas, HTMLMeterElement* meter, const Box& box) {
    if (!meter) return;

    double value = meter->GetValue();
    double min = meter->GetMin();
    double max = meter->GetMax();
    double low = meter->GetLow();
    double high = meter->GetHigh();
    double optimum = meter->GetOptimum();

    // 计算填充比例
    double range = max - min;
    double position = (range > 0) ? ((value - min) / range) : 0.0;
    position = std::clamp(position, 0.0, 1.0);

    // 背景轨道
    SkPaint track_paint;
    track_paint.setColor(SkColorSetRGB(230, 230, 230));
    track_paint.setStyle(SkPaint::kFill_Style);
    track_paint.setAntiAlias(true);

    float track_height = box.content_height * 0.6f;
    float track_y = box.content_y + (box.content_height - track_height) / 2.0f;
    float corner_radius = track_height / 2.0f;

    SkRRect track_rrect;
    track_rrect.setRectXY(
        SkRect::MakeXYWH(box.content_x, track_y, box.content_width, track_height),
        corner_radius, corner_radius
    );
    canvas->drawRRect(track_rrect, track_paint);

    // 根据值确定颜色
    SkColor meter_color;
    double low_threshold = (low - min) / range;
    double high_threshold = (high - min) / range;
    double optimum_normalized = (optimum - min) / range;

    // 颜色逻辑：基于 optimum 位置和当前值
    if (optimum_normalized >= high_threshold) {
        // optimum在高区域，高值是好的
        if (position >= high_threshold) {
            meter_color = SkColorSetRGB(76, 175, 80);   // 绿色
        } else if (position >= low_threshold) {
            meter_color = SkColorSetRGB(255, 193, 7);  // 黄色
        } else {
            meter_color = SkColorSetRGB(244, 67, 54);  // 红色
        }
    } else if (optimum_normalized <= low_threshold) {
        // optimum在低区域，低值是好的
        if (position <= low_threshold) {
            meter_color = SkColorSetRGB(76, 175, 80);   // 绿色
        } else if (position <= high_threshold) {
            meter_color = SkColorSetRGB(255, 193, 7);  // 黄色
        } else {
            meter_color = SkColorSetRGB(244, 67, 54);  // 红色
        }
    } else {
        // optimum在中间区域，中间值是好的
        if (position >= low_threshold && position <= high_threshold) {
            meter_color = SkColorSetRGB(76, 175, 80);   // 绿色
        } else {
            meter_color = SkColorSetRGB(255, 193, 7);  // 黄色
        }
    }

    // 绘制meter填充
    if (position > 0) {
        SkPaint meter_paint;
        meter_paint.setColor(meter_color);
        meter_paint.setStyle(SkPaint::kFill_Style);
        meter_paint.setAntiAlias(true);

        float meter_width = box.content_width * static_cast<float>(position);
        SkRRect meter_rrect;
        meter_rrect.setRectXY(
            SkRect::MakeXYWH(box.content_x, track_y, meter_width, track_height),
            corner_radius, corner_radius
        );
        canvas->drawRRect(meter_rrect, meter_paint);
    }
}

} // namespace lightui

