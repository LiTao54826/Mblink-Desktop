/**
 * @file render_svg.cpp
 * @brief SVG渲染对象实现
 */

#include "render_svg.h"
#include "dom/svg_element.h"
#include "color.h"
#include "text/font_manager.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkTypeface.h"
#include <cmath>
#include <iostream>

namespace lightui {

// ========== RenderSVG 基类实现 ==========

RenderSVG::RenderSVG() : RenderObject(RenderObjectType::BLOCK) {}

void RenderSVG::SetSVGElement(std::shared_ptr<SVGElement> element) {
    svg_element_ = element;
}

void RenderSVG::Paint(SkCanvas* canvas) {
    // 基类不绘制任何内容
}

void RenderSVG::Layout(float parent_width, float parent_height) {
    // 基类使用默认布局
    RenderObject::Layout(parent_width, parent_height);
}

SkPaint RenderSVG::CreateFillPaint() const {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);

    auto element = svg_element_.lock();
    if (element) {
        std::string fill = element->GetFill();
        if (fill != "none" && !fill.empty()) {
            SkColor color = ParseColor(fill);
            float opacity = element->GetOpacity() * element->GetFillOpacity();
            paint.setColor(SkColorSetA(color, static_cast<uint8_t>(SkColorGetA(color) * opacity)));
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    } else {
        paint.setColor(SK_ColorBLACK);
    }

    return paint;
}

SkPaint RenderSVG::CreateStrokePaint() const {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);

    auto element = svg_element_.lock();
    if (element) {
        std::string stroke = element->GetStroke();
        if (!stroke.empty() && stroke != "none") {
            SkColor color = ParseColor(stroke);
            float opacity = element->GetOpacity() * element->GetStrokeOpacity();
            paint.setColor(SkColorSetA(color, static_cast<uint8_t>(SkColorGetA(color) * opacity)));
            paint.setStrokeWidth(element->GetStrokeWidth());
        } else {
            paint.setColor(SK_ColorTRANSPARENT);
        }
    }

    return paint;
}

SkColor RenderSVG::ParseColor(const std::string& color) {
    if (color.empty() || color == "none") {
        return SK_ColorTRANSPARENT;
    }

    // 使用现有的颜色解析器
    return Color::Parse(color);
}

// ========== RenderSVGRoot 实现 ==========

RenderSVGRoot::RenderSVGRoot() : RenderSVG() {}

void RenderSVGRoot::SetSVGSVGElement(std::shared_ptr<SVGSVGElement> element) {
    svg_svg_element_ = element;
    SetSVGElement(element);
}

void RenderSVGRoot::Paint(SkCanvas* canvas) {
    auto element = svg_svg_element_.lock();
    if (!element) {
        // std::cerr << "[SVG] RenderSVGRoot::Paint - element is null!" << std::endl;
        return;
    }

    auto& layout = GetLayoutInfo();
    (void)layout; // Suppress unused warning

    // 先绘制背景（如果有）
    const auto& style = GetComputedStyle();
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        canvas->drawRect(SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height), bg_paint);
    }

    canvas->save();

    // 先移动到 SVG 元素的位置
    canvas->translate(layout.x, layout.y);

    // 设置裁剪区域（在 SVG 元素的本地坐标系中）
    canvas->clipRect(SkRect::MakeXYWH(0, 0, layout.width, layout.height));

    // 应用viewBox变换（如果有）
    float view_min_x, view_min_y, view_width, view_height;
    if (element->ParseViewBox(view_min_x, view_min_y, view_width, view_height)) {
        float scale_x = layout.width / view_width;
        float scale_y = layout.height / view_height;
        float scale = std::min(scale_x, scale_y);  // preserveAspectRatio: xMidYMid meet

        float translate_x = (layout.width - view_width * scale) / 2 - view_min_x * scale;
        float translate_y = (layout.height - view_height * scale) / 2 - view_min_y * scale;

        canvas->translate(translate_x, translate_y);
        canvas->scale(scale, scale);
    }

    // 绘制子元素
    for (auto& child : GetChildren()) {
        child->Paint(canvas);
    }

    canvas->restore();
}

void RenderSVGRoot::Layout(float parent_width, float parent_height) {
    auto element = svg_svg_element_.lock();
    if (!element) {
        RenderObject::Layout(parent_width, parent_height);
        return;
    }

    auto& layout = GetLayoutInfo();

    // 解析width和height属性
    std::string width_str = element->GetWidth();
    std::string height_str = element->GetHeight();

    // 解析宽度
    if (!width_str.empty()) {
        try {
            layout.width = std::stof(width_str);
        } catch (...) {
            layout.width = 0;
        }
    }

    // 解析高度
    if (!height_str.empty()) {
        try {
            layout.height = std::stof(height_str);
        } catch (...) {
            layout.height = 0;
        }
    }

    // 如果没有指定宽高，使用viewBox
    float view_min_x, view_min_y, view_width, view_height;
    if (element->ParseViewBox(view_min_x, view_min_y, view_width, view_height)) {
        if (layout.width == 0) layout.width = view_width;
        if (layout.height == 0) layout.height = view_height;
    }

    // 如果仍然没有宽高，使用默认值
    if (layout.width == 0) layout.width = 300;  // SVG默认宽度
    if (layout.height == 0) layout.height = 150; // SVG默认高度

    // 对子元素进行布局
    for (auto& child : GetChildren()) {
        child->Layout(layout.width, layout.height);
    }
}

// ========== RenderSVGPath 实现 ==========

RenderSVGPath::RenderSVGPath() : RenderSVG() {}

void RenderSVGPath::SetSVGPathElement(std::shared_ptr<SVGPathElement> element) {
    path_element_ = element;
    SetSVGElement(element);
    path_dirty_ = true;
}

void RenderSVGPath::UpdatePath() {
    auto element = path_element_.lock();
    if (!element) return;

    path_ = SVGPathParser::Parse(element->GetD());
    path_dirty_ = false;
}

void RenderSVGPath::Paint(SkCanvas* canvas) {
    auto element = path_element_.lock();
    if (!element) return;

    if (path_dirty_) {
        UpdatePath();
    }

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        SkMatrix matrix = SVGTransformParser::Parse(transform);
        canvas->concat(matrix);
    }

    // 填充
    std::string fill = element->GetFill();
    if (fill != "none") {
        canvas->drawPath(path_, CreateFillPaint());
    }

    // 描边
    std::string stroke = element->GetStroke();
    if (!stroke.empty() && stroke != "none") {
        canvas->drawPath(path_, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGCircle 实现 ==========

RenderSVGCircle::RenderSVGCircle() : RenderSVG() {}

void RenderSVGCircle::SetSVGCircleElement(std::shared_ptr<SVGCircleElement> element) {
    circle_element_ = element;
    SetSVGElement(element);
}

void RenderSVGCircle::Paint(SkCanvas* canvas) {
    auto element = circle_element_.lock();
    if (!element) {
        // std::cerr << "[SVG] RenderSVGCircle::Paint - element is null!" << std::endl;
        return;
    }

    float cx = element->GetCx();
    float cy = element->GetCy();
    float r = element->GetR();

    if (r <= 0) return;

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 填充
    if (element->GetFill() != "none") {
        canvas->drawCircle(cx, cy, r, CreateFillPaint());
    }

    // 描边
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawCircle(cx, cy, r, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGRect 实现 ==========

RenderSVGRect::RenderSVGRect() : RenderSVG() {}

void RenderSVGRect::SetSVGRectElement(std::shared_ptr<SVGRectElement> element) {
    rect_element_ = element;
    SetSVGElement(element);
}

void RenderSVGRect::Paint(SkCanvas* canvas) {
    auto element = rect_element_.lock();
    if (!element) return;

    float x = element->GetX();
    float y = element->GetY();
    float width = element->GetWidth();
    float height = element->GetHeight();
    float rx = element->GetRx();
    float ry = element->GetRy();

    if (width <= 0 || height <= 0) return;

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    SkRect rect = SkRect::MakeXYWH(x, y, width, height);

    // 填充
    if (element->GetFill() != "none") {
        if (rx > 0 || ry > 0) {
            canvas->drawRoundRect(rect, rx, ry > 0 ? ry : rx, CreateFillPaint());
        } else {
            canvas->drawRect(rect, CreateFillPaint());
        }
    }

    // 描边
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        if (rx > 0 || ry > 0) {
            canvas->drawRoundRect(rect, rx, ry > 0 ? ry : rx, CreateStrokePaint());
        } else {
            canvas->drawRect(rect, CreateStrokePaint());
        }
    }

    canvas->restore();
}

// ========== RenderSVGEllipse 实现 ==========

RenderSVGEllipse::RenderSVGEllipse() : RenderSVG() {}

void RenderSVGEllipse::SetSVGEllipseElement(std::shared_ptr<SVGEllipseElement> element) {
    ellipse_element_ = element;
    SetSVGElement(element);
}

void RenderSVGEllipse::Paint(SkCanvas* canvas) {
    auto element = ellipse_element_.lock();
    if (!element) return;

    float cx = element->GetCx();
    float cy = element->GetCy();
    float rx = element->GetRx();
    float ry = element->GetRy();

    if (rx <= 0 || ry <= 0) return;

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    SkRect oval = SkRect::MakeXYWH(cx - rx, cy - ry, rx * 2, ry * 2);

    // 填充
    if (element->GetFill() != "none") {
        canvas->drawOval(oval, CreateFillPaint());
    }

    // 描边
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawOval(oval, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGLine 实现 ==========

RenderSVGLine::RenderSVGLine() : RenderSVG() {}

void RenderSVGLine::SetSVGLineElement(std::shared_ptr<SVGLineElement> element) {
    line_element_ = element;
    SetSVGElement(element);
}

void RenderSVGLine::Paint(SkCanvas* canvas) {
    auto element = line_element_.lock();
    if (!element) return;

    float x1 = element->GetX1();
    float y1 = element->GetY1();
    float x2 = element->GetX2();
    float y2 = element->GetY2();

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 线条只有描边，没有填充
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawLine(x1, y1, x2, y2, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGPolyline 实现 ==========

RenderSVGPolyline::RenderSVGPolyline() : RenderSVG() {}

void RenderSVGPolyline::SetSVGPolylineElement(std::shared_ptr<SVGPolylineElement> element) {
    polyline_element_ = element;
    SetSVGElement(element);
}

void RenderSVGPolyline::Paint(SkCanvas* canvas) {
    auto element = polyline_element_.lock();
    if (!element) return;

    std::string points = element->GetPoints();
    if (points.empty()) return;

    path_ = SVGPointsParser::ToPolylinePath(points);

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 填充
    if (element->GetFill() != "none") {
        canvas->drawPath(path_, CreateFillPaint());
    }

    // 描边
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawPath(path_, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGPolygon 实现 ==========

RenderSVGPolygon::RenderSVGPolygon() : RenderSVG() {}

void RenderSVGPolygon::SetSVGPolygonElement(std::shared_ptr<SVGPolygonElement> element) {
    polygon_element_ = element;
    SetSVGElement(element);
}

void RenderSVGPolygon::Paint(SkCanvas* canvas) {
    auto element = polygon_element_.lock();
    if (!element) return;

    std::string points = element->GetPoints();
    if (points.empty()) return;

    path_ = SVGPointsParser::ToPolygonPath(points);

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 填充
    if (element->GetFill() != "none") {
        canvas->drawPath(path_, CreateFillPaint());
    }

    // 描边
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawPath(path_, CreateStrokePaint());
    }

    canvas->restore();
}

// ========== RenderSVGGroup 实现 ==========

RenderSVGGroup::RenderSVGGroup() : RenderSVG() {}

void RenderSVGGroup::SetSVGGElement(std::shared_ptr<SVGGElement> element) {
    g_element_ = element;
    SetSVGElement(element);
}

void RenderSVGGroup::Paint(SkCanvas* canvas) {
    auto element = g_element_.lock();
    if (!element) return;

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 绘制子元素
    for (auto& child : GetChildren()) {
        child->Paint(canvas);
    }

    canvas->restore();
}

// ========== RenderSVGText 实现 ==========

RenderSVGText::RenderSVGText() : RenderSVG() {}

void RenderSVGText::SetSVGTextElement(std::shared_ptr<SVGTextElement> element) {
    text_element_ = element;
    SetSVGElement(element);
}

void RenderSVGText::Paint(SkCanvas* canvas) {
    auto element = text_element_.lock();
    if (!element) return;

    float x = element->GetX();
    float y = element->GetY();
    std::string text = element->GetTextContent();

    if (text.empty()) {
        return;
    }

    canvas->save();

    // 应用transform
    std::string transform = element->GetTransform();
    if (!transform.empty()) {
        canvas->concat(SVGTransformParser::Parse(transform));
    }

    // 获取字体大小
    float font_size = element->GetFontSize();

    // 使用 FontManager 获取正确配置的字体
    SkFont font = FontManager::GetInstance().GetDefaultFont(font_size);

    // 处理 text-anchor 属性
    std::string text_anchor = element->GetTextAnchor();
    float text_width = font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);

    float draw_x = x;
    if (text_anchor == "middle") {
        draw_x = x - text_width / 2.0f;
    } else if (text_anchor == "end") {
        draw_x = x - text_width;
    }
    // "start" 或默认情况下 draw_x = x

    // 填充文本
    if (element->GetFill() != "none") {
        canvas->drawString(text.c_str(), draw_x, y, font, CreateFillPaint());
    }

    // 描边文本
    if (!element->GetStroke().empty() && element->GetStroke() != "none") {
        canvas->drawString(text.c_str(), draw_x, y, font, CreateStrokePaint());
    }

    canvas->restore();
}

} // namespace lightui

