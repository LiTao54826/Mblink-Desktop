/**
 * @file element_picker.cpp
 * @brief 元素拾取工具实现
 */

#include "element_picker.h"
#include "element_highlighter.h"
#include "core/dom/event.h"
#include "core/window/window_manager.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

#include <iostream>

namespace mbink {

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
    hovered_render_object_.reset();
}

void ElementPicker::SetHoverElement(std::shared_ptr<Element> element, std::shared_ptr<RenderObject> render_obj) {
    hovered_element_ = element;
    hovered_render_object_ = render_obj;
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

    // 优先使用 HitTest 命中的 RenderObject，避免 element->GetRenderObject() 在层提升/重建后指向非可见对象
    auto render_obj = hovered_render_object_;
    if (!render_obj) {
        // 回退路径：从 element 获取最新 RenderObject
        render_obj = hovered_element_->GetRenderObject();
    }

    if (!render_obj) {
        return;
    }

    const auto& layout = render_obj->GetLayoutInfo();
    if (!layout.is_laid_out) {
        return;
    }

    // 使用视口坐标缓存优先，确保 fixed 元素在滚动后仍按视口坐标高亮
    if (!render_obj->GetViewportBounds().valid) {
        render_obj->UpdateViewportBounds();
    }

    SkRect bounds;
    const auto& viewport_bounds = render_obj->GetViewportBounds();
    if (viewport_bounds.valid) {
        bounds = viewport_bounds.has_transform
            ? viewport_bounds.transformed_bounds
            : SkRect::MakeXYWH(viewport_bounds.x, viewport_bounds.y,
                               viewport_bounds.width, viewport_bounds.height);
    } else {
        // 回退路径
        bounds = render_obj->GetViewportBoundingRect();
    }

    if (bounds.isEmpty()) {
        return;
    }

    float x = bounds.x();
    float y = bounds.y();
    float width = bounds.width();
    float height = bounds.height();

    // 半透明蓝色覆盖
    SkPaint fill_paint;
    fill_paint.setColor(SkColorSetARGB(64, 66, 133, 244));
    canvas->drawRect(bounds, fill_paint);

    // 蓝色边框
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(66, 133, 244));
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(2);
    canvas->drawRect(bounds, border_paint);

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

    // 提示框（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 11.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

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

    // 获取渲染树
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return nullptr;

    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return nullptr;

    // 递归查找命中的元素（统一使用视口坐标，避免滚动后 fixed 命中偏移）
    std::shared_ptr<Element> hit_element;
    std::shared_ptr<RenderObject> hit_render_obj;

    const float test_x = static_cast<float>(x);
    const float test_y = static_cast<float>(y);

    std::function<void(std::shared_ptr<RenderObject>)> traverse;
    traverse = [&](std::shared_ptr<RenderObject> obj) {
        if (!obj || hit_element) return;

        const auto& layout = obj->GetLayoutInfo();
        if (!layout.is_laid_out) return;

        // 优先使用视口缓存，必要时更新
        if (!obj->GetViewportBounds().valid) {
            obj->UpdateViewportBounds();
        }

        const auto& viewport_bounds = obj->GetViewportBounds();
        if (!viewport_bounds.valid) return;

        // 使用统一的视口命中，避免 body/overflow 坐标换算把 fixed 元素命中搞偏
        if (!obj->ContainsViewportPoint(test_x, test_y)) {
            return;
        }

        // 先递归检查子元素（优先命中深层元素）
        for (const auto& child : obj->GetChildren()) {
            traverse(child);
            if (hit_element) return;
        }

        // 如果子元素没有命中，当前元素就是目标
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            hit_element = std::static_pointer_cast<Element>(node);
            hit_render_obj = obj;
        }
    };

    traverse(root_render);

    // 更新 hovered_render_object_ 以便高亮显示
    if (hit_element) {
        hovered_render_object_ = hit_render_obj;
    }

    return hit_element ? hit_element : document_->GetBody();
}

} // namespace mbink
