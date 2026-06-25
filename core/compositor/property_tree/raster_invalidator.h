/**
 * @file raster_invalidator.h
 * @brief 光栅化失效器
 *
 * 光栅化失效器计算需要重新光栅化的区域：
 * - 出现/消失的绘制块
 * - 移动的绘制块
 * - 内容变化的绘制块
 *
 * 参考 Chromium Blink: platform/graphics/paint/raster_invalidator.h
 */

#pragma once

#include "core/compositor/property_tree/paint/paint_artifact.h"
#include "core/compositor/property_tree/geometry_mapper.h"
#include "include/core/SkRect.h"
#include <vector>

namespace mblink {

/**
 * @brief 失效结果
 */
struct InvalidationResult {
    std::vector<SkRect> rects;      // 失效区域列表
    bool full_invalidation = false;  // 是否需要完整重绘
    
    /**
     * @brief 是否有失效
     */
    bool HasInvalidation() const {
        return full_invalidation || !rects.empty();
    }
    
    /**
     * @brief 添加失效区域
     */
    void AddRect(const SkRect& rect) {
        if (!rect.isEmpty()) {
            rects.push_back(rect);
        }
    }
    
    /**
     * @brief 合并失效区域
     */
    void Merge(const InvalidationResult& other) {
        if (other.full_invalidation) {
            full_invalidation = true;
        }
        for (const auto& rect : other.rects) {
            rects.push_back(rect);
        }
    }
    
    /**
     * @brief 获取合并后的边界
     */
    SkRect GetUnionBounds() const {
        if (rects.empty()) {
            return SkRect::MakeEmpty();
        }
        SkRect result = rects[0];
        for (size_t i = 1; i < rects.size(); ++i) {
            result.join(rects[i]);
        }
        return result;
    }
};

/**
 * @brief 光栅化失效器
 *
 * 计算需要重新光栅化的区域。
 */
class RasterInvalidator {
public:
    /**
     * @brief 构造函数
     * @param mapper 几何映射器
     */
    explicit RasterInvalidator(const GeometryMapper& mapper);
    
    ~RasterInvalidator() = default;

    // 禁止拷贝
    RasterInvalidator(const RasterInvalidator&) = delete;
    RasterInvalidator& operator=(const RasterInvalidator&) = delete;

    // =========================================================================
    // 失效计算
    // =========================================================================

    /**
     * @brief 比较新旧绘制产物，计算失效区域
     * @param old_artifact 旧绘制产物
     * @param new_artifact 新绘制产物
     * @param layer_state 层的属性树状态
     * @return 失效结果
     */
    InvalidationResult ComputeInvalidation(
        const PaintArtifact& old_artifact,
        const PaintArtifact& new_artifact,
        const PropertyTreeState& layer_state);

    /**
     * @brief 比较新旧绘制块，计算失效区域
     * @param old_chunk 旧绘制块
     * @param new_chunk 新绘制块
     * @param layer_state 层的属性树状态
     * @return 失效结果
     */
    InvalidationResult ComputeChunkInvalidation(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state);

private:
    // =========================================================================
    // 失效处理
    // =========================================================================

    /**
     * @brief 处理出现的绘制块
     */
    void HandleAppearingChunk(
        const PaintChunk& chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);

    /**
     * @brief 处理消失的绘制块
     */
    void HandleDisappearingChunk(
        const PaintChunk& chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);

    /**
     * @brief 处理移动的绘制块
     */
    void HandleMovedChunk(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);

    /**
     * @brief 处理内容变化的绘制块
     */
    void HandleContentChangedChunk(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);

    // =========================================================================
    // 辅助方法
    // =========================================================================

    /**
     * @brief 将绘制块边界转换到层坐标系
     */
    SkRect MapChunkBoundsToLayerSpace(
        const PaintChunk& chunk,
        const PropertyTreeState& layer_state) const;

    /**
     * @brief 检查绘制块是否移动
     */
    bool ChunkHasMoved(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk) const;

    /**
     * @brief 检查绘制块内容是否变化
     */
    bool ChunkContentChanged(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk) const;

    // =========================================================================
    // 成员变量
    // =========================================================================

    const GeometryMapper& mapper_;
};

} // namespace mblink
