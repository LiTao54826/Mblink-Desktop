/**
 * @file pending_layer.cpp
 * @brief 待定层实现
 */

#include "core/compositor/property_tree/pending_layer.h"

namespace lightui {

// =========================================================================
// 绘制块
// =========================================================================

void PendingLayer::AddChunk(const PaintChunk* chunk) {
    if (!chunk) return;
    
    chunks_.push_back(chunk);
    UnionBounds(chunk->GetBounds());
    
    // 合并合成原因
    compositing_reasons_ |= chunk->GetCompositingReasons();
}

// =========================================================================
// 边界
// =========================================================================

void PendingLayer::UpdateBounds() {
    bounds_ = SkRect::MakeEmpty();
    for (const auto* chunk : chunks_) {
        if (chunk) {
            UnionBounds(chunk->GetBounds());
        }
    }
}

// =========================================================================
// 合并
// =========================================================================

bool PendingLayer::CanMergeWith(const PendingLayer& other) const {
    // 如果任一层需要独立层，不能合并
    if (RequiresOwnLayer() || other.RequiresOwnLayer()) {
        return false;
    }
    
    // 属性树状态必须可以合并
    if (!state_.CanMergeWith(other.state_)) {
        return false;
    }
    
    // 如果有强制合成原因，不能合并
    if (lightui::RequiresOwnLayer(compositing_reasons_) ||
        lightui::RequiresOwnLayer(other.compositing_reasons_)) {
        return false;
    }
    
    return true;
}

void PendingLayer::MergeWith(PendingLayer&& other) {
    // 合并绘制块
    for (const auto* chunk : other.chunks_) {
        chunks_.push_back(chunk);
    }
    
    // 合并边界
    UnionBounds(other.bounds_);
    
    // 合并合成原因
    compositing_reasons_ |= other.compositing_reasons_;
    
    // 清空 other
    other.chunks_.clear();
    other.bounds_ = SkRect::MakeEmpty();
}

} // namespace lightui
