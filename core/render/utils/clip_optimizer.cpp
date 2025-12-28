/**
 * @file clip_optimizer.cpp
 * @brief 裁剪优化系统实现
 */

#include "clip_optimizer.h"
#include "core/render/render_object.h"

namespace lightui {

// ========== ClipOptimizer 实现 ==========

bool ClipOptimizer::IsInViewport(const SkRect& rect) const {
    return SkRect::Intersects(viewport_, rect);
}

bool ClipOptimizer::IsOutsideViewport(const SkRect& rect) const {
    return !SkRect::Intersects(viewport_, rect);
}

SkRect ClipOptimizer::ClipToViewport(const SkRect& rect) const {
    SkRect result;
    if (result.intersect(viewport_, rect)) {
        return result;
    }
    return SkRect::MakeEmpty();
}

bool ClipOptimizer::IsVisible(const RenderObject* render_object) const {
    if (!render_object) return false;

    const auto& layout = render_object->GetLayoutInfo();
    if (!layout.is_laid_out) return false;

    // 检查对象边界是否在视口内
    SkRect obj_bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);

    // 如果有裁剪区域，检查是否在裁剪区域内
    if (!clip_rects_.empty()) {
        SkRect current_clip = GetCurrentClipRect();
        if (!SkRect::Intersects(current_clip, obj_bounds)) {
            return false;
        }
    }

    // 检查是否在视口内
    return IsInViewport(obj_bounds);
}

std::vector<std::shared_ptr<RenderObject>> ClipOptimizer::FilterVisible(
    const std::vector<std::shared_ptr<RenderObject>>& objects) {

    std::vector<std::shared_ptr<RenderObject>> visible_objects;
    visible_objects.reserve(objects.size());

    for (const auto& obj : objects) {
        total_object_count_++;

        if (IsVisible(obj.get())) {
            visible_objects.push_back(obj);
        } else {
            clipped_object_count_++;
        }
    }

    return visible_objects;
}

void ClipOptimizer::PushClipRect(const SkRect& clip_rect) {
    // 如果已有裁剪区域，与当前裁剪区域求交集
    if (!clip_rects_.empty()) {
        SkRect current = GetCurrentClipRect();
        SkRect intersected;
        if (intersected.intersect(current, clip_rect)) {
            clip_rects_.push_back(intersected);
        } else {
            // 如果没有交集，添加空矩形
            clip_rects_.push_back(SkRect::MakeEmpty());
        }
    } else {
        clip_rects_.push_back(clip_rect);
    }
}

void ClipOptimizer::PopClipRect() {
    if (!clip_rects_.empty()) {
        clip_rects_.pop_back();
    }
}

SkRect ClipOptimizer::GetCurrentClipRect() const {
    if (clip_rects_.empty()) {
        return viewport_;
    }
    return clip_rects_.back();
}

void ClipOptimizer::ClearClipRects() {
    clip_rects_.clear();
}

void ClipOptimizer::OptimizeClipRects() {
    if (clip_rects_.size() < 2) return;

    // 合并重叠的裁剪区域
    std::vector<SkRect> optimized;
    optimized.reserve(clip_rects_.size());

    for (size_t i = 0; i < clip_rects_.size(); ++i) {
        const auto& rect = clip_rects_[i];
        
        // 检查是否可以与已有的矩形合并
        bool merged = false;
        for (auto& opt_rect : optimized) {
            if (SkRect::Intersects(rect, opt_rect)) {
                // 合并为包含两者的最小矩形
                opt_rect.join(rect);
                merged = true;
                break;
            }
        }
        
        if (!merged) {
            optimized.push_back(rect);
        }
    }

    clip_rects_ = std::move(optimized);
}

void ClipOptimizer::ApplyClip(SkCanvas* canvas) const {
    if (!canvas) return;

    // 应用视口裁剪
    canvas->clipRect(viewport_);

    // 应用额外的裁剪区域
    for (const auto& clip_rect : clip_rects_) {
        canvas->clipRect(clip_rect);
    }
}

} // namespace lightui

