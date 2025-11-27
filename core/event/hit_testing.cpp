/**
 * @file hit_testing.cpp
 * @brief Hit Testing 实现
 */

#include "hit_testing.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/render/render_object.h"
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
    
    // 计算当前元素的绝对位置
    float current_offset_x = offset_x + layout.x;
    float current_offset_y = offset_y + layout.y;
    
    // 检查点是否在当前元素边界内
    if (!IsPointInBounds(render_object, x, y, offset_x, offset_y)) {
        return false;
    }
    
    // 从后向前遍历子元素（后面的元素在上层）
    const auto& children = render_object->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child = *it;
        
        // 递归检查子元素
        if (HitTestRecursive(child, x, y, current_offset_x, current_offset_y, result)) {
            // 子元素命中，返回 true
            return true;
        }
    }
    
    // 没有子元素命中，当前元素就是目标
    // 获取对应的 DOM 元素
    auto node = render_object->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);

    if (element) {
        result.element = element;
        result.render_object = render_object;  // 保存渲染对象引用
        result.local_x = x - current_offset_x;
        result.local_y = y - current_offset_y;
        return true;
    }

    return false;
}

} // namespace lightui

