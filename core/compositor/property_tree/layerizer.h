/**
 * @file layerizer.h
 * @brief 层化器
 *
 * 层化器将绘制块分配到合成层：
 * - 确定合成类型
 * - 检测重叠
 * - 合并可合并的层
 *
 * 参考 Chromium Blink: platform/graphics/compositing/paint_artifact_compositor.h
 */

#pragma once

#include "core/compositor/property_tree/paint/pending_layer.h"
#include "core/compositor/property_tree/paint/paint_artifact.h"
#include "core/compositor/property_tree/geometry_mapper.h"
#include <vector>

namespace mbink {

/**
 * @brief 层化器
 *
 * 将绘制块分配到合成层。
 */
class Layerizer {
public:
    /**
     * @brief 构造函数
     * @param trees 属性树集合
     * @param mapper 几何映射器
     */
    Layerizer(const PropertyTrees& trees, const GeometryMapper& mapper);
    
    ~Layerizer() = default;

    // 禁止拷贝
    Layerizer(const Layerizer&) = delete;
    Layerizer& operator=(const Layerizer&) = delete;

    // =========================================================================
    // 层化
    // =========================================================================

    /**
     * @brief 执行层化
     * @param artifact 绘制产物
     * @return 待定层列表
     */
    std::vector<PendingLayer> Layerize(const PaintArtifact& artifact);

    // =========================================================================
    // 配置
    // =========================================================================

    /**
     * @brief 设置是否启用层合并
     */
    void SetLayerMergingEnabled(bool enabled) { layer_merging_enabled_ = enabled; }

    /**
     * @brief 是否启用层合并
     */
    bool IsLayerMergingEnabled() const { return layer_merging_enabled_; }

private:
    // =========================================================================
    // 合成类型判断
    // =========================================================================

    /**
     * @brief 判断绘制块的合成类型
     * @param chunk 绘制块
     * @return 合成类型
     */
    CompositingType DetermineCompositingType(const PaintChunk& chunk) const;

    /**
     * @brief 获取绘制块的合成原因
     * @param chunk 绘制块
     * @return 合成原因
     */
    CompositingReasons GetCompositingReasons(const PaintChunk& chunk) const;

    // =========================================================================
    // 重叠检测
    // =========================================================================

    /**
     * @brief 检查绘制块是否与已有层重叠
     * @param chunk 绘制块
     * @param layers 已有层列表
     * @return true 如果重叠
     */
    bool OverlapsWithExistingLayers(
        const PaintChunk& chunk,
        const std::vector<PendingLayer>& layers) const;

    /**
     * @brief 获取绘制块在根坐标系中的边界
     * @param chunk 绘制块
     * @return 边界矩形
     */
    SkRect GetChunkBoundsInRootSpace(const PaintChunk& chunk) const;

    // =========================================================================
    // 层合并
    // =========================================================================

    /**
     * @brief 尝试合并到已有层
     * @param chunk 绘制块
     * @param layers 已有层列表
     * @return true 如果成功合并
     */
    bool TryMergeIntoExistingLayer(
        const PaintChunk& chunk,
        std::vector<PendingLayer>& layers) const;

    /**
     * @brief 查找可以合并的层
     * @param chunk 绘制块
     * @param layers 已有层列表
     * @return 可合并的层索引，-1 表示没有
     */
    int FindMergeableLayer(
        const PaintChunk& chunk,
        const std::vector<PendingLayer>& layers) const;

    // =========================================================================
    // 成员变量
    // =========================================================================

    const PropertyTrees& trees_;
    const GeometryMapper& mapper_;
    bool layer_merging_enabled_ = true;
};

} // namespace mbink
