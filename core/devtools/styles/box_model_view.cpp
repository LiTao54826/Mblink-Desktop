/**
 * @file box_model_view.cpp
 * @brief 盒模型可视化视图实现
 */

#include "box_model_view.h"
#include "core/dom/document.h"
#include "core/lexbor/style_manager.h"
#include "core/window/window_manager.h"
#include "core/render/objects/render_object.h"

#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

#include <string>
#include <functional>
#include <iostream>
#include <cstdio>

namespace mbink {

BoxModelView::BoxModelView() = default;

BoxModelView::~BoxModelView() = default;

void BoxModelView::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
}

BoxModelData BoxModelView::GetBoxModelData() const {
    BoxModelData data;

    auto element = element_.lock();
    if (!element) return data;

    // 从渲染树获取实际布局数据
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return data;
    
    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return data;
    
    // 递归查找对应元素的 RenderObject
    std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> find_render_object;
    find_render_object = [&](std::shared_ptr<RenderObject> obj) -> std::shared_ptr<RenderObject> {
        if (!obj) return nullptr;
        
        auto node = obj->GetNode();
        if (node && node.get() == element.get()) {
            return obj;
        }
        
        for (const auto& child : obj->GetChildren()) {
            auto found = find_render_object(child);
            if (found) return found;
        }
        
        return nullptr;
    };
    
    auto render_obj = find_render_object(root_render);
    if (!render_obj) {
        return data;
    }
    
    // 从 RenderObject 获取布局信息和计算样式
    const auto& layout = render_obj->GetLayoutInfo();
    const auto& style = render_obj->GetComputedStyle();
    
    // 布局引擎输出的 layout.width 和 layout.height 已经是 CSS 像素值
    // 不需要进行 DPI 缩放转换
    float total_width = layout.width;
    float total_height = layout.height;
    
    // 获取 CSS 像素宽度用于百分比计算
    float css_width = total_width;
    
    // 获取 padding（优先使用 CSSEdges，如果为0则尝试单独字段）
    // ToPx 返回的是 CSS 像素值，不需要再除以 dpi_scale
    data.padding_top = style.padding.top.ToPx(css_width, style.font_size);
    data.padding_right = style.padding.right.ToPx(css_width, style.font_size);
    data.padding_bottom = style.padding.bottom.ToPx(css_width, style.font_size);
    data.padding_left = style.padding.left.ToPx(css_width, style.font_size);
    // 如果 CSSEdges 为0，尝试单独字段
    if (data.padding_top == 0) data.padding_top = style.padding_top.ToPx(css_width, style.font_size);
    if (data.padding_right == 0) data.padding_right = style.padding_right.ToPx(css_width, style.font_size);
    if (data.padding_bottom == 0) data.padding_bottom = style.padding_bottom.ToPx(css_width, style.font_size);
    if (data.padding_left == 0) data.padding_left = style.padding_left.ToPx(css_width, style.font_size);
    
    // 获取 margin（优先使用 CSSEdges，如果为0则尝试单独字段）
    data.margin_top = style.margin.top.ToPx(css_width, style.font_size);
    data.margin_right = style.margin.right.ToPx(css_width, style.font_size);
    data.margin_bottom = style.margin.bottom.ToPx(css_width, style.font_size);
    data.margin_left = style.margin.left.ToPx(css_width, style.font_size);
    // 如果 CSSEdges 为0，尝试单独字段
    if (data.margin_top == 0) data.margin_top = style.margin_top.ToPx(css_width, style.font_size);
    if (data.margin_right == 0) data.margin_right = style.margin_right.ToPx(css_width, style.font_size);
    if (data.margin_bottom == 0) data.margin_bottom = style.margin_bottom.ToPx(css_width, style.font_size);
    if (data.margin_left == 0) data.margin_left = style.margin_left.ToPx(css_width, style.font_size);
    
    // 获取 border（使用单独的 float 字段，已经是计算后的 CSS 像素值）
    data.border_top = style.border_top_width;
    data.border_right = style.border_right_width;
    data.border_bottom = style.border_bottom_width;
    data.border_left = style.border_left_width;
    
    // 计算内容尺寸：总尺寸 - padding - border
    // 这与浏览器 DevTools 的 box model 显示一致
    data.content_width = total_width - data.padding_left - data.padding_right - data.border_left - data.border_right;
    data.content_height = total_height - data.padding_top - data.padding_bottom - data.border_top - data.border_bottom;
    
    // 确保内容尺寸不为负
    if (data.content_width < 0) data.content_width = 0;
    if (data.content_height < 0) data.content_height = 0;

    return data;
}

void BoxModelView::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    auto data = GetBoxModelData();
    RenderBoxDiagram(canvas, x, y, width, height, data);
}

void BoxModelView::RenderBoxDiagram(SkCanvas* canvas, float x, float y, float width, float height,
                                     const BoxModelData& data) {
    // Chrome DevTools 风格的 Box Model 图表
    // 保存视图原点（用于坐标转换）
    view_x_ = x;
    view_y_ = y;
    
    // 计算图表尺寸（居中显示）
    diagram_width_ = 280;
    diagram_height_ = 180;
    // 保存相对于视图的偏移（不是绝对坐标）
    diagram_x_ = (width - diagram_width_) / 2;
    diagram_y_ = 15;
    
    // 计算绝对坐标用于绘制
    float abs_diagram_x = x + diagram_x_;
    float abs_diagram_y = y + diagram_y_;

    // 各层的固定像素大小（Chrome 风格）
    const float layer_size = 28.0f;  // 每层的厚度
    
    // 保存 inset 值用于命中测试
    margin_inset_ = 0;
    border_inset_ = layer_size;
    padding_inset_ = layer_size * 2;
    content_inset_ = layer_size * 3;

    // Chrome DevTools 颜色
    SkColor margin_color = SkColorSetARGB(255, 243, 181, 133);   // 橙色/米色
    SkColor border_color = SkColorSetARGB(255, 253, 221, 155);   // 黄色
    SkColor padding_color = SkColorSetARGB(255, 194, 221, 166);  // 绿色
    SkColor content_color = SkColorSetARGB(255, 162, 197, 224);  // 蓝色
    SkColor inactive_color = SkColorSetARGB(255, 240, 240, 240); // 非活动区域（浅灰/白色）
    SkColor dash_color = SkColorSetARGB(255, 128, 128, 128);     // 虚线颜色

    // 根据悬停状态调整颜色 - Chrome 风格：只显示悬停区域的颜色，其他显示白色
    auto getColor = [this, inactive_color](BoxAreaType area, SkColor normal) -> SkColor {
        if (hovered_area_ == BoxAreaType::None) {
            // 没有悬停时，显示所有区域的正常颜色
            return normal;
        }
        if (hovered_area_ == area) {
            // 悬停的区域显示正常颜色
            return normal;
        }
        // 其他区域显示浅灰色/白色
        return inactive_color;
    };

    // 辅助函数：绘制虚线矩形边框
    auto drawDashedRect = [&](float rx, float ry, float rw, float rh) {
        SkPaint dash_paint;
        dash_paint.setColor(dash_color);
        dash_paint.setStyle(SkPaint::kStroke_Style);
        dash_paint.setStrokeWidth(1);
        float intervals[] = {4, 2};
        dash_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        canvas->drawRect(SkRect::MakeXYWH(rx, ry, rw, rh), dash_paint);
    };

    // ========== 绘制各层 ==========
    
    // Margin 层
    SkPaint margin_paint;
    margin_paint.setColor(getColor(BoxAreaType::Margin, margin_color));
    canvas->drawRect(SkRect::MakeXYWH(abs_diagram_x, abs_diagram_y, diagram_width_, diagram_height_), margin_paint);
    drawDashedRect(abs_diagram_x, abs_diagram_y, diagram_width_, diagram_height_);

    // Border 层
    float border_x = abs_diagram_x + layer_size;
    float border_y = abs_diagram_y + layer_size;
    float border_w = diagram_width_ - layer_size * 2;
    float border_h = diagram_height_ - layer_size * 2;
    SkPaint border_paint;
    border_paint.setColor(getColor(BoxAreaType::Border, border_color));
    canvas->drawRect(SkRect::MakeXYWH(border_x, border_y, border_w, border_h), border_paint);
    drawDashedRect(border_x, border_y, border_w, border_h);

    // Padding 层
    float padding_x = abs_diagram_x + layer_size * 2;
    float padding_y = abs_diagram_y + layer_size * 2;
    float padding_w = diagram_width_ - layer_size * 4;
    float padding_h = diagram_height_ - layer_size * 4;
    SkPaint padding_paint;
    padding_paint.setColor(getColor(BoxAreaType::Padding, padding_color));
    canvas->drawRect(SkRect::MakeXYWH(padding_x, padding_y, padding_w, padding_h), padding_paint);
    drawDashedRect(padding_x, padding_y, padding_w, padding_h);

    // Content 层
    float content_x = abs_diagram_x + layer_size * 3;
    float content_y = abs_diagram_y + layer_size * 3;
    float content_w = diagram_width_ - layer_size * 6;
    float content_h = diagram_height_ - layer_size * 6;
    SkPaint content_paint;
    content_paint.setColor(getColor(BoxAreaType::Content, content_color));
    canvas->drawRect(SkRect::MakeXYWH(content_x, content_y, content_w, content_h), content_paint);
    drawDashedRect(content_x, content_y, content_w, content_h);

    // ========== 绘制文本（使用支持中文的字体）==========
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 11.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);
    
    SkPaint text_paint;
    text_paint.setColor(SkColorSetRGB(48, 48, 48));
    text_paint.setAntiAlias(true);

    // 辅助函数：格式化数值（保留小数）
    auto formatValue = [](float val) -> std::string {
        if (val == static_cast<int>(val)) {
            return std::to_string(static_cast<int>(val));
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3g", val);
        return std::string(buf);
    };

    // 辅助函数：绘制居中文本
    auto drawCenteredText = [&](const std::string& text, float cx, float cy) {
        float tw = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
        canvas->drawString(text.c_str(), cx - tw / 2, cy + 4, font, text_paint);
    };

    // ========== Margin 层文本 ==========
    // 左上角标签
    canvas->drawString("margin", abs_diagram_x + 4, abs_diagram_y + 14, font, text_paint);
    // 顶部数值
    drawCenteredText(formatValue(data.margin_top), abs_diagram_x + diagram_width_ / 2, abs_diagram_y + layer_size / 2);
    // 底部数值
    drawCenteredText(formatValue(data.margin_bottom), abs_diagram_x + diagram_width_ / 2, abs_diagram_y + diagram_height_ - layer_size / 2);
    // 左侧数值
    drawCenteredText(formatValue(data.margin_left), abs_diagram_x + layer_size / 2, abs_diagram_y + diagram_height_ / 2);
    // 右侧数值
    drawCenteredText(formatValue(data.margin_right), abs_diagram_x + diagram_width_ - layer_size / 2, abs_diagram_y + diagram_height_ / 2);

    // ========== Border 层文本 ==========
    canvas->drawString("border", border_x + 4, border_y + 14, font, text_paint);
    drawCenteredText(formatValue(data.border_top), abs_diagram_x + diagram_width_ / 2, border_y + layer_size / 2);
    drawCenteredText(formatValue(data.border_bottom), abs_diagram_x + diagram_width_ / 2, border_y + border_h - layer_size / 2);
    drawCenteredText(formatValue(data.border_left), border_x + layer_size / 2, abs_diagram_y + diagram_height_ / 2);
    drawCenteredText(formatValue(data.border_right), border_x + border_w - layer_size / 2, abs_diagram_y + diagram_height_ / 2);

    // ========== Padding 层文本 ==========
    canvas->drawString("padding", padding_x + 4, padding_y + 14, font, text_paint);
    drawCenteredText(formatValue(data.padding_top), abs_diagram_x + diagram_width_ / 2, padding_y + layer_size / 2);
    drawCenteredText(formatValue(data.padding_bottom), abs_diagram_x + diagram_width_ / 2, padding_y + padding_h - layer_size / 2);
    drawCenteredText(formatValue(data.padding_left), padding_x + layer_size / 2, abs_diagram_y + diagram_height_ / 2);
    drawCenteredText(formatValue(data.padding_right), padding_x + padding_w - layer_size / 2, abs_diagram_y + diagram_height_ / 2);

    // ========== Content 层文本 ==========
    std::string content_size = formatValue(data.content_width) + " × " + formatValue(data.content_height);
    drawCenteredText(content_size, content_x + content_w / 2, content_y + content_h / 2);
}

void BoxModelView::RenderLabel(SkCanvas* canvas, const std::string& text, float x, float y) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 10.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    SkPaint paint;
    paint.setColor(SkColorSetRGB(51, 51, 51));
    paint.setAntiAlias(true);

    canvas->drawString(text.c_str(), x, y, font, paint);
}

BoxAreaType BoxModelView::HitTest(int x, int y) const {
    // x, y 是相对于 BoxModelView 的坐标
    // diagram_x_, diagram_y_ 是图表相对于视图的偏移
    
    // 检查是否在图表区域内
    if (x < diagram_x_ || x > diagram_x_ + diagram_width_ ||
        y < diagram_y_ || y > diagram_y_ + diagram_height_) {
        return BoxAreaType::None;
    }
    
    // 计算相对于图表的坐标
    float rel_x = x - diagram_x_;
    float rel_y = y - diagram_y_;
    
    // 使用固定的 layer_size（与 RenderBoxDiagram 一致）
    const float layer_size = 28.0f;
    
    // 检查是否在 Content 区域（最内层）
    float content_x_start = layer_size * 3;
    float content_x_end = diagram_width_ - layer_size * 3;
    float content_y_start = layer_size * 3;
    float content_y_end = diagram_height_ - layer_size * 3;
    
    if (rel_x >= content_x_start && rel_x <= content_x_end &&
        rel_y >= content_y_start && rel_y <= content_y_end) {
        return BoxAreaType::Content;
    }
    
    // 检查是否在 Padding 区域
    float padding_x_start = layer_size * 2;
    float padding_x_end = diagram_width_ - layer_size * 2;
    float padding_y_start = layer_size * 2;
    float padding_y_end = diagram_height_ - layer_size * 2;
    
    if (rel_x >= padding_x_start && rel_x <= padding_x_end &&
        rel_y >= padding_y_start && rel_y <= padding_y_end) {
        return BoxAreaType::Padding;
    }
    
    // 检查是否在 Border 区域
    float border_x_start = layer_size;
    float border_x_end = diagram_width_ - layer_size;
    float border_y_start = layer_size;
    float border_y_end = diagram_height_ - layer_size;
    
    if (rel_x >= border_x_start && rel_x <= border_x_end &&
        rel_y >= border_y_start && rel_y <= border_y_end) {
        return BoxAreaType::Border;
    }
    
    // 否则在 Margin 区域
    return BoxAreaType::Margin;
}

bool BoxModelView::HandleMouseMove(int x, int y) {
    BoxAreaType new_area = HitTest(x, y);
    
    if (new_area != hovered_area_) {
        hovered_area_ = new_area;
        
        // 调用回调通知外部（用于在 HTML 页面中高亮元素）
        if (on_hover_changed_) {
            on_hover_changed_(element_.lock(), new_area);
        }
        
        return true;  // 状态发生变化，需要重绘
    }
    return false;
}

} // namespace mbink
