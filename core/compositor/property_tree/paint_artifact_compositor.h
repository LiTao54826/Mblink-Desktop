/**
 * @file paint_artifact_compositor.h
 * @brief 绘制产物合成器 - 将 PaintArtifact 转换为 CompositorLayer 列表
 *
 * PaintArtifactCompositor 负责：
 * - 将 PaintArtifact 转换为合成层
 * - 支持直接属性更新（transform/opacity/scroll）
 * - 管理层的创建、更新和销毁
 * - 支持快速路径更新（不重建层结构）
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "core/compositor/property_tree/paint_artifact.h"
#include "core/compositor/property_tree/pending_layer.h"
#include "core/compositor/property_tree/layerizer.h"
#include "core/compositor/property_tree/raster_invalidator.h"
#include "core/compositor/property_tree/geometry_mapper.h"
#include "include/core/SkM44.h"
#include "include/core/SkPoint.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"

namespace lightui {

// 前向声明
class CompositorLayer;
class RenderObject;

/**
 * @brief 更新类型
 */
enum class UpdateType {
    kNone,              // 无更新
    kDirectUpdate,      // 直接属性更新（不触发光栅化）
    kRepaint,           // 重绘更新（触发增量光栅化）
    kFullUpdate         // 完整更新（重建层结构）
};

/**
 * @brief 绘制产物合成器
 *
 * 将 PaintArtifact 转换为 CompositorLayer 列表，
 * 支持直接属性更新以实现高效动画。
 */
class PaintArtifactCompositor {
public:
    PaintArtifactCompositor();
    ~PaintArtifactCompositor();

    // 禁止拷贝
    PaintArtifactCompositor(const PaintArtifactCompositor&) = delete;
    PaintArtifactCompositor& operator=(const PaintArtifactCompositor&) = delete;

    // =========================================================================
    // 初始化和配置
    // =========================================================================

    /**
     * @brief 设置属性树集合
     */
    void SetPropertyTrees(PropertyTrees* trees);

    /**
     * @brief 获取属性树集合
     */
    PropertyTrees* GetPropertyTrees() const { return property_trees_; }

    // =========================================================================
    // 更新方法
    // =========================================================================

    /**
     * @brief 完整更新
     * @param artifact 新的绘制产物
     * @return 更新类型
     *
     * 执行完整的层化和合成流程：
     * 1. 层化 PaintArtifact
     * 2. 创建/更新 CompositorLayer
     * 3. 计算光栅化失效区域
     */
    UpdateType Update(const PaintArtifact& artifact);

    /**
     * @brief 尝试快速路径更新
     * @param artifact 新的绘制产物
     * @return true 如果快速路径成功，false 需要完整更新
     *
     * 快速路径条件：
     * - 层结构未变化
     * - 只有属性树节点值变化
     */
    bool TryFastPathUpdate(const PaintArtifact& artifact);

    /**
     * @brief 检查是否需要完整更新
     */
    bool NeedsFullUpdate() const { return needs_full_update_; }

    /**
     * @brief 标记需要完整更新
     */
    void SetNeedsFullUpdate() { needs_full_update_ = true; }

    // =========================================================================
    // 直接属性更新（不触发光栅化）
    // =========================================================================

    /**
     * @brief 直接更新变换
     * @param transform_node 变换节点
     * @param new_matrix 新变换矩阵
     * @return true 如果更新成功
     *
     * 只更新属性树节点和关联层的变换，不触发光栅化。
     */
    bool DirectlyUpdateTransform(TransformTreeNode* transform_node,
                                  const SkM44& new_matrix);

    /**
     * @brief 直接更新透明度
     * @param effect_node 效果节点
     * @param new_opacity 新透明度
     * @return true 如果更新成功
     *
     * 只更新属性树节点和关联层的透明度，不触发光栅化。
     */
    bool DirectlyUpdateOpacity(EffectTreeNode* effect_node,
                                float new_opacity);

    /**
     * @brief 直接更新滚动偏移
     * @param scroll_node 滚动节点
     * @param new_offset 新滚动偏移
     * @return true 如果更新成功
     *
     * 只更新属性树节点和关联层的滚动偏移，不触发光栅化。
     */
    bool DirectlyUpdateScrollOffset(ScrollTreeNode* scroll_node,
                                     const SkPoint& new_offset);

    /**
     * @brief 检查变换节点是否可以直接更新
     */
    bool CanDirectlyUpdateTransform(const TransformTreeNode* node) const;

    /**
     * @brief 检查效果节点是否可以直接更新透明度
     */
    bool CanDirectlyUpdateOpacity(const EffectTreeNode* node) const;

    /**
     * @brief 检查滚动节点是否可以直接更新
     */
    bool CanDirectlyUpdateScrollOffset(const ScrollTreeNode* node) const;

    // =========================================================================
    // 层管理
    // =========================================================================

    /**
     * @brief 获取所有合成层
     */
    const std::vector<std::shared_ptr<CompositorLayer>>& GetLayers() const { 
        return layers_; 
    }

    /**
     * @brief 获取层数量
     */
    size_t GetLayerCount() const { return layers_.size(); }

    /**
     * @brief 通过 RenderObject 查找层
     */
    std::shared_ptr<CompositorLayer> GetLayerForRenderObject(
        RenderObject* obj) const;

    /**
     * @brief 通过属性树状态查找层
     */
    std::shared_ptr<CompositorLayer> GetLayerForState(
        const PropertyTreeState& state) const;

    /**
     * @brief 清除所有层
     */
    void ClearLayers();

    // =========================================================================
    // 光栅化和合成
    // =========================================================================

    /**
     * @brief 获取需要光栅化的层
     */
    std::vector<CompositorLayer*> GetLayersNeedingRasterization() const;

    /**
     * @brief 获取上次更新类型
     */
    UpdateType GetLastUpdateType() const { return last_update_type_; }

    /**
     * @brief 检查是否有任何层需要光栅化
     */
    bool HasLayersNeedingRasterization() const;

    /**
     * @brief 光栅化脏层
     * @param canvas 用于光栅化的 Canvas（可选，如果为 nullptr 则使用层自己的 Canvas）
     * @return 光栅化的层数
     */
    int RasterizeDirtyLayers(SkCanvas* canvas = nullptr);

    /**
     * @brief 合成到 Canvas（CPU 路径）
     * @param canvas 目标 Canvas
     * @param viewport 视口区域
     */
    void CompositeToCanvas(SkCanvas* canvas, const SkRect& viewport);

    /**
     * @brief 合成到 GPU（GPU 路径）
     * @return true 如果合成成功
     */
    bool CompositeToGPU();

    /**
     * @brief 上传脏纹理到 GPU
     * @return 上传的层数
     */
    int UploadDirtyTextures();

    // =========================================================================
    // 调试支持
    // =========================================================================

    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        size_t total_layers = 0;
        size_t layers_needing_raster = 0;
        size_t direct_updates = 0;
        size_t full_updates = 0;
        size_t fast_path_updates = 0;
    };
    Statistics GetStatistics() const { return statistics_; }

    /**
     * @brief 重置统计信息
     */
    void ResetStatistics();

private:
    // =========================================================================
    // 内部方法
    // =========================================================================

    /**
     * @brief 从 PendingLayer 创建 CompositorLayer
     */
    std::shared_ptr<CompositorLayer> CreateLayerFromPendingLayer(
        const PendingLayer& pending_layer);

    /**
     * @brief 更新已有层
     */
    void UpdateExistingLayer(CompositorLayer* layer,
                             const PendingLayer& pending_layer);

    /**
     * @brief 匹配新旧层
     */
    void MatchLayers(const std::vector<PendingLayer>& new_pending_layers);

    /**
     * @brief 计算层的光栅化失效区域
     */
    void ComputeLayerInvalidation(CompositorLayer* layer,
                                   const PendingLayer& pending_layer);

    /**
     * @brief 查找关联的层
     */
    CompositorLayer* FindLayerForTransformNode(
        const TransformTreeNode* node) const;
    CompositorLayer* FindLayerForEffectNode(
        const EffectTreeNode* node) const;
    CompositorLayer* FindLayerForScrollNode(
        const ScrollTreeNode* node) const;

    /**
     * @brief 光栅化层的绘制块
     */
    void RasterizeLayerChunks(CompositorLayer* layer);

    // =========================================================================
    // 成员变量
    // =========================================================================

    // 属性树集合
    PropertyTrees* property_trees_ = nullptr;

    // 几何映射器
    std::unique_ptr<GeometryMapper> geometry_mapper_;

    // 层化器
    std::unique_ptr<Layerizer> layerizer_;

    // 光栅化失效器
    std::unique_ptr<RasterInvalidator> raster_invalidator_;

    // 合成层列表
    std::vector<std::shared_ptr<CompositorLayer>> layers_;

    // 上一帧的绘制产物（用于比较）
    std::unique_ptr<PaintArtifact> previous_artifact_;

    // 上一帧的待定层（用于匹配）
    std::vector<PendingLayer> previous_pending_layers_;

    // 状态标志
    bool needs_full_update_ = true;
    UpdateType last_update_type_ = UpdateType::kNone;

    // 节点到层的映射
    std::unordered_map<const TransformTreeNode*, CompositorLayer*> transform_to_layer_;
    std::unordered_map<const EffectTreeNode*, CompositorLayer*> effect_to_layer_;
    std::unordered_map<const ScrollTreeNode*, CompositorLayer*> scroll_to_layer_;

    // 统计信息
    mutable Statistics statistics_;
};

} // namespace lightui
