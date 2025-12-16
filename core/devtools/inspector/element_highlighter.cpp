/**
 * @file element_highlighter.cpp
 * @brief 元素高亮覆盖层实现
 */

#include "element_highlighter.h"
#include "core/window/window_manager.h"
#include "core/render/render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"
#include <functional>
#include <iostream>

namespace lightui {

ElementHighlighter::ElementHighlighter() = default;

ElementHighlighter::~ElementHighlighter() = default;

void ElementHighlighter::SetHighlightedElement(std::shared_ptr<Element> element) {
    highlighted_element_ = element;
}

void ElementHighlighter::SetHoveredElement(std::shared_ptr<Element> element) {
    hovered_element_ = element;
}

void ElementHighlighter::ClearHighlight() {
    highlighted_element_.reset();
}

void ElementHighlighter::ClearHover() {
    hovered_element_.reset();
}

void ElementHighlighter::SetBoxModelHover(std::shared_ptr<Element> element, HighlightAreaType area) {
    box_model_hover_element_ = element;
    box_model_hover_area_ = area;
}

void ElementHighlighter::ClearBoxModelHover() {
    box_model_hover_element_.reset();
    box_model_hover_area_ = HighlightAreaType::None;
}

void ElementHighlighter::Render(SkCanvas* canvas) {
    // 优先渲染 Box Model 悬停高亮
    auto box_hover = box_model_hover_element_.lock();
    if (box_hover && box_model_hover_area_ != HighlightAreaType::None) {
        RenderBoxModelHighlight(canvas, box_hover, box_model_hover_area_);
        return;  // Box Model 悬停时不显示其他高亮
    }

    // 注意：RenderElementHighlight 目前使用硬编码坐标，暂时禁用
    // 选中元素的高亮通过 Box Model 悬停来实现
    // TODO: 实现从渲染树获取实际布局信息的 RenderElementHighlight
}

void ElementHighlighter::RenderElementHighlight(SkCanvas* canvas,
                                                  std::shared_ptr<Element> element,
                                                  bool is_hover) {
    if (!element) return;

    // 获取元素的布局信息
    // TODO: 从 RenderObject 获取实际的布局盒
    // 这里暂时使用模拟数据
    float x = 0, y = 0, width = 100, height = 50;
    float margin_top = 0, margin_right = 0, margin_bottom = 0, margin_left = 0;
    float border_top = 0, border_right = 0, border_bottom = 0, border_left = 0;
    float padding_top = 0, padding_right = 0, padding_bottom = 0, padding_left = 0;

    // 尝试从元素样式获取值
    // TODO: 实现从 ComputedStyle 获取实际值

    // 透明度调整
    float alpha_multiplier = is_hover ? 0.5f : 1.0f;

    auto adjust_alpha = [alpha_multiplier](SkColor color) -> SkColor {
        int a = SkColorGetA(color);
        a = static_cast<int>(a * alpha_multiplier);
        return SkColorSetA(color, a);
    };

    // 渲染盒模型各层
    if (options_.show_margin) {
        SkPaint paint;
        paint.setColor(adjust_alpha(margin_color_));
        // Margin 区域（外层）
        canvas->drawRect(SkRect::MakeXYWH(
            x - margin_left,
            y - margin_top,
            width + margin_left + margin_right,
            margin_top
        ), paint);
        canvas->drawRect(SkRect::MakeXYWH(
            x - margin_left,
            y + height,
            width + margin_left + margin_right,
            margin_bottom
        ), paint);
        canvas->drawRect(SkRect::MakeXYWH(
            x - margin_left,
            y,
            margin_left,
            height
        ), paint);
        canvas->drawRect(SkRect::MakeXYWH(
            x + width,
            y,
            margin_right,
            height
        ), paint);
    }

    if (options_.show_border) {
        SkPaint paint;
        paint.setColor(adjust_alpha(border_color_));
        // Border 区域
        canvas->drawRect(SkRect::MakeXYWH(x, y, width, border_top), paint);
        canvas->drawRect(SkRect::MakeXYWH(x, y + height - border_bottom, width, border_bottom), paint);
        canvas->drawRect(SkRect::MakeXYWH(x, y + border_top, border_left, height - border_top - border_bottom), paint);
        canvas->drawRect(SkRect::MakeXYWH(x + width - border_right, y + border_top, border_right, height - border_top - border_bottom), paint);
    }

    if (options_.show_padding) {
        SkPaint paint;
        paint.setColor(adjust_alpha(padding_color_));
        float inner_x = x + border_left;
        float inner_y = y + border_top;
        float inner_width = width - border_left - border_right;
        float inner_height = height - border_top - border_bottom;
        // Padding 区域
        canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y, inner_width, padding_top), paint);
        canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + inner_height - padding_bottom, inner_width, padding_bottom), paint);
        canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + padding_top, padding_left, inner_height - padding_top - padding_bottom), paint);
        canvas->drawRect(SkRect::MakeXYWH(inner_x + inner_width - padding_right, inner_y + padding_top, padding_right, inner_height - padding_top - padding_bottom), paint);
    }

    if (options_.show_content) {
        SkPaint paint;
        paint.setColor(adjust_alpha(content_color_));
        float content_x = x + border_left + padding_left;
        float content_y = y + border_top + padding_top;
        float content_width = width - border_left - border_right - padding_left - padding_right;
        float content_height = height - border_top - border_bottom - padding_top - padding_bottom;
        canvas->drawRect(SkRect::MakeXYWH(content_x, content_y, content_width, content_height), paint);
    }

    // 边框线
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(66, 133, 244));  // 蓝色边框
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(1);
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), border_paint);

    // 信息提示框
    if (options_.show_info_tooltip && !is_hover) {
        RenderInfoTooltip(canvas, element);
    }
}

void ElementHighlighter::RenderBoxModelHighlight(SkCanvas* canvas, std::shared_ptr<Element> element,
                                                   HighlightAreaType area) {
    if (!element || area == HighlightAreaType::None) return;

    // 从渲染树获取元素的布局信息
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return;
    
    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return;
    
    // 递归查找对应元素的 RenderObject 并计算绝对位置
    struct FindResult {
        std::shared_ptr<RenderObject> render_obj;
        float abs_x = 0;
        float abs_y = 0;
    };
    
    std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> find_render_object;
    find_render_object = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
        if (!obj) return {};
        
        const auto& layout = obj->GetLayoutInfo();
        // 当前元素的绝对位置（不受自身滚动影响）
        float current_x = offset_x + layout.x;
        float current_y = offset_y + layout.y;
        
        auto node = obj->GetNode();
        if (node && node.get() == element.get()) {
            // 找到目标元素，返回其绝对位置（不减去自身滚动）
            return {obj, current_x, current_y};
        }
        
        // 查找子元素时，需要减去当前元素的滚动偏移
        // 因为子元素的可见位置会随父元素滚动而移动
        float child_offset_x = current_x - obj->GetScrollX();
        float child_offset_y = current_y - obj->GetScrollY();
        
        for (const auto& child : obj->GetChildren()) {
            auto result = find_render_object(child, child_offset_x, child_offset_y);
            if (result.render_obj) return result;
        }
        
        return {};
    };
    
    auto result = find_render_object(root_render, 0.0f, 0.0f);
    if (!result.render_obj) return;
    
    const auto& layout = result.render_obj->GetLayoutInfo();
    const auto& style = result.render_obj->GetComputedStyle();
    
    // 对于有滚动的元素，高亮位置需要减去自身的滚动偏移
    // 这样高亮才能正确显示元素内容的实际渲染位置（可能在负坐标）
    float x = result.abs_x - result.render_obj->GetScrollX();
    float y = result.abs_y - result.render_obj->GetScrollY();
    float width = layout.width;
    float height = layout.height;
    
    // 获取盒模型数据
    float margin_top = style.margin.top.ToPx(width, style.font_size);
    float margin_right = style.margin.right.ToPx(width, style.font_size);
    float margin_bottom = style.margin.bottom.ToPx(width, style.font_size);
    float margin_left = style.margin.left.ToPx(width, style.font_size);
    if (margin_top == 0) margin_top = style.margin_top.ToPx(width, style.font_size);
    if (margin_right == 0) margin_right = style.margin_right.ToPx(width, style.font_size);
    if (margin_bottom == 0) margin_bottom = style.margin_bottom.ToPx(width, style.font_size);
    if (margin_left == 0) margin_left = style.margin_left.ToPx(width, style.font_size);
    
    float border_top = style.border_top_width;
    float border_right = style.border_right_width;
    float border_bottom = style.border_bottom_width;
    float border_left = style.border_left_width;
    
    float padding_top = style.padding.top.ToPx(width, style.font_size);
    float padding_right = style.padding.right.ToPx(width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(width, style.font_size);
    float padding_left = style.padding.left.ToPx(width, style.font_size);
    if (padding_top == 0) padding_top = style.padding_top.ToPx(width, style.font_size);
    if (padding_right == 0) padding_right = style.padding_right.ToPx(width, style.font_size);
    if (padding_bottom == 0) padding_bottom = style.padding_bottom.ToPx(width, style.font_size);
    if (padding_left == 0) padding_left = style.padding_left.ToPx(width, style.font_size);

    SkPaint paint;
    
    // 根据悬停区域绘制对应的高亮
    switch (area) {
        case HighlightAreaType::Margin: {
            paint.setColor(margin_color_);
            // 绘制 margin 区域（四个边）
            // 顶部 margin
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y - margin_top,
                width + margin_left + margin_right, margin_top), paint);
            // 底部 margin
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y + height,
                width + margin_left + margin_right, margin_bottom), paint);
            // 左侧 margin
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y,
                margin_left, height), paint);
            // 右侧 margin
            canvas->drawRect(SkRect::MakeXYWH(x + width, y,
                margin_right, height), paint);
            break;
        }
        case HighlightAreaType::Border: {
            paint.setColor(border_color_);
            // 绘制 border 区域
            // 顶部 border
            canvas->drawRect(SkRect::MakeXYWH(x, y, width, border_top), paint);
            // 底部 border
            canvas->drawRect(SkRect::MakeXYWH(x, y + height - border_bottom, width, border_bottom), paint);
            // 左侧 border
            canvas->drawRect(SkRect::MakeXYWH(x, y + border_top, border_left, height - border_top - border_bottom), paint);
            // 右侧 border
            canvas->drawRect(SkRect::MakeXYWH(x + width - border_right, y + border_top, border_right, height - border_top - border_bottom), paint);
            break;
        }
        case HighlightAreaType::Padding: {
            paint.setColor(padding_color_);
            float inner_x = x + border_left;
            float inner_y = y + border_top;
            float inner_width = width - border_left - border_right;
            float inner_height = height - border_top - border_bottom;
            // 绘制 padding 区域
            // 顶部 padding
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y, inner_width, padding_top), paint);
            // 底部 padding
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + inner_height - padding_bottom, inner_width, padding_bottom), paint);
            // 左侧 padding
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + padding_top, padding_left, inner_height - padding_top - padding_bottom), paint);
            // 右侧 padding
            canvas->drawRect(SkRect::MakeXYWH(inner_x + inner_width - padding_right, inner_y + padding_top, padding_right, inner_height - padding_top - padding_bottom), paint);
            break;
        }
        case HighlightAreaType::Content: {
            paint.setColor(content_color_);
            float content_x = x + border_left + padding_left;
            float content_y = y + border_top + padding_top;
            float content_width = width - border_left - border_right - padding_left - padding_right;
            float content_height = height - border_top - border_bottom - padding_top - padding_bottom;
            canvas->drawRect(SkRect::MakeXYWH(content_x, content_y, content_width, content_height), paint);
            break;
        }
        case HighlightAreaType::All: {
            // 显示所有区域
            // Margin
            paint.setColor(margin_color_);
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y - margin_top,
                width + margin_left + margin_right, margin_top), paint);
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y + height,
                width + margin_left + margin_right, margin_bottom), paint);
            canvas->drawRect(SkRect::MakeXYWH(x - margin_left, y, margin_left, height), paint);
            canvas->drawRect(SkRect::MakeXYWH(x + width, y, margin_right, height), paint);
            
            // Border
            paint.setColor(border_color_);
            canvas->drawRect(SkRect::MakeXYWH(x, y, width, border_top), paint);
            canvas->drawRect(SkRect::MakeXYWH(x, y + height - border_bottom, width, border_bottom), paint);
            canvas->drawRect(SkRect::MakeXYWH(x, y + border_top, border_left, height - border_top - border_bottom), paint);
            canvas->drawRect(SkRect::MakeXYWH(x + width - border_right, y + border_top, border_right, height - border_top - border_bottom), paint);
            
            // Padding
            paint.setColor(padding_color_);
            float inner_x = x + border_left;
            float inner_y = y + border_top;
            float inner_width = width - border_left - border_right;
            float inner_height = height - border_top - border_bottom;
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y, inner_width, padding_top), paint);
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + inner_height - padding_bottom, inner_width, padding_bottom), paint);
            canvas->drawRect(SkRect::MakeXYWH(inner_x, inner_y + padding_top, padding_left, inner_height - padding_top - padding_bottom), paint);
            canvas->drawRect(SkRect::MakeXYWH(inner_x + inner_width - padding_right, inner_y + padding_top, padding_right, inner_height - padding_top - padding_bottom), paint);
            
            // Content
            paint.setColor(content_color_);
            float content_x = x + border_left + padding_left;
            float content_y = y + border_top + padding_top;
            float content_width = width - border_left - border_right - padding_left - padding_right;
            float content_height = height - border_top - border_bottom - padding_top - padding_bottom;
            canvas->drawRect(SkRect::MakeXYWH(content_x, content_y, content_width, content_height), paint);
            break;
        }
        default:
            break;
    }
}

void ElementHighlighter::RenderBoxModel(SkCanvas* canvas, std::shared_ptr<Element> element) {
    // 在 BoxModelView 中实现详细的盒模型渲染
}

void ElementHighlighter::RenderInfoTooltip(SkCanvas* canvas, std::shared_ptr<Element> element) {
    if (!element) return;

    // 构建提示文本
    std::string tag = element->GetTagName();
    std::string id = element->GetAttribute("id");
    std::string cls = element->GetAttribute("class");

    std::string text = tag;
    if (!id.empty()) {
        text += "#" + id;
    }
    if (!cls.empty()) {
        // 只显示第一个 class
        size_t space_pos = cls.find(' ');
        if (space_pos != std::string::npos) {
            text += "." + cls.substr(0, space_pos);
        } else {
            text += "." + cls;
        }
    }

    // TODO: 添加尺寸信息
    // text += " | 100 × 50";

    // 获取元素位置
    float x = 0, y = 0;  // TODO: 从布局获取

    // 提示框背景
    SkFont font;
    font.setSize(11);

    float text_width = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
    float tooltip_width = text_width + 12;
    float tooltip_height = 20;
    float tooltip_x = x;
    float tooltip_y = y - tooltip_height - 4;

    SkPaint bg_paint;
    bg_paint.setColor(SkColorSetRGB(255, 238, 128));  // 黄色背景
    canvas->drawRect(SkRect::MakeXYWH(tooltip_x, tooltip_y, tooltip_width, tooltip_height), bg_paint);

    // 提示框文本
    SkPaint text_paint;
    text_paint.setColor(SkColorSetRGB(51, 51, 51));
    text_paint.setAntiAlias(true);
    canvas->drawString(text.c_str(), tooltip_x + 6, tooltip_y + 14, font, text_paint);
}

} // namespace lightui
