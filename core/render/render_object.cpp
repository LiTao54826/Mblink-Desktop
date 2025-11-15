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
#include <algorithm>
#include <iostream>

namespace lightui {

// ========== RenderObject 基类实现 ==========

RenderObject::RenderObject(RenderObjectType type)
    : type_(type)
    , node_()
    , parent_()
    , children_()
    , computed_style_()
    , layout_info_()
    , needs_layout_(true)
    , needs_paint_(true)
    , yoga_node_(nullptr) {
    // 创建 Yoga 节点
    yoga_node_ = YGNodeNew();
}

RenderObject::~RenderObject() {
    // 释放 Yoga 节点
    if (yoga_node_) {
        YGNodeFree(yoga_node_);
        yoga_node_ = nullptr;
    }
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

    // 同步 Yoga 树结构
    if (yoga_node_ && child->yoga_node_) {
        YGNodeInsertChild(yoga_node_, child->yoga_node_, YGNodeGetChildCount(yoga_node_));
    }

    MarkNeedsLayout();
    MarkNeedsPaint();
}

void RenderObject::RemoveChild(std::shared_ptr<RenderObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        // 同步 Yoga 树结构
        if (yoga_node_ && child->yoga_node_) {
            YGNodeRemoveChild(yoga_node_, child->yoga_node_);
        }

        (*it)->SetParent(nullptr);
        children_.erase(it);
        MarkNeedsLayout();
        MarkNeedsPaint();
    }
}

void RenderObject::RemoveAllChildren() {
    // 同步 Yoga 树结构
    if (yoga_node_) {
        for (auto& child : children_) {
            if (child->yoga_node_) {
                YGNodeRemoveChild(yoga_node_, child->yoga_node_);
            }
            child->SetParent(nullptr);
        }
    } else {
        for (auto& child : children_) {
            child->SetParent(nullptr);
        }
    }

    children_.clear();
    MarkNeedsLayout();
    MarkNeedsPaint();
}

void RenderObject::Layout(float parent_width, float parent_height) {
    // 判断是否使用 Flexbox 布局
    bool use_flexbox = (computed_style_.display == "flex" ||
                        computed_style_.display == "inline-flex");

    if (use_flexbox && yoga_node_) {
        // 使用 Yoga 进行 Flexbox 布局
        LayoutWithYoga(parent_width, parent_height);
    } else {
        // 使用 Block/Inline 布局（CSS 标准布局）
        LayoutBlockInline(parent_width, parent_height);
    }

    needs_layout_ = false;
}

void RenderObject::Paint(SkCanvas* canvas) {
    // 基类默认实现：什么都不做
    needs_paint_ = false;
}

// ========== RenderBlock 实现 ==========

void RenderBlock::Layout(float parent_width, float parent_height) {
    // 判断是否使用 Flexbox 布局
    bool use_flexbox = (computed_style_.display == "flex" ||
                        computed_style_.display == "inline-flex");

    if (use_flexbox && yoga_node_) {
        // 使用 Yoga 进行 Flexbox 布局
        LayoutWithYoga(parent_width, parent_height);
        needs_layout_ = false;
        return;
    }

    // 使用传统布局
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

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

    // 创建盒模型
    Box box;
    box.content_x = layout.content_rect.left();
    box.content_y = layout.content_rect.top();
    box.content_width = layout.content_rect.width();
    box.content_height = layout.content_rect.height();

    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);

    box.border_top_width = style.border.width.ToPx();
    box.border_right_width = style.border.width.ToPx();
    box.border_bottom_width = style.border.width.ToPx();
    box.border_left_width = style.border.width.ToPx();

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

    // 渲染边框
    if (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) {
        std::string border_width = std::to_string(style.border.width.value) + "px";
        std::string border_style = "solid"; // 简化
        std::string border_color = "#000000"; // 简化

        if (style.border_radius.top_left.IsZero() &&
            style.border_radius.top_right.IsZero() &&
            style.border_radius.bottom_right.IsZero() &&
            style.border_radius.bottom_left.IsZero()) {
            renderer.RenderBorder(box, border_width, border_style, border_color);
        } else {
            renderer.RenderRoundedBorder(box, border_width, border_style, border_color, style.border_radius);
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

    // 绘制子元素
    for (auto& child : children_) {
        if (child->NeedsPaint()) {
            child->Paint(canvas);
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

            // 如果有焦点且不是placeholder，绘制光标
            if (!value.empty()) {
                auto element = std::static_pointer_cast<Element>(GetNode());
                if (element && element->HasPseudoClass("focus")) {
                    // 计算光标位置
                    int cursor_pos = input->GetSelectionStart();
                    std::string text_before_cursor = value.substr(0, cursor_pos);

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

                    // 绘制光标（简单的竖线，暂时不实现闪烁）
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

    // 设置内联元素的尺寸
    // 如果有显式宽度/高度，使用显式值；否则使用子元素计算的值
    layout_info_.width = has_explicit_width ? explicit_width : total_width;
    layout_info_.height = has_explicit_height ? explicit_height : (max_height > 0 ? max_height : 20.0f);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;

    // 设置子元素的位置（水平排列，垂直居中）
    float current_x = 0;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        child_layout.x = current_x;
        // 垂直居中：如果父元素高度大于子元素高度，则居中对齐
        child_layout.y = (layout_info_.height - child_layout.height) / 2.0f;
        current_x += child_layout.width;
    }
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

    // 绘制背景（如果有）
    if (!style.background_color.empty()) {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setAntiAlias(true);

        // 绘制背景矩形（只覆盖内容宽度）
        SkRect bg_rect = SkRect::MakeWH(layout.width, layout.height);
        canvas->drawRect(bg_rect, bg_paint);
    }

    // 绘制边框（如果有）
    if (style.border.style != CSSBorderStyle::NONE && !style.border.width.IsZero()) {
        SkPaint border_paint;
        border_paint.setColor(style.border.color);
        border_paint.setStyle(SkPaint::kStroke_Style);
        border_paint.setStrokeWidth(style.border.width.ToPx());
        border_paint.setAntiAlias(true);

        // 绘制边框矩形
        SkRect border_rect = SkRect::MakeWH(layout.width, layout.height);

        // 如果有圆角，使用圆角矩形
        if (!style.border_radius.top_left.IsZero() ||
            !style.border_radius.top_right.IsZero() ||
            !style.border_radius.bottom_right.IsZero() ||
            !style.border_radius.bottom_left.IsZero()) {
            SkRRect rrect;
            SkVector radii[4] = {
                {style.border_radius.top_left.ToPx(), style.border_radius.top_left.ToPx()},
                {style.border_radius.top_right.ToPx(), style.border_radius.top_right.ToPx()},
                {style.border_radius.bottom_right.ToPx(), style.border_radius.bottom_right.ToPx()},
                {style.border_radius.bottom_left.ToPx(), style.border_radius.bottom_left.ToPx()}
            };
            rrect.setRectRadii(border_rect, radii);
            canvas->drawRRect(rrect, border_paint);
        } else {
            canvas->drawRect(border_rect, border_paint);
        }
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

    // 绘制所有子元素
    for (auto& child : children_) {
        if (child->NeedsPaint()) {
            child->Paint(canvas);
        }
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

            // 如果有焦点且不是placeholder，绘制光标
            if (!value.empty() && !is_placeholder) {
                auto element = std::static_pointer_cast<Element>(GetNode());
                if (element && element->HasPseudoClass("focus")) {
                    // 计算光标位置
                    int cursor_pos = input->GetSelectionStart();
                    std::string text_before_cursor = value.substr(0, cursor_pos);

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

                    // 绘制光标（简单的竖线，暂时不实现闪烁）
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

    // 测量文本
    auto metrics = text_renderer.MeasureText(text_, font);

    // 注意：只设置 width 和 height，不修改 x 和 y（由父元素设置）
    layout_info_.width = metrics.width;
    layout_info_.height = metrics.height;
    layout_info_.content_rect = SkRect::MakeWH(metrics.width, metrics.height);
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

    // 保存画布状态
    canvas->save();
    canvas->translate(layout.x, layout.y);

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

    // 绘制文本（支持阴影）
    if (!style.text_shadow.empty()) {
        // 使用 ShadowRenderer 渲染带阴影的文本
        ShadowRenderer::RenderTextWithShadow(canvas, text_, font, 0, baseline_y, text_color, style.text_shadow);
    } else {
        // 普通文本渲染
        lightui::Paint text_paint;
        text_paint.SetColor(text_color);
        text_renderer.DrawText(text_, 0, baseline_y, font, text_paint);
    }

    // 绘制文本装饰（下划线、删除线等）
    if (style.text_decoration == "underline") {
        // 下划线：在基线下方
        float underline_y = baseline_y + font_metrics.fUnderlinePosition;
        float underline_thickness = font_metrics.fUnderlineThickness;
        if (underline_thickness < 1.0f) underline_thickness = 1.0f;

        SkPaint line_paint;
        line_paint.setColor(text_color);
        line_paint.setStrokeWidth(underline_thickness);
        line_paint.setAntiAlias(true);

        canvas->drawLine(0, underline_y, layout.width, underline_y, line_paint);
    } else if (style.text_decoration == "line-through") {
        // 删除线：在文字中间
        float strikethrough_y = baseline_y + font_metrics.fStrikeoutPosition;
        float strikethrough_thickness = font_metrics.fStrikeoutThickness;
        if (strikethrough_thickness < 1.0f) strikethrough_thickness = 1.0f;

        SkPaint line_paint;
        line_paint.setColor(text_color);
        line_paint.setStrokeWidth(strikethrough_thickness);
        line_paint.setAntiAlias(true);

        canvas->drawLine(0, strikethrough_y, layout.width, strikethrough_y, line_paint);
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

// ========== Yoga 布局辅助函数 ==========

void RenderObject::ApplyYogaStyle() {
    if (!yoga_node_) {
        return;
    }

    const auto& style = computed_style_;

    // Display
    if (style.display == "none") {
        YGNodeStyleSetDisplay(yoga_node_, YGDisplayNone);
        return;  // 如果是 none，其他样式不需要设置
    } else if (style.display == "flex" || style.display == "inline-flex") {
        YGNodeStyleSetDisplay(yoga_node_, YGDisplayFlex);
    } else {
        // block, inline, inline-block 等使用默认的 flex 显示
        YGNodeStyleSetDisplay(yoga_node_, YGDisplayFlex);
    }

    // Flex Direction
    if (style.flex_direction == "row") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionRow);
    } else if (style.flex_direction == "row-reverse") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionRowReverse);
    } else if (style.flex_direction == "column") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionColumn);
    } else if (style.flex_direction == "column-reverse") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionColumnReverse);
    }

    // Flex Wrap
    if (style.flex_wrap == "nowrap") {
        YGNodeStyleSetFlexWrap(yoga_node_, YGWrapNoWrap);
    } else if (style.flex_wrap == "wrap") {
        YGNodeStyleSetFlexWrap(yoga_node_, YGWrapWrap);
    } else if (style.flex_wrap == "wrap-reverse") {
        YGNodeStyleSetFlexWrap(yoga_node_, YGWrapWrapReverse);
    }

    // Justify Content
    if (style.justify_content == "flex-start") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifyFlexStart);
    } else if (style.justify_content == "flex-end") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifyFlexEnd);
    } else if (style.justify_content == "center") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifyCenter);
    } else if (style.justify_content == "space-between") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifySpaceBetween);
    } else if (style.justify_content == "space-around") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifySpaceAround);
    } else if (style.justify_content == "space-evenly") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifySpaceEvenly);
    }

    // Align Items
    if (style.align_items == "flex-start") {
        YGNodeStyleSetAlignItems(yoga_node_, YGAlignFlexStart);
    } else if (style.align_items == "flex-end") {
        YGNodeStyleSetAlignItems(yoga_node_, YGAlignFlexEnd);
    } else if (style.align_items == "center") {
        YGNodeStyleSetAlignItems(yoga_node_, YGAlignCenter);
    } else if (style.align_items == "baseline") {
        YGNodeStyleSetAlignItems(yoga_node_, YGAlignBaseline);
    } else if (style.align_items == "stretch") {
        YGNodeStyleSetAlignItems(yoga_node_, YGAlignStretch);
    }

    // Align Content
    if (style.align_content == "flex-start") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignFlexStart);
    } else if (style.align_content == "flex-end") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignFlexEnd);
    } else if (style.align_content == "center") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignCenter);
    } else if (style.align_content == "stretch") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignStretch);
    } else if (style.align_content == "space-between") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignSpaceBetween);
    } else if (style.align_content == "space-around") {
        YGNodeStyleSetAlignContent(yoga_node_, YGAlignSpaceAround);
    }

    // Align Self
    if (style.align_self == "auto") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignAuto);
    } else if (style.align_self == "flex-start") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignFlexStart);
    } else if (style.align_self == "flex-end") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignFlexEnd);
    } else if (style.align_self == "center") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignCenter);
    } else if (style.align_self == "baseline") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignBaseline);
    } else if (style.align_self == "stretch") {
        YGNodeStyleSetAlignSelf(yoga_node_, YGAlignStretch);
    }

    // Flex Grow, Shrink
    YGNodeStyleSetFlexGrow(yoga_node_, style.flex_grow);
    YGNodeStyleSetFlexShrink(yoga_node_, style.flex_shrink);

    // Flex Basis
    if (style.flex_basis.IsAuto()) {
        YGNodeStyleSetFlexBasisAuto(yoga_node_);
    } else if (style.flex_basis.unit == CSSUnit::PERCENT) {
        YGNodeStyleSetFlexBasisPercent(yoga_node_, style.flex_basis.value);
    } else {
        YGNodeStyleSetFlexBasis(yoga_node_, style.flex_basis.ToPx(0, style.font_size));
    }

    // Width
    if (style.width.IsAuto()) {
        YGNodeStyleSetWidthAuto(yoga_node_);
    } else if (style.width.unit == CSSUnit::PERCENT) {
        YGNodeStyleSetWidthPercent(yoga_node_, style.width.value);
    } else {
        YGNodeStyleSetWidth(yoga_node_, style.width.ToPx(0, style.font_size));
    }

    // Height
    if (style.height.IsAuto()) {
        YGNodeStyleSetHeightAuto(yoga_node_);
    } else if (style.height.unit == CSSUnit::PERCENT) {
        YGNodeStyleSetHeightPercent(yoga_node_, style.height.value);
    } else {
        YGNodeStyleSetHeight(yoga_node_, style.height.ToPx(0, style.font_size));
    }

    // Min/Max Width
    if (!style.min_width.IsZero()) {
        if (style.min_width.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetMinWidthPercent(yoga_node_, style.min_width.value);
        } else {
            YGNodeStyleSetMinWidth(yoga_node_, style.min_width.ToPx(0, style.font_size));
        }
    }

    if (style.max_width.unit != CSSUnit::NONE) {
        if (style.max_width.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetMaxWidthPercent(yoga_node_, style.max_width.value);
        } else {
            YGNodeStyleSetMaxWidth(yoga_node_, style.max_width.ToPx(0, style.font_size));
        }
    }

    // Min/Max Height
    if (!style.min_height.IsZero()) {
        if (style.min_height.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetMinHeightPercent(yoga_node_, style.min_height.value);
        } else {
            YGNodeStyleSetMinHeight(yoga_node_, style.min_height.ToPx(0, style.font_size));
        }
    }

    if (style.max_height.unit != CSSUnit::NONE) {
        if (style.max_height.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetMaxHeightPercent(yoga_node_, style.max_height.value);
        } else {
            YGNodeStyleSetMaxHeight(yoga_node_, style.max_height.ToPx(0, style.font_size));
        }
    }

    // Padding
    auto apply_padding = [this](YGEdge edge, const CSSLength& value) {
        if (value.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetPaddingPercent(yoga_node_, edge, value.value);
        } else {
            YGNodeStyleSetPadding(yoga_node_, edge, value.ToPx(0, computed_style_.font_size));
        }
    };

    apply_padding(YGEdgeTop, style.padding_top);
    apply_padding(YGEdgeRight, style.padding_right);
    apply_padding(YGEdgeBottom, style.padding_bottom);
    apply_padding(YGEdgeLeft, style.padding_left);

    // Margin
    auto apply_margin = [this](YGEdge edge, const CSSLength& value) {
        if (value.IsAuto()) {
            YGNodeStyleSetMarginAuto(yoga_node_, edge);
        } else if (value.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetMarginPercent(yoga_node_, edge, value.value);
        } else {
            YGNodeStyleSetMargin(yoga_node_, edge, value.ToPx(0, computed_style_.font_size));
        }
    };

    apply_margin(YGEdgeTop, style.margin_top);
    apply_margin(YGEdgeRight, style.margin_right);
    apply_margin(YGEdgeBottom, style.margin_bottom);
    apply_margin(YGEdgeLeft, style.margin_left);

    // Border
    YGNodeStyleSetBorder(yoga_node_, YGEdgeTop, style.border_top_width);
    YGNodeStyleSetBorder(yoga_node_, YGEdgeRight, style.border_right_width);
    YGNodeStyleSetBorder(yoga_node_, YGEdgeBottom, style.border_bottom_width);
    YGNodeStyleSetBorder(yoga_node_, YGEdgeLeft, style.border_left_width);

    // Gap
    if (!style.gap.IsZero()) {
        float gap_px = style.gap.ToPx(0, style.font_size);
        YGNodeStyleSetGap(yoga_node_, YGGutterAll, gap_px);
    }
    if (!style.row_gap.IsZero()) {
        float row_gap_px = style.row_gap.ToPx(0, style.font_size);
        YGNodeStyleSetGap(yoga_node_, YGGutterRow, row_gap_px);
    }
    if (!style.column_gap.IsZero()) {
        float column_gap_px = style.column_gap.ToPx(0, style.font_size);
        YGNodeStyleSetGap(yoga_node_, YGGutterColumn, column_gap_px);
    }

    // Position
    if (style.position == "relative") {
        YGNodeStyleSetPositionType(yoga_node_, YGPositionTypeRelative);
    } else if (style.position == "absolute") {
        YGNodeStyleSetPositionType(yoga_node_, YGPositionTypeAbsolute);
    } else {
        YGNodeStyleSetPositionType(yoga_node_, YGPositionTypeStatic);
    }

    // Position offsets
    auto apply_position = [this](YGEdge edge, const CSSLength& value) {
        if (value.IsAuto()) {
            YGNodeStyleSetPositionAuto(yoga_node_, edge);
        } else if (value.unit == CSSUnit::PERCENT) {
            YGNodeStyleSetPositionPercent(yoga_node_, edge, value.value);
        } else {
            YGNodeStyleSetPosition(yoga_node_, edge, value.ToPx(0, computed_style_.font_size));
        }
    };

    if (!style.top.IsAuto()) {
        apply_position(YGEdgeTop, style.top);
    }
    if (!style.right.IsAuto()) {
        apply_position(YGEdgeRight, style.right);
    }
    if (!style.bottom.IsAuto()) {
        apply_position(YGEdgeBottom, style.bottom);
    }
    if (!style.left.IsAuto()) {
        apply_position(YGEdgeLeft, style.left);
    }
}

void RenderObject::ReadYogaLayout() {
    if (!yoga_node_) {
        return;
    }

    // 读取 Yoga 计算的布局结果
    layout_info_.x = YGNodeLayoutGetLeft(yoga_node_);
    layout_info_.y = YGNodeLayoutGetTop(yoga_node_);
    layout_info_.width = YGNodeLayoutGetWidth(yoga_node_);
    layout_info_.height = YGNodeLayoutGetHeight(yoga_node_);

    // 计算各个矩形区域
    float left = layout_info_.x;
    float top = layout_info_.y;
    float right = left + layout_info_.width;
    float bottom = top + layout_info_.height;

    // Margin 区域（最外层）
    float margin_left = YGNodeLayoutGetMargin(yoga_node_, YGEdgeLeft);
    float margin_top = YGNodeLayoutGetMargin(yoga_node_, YGEdgeTop);
    float margin_right = YGNodeLayoutGetMargin(yoga_node_, YGEdgeRight);
    float margin_bottom = YGNodeLayoutGetMargin(yoga_node_, YGEdgeBottom);

    layout_info_.margin_rect = SkRect::MakeLTRB(
        left - margin_left,
        top - margin_top,
        right + margin_right,
        bottom + margin_bottom
    );

    // Border 区域
    layout_info_.border_rect = SkRect::MakeLTRB(left, top, right, bottom);

    // Padding 区域
    float border_left = YGNodeLayoutGetBorder(yoga_node_, YGEdgeLeft);
    float border_top = YGNodeLayoutGetBorder(yoga_node_, YGEdgeTop);
    float border_right = YGNodeLayoutGetBorder(yoga_node_, YGEdgeRight);
    float border_bottom = YGNodeLayoutGetBorder(yoga_node_, YGEdgeBottom);

    layout_info_.padding_rect = SkRect::MakeLTRB(
        left + border_left,
        top + border_top,
        right - border_right,
        bottom - border_bottom
    );

    // Content 区域（最内层）
    float padding_left = YGNodeLayoutGetPadding(yoga_node_, YGEdgeLeft);
    float padding_top = YGNodeLayoutGetPadding(yoga_node_, YGEdgeTop);
    float padding_right = YGNodeLayoutGetPadding(yoga_node_, YGEdgeRight);
    float padding_bottom = YGNodeLayoutGetPadding(yoga_node_, YGEdgeBottom);

    layout_info_.content_rect = SkRect::MakeLTRB(
        left + border_left + padding_left,
        top + border_top + padding_top,
        right - border_right - padding_right,
        bottom - border_bottom - padding_bottom
    );

    layout_info_.is_laid_out = true;
}

void RenderObject::LayoutWithYoga(float available_width, float available_height) {
    if (!yoga_node_) {
        return;
    }

    // 应用样式到 Yoga 节点
    ApplyYogaStyle();

    // 递归应用子节点样式
    for (auto& child : children_) {
        child->ApplyYogaStyle();
    }

    // 执行 Yoga 布局计算
    YGNodeCalculateLayout(yoga_node_, available_width, available_height, YGDirectionLTR);

    // 读取布局结果
    ReadYogaLayout();

    // 递归读取子节点布局结果并递归布局子节点
    for (size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        child->ReadYogaLayout();

        // 递归布局子节点（使用子节点的计算尺寸）
        float child_width = child->layout_info_.width;
        float child_height = child->layout_info_.height;
        child->Layout(child_width, child_height);
    }
}

void RenderObject::LayoutTraditional(float parent_width, float parent_height) {
    // 这是原来的布局逻辑，保留用于非 Flexbox 布局
    // 子类会覆盖这个方法
    layout_info_.is_laid_out = true;
}

} // namespace lightui

