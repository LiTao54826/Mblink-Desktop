/**
 * @file property_tree_state.cpp
 * @brief 属性树状态实现
 */

#include "core/compositor/property_tree/property_tree_state.h"

namespace mbink {

PropertyTreeState::PropertyTreeState(TransformTreeNode* transform,
                                     ClipTreeNode* clip,
                                     EffectTreeNode* effect,
                                     ScrollTreeNode* scroll)
    : transform_(transform)
    , clip_(clip)
    , effect_(effect)
    , scroll_(scroll) {
}

bool PropertyTreeState::operator==(const PropertyTreeState& other) const {
    return transform_ == other.transform_ &&
           clip_ == other.clip_ &&
           effect_ == other.effect_ &&
           scroll_ == other.scroll_;
}

bool PropertyTreeState::CanMergeWith(const PropertyTreeState& other) const {
    // 如果完全相同，可以合并
    if (*this == other) {
        return true;
    }
    
    // 检查变换节点
    // 如果变换节点不同，需要检查是否兼容
    if (transform_ != other.transform_) {
        // 如果一个是另一个的祖先，可能可以合并
        // 但为了简化，这里要求变换节点相同
        return false;
    }
    
    // 检查裁剪节点
    if (clip_ != other.clip_) {
        // 如果一个是另一个的祖先，可能可以合并
        // 但为了简化，这里要求裁剪节点相同
        return false;
    }
    
    // 检查效果节点
    if (effect_ != other.effect_) {
        // 效果节点不同通常不能合并
        // 因为效果（如 opacity、filter）需要独立应用
        return false;
    }
    
    // 滚动节点可以不同
    // 因为滚动只影响变换，不影响绘制内容
    
    return true;
}

PropertyTreeStateDifference PropertyTreeState::ComputeDifference(
    const PropertyTreeState& other) const {
    PropertyTreeStateDifference diff;
    
    diff.transform_changed = (transform_ != other.transform_);
    diff.clip_changed = (clip_ != other.clip_);
    diff.effect_changed = (effect_ != other.effect_);
    diff.scroll_changed = (scroll_ != other.scroll_);
    
    return diff;
}

PropertyTreeState PropertyTreeState::Root() {
    // 返回空状态，实际的根状态需要从 PropertyTrees 获取
    return PropertyTreeState();
}

} // namespace mbink
