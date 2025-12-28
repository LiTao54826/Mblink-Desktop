/**
 * @file paint_artifact.cpp
 * @brief 绘制产物实现
 */

#include "core/compositor/property_tree/paint/paint_artifact.h"
#include <unordered_set>

namespace lightui {

// =========================================================================
// 绘制指令
// =========================================================================

void PaintArtifact::AppendDisplayItem(DisplayItem item) {
    // 更新当前块的边界
    if (has_current_chunk_ && !paint_chunks_.empty()) {
        paint_chunks_.back().UnionBounds(item.GetBounds());
    }
    
    display_items_.push_back(std::move(item));
}

// =========================================================================
// 块管理
// =========================================================================

void PaintArtifact::StartNewChunk(const PropertyTreeState& state) {
    // 先结束当前块
    if (has_current_chunk_) {
        FinishCurrentChunk();
    }
    
    // 创建新块
    PaintChunk chunk;
    chunk.SetState(state);
    chunk.SetRange(display_items_.size(), display_items_.size());
    chunk.AssignId();
    
    paint_chunks_.push_back(std::move(chunk));
    current_chunk_begin_ = display_items_.size();
    has_current_chunk_ = true;
}

void PaintArtifact::FinishCurrentChunk() {
    if (!has_current_chunk_ || paint_chunks_.empty()) {
        return;
    }
    
    // 更新范围
    paint_chunks_.back().SetRange(current_chunk_begin_, display_items_.size());
    has_current_chunk_ = false;
}

PaintChunk* PaintArtifact::GetCurrentChunk() {
    if (!has_current_chunk_ || paint_chunks_.empty()) {
        return nullptr;
    }
    return &paint_chunks_.back();
}

// =========================================================================
// 状态
// =========================================================================

void PaintArtifact::Clear() {
    display_items_.clear();
    paint_chunks_.clear();
    current_chunk_begin_ = 0;
    has_current_chunk_ = false;
}

// =========================================================================
// 变化检测
// =========================================================================

PaintArtifactChangeInfo PaintArtifact::ComputeChanges(const PaintArtifact& previous) const {
    PaintArtifactChangeInfo info;
    
    // 构建旧块的 ID 集合
    std::unordered_set<uint64_t> old_chunk_ids;
    for (const auto& chunk : previous.paint_chunks_) {
        old_chunk_ids.insert(chunk.GetId());
    }
    
    // 构建新块的 ID 集合
    std::unordered_set<uint64_t> new_chunk_ids;
    for (const auto& chunk : paint_chunks_) {
        new_chunk_ids.insert(chunk.GetId());
    }
    
    // 查找新增的块
    for (size_t i = 0; i < paint_chunks_.size(); ++i) {
        if (old_chunk_ids.find(paint_chunks_[i].GetId()) == old_chunk_ids.end()) {
            info.added_chunks.push_back(i);
        }
    }
    
    // 查找移除的块
    for (size_t i = 0; i < previous.paint_chunks_.size(); ++i) {
        if (new_chunk_ids.find(previous.paint_chunks_[i].GetId()) == new_chunk_ids.end()) {
            info.removed_chunks.push_back(i);
        }
    }
    
    // 查找修改的块（有光栅化失效）
    for (size_t i = 0; i < paint_chunks_.size(); ++i) {
        if (paint_chunks_[i].HasRasterInvalidation()) {
            info.modified_chunks.push_back(i);
        }
    }
    
    // 检查属性树状态变化
    // 简单比较：如果块数量不同，认为属性树变化
    if (paint_chunks_.size() != previous.paint_chunks_.size()) {
        info.property_trees_changed = true;
    }
    
    return info;
}

// =========================================================================
// 调试
// =========================================================================

size_t PaintArtifact::GetMemoryUsage() const {
    size_t usage = 0;
    
    // DisplayItems
    usage += display_items_.capacity() * sizeof(DisplayItem);
    for (const auto& item : display_items_) {
        if (item.GetPicture()) {
            usage += item.GetPicture()->approximateBytesUsed();
        }
    }
    
    // PaintChunks
    usage += paint_chunks_.capacity() * sizeof(PaintChunk);
    for (const auto& chunk : paint_chunks_) {
        usage += chunk.GetRasterInvalidationRects().capacity() * sizeof(SkRect);
    }
    
    return usage;
}

} // namespace lightui
