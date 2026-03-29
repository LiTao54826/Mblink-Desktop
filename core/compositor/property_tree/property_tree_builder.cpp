/**
 * @file property_tree_builder.cpp
 * @brief Property tree builder implementation
 */

#include "core/compositor/property_tree/property_tree_builder.h"
#include "core/render/objects/render_object.h"
#include <cmath>

namespace mbink {

PropertyTreeBuilder::PropertyTreeBuilder(PropertyTrees& trees)
    : trees_(trees) {
}

void PropertyTreeBuilder::Build(RenderObject* root) {
    if (!root) return;
    
    // Clear old state
    Clear();
    
    // Build from root state
    PropertyTreeState root_state = trees_.GetRootState();
    BuildRecursive(root, root_state);
    
    // 关键修复：将属性树状态设置到 RenderObject 上
    // 这样 AnimationApplicator 才能通过 object->GetPropertyTreeState() 获取状态
    ApplyStatesToRenderObjects();
    
    // Increment version
    trees_.IncrementVersion();
}

void PropertyTreeBuilder::Update(RenderObject* changed_node) {
    if (!changed_node) return;
    
    // Get parent's state
    PropertyTreeState parent_state = trees_.GetRootState();
    auto parent = changed_node->GetParent();
    if (parent) {
        auto it = render_object_states_.find(parent.get());
        if (it != render_object_states_.end()) {
            parent_state = it->second;
        }
    }
    
    // Rebuild this node and its subtree
    BuildRecursive(changed_node, parent_state);
    
    // 关键修复：将更新后的状态设置到 RenderObject 上
    ApplyStatesToRenderObjects();
    
    // Increment version
    trees_.IncrementVersion();
}

void PropertyTreeBuilder::Clear() {
    render_object_states_.clear();
    trees_.Clear();
}

PropertyTreeState PropertyTreeBuilder::GetStateForRenderObject(RenderObject* obj) const {
    auto it = render_object_states_.find(obj);
    if (it != render_object_states_.end()) {
        return it->second;
    }
    return PropertyTreeState();
}

bool PropertyTreeBuilder::HasStateForRenderObject(RenderObject* obj) const {
    return render_object_states_.find(obj) != render_object_states_.end();
}

void PropertyTreeBuilder::BuildRecursive(RenderObject* obj, const PropertyTreeState& parent_state) {
    if (!obj) return;
    
    // Create current node's property tree state
    PropertyTreeState current_state = parent_state;
    
    // Create transform node (if needed)
    TransformTreeNode* transform_node = CreateTransformNodeIfNeeded(
        obj, parent_state.Transform());
    current_state.SetTransform(transform_node);
    
    // Create clip node (if needed)
    ClipTreeNode* clip_node = CreateClipNodeIfNeeded(
        obj, parent_state.Clip(), transform_node);
    current_state.SetClip(clip_node);
    
    // Create effect node (if needed)
    EffectTreeNode* effect_node = CreateEffectNodeIfNeeded(
        obj, parent_state.Effect(), transform_node, clip_node);
    current_state.SetEffect(effect_node);
    
    // Create scroll node (if needed)
    ScrollTreeNode* scroll_node = CreateScrollNodeIfNeeded(
        obj, parent_state.Scroll(), transform_node);
    current_state.SetScroll(scroll_node);
    
    // Save state mapping
    render_object_states_[obj] = current_state;
    
    // Recursively process children
    for (const auto& child : obj->GetChildren()) {
        BuildRecursive(child.get(), current_state);
    }
}

bool PropertyTreeBuilder::NeedsTransformNode(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // Has transform property
    if (style.transform.has_value()) {
        return true;
    }
    
    // Has positioning offset
    if (style.position == "relative" || style.position == "absolute" || 
        style.position == "fixed") {
        const auto& layout = obj->GetLayoutInfo();
        if (layout.x != 0 || layout.y != 0) {
            return true;
        }
    }
    
    // Has will-change: transform
    if (HasWillChangeTransform(obj)) {
        return true;
    }
    
    // Has active transform animation
    if (HasActiveTransformAnimation(obj)) {
        return true;
    }
    
    return false;
}

bool PropertyTreeBuilder::NeedsClipNode(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // overflow: hidden/scroll/auto
    if (style.overflow == "hidden" || style.overflow == "scroll" || 
        style.overflow == "auto") {
        return true;
    }
    if (style.overflow_x == "hidden" || style.overflow_x == "scroll" || 
        style.overflow_x == "auto") {
        return true;
    }
    if (style.overflow_y == "hidden" || style.overflow_y == "scroll" || 
        style.overflow_y == "auto") {
        return true;
    }
    
    // Has clip-path
    if (style.clip_path.has_value()) {
        return true;
    }
    
    return false;
}

bool PropertyTreeBuilder::NeedsEffectNode(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // opacity < 1
    if (style.opacity < 1.0f) {
        return true;
    }
    
    // Has filter
    if (style.filter.has_value()) {
        return true;
    }
    
    // Has backdrop-filter
    if (style.backdrop_filter.has_value()) {
        return true;
    }
    
    // Has will-change: opacity
    if (HasWillChangeOpacity(obj)) {
        return true;
    }
    
    // Has active opacity animation
    if (HasActiveOpacityAnimation(obj)) {
        return true;
    }
    
    return false;
}

bool PropertyTreeBuilder::NeedsScrollNode(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // overflow: scroll/auto
    if (style.overflow == "scroll" || style.overflow == "auto") {
        return true;
    }
    if (style.overflow_x == "scroll" || style.overflow_x == "auto") {
        return true;
    }
    if (style.overflow_y == "scroll" || style.overflow_y == "auto") {
        return true;
    }
    
    return false;
}

TransformTreeNode* PropertyTreeBuilder::CreateTransformNodeIfNeeded(
    RenderObject* obj,
    TransformTreeNode* parent) {
    
    if (!NeedsTransformNode(obj)) {
        return parent;
    }
    
    // Create new node
    TransformTreeNode* node = trees_.GetTransformTree().CreateNode(parent);
    node->SetRenderObject(obj);
    trees_.GetTransformTree().RegisterRenderObject(obj, node);
    
    // Set transform matrix
    SkM44 matrix = CreateTransformMatrix(obj);
    node->SetMatrix(matrix);
    
    // Set transform origin
    SkV3 origin = GetTransformOrigin(obj);
    node->SetOrigin(origin);
    
    // Set can directly update
    // 关键：如果有 will-change: transform 或任何动画，都允许直接更新
    // 这是因为 will-change 的目的就是告诉浏览器这个元素会变化
    bool can_directly_update = HasWillChangeTransform(obj) || 
                               HasActiveTransformAnimation(obj) ||
                               !obj->GetComputedStyle().animations.empty();  // 任何动画都允许
    node->SetCanDirectlyUpdate(can_directly_update);
    
    return node;
}

ClipTreeNode* PropertyTreeBuilder::CreateClipNodeIfNeeded(
    RenderObject* obj,
    ClipTreeNode* parent,
    TransformTreeNode* transform) {
    
    if (!NeedsClipNode(obj)) {
        return parent;
    }
    
    // Create new node
    ClipTreeNode* node = trees_.GetClipTree().CreateNode(parent);
    node->SetRenderObject(obj);
    node->SetTransformNode(transform);
    trees_.GetClipTree().RegisterRenderObject(obj, node);
    
    const auto& style = obj->GetComputedStyle();
    const auto& layout = obj->GetLayoutInfo();
    
    // Set clip rect
    SkRect clip_rect = SkRect::MakeXYWH(0, 0, layout.width, layout.height);
    node->SetClipRect(clip_rect);
    
    // Set border radius
    const auto& br = style.border_radius;
    float tl = br.top_left.ToPx(layout.width);
    float tr = br.top_right.ToPx(layout.width);
    float br_val = br.bottom_right.ToPx(layout.width);
    float bl = br.bottom_left.ToPx(layout.width);
    
    if (tl > 0 || tr > 0 || br_val > 0 || bl > 0) {
        SkVector radii[4] = {
            {tl, tl},
            {tr, tr},
            {br_val, br_val},
            {bl, bl}
        };
        node->SetRadii(radii);
    }
    
    return node;
}

EffectTreeNode* PropertyTreeBuilder::CreateEffectNodeIfNeeded(
    RenderObject* obj,
    EffectTreeNode* parent,
    TransformTreeNode* transform,
    ClipTreeNode* clip) {
    
    if (!NeedsEffectNode(obj)) {
        return parent;
    }
    
    // Create new node
    EffectTreeNode* node = trees_.GetEffectTree().CreateNode(parent);
    node->SetRenderObject(obj);
    node->SetTransformNode(transform);
    node->SetOutputClipNode(clip);
    trees_.GetEffectTree().RegisterRenderObject(obj, node);
    
    const auto& style = obj->GetComputedStyle();
    
    // Set opacity
    node->SetOpacity(style.opacity);
    
    // Set can directly update opacity
    bool can_directly_update = HasWillChangeOpacity(obj) || 
                               HasActiveOpacityAnimation(obj);
    node->SetCanDirectlyUpdateOpacity(can_directly_update);
    
    return node;
}

ScrollTreeNode* PropertyTreeBuilder::CreateScrollNodeIfNeeded(
    RenderObject* obj,
    ScrollTreeNode* parent,
    TransformTreeNode* transform) {
    
    if (!NeedsScrollNode(obj)) {
        return parent;
    }
    
    // Create new node
    ScrollTreeNode* node = trees_.GetScrollTree().CreateNode(parent);
    node->SetRenderObject(obj);
    trees_.GetScrollTree().RegisterRenderObject(obj, node);
    
    const auto& layout = obj->GetLayoutInfo();
    
    // Set container size
    node->SetContainerSize(layout.width, layout.height);
    
    // Set content size
    node->SetContentSize(obj->GetContentWidth(), obj->GetContentHeight());
    
    // Set scroll offset
    node->SetScrollOffset(obj->GetScrollX(), obj->GetScrollY());
    
    // Create scroll transform node
    TransformTreeNode* scroll_transform = trees_.GetTransformTree().CreateNode(transform);
    scroll_transform->SetScrollNode(node);
    node->SetScrollTransformNode(scroll_transform);
    
    // Set scroll transform matrix
    SkM44 scroll_matrix = SkM44::Translate(-obj->GetScrollX(), -obj->GetScrollY(), 0);
    scroll_transform->SetMatrix(scroll_matrix);
    
    return node;
}

SkM44 PropertyTreeBuilder::CreateTransformMatrix(RenderObject* obj) const {
    if (!obj) return SkM44();
    
    const auto& layout = obj->GetLayoutInfo();
    
    SkM44 result;
    
    // Apply positioning offset
    if (layout.x != 0 || layout.y != 0) {
        result = SkM44::Translate(layout.x, layout.y, 0);
    }
    
    // CSS transform is handled separately by the existing transform system
    // Here we just handle basic positioning
    
    return result;
}

SkV3 PropertyTreeBuilder::GetTransformOrigin(RenderObject* obj) const {
    if (!obj) return {0, 0, 0};
    
    const auto& style = obj->GetComputedStyle();
    const auto& layout = obj->GetLayoutInfo();
    
    // Default transform origin is center (50% 50%)
    float x = layout.width * 0.5f;
    float y = layout.height * 0.5f;
    
    // Parse transform-origin if set
    // For now, use default center
    
    return {x, y, 0};
}

bool PropertyTreeBuilder::HasActiveTransformAnimation(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // Check CSS animations
    for (const auto& anim : style.animations) {
        // 检查动画名称是否包含 transform 相关关键字
        if (anim.name.find("transform") != std::string::npos ||
            anim.name.find("move") != std::string::npos ||
            anim.name.find("slide") != std::string::npos ||
            anim.name.find("rotate") != std::string::npos ||
            anim.name.find("scale") != std::string::npos ||
            anim.name.find("spin") != std::string::npos ||      // 添加 spin
            anim.name.find("bounce") != std::string::npos ||    // 添加 bounce
            anim.name.find("shake") != std::string::npos ||     // 添加 shake
            anim.name.find("flip") != std::string::npos ||      // 添加 flip
            anim.name.find("zoom") != std::string::npos) {      // 添加 zoom
            return true;
        }
    }
    
    // Check CSS transitions
    for (const auto& trans : style.transitions) {
        if (trans.property == "transform" || trans.property == "all") {
            return true;
        }
    }
    
    // 如果元素有 will-change: transform，也认为有活动动画
    // 这样可以确保 will-change 元素总是可以直接更新
    if (HasWillChangeTransform(obj)) {
        return true;
    }
    
    return false;
}

bool PropertyTreeBuilder::HasActiveOpacityAnimation(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    
    // Check CSS animations
    for (const auto& anim : style.animations) {
        if (anim.name.find("opacity") != std::string::npos ||
            anim.name.find("fade") != std::string::npos) {
            return true;
        }
    }
    
    // Check CSS transitions
    for (const auto& trans : style.transitions) {
        if (trans.property == "opacity" || trans.property == "all") {
            return true;
        }
    }
    
    return false;
}

bool PropertyTreeBuilder::HasWillChangeTransform(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    return style.will_change.find("transform") != std::string::npos;
}

bool PropertyTreeBuilder::HasWillChangeOpacity(RenderObject* obj) const {
    if (!obj) return false;
    
    const auto& style = obj->GetComputedStyle();
    return style.will_change.find("opacity") != std::string::npos;
}

void PropertyTreeBuilder::ApplyStatesToRenderObjects() {
    // 将构建的属性树状态设置到对应的 RenderObject 上
    // 这样 AnimationApplicator 才能通过 object->GetPropertyTreeState() 获取状态
    for (auto& [obj, state] : render_object_states_) {
        if (obj) {
            // 创建一个新的 PropertyTreeState 副本并设置到 RenderObject
            auto state_copy = std::make_unique<PropertyTreeState>(state);
            obj->SetPropertyTreeState(std::move(state_copy));
        }
    }
}

} // namespace mbink
