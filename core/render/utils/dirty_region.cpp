/**
 * @file dirty_region.cpp
 * @brief 脏区域检测系统实现
 */

#include "dirty_region.h"
#include <algorithm>
#include <cmath>

namespace mblink {

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
    OptimizeWithThreshold(kMergeThreshold);
}

void DirtyRegion::OptimizeAdaptive(float viewport_width, float viewport_height) {
    if (regions_.empty()) {
        return;
    }

    float viewport_area = viewport_width * viewport_height;
    
    // 如果脏区域数量过多，直接合并为一个
    if (regions_.size() > kMaxRegions) {
        MergeAll();
        return;
    }
    
    // 计算脏区域总面积
    float total_dirty_area = CalculateTotalArea();
    
    // 如果脏区域总面积超过视口50%，直接合并为一个
    // 这种情况下分开绘制反而更慢
    if (total_dirty_area > viewport_area * 0.5f) {
        MergeAll();
        return;
    }
    
    // 大窗口使用更激进的合并阈值
    // 阈值 = max(10, sqrt(viewport_area) * 0.03)
    // 例如：1920x1080 -> sqrt(2073600) * 0.03 ≈ 43px
    float adaptive_threshold = std::max(kMergeThreshold, 
                                        std::sqrt(viewport_area) * 0.03f);
    
    OptimizeWithThreshold(adaptive_threshold);
    
    // 合并后如果仍然太多，继续用更大的阈值合并
    if (regions_.size() > 4) {
        OptimizeWithThreshold(adaptive_threshold * 2.0f);
    }
}

void DirtyRegion::OptimizeWithThreshold(float threshold) {
    if (regions_.size() <= 1) {
        return;
    }

    // 使用简单的合并算法
    bool merged = true;
    while (merged && regions_.size() > 1) {
        merged = false;
        
        for (size_t i = 0; i < regions_.size(); ++i) {
            for (size_t j = i + 1; j < regions_.size(); ++j) {
                if (ShouldMergeWithThreshold(regions_[i], regions_[j], threshold)) {
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

float DirtyRegion::CalculateTotalArea() const {
    float total = 0.0f;
    for (const auto& rect : regions_) {
        total += rect.width() * rect.height();
    }
    return total;
}

void DirtyRegion::MergeAll() {
    if (regions_.size() <= 1) {
        return;
    }
    
    auto bounds = GetBoundingRect();
    if (bounds.has_value()) {
        regions_.clear();
        regions_.push_back(bounds.value());
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
    return ShouldMergeWithThreshold(a, b, kMergeThreshold);
}

bool DirtyRegion::ShouldMergeWithThreshold(const SkRect& a, const SkRect& b, float threshold) const {
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
    return distance < threshold;
}

SkRect DirtyRegion::Merge(const SkRect& a, const SkRect& b) const {
    SkRect result = a;
    result.join(b);
    return result;
}

} // namespace mblink

