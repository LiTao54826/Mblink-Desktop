/**
 * @file render_pipeline.h
 * @brief 统一渲染管线
 *
 * 合并 RenderPipelineLegacy (V1) 和 RenderPipelineV2 的功能：
 * - V1: DOM 同步、样式计算、布局
 * - V2: 层树构建、光栅化、GPU 合成、动画优化、滚动优化
 *
 * 设计原则：
 * - 复用已验证的代码，不重新实现
 * - 统一的渲染入口
 * - 完整的渲染流程
 */

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>

// 复用 V2 的组件头文件
#include "core/compositor/compositor_layer.h"
#include "core/compositor/layer_tree_builder.h"
#include "core/compositor/layer_tree_manager.h"
#include "core/compositor/rasterizer.h"
#include "core/compositor/compositor.h"
#include "core/compositor/animation/animation_layer_bridge.h"
#include "core/compositor/scroll_layer_manager.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_builder.h"
#include "core/compositor/property_tree/paint/paint_artifact_compositor.h"

// Skia 前向声明
class SkCanvas;
struct SkRect;

namespace mbink {

// 前向声明
class Document;
class DirtyNodeTracker;
class RenderObject;
class RenderTreeBuilder;
class RenderTreeSynchronizer;
class NativeLayoutEngine;

/**
 * @brief 渲染生命周期阶段
 * 
 * 合并 V1 的 RenderLifecycleLegacy 和 V2 的阶段
 */
enum class RenderStage {
    Idle,           ///< 空闲状态
    DOMSync,        ///< DOM 同步（来自 V1）
    StyleRecalc,    ///< 样式重算（来自 V1）
    Layout,         ///< 布局计算（来自 V1）
    LayerTreeBuild, ///< 层树构建（来自 V2）
    Rasterize,      ///< 光栅化（来自 V2）
    Composite       ///< 合成（来自 V2）
};

/**
 * @brief 统一渲染管线配置
 * 
 * 直接复制自 V2 的 RenderPipelineConfig
 */
struct UnifiedPipelineConfig {
    bool enable_gpu_compositing = true;       ///< 启用 GPU 合成
    bool enable_layer_promotion = true;       ///< 启用层提升
    bool enable_incremental_rasterize = true; ///< 启用增量光栅化
    bool enable_scroll_optimization = true;   ///< 启用滚动优化
    bool enable_animation_optimization = true;///< 启用动画优化
    bool enable_frame_skip = true;            ///< 启用帧跳过
    bool enable_property_trees = true;        ///< 启用属性树系统
    bool enable_incremental_layer_tree = true;   ///< 启用增量层树更新
    bool show_layer_borders = false;          ///< 显示层边界（调试）
};

/**
 * @brief 帧统计信息
 * 
 * 合并 V1 和 V2 的统计信息
 */
struct UnifiedFrameStats {
    // 各阶段耗时（毫秒）
    double dom_sync_time = 0.0;
    double style_time = 0.0;
    double layout_time = 0.0;
    double layer_tree_time = 0.0;
    double rasterize_time = 0.0;
    double composite_time = 0.0;
    double total_time = 0.0;
    
    // 计数统计
    int dirty_nodes = 0;
    int layers_built = 0;
    int layers_rasterized = 0;
    int layers_composited = 0;
    int dirty_regions_count = 0;
    
    // 增量样式重算统计
    int style_nodes_visited = 0;
    int style_nodes_recalculated = 0;
    int style_subtrees_skipped = 0;
    
    // 增量布局统计
    int layout_dirty_nodes = 0;
    bool layout_performed = false;
    
    // 增量更新优化统计 (Phase 7)
    int text_changes_count = 0;           // 文本变化数量
    int structural_changes_count = 0;     // 结构变化数量
    int paint_only_changes_count = 0;     // 仅绘制变化数量
    bool used_incremental_update = false; // 是否使用了增量更新
    double optimization_ratio = 0.0;      // 优化比率 (跳过的节点 / 总节点)
    
    // 状态
    bool frame_skipped = false;
    bool using_gpu = false;
    
    void Reset() {
        dom_sync_time = 0.0;
        style_time = 0.0;
        layout_time = 0.0;
        layer_tree_time = 0.0;
        rasterize_time = 0.0;
        composite_time = 0.0;
        total_time = 0.0;
        dirty_nodes = 0;
        layers_built = 0;
        layers_rasterized = 0;
        layers_composited = 0;
        dirty_regions_count = 0;
        style_nodes_visited = 0;
        style_nodes_recalculated = 0;
        style_subtrees_skipped = 0;
        layout_dirty_nodes = 0;
        layout_performed = false;
        text_changes_count = 0;
        structural_changes_count = 0;
        paint_only_changes_count = 0;
        used_incremental_update = false;
        optimization_ratio = 0.0;
        frame_skipped = false;
        using_gpu = false;
    }
};

/**
 * @brief 统一渲染管线
 *
 * 合并 RenderPipelineLegacy (V1) 和 RenderPipelineV2 的所有功能。
 * 提供完整的渲染流程：DOM同步 → 样式 → 布局 → 层树 → 光栅化 → 合成
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    // 禁止拷贝
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // =========================================================================
    // 初始化（来自 V2）
    // =========================================================================

    bool Initialize(int width, int height, 
                    const UnifiedPipelineConfig& config = {});
    void Shutdown();
    void Resize(int width, int height);
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // 配置
    // =========================================================================

    void SetDocument(std::shared_ptr<Document> doc);
    void SetConfig(const UnifiedPipelineConfig& config);
    const UnifiedPipelineConfig& GetConfig() const { return config_; }
    void SetDpiScale(float scale);
    float GetDpiScale() const { return dpi_scale_; }

    // =========================================================================
    // 主渲染入口
    // =========================================================================

    /**
     * @brief 处理一帧（主入口）
     * 
     * 完整流程：
     * 1. DOM 同步（来自 V1）
     * 2. 样式重算（来自 V1）
     * 3. 布局计算（来自 V1）
     * 4. 层树构建（来自 V2）
     * 5. 光栅化（来自 V2）
     * 6. 合成（来自 V2）
     */
    bool ProcessFrame(SkCanvas* canvas);
    
    bool NeedsUpdate() const;
    void MarkNeedsRender() { needs_render_ = true; }

    // =========================================================================
    // 脏标记（来自 V1）
    // =========================================================================

    void MarkNeedsStyleRecalc();
    void MarkNeedsLayout();
    void MarkNeedsPaint();
    void ForceFullUpdate();

    // =========================================================================
    // 层树管理（来自 V2）
    // =========================================================================

    void InvalidateLayerTree();
    void ForceRasterize();
    void MarkDirty(RenderObject* object);
    void MarkDirtyRegion(const SkRect& region);

    // =========================================================================
    // 滚动处理（来自 V2）
    // =========================================================================

    bool HandleScroll(RenderObject* container, float delta_x, float delta_y);
    bool ScrollTo(RenderObject* container, float scroll_x, float scroll_y);

    // =========================================================================
    // 动画处理（来自 V2）
    // =========================================================================

    void BeginAnimationFrame();
    AnimationUpdateType UpdateAnimationProperty(RenderObject* object,
                                                 const std::string& property,
                                                 const std::string& value);
    bool EndAnimationFrame();
    void OnAnimationStart(RenderObject* object,
                          const std::string& animation_name,
                          const std::vector<std::string>& properties);
    void OnAnimationEnd(RenderObject* object, const std::string& animation_name);

    // =========================================================================
    // 状态查询
    // =========================================================================

    RenderStage GetCurrentStage() const { return current_stage_; }
    const UnifiedFrameStats& GetLastFrameStats() const { return last_frame_stats_; }
    std::shared_ptr<RenderObject> GetRenderTree() const { return render_tree_; }
    void SetRenderTree(std::shared_ptr<RenderObject> tree);
    std::shared_ptr<CompositorLayer> GetRootLayer() const { return root_layer_; }

    // =========================================================================
    // 属性树系统访问（来自 V2）
    // =========================================================================

    bool IsUsingPropertyTreeSystem() const { return config_.enable_property_trees; }
    PropertyTrees* GetPropertyTrees() { return property_trees_.get(); }
    PropertyTreeBuilder* GetPropertyTreeBuilder() { return property_tree_builder_.get(); }
    PaintArtifactCompositor* GetPaintArtifactCompositor() { return paint_artifact_compositor_.get(); }

    // =========================================================================
    // 组件访问（调试用，来自 V2）
    // =========================================================================

    LayerTreeBuilder* GetLayerTreeBuilder() { return layer_tree_builder_.get(); }
    Rasterizer* GetRasterizer() { return rasterizer_.get(); }
    Compositor* GetCompositor() { return compositor_.get(); }
    AnimationLayerBridge* GetAnimationBridge() { return animation_bridge_.get(); }
    ScrollLayerManager* GetScrollManager() { return scroll_manager_.get(); }
    LayerTreeManager* GetLayerTreeManager() { return layer_tree_manager_.get(); }
    void SetShowLayerBorders(bool show);

private:
    // =========================================================================
    // 渲染阶段实现
    // =========================================================================

    void DoDOMSync();       // 来自 V1
    void DoStyleRecalc();   // 来自 V1
    void DoLayout();        // 来自 V1
    void DoLayerTreeBuild();// 来自 V2
    void DoRasterize();     // 来自 V2
    void DoComposite(SkCanvas* canvas); // 来自 V2

    // =========================================================================
    // 辅助方法（来自 V2）
    // =========================================================================

    void UpdateLayerTreeBounds(CompositorLayer* layer);
    void CollectDirtyRectsForLayer(RenderObject* obj, CompositorLayer* layer);
    void RegisterScrollableElements(RenderObject* root);
    void RegisterScrollableElementsRecursive(RenderObject* obj);
    bool CheckRenderObjectNeedsPaint(RenderObject* obj);

    // 检测并为新添加的元素创建层（增量更新优化）
    void DetectAndCreateNewLayers(RenderObject* root);
    void DetectAndCreateNewLayersRecursive(RenderObject* obj);

    // 检测并删除孤立层（对应的 RenderObject 已被删除）
    void RemoveOrphanedLayers(CompositorLayer* layer);

    // 标记所有层为脏（完整重建后使用）
    void MarkAllLayersDirty(CompositorLayer* layer);

    // 🐛 修复：完整重建后恢复所有可滚动层的滚动偏移
    void RestoreScrollOffsetsAfterRebuild(CompositorLayer* layer);

    // =========================================================================
    // 辅助方法（来自 V1）
    // =========================================================================

    void EnsureRenderTree();

private:
    // =========================================================================
    // 状态
    // =========================================================================

    bool initialized_ = false;
    RenderStage current_stage_ = RenderStage::Idle;
    UnifiedPipelineConfig config_;

    // 脏标记（合并 V1 和 V2）
    bool needs_style_recalc_ = false;  // V1
    bool needs_layout_ = false;         // V1
    bool needs_paint_ = false;          // V1
    bool needs_render_ = true;          // V2
    bool needs_layer_tree_rebuild_ = true; // V2

    // 视口
    int viewport_width_ = 0;
    int viewport_height_ = 0;
    float dpi_scale_ = 1.0f;

    // =========================================================================
    // 文档和渲染树
    // =========================================================================

    std::weak_ptr<Document> document_;
    std::shared_ptr<RenderObject> render_tree_;

    // =========================================================================
    // 来自 V1 的组件
    // =========================================================================

    std::shared_ptr<RenderTreeBuilder> render_tree_builder_;
    std::shared_ptr<RenderTreeSynchronizer> synchronizer_;
    NativeLayoutEngine* layout_engine_ = nullptr;  // 外部持有，不拥有

    // =========================================================================
    // 来自 V2 的组件
    // =========================================================================

    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<LayerTreeManager> layer_tree_manager_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;
    std::unique_ptr<AnimationLayerBridge> animation_bridge_;
    std::unique_ptr<ScrollLayerManager> scroll_manager_;

    // 属性树系统
    std::unique_ptr<PropertyTrees> property_trees_;
    std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
    std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;

    // 层树
    std::shared_ptr<CompositorLayer> root_layer_;

    // =========================================================================
    // 统计
    // =========================================================================

    UnifiedFrameStats last_frame_stats_;
    UnifiedFrameStats current_frame_stats_;
    double frame_start_time_ = 0.0;
};

} // namespace mbink
