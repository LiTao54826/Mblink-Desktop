/**
 * @file geometry_mapper.cpp
 * @brief 几何映射器实现
 */

#include "core/compositor/property_tree/geometry_mapper.h"
#include <algorithm>
#include <cmath>

namespace mblink {

// =========================================================================
// 构造函数
// =========================================================================

GeometryMapper::GeometryMapper(const PropertyTrees& trees)
    : trees_(trees) {
}

// =========================================================================
// 点转换
// =========================================================================

SkPoint GeometryMapper::MapPoint(const SkPoint& point,
                                  const PropertyTreeState& source,
                                  const PropertyTreeState& target) const {
    if (source.Transform() == target.Transform()) {
        return point;
    }
    
    SkM44 matrix = GetTransformMatrix(source, target);
    
    // 将 2D 点转换为 4D 齐次坐标
    SkV4 p = {point.x(), point.y(), 0.0f, 1.0f};
    SkV4 result = matrix * p;
    
    // 透视除法
    if (std::abs(result.w) > 1e-6f) {
        return SkPoint::Make(result.x / result.w, result.y / result.w);
    }
    
    return SkPoint::Make(result.x, result.y);
}

// =========================================================================
// 矩形转换
// =========================================================================

SkRect GeometryMapper::MapRect(const SkRect& rect,
                                const PropertyTreeState& source,
                                const PropertyTreeState& target) const {
    if (source.Transform() == target.Transform()) {
        return rect;
    }
    
    SkM44 matrix = GetTransformMatrix(source, target);
    return TransformRect(rect, matrix);
}

SkRect GeometryMapper::MapVisualRect(const SkRect& rect,
                                      const PropertyTreeState& source,
                                      const PropertyTreeState& target) const {
    // 首先应用变换
    SkRect transformed = MapRect(rect, source, target);
    
    if (transformed.isEmpty()) {
        return SkRect::MakeEmpty();
    }
    
    // 然后应用裁剪
    SkRect clip_rect = GetClipRect(source, target);
    
    if (clip_rect.isEmpty()) {
        return SkRect::MakeEmpty();
    }
    
    // 计算交集
    SkRect result;
    if (!result.intersect(transformed, clip_rect)) {
        return SkRect::MakeEmpty();
    }
    
    return result;
}

// =========================================================================
// 变换矩阵
// =========================================================================

SkM44 GeometryMapper::GetTransformMatrix(const PropertyTreeState& source,
                                          const PropertyTreeState& target) const {
    return GetTransformMatrix(source.Transform(), target.Transform());
}

SkM44 GeometryMapper::GetTransformMatrix(const TransformTreeNode* source,
                                          const TransformTreeNode* target) const {
    // 相同节点，返回单位矩阵
    if (source == target) {
        return SkM44();
    }
    
    // 处理空节点
    if (!source && !target) {
        return SkM44();
    }
    
    // 获取节点 ID
    PropertyTreeNodeId source_id = source ? source->GetId() : kInvalidNodeId;
    PropertyTreeNodeId target_id = target ? target->GetId() : kInvalidNodeId;
    
    // 检查缓存
    TransformCacheKey key{source_id, target_id};
    auto it = transform_cache_.find(key);
    if (it != transform_cache_.end()) {
        return it->second;
    }
    
    // 计算变换矩阵
    SkM44 result;
    
    if (!target) {
        // 目标是根，返回源的累积变换
        if (source) {
            result = source->GetAccumulatedTransform();
        }
    } else if (!source) {
        // 源是根，返回目标累积变换的逆
        SkM44 target_accumulated = target->GetAccumulatedTransform();
        SkM44 inverse;
        if (target_accumulated.invert(&inverse)) {
            result = inverse;
        }
    } else {
        // 计算从源到目标的变换
        // result = target_accumulated^-1 * source_accumulated
        SkM44 source_accumulated = source->GetAccumulatedTransform();
        SkM44 target_accumulated = target->GetAccumulatedTransform();
        
        SkM44 target_inverse;
        if (target_accumulated.invert(&target_inverse)) {
            result = target_inverse * source_accumulated;
        } else {
            // 如果目标变换不可逆，只使用源变换
            result = source_accumulated;
        }
    }
    
    // 缓存结果
    transform_cache_[key] = result;
    
    return result;
}

// =========================================================================
// 裁剪区域
// =========================================================================

SkRect GeometryMapper::GetClipRect(const PropertyTreeState& source,
                                    const PropertyTreeState& target) const {
    // 如果没有裁剪节点，返回无限大的矩形
    if (!source.Clip()) {
        return SkRect::MakeLTRB(-1e9f, -1e9f, 1e9f, 1e9f);
    }
    
    TransformTreeNode* target_transform = target.Transform();
    return ComputeAccumulatedClipRect(source.Clip(), target_transform);
}

SkRect GeometryMapper::ComputeAccumulatedClipRect(const ClipTreeNode* clip,
                                                   const TransformTreeNode* target_transform) const {
    if (!clip) {
        return SkRect::MakeLTRB(-1e9f, -1e9f, 1e9f, 1e9f);
    }
    
    // 检查缓存
    ClipCacheKey key{
        clip->GetId(),
        target_transform ? target_transform->GetId() : kInvalidNodeId
    };
    auto it = clip_cache_.find(key);
    if (it != clip_cache_.end()) {
        return it->second;
    }
    
    // 获取当前裁剪节点的裁剪矩形
    SkRect local_clip = clip->GetClipRect();
    
    // 将裁剪矩形转换到目标坐标系
    TransformTreeNode* clip_transform = clip->GetTransformNode();
    SkM44 transform = GetTransformMatrix(clip_transform, target_transform);
    SkRect transformed_clip = TransformRect(local_clip, transform);
    
    // 递归计算父裁剪节点的累积裁剪
    ClipTreeNode* parent = clip->GetParent();
    if (parent) {
        SkRect parent_clip = ComputeAccumulatedClipRect(parent, target_transform);
        
        // 计算交集
        SkRect result;
        if (!result.intersect(transformed_clip, parent_clip)) {
            result = SkRect::MakeEmpty();
        }
        
        // 缓存结果
        clip_cache_[key] = result;
        return result;
    }
    
    // 缓存结果
    clip_cache_[key] = transformed_clip;
    return transformed_clip;
}

bool GeometryMapper::PointInClip(const SkPoint& point,
                                  const PropertyTreeState& state) const {
    if (!state.Clip()) {
        return true;
    }
    
    // 获取累积裁剪区域
    SkRect clip_rect = ComputeAccumulatedClipRect(state.Clip(), state.Transform());
    
    return clip_rect.contains(point.x(), point.y());
}

bool GeometryMapper::RectIntersectsClip(const SkRect& rect,
                                         const PropertyTreeState& state) const {
    if (!state.Clip()) {
        return true;
    }
    
    // 获取累积裁剪区域
    SkRect clip_rect = ComputeAccumulatedClipRect(state.Clip(), state.Transform());
    
    return SkRect::Intersects(rect, clip_rect);
}

// =========================================================================
// 缓存管理
// =========================================================================

void GeometryMapper::ClearCache() {
    transform_cache_.clear();
    clip_cache_.clear();
}

void GeometryMapper::InvalidateCacheForNode(PropertyTreeNodeId node_id) {
    // 移除所有涉及该节点的变换缓存
    for (auto it = transform_cache_.begin(); it != transform_cache_.end();) {
        if (it->first.source_transform == node_id || 
            it->first.target_transform == node_id) {
            it = transform_cache_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 移除所有涉及该节点的裁剪缓存
    for (auto it = clip_cache_.begin(); it != clip_cache_.end();) {
        if (it->first.clip_node == node_id || 
            it->first.target_transform == node_id) {
            it = clip_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

// =========================================================================
// 静态辅助方法
// =========================================================================

SkRect GeometryMapper::TransformRect(const SkRect& rect, const SkM44& matrix) {
    if (rect.isEmpty()) {
        return SkRect::MakeEmpty();
    }
    
    // 检查是否是单位矩阵
    if (matrix == SkM44()) {
        return rect;
    }
    
    // 转换四个角点
    SkV4 corners[4] = {
        {rect.fLeft, rect.fTop, 0.0f, 1.0f},
        {rect.fRight, rect.fTop, 0.0f, 1.0f},
        {rect.fRight, rect.fBottom, 0.0f, 1.0f},
        {rect.fLeft, rect.fBottom, 0.0f, 1.0f}
    };
    
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    
    for (int i = 0; i < 4; ++i) {
        SkV4 transformed = matrix * corners[i];
        
        // 透视除法
        float x, y;
        if (std::abs(transformed.w) > 1e-6f) {
            x = transformed.x / transformed.w;
            y = transformed.y / transformed.w;
        } else {
            x = transformed.x;
            y = transformed.y;
        }
        
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }
    
    return SkRect::MakeLTRB(min_x, min_y, max_x, max_y);
}

} // namespace mblink
