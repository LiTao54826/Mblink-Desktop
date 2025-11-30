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

    // 计算 padding
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_height, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_height, style.font_size);

    // 1. 计算宽度
    if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO) {
        // 显式设置了宽度
        layout_info_.width = style.width.ToPx(parent_width, style.font_size);
    } else {
        // 使用shrink-to-fit算法（包含 padding）
        layout_info_.width = CalculateShrinkToFitWidth(parent_width);
    }

    // 2. 布局子元素（考虑 padding）
    float content_height = 0;
    float total_child_width = 0;
    float content_width = layout_info_.width - padding_left - padding_right;

    // 首先布局所有子元素并计算总宽度和高度
    for (auto& child : children_) {
        if (child->NeedsLayout()) {
            child->Layout(content_width, parent_height);
        }

        auto& child_layout = child->GetLayoutInfo();
        total_child_width += child_layout.width;
        content_height = std::max(content_height, child_layout.height);
    }

    // 3. 计算高度（包含 padding）
    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
        // 显式设置了高度
        layout_info_.height = style.height.ToPx(parent_height, style.font_size);
    } else {
        // 根据内容计算高度 + padding
        layout_info_.height = content_height > 0 ? (content_height + padding_top + padding_bottom) : 20.0f;
    }

    // 4. 设置子元素位置，支持 text-align
    float start_x = padding_left;

    // 处理 text-align
    if (style.text_align == "center" && total_child_width < content_width) {
        // 居中对齐
        start_x = padding_left + (content_width - total_child_width) / 2.0f;
    } else if (style.text_align == "right" && total_child_width < content_width) {
        // 右对齐
        start_x = padding_left + content_width - total_child_width;
    }

    float current_x = start_x;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        child_layout.x = current_x;
        // 垂直居中
        child_layout.y = padding_top + (layout_info_.height - padding_top - padding_bottom - child_layout.height) / 2.0f;
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
            const auto& children_nodes = element->GetChildNodes();
            for (const auto& child : children_nodes) {
                if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto option = std::static_pointer_cast<Element>(child);
                    if (option->GetTagName() == "option") {
                        const auto& option_children = option->GetChildNodes();
                        for (const auto& text_node : option_children) {
                            if (text_node->GetNodeType() == NodeType::TEXT_NODE) {
                                auto text = std::static_pointer_cast<Text>(text_node);
                                std::string option_text = text->GetData();
                                if (option_text.length() > longest_text.length()) {
                                    longest_text = option_text;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if (!longest_text.empty()) {
                // 使用字体测量实际宽度
                FontDescriptor desc;
                desc.family = style.font_family;
                desc.size = style.font_size;
                desc.weight = FontWeight::NORMAL;
                desc.style = FontStyle::NORMAL;

                SkFont font = FontManager::GetInstance().LoadFont(desc);
                content_width = font.measureText(
                    longest_text.c_str(),
                    longest_text.length(),
                    SkTextEncoding::kUTF8
                );
            }
        }
    }

    // 对于其他元素，使用子元素计算或使用字体测量
    if (content_width == 0) {
        for (auto& child : children_) {
            if (child->GetType() == RenderObjectType::TEXT) {
                auto text_child = std::static_pointer_cast<RenderText>(child);
                std::string text = text_child->GetText();

                // 使用实际字体测量宽度
                FontDescriptor desc;
                desc.family = style.font_family;
                desc.size = style.font_size;
                desc.weight = FontWeight::NORMAL;
                desc.style = FontStyle::NORMAL;

                SkFont font = FontManager::GetInstance().LoadFont(desc);
                float text_width = font.measureText(
                    text.c_str(),
                    text.length(),
                    SkTextEncoding::kUTF8
                );
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
        // 显式设置了宽度 - 包含 padding 和 border (border-box)
        width = style.width.ToPx(available_width, style.font_size);
    } else {
        // 使用 shrink-to-fit 算法
        width = CalculateShrinkToFitWidth(available_width);
    }

    // 计算高度
    float height;
    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO) {
        // 显式设置了高度
        height = style.height.ToPx(available_width, style.font_size);
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
            // 没有子元素（如 input 元素），基于 font-size 计算
            // 浏览器 input 高度 ≈ font-size + 小量内部空间 + padding + border
            // 实测：16px font + 10px*2 padding + 2px*2 border = 42.5px
            // 所以内部空间约为 42.5 - 24 - 16 = 2.5px，即 font-size * 0.15
            float content_line_height = style.font_size * 1.15f;
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

    auto node = GetNode();
    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 创建盒模型
    Box box;
    box.content_x = 0;
    box.content_y = 0;
    box.content_width = layout.width;
    box.content_height = layout.height;

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

        if (style.border_radius.top_left.IsZero() &&
            style.border_radius.top_right.IsZero() &&
            style.border_radius.bottom_right.IsZero() &&
            style.border_radius.bottom_left.IsZero()) {
            renderer.RenderBorder(box, border_width, border_style, border_color);
        } else {
            renderer.RenderRoundedBorder(box, border_width, border_style, border_color, style.border_radius);
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

    // 渲染表单控件特定内容
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        
        // Input元素
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            PaintInputElement(canvas, input_element.get(), box);
        }
        
        // Textarea元素
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            PaintTextAreaElement(canvas, textarea_element.get(), box);
        }
        
        // Select元素
        if (element->GetTagName() == "select") {
            PaintSelectElement(canvas, element.get(), box);
            // Select元素不绘制子元素（option元素由PaintSelectElement处理）
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

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

void RenderInlineBlock::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    if (!input) return;

    InputType type = input->GetInputType();
    std::string value = input->GetValue();

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

        // 计算文本位置
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        float text_height = -font_metrics.fAscent + font_metrics.fDescent;
        float text_y = box.content_y + (box.content_height - text_height) / 2 - font_metrics.fAscent;
        float text_x = box.content_x + computed_style_.padding.left.ToPx();

        // 计算文本可用宽度（减去左右padding）
        float text_available_width = box.content_width - computed_style_.padding.left.ToPx() - computed_style_.padding.right.ToPx();

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

            // 绘制文本
            text_renderer.DrawText(display_text, text_x, text_y, font, text_paint);
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

                float sel_y_top = box.content_y + computed_style_.padding.top.ToPx();
                float sel_height = box.content_height - computed_style_.padding.top.ToPx() - computed_style_.padding.bottom.ToPx();
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

                // 计算光标的Y坐标（从文本顶部到底部）
                float cursor_y_top = box.content_y + computed_style_.padding.top.ToPx();
                float cursor_y_bottom = box.content_y + box.content_height - computed_style_.padding.bottom.ToPx();

                // 绘制光标
                SkPaint cursor_paint;
                cursor_paint.setColor(SK_ColorBLACK);
                cursor_paint.setStrokeWidth(1);
                cursor_paint.setAntiAlias(true);

                canvas->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
            }
        }

        // 恢复裁剪
        canvas->restore();
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

    // 计算文本起始位置 - 使用 border-box 的坐标系统
    float padding_left = computed_style_.padding.left.ToPx();
    float padding_top = computed_style_.padding.top.ToPx();
    float padding_right = computed_style_.padding.right.ToPx();
    float padding_bottom = computed_style_.padding.bottom.ToPx();

    // 获取滚动偏移量
    float scroll_top = textarea->GetScrollTop();
    float scroll_left = textarea->GetScrollLeft();

    // 基础可见区域（不考虑滚动条）
    float base_visible_height = box.content_height - padding_top - padding_bottom;
    float base_visible_width = box.content_width - padding_left - padding_right;

    // 计算内容尺寸以确定是否需要滚动条
    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
    float content_height = textarea->GetContentHeight(line_height);
    float max_line_width = textarea->GetMaxLineWidth(font);

    // 判断是否需要滚动条（先用基础可见区域判断）
    bool need_v_scrollbar = content_height > base_visible_height;
    bool need_h_scrollbar = max_line_width > base_visible_width;

    // 如果需要滚动条，减去滚动条占用的空间
    float visible_height = base_visible_height - (need_h_scrollbar ? scrollbar_width : 0);
    float visible_width = base_visible_width - (need_v_scrollbar ? scrollbar_width : 0);

    // 重新检查是否需要另一个滚动条（因为可见区域减小了）
    if (!need_v_scrollbar && content_height > visible_height) {
        need_v_scrollbar = true;
        visible_width = base_visible_width - scrollbar_width;
    }
    if (!need_h_scrollbar && max_line_width > visible_width) {
        need_h_scrollbar = true;
        visible_height = base_visible_height - scrollbar_width;
    }

    float text_x = box.content_x + padding_left - scroll_left;  // 应用横向滚动偏移
    float text_y = box.content_y + padding_top - font_metrics.fAscent - scroll_top;  // 应用垂直滚动偏移

    // 计算文本可用宽度（减去左右padding和滚动条）
    float text_available_width = visible_width;

    // 裁剪文本区域（防止文本溢出）
    // 注意：裁剪区域应该是固定的可见区域，不受滚动影响
    float clip_x = box.content_x + padding_left;
    float clip_y = box.content_y + padding_top;
    canvas->save();
    SkRect text_clip_rect = SkRect::MakeXYWH(
        clip_x,
        clip_y,
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

        // 绘制多行文本
        text_renderer.DrawMultilineText(value, text_x, text_y, text_available_width, line_height, font, text_paint);
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
            text_x = box.content_x + padding_left - scroll_left;
            text_y = box.content_y + padding_top - font_metrics.fAscent - scroll_top;
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

                    // 计算选中区域的 x 坐标
                    float sel_x_start = text_x;
                    float sel_x_end = text_x;

                    if (sel_start_in_line > 0) {
                        std::string before_sel = utf8::SubstrByChar(current_line, 0, sel_start_in_line);
                        sel_x_start += font.measureText(before_sel.c_str(), before_sel.length(), SkTextEncoding::kUTF8);
                    }

                    if (sel_end_in_line > 0) {
                        std::string to_sel_end = utf8::SubstrByChar(current_line, 0, sel_end_in_line);
                        sel_x_end += font.measureText(to_sel_end.c_str(), to_sel_end.length(), SkTextEncoding::kUTF8);
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

    // 恢复裁剪
    canvas->restore();

    // ========== 绘制滚动条 ==========
    // scrollbar_width 已在前面定义
    const float scrollbar_min_size = 20.0f;
    const SkColor scrollbar_track_color = SkColorSetARGB(30, 0, 0, 0);
    const SkColor scrollbar_thumb_color = SkColorSetARGB(128, 100, 100, 100);
    // content_height, max_line_width, need_v_scrollbar, need_h_scrollbar 已在前面计算

    // 绘制垂直滚动条
    if (need_v_scrollbar) {
        float track_x = box.content_x + box.content_width - scrollbar_width - 2;
        float track_y = box.content_y + padding_top;
        float track_height = visible_height;

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

    // 绘制水平滚动条
    if (need_h_scrollbar) {
        float track_x = box.content_x + padding_left;
        float track_y = box.content_y + box.content_height - scrollbar_width - 2;
        float track_width = visible_width;

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
    
    // 获取选中的option
    std::string selected_text = "";
    const auto& children = select->GetChildNodes();
    
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto option = std::static_pointer_cast<Element>(child);
            if (option->GetTagName() == "option") {
                // 检查是否有selected属性
                if (option->HasAttribute("selected")) {
                    // 获取option的文本内容
                    const auto& option_children = option->GetChildNodes();
                    for (const auto& text_node : option_children) {
                        if (text_node->GetNodeType() == NodeType::TEXT_NODE) {
                            auto text = std::static_pointer_cast<Text>(text_node);
                            selected_text = text->GetData();
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // 如果没有找到selected的option，使用第一个option
    if (selected_text.empty()) {
        for (const auto& child : children) {
            if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto option = std::static_pointer_cast<Element>(child);
                if (option->GetTagName() == "option") {
                    const auto& option_children = option->GetChildNodes();
                    for (const auto& text_node : option_children) {
                        if (text_node->GetNodeType() == NodeType::TEXT_NODE) {
                            auto text = std::static_pointer_cast<Text>(text_node);
                            selected_text = text->GetData();
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // 渲染选中的文本
    if (!selected_text.empty()) {
        FontDescriptor desc;
        desc.family = computed_style_.font_family;
        desc.size = computed_style_.font_size;
        desc.weight = FontWeight::NORMAL;
        desc.style = FontStyle::NORMAL;

        SkFont font = FontManager::GetInstance().LoadFont(desc);

        // 获取字体度量
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);

        // 计算文本位置（垂直居中 - 和Input一样的公式）
        float text_height = -font_metrics.fAscent + font_metrics.fDescent;
        float text_x = box.content_x + computed_style_.padding.left.ToPx();
        float text_y = box.content_y + (box.content_height - text_height) / 2.0f - font_metrics.fAscent;

        // 计算文本可用宽度（减去左右padding和箭头空间）
        float text_available_width = box.content_width - computed_style_.padding.left.ToPx() - computed_style_.padding.right.ToPx();

        // 裁剪文本区域（防止文本溢出）
        canvas->save();
        SkRect text_clip_rect = SkRect::MakeXYWH(
            text_x,
            box.content_y,
            text_available_width,
            box.content_height
        );
        canvas->clipRect(text_clip_rect);

        // 创建文本渲染器
        TextRenderer text_renderer(canvas);

        // 设置文本颜色
        lightui::Paint text_paint;
        text_paint.SetColor(SK_ColorBLACK);

        // 绘制文本
        text_renderer.DrawText(selected_text, text_x, text_y, font, text_paint);

        // 恢复裁剪
        canvas->restore();
    }
    
    // 绘制下拉箭头
    SkPaint arrow_paint;
    arrow_paint.setColor(SK_ColorBLACK);
    arrow_paint.setStyle(SkPaint::kFill_Style);
    arrow_paint.setAntiAlias(true);

    // 箭头位置（右侧，在padding区域内居中）
    float arrow_size = 8.0f;  // 增大箭头尺寸，更容易看到
    float padding_right = computed_style_.padding.right.ToPx();

    // 箭头在右侧padding区域内水平居中
    float arrow_x = box.content_x + box.content_width - padding_right + (padding_right - arrow_size) / 2.0f;
    float arrow_y = box.content_y + box.content_height / 2.0f;

    // 绘制向下的三角形（更大更明显）
    SkPath arrow_path;
    arrow_path.moveTo(arrow_x, arrow_y - 4);  // 顶部左点
    arrow_path.lineTo(arrow_x + arrow_size, arrow_y - 4);  // 顶部右点
    arrow_path.lineTo(arrow_x + arrow_size / 2.0f, arrow_y + 4);  // 底部中点
    arrow_path.close();

    canvas->drawPath(arrow_path, arrow_paint);
}

} // namespace lightui

