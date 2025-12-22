/**
 * @file hit_testing.cpp
 * @brief Hit Testing 实现
 */

#include "hit_testing.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/render/render_object.h"
#include "core/render/layer_manager.h"
#include <iostream>

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

    // 先在 LayerManager 的 Overlay/Modal 层中测试
    auto& layer_manager = LayerManager::Instance();
    if (layer_manager.HitTest(x, y, result)) {
        return result;
    }

    // 如果 Overlay/Modal 层未命中，在 Base 层（渲染树）中测试
    auto body = document->GetBody();
    if (!body) {
        return result;
    }

    auto render_object = body->GetRenderObject();
    if (render_object) {
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
    float offset_y) {
    
    if (!render_object) {
        return false;
    }
    
    const auto& layout = render_object->GetLayoutInfo();
    if (!layout.is_laid_out) {
        return false;
    }
    
    // 计算元素的绝对位置
    float abs_x = offset_x + layout.x;
    float abs_y = offset_y + layout.y;
    
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

    // 计算当前元素在文档中的绝对位置
    float current_offset_x = offset_x + layout.x;
    float current_offset_y = offset_y + layout.y;

    // 检查当前元素是否有 overflow 属性
    const auto& style = render_object->GetComputedStyle();
    bool has_overflow = (style.overflow == "auto" || style.overflow == "scroll" || 
                         style.overflow == "hidden" ||
                         style.overflow_y == "auto" || style.overflow_y == "scroll" ||
                         style.overflow_y == "hidden");
    
    // 检查 pointer-events 属性
    // 如果 pointer-events: none，跳过当前元素但仍检查子元素
    // （子元素可能有 pointer-events: auto 覆盖）
    bool pointer_events_none = (style.pointer_events == "none");
    
    // 获取当前元素的滚动偏移量
    float scroll_x = render_object->GetScrollX();
    float scroll_y = render_object->GetScrollY();
    
    // 用于检查子元素的鼠标坐标（可能需要转换为文档坐标）
    float child_test_x = x;
    float child_test_y = y;
    
    // 检查是否是 body 元素（根滚动容器）
    auto node = render_object->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    bool is_body = element && (element->GetTagName() == "body" || element->GetTagName() == "BODY");
    
    if (has_overflow && is_body) {
        // 对于 body 元素，它的可见区域是整个视口，不是 CSS 设置的高度
        // 所以不需要检查边界，直接将鼠标坐标转换为文档坐标
        // 
        // 例如：body CSS 高度 400px，但视口高度 800px，滚动了 100px
        // - 鼠标在视口 y=500（超出 body 的 CSS 高度）
        // - 转换为文档坐标：y=500+100=600
        // - 子元素布局位置 y=600，可以命中
        child_test_x = x + scroll_x;
        child_test_y = y + scroll_y;
    } else if (has_overflow) {
        // 对于其他有 overflow 的容器，检查点是否在可见区域内
        if (!IsPointInBounds(render_object, x, y, offset_x, offset_y)) {
            return false;  // 点不在可见区域内，跳过此元素及其子元素
        }
        
        // 将鼠标坐标转换为文档坐标
        child_test_x = x + scroll_x;
        child_test_y = y + scroll_y;
    } else {
        // 对于普通元素，检查点是否在边界内
        if (!IsPointInBounds(render_object, x, y, offset_x, offset_y)) {
            return false;
        }
    }

    // 从后向前遍历子元素（后面的元素在上层）
    const auto& children = render_object->GetChildren();
    
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child = *it;

        // 递归检查子元素，使用转换后的坐标
        if (HitTestRecursive(child, child_test_x, child_test_y, current_offset_x, current_offset_y, result)) {
            return true;
        }
    }

    // 如果 pointer-events: none，不将当前元素作为命中目标
    // 让事件穿透到下面的元素
    if (pointer_events_none) {
        return false;
    }

    // 没有子元素命中，当前元素就是目标（复用前面已获取的 node 和 element）
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
            // 检查父元素的 pointer-events 属性
            const auto& parent_style = parent_ro->GetComputedStyle();
            if (parent_style.pointer_events == "none") {
                return false;  // 父元素也是 pointer-events: none，不命中
            }
            result.element = parent_element;
            result.render_object = parent_ro;
            result.local_x = x - current_offset_x;
            result.local_y = y - current_offset_y;
            return true;
        }
        parent_ro = parent_ro->GetParent();
    }

    return false;
}

} // namespace lightui

