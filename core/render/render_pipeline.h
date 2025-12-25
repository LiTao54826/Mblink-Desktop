/**
 * @file render_pipeline.h
 * @brief 统一渲染管线
 *
 * 整合完整渲染流程：
 * - DOM 同步、样式计算、布局（来自原 V1）
 * - 层树构建、光栅化、GPU 合成（来自原 V2）
 *
 * 设计参考：
 * - Chromium Blink 的 DocumentLifecycle
 * - Chromium CC 的合成器架构
 * - Flutter 的渲染管线
 */

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>

// Skia 前向声明
class SkCanvas;
class SkM44;
class SkPoint;
class SkRect;

namespace lightui {

// 前向声明
class Document;
class DirtyNodeTracker;
class RenderObject;
class RenderTreeBuilder;
class RenderTreeSynchronizer;
class NativeLayoutEngine;
class LayoutEngine;
class LayerTreeBuilder;
class Rasterizer;
class Compositor;
class CompositorLayer;
class AnimationLayerBridge;
class ScrollLayerManager;
class PropertyTrees;
class PropertyTreeBuilder;
class PaintArtifactCompositor;
enum class AnimationUpdateType;

/**
 * @brief 渲染生命周期阶段
 */
enum class RenderStage {
    Idle,           ///< 空闲状态
    DOMSync,        ///< DOM 同步
    Style,          ///< 样式计算
    Layout,         ///< 布局计算
    LayerTree,      ///< 层树构建
    Rasterize,      ///< 光栅化
    Composite       ///< 合成
};

/**
 * @brief 渲染管线配置
 */
struct RenderPipelineConfig {
    // 功能开关
    bool enable_gpu_compositing = true;       ///< 启用 GPU 合成
    bool enable_layer_promotion = true;       ///< 启用层提升
    bool enable_incremental_rasterize = true; ///< 启用增量光栅化
    bool enable_scroll_optimization = true;   ///< 启用滚动优化
    bool enable_animation_optimization = true;///< 启用动画优化
    bool enable_frame_skip = true;            ///< 启用帧跳过
    bool enable_property_trees = true;        ///< 启用属性树系统

    // 调试选项
    bool show_layer_borders = false;          ///< 显示层边界
    bool show_dirty_regions = false;          ///< 显示脏区域
    bool enable_stats = true;                 ///< 启用统计
};

/**
 * @brief 帧统计信息
 */
struct FrameStats {
    // 时间统计（毫秒）
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
    int dirty_regions_count = 0;

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
        dirty_regions_count = 0;
        frame_skipped = false;
    }
};

/**
 * @brief 统一渲染管线
 *
 * 管理完整的渲染生命周期，从 DOM 变化到屏幕显示。
 * 整合了原 RenderPipeline (V1) 和 RenderPipelineV2 的功能。
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    // 禁止拷贝
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // =========================================================================
    // 初始化
    // =========================================================================

    /**
     * @brief 初始化渲染管线
     * @param width 视口宽度
     * @param height 视口高度
     * @param config 配置选项
     * @return true 如果初始化成功
     */
    bool Initialize(int width, int height,
                    const RenderPipelineConfig& config = {});

    /**
     * @brief 关闭渲染管线
     */
    void Shutdown();

    /**
     * @brief 调整视口大小
     * @param width 新宽度
     * @param height 新高度
     */
    void Resize(int width, int height);

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // 配置
    // =========================================================================

    /**
     * @brief 设置文档
     * @param doc 文档对象
     */
    void SetDocument(std::shared_ptr<Document> doc);

    /**
     * @brief 设置配置
     * @param config 配置选项
     */
    void SetConfig(const RenderPipelineConfig& config);

    /**
     * @brief 获取当前配置
     */
    const RenderPipelineConfig& GetConfig() const { return config_; }

    /**
     * @brief 设置 DPI 缩放比
     * @param scale DPI 缩放比
     */
    void SetDpiScale(float scale);

    /**
     * @brief 获取 DPI 缩放比
     */
    float GetDpiScale() const { return dpi_scale_; }

    // =========================================================================
    // 渲染
    // =========================================================================

    /**
     * @brief 处理一帧（主入口）
     *
     * 执行完整渲染流程：
     * 1. DOM 同步（如果有变化）
     * 2. 样式计算（如果需要）
     * 3. 布局（如果需要）
     * 4. 层树构建/更新
     * 5. 光栅化脏层
     * 6. 合成到屏幕
     *
     * @param canvas 目标 Canvas
     * @return true 如果渲染成功
     */
    bool ProcessFrame(SkCanvas* canvas);

    /**
     * @brief 检查是否需要更新
     */
    bool NeedsUpdate() const;

    /**
     * @brief 标记需要渲染
     */
    void MarkNeedsRender() { needs_paint_ = true; }

    // =========================================================================
    // 脏标记
    // =========================================================================

    /**
     * @brief 标记需要样式重算
     */
    void MarkNeedsStyleRecalc();

    /**
     * @brief 标记需要布局
     */
    void MarkNeedsLayout();

    /**
     * @brief 标记需要绘制
     */
    void MarkNeedsPaint();

    /**
     * @brief 标记需要重建层树
     */
    void MarkNeedsLayerTreeRebuild();

    /**
     * @brief 强制完整更新
     */
    void ForceFullUpdate();

    /**
     * @brief 使层树无效
     *
     * 当渲染树被重建时调用，确保层树中的指针不会悬空
     */
    void InvalidateLayerTree();

    /**
     * @brief 强制重新光栅化所有层
     */
    void ForceRasterize();

    // =========================================================================
    // 滚动处理
    // =========================================================================

    /**
     * @brief 处理滚动事件
     * @param container 滚动容器
     * @param delta_x X 方向滚动增量
     * @param delta_y Y 方向滚动增量
     * @return true 如果滚动成功
     */
    bool HandleScroll(RenderObject* container, float delta_x, float delta_y);

    /**
     * @brief 滚动到指定位置
     * @param container 滚动容器
     * @param scroll_x 目标 X 滚动位置
     * @param scroll_y 目标 Y 滚动位置
     * @return true 如果滚动成功
     */
    bool ScrollTo(RenderObject* container, float scroll_x, float scroll_y);

    // =========================================================================
    // 动画处理
    // =========================================================================

    /**
     * @brief 开始动画帧
     */
    void BeginAnimationFrame();

    /**
     * @brief 更新动画属性
     * @param object 渲染对象
     * @param property 属性名
     * @param value 属性值
     * @return 更新类型
     */
    AnimationUpdateType UpdateAnimationProperty(RenderObject* object,
                                                 const std::string& property,
                                                 const std::string& value);

    /**
     * @brief 结束动画帧
     * @return true 如果有层属性更新
     */
    bool EndAnimationFrame();

    /**
     * @brief 通知动画开始
     */
    void OnAnimationStart(RenderObject* object,
                          const std::string& animation_name,
                          const std::vector<std::string>& properties);

    /**
     * @brief 通知动画结束
     */
    void OnAnimationEnd(RenderObject* object, const std::string& animation_name);

    // =========================================================================
    // 属性树直接更新
    // =========================================================================

    /**
     * @brief 直接更新变换（不触发光栅化）
     */
    bool DirectlyUpdateTransform(RenderObject* object, const SkM44& matrix);

    /**
     * @brief 直接更新透明度（不触发光栅化）
     */
    bool DirectlyUpdateOpacity(RenderObject* object, float opacity);

    /**
     * @brief 直接更新滚动偏移（不触发光栅化）
     */
    bool DirectlyUpdateScrollOffset(RenderObject* object, const SkPoint& offset);

    // =========================================================================
    // 脏区域管理
    // =========================================================================

    /**
     * @brief 标记渲染对象为脏
     */
    void MarkDirty(RenderObject* object);

    /**
     * @brief 标记区域为脏
     */
    void MarkDirtyRegion(const SkRect& region);

    // =========================================================================
    // 状态查询
    // =========================================================================

    /**
     * @brief 获取当前渲染阶段
     */
    RenderStage GetCurrentStage() const { return current_stage_; }

    /**
     * @brief 获取上一帧统计
     */
    const FrameStats& GetLastFrameStats() const { return last_frame_stats_; }

    /**
     * @brief 重置统计
     */
    void ResetStats();

    // =========================================================================
    // 组件访问（调试和高级用途）
    // =========================================================================

    /**
     * @brief 获取渲染树
     */
    std::shared_ptr<RenderObject> GetRenderTree() const { return render_tree_; }

    /**
     * @brief 获取层树根节点
     */
    std::shared_ptr<CompositorLayer> GetRootLayer() const { return root_layer_; }

    /**
     * @brief 获取属性树
     */
    PropertyTrees* GetPropertyTrees() const;

    /**
     * @brief 获取属性树构建器
     */
    PropertyTreeBuilder* GetPropertyTreeBuilder() const;

    /**
     * @brief 获取绘制产物合成器
     */
    PaintArtifactCompositor* GetPaintArtifactCompositor() const;

    /**
     * @brief 检查是否使用属性树系统
     */
    bool IsUsingPropertyTreeSystem() const { return config_.enable_property_trees; }

    /**
     * @brief 设置是否显示层边界
     */
    void SetShowLayerBorders(bool show);

private:
    // =========================================================================
    // 内部方法 - 渲染阶段
    // =========================================================================

    void DoDOMSync();
    void DoStyleRecalc();
    void DoLayout();
    void DoLayerTreeBuild();
    void DoRasterize();
    void DoComposite(SkCanvas* canvas);

    // =========================================================================
    // 内部方法 - 辅助
    // =========================================================================

    void EnsureRenderTree();
    void RegisterScrollableElements(RenderObject* root);
    void RegisterScrollableElementsRecursive(RenderObject* obj);
    void UpdateLayerTreeBounds(CompositorLayer* layer);
    bool CheckRenderObjectNeedsPaint(RenderObject* obj);
    double GetCurrentTimeMs() const;

private:
    // =========================================================================
    // 状态
    // =========================================================================

    bool initialized_ = false;
    RenderStage current_stage_ = RenderStage::Idle;
    RenderPipelineConfig config_;

    // 脏标记
    bool needs_dom_sync_ = false;
    bool needs_style_recalc_ = false;
    bool needs_layout_ = false;
    bool needs_paint_ = false;
    bool needs_layer_tree_rebuild_ = true;

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
    // 核心组件
    // =========================================================================

    std::shared_ptr<RenderTreeBuilder> render_tree_builder_;
    std::shared_ptr<RenderTreeSynchronizer> synchronizer_;
    std::shared_ptr<LayoutEngine> layout_engine_wrapper_;
    NativeLayoutEngine* layout_engine_ = nullptr;

    // 合成器组件
    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;

    // 优化组件
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

    FrameStats last_frame_stats_;
    FrameStats current_frame_stats_;
    double frame_start_time_ = 0.0;
};

} // namespace lightui
