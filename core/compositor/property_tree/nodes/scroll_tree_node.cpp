/**
 * @file scroll_tree_node.cpp
 * @brief 滚动树节点实现
 */

#include "core/compositor/property_tree/nodes/scroll_tree_node.h"
#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include <algorithm>

namespace lightui {

ScrollTreeNode::ScrollTreeNode() {
    // 默认可以在合成器线程滚动
    can_compositor_scroll_ = true;
}

void ScrollTreeNode::SetContainerSize(const SkSize& size) {
    if (container_size_.width() == size.width() && 
        container_size_.height() == size.height()) {
        return;
    }
    
    container_size_ = size;
    
    // 重新限制滚动偏移
    scroll_offset_ = ClampScrollOffset(scroll_offset_);
    
    MarkDirty();
}

void ScrollTreeNode::SetContentSize(const SkSize& size) {
    if (content_size_.width() == size.width() && 
        content_size_.height() == size.height()) {
        return;
    }
    
    content_size_ = size;
    
    // 重新限制滚动偏移
    scroll_offset_ = ClampScrollOffset(scroll_offset_);
    
    MarkDirty();
}

void ScrollTreeNode::SetScrollOffset(const SkPoint& offset) {
    SkPoint clamped = ClampScrollOffset(offset);
    
    if (scroll_offset_.x() == clamped.x() && scroll_offset_.y() == clamped.y()) {
        return;
    }
    
    scroll_offset_ = clamped;
    
    // 更新关联的变换节点
    if (scroll_transform_node_) {
        // 滚动偏移作为负平移应用
        SkM44 scroll_transform = SkM44::Translate(-scroll_offset_.x(), -scroll_offset_.y(), 0);
        scroll_transform_node_->SetMatrix(scroll_transform);
    }
    
    MarkDirty();
}

void ScrollTreeNode::ScrollBy(float dx, float dy) {
    SetScrollOffset(scroll_offset_.x() + dx, scroll_offset_.y() + dy);
}

SkPoint ScrollTreeNode::GetMaxScrollOffset() const {
    float max_x = std::max(0.0f, content_size_.width() - container_size_.width());
    float max_y = std::max(0.0f, content_size_.height() - container_size_.height());
    return SkPoint::Make(max_x, max_y);
}

bool ScrollTreeNode::CanScroll() const {
    SkPoint max_offset = GetMaxScrollOffset();
    return max_offset.x() > 0 || max_offset.y() > 0;
}

ScrollDirection ScrollTreeNode::GetScrollDirection() const {
    ScrollDirection dir = ScrollDirection::kNone;
    
    if (CanScrollHorizontally()) {
        dir = dir | ScrollDirection::kHorizontal;
    }
    
    if (CanScrollVertically()) {
        dir = dir | ScrollDirection::kVertical;
    }
    
    return dir;
}

bool ScrollTreeNode::CanScrollHorizontally() const {
    return content_size_.width() > container_size_.width();
}

bool ScrollTreeNode::CanScrollVertically() const {
    return content_size_.height() > container_size_.height();
}

SkPoint ScrollTreeNode::ClampScrollOffset(const SkPoint& offset) const {
    SkPoint max_offset = GetMaxScrollOffset();
    SkPoint min_offset = GetMinScrollOffset();
    
    float x = std::max(min_offset.x(), std::min(max_offset.x(), offset.x()));
    float y = std::max(min_offset.y(), std::min(max_offset.y(), offset.y()));
    
    return SkPoint::Make(x, y);
}

} // namespace lightui
