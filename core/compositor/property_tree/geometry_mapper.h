/**
 * @file geometry_mapper.h
 * @brief 几何映射器
 *
 * 在不同属性树状态之间转换坐标和区域：
 * - 点转换
 * - 矩形转换
 * - 可见区域计算
 * - 变换矩阵计算
 * - 裁剪区域计算
 *
 * 参考 Chromium Blink: platform/graphics/paint/geometry_mapper.h
 */

#pragma once

#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "include/core/SkM44.h"
#include "include/core/SkRect.h"
#include "include/core/SkPoint.h"
#include <unordered_map>

namespace mblink {

/**
 * @brief 几何映射器
 *
 * 在不同属性树状态之间转换坐标和区域。
 * 使用缓存优化重复计算。
 */
class GeometryMapper {
public:
    /**
     * @brief 构造函数
     * @param trees 属性树集合
     */
    explicit GeometryMapper(const PropertyTrees& trees);
    
    ~GeometryMapper() = default;

    // 禁止拷贝
    GeometryMapper(const GeometryMapper&) = delete;
    GeometryMapper& operator=(const GeometryMapper&) = delete;

    // =========================================================================
    // 点转换
    // =========================================================================

    /**
     * @brief 将点从源状态转换到目标状态
     * @param point 源坐标系中的点
     * @param source 源属性树状态
     * @param target 目标属性树状态
     * @return 目标坐标系中的点
     */
    SkPoint MapPoint(const SkPoint& point,
                     const PropertyTreeState& source,
                     const PropertyTreeState& target) const;

    // =========================================================================
    // 矩形转换
    // =========================================================================

    /**
     * @brief 将矩形从源状态转换到目标状态（只考虑变换）
     * @param rect 源坐标系中的矩形
     * @param source 源属性树状态
     * @param target 目标属性树状态
     * @return 目标坐标系中的矩形（变换后的包围盒）
     */
    SkRect MapRect(const SkRect& rect,
                   const PropertyTreeState& source,
                   const PropertyTreeState& target) const;

    /**
     * @brief 将矩形从源状态转换到目标状态（考虑变换和裁剪）
     * @param rect 源坐标系中的矩形
     * @param source 源属性树状态
     * @param target 目标属性树状态
     * @return 目标坐标系中的可见矩形
     */
    SkRect MapVisualRect(const SkRect& rect,
                         const PropertyTreeState& source,
                         const PropertyTreeState& target) const;

    // =========================================================================
    // 变换矩阵
    // =========================================================================

    /**
     * @brief 计算从源状态到目标状态的变换矩阵
     * @param source 源属性树状态
     * @param target 目标属性树状态
     * @return 变换矩阵
     */
    SkM44 GetTransformMatrix(const PropertyTreeState& source,
                             const PropertyTreeState& target) const;

    /**
     * @brief 计算从源变换节点到目标变换节点的变换矩阵
     * @param source 源变换节点
     * @param target 目标变换节点
     * @return 变换矩阵
     */
    SkM44 GetTransformMatrix(const TransformTreeNode* source,
                             const TransformTreeNode* target) const;

    // =========================================================================
    // 裁剪区域
    // =========================================================================

    /**
     * @brief 计算在目标状态中的裁剪区域
     * @param source 源属性树状态
     * @param target 目标属性树状态
     * @return 裁剪矩形
     */
    SkRect GetClipRect(const PropertyTreeState& source,
                       const PropertyTreeState& target) const;

    /**
     * @brief 检查点是否在裁剪区域内
     * @param point 点坐标
     * @param state 属性树状态
     * @return true 如果点在裁剪区域内
     */
    bool PointInClip(const SkPoint& point,
                     const PropertyTreeState& state) const;

    /**
     * @brief 检查矩形是否与裁剪区域相交
     * @param rect 矩形
     * @param state 属性树状态
     * @return true 如果矩形与裁剪区域相交
     */
    bool RectIntersectsClip(const SkRect& rect,
                            const PropertyTreeState& state) const;

    // =========================================================================
    // 缓存管理
    // =========================================================================

    /**
     * @brief 清除所有缓存
     */
    void ClearCache();

    /**
     * @brief 使指定节点相关的缓存失效
     * @param node_id 节点 ID
     */
    void InvalidateCacheForNode(PropertyTreeNodeId node_id);

private:
    // =========================================================================
    // 缓存键类型
    // =========================================================================

    /**
     * @brief 变换缓存键
     */
    struct TransformCacheKey {
        PropertyTreeNodeId source_transform;
        PropertyTreeNodeId target_transform;
        
        bool operator==(const TransformCacheKey& other) const {
            return source_transform == other.source_transform &&
                   target_transform == other.target_transform;
        }
    };

    /**
     * @brief 变换缓存键哈希
     */
    struct TransformCacheKeyHash {
        size_t operator()(const TransformCacheKey& key) const {
            return std::hash<PropertyTreeNodeId>()(key.source_transform) ^
                   (std::hash<PropertyTreeNodeId>()(key.target_transform) << 1);
        }
    };

    /**
     * @brief 裁剪缓存键
     */
    struct ClipCacheKey {
        PropertyTreeNodeId clip_node;
        PropertyTreeNodeId target_transform;
        
        bool operator==(const ClipCacheKey& other) const {
            return clip_node == other.clip_node &&
                   target_transform == other.target_transform;
        }
    };

    /**
     * @brief 裁剪缓存键哈希
     */
    struct ClipCacheKeyHash {
        size_t operator()(const ClipCacheKey& key) const {
            return std::hash<PropertyTreeNodeId>()(key.clip_node) ^
                   (std::hash<PropertyTreeNodeId>()(key.target_transform) << 1);
        }
    };

    // =========================================================================
    // 内部方法
    // =========================================================================

    /**
     * @brief 计算累积裁剪区域
     */
    SkRect ComputeAccumulatedClipRect(const ClipTreeNode* clip,
                                      const TransformTreeNode* target_transform) const;

    /**
     * @brief 将矩形通过变换矩阵转换
     */
    static SkRect TransformRect(const SkRect& rect, const SkM44& matrix);

    // =========================================================================
    // 成员变量
    // =========================================================================

    const PropertyTrees& trees_;

    // 变换矩阵缓存
    mutable std::unordered_map<TransformCacheKey, SkM44, TransformCacheKeyHash> 
        transform_cache_;

    // 裁剪区域缓存
    mutable std::unordered_map<ClipCacheKey, SkRect, ClipCacheKeyHash> 
        clip_cache_;
};

} // namespace mblink
