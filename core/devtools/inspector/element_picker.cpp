/**
 * @file element_picker.cpp
 * @brief 元素拾取工具实现
 */

#include "element_picker.h"
#include "element_highlighter.h"
#include "core/dom/event.h"
#include "core/window/window_manager.h"
#include "core/render/render_object.h"

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
    hovered_render_object_.reset();
}

void ElementPicker::SetHoverElement(std::shared_ptr<Element> element, std::shared_ptr<RenderObject> render_obj) {
    hovered_element_ = element;
    hovered_render_object_ = render_obj;
    std::cout << "[ElementPicker::SetHoverElement] element=" << (element ? element->GetTagName() : "null")
              << " render_obj=" << render_obj.get() << std::endl;
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

    // 始终从element获取最新的RenderObject，因为render tree可能已重建
    auto render_obj = hovered_element_->GetRenderObject();
    
    if (!render_obj) {
        std::cout << "[ElementPicker] No render_obj for element " << hovered_element_->GetTagName() << std::endl;
        return;
    }
    
    // 检查render_obj是否有效（parent chain是否完整）
    const auto& layout = render_obj->GetLayoutInfo();
    
    // 追踪parent chain
    int parent_count = 0;
    auto parent = render_obj->GetParent();
    while (parent) {
        parent_count++;
        parent = parent->GetParent();
    }
    
    std::cout << "[ElementPicker] element=" << hovered_element_->GetTagName()
              << " class=" << hovered_element_->GetAttribute("class")
              << " render_obj=" << render_obj.get()
              << " parent_chain_depth=" << parent_count
              << " is_laid_out=" << layout.is_laid_out
              << " layout=(" << layout.x << "," << layout.y << "," << layout.width << "," << layout.height << ")"
              << " self_scroll=(" << render_obj->GetScrollX() << "," << render_obj->GetScrollY() << ")"
              << " has_parent=" << (render_obj->GetParent() != nullptr)
              << std::endl;

    // 获取元素的视口坐标边界矩形（已经考虑了滚动偏移）
    SkRect bounds = render_obj->GetViewportBoundingRect();
    if (bounds.isEmpty()) {
        std::cout << "[ElementPicker] Empty bounds for element " << hovered_element_->GetTagName() << std::endl;
        return;
    }

    float x = bounds.x();
    float y = bounds.y();
    float width = bounds.width();
    float height = bounds.height();
    
    std::cout << "[ElementPicker] Absolute bounds: (" << x << "," << y << ") " << width << "x" << height << std::endl;

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

    // 获取渲染树
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return nullptr;
    
    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return nullptr;
    
    // 递归查找命中的元素
    std::shared_ptr<Element> hit_element;
    std::shared_ptr<RenderObject> hit_render_obj;
    
    // traverse 函数参数：
    // - obj: 当前检查的渲染对象
    // - offset_x/y: 父元素在文档中的绝对位置
    // - test_x/y: 用于命中测试的坐标（可能是可视坐标或文档坐标）
    std::function<void(std::shared_ptr<RenderObject>, float, float, float, float)> traverse;
    traverse = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y, float test_x, float test_y) {
        if (!obj) return;
        
        const auto& layout = obj->GetLayoutInfo();
        if (!layout.is_laid_out) return;
        
        // 计算当前元素在文档中的绝对位置
        float abs_x = offset_x + layout.x;
        float abs_y = offset_y + layout.y;
        
        // 检查当前元素是否有 overflow 属性
        const auto& style = obj->GetComputedStyle();
        bool has_overflow = (style.overflow == "auto" || style.overflow == "scroll" || 
                             style.overflow == "hidden" ||
                             style.overflow_y == "auto" || style.overflow_y == "scroll" ||
                             style.overflow_y == "hidden");
        
        // 获取滚动偏移
        float scroll_x = obj->GetScrollX();
        float scroll_y = obj->GetScrollY();
        
        // 用于检查子元素的坐标
        float child_test_x = test_x;
        float child_test_y = test_y;
        
        // 检查是否是 body 元素
        auto node = obj->GetNode();
        auto elem = node ? std::dynamic_pointer_cast<Element>(node) : nullptr;
        bool is_body = elem && (elem->GetTagName() == "body" || elem->GetTagName() == "BODY");
        
        if (has_overflow && is_body) {
            // 对于 body 元素，它的可见区域是整个视口，不是 CSS 设置的高度
            // 所以不需要检查边界，直接将鼠标坐标转换为文档坐标
            child_test_x = test_x + scroll_x;
            child_test_y = test_y + scroll_y;
        } else if (has_overflow) {
            // 对于其他有 overflow 的容器，检查点是否在可见区域内
            SkRect visible_bounds = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);
            if (!visible_bounds.contains(test_x, test_y)) {
                return;  // 点不在可见区域内，跳过此元素及其子元素
            }
            
            // 将可视坐标转换为文档坐标
            child_test_x = test_x + scroll_x;
            child_test_y = test_y + scroll_y;
        } else {
            // 对于普通元素，检查点是否在边界内
            SkRect bounds = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);
            if (!bounds.contains(test_x, test_y)) {
                return;  // 点不在当前元素内
            }
        }
        
        // 先递归检查子元素（优先命中深层元素）
        for (const auto& child : obj->GetChildren()) {
            traverse(child, abs_x, abs_y, child_test_x, child_test_y);
            if (hit_element) return;  // 已经找到，提前返回
        }
        
        // 如果子元素没有命中，当前元素就是目标（复用前面已获取的 node）
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            hit_element = std::static_pointer_cast<Element>(node);
            hit_render_obj = obj;
        }
    };
    
    traverse(root_render, 0, 0, static_cast<float>(x), static_cast<float>(y));
    
    // 更新 hovered_render_object_ 以便高亮显示
    if (hit_element) {
        hovered_render_object_ = hit_render_obj;
    }
    
    return hit_element ? hit_element : document_->GetBody();  // 如果没找到，返回body
}

} // namespace lightui
