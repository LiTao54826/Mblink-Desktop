/**
 * @file paint_chunk.cpp
 * @brief 绘制块实现
 */

#include "core/compositor/property_tree/paint/paint_chunk.h"

namespace lightui {

// 静态成员初始化
uint64_t PaintChunk::next_id_ = 1;

// =========================================================================
// 合并
// =========================================================================

bool PaintChunk::CanMergeWith(const PaintChunk& other) const {
    // 属性树状态必须可以合并
    if (!state_.CanMergeWith(other.state_)) {
        return false;
    }
    
    // 如果任一块有强制合成原因，不能合并
    if (HasCompositingReason(compositing_reasons_, CompositingReasons::kWillChangeTransform) ||
        HasCompositingReason(compositing_reasons_, CompositingReasons::kWillChangeOpacity) ||
        HasCompositingReason(compositing_reasons_, CompositingReasons::kActiveTransformAnimation) ||
        HasCompositingReason(compositing_reasons_, CompositingReasons::kActiveOpacityAnimation) ||
        HasCompositingReason(compositing_reasons_, CompositingReasons::kVideo) ||
        HasCompositingReason(compositing_reasons_, CompositingReasons::kCanvas)) {
        return false;
    }
    
    if (HasCompositingReason(other.compositing_reasons_, CompositingReasons::kWillChangeTransform) ||
        HasCompositingReason(other.compositing_reasons_, CompositingReasons::kWillChangeOpacity) ||
        HasCompositingReason(other.compositing_reasons_, CompositingReasons::kActiveTransformAnimation) ||
        HasCompositingReason(other.compositing_reasons_, CompositingReasons::kActiveOpacityAnimation) ||
        HasCompositingReason(other.compositing_reasons_, CompositingReasons::kVideo) ||
        HasCompositingReason(other.compositing_reasons_, CompositingReasons::kCanvas)) {
        return false;
    }
    
    return true;
}

void PaintChunk::MergeWith(const PaintChunk& other) {
    // 扩展范围
    if (other.begin_index_ < begin_index_) {
        begin_index_ = other.begin_index_;
    }
    if (other.end_index_ > end_index_) {
        end_index_ = other.end_index_;
    }
    
    // 合并边界
    UnionBounds(other.bounds_);
    
    // 合并合成原因
    compositing_reasons_ |= other.compositing_reasons_;
    
    // 合并光栅化失效区域
    for (const auto& rect : other.raster_invalidation_rects_) {
        raster_invalidation_rects_.push_back(rect);
    }
}

} // namespace lightui
