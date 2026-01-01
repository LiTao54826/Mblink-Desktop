/**
 * @file hit_testing.cpp
 * @brief Hit Testing 实现
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

    // 简化实现：直接使用 body 元素
    // TODO: 未来需要使用渲染树进行精确的 Hit Testing
    auto body = document->GetBody();
    if (!body) {
        return result;
    }

    // 简化的 DOM 树遍历
    HitTestElement(body, x, y, 0.0f, 0.0f, result);

    return result;
}

HitTestResult HitTesting::HitTestWithLayers(std::shared_ptr<Document> document, float x, float y) {
    HitTestResult result;

    if (!document) {
        return result;
    }

    // 使用 PaintLayer 进行 hit testing
    auto body = document->GetBody();
    if (!body) {
        return result;
    }

    auto render_object = body->GetRenderObject();
    if (render_object) {
        // 如果有 PaintLayer，使用 PaintLayer 的 HitTest
        PaintLayer* paint_layer = render_object->GetPaintLayer();
        if (paint_layer) {
            if (paint_layer->HitTest(x, y, result)) {
                return result;
            }
        }
        
        // 回退到传统的 HitTest
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

    // 简化实现：假设每个元素占据整个宽度，高度为 50px
    // TODO: 从渲染树获取实际的布局信息
    float elem_x = offset_x;
    float elem_y = offset_y;
    float elem_width = 800;  // 默认宽度
    float elem_height = 50;  // 默认高度

    // 检查点是否在当前元素的边界内
    bool in_bounds = (x >= elem_x && x < elem_x + elem_width &&
                      y >= elem_y && y < elem_y + elem_height);

    if (!in_bounds) {
        return false;
    }

    // 从后向前遍历子元素（后面的元素在上层）
    const auto& children = element->GetChildNodes();
    float child_offset_y = elem_y;

    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto child_element = std::dynamic_pointer_cast<Element>(*it);
        if (child_element) {
            if (HitTestElement(child_element, x, y, elem_x, child_offset_y, result)) {
                return true;  // 找到了，停止搜索
            }
            child_offset_y += elem_height;  // 简化：垂直堆叠
        }
    }

    // 如果没有子元素命中，当前元素就是目标
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
    
    // 对于 position: fixed 元素，layout.x/y 已经是视口绝对坐标
    float abs_x = is_fixed ? layout.x : (offset_x + layout.x);
    float abs_y = is_fixed ? layout.y : (offset_y + layout.y);
    
    // 检查点是否在元素边界内
    bool in_bounds = (x >= abs_x && x < abs_x + layout.width &&
                      y >= abs_y && y < abs_y + layout.height);
    
    return in_bounds;
}

bool HitTesting::HitTestRecursive(
    std::shared_ptr<RenderObject> render_object,
    float x, float y,
    float offset_x,
    float offset_y,
    HitTestResult& result) {

    if (!render_object) {
        return false;
    }

    const auto& layout = render_object->GetLayoutInfo();
    
    if (!layout.is_laid_out) {
        return false;
    }

    // 检查当前元素的 position 属性
    const auto& style = render_object->GetComputedStyle();
    bool is_fixed = (style.position == "fixed");
    
    // 计算当前元素的绝对位置
    // 对于 position: fixed 元素，其 layout.x/y 已经是相对于视口的绝对坐标
    // 对于普通元素，需要加上父元素的偏移量
    float current_offset_x, current_offset_y;
    if (is_fixed) {
        // fixed 元素：layout.x/y 是视口绝对坐标，不需要加 offset
        current_offset_x = layout.x;
        current_offset_y = layout.y;
    } else {
        // 普通元素：layout.x/y 是相对于父元素的，需要加上父元素的偏移
        current_offset_x = offset_x + layout.x;
        current_offset_y = offset_y + layout.y;
    }

    // 检查当前元素是否有 overflow 属性
    bool has_overflow = (style.overflow == "auto" || style.overflow == "scroll" || 
                         style.overflow == "hidden" ||
                         style.overflow_y == "auto" || style.overflow_y == "scroll" ||
                         style.overflow_y == "hidden");
    
    // 检查 pointer-events 属性
    bool pointer_events_none = (style.pointer_events == "none");
    
    // 获取当前元素的滚动偏移量
    float scroll_x = render_object->GetScrollX();
    float scroll_y = render_object->GetScrollY();
    
    // 用于检查子元素的鼠标坐标
    float child_test_x = x;
    float child_test_y = y;
    
    // 获取 DOM 节点和元素
    auto node = render_object->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    
    // 检查是否是 body 元素（根滚动容器）
    bool is_body = element && (element->GetTagName() == "body" || element->GetTagName() == "BODY");
    
    // 边界检查
    bool in_bounds = (x >= current_offset_x && x < current_offset_x + layout.width &&
                      y >= current_offset_y && y < current_offset_y + layout.height);
    
    if (is_fixed) {
        // fixed 元素：直接检查边界
        if (!in_bounds) {
            return false;
        }
    } else if (has_overflow && is_body) {
        // body 元素：不检查边界，转换坐标用于滚动
        child_test_x = x + scroll_x;
        child_test_y = y + scroll_y;
    } else if (has_overflow) {
        // 有 overflow 的容器：检查边界，转换坐标
        if (!in_bounds) {
            return false;
        }
        child_test_x = x + scroll_x;
        child_test_y = y + scroll_y;
    } else {
        // 普通元素：检查边界
        if (!in_bounds) {
            return false;
        }
    }

    // 参考 Blink 的做法：按 z-index 排序子元素，高 z-index 优先测试
    // 将子元素分为三组：positive z-index, normal flow, negative z-index
    const auto& children = render_object->GetChildren();
    
    // 收集并按 z-index 排序子元素
    std::vector<std::shared_ptr<RenderObject>> sorted_children(children.begin(), children.end());
    std::stable_sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            const auto& style_a = a->GetComputedStyle();
            const auto& style_b = b->GetComputedStyle();
            // 高 z-index 排在前面（优先测试）
            return style_a.z_index > style_b.z_index;
        });
    
    // 从高 z-index 到低 z-index 遍历子元素
    for (const auto& child : sorted_children) {
        // 子元素的偏移量 = 当前元素的绝对位置
        // 这样子元素的绝对位置 = child_offset + child.layout.x/y
        if (HitTestRecursive(child, child_test_x, child_test_y, current_offset_x, current_offset_y, result)) {
            return true;
        }
    }

    // 如果 pointer-events: none，不将当前元素作为命中目标
    if (pointer_events_none) {
        return false;
    }

    // 没有子元素命中，当前元素就是目标
    if (element) {
        result.element = element;
        result.render_object = render_object;
        result.local_x = x - current_offset_x;
        result.local_y = y - current_offset_y;
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
            float parent_offset_x = current_offset_x - layout.x;
            float parent_offset_y = current_offset_y - layout.y;
            result.local_x = x - parent_offset_x;
            result.local_y = y - parent_offset_y;
            return true;
        }
        parent_ro = parent_ro->GetParent();
    }

    return false;
}

} // namespace lightui

