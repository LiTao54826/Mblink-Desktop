/**
 * @file layer_tree_manager.cpp
 * @brief 层树管理器实现
 */

#include "layer_tree_manager.h"
#include "layer_tree_builder.h"
#include "rasterizer.h"
#include "compositor.h"
#include "core/render/objects/render_object.h"
#include <algorithm>
#include <functional>
#include <iostream>

namespace lightui {

// ============================================================================
// 构造和析构
// ============================================================================

LayerTreeManager::LayerTreeManager() = default;

LayerTreeManager::~LayerTreeManager() {
    scroll_states_.clear();
    scroll_listeners_.clear();
    pending_updates_.clear();
}

// ============================================================================
// 初始化和配置
// ============================================================================

void LayerTreeManager::Initialize(LayerTreeBuilder* builder,
                                   Rasterizer* rasterizer,
                                   Compositor* compositor) {
    builder_ = builder;
    rasterizer_ = rasterizer;
    compositor_ = compositor;
    
    LogDebug("LayerTreeManager initialized");
}

void LayerTreeManager::SetViewport(float width, float height, float dpi_scale) {
    viewport_width_ = width;
    viewport_height_ = height;
    dpi_scale_ = dpi_scale;
    
    LogDebug("Viewport set: " + std::to_string(width) + "x" + 
             std::to_string(height) + " @ " + std::to_string(dpi_scale) + "x");
}

// ============================================================================
// 增量更新接口
// ============================================================================

void LayerTreeManager::RequestAddLayer(RenderObject* obj, LayerPromotionReason reason) {
    if (!obj) {
        return;
    }
    
    PendingLayerUpdate update;
    update.type = LayerUpdateType::Add;
    update.target = obj;
    update.reason = reason;
    update.debug_info = std::string("Add layer for ") + 
        CompositorLayer::PromotionReasonToString(reason);
    
    pending_updates_.push_back(update);
    
    LogDebug("Requested add layer: " + update.debug_info);
}

void LayerTreeManager::RequestRemoveLayer(RenderObject* obj) {
    if (!obj) {
        return;
    }
    
    PendingLayerUpdate update;
    update.type = LayerUpdateType::Remove;
    update.target = obj;
    update.debug_info = "Remove layer";
    
    pending_updates_.push_back(update);
    
    LogDebug("Requested remove layer");
}

void LayerTreeManager::RequestUpdateBounds(RenderObject* obj) {
    if (!obj) {
        return;
    }
    
    PendingLayerUpdate update;
    update.type = LayerUpdateType::UpdateBounds;
    update.target = obj;
    update.debug_info = "Update bounds";
    
    pending_updates_.push_back(update);
    
    LogDebug("Requested update bounds");
}

void LayerTreeManager::RequestReparent(RenderObject* obj) {
    if (!obj) {
        return;
    }
    
    PendingLayerUpdate update;
    update.type = LayerUpdateType::Reparent;
    update.target = obj;
    update.debug_info = "Reparent layer";
    
    pending_updates_.push_back(update);
    
    LogDebug("Requested reparent");
}

void LayerTreeManager::RequestUpdateZIndex(RenderObject* obj, int z_index) {
    if (!obj) {
        return;
    }
    
    PendingLayerUpdate update;
    update.type = LayerUpdateType::UpdateZIndex;
    update.target = obj;
    update.z_index = z_index;
    update.debug_info = "Update z-index to " + std::to_string(z_index);
    
    pending_updates_.push_back(update);
    
    LogDebug("Requested update z-index: " + std::to_string(z_index));
}

bool LayerTreeManager::ApplyPendingUpdates() {
    if (pending_updates_.empty()) {
        return false;
    }
    
    LogDebug("Applying " + std::to_string(pending_updates_.size()) + " pending updates");
    
    bool any_applied = false;
    for (const auto& update : pending_updates_) {
        if (ApplyUpdate(update)) {
            any_applied = true;
        }
    }
    
    pending_updates_.clear();
    
    if (any_applied) {
        IncrementTreeVersion();
    }
    
    return any_applied;
}

bool LayerTreeManager::ApplyUpdate(const PendingLayerUpdate& update) {
    if (!update.target || !builder_) {
        return false;
    }
    
    bool success = false;
    
    switch (update.type) {
        case LayerUpdateType::Add: {
            auto layer = builder_->AddLayerForObject(update.target, update.reason);
            success = (layer != nullptr);
            break;
        }
        
        case LayerUpdateType::Remove: {
            success = builder_->RemoveLayerForObject(update.target);
            break;
        }
        
        case LayerUpdateType::UpdateBounds: {
            auto layer = builder_->GetLayerForRenderObject(update.target);
            if (layer) {
                builder_->UpdateLayerBoundsDeferred(layer.get(), update.target);
                success = true;
            }
            break;
        }
        
        case LayerUpdateType::Reparent: {
            // 重新附加父层：先移除再添加
            auto layer = builder_->GetLayerForRenderObject(update.target);
            if (layer) {
                auto reason = layer->GetPromotionReason();
                builder_->RemoveLayerForObject(update.target);
                auto new_layer = builder_->AddLayerForObject(update.target, reason);
                success = (new_layer != nullptr);
            }
            break;
        }
        
        case LayerUpdateType::UpdateZIndex: {
            // z-index 更新：需要重新排序
            auto layer = builder_->GetLayerForRenderObject(update.target);
            if (layer && layer->GetParent()) {
                // 从父层移除并重新按 z-index 插入
                auto parent = layer->GetParent();
                parent->RemoveChild(layer.get());
                parent->InsertChildByZIndex(
                    builder_->GetLayerForRenderObject(update.target), 
                    update.z_index);
                success = true;
            }
            break;
        }
    }
    
    LogDebug("Applied update: " + update.debug_info + 
             (success ? " (success)" : " (failed)"));
    
    return success;
}

void LayerTreeManager::ForceFullRebuild(const std::string& reason) {
    needs_full_rebuild_ = true;
    last_rebuild_reason_ = reason;
    
    LogDebug("Forced full rebuild: " + reason);
}

// ============================================================================
// 滚动状态管理（SSOT）
// ============================================================================

bool LayerTreeManager::RegisterScrollContainer(RenderObject* container) {
    if (!container) {
        return false;
    }
    
    // 检查是否已注册
    if (scroll_states_.find(container) != scroll_states_.end()) {
        return true;  // 已注册
    }
    
    // 创建新的滚动状态
    ScrollState state;
    CalculateScrollBounds(state, container);
    
    scroll_states_[container] = state;
    
    LogDebug("Registered scroll container");
    
    return true;
}

void LayerTreeManager::UnregisterScrollContainer(RenderObject* container) {
    if (!container) {
        return;
    }
    
    auto it = scroll_states_.find(container);
    if (it != scroll_states_.end()) {
        scroll_states_.erase(it);
        LogDebug("Unregistered scroll container");
    }
}

bool LayerTreeManager::IsScrollContainer(RenderObject* container) const {
    return scroll_states_.find(container) != scroll_states_.end();
}

const ScrollState* LayerTreeManager::GetScrollState(RenderObject* container) const {
    auto it = scroll_states_.find(container);
    if (it != scroll_states_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool LayerTreeManager::SetScrollPosition(RenderObject* container, float x, float y) {
    auto it = scroll_states_.find(container);
    if (it == scroll_states_.end()) {
        return false;
    }
    
    ScrollState& state = it->second;
    
    // 保存旧值用于检测变化
    float old_x = state.scroll_x;
    float old_y = state.scroll_y;
    
    // 设置新值
    state.scroll_x = x;
    state.scroll_y = y;
    
    // 限制在有效范围内
    ClampScrollPosition(state);
    
    // 检查是否有变化
    if (state.scroll_x != old_x || state.scroll_y != old_y) {
        state.version++;
        NotifyScrollListeners(container, state);
        
        // 关键修复：更新层的滚动偏移
        // 这样合成时才能正确应用滚动偏移
        auto layer = container->GetCompositorLayer();
        if (layer) {
            layer->SetScrollOffset(SkPoint::Make(state.scroll_x, state.scroll_y));
            layer->MarkFullDirty();  // 需要重新光栅化
            

        }
        
        // 同步到 RenderObject
        container->SetScrollX(state.scroll_x);
        container->SetScrollY(state.scroll_y);
        
        LogDebug("Scroll position set: (" + std::to_string(state.scroll_x) + 
                 ", " + std::to_string(state.scroll_y) + ")");
    }
    
    return true;
}

bool LayerTreeManager::ScrollBy(RenderObject* container, float dx, float dy) {
    auto it = scroll_states_.find(container);
    if (it == scroll_states_.end()) {
        return false;
    }
    
    const ScrollState& state = it->second;
    return SetScrollPosition(container, state.scroll_x + dx, state.scroll_y + dy);
}

void LayerTreeManager::UpdateScrollContentSize(RenderObject* container) {
    auto it = scroll_states_.find(container);
    if (it == scroll_states_.end()) {
        return;
    }
    
    ScrollState& state = it->second;
    CalculateScrollBounds(state, container);
    ClampScrollPosition(state);
    
    LogDebug("Updated scroll content size");
}

uint32_t LayerTreeManager::AddScrollListener(ScrollListener listener) {
    uint32_t id = next_listener_id_++;
    scroll_listeners_[id] = std::move(listener);
    return id;
}

void LayerTreeManager::RemoveScrollListener(uint32_t id) {
    scroll_listeners_.erase(id);
}

void LayerTreeManager::NotifyScrollListeners(RenderObject* container, 
                                              const ScrollState& state) {
    for (const auto& [id, listener] : scroll_listeners_) {
        if (listener) {
            listener(container, state);
        }
    }
}

void LayerTreeManager::CalculateScrollBounds(ScrollState& state, 
                                              RenderObject* container) {
    if (!container) {
        return;
    }
    
    // 从 RenderObject 获取尺寸信息
    const LayoutInfo& layout = container->GetLayoutInfo();
    state.viewport_width = layout.width;
    state.viewport_height = layout.height;
    state.content_width = container->GetScrollWidth();
    state.content_height = container->GetScrollHeight();
    
    // 计算最大滚动位置
    state.max_scroll_x = std::max(0.0f, state.content_width - state.viewport_width);
    state.max_scroll_y = std::max(0.0f, state.content_height - state.viewport_height);
}

void LayerTreeManager::ClampScrollPosition(ScrollState& state) {
    state.scroll_x = std::clamp(state.scroll_x, 0.0f, state.max_scroll_x);
    state.scroll_y = std::clamp(state.scroll_y, 0.0f, state.max_scroll_y);
}

// ============================================================================
// 坐标转换
// ============================================================================

SkPoint LayerTreeManager::ConvertPoint(const SkPoint& point,
                                        CoordinateSpace from,
                                        CoordinateSpace to,
                                        CompositorLayer* layer) const {
    if (from == to) {
        return point;
    }
    
    SkPoint result = point;
    
    // 先转换到文档坐标
    if (from == CoordinateSpace::Viewport) {
        result.fX += document_scroll_x_;
        result.fY += document_scroll_y_;
    } else if (from == CoordinateSpace::Layer && layer) {
        const SkRect& bounds = layer->GetBounds();
        result.fX += bounds.fLeft;
        result.fY += bounds.fTop;
    }
    
    // 再从文档坐标转换到目标坐标
    if (to == CoordinateSpace::Viewport) {
        result.fX -= document_scroll_x_;
        result.fY -= document_scroll_y_;
    } else if (to == CoordinateSpace::Layer && layer) {
        const SkRect& bounds = layer->GetBounds();
        result.fX -= bounds.fLeft;
        result.fY -= bounds.fTop;
    }
    
    return result;
}

SkRect LayerTreeManager::ConvertRect(const SkRect& rect,
                                      CoordinateSpace from,
                                      CoordinateSpace to,
                                      CompositorLayer* layer) const {
    SkPoint topLeft = ConvertPoint(SkPoint::Make(rect.fLeft, rect.fTop), from, to, layer);
    SkPoint bottomRight = ConvertPoint(SkPoint::Make(rect.fRight, rect.fBottom), from, to, layer);
    
    return SkRect::MakeLTRB(topLeft.fX, topLeft.fY, bottomRight.fX, bottomRight.fY);
}

SkRect LayerTreeManager::CalculateLayerBoundsInDocument(RenderObject* obj,
                                                         CompositorLayer* parent_layer) const {
    if (!obj) {
        return SkRect::MakeEmpty();
    }
    
    // 获取 RenderObject 的布局位置和尺寸
    const LayoutInfo& layout = obj->GetLayoutInfo();
    float x = layout.x;
    float y = layout.y;
    float width = layout.width;
    float height = layout.height;
    
    // 累加祖先位置（直到父层的 RenderObject）
    auto parent = obj->GetParent();
    RenderObject* parent_layer_obj = parent_layer ? parent_layer->GetRenderObject() : nullptr;
    
    while (parent && parent.get() != parent_layer_obj) {
        const LayoutInfo& parent_layout = parent->GetLayoutInfo();
        x += parent_layout.x;
        y += parent_layout.y;
        
        // 考虑滚动偏移
        const ScrollState* scroll_state = GetScrollState(parent.get());
        if (scroll_state) {
            x -= scroll_state->scroll_x;
            y -= scroll_state->scroll_y;
        }
        
        parent = parent->GetParent();
    }
    
    return SkRect::MakeXYWH(x, y, width, height);
}

SkRect LayerTreeManager::CalculateFixedLayerBounds(RenderObject* obj) const {
    if (!obj) {
        return SkRect::MakeEmpty();
    }
    
    // Fixed 元素使用视口坐标，不累加祖先位置
    const LayoutInfo& layout = obj->GetLayoutInfo();
    float x = layout.x;
    float y = layout.y;
    float width = layout.width;
    float height = layout.height;
    
    return SkRect::MakeXYWH(x, y, width, height);
}

// ============================================================================
// 文档滚动
// ============================================================================

void LayerTreeManager::SetDocumentScroll(float x, float y) {
    document_scroll_x_ = x;
    document_scroll_y_ = y;
}

// ============================================================================
// 层树版本和统计
// ============================================================================

size_t LayerTreeManager::GetLayerCount() const {
    if (builder_) {
        return builder_->GetLayerCount();
    }
    return 0;
}

// ============================================================================
// 调试支持
// ============================================================================

void LayerTreeManager::LogDebug(const std::string& message) const {
    if (debug_logging_) {
        std::cout << "[LayerTreeManager] " << message << std::endl;
    }
}

// ============================================================================
// 脏标记优化
// ============================================================================

void LayerTreeManager::MarkContentDirty(RenderObject* obj) {
    if (!obj || !builder_) {
        return;
    }
    
    // 找到对应的层
    auto layer = builder_->GetLayerForRenderObject(obj);
    if (layer) {
        // 只标记该层为脏，不影响其他层
        const auto& layout = obj->GetLayoutInfo();
        SkRect bounds = SkRect::MakeXYWH(0, 0, layout.width, layout.height);
        layer->MarkDirty(bounds);
        
        LogDebug("Marked content dirty for layer");
    }
}

void LayerTreeManager::MarkTransformDirty(RenderObject* obj) {
    if (!obj || !builder_) {
        return;
    }
    
    // Transform 变化不需要重新光栅化
    // 只需要在合成时应用新的 transform
    auto layer = builder_->GetLayerForRenderObject(obj);
    if (layer) {
        // 更新层的 transform（从 RenderObject 获取）
        const auto& style = obj->GetComputedStyle();
        if (style.transform.has_value()) {
            const auto& layout = obj->GetLayoutInfo();
            SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);
            SkMatrix transform = style.transform->ToSkMatrix(local_rect, style.transform_origin);
            layer->SetTransform(transform);
        } else {
            layer->SetTransform(SkMatrix::I());
        }
        
        LogDebug("Marked transform dirty (no rasterization needed)");
    }
}

void LayerTreeManager::MarkOpacityDirty(RenderObject* obj) {
    if (!obj || !builder_) {
        return;
    }
    
    // Opacity 变化不需要重新光栅化
    // 只需要在合成时应用新的 opacity
    auto layer = builder_->GetLayerForRenderObject(obj);
    if (layer) {
        const auto& style = obj->GetComputedStyle();
        layer->SetOpacity(style.opacity);
        
        LogDebug("Marked opacity dirty (no rasterization needed)");
    }
}

void LayerTreeManager::MarkBoundsDirty(RenderObject* obj) {
    if (!obj || !builder_) {
        return;
    }
    
    auto layer = builder_->GetLayerForRenderObject(obj);
    if (layer) {
        // 更新层边界
        builder_->UpdateLayerBoundsDeferred(layer.get(), obj);
        
        // 检查是否需要重新光栅化
        // 如果新边界比旧边界大，需要重新光栅化以显示新内容
        const SkRect& old_bounds = layer->GetBounds();
        const auto& layout = obj->GetLayoutInfo();
        
        if (layout.width > old_bounds.width() || layout.height > old_bounds.height()) {
            layer->MarkFullDirty();
            LogDebug("Bounds increased, marked for rasterization");
        } else {
            LogDebug("Bounds changed (no rasterization needed)");
        }
    }
}

// ============================================================================
// 动画状态保持
// ============================================================================

LayerTreeManager::AnimationStateData LayerTreeManager::SaveAnimationState(
    CompositorLayer* layer) const {
    
    AnimationStateData state;
    state.transform = SkMatrix::I();
    state.opacity = 1.0f;
    state.has_animation_bounds = false;
    
    if (layer) {
        state.transform = layer->GetTransform();
        state.opacity = layer->GetOpacity();
        
        const AnimationBounds* bounds = layer->GetAnimationBounds();
        if (bounds) {
            state.has_animation_bounds = true;
            state.animation_bounds = *bounds;
        }
    }
    
    return state;
}

void LayerTreeManager::RestoreAnimationState(CompositorLayer* layer,
                                              const AnimationStateData& state) {
    if (!layer) {
        return;
    }
    
    layer->SetTransform(state.transform);
    layer->SetOpacity(state.opacity);
    
    if (state.has_animation_bounds) {
        layer->SetAnimationBounds(state.animation_bounds);
    }
    
    LogDebug("Restored animation state to layer");
}

void LayerTreeManager::TransferAnimationState(CompositorLayer* old_layer,
                                               CompositorLayer* new_layer) {
    if (!old_layer || !new_layer) {
        return;
    }
    
    // 保存旧层的动画状态
    AnimationStateData state = SaveAnimationState(old_layer);
    
    // 恢复到新层
    RestoreAnimationState(new_layer, state);
    
    LogDebug("Transferred animation state from old layer to new layer");
}

// ============================================================================
// 调试支持
// ============================================================================

LayerTreeManager::LayerInspectionInfo LayerTreeManager::InspectLayer(
    CompositorLayer* layer) const {
    
    LayerInspectionInfo info;
    info.layer_identity = 0;
    info.layer_id = 0;
    info.promotion_reason = LayerPromotionReason::None;
    info.coordinate_space = CoordinateSpace::Document;
    info.bounds = SkRect::MakeEmpty();
    info.z_index = 0;
    info.tree_depth = 0;
    info.has_scroll_offset = false;
    info.scroll_x = 0;
    info.scroll_y = 0;
    info.parent_identity = 0;
    info.children_count = 0;
    
    if (!layer) {
        return info;
    }
    
    info.layer_identity = layer->GetLayerIdentity();
    info.layer_id = layer->GetId();
    info.debug_name = layer->GetDebugName();
    info.promotion_reason = layer->GetPromotionReason();
    info.bounds = layer->GetBounds();
    info.z_index = layer->GetZIndex();
    info.tree_depth = layer->GetTreeDepth();
    info.children_count = layer->GetChildren().size();
    
    // 确定坐标空间
    if (layer->IsFixedLayer()) {
        info.coordinate_space = CoordinateSpace::Viewport;
    } else {
        info.coordinate_space = CoordinateSpace::Document;
    }
    
    // 获取滚动偏移
    const SkPoint& scroll = layer->GetScrollOffset();
    if (scroll.fX != 0 || scroll.fY != 0) {
        info.has_scroll_offset = true;
        info.scroll_x = scroll.fX;
        info.scroll_y = scroll.fY;
    }
    
    // 获取父层标识
    auto parent = layer->GetParent();
    if (parent) {
        info.parent_identity = parent->GetLayerIdentity();
    }
    
    return info;
}

std::vector<LayerTreeManager::LayerInspectionInfo> LayerTreeManager::InspectAllLayers() const {
    std::vector<LayerInspectionInfo> result;
    
    if (!builder_) {
        return result;
    }
    
    auto root = builder_->GetRootLayer();
    if (!root) {
        return result;
    }
    
    // 递归收集所有层信息
    std::function<void(CompositorLayer*)> collect = [&](CompositorLayer* layer) {
        if (!layer) return;
        
        result.push_back(InspectLayer(layer));
        
        for (const auto& child : layer->GetChildren()) {
            collect(child.get());
        }
    };
    
    collect(root.get());
    
    return result;
}

void LayerTreeManager::DumpLayerTree() const {
    if (!builder_) {
        std::cout << "[LayerTreeManager] No layer tree builder" << std::endl;
        return;
    }
    
    auto root = builder_->GetRootLayer();
    if (!root) {
        std::cout << "[LayerTreeManager] No root layer" << std::endl;
        return;
    }
    
    std::cout << "[LayerTreeManager] Layer Tree Dump:" << std::endl;
    std::cout << "  Version: " << tree_version_ << std::endl;
    std::cout << "  Layer Count: " << GetLayerCount() << std::endl;
    std::cout << "  Scroll Containers: " << scroll_states_.size() << std::endl;
    std::cout << std::endl;
    
    // 递归打印层树
    std::function<void(CompositorLayer*, int)> dump = [&](CompositorLayer* layer, int indent) {
        if (!layer) return;
        
        std::string prefix(indent * 2, ' ');
        auto info = InspectLayer(layer);
        
        std::cout << prefix << "Layer #" << info.layer_id 
                  << " (identity=" << info.layer_identity << ")" << std::endl;
        std::cout << prefix << "  Name: " << info.debug_name << std::endl;
        std::cout << prefix << "  Reason: " 
                  << CompositorLayer::PromotionReasonToString(info.promotion_reason) << std::endl;
        std::cout << prefix << "  Bounds: (" << info.bounds.left() << ", " << info.bounds.top()
                  << ", " << info.bounds.width() << "x" << info.bounds.height() << ")" << std::endl;
        std::cout << prefix << "  Z-Index: " << info.z_index << std::endl;
        std::cout << prefix << "  Depth: " << info.tree_depth << std::endl;
        
        if (info.has_scroll_offset) {
            std::cout << prefix << "  Scroll: (" << info.scroll_x << ", " << info.scroll_y << ")" << std::endl;
        }
        
        std::cout << prefix << "  Children: " << info.children_count << std::endl;
        
        for (const auto& child : layer->GetChildren()) {
            dump(child.get(), indent + 1);
        }
    };
    
    dump(root.get(), 0);
}

} // namespace lightui
