/**
 * @file effect_tree_node.cpp
 * @brief 效果树节点实现
 */

#include "core/compositor/property_tree/effect_tree_node.h"
#include <algorithm>

namespace lightui {

EffectTreeNode::EffectTreeNode() {
    flags_ = EffectFlags::kNone;
}

void EffectTreeNode::SetOpacity(float opacity) {
    // 限制范围
    opacity = std::max(0.0f, std::min(1.0f, opacity));
    
    if (opacity_ == opacity) return;
    
    opacity_ = opacity;
    accumulated_opacity_valid_ = false;
    UpdateFlags();
    MarkDirty();
}

float EffectTreeNode::GetAccumulatedOpacity() const {
    if (accumulated_opacity_valid_) {
        return cached_accumulated_opacity_;
    }
    
    float accumulated = opacity_;
    if (parent_) {
        accumulated *= parent_->GetAccumulatedOpacity();
    }
    
    cached_accumulated_opacity_ = accumulated;
    accumulated_opacity_valid_ = true;
    return accumulated;
}

void EffectTreeNode::SetFilters(std::vector<FilterOperation> filters) {
    filters_ = std::move(filters);
    UpdateFlags();
    MarkDirty();
}

void EffectTreeNode::AddFilter(const FilterOperation& filter) {
    filters_.push_back(filter);
    UpdateFlags();
    MarkDirty();
}

void EffectTreeNode::ClearFilters() {
    if (filters_.empty()) return;
    
    filters_.clear();
    UpdateFlags();
    MarkDirty();
}

void EffectTreeNode::SetBackdropFilters(std::vector<FilterOperation> filters) {
    backdrop_filters_ = std::move(filters);
    UpdateFlags();
    MarkDirty();
}

void EffectTreeNode::SetBlendMode(SkBlendMode mode) {
    if (blend_mode_ == mode) return;
    
    blend_mode_ = mode;
    UpdateFlags();
    MarkDirty();
}

void EffectTreeNode::SetMask(RenderObject* mask) {
    if (mask_ == mask) return;
    
    mask_ = mask;
    UpdateFlags();
    MarkDirty();
}

bool EffectTreeNode::RequiresIsolation() const {
    // 有 backdrop-filter 需要隔离
    if (HasBackdropFilter()) {
        return true;
    }
    
    // 有非默认 blend-mode 需要隔离
    if (HasBlendMode()) {
        return true;
    }
    
    // 有遮罩需要隔离
    if (HasMask()) {
        return true;
    }
    
    // opacity < 1 且有多个子元素时需要隔离
    // 这样才能正确地将所有子元素作为一个整体应用透明度
    if (HasOpacity() && children_.size() > 1) {
        return true;
    }
    
    return false;
}

void EffectTreeNode::UpdateFlags() {
    flags_ = EffectFlags::kNone;
    
    if (HasOpacity()) {
        flags_ = flags_ | EffectFlags::kHasOpacity;
    }
    
    if (HasFilter()) {
        flags_ = flags_ | EffectFlags::kHasFilter;
    }
    
    if (HasBackdropFilter()) {
        flags_ = flags_ | EffectFlags::kHasBackdropFilter;
    }
    
    if (HasBlendMode()) {
        flags_ = flags_ | EffectFlags::kHasBlendMode;
    }
    
    if (HasMask()) {
        flags_ = flags_ | EffectFlags::kHasMask;
    }
    
    if (RequiresIsolation()) {
        flags_ = flags_ | EffectFlags::kRequiresIsolation;
    }
}

} // namespace lightui
