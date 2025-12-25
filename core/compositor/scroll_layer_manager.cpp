/**
 * @file scroll_layer_manager.cpp
 * @brief 滚动层管理器实现
 */

#include "scroll_layer_manager.h"
#include "layer_tree_builder.h"
#include "rasterizer.h"
#include "core/compositor/property_tree/paint_artifact_compositor.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/scroll_tree_node.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "../render/render_object.h"
#include <algorithm>
#include <cmath>

namespace lightui {

uint32_t ScrollLayerManager::next_layer_id_ = 1000;  // 从 1000 开始，避免与其他层冲突

ScrollLayerManager::ScrollLayerManager() = default;
ScrollLayerManager::~ScrollLayerManager() = default;

// =========================================================================
// 滚动容器管理
// =========================================================================

bool ScrollLayerManager::RegisterScrollContainer(RenderObject* container) {
    if (!container) {
        return false;
    }

    // 检查是否已注册
    if (scroll_containers_.find(container) != scroll_containers_.end()) {
        return true;  // 已注册
    }

    // 检查是否可滚动
    if (!IsScrollable(container)) {
        return false;
    }

    // 创建滚动容器信息
    ScrollContainerInfo info;
    info.container = container;
    
    // 获取视口尺寸
    const auto& layout = container->GetLayoutInfo();
    info.viewport_width = layout.width;
    info.viewport_height = layout.height;
    
    // 计算内容尺寸
    info.content_width = container->GetContentWidth();
    info.content_height = container->GetContentHeight();
    
    // 获取当前滚动位置
    info.scroll_x = container->GetScrollX();
    info.scroll_y = container->GetScrollY();
    
    // 计算滚动范围
    CalculateScrollBounds(info);
    
    // 创建滚动内容层
    info.content_layer = CreateScrollContentLayer(container);
    
    scroll_containers_[container] = std::move(info);
    return true;
}

void ScrollLayerManager::UnregisterScrollContainer(RenderObject* container) {
    if (!container) {
        return;
    }
    scroll_containers_.erase(container);
}

bool ScrollLayerManager::IsScrollContainer(RenderObject* obj) const {
    return scroll_containers_.find(obj) != scroll_containers_.end();
}

ScrollContainerInfo* ScrollLayerManager::GetScrollContainerInfo(RenderObject* container) {
    auto it = scroll_containers_.find(container);
    if (it != scroll_containers_.end()) {
        return &it->second;
    }
    return nullptr;
}

const ScrollContainerInfo* ScrollLayerManager::GetScrollContainerInfo(RenderObject* container) const {
    auto it = scroll_containers_.find(container);
    if (it != scroll_containers_.end()) {
        return &it->second;
    }
    return nullptr;
}

// =========================================================================
// 滚动处理
// =========================================================================

bool ScrollLayerManager::HandleScroll(RenderObject* container, float delta_x, float delta_y) {
    auto* info = GetScrollContainerInfo(container);
    if (!info) {
        return false;
    }

    float old_scroll_x = info->scroll_x;
    float old_scroll_y = info->scroll_y;

    // 更新滚动位置
    info->scroll_x += delta_x;
    info->scroll_y += delta_y;
    
    // 限制在有效范围内
    ClampScrollPosition(*info);

    // 检查是否实际发生了滚动
    if (std::abs(info->scroll_x - old_scroll_x) < 0.001f &&
        std::abs(info->scroll_y - old_scroll_y) < 0.001f) {
        return false;  // 没有滚动
    }

    // 优先使用属性树系统的直接更新（不触发光栅化）
    if (IsUsingPropertyTreeSystem()) {
        PropertyTreeState* state = container->GetPropertyTreeState();
        if (state) {
            ScrollTreeNode* scroll_node = state->Scroll();
            if (scroll_node && paint_artifact_compositor_->CanDirectlyUpdateScrollOffset(scroll_node)) {
                // 通过属性树系统直接更新滚动偏移
                SkPoint new_offset = SkPoint::Make(info->scroll_x, info->scroll_y);
                if (paint_artifact_compositor_->DirectlyUpdateScrollOffset(scroll_node, new_offset)) {
                    // 同步到 RenderObject
                    container->SetScrollX(info->scroll_x);
                    container->SetScrollY(info->scroll_y);
                    
                    // 更新固定元素位置
                    UpdateFixedElementPositions();
                    
                    return true;
                }
            }
        }
    }

    // 回退：更新层的滚动偏移（不需要重新光栅化）
    if (info->content_layer) {
        info->content_layer->SetScrollOffset(SkPoint::Make(-info->scroll_x, -info->scroll_y));
    }

    // 同步到 RenderObject
    container->SetScrollX(info->scroll_x);
    container->SetScrollY(info->scroll_y);

    // 更新固定元素位置
    UpdateFixedElementPositions();

    return true;
}

bool ScrollLayerManager::ScrollTo(RenderObject* container, float scroll_x, float scroll_y) {
    auto* info = GetScrollContainerInfo(container);
    if (!info) {
        return false;
    }

    float delta_x = scroll_x - info->scroll_x;
    float delta_y = scroll_y - info->scroll_y;

    return HandleScroll(container, delta_x, delta_y);
}

void ScrollLayerManager::UpdateContentSize(RenderObject* container) {
    auto* info = GetScrollContainerInfo(container);
    if (!info) {
        return;
    }

    // 更新内容尺寸
    info->content_width = container->GetContentWidth();
    info->content_height = container->GetContentHeight();

    // 更新视口尺寸
    const auto& layout = container->GetLayoutInfo();
    info->viewport_width = layout.width;
    info->viewport_height = layout.height;

    // 重新计算滚动范围
    CalculateScrollBounds(*info);

    // 确保滚动位置在有效范围内
    ClampScrollPosition(*info);

    // 更新层边界
    if (info->content_layer) {
        SkRect bounds = SkRect::MakeWH(info->content_width, info->content_height);
        info->content_layer->SetBounds(bounds);
    }
}

bool ScrollLayerManager::NeedsRasterizeAfterScroll(RenderObject* container,
                                                    float old_scroll_x, float old_scroll_y) {
    auto* info = GetScrollContainerInfo(container);
    if (!info || !info->content_layer) {
        return false;
    }

    // 计算滚动增量
    float delta_x = info->scroll_x - old_scroll_x;
    float delta_y = info->scroll_y - old_scroll_y;

    // 如果没有滚动，不需要光栅化
    if (std::abs(delta_x) < 0.001f && std::abs(delta_y) < 0.001f) {
        return false;
    }

    // 检查是否有新区域需要光栅化
    // 这取决于内容层是否已经光栅化了足够的内容
    // 简单实现：如果滚动超出已光栅化区域，需要光栅化
    
    // 当前实现假设整个内容已经光栅化
    // 未来可以实现按需光栅化（只光栅化可见区域 + 缓冲区）
    return false;
}

// =========================================================================
// 固定元素管理
// =========================================================================

bool ScrollLayerManager::RegisterFixedElement(RenderObject* element) {
    if (!element) {
        return false;
    }

    // 检查是否已注册
    if (fixed_elements_.find(element) != fixed_elements_.end()) {
        return true;
    }

    // 检查是否是 position: fixed
    if (!IsPositionFixed(element)) {
        return false;
    }

    // 创建固定元素信息
    FixedElementInfo info;
    info.element = element;
    
    // 获取固定位置
    const auto& layout = element->GetLayoutInfo();
    info.fixed_x = layout.x;
    info.fixed_y = layout.y;
    
    // 创建固定元素层
    info.layer = CreateFixedElementLayer(element);
    
    fixed_elements_[element] = std::move(info);
    return true;
}

void ScrollLayerManager::UnregisterFixedElement(RenderObject* element) {
    if (!element) {
        return;
    }
    fixed_elements_.erase(element);
}

bool ScrollLayerManager::IsFixedElement(RenderObject* obj) const {
    return fixed_elements_.find(obj) != fixed_elements_.end();
}

FixedElementInfo* ScrollLayerManager::GetFixedElementInfo(RenderObject* element) {
    auto it = fixed_elements_.find(element);
    if (it != fixed_elements_.end()) {
        return &it->second;
    }
    return nullptr;
}

const FixedElementInfo* ScrollLayerManager::GetFixedElementInfo(RenderObject* element) const {
    auto it = fixed_elements_.find(element);
    if (it != fixed_elements_.end()) {
        return &it->second;
    }
    return nullptr;
}

void ScrollLayerManager::UpdateFixedElementPositions() {
    // 固定元素的层位置不随滚动改变
    // 它们始终保持在视口的固定位置
    for (auto& [element, info] : fixed_elements_) {
        if (info.layer) {
            // 固定元素的变换矩阵保持不变
            // 滚动偏移为 0（不受滚动影响）
            info.layer->SetScrollOffset(SkPoint::Make(0, 0));
        }
    }
}

// =========================================================================
// 层创建
// =========================================================================

std::shared_ptr<CompositorLayer> ScrollLayerManager::CreateScrollContentLayer(RenderObject* container) {
    if (!container) {
        return nullptr;
    }

    auto layer = std::make_shared<CompositorLayer>(next_layer_id_++);
    layer->SetRenderObject(container);
    layer->SetPromotionReason(LayerPromotionReason::ScrollableContent);
    layer->SetDebugName("ScrollContent");

    // 设置层边界为内容尺寸
    float content_width = container->GetContentWidth();
    float content_height = container->GetContentHeight();
    
    // 确保至少有视口大小
    const auto& layout = container->GetLayoutInfo();
    content_width = std::max(content_width, layout.width);
    content_height = std::max(content_height, layout.height);
    
    layer->SetBounds(SkRect::MakeWH(content_width, content_height));

    // 设置初始滚动偏移
    layer->SetScrollOffset(SkPoint::Make(-container->GetScrollX(), -container->GetScrollY()));

    // 标记需要完整光栅化
    layer->MarkFullDirty();

    return layer;
}

std::shared_ptr<CompositorLayer> ScrollLayerManager::CreateFixedElementLayer(RenderObject* element) {
    if (!element) {
        return nullptr;
    }

    auto layer = std::make_shared<CompositorLayer>(next_layer_id_++);
    layer->SetRenderObject(element);
    layer->SetPromotionReason(LayerPromotionReason::PositionFixed);
    layer->SetDebugName("FixedElement");

    // 设置层边界
    const auto& layout = element->GetLayoutInfo();
    layer->SetBounds(SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height));

    // 固定元素不受滚动影响
    layer->SetScrollOffset(SkPoint::Make(0, 0));

    // 标记需要完整光栅化
    layer->MarkFullDirty();

    return layer;
}

// =========================================================================
// 查询
// =========================================================================

RenderObject* ScrollLayerManager::FindScrollContainerAt(float x, float y) const {
    // 从后往前遍历（后添加的在上面）
    for (const auto& [container, info] : scroll_containers_) {
        const auto& layout = container->GetLayoutInfo();
        SkRect bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
        if (bounds.contains(x, y)) {
            return container;
        }
    }
    return nullptr;
}

// =========================================================================
// 清理
// =========================================================================

void ScrollLayerManager::Clear() {
    scroll_containers_.clear();
    fixed_elements_.clear();
}

// =========================================================================
// 私有方法
// =========================================================================

bool ScrollLayerManager::IsScrollable(RenderObject* obj) const {
    if (!obj) {
        return false;
    }
    return obj->IsScrollable();
}

bool ScrollLayerManager::IsPositionFixed(RenderObject* obj) const {
    if (!obj) {
        return false;
    }
    const auto& style = obj->GetComputedStyle();
    return style.position == "fixed";
}

void ScrollLayerManager::CalculateScrollBounds(ScrollContainerInfo& info) {
    // 最大滚动范围 = 内容尺寸 - 视口尺寸
    info.max_scroll_x = std::max(0.0f, info.content_width - info.viewport_width);
    info.max_scroll_y = std::max(0.0f, info.content_height - info.viewport_height);
}

void ScrollLayerManager::ClampScrollPosition(ScrollContainerInfo& info) {
    info.scroll_x = std::max(0.0f, std::min(info.scroll_x, info.max_scroll_x));
    info.scroll_y = std::max(0.0f, std::min(info.scroll_y, info.max_scroll_y));
}

} // namespace lightui
