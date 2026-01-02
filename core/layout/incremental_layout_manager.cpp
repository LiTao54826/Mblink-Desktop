/**
 * @file incremental_layout_manager.cpp
 * @brief 增量布局管理器实现
 */

#include "incremental_layout_manager.h"
#include "layout_boundary_detector.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/render/objects/render_object.h"
#include "core/window/window.h"

#include <iostream>

namespace lightui {

IncrementalLayoutManager::IncrementalLayoutManager(Window* window)
    : window_(window) {
}

bool IncrementalLayoutManager::AddOutOfFlowElement(Element* element, Node* parent) {
    if (!element || !window_) {
        return false;
    }

    // 获取元素的 RenderObject
    auto render_object = element->GetRenderObject();
    if (!render_object) {
        // 如果还没有 RenderObject，需要创建
        // 这里我们只标记需要重绘，让渲染管线处理创建
        // 因为创建 RenderObject 需要完整的样式解析
        return false;
    }

    // 找到父元素的 RenderObject
    RenderObject* parent_ro = nullptr;
    if (parent) {
        if (parent->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto parent_elem = static_cast<Element*>(parent);
            auto parent_render = parent_elem->GetRenderObject();
            if (parent_render) {
                parent_ro = parent_render.get();
            }
        }
    }

    // 如果找不到父 RenderObject，回退到根
    if (!parent_ro && window_->GetCachedRenderTree()) {
        parent_ro = window_->GetCachedRenderTree().get();
    }

    if (!parent_ro) {
        return false;
    }

    // 将 RenderObject 添加到父节点
    // 注意：这里不触发 InvalidateRenderTree
    parent_ro->AppendChild(render_object);
    render_object->SetParent(parent_ro->shared_from_this());

    // 标记需要布局和重绘
    render_object->MarkNeedsLayout(false);  // 不向上传播
    render_object->MarkNeedsPaint();

    // 标记脏区域
    window_->SetNeedsRepaint();

    std::cout << "[IncrementalLayout] Added out-of-flow element without full rebuild" << std::endl;

    return true;
}

bool IncrementalLayoutManager::RemoveOutOfFlowElement(Element* element) {
    if (!element || !window_) {
        return false;
    }

    auto render_object = element->GetRenderObject();
    if (!render_object) {
        return false;
    }

    auto parent = render_object->GetParent();
    if (!parent) {
        return false;
    }

    // 从父节点移除
    parent->RemoveChild(render_object);

    // 标记需要重绘
    window_->SetNeedsRepaint();

    std::cout << "[IncrementalLayout] Removed out-of-flow element without full rebuild" << std::endl;

    return true;
}

void IncrementalLayoutManager::MarkBoundaryNeedsLayout(Element* boundary) {
    if (!boundary) {
        return;
    }

    auto render_object = boundary->GetRenderObject();
    if (!render_object) {
        return;
    }

    MarkBoundaryNeedsLayout(render_object.get());
    dirty_boundaries_.insert(boundary);
}

void IncrementalLayoutManager::MarkBoundaryNeedsLayout(RenderObject* boundary) {
    if (!boundary) {
        return;
    }

    // 标记边界及其子树需要重新布局
    // 使用 false 参数，不向上传播到父节点
    boundary->MarkNeedsLayout(false);

    // 递归标记所有子节点
    for (auto& child : boundary->GetChildren()) {
        child->MarkNeedsLayout(false);
    }

    // 标记需要重绘
    boundary->MarkNeedsPaint();

    dirty_render_boundaries_.insert(boundary);

    std::cout << "[IncrementalLayout] Marked boundary for layout (type: " 
              << boundary->GetLayoutBoundaryType() << ")" << std::endl;
}

void IncrementalLayoutManager::UpdateScrollContainerSize(Element* scroll_container) {
    if (!scroll_container) {
        return;
    }

    auto render_object = scroll_container->GetRenderObject();
    if (!render_object) {
        return;
    }

    // 重新计算内容尺寸
    float content_width = render_object->CalculateContentWidth();
    float content_height = render_object->CalculateContentHeight();

    render_object->SetContentSize(content_width, content_height);

    std::cout << "[IncrementalLayout] Updated scroll container size: " 
              << content_width << "x" << content_height << std::endl;
}

bool IncrementalLayoutManager::HasPendingUpdates() const {
    return !dirty_boundaries_.empty() || !dirty_render_boundaries_.empty();
}

void IncrementalLayoutManager::ClearPendingUpdates() {
    dirty_boundaries_.clear();
    dirty_render_boundaries_.clear();
}

}  // namespace lightui
