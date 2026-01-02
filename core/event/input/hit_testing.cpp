/**
 * @file hit_testing.cpp
 * @brief Hit Testing 实现
 * 
 * 核心设计：
 * - x, y 始终是原始视口坐标，不修改
 * - offset_x, offset_y 是元素的累积布局偏移
 * - scroll_offset_x, scroll_offset_y 是累积滚动偏移（用于非 fixed 元素）
 * - fixed 元素忽略滚动偏移，直接使用视口坐标
 */

#include "hit_testing.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/render/objects/render_object.h"
#include "core/render/layer/paint_layer.h"
#include <algorithm>
#include <iostream>
#include <vector>

namespace lightui {

HitTestResult HitTesting::HitTest(std::shared_ptr<Document> document, float x, float y) {
    HitTestResult result;

    if (!document) {
        return result;
    }

    auto body = document->GetBody();
    if (!body) {
        return result;
    }

    HitTestElement(body, x, y, 0.0f, 0.0f, result);

    return result;
}

HitTestResult HitTesting::HitTestWithLayers(std::shared_ptr<Document> document, float x, float y) {
    HitTestResult result;

    if (!document) {
        return result;
    }

    auto body = document->GetBody();
    if (!body) {
        return result;
    }

    auto render_object = body->GetRenderObject();
    if (render_object) {
        PaintLayer* paint_layer = render_object->GetPaintLayer();
        if (paint_layer) {
            if (paint_layer->HitTest(x, y, result)) {
                return result;
            }
        }
        
        HitTestRecursive(render_object, x, y, 0.0f, 0.0f, result);
    }

    return result;
}

bool HitTesting::HitTestElement(
    std::shared_ptr<Element> element,
    float x, float y,
    float offset_x,
    float offset_y,
    HitTestResult& result) {

    if (!element) {
        return false;
    }

    float elem_x = offset_x;
    float elem_y = offset_y;
    float elem_width = 800;
    float elem_height = 50;

    bool in_bounds = (x >= elem_x && x < elem_x + elem_width &&
                      y >= elem_y && y < elem_y + elem_height);

    if (!in_bounds) {
        return false;
    }

    const auto& children = element->GetChildNodes();
    float child_offset_y = elem_y;

    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto child_element = std::dynamic_pointer_cast<Element>(*it);
        if (child_element) {
            if (HitTestElement(child_element, x, y, elem_x, child_offset_y, result)) {
                return true;
            }
            child_offset_y += elem_height;
        }
    }

    result.element = element;
    result.local_x = x - elem_x;
    result.local_y = y - elem_y;
    return true;
}

HitTestResult HitTesting::HitTestRenderObject(
    std::shared_ptr<RenderObject> render_object,
    float x, float y,
    float offset_x,
    float offset_y) {
    
    HitTestResult result;
    HitTestRecursive(render_object, x, y, offset_x, offset_y, result);
    return result;
}

bool HitTesting::IsPointInBounds(
    std::shared_ptr<RenderObject> render_object,
    float x, float y,
    float offset_x,
    float offset_y,
    bool is_fixed) {
    
    if (!render_object) {
        return false;
    }
    
    const auto& layout = render_object->GetLayoutInfo();
    if (!layout.is_laid_out) {
        return false;
    }
    
    float abs_x = is_fixed ? layout.x : (offset_x + layout.x);
    float abs_y = is_fixed ? layout.y : (offset_y + layout.y);
    
    bool in_bounds = (x >= abs_x && x < abs_x + layout.width &&
                      y >= abs_y && y < abs_y + layout.height);
    
    return in_bounds;
}

/**
 * 内部递归函数，带累积滚动偏移
 */
static bool HitTestRecursiveInternal(
    std::shared_ptr<RenderObject> render_object,
    float viewport_x, float viewport_y,  // 原始视口坐标，不修改
    float offset_x, float offset_y,      // 累积布局偏移
    float scroll_offset_x, float scroll_offset_y,  // 累积滚动偏移
    HitTestResult& result) {

    if (!render_object) {
        return false;
    }

    const auto& layout = render_object->GetLayoutInfo();
    
    if (!layout.is_laid_out) {
        return false;
    }

    const auto& style = render_object->GetComputedStyle();
    bool is_fixed = (style.position == "fixed");
    
    // 计算当前元素的绝对位置
    float current_abs_x, current_abs_y;
    if (is_fixed) {
        // fixed 元素：layout.x/y 是视口绝对坐标，不受滚动影响
        current_abs_x = layout.x;
        current_abs_y = layout.y;
    } else {
        // 普通元素：layout 是相对于父元素的，需要加上布局偏移和滚动偏移
        current_abs_x = offset_x + layout.x - scroll_offset_x;
        current_abs_y = offset_y + layout.y - scroll_offset_y;
    }

    // 边界检查（使用视口坐标）
    bool in_bounds = (viewport_x >= current_abs_x && viewport_x < current_abs_x + layout.width &&
                      viewport_y >= current_abs_y && viewport_y < current_abs_y + layout.height);
    
    // 获取 DOM 节点和元素
    auto node = render_object->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    bool is_body = element && (element->GetTagName() == "body" || element->GetTagName() == "BODY");
    
    // 检查当前元素是否有 overflow 属性
    bool has_overflow = (style.overflow == "auto" || style.overflow == "scroll" || 
                         style.overflow == "hidden" ||
                         style.overflow_y == "auto" || style.overflow_y == "scroll" ||
                         style.overflow_y == "hidden");
    
    // 对于非 body 元素，需要检查边界
    if (!is_body && !in_bounds) {
        return false;
    }
    
    // 检查 pointer-events 属性
    bool pointer_events_none = (style.pointer_events == "none");
    
    // 计算子元素的累积滚动偏移
    float child_scroll_offset_x = scroll_offset_x;
    float child_scroll_offset_y = scroll_offset_y;
    if (has_overflow) {
        child_scroll_offset_x += render_object->GetScrollX();
        child_scroll_offset_y += render_object->GetScrollY();
    }
    
    // 计算子元素的布局偏移
    float child_offset_x = offset_x + layout.x;
    float child_offset_y = offset_y + layout.y;

    // 按 z-index 排序子元素
    const auto& children = render_object->GetChildren();
    std::vector<std::shared_ptr<RenderObject>> sorted_children(children.begin(), children.end());
    std::stable_sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            return a->GetComputedStyle().z_index > b->GetComputedStyle().z_index;
        });
    
    // 先测试 fixed 元素（使用原始视口坐标，滚动偏移为 0）
    for (const auto& child : sorted_children) {
        const auto& child_style = child->GetComputedStyle();
        if (child_style.position == "fixed") {
            if (HitTestRecursiveInternal(child, viewport_x, viewport_y, 0, 0, 0, 0, result)) {
                return true;
            }
        }
    }
    
    // 再测试非 fixed 元素（使用累积滚动偏移）
    for (const auto& child : sorted_children) {
        const auto& child_style = child->GetComputedStyle();
        if (child_style.position != "fixed") {
            if (HitTestRecursiveInternal(child, viewport_x, viewport_y, 
                                         child_offset_x, child_offset_y,
                                         child_scroll_offset_x, child_scroll_offset_y, result)) {
                return true;
            }
        }
    }

    // 如果 pointer-events: none，不将当前元素作为命中目标
    if (pointer_events_none) {
        return false;
    }

    // 边界检查（对于 body 元素在这里检查）
    if (is_body && !in_bounds) {
        return false;
    }

    // 没有子元素命中，当前元素就是目标
    if (element) {
        result.element = element;
        result.render_object = render_object;
        result.local_x = viewport_x - current_abs_x;
        result.local_y = viewport_y - current_abs_y;
        return true;
    }

    // 如果是 Text 节点，向上查找最近的 Element 祖先
    auto parent_ro = render_object->GetParent();
    while (parent_ro) {
        auto parent_node = parent_ro->GetNode();
        auto parent_element = std::dynamic_pointer_cast<Element>(parent_node);
        if (parent_element) {
            const auto& parent_style = parent_ro->GetComputedStyle();
            if (parent_style.pointer_events == "none") {
                return false;
            }
            result.element = parent_element;
            result.render_object = parent_ro;
            result.local_x = viewport_x - current_abs_x;
            result.local_y = viewport_y - current_abs_y;
            return true;
        }
        parent_ro = parent_ro->GetParent();
    }

    return false;
}

bool HitTesting::HitTestRecursive(
    std::shared_ptr<RenderObject> render_object,
    float x, float y,
    float offset_x,
    float offset_y,
    HitTestResult& result) {
    
    // 调用内部函数，初始滚动偏移为 0
    return HitTestRecursiveInternal(render_object, x, y, offset_x, offset_y, 0, 0, result);
}

} // namespace lightui
