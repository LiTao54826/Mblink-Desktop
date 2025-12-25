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
#include <iostream>
#include "core/dom/element.h"

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
    
    // 获取布局和样式信息
    const auto& layout = container->GetLayoutInfo();
    const auto& style = container->GetComputedStyle();
    
    // 获取 border 宽度（从ComputedStyle）
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();
    
    // 计算可见区域（减去border）
    float visible_width = layout.width - border_left - border_right;
    float visible_height = layout.height - border_top - border_bottom;
    
    // 获取overflow设置
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    
    // 判断是否允许滚动
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");
    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    
    // 计算内容尺寸
    // 注意：如果缓存的内容尺寸为 0，需要动态计算
    // 这在首次注册滚动容器时很重要，因为 Paint 可能还没有被调用
    info.content_width = container->GetContentWidth();
    info.content_height = container->GetContentHeight();
    
    if (info.content_width <= 0) {
        info.content_width = container->CalculateContentWidth();
    }
    if (info.content_height <= 0) {
        info.content_height = container->CalculateContentHeight();
    }
    
    // 计算是否需要滚动条（与Paint逻辑一致）
    const float scrollbar_width = 12.0f;
    bool needs_v_scroll = allow_v_scroll && (info.content_height > visible_height || overflow_y == "scroll");
    
    float content_area_width = visible_width;
    if (needs_v_scroll) {
        content_area_width -= scrollbar_width;
    }
    
    bool needs_h_scroll = allow_h_scroll && (info.content_width > content_area_width || overflow_x == "scroll");
    
    float content_area_height = visible_height;
    if (needs_h_scroll) {
        content_area_height -= scrollbar_width;
        // 重新检查是否需要垂直滚动条（水平滚动条可能导致需要垂直滚动条）
        if (allow_v_scroll && !needs_v_scroll && info.content_height > content_area_height) {
            needs_v_scroll = true;
            content_area_width = visible_width - scrollbar_width;
            // 重新检查水平滚动条
            needs_h_scroll = allow_h_scroll && info.content_width > content_area_width;
        }
    }
    
    // 初始 viewport 为可见区域
    info.viewport_width = visible_width;
    info.viewport_height = visible_height;
    
    // 如果有垂直滚动条，视口宽度要减去滚动条宽度
    if (needs_v_scroll) {
        info.viewport_width -= scrollbar_width;
    }
    
    // 重新检查水平滚动条
    if (overflow_x == "scroll" || (overflow_x == "auto" && info.content_width > info.viewport_width)) {
        needs_h_scroll = true;
    }
    
    // 如果有水平滚动条，视口高度要减去滚动条宽度
    if (needs_h_scroll) {
        info.viewport_height -= scrollbar_width;
    }

    std::cout << "[ScrollLayerManager] Registered container " << container 
              << " Viewport: " << info.viewport_width << "x" << info.viewport_height
              << " Content: " << info.content_width << "x" << info.content_height 
              << " NeedsScroll: V=" << needs_v_scroll << " H=" << needs_h_scroll << std::endl;
    
    // 获取当前滚动位置
    info.scroll_x = container->GetScrollX();
    info.scroll_y = container->GetScrollY();
    
    // 计算滚动范围
    CalculateScrollBounds(info);
    
    // 创建滚动内容层
    info.content_layer = CreateScrollContentLayer(container);
    
    // =========================================================================
    // 集成到 Layer Tree
    // =========================================================================
    auto clip_layer = container->GetCompositorLayer();
    if (clip_layer) {
        // 设置初始滚动偏移
        clip_layer->SetScrollOffset(SkPoint::Make(container->GetScrollX(), container->GetScrollY()));
        
        // 清空 content_layer 的旧子层，防止每一帧累积重复的子层
        info.content_layer->RemoveAllChildren();

        // 1. 转移 clip_layer 的所有子层到 content_layer
        // 这些子层是 container 的子元素，应该随内容滚动
        auto children = clip_layer->GetChildren(); // 复制列表
        for (const auto& child : children) {
            clip_layer->RemoveChild(child.get());
            info.content_layer->AddChild(child);
        }
        
        // 2. 将 content_layer 挂载到 clip_layer 下
        clip_layer->AddChild(info.content_layer);
        
        std::cout << "[ScrollLayerManager] Registered scroll container " << container 
                  << ", content_layer has " << info.content_layer->GetChildren().size() << " children" << std::endl;
    } else {
        std::cout << "[ScrollLayerManager] No clip_layer for container " << container << std::endl;
    }
    
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
        // 如果两次滚动尝试都失败，才返回false。
        // 有时候可能是因为精度问题或者已经到顶/底，但我们还是想看看日志
    }
    
    std::cout << "[ScrollLayerManager] HandleScroll: " << container << " delta=" << delta_x << "," << delta_y 
              << " pos=" << old_scroll_x << "," << old_scroll_y << " -> " << info->scroll_x << "," << info->scroll_y
              << " layer=" << info->content_layer.get() 
              << " max=" << info->max_scroll_x << "," << info->max_scroll_y << std::endl;

    // 检查是否实际发生了滚动
    if (std::abs(info->scroll_x - old_scroll_x) < 0.001f &&
        std::abs(info->scroll_y - old_scroll_y) < 0.001f) {
        return false;  // 没有滚动
    }

    // 优先使用属性树系统的直接更新（不触发光栅化）
    // 注意：这个优化只适用于所有子元素都有独立层的情况
    // 在当前架构下，没有独立层的子元素在 RenderObject::Paint 中绘制
    // 所以滚动时必须重新光栅化，不能使用这个优化
    // 
    // 暂时禁用属性树系统的直接更新，直到架构改进
    /*
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
    */

    // 更新层的滚动偏移（用于有独立层的子元素）
    auto clip_layer = container->GetCompositorLayer();
    std::cout << "[ScrollLayerManager] HandleScroll: clip_layer=" << clip_layer.get() << std::endl;
    
    if (clip_layer) {
        clip_layer->SetScrollOffset(SkPoint::Make(info->scroll_x, info->scroll_y));
    }
    
    if (info->content_layer) {
        info->content_layer->SetScrollOffset(SkPoint::Make(info->scroll_x, info->scroll_y));
    }
    
    // 关键：滚动时必须重新光栅化
    // 因为没有独立层的子元素在 RenderObject::Paint 中绘制
    // Paint 中应用滚动偏移，所以需要重新光栅化
    if (clip_layer) {
        clip_layer->MarkFullDirty();
        std::cout << "[ScrollLayerManager] Scroll: marked clip_layer dirty, "
                  << "scroll=" << info->scroll_x << "," << info->scroll_y << std::endl;
    } else {
        // 滚动容器没有独立层，需要标记其父层或根层为脏
        std::cout << "[ScrollLayerManager] No clip_layer in HandleScroll, finding parent layer" << std::endl;
        
        bool found_layer = false;
        auto parent = container->GetParent();
        while (parent) {
            auto parent_layer = parent->GetCompositorLayer();
            if (parent_layer) {
                parent_layer->MarkFullDirty();
                std::cout << "[ScrollLayerManager] Marked parent layer dirty for scroll" << std::endl;
                found_layer = true;
                break;
            }
            parent = parent->GetParent();
        }
        
        // 如果没有找到任何父层，说明整个层树只有根层
        // 这种情况下，我们需要通过其他方式触发重绘
        // 注意：这里我们已经调用了 container->SetScrollX/Y()，
        // 这会设置 needs_paint_ = true，在下一帧的 UpdateLayerTreeBounds 中会被检测到
        if (!found_layer) {
            std::cout << "[ScrollLayerManager] No parent layer found, relying on needs_paint flag" << std::endl;
        }
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
    // 注意：如果缓存的内容尺寸为 0，需要动态计算
    info->content_width = container->GetContentWidth();
    info->content_height = container->GetContentHeight();
    
    if (info->content_width <= 0) {
        info->content_width = container->CalculateContentWidth();
    }
    if (info->content_height <= 0) {
        info->content_height = container->CalculateContentHeight();
    }

    // 更新视口尺寸（与RegisterScrollContainer逻辑一致）
    const auto& layout = container->GetLayoutInfo();
    const auto& style = container->GetComputedStyle();
    
    // 获取 border 宽度
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 真正的可见区域（layout 减去 border）
    float visible_width = layout.width - border_left - border_right;
    float visible_height = layout.height - border_top - border_bottom;

    const float scrollbar_width = 12.0f;
    bool needs_v_scroll = false;
    bool needs_h_scroll = false;
    
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    
    if (overflow_y == "scroll" || (overflow_y == "auto" && info->content_height > visible_height)) {
        needs_v_scroll = true;
    }
    
    info->viewport_width = visible_width;
    info->viewport_height = visible_height;
    
    if (needs_v_scroll) {
        info->viewport_width -= scrollbar_width;
    }
    
    if (overflow_x == "scroll" || (overflow_x == "auto" && info->content_width > info->viewport_width)) {
        needs_h_scroll = true;
    }
    
    if (needs_h_scroll) {
        info->viewport_height -= scrollbar_width;
    }
    
    std::cout << "[ScrollLayerManager] Updated container " << container 
              << " Viewport: " << info->viewport_width << "x" << info->viewport_height 
              << " Content: " << info->content_width << "x" << info->content_height << std::endl;

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
    
    // 关键：content_layer 不设置 RenderObject
    // 它只是一个容器层，用于组织子层并应用滚动偏移
    // 
    // 层结构：
    // - clip_layer (RenderObject = container) - 绘制背景、边框、没有独立层的子元素
    //   - content_layer (RenderObject = nullptr) - 只用于应用滚动偏移
    //     - 子层们 (有独立层的子元素)
    //
    // 滚动时：
    // - clip_layer 需要重新光栅化（因为没有独立层的子元素在这里绘制）
    // - content_layer 的滚动偏移在合成阶段应用，影响其子层的位置
    //
    // 注意：这不是最优的实现。理想情况下应该：
    // 1. clip_layer 只绘制背景和边框
    // 2. content_layer 绘制所有子元素
    // 但这需要修改 RenderObject::Paint 的逻辑
    
    layer->SetPromotionReason(LayerPromotionReason::ScrollableContent);
    layer->SetDebugName("ScrollContent");

    // 设置层边界为内容尺寸
    // 注意：如果缓存的内容尺寸为 0，需要动态计算
    float content_width = container->GetContentWidth();
    float content_height = container->GetContentHeight();
    
    if (content_width <= 0) {
        content_width = container->CalculateContentWidth();
    }
    if (content_height <= 0) {
        content_height = container->CalculateContentHeight();
    }
    
    // 确保至少有视口大小
    const auto& layout = container->GetLayoutInfo();
    content_width = std::max(content_width, layout.width);
    content_height = std::max(content_height, layout.height);
    
    layer->SetBounds(SkRect::MakeWH(content_width, content_height));

    // 设置初始滚动偏移
    layer->SetScrollOffset(SkPoint::Make(container->GetScrollX(), container->GetScrollY()));

    // content_layer 没有 RenderObject，不需要光栅化
    // 它的位图是空的（透明的）

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
