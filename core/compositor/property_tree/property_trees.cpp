/**
 * @file property_trees.cpp
 * @brief 属性树集合实现
 */

#include "core/compositor/property_tree/property_trees.h"

namespace lightui {

PropertyTrees::PropertyTrees() {
    // 四棵树在构造时会自动创建根节点
}

PropertyTrees::~PropertyTrees() = default;

PropertyTreeState PropertyTrees::GetRootState() const {
    return PropertyTreeState(
        transform_tree_.GetRoot(),
        clip_tree_.GetRoot(),
        effect_tree_.GetRoot(),
        scroll_tree_.GetRoot()
    );
}

bool PropertyTrees::HasDirtyNodes() const {
    return transform_tree_.HasDirtyNodes() ||
           clip_tree_.HasDirtyNodes() ||
           effect_tree_.HasDirtyNodes() ||
           scroll_tree_.HasDirtyNodes();
}

void PropertyTrees::ClearAllDirtyFlags() {
    transform_tree_.ClearAllDirtyFlags();
    clip_tree_.ClearAllDirtyFlags();
    effect_tree_.ClearAllDirtyFlags();
    scroll_tree_.ClearAllDirtyFlags();
}

void PropertyTrees::Clear() {
    transform_tree_.Clear();
    clip_tree_.Clear();
    effect_tree_.Clear();
    scroll_tree_.Clear();
    ++version_;
}

} // namespace lightui
