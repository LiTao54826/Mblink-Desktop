/**
 * @file dirty_region.cpp
 * @brief 脏区域检测系统实现
 */

#include "dirty_region.h"
#include <algorithm>
#include <cmath>

namespace lightui {

void DirtyRegion::AddRect(const SkRect& rect) {
    if (rect.isEmpty()) {
        return;
    }
    regions_.push_back(rect);
}

void DirtyRegion::AddRect(float x, float y, float width, float height) {
    AddRect(SkRect::MakeXYWH(x, y, width, height));
}

void DirtyRegion::MarkAll(float width, float height) {
    Clear();
    AddRect(0, 0, width, height);
}

void DirtyRegion::Clear() {
    regions_.clear();
}

std::optional<SkRect> DirtyRegion::GetBoundingRect() const {
    if (regions_.empty()) {
        return std::nullopt;
    }

    SkRect bounds = regions_[0];
    for (size_t i = 1; i < regions_.size(); ++i) {
        bounds.join(regions_[i]);
    }
    return bounds;
}

void DirtyRegion::Optimize() {
    if (regions_.size() <= 1) {
        return;
    }

    // 使用简单的合并算法
    bool merged = true;
    while (merged && regions_.size() > 1) {
        merged = false;
        
        for (size_t i = 0; i < regions_.size(); ++i) {
            for (size_t j = i + 1; j < regions_.size(); ++j) {
                if (ShouldMerge(regions_[i], regions_[j])) {
                    // 合并两个矩形
                    regions_[i] = Merge(regions_[i], regions_[j]);
                    // 删除第二个矩形
                    regions_.erase(regions_.begin() + j);
                    merged = true;
                    break;
                }
            }
            if (merged) {
                break;
            }
        }
    }
}

bool DirtyRegion::Intersects(const SkRect& rect) const {
    for (const auto& region : regions_) {
        if (SkRect::Intersects(region, rect)) {
            return true;
        }
    }
    return false;
}

bool DirtyRegion::ShouldMerge(const SkRect& a, const SkRect& b) const {
    // 如果两个矩形相交，则合并
    if (SkRect::Intersects(a, b)) {
        return true;
    }

    // 计算两个矩形之间的距离
    float dx = 0.0f;
    float dy = 0.0f;

    // 水平距离
    if (a.fRight < b.fLeft) {
        dx = b.fLeft - a.fRight;
    } else if (b.fRight < a.fLeft) {
        dx = a.fLeft - b.fRight;
    }

    // 垂直距离
    if (a.fBottom < b.fTop) {
        dy = b.fTop - a.fBottom;
    } else if (b.fBottom < a.fTop) {
        dy = a.fTop - b.fBottom;
    }

    // 如果距离小于阈值，则合并
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance < kMergeThreshold;
}

SkRect DirtyRegion::Merge(const SkRect& a, const SkRect& b) const {
    SkRect result = a;
    result.join(b);
    return result;
}

} // namespace lightui

