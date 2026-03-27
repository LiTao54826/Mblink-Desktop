/**
 * @file raster_invalidator.cpp
 * @brief 光栅化失效器实现
 */

#include "core/compositor/property_tree/raster_invalidator.h"
#include <unordered_map>

namespace lightui {

// =========================================================================
// 构造函数
// =========================================================================

RasterInvalidator::RasterInvalidator(const GeometryMapper& mapper)
    : mapper_(mapper) {
}

// =========================================================================
// 失效计算
// =========================================================================

InvalidationResult RasterInvalidator::ComputeInvalidation(
    const PaintArtifact& old_artifact,
    const PaintArtifact& new_artifact,
    const PropertyTreeState& layer_state) {
    
    InvalidationResult result;
    
    const auto& old_chunks = old_artifact.GetPaintChunks();
    const auto& new_chunks = new_artifact.GetPaintChunks();
    
    // 构建旧块的 ID 映射
    std::unordered_map<uint64_t, const PaintChunk*> old_chunk_map;
    for (const auto& chunk : old_chunks) {
        old_chunk_map[chunk.GetId()] = &chunk;
    }
    
    // 构建新块的 ID 映射
    std::unordered_map<uint64_t, const PaintChunk*> new_chunk_map;
    for (const auto& chunk : new_chunks) {
        new_chunk_map[chunk.GetId()] = &chunk;
    }
    
    // 处理消失的块
    for (const auto& chunk : old_chunks) {
        if (new_chunk_map.find(chunk.GetId()) == new_chunk_map.end()) {
            HandleDisappearingChunk(chunk, layer_state, result);
        }
    }
    
    // 处理出现的块和变化的块
    for (const auto& chunk : new_chunks) {
        auto it = old_chunk_map.find(chunk.GetId());
        if (it == old_chunk_map.end()) {
            // 新出现的块
            HandleAppearingChunk(chunk, layer_state, result);
        } else {
            // 已存在的块，检查变化
            const PaintChunk& old_chunk = *it->second;
            
            if (ChunkHasMoved(old_chunk, chunk)) {
                HandleMovedChunk(old_chunk, chunk, layer_state, result);
            } else if (ChunkContentChanged(old_chunk, chunk)) {
                HandleContentChangedChunk(old_chunk, chunk, layer_state, result);
            }
        }
    }
    
    return result;
}

InvalidationResult RasterInvalidator::ComputeChunkInvalidation(
    const PaintChunk& old_chunk,
    const PaintChunk& new_chunk,
    const PropertyTreeState& layer_state) {
    
    InvalidationResult result;
    
    if (ChunkHasMoved(old_chunk, new_chunk)) {
        HandleMovedChunk(old_chunk, new_chunk, layer_state, result);
    } else if (ChunkContentChanged(old_chunk, new_chunk)) {
        HandleContentChangedChunk(old_chunk, new_chunk, layer_state, result);
    }
    
    return result;
}

// =========================================================================
// 失效处理
// =========================================================================

void RasterInvalidator::HandleAppearingChunk(
    const PaintChunk& chunk,
    const PropertyTreeState& layer_state,
    InvalidationResult& result) {
    
    // 新出现的块，失效其整个区域
    SkRect bounds = MapChunkBoundsToLayerSpace(chunk, layer_state);
    result.AddRect(bounds);
}

void RasterInvalidator::HandleDisappearingChunk(
    const PaintChunk& chunk,
    const PropertyTreeState& layer_state,
    InvalidationResult& result) {
    
    // 消失的块，失效其原来的区域
    SkRect bounds = MapChunkBoundsToLayerSpace(chunk, layer_state);
    result.AddRect(bounds);
}

void RasterInvalidator::HandleMovedChunk(
    const PaintChunk& old_chunk,
    const PaintChunk& new_chunk,
    const PropertyTreeState& layer_state,
    InvalidationResult& result) {
    
    // 移动的块，失效旧位置和新位置
    SkRect old_bounds = MapChunkBoundsToLayerSpace(old_chunk, layer_state);
    SkRect new_bounds = MapChunkBoundsToLayerSpace(new_chunk, layer_state);
    
    result.AddRect(old_bounds);
    result.AddRect(new_bounds);
}

void RasterInvalidator::HandleContentChangedChunk(
    const PaintChunk& old_chunk,
    const PaintChunk& new_chunk,
    const PropertyTreeState& layer_state,
    InvalidationResult& result) {
    PropertyTreeStateDifference diff =
        old_chunk.GetState().ComputeDifference(new_chunk.GetState());
    const bool has_state_change_beyond_transform =
        diff.clip_changed || diff.effect_changed || diff.scroll_changed;
    const bool has_chunk_identity_change =
        old_chunk.GetCompositingReasons() != new_chunk.GetCompositingReasons() ||
        old_chunk.GetRenderObject() != new_chunk.GetRenderObject();

    // 对于状态树/合成归属/关联对象切换，直接同时失效旧块和新块。
    // 这类变化可能不会反映在 bounds/item_count 上，但会改变实际绘制结果，
    // 增量光栅化如果只刷新新区域，容易保留旧像素造成重影。
    if (has_state_change_beyond_transform || has_chunk_identity_change) {
        result.AddRect(MapChunkBoundsToLayerSpace(old_chunk, layer_state));
        result.AddRect(MapChunkBoundsToLayerSpace(new_chunk, layer_state));
        return;
    }

    // 内容变化的块，优先使用块自带的光栅化失效区域。
    const auto& invalidation_rects = new_chunk.GetRasterInvalidationRects();

    if (invalidation_rects.empty()) {
        // 如果没有精确的失效区域，失效整个块
        SkRect bounds = MapChunkBoundsToLayerSpace(new_chunk, layer_state);
        result.AddRect(bounds);
    } else {
        // 使用精确的失效区域
        for (const auto& rect : invalidation_rects) {
            // 将失效区域转换到层坐标系
            SkRect layer_rect = mapper_.MapRect(
                rect, new_chunk.GetState(), layer_state);
            result.AddRect(layer_rect);
        }
    }
}

// =========================================================================
// 辅助方法
// =========================================================================

SkRect RasterInvalidator::MapChunkBoundsToLayerSpace(
    const PaintChunk& chunk,
    const PropertyTreeState& layer_state) const {
    
    return mapper_.MapRect(chunk.GetBounds(), chunk.GetState(), layer_state);
}

bool RasterInvalidator::ChunkHasMoved(
    const PaintChunk& old_chunk,
    const PaintChunk& new_chunk) const {
    
    // 检查属性树状态是否变化
    PropertyTreeStateDifference diff = 
        old_chunk.GetState().ComputeDifference(new_chunk.GetState());
    
    // 如果变换变化，认为块移动了
    return diff.transform_changed;
}

bool RasterInvalidator::ChunkContentChanged(
    const PaintChunk& old_chunk,
    const PaintChunk& new_chunk) const {
    // 检查是否有光栅化失效区域
    if (new_chunk.HasRasterInvalidation()) {
        return true;
    }

    // 检查边界是否变化
    if (old_chunk.GetBounds() != new_chunk.GetBounds()) {
        return true;
    }

    // 检查绘制指令数量是否变化
    if (old_chunk.GetItemCount() != new_chunk.GetItemCount()) {
        return true;
    }

    // 检查属性树状态中的非 transform 变化。
    // transform 变化由 ChunkHasMoved 处理，这里只兜住 clip/effect/scroll。
    PropertyTreeStateDifference diff =
        old_chunk.GetState().ComputeDifference(new_chunk.GetState());
    if (diff.clip_changed || diff.effect_changed || diff.scroll_changed) {
        return true;
    }

    // 检查块是否在相同 bounds 下切换了合成归属或关联对象。
    // 这种情况下视觉内容可能已经变化，但旧逻辑会漏判。
    if (old_chunk.GetCompositingReasons() != new_chunk.GetCompositingReasons()) {
        return true;
    }
    if (old_chunk.GetRenderObject() != new_chunk.GetRenderObject()) {
        return true;
    }

    return false;
}

} // namespace lightui
