/**
 * @file render_pipeline.cpp
 * @brief 统一渲染管线实现
 */

#include "render_pipeline.h"
#include "render_object.h"
#include "render_tree_synchronizer.h"
#include "style_resolver.h"  // RenderTreeBuilder
#include "../dom/document.h"
#include "../dom/dirty_node_tracker.h"
#include "../layout/native_layout_engine.h"
#include "../layout/layout_engine.h"
#include "../compositor/layer_tree_builder.h"
#include "../compositor/rasterizer.h"
#include "../compositor/compositor.h"
#include "../compositor/compositor_layer.h"
#include "../compositor/animation_layer_bridge.h"
#include "../compositor/scroll_layer_manager.h"
#include "../compositor/property_tree/property_trees.h"
#include "../compositor/property_tree/property_tree_builder.h"
#include "../compositor/property_tree/paint_artifact_compositor.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkM44.h"
#include <chrono>
#include <iostream>

namespace lightui {

// =========================================================================
// 构造和析构
// =========================================================================

RenderPipeline::RenderPipeline()
    : layer_tree_builder_(std::make_unique<LayerTreeBuilder>())
    , rasterizer_(std::make_unique<Rasterizer>())
    , compositor_(std::make_unique<Compositor>())
    , animation_bridge_(std::make_unique<AnimationLayerBridge>())
    , scroll_manager_(std::make_unique<ScrollLayerManager>())
    , property_trees_(std::make_unique<PropertyTrees>())
    , property_tree_builder_(std::make_unique<PropertyTreeBuilder>(*property_trees_))
    , paint_artifact_compositor_(std::make_unique<PaintArtifactCompositor>()) {

    // 连接组件
    animation_bridge_->SetLayerTreeBuilder(layer_tree_builder_.get());
    scroll_manager_->SetLayerTreeBuilder(layer_tree_builder_.get());
    scroll_manager_->SetRasterizer(rasterizer_.get());

    // 设置属性树系统
    paint_artifact_compositor_->SetPropertyTrees(property_trees_.get());
    scroll_manager_->SetPaintArtifactCompositor(paint_artifact_compositor_.get());
    scroll_manager_->SetPropertyTrees(property_trees_.get());
}

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

// =========================================================================
// 初始化
// =========================================================================

bool RenderPipeline::Initialize(int width, int height,
                                 const RenderPipelineConfig& config) {
    if (initialized_) {
        return true;
    }

    viewport_width_ = width;
    viewport_height_ = height;
    config_ = config;

    // 应用配置到组件
    layer_tree_builder_->SetLayerPromotionEnabled(config.enable_layer_promotion);
    rasterizer_->SetIncrementalEnabled(config.enable_incremental_rasterize);
    rasterizer_->SetScrollOptimizationEnabled(config.enable_scroll_optimization);
    compositor_->SetFrameSkipEnabled(config.enable_frame_skip);
    compositor_->SetShowLayerBorders(config.show_layer_borders);

    // 初始化合成器
    if (config.enable_gpu_compositing) {
        if (!compositor_->Initialize(width, height)) {
            // GPU 初始化失败，回退到 CPU
            config_.enable_gpu_compositing = false;
        }
    }

    initialized_ = true;
    needs_paint_ = true;
    needs_layer_tree_rebuild_ = true;

    return true;
}

void RenderPipeline::Shutdown() {
    if (!initialized_) {
        return;
    }

    compositor_->Shutdown();
    layer_tree_builder_->Clear();
    scroll_manager_->Clear();
    animation_bridge_->Clear();
    root_layer_.reset();
    render_tree_.reset();

    initialized_ = false;
}

void RenderPipeline::Resize(int width, int height) {
    if (width == viewport_width_ && height == viewport_height_) {
        return;
    }

    viewport_width_ = width;
    viewport_height_ = height;

    if (compositor_->IsInitialized()) {
        compositor_->Resize(width, height);
    }

    // 标记需要重新渲染和重建层树
    needs_paint_ = true;
    needs_layout_ = true;
    needs_layer_tree_rebuild_ = true;
}

// =========================================================================
// 配置
// =========================================================================

void RenderPipeline::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;

    // 创建渲染树同步器
    if (!synchronizer_) {
        synchronizer_ = std::make_shared<RenderTreeSynchronizer>();
    }
    synchronizer_->SetDocument(doc);

    // 如果有布局引擎，也设置到同步器
    if (layout_engine_wrapper_) {
        synchronizer_->SetLayoutEngine(layout_engine_wrapper_);
    }
}

void RenderPipeline::SetConfig(const RenderPipelineConfig& config) {
    config_ = config;

    // 应用配置到组件
    layer_tree_builder_->SetLayerPromotionEnabled(config.enable_layer_promotion);
    rasterizer_->SetIncrementalEnabled(config.enable_incremental_rasterize);
    rasterizer_->SetScrollOptimizationEnabled(config.enable_scroll_optimization);
    compositor_->SetFrameSkipEnabled(config.enable_frame_skip);
    compositor_->SetShowLayerBorders(config.show_layer_borders);

    // GPU 合成状态变化
    if (config.enable_gpu_compositing && !compositor_->IsInitialized()) {
        compositor_->Initialize(viewport_width_, viewport_height_);
    }

    needs_paint_ = true;
}

void RenderPipeline::SetDpiScale(float scale) {
    if (scale <= 0) {
        scale = 1.0f;
    }
    dpi_scale_ = scale;

    if (layer_tree_builder_) {
        layer_tree_builder_->SetDpiScale(scale);
    }

    needs_paint_ = true;
}

void RenderPipeline::SetShowLayerBorders(bool show) {
    config_.show_layer_borders = show;
    compositor_->SetShowLayerBorders(show);
}

// =========================================================================
// 渲染主入口
// =========================================================================

bool RenderPipeline::ProcessFrame(SkCanvas* canvas) {
    if (!initialized_ || !canvas) {
        return false;
    }

    frame_start_time_ = GetCurrentTimeMs();
    current_frame_stats_.Reset();

    // 1. DOM 同步阶段
    double stage_start = GetCurrentTimeMs();
    DoDOMSync();
    current_frame_stats_.dom_sync_time = GetCurrentTimeMs() - stage_start;

    // 2. 样式计算阶段
    stage_start = GetCurrentTimeMs();
    DoStyleRecalc();
    current_frame_stats_.style_time = GetCurrentTimeMs() - stage_start;

    // 3. 布局阶段
    stage_start = GetCurrentTimeMs();
    DoLayout();
    current_frame_stats_.layout_time = GetCurrentTimeMs() - stage_start;

    // 4. 层树构建阶段
    stage_start = GetCurrentTimeMs();
    DoLayerTreeBuild();
    current_frame_stats_.layer_tree_time = GetCurrentTimeMs() - stage_start;

    // 5. 光栅化阶段
    stage_start = GetCurrentTimeMs();
    DoRasterize();
    current_frame_stats_.rasterize_time = GetCurrentTimeMs() - stage_start;

    // 6. 合成阶段
    stage_start = GetCurrentTimeMs();
    DoComposite(canvas);
    current_frame_stats_.composite_time = GetCurrentTimeMs() - stage_start;

    // 更新统计
    current_frame_stats_.total_time = GetCurrentTimeMs() - frame_start_time_;
    current_frame_stats_.using_gpu = compositor_->IsUsingGPU();
    last_frame_stats_ = current_frame_stats_;

    // 重置脏标记
    needs_paint_ = false;
    current_stage_ = RenderStage::Idle;

    return true;
}

bool RenderPipeline::NeedsUpdate() const {
    // 检查 DOM 变化
    auto doc = document_.lock();
    if (doc && doc->GetDirtyTracker().HasPendingChanges()) {
        return true;
    }

    return needs_dom_sync_ || needs_style_recalc_ ||
           needs_layout_ || needs_paint_;
}

// =========================================================================
// 脏标记
// =========================================================================

void RenderPipeline::MarkNeedsStyleRecalc() {
    needs_style_recalc_ = true;
    needs_layout_ = true;
    needs_paint_ = true;
}

void RenderPipeline::MarkNeedsLayout() {
    needs_layout_ = true;
    needs_paint_ = true;
}

void RenderPipeline::MarkNeedsPaint() {
    needs_paint_ = true;
}

void RenderPipeline::MarkNeedsLayerTreeRebuild() {
    needs_layer_tree_rebuild_ = true;
    needs_paint_ = true;
}

void RenderPipeline::ForceFullUpdate() {
    needs_dom_sync_ = true;
    needs_style_recalc_ = true;
    needs_layout_ = true;
    needs_paint_ = true;
    needs_layer_tree_rebuild_ = true;
}

void RenderPipeline::InvalidateLayerTree() {
    needs_layer_tree_rebuild_ = true;
    needs_paint_ = true;
}

void RenderPipeline::ForceRasterize() {
    if (root_layer_) {
        root_layer_->MarkFullDirty();
    }
    needs_paint_ = true;
}

// =========================================================================
// 渲染阶段实现
// =========================================================================

void RenderPipeline::DoDOMSync() {
    current_stage_ = RenderStage::DOMSync;

    auto doc = document_.lock();
    if (!doc) {
        return;
    }

    auto& dirty_tracker = doc->GetDirtyTracker();
    if (!dirty_tracker.HasPendingChanges() && !needs_dom_sync_) {
        return;
    }

    // 确保渲染树存在
    EnsureRenderTree();

    // 同步渲染树
    if (synchronizer_ && render_tree_) {
        bool changed = synchronizer_->Synchronize(dirty_tracker, render_tree_);
        if (changed) {
            needs_layout_ = true;
            needs_paint_ = true;
            needs_layer_tree_rebuild_ = true;
        }
    }

    needs_dom_sync_ = false;
}

void RenderPipeline::DoStyleRecalc() {
    current_stage_ = RenderStage::Style;

    if (!needs_style_recalc_) {
        return;
    }

    // 样式在 DOM 操作时已经计算，这里主要处理级联样式变化
    // TODO: 实现更完整的样式重算逻辑

    needs_style_recalc_ = false;
}

void RenderPipeline::DoLayout() {
    current_stage_ = RenderStage::Layout;

    if (!needs_layout_ || !render_tree_) {
        return;
    }

    // 设置视口尺寸
    RenderObject::SetViewportSize(
        static_cast<float>(viewport_width_),
        static_cast<float>(viewport_height_));

    // 使用布局引擎计算布局
    if (layout_engine_) {
        layout_engine_->ComputeLayout(
            static_cast<float>(viewport_width_),
            static_cast<float>(viewport_height_));
    } else if (render_tree_) {
        render_tree_->Layout(
            static_cast<float>(viewport_width_),
            static_cast<float>(viewport_height_));
    }

    needs_layout_ = false;
}

void RenderPipeline::DoLayerTreeBuild() {
    current_stage_ = RenderStage::LayerTree;

    if (!render_tree_) {
        return;
    }

    // 构建或更新层树
    if (!root_layer_ || needs_layer_tree_rebuild_) {
        root_layer_ = layer_tree_builder_->Build(render_tree_.get());
        needs_layer_tree_rebuild_ = false;

        // 构建属性树
        if (config_.enable_property_trees && property_tree_builder_) {
            property_tree_builder_->Build(render_tree_.get());
        }
    } else {
        // 更新现有层的边界
        UpdateLayerTreeBounds(root_layer_.get());
    }

    current_frame_stats_.layers_built =
        static_cast<int>(layer_tree_builder_->GetLayerCount());

    // 注册滚动容器
    RegisterScrollableElements(render_tree_.get());
}

void RenderPipeline::DoRasterize() {
    current_stage_ = RenderStage::Rasterize;

    if (!root_layer_) {
        return;
    }

    // 光栅化脏层
    int rasterized = rasterizer_->RasterizeDirtyLayers(root_layer_.get());
    current_frame_stats_.layers_rasterized = rasterized;

    // 获取脏区域统计
    const auto& stats = rasterizer_->GetStats();
    current_frame_stats_.dirty_regions_count = stats.incremental_rasterizations;
}

void RenderPipeline::DoComposite(SkCanvas* canvas) {
    current_stage_ = RenderStage::Composite;

    if (!root_layer_ || !canvas) {
        return;
    }

    // 检查帧跳过
    if (config_.enable_frame_skip && !compositor_->NeedsComposite(root_layer_.get())) {
        current_frame_stats_.frame_skipped = true;
        return;
    }

    // 合成到 Canvas
    compositor_->CompositeToCanvas(root_layer_.get(), canvas);

    // 更新统计
    const auto& stats = compositor_->GetStats();
    current_frame_stats_.layers_rasterized = stats.layers_composited;
}

// =========================================================================
// 辅助方法
// =========================================================================

void RenderPipeline::EnsureRenderTree() {
    if (render_tree_) {
        return;
    }

    auto doc = document_.lock();
    if (!doc) {
        return;
    }

    // 创建渲染树构建器
    if (!render_tree_builder_) {
        render_tree_builder_ = std::make_shared<RenderTreeBuilder>();
    }

    // 构建渲染树
    auto body = doc->GetBody();
    if (body) {
        render_tree_ = render_tree_builder_->BuildRenderTree(body, nullptr);

        // 设置到同步器
        if (synchronizer_) {
            synchronizer_->SetRenderTreeBuilder(render_tree_builder_);
        }
    }
}

void RenderPipeline::RegisterScrollableElements(RenderObject* root) {
    if (!root) {
        return;
    }
    RegisterScrollableElementsRecursive(root);
}

void RenderPipeline::RegisterScrollableElementsRecursive(RenderObject* obj) {
    if (!obj) {
        return;
    }

    // 检查是否是滚动容器
    if (obj->IsScrollable()) {
        scroll_manager_->RegisterScrollContainer(obj);
    }

    // 检查是否是固定元素
    const auto& style = obj->GetComputedStyle();
    if (style.position == "fixed") {
        scroll_manager_->RegisterFixedElement(obj);
    }

    // 递归处理子元素
    for (const auto& child : obj->GetChildren()) {
        RegisterScrollableElementsRecursive(child.get());
    }
}

void RenderPipeline::UpdateLayerTreeBounds(CompositorLayer* layer) {
    if (!layer) {
        return;
    }

    RenderObject* render_obj = layer->GetRenderObject();
    if (render_obj) {
        layer_tree_builder_->UpdateLayerBounds(layer, render_obj);

        if (CheckRenderObjectNeedsPaint(render_obj)) {
            layer->MarkFullDirty();
        }
    }

    // 递归更新子层
    for (const auto& child : layer->GetChildren()) {
        UpdateLayerTreeBounds(child.get());
    }
}

bool RenderPipeline::CheckRenderObjectNeedsPaint(RenderObject* obj) {
    if (!obj) {
        return false;
    }

    if (obj->NeedsPaint()) {
        return true;
    }

    // 递归检查子对象
    for (const auto& child : obj->GetChildren()) {
        if (CheckRenderObjectNeedsPaint(child.get())) {
            return true;
        }
    }

    return false;
}

double RenderPipeline::GetCurrentTimeMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(duration).count();
}

// =========================================================================
// 滚动处理
// =========================================================================

bool RenderPipeline::HandleScroll(RenderObject* container, float delta_x, float delta_y) {
    if (!initialized_ || !container) {
        return false;
    }

    // 注册滚动容器（如果尚未注册）
    if (!scroll_manager_->IsScrollContainer(container)) {
        scroll_manager_->RegisterScrollContainer(container);
    }

    // 处理滚动
    bool scrolled = scroll_manager_->HandleScroll(container, delta_x, delta_y);

    if (scrolled) {
        compositor_->MarkNeedsComposite();
        needs_paint_ = true;
    }

    return scrolled;
}

bool RenderPipeline::ScrollTo(RenderObject* container, float scroll_x, float scroll_y) {
    if (!initialized_ || !container) {
        return false;
    }

    if (!scroll_manager_->IsScrollContainer(container)) {
        scroll_manager_->RegisterScrollContainer(container);
    }

    bool scrolled = scroll_manager_->ScrollTo(container, scroll_x, scroll_y);

    if (scrolled) {
        compositor_->MarkNeedsComposite();
        needs_paint_ = true;
    }

    return scrolled;
}

// =========================================================================
// 动画处理
// =========================================================================

void RenderPipeline::BeginAnimationFrame() {
    if (config_.enable_animation_optimization) {
        animation_bridge_->BeginAnimationUpdates();
    }
}

AnimationUpdateType RenderPipeline::UpdateAnimationProperty(
    RenderObject* object,
    const std::string& property,
    const std::string& value) {

    if (!config_.enable_animation_optimization) {
        return AnimationUpdateType::Paint;
    }

    return animation_bridge_->ApplyAnimationProperty(object, property, value);
}

bool RenderPipeline::EndAnimationFrame() {
    if (!config_.enable_animation_optimization) {
        return false;
    }

    bool has_layer_updates = animation_bridge_->EndAnimationUpdates();

    if (has_layer_updates) {
        compositor_->MarkNeedsComposite();
    }

    return has_layer_updates;
}

void RenderPipeline::OnAnimationStart(
    RenderObject* object,
    const std::string& animation_name,
    const std::vector<std::string>& properties) {

    if (config_.enable_animation_optimization) {
        animation_bridge_->OnAnimationStart(object, animation_name, properties);
    }
}

void RenderPipeline::OnAnimationEnd(RenderObject* object, const std::string& animation_name) {
    if (config_.enable_animation_optimization) {
        animation_bridge_->OnAnimationEnd(object, animation_name);
    }
}

// =========================================================================
// 属性树直接更新
// =========================================================================

bool RenderPipeline::DirectlyUpdateTransform(RenderObject* object, const SkM44& matrix) {
    if (!config_.enable_property_trees || !paint_artifact_compositor_ || !object) {
        return false;
    }
    // TODO: 需要从 RenderObject 获取对应的 TransformTreeNode
    // 当前简化实现，后续完善
    return false;
}

bool RenderPipeline::DirectlyUpdateOpacity(RenderObject* object, float opacity) {
    if (!config_.enable_property_trees || !paint_artifact_compositor_ || !object) {
        return false;
    }
    // TODO: 需要从 RenderObject 获取对应的 EffectTreeNode
    // 当前简化实现，后续完善
    return false;
}

bool RenderPipeline::DirectlyUpdateScrollOffset(RenderObject* object, const SkPoint& offset) {
    if (!config_.enable_property_trees || !paint_artifact_compositor_ || !object) {
        return false;
    }
    // TODO: 需要从 RenderObject 获取对应的 ScrollTreeNode
    // 当前简化实现，后续完善
    return false;
}

// =========================================================================
// 脏区域管理
// =========================================================================

void RenderPipeline::MarkDirty(RenderObject* object) {
    if (!object) {
        return;
    }

    auto layer = layer_tree_builder_->GetLayerForRenderObject(object);
    if (layer) {
        const auto& layout = object->GetLayoutInfo();
        SkRect bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
        layer->MarkDirty(bounds);
    }

    needs_paint_ = true;
}

void RenderPipeline::MarkDirtyRegion(const SkRect& region) {
    if (root_layer_) {
        root_layer_->MarkDirty(region);
    }
    needs_paint_ = true;
}

// =========================================================================
// 统计
// =========================================================================

void RenderPipeline::ResetStats() {
    last_frame_stats_.Reset();
    current_frame_stats_.Reset();
    rasterizer_->ResetStats();
    compositor_->ResetStats();
}

// =========================================================================
// 组件访问
// =========================================================================

PropertyTrees* RenderPipeline::GetPropertyTrees() const {
    return property_trees_.get();
}

PropertyTreeBuilder* RenderPipeline::GetPropertyTreeBuilder() const {
    return property_tree_builder_.get();
}

PaintArtifactCompositor* RenderPipeline::GetPaintArtifactCompositor() const {
    return paint_artifact_compositor_.get();
}

} // namespace lightui
