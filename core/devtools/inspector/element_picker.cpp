/**
 * @file element_picker.cpp
 * @brief 元素拾取工具实现
 */

#include "element_picker.h"
#include "element_highlighter.h"
#include "core/dom/event.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

namespace lightui {

ElementPicker::ElementPicker(Document* document)
    : document_(document) {
}

ElementPicker::~ElementPicker() = default;

void ElementPicker::Start() {
    active_ = true;
    picked_element_.reset();
    hovered_element_.reset();
}

void ElementPicker::Stop() {
    active_ = false;
    hovered_element_.reset();
}

bool ElementPicker::HandleEvent(const Event& event) {
    if (!active_) return false;

    // TODO: 根据事件类型处理
    // 这里需要根据实际的 Event 类实现

    return false;
}

void ElementPicker::HandleMouseMove(int x, int y) {
    if (!active_) return;

    last_mouse_x_ = x;
    last_mouse_y_ = y;

    // 命中测试
    hovered_element_ = HitTest(x, y);
}

bool ElementPicker::HandleMouseClick(int x, int y) {
    if (!active_) return false;

    // 命中测试
    auto element = HitTest(x, y);
    if (element) {
        picked_element_ = element;
        return true;
    }

    return false;
}

void ElementPicker::RenderHoverHighlight(SkCanvas* canvas) {
    if (!active_ || !hovered_element_) return;

    // 获取元素的布局信息
    // TODO: 从 RenderObject 获取实际的布局盒
    float x = 0, y = 0, width = 100, height = 50;

    // 半透明蓝色覆盖
    SkPaint fill_paint;
    fill_paint.setColor(SkColorSetARGB(64, 66, 133, 244));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), fill_paint);

    // 蓝色边框
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(66, 133, 244));
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(2);
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), border_paint);

    // 信息提示框
    std::string tag = hovered_element_->GetTagName();
    std::string id = hovered_element_->GetAttribute("id");
    std::string cls = hovered_element_->GetAttribute("class");

    std::string text = tag;
    if (!id.empty()) {
        text += "#" + id;
    }
    if (!cls.empty()) {
        size_t space_pos = cls.find(' ');
        if (space_pos != std::string::npos) {
            text += "." + cls.substr(0, space_pos);
        } else {
            text += "." + cls;
        }
    }

    // 添加尺寸
    text += " | " + std::to_string(static_cast<int>(width)) + " × " + std::to_string(static_cast<int>(height));

    // 提示框
    SkFont font;
    font.setSize(11);

    float text_width = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
    float tooltip_width = text_width + 12;
    float tooltip_height = 20;
    float tooltip_x = x;
    float tooltip_y = y - tooltip_height - 4;

    // 确保提示框在屏幕内
    if (tooltip_y < 0) {
        tooltip_y = y + height + 4;
    }

    SkPaint bg_paint;
    bg_paint.setColor(SkColorSetRGB(66, 133, 244));
    canvas->drawRect(SkRect::MakeXYWH(tooltip_x, tooltip_y, tooltip_width, tooltip_height), bg_paint);

    SkPaint text_paint;
    text_paint.setColor(SK_ColorWHITE);
    text_paint.setAntiAlias(true);
    canvas->drawString(text.c_str(), tooltip_x + 6, tooltip_y + 14, font, text_paint);
}

std::shared_ptr<Element> ElementPicker::HitTest(int x, int y) {
    if (!document_) return nullptr;

    // TODO: 实现真正的命中测试
    // 需要遍历渲染树，找到包含指定坐标的最深层元素

    // 暂时返回 body
    return document_->GetBody();
}

} // namespace lightui
