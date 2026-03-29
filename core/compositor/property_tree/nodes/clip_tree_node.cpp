/**
 * @file clip_tree_node.cpp
 * @brief 裁剪树节点实现
 */

#include "core/compositor/property_tree/nodes/clip_tree_node.h"
#include "core/compositor/property_tree/nodes/transform_tree_node.h"

namespace mbink {

ClipTreeNode::ClipTreeNode() {
    type_ = ClipType::kNone;
}

void ClipTreeNode::SetClipRect(const SkRect& rect) {
    if (clip_rect_ == rect && type_ == ClipType::kRect) return;
    
    clip_rect_ = rect;
    type_ = ClipType::kRect;
    
    // 检查是否有圆角
    if (HasRoundedCorners()) {
        type_ = ClipType::kRoundedRect;
    }
    
    InvalidateAccumulatedClipCache();
    MarkDirty();
}

void ClipTreeNode::ClearClip() {
    if (type_ == ClipType::kNone) return;
    
    type_ = ClipType::kNone;
    clip_rect_ = SkRect::MakeEmpty();
    for (int i = 0; i < 4; ++i) {
        radii_[i] = {0, 0};
    }
    clip_path_.reset();
    
    InvalidateAccumulatedClipCache();
    MarkDirty();
}

void ClipTreeNode::SetRadii(const SkVector radii[4]) {
    bool changed = false;
    for (int i = 0; i < 4; ++i) {
        if (radii_[i].x() != radii[i].x() || radii_[i].y() != radii[i].y()) {
            radii_[i] = radii[i];
            changed = true;
        }
    }
    
    if (!changed) return;
    
    // 更新类型
    if (type_ == ClipType::kRect || type_ == ClipType::kRoundedRect) {
        type_ = HasRoundedCorners() ? ClipType::kRoundedRect : ClipType::kRect;
    }
    
    InvalidateAccumulatedClipCache();
    MarkDirty();
}

void ClipTreeNode::SetUniformRadius(float radius) {
    SkVector radii[4] = {{radius, radius}, {radius, radius}, 
                         {radius, radius}, {radius, radius}};
    SetRadii(radii);
}

bool ClipTreeNode::HasRoundedCorners() const {
    for (int i = 0; i < 4; ++i) {
        if (radii_[i].x() > 0 || radii_[i].y() > 0) {
            return true;
        }
    }
    return false;
}

SkRRect ClipTreeNode::GetRoundedRect() const {
    SkRRect rrect;
    rrect.setRectRadii(clip_rect_, radii_);
    return rrect;
}

void ClipTreeNode::SetClipPath(std::unique_ptr<SkPath> path) {
    clip_path_ = std::move(path);
    if (clip_path_) {
        type_ = ClipType::kPath;
        clip_rect_ = clip_path_->getBounds();
    } else {
        type_ = ClipType::kNone;
        clip_rect_ = SkRect::MakeEmpty();
    }
    
    InvalidateAccumulatedClipCache();
    MarkDirty();
}

void ClipTreeNode::SetClipPath(const SkPath& path) {
    SetClipPath(std::make_unique<SkPath>(path));
}

void ClipTreeNode::ClearClipPath() {
    if (!clip_path_) return;
    
    clip_path_.reset();
    type_ = ClipType::kNone;
    clip_rect_ = SkRect::MakeEmpty();
    
    InvalidateAccumulatedClipCache();
    MarkDirty();
}

SkRect ClipTreeNode::GetClipRectInSpace(const TransformTreeNode* target_space) const {
    if (type_ == ClipType::kNone) {
        return SkRect::MakeLTRB(-1e9f, -1e9f, 1e9f, 1e9f);  // 无限大
    }
    
    if (!transform_node_ || !target_space) {
        return clip_rect_;
    }
    
    // 计算从裁剪空间到目标空间的变换
    SkM44 transform = transform_node_->GetTransformTo(target_space);
    
    // 变换裁剪矩形的四个角
    SkV4 corners[4] = {
        {clip_rect_.fLeft, clip_rect_.fTop, 0, 1},
        {clip_rect_.fRight, clip_rect_.fTop, 0, 1},
        {clip_rect_.fRight, clip_rect_.fBottom, 0, 1},
        {clip_rect_.fLeft, clip_rect_.fBottom, 0, 1}
    };
    
    SkRect result = SkRect::MakeEmpty();
    for (int i = 0; i < 4; ++i) {
        SkV4 transformed = transform * corners[i];
        if (transformed.w != 0) {
            float x = transformed.x / transformed.w;
            float y = transformed.y / transformed.w;
            if (i == 0) {
                result = SkRect::MakeXYWH(x, y, 0, 0);
            } else {
                result.fLeft = std::min(result.fLeft, x);
                result.fTop = std::min(result.fTop, y);
                result.fRight = std::max(result.fRight, x);
                result.fBottom = std::max(result.fBottom, y);
            }
        }
    }
    
    return result;
}

SkRect ClipTreeNode::GetAccumulatedClipRect(const TransformTreeNode* target_space) const {
    // 检查缓存
    if (accumulated_clip_valid_ && cached_target_space_ == target_space) {
        return cached_accumulated_clip_;
    }
    
    // 计算当前节点的裁剪区域
    SkRect current_clip = GetClipRectInSpace(target_space);
    
    // 如果有父节点，与父节点的累积裁剪求交
    if (parent_) {
        SkRect parent_clip = parent_->GetAccumulatedClipRect(target_space);
        if (!current_clip.intersect(parent_clip)) {
            // 没有交集，返回空矩形
            current_clip = SkRect::MakeEmpty();
        }
    }
    
    // 更新缓存
    cached_accumulated_clip_ = current_clip;
    cached_target_space_ = target_space;
    accumulated_clip_valid_ = true;
    
    return current_clip;
}

void ClipTreeNode::InvalidateAccumulatedClipCache() {
    accumulated_clip_valid_ = false;
    cached_target_space_ = nullptr;
    
    // 递归使子节点缓存失效
    for (ClipTreeNode* child : children_) {
        child->InvalidateAccumulatedClipCache();
    }
}

} // namespace mbink
