/**
 * @file layerizer.cpp
 * @brief 层化器实现
 */

#include "core/compositor/property_tree/layerizer.h"

namespace mblink {

// =========================================================================
// 构造函数
// =========================================================================

Layerizer::Layerizer(const PropertyTrees& trees, const GeometryMapper& mapper)
    : trees_(trees)
    , mapper_(mapper) {
}

// =========================================================================
// 层化
// =========================================================================

std::vector<PendingLayer> Layerizer::Layerize(const PaintArtifact& artifact) {
    std::vector<PendingLayer> layers;
    
    const auto& chunks = artifact.GetPaintChunks();
    
    for (const auto& chunk : chunks) {
        // 确定合成类型
        CompositingType type = DetermineCompositingType(chunk);
        CompositingReasons reasons = GetCompositingReasons(chunk);
        
        // 如果需要独立层
        if (type != CompositingType::kNone) {
            PendingLayer layer;
            layer.SetState(chunk.GetState());
            layer.SetCompositingType(type);
            layer.SetCompositingReasons(reasons);
            layer.AddChunk(&chunk);
            layers.push_back(std::move(layer));
            continue;
        }
        
        // 检查是否与已有层重叠
        if (OverlapsWithExistingLayers(chunk, layers)) {
            // 因重叠需要独立层
            PendingLayer layer;
            layer.SetState(chunk.GetState());
            layer.SetCompositingType(CompositingType::kOverlap);
            layer.SetCompositingReasons(CompositingReasons::kOverlap);
            layer.AddChunk(&chunk);
            layers.push_back(std::move(layer));
            continue;
        }
        
        // 尝试合并到已有层
        if (layer_merging_enabled_ && TryMergeIntoExistingLayer(chunk, layers)) {
            continue;
        }
        
        // 创建新层
        PendingLayer layer;
        layer.SetState(chunk.GetState());
        layer.SetCompositingType(CompositingType::kNone);
        layer.AddChunk(&chunk);
        layers.push_back(std::move(layer));
    }
    
    return layers;
}

// =========================================================================
// 合成类型判断
// =========================================================================

CompositingType Layerizer::DetermineCompositingType(const PaintChunk& chunk) const {
    CompositingReasons reasons = GetCompositingReasons(chunk);
    
    // 检查是否是外部层
    if (HasCompositingReason(reasons, CompositingReasons::kVideo) ||
        HasCompositingReason(reasons, CompositingReasons::kCanvas) ||
        HasCompositingReason(reasons, CompositingReasons::kIFrame)) {
        return CompositingType::kForeignLayer;
    }
    
    // 检查是否是滚动条
    if (HasCompositingReason(reasons, CompositingReasons::kScrollbar)) {
        return CompositingType::kScrollbar;
    }
    
    // 检查是否有直接合成原因
    if (HasCompositingReason(reasons, CompositingReasons::kWillChangeTransform) ||
        HasCompositingReason(reasons, CompositingReasons::kWillChangeOpacity) ||
        HasCompositingReason(reasons, CompositingReasons::kActiveTransformAnimation) ||
        HasCompositingReason(reasons, CompositingReasons::kActiveOpacityAnimation) ||
        HasCompositingReason(reasons, CompositingReasons::kBackdropFilter) ||
        HasCompositingReason(reasons, CompositingReasons::k3DTransform) ||
        HasCompositingReason(reasons, CompositingReasons::kRoot)) {
        return CompositingType::kDirectReason;
    }
    
    return CompositingType::kNone;
}

CompositingReasons Layerizer::GetCompositingReasons(const PaintChunk& chunk) const {
    CompositingReasons reasons = chunk.GetCompositingReasons();
    
    // 从属性树状态获取额外的合成原因
    const PropertyTreeState& state = chunk.GetState();
    
    // 检查变换节点
    if (state.Transform()) {
        if (state.Transform()->CanDirectlyUpdate()) {
            reasons |= CompositingReasons::kWillChangeTransform;
        }
        if (state.Transform()->Is3DTransform()) {
            reasons |= CompositingReasons::k3DTransform;
        }
    }
    
    // 检查效果节点
    if (state.Effect()) {
        if (state.Effect()->CanDirectlyUpdateOpacity()) {
            reasons |= CompositingReasons::kWillChangeOpacity;
        }
        if (state.Effect()->HasBackdropFilter()) {
            reasons |= CompositingReasons::kBackdropFilter;
        }
    }
    
    return reasons;
}

// =========================================================================
// 重叠检测
// =========================================================================

bool Layerizer::OverlapsWithExistingLayers(
    const PaintChunk& chunk,
    const std::vector<PendingLayer>& layers) const {
    
    // 获取绘制块在根坐标系中的边界
    SkRect chunk_bounds = GetChunkBoundsInRootSpace(chunk);
    
    // 检查是否与任何需要独立层的层重叠
    for (const auto& layer : layers) {
        if (!layer.RequiresOwnLayer()) {
            continue;
        }
        
        if (layer.OverlapsWith(chunk_bounds)) {
            return true;
        }
    }
    
    return false;
}

SkRect Layerizer::GetChunkBoundsInRootSpace(const PaintChunk& chunk) const {
    const PropertyTreeState& state = chunk.GetState();
    SkRect bounds = chunk.GetBounds();
    
    // 将边界转换到根坐标系
    if (state.Transform()) {
        SkM44 transform = state.Transform()->GetAccumulatedTransform();
        
        // 转换四个角点
        SkV4 corners[4] = {
            {bounds.fLeft, bounds.fTop, 0.0f, 1.0f},
            {bounds.fRight, bounds.fTop, 0.0f, 1.0f},
            {bounds.fRight, bounds.fBottom, 0.0f, 1.0f},
            {bounds.fLeft, bounds.fBottom, 0.0f, 1.0f}
        };
        
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();
        
        for (int i = 0; i < 4; ++i) {
            SkV4 transformed = transform * corners[i];
            float x = transformed.w != 0 ? transformed.x / transformed.w : transformed.x;
            float y = transformed.w != 0 ? transformed.y / transformed.w : transformed.y;
            
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        }
        
        return SkRect::MakeLTRB(min_x, min_y, max_x, max_y);
    }
    
    return bounds;
}

// =========================================================================
// 层合并
// =========================================================================

bool Layerizer::TryMergeIntoExistingLayer(
    const PaintChunk& chunk,
    std::vector<PendingLayer>& layers) const {
    
    int index = FindMergeableLayer(chunk, layers);
    if (index < 0) {
        return false;
    }
    
    // 合并到找到的层
    layers[index].AddChunk(&chunk);
    return true;
}

int Layerizer::FindMergeableLayer(
    const PaintChunk& chunk,
    const std::vector<PendingLayer>& layers) const {
    
    // 从后向前查找（优先合并到最近的层）
    for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
        const PendingLayer& layer = layers[i];
        
        // 跳过需要独立层的层
        if (layer.RequiresOwnLayer()) {
            continue;
        }
        
        // 检查属性树状态是否兼容
        if (!layer.GetState().CanMergeWith(chunk.GetState())) {
            continue;
        }
        
        // 检查是否有强制合成原因
        if (RequiresOwnLayer(chunk.GetCompositingReasons())) {
            continue;
        }
        
        return i;
    }
    
    return -1;
}

} // namespace mblink
