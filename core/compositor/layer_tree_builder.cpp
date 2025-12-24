/**
 * @file layer_tree_builder.cpp
 * @brief 层树构建器实现
 */

#include "layer_tree_builder.h"
#include "core/render/render_object.h"
#include <algorithm>

namespace lightui {

LayerTreeBuilder::LayerTreeBuilder() = default;

LayerTreeBuilder::~LayerTreeBuilder() {
    Clear();
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::Build(RenderObject* root) {
    if (!root) {
        return nullptr;
    }

    // 清除旧的层树
    Clear();

    // 创建根层
    root_layer_ = CreateLayer(root, LayerPromotionReason::RootLayer);
    root_layer_->SetDebugName("root");

    // 递归构建层树
    BuildRecursive(root, root_layer_.get());

    return root_layer_;
}

void LayerTreeBuilder::Update(RenderObject* changed_node) {
    if (!changed_node || !root_layer_) {
        return;
    }

    // 检查节点是否需要层提升
    LayerPromotionReason reason = ShouldPromote(changed_node);

    // 查找现有层
    auto it = render_object_to_layer_.find(changed_node);
    bool has_layer = (it != render_object_to_layer_.end());

    if (reason != LayerPromotionReason::None && !has_layer) {
        // 需要层但没有 -> 创建新层
        // TODO: 找到正确的父层并插入
    } else if (reason == LayerPromotionReason::None && has_layer) {
        // 不需要层但有 -> 移除层
        auto layer = it->second;
        if (auto parent = layer->GetParent()) {
            parent->RemoveChild(layer.get());
        }
        render_object_to_layer_.erase(it);
        layer_count_--;
    } else if (has_layer) {
        // 更新现有层的边界
        UpdateLayerBounds(it->second.get(), changed_node);
    }
}

LayerPromotionReason LayerTreeBuilder::ShouldPromote(RenderObject* obj) const {
    if (!obj || !layer_promotion_enabled_) {
        return LayerPromotionReason::None;
    }

    // 检查各种提升条件（按优先级）

    // 1. will-change: transform
    if (HasWillChangeTransform(obj)) {
        return LayerPromotionReason::WillChangeTransform;
    }

    // 2. will-change: opacity
    if (HasWillChangeOpacity(obj)) {
        return LayerPromotionReason::WillChangeOpacity;
    }

    // 3. position: fixed
    if (HasPositionFixed(obj)) {
        return LayerPromotionReason::PositionFixed;
    }

    // 4. transform 动画
    if (HasTransformAnimation(obj)) {
        return LayerPromotionReason::TransformAnimation;
    }

    // 5. opacity 动画
    if (HasOpacityAnimation(obj)) {
        return LayerPromotionReason::OpacityAnimation;
    }

    // 6. 可滚动容器
    if (IsScrollableContainer(obj)) {
        return LayerPromotionReason::ScrollableContent;
    }

    return LayerPromotionReason::None;
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::GetLayerForRenderObject(RenderObject* obj) const {
    auto it = render_object_to_layer_.find(obj);
    if (it != render_object_to_layer_.end()) {
        return it->second;
    }
    return nullptr;
}

void LayerTreeBuilder::Clear() {
    render_object_to_layer_.clear();
    root_layer_.reset();
    layer_count_ = 0;
}

void LayerTreeBuilder::BuildRecursive(RenderObject* obj, CompositorLayer* parent_layer) {
    if (!obj || !parent_layer) {
        return;
    }

    CompositorLayer* current_layer = parent_layer;

    // 检查是否需要为此节点创建新层
    LayerPromotionReason reason = ShouldPromote(obj);
    if (reason != LayerPromotionReason::None) {
        auto new_layer = CreateLayer(obj, reason);
        parent_layer->AddChild(new_layer);
        current_layer = new_layer.get();
    }

    // 递归处理子节点
    for (const auto& child : obj->GetChildren()) {
        BuildRecursive(child.get(), current_layer);
    }
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::CreateLayer(
    RenderObject* obj, LayerPromotionReason reason) {

    auto layer = CreateCompositorLayer();
    layer->SetRenderObject(obj);
    layer->SetPromotionReason(reason);
    layer->SetDpiScale(dpi_scale_);  // 设置 DPI 缩放

    // 设置层边界
    UpdateLayerBounds(layer.get(), obj);
    
    // 关键修复：新创建的层需要标记为脏，以便被光栅化
    layer->MarkFullDirty();

    // 关键修复：设置 RenderObject 的层关联
    // 这使得 HasOwnCompositorLayer() 和 GetCompositorLayer() 能正确工作
    if (obj) {
        obj->SetCompositorLayer(layer);
    }

    // 记录映射
    render_object_to_layer_[obj] = layer;
    layer_count_++;

    return layer;
}

void LayerTreeBuilder::UpdateLayerBounds(CompositorLayer* layer, RenderObject* obj) {
    if (!layer || !obj) {
        return;
    }

    const auto& layout = obj->GetLayoutInfo();
    
    // 对于根层，边界从 (0,0) 开始
    if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        layer->SetBounds(bounds);
        return;
    }
    
    // 对于非根层，需要计算相对于层树父层的位置
    // 
    // 关键理解：
    // - 层树父层对应的 RenderObject 可能是当前元素的祖先（不一定是直接父元素）
    // - 例如：RenderObject 树是 A -> B -> C -> D，其中 A 和 D 有独立层
    // - 层树是：A层 -> D层
    // - D 的 layout.x, layout.y 是相对于 C 的
    // - 但在层树中，D层的父层是 A层
    // - 所以 D层的边界应该是 D 相对于 A 的位置
    //
    // 合成器 (compositor.cpp) 在 CompositeLayerCPU 中会递归应用父层的位置：
    //   canvas->translate(bounds.left(), bounds.top());
    // 然后递归绘制子层时，canvas 已经被平移到了父层的位置。
    //
    // 所以这里需要计算的是：相对于层树父层对应的 RenderObject 的位置
    
    // 获取层树父层对应的 RenderObject
    auto parent_layer = layer->GetParent();
    RenderObject* parent_layer_obj = parent_layer ? parent_layer->GetRenderObject() : nullptr;
    
    // 计算相对于层树父层的位置
    float rel_x = layout.x;
    float rel_y = layout.y;
    
    // 从当前元素的直接父元素开始，累加位置和滚动偏移
    // 直到到达层树父层对应的 RenderObject
    auto parent = obj->GetParent();
    while (parent && parent.get() != parent_layer_obj) {
        const auto& parent_layout = parent->GetLayoutInfo();
        rel_x += parent_layout.x;
        rel_y += parent_layout.y;
        
        // 减去父元素的滚动偏移
        // 当父元素滚动时，子元素的视觉位置会相应移动
        rel_x -= parent->GetScrollX();
        rel_y -= parent->GetScrollY();
        
        parent = parent->GetParent();
    }
    
    // 关键修复：还需要减去层树父层的滚动偏移
    // 上面的循环在 parent == parent_layer_obj 时停止，没有减去它的滚动偏移
    // 但层树父层的滚动偏移同样会影响子层的视觉位置
    if (parent_layer_obj) {
        rel_x -= parent_layer_obj->GetScrollX();
        rel_y -= parent_layer_obj->GetScrollY();
    }
    
    SkRect bounds = SkRect::MakeXYWH(rel_x, rel_y, layout.width, layout.height);
    layer->SetBounds(bounds);
}

bool LayerTreeBuilder::HasWillChangeTransform(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    const std::string& will_change = style.will_change;
    
    // 检查 will-change 属性是否包含 "transform"
    // will-change 可以是 "auto", "transform", "opacity", "transform, opacity" 等
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    return will_change.find("transform") != std::string::npos;
}

bool LayerTreeBuilder::HasWillChangeOpacity(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    const std::string& will_change = style.will_change;
    
    // 检查 will-change 属性是否包含 "opacity"
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    return will_change.find("opacity") != std::string::npos;
}

bool LayerTreeBuilder::HasPositionFixed(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    return style.position == "fixed";
}

bool LayerTreeBuilder::HasTransformAnimation(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查是否有活动的 CSS 动画
    // 策略：如果元素有非空的动画名称，检查是否可能影响 transform
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 方法1：检查元素当前是否有 transform 属性
        // 如果有 transform 且有动画，很可能是 transform 动画
        if (style.transform.has_value()) {
            return true;
        }
        
        // 方法2：检查动画名称是否包含 transform 相关关键字
        // 这是一个启发式方法，不完美但有用
        std::string lower_name = anim.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find("transform") != std::string::npos ||
            lower_name.find("rotate") != std::string::npos ||
            lower_name.find("scale") != std::string::npos ||
            lower_name.find("translate") != std::string::npos ||
            lower_name.find("move") != std::string::npos ||
            lower_name.find("slide") != std::string::npos ||
            lower_name.find("spin") != std::string::npos ||
            lower_name.find("bounce") != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool LayerTreeBuilder::HasOpacityAnimation(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查是否有活动的 CSS 动画
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 方法1：检查元素当前 opacity 是否不为 1.0
        // 如果 opacity 不为 1.0 且有动画，很可能是 opacity 动画
        if (style.opacity < 1.0f) {
            return true;
        }
        
        // 方法2：检查动画名称是否包含 opacity 相关关键字
        std::string lower_name = anim.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find("opacity") != std::string::npos ||
            lower_name.find("fade") != std::string::npos ||
            lower_name.find("appear") != std::string::npos ||
            lower_name.find("disappear") != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool LayerTreeBuilder::IsScrollableContainer(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查 overflow 属性
    bool has_overflow_scroll = 
        style.overflow_x == "scroll" || style.overflow_x == "auto" ||
        style.overflow_y == "scroll" || style.overflow_y == "auto" ||
        style.overflow == "scroll" || style.overflow == "auto";

    if (!has_overflow_scroll) {
        return false;
    }

    // 检查是否有溢出内容
    const auto& layout = obj->GetLayoutInfo();
    float content_width = obj->GetContentWidth();
    float content_height = obj->GetContentHeight();

    bool has_overflow_content = 
        content_width > layout.width || 
        content_height > layout.height;

    return has_overflow_content;
}

} // namespace lightui
