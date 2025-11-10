/**
 * @file render_object.cpp
 * @brief 渲染对象实现
 */

#include "render_object.h"
#include "box_renderer.h"
#include "text_renderer.h"
#include "color.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
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
    // 基类默认实现：什么都不做
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderObject::Paint(SkCanvas* canvas) {
    // 基类默认实现：什么都不做
    needs_paint_ = false;
}

// ========== RenderBlock 实现 ==========

void RenderBlock::Layout(float parent_width, float parent_height) {
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
    float current_y = 0;
    float max_child_height = 0;

    // HACK: 如果是最外层的 body RenderBlock（没有父元素），添加 120px 的 top offset
    // 这是为了避免内容被窗口顶部的黑色区域遮挡
    float body_top_offset = 0;
    if (!GetParent()) {
        body_top_offset = 120.0f;
        static bool logged = false;
        if (!logged) {
            std::cout << "[RenderBlock] Body element detected, adding top offset: " << body_top_offset << std::endl;
            logged = true;
        }
    }

    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();

        float new_x = padding_left + border_left;
        float new_y = current_y + padding_top + border_top + body_top_offset;

        child_layout.x = new_x;
        child_layout.y = new_y;

        current_y += child_layout.height;
        max_child_height = std::max(max_child_height, child_layout.height);
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
    
    // 渲染阴影
    if (!style.box_shadow.empty()) {
        renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);
    }
    
    // 渲染背景
    renderer.RenderBackgroundAdvanced(box, styles, &style.border_radius);
    
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

// ========== RenderInline 实现 ==========

void RenderInline::Layout(float parent_width, float parent_height) {
    // 简化实现：内联元素暂时按块级处理
    // 调用基类的布局逻辑（简化版）
    layout_info_.width = parent_width;
    layout_info_.height = 20.0f; // 默认高度
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderInline::Paint(SkCanvas* canvas) {
    // 简化实现：内联元素暂时不绘制
    needs_paint_ = false;
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

    // 创建文本渲染器
    TextRenderer text_renderer(canvas);

    // 设置文本样式
    lightui::Paint text_paint;
    if (!style.color.empty()) {
        text_paint.SetColor(lightui::Color::Parse(style.color));
    } else {
        // 默认黑色
        text_paint.SetColor(SK_ColorBLACK);
    }

    // 绘制文本
    text_renderer.DrawText(text_, 0, layout.height * 0.8f, font, text_paint);

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

} // namespace lightui

