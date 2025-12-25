/**
 * @file render_pipeline_v2.cpp
 * @brief 渲染管线 V2 实现
 */

#include "render_pipeline_v2.h"
#include "../render/render_object.h"
#include <chrono>
#include <iostream>

namespace lightui {

namespace {
    double GetCurrentTimeMs() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<double, std::milli>(duration).count();
    }
}

RenderPipelineV2::RenderPipelineV2()
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
    
    // 连接属性树系统到滚动管理器
    scroll_manager_->SetPaintArtifactCompositor(paint_artifact_compositor_.get());
    scroll_manager_->SetPropertyTrees(property_trees_.get());
}

RenderPipelineV2::~RenderPipelineV2() {
    Shutdown();
}

// =========================================================================
// 初始化
// =========================================================================

bool RenderPipelineV2::Initialize(int width, int height, const RenderPipelineConfig& config) {
    if (initialized_) {
        return true;
    }

    viewport_width_ = width;
    viewport_height_ = height;
    config_ = config;

    // 应用配置
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
    needs_render_ = true;
    return true;
}

void RenderPipelineV2::Resize(int width, int height) {
    if (width == viewport_width_ && height == viewport_height_) {
        return;
    }

    viewport_width_ = width;
    viewport_height_ = height;

    if (compositor_->IsInitialized()) {
        compositor_->Resize(width, height);
    }

    // 标记需要重新渲染和重建层树
    needs_render_ = true;
    needs_rebuild_layer_tree_ = true;
}

void RenderPipelineV2::Shutdown() {
    if (!initialized_) {
        return;
    }

    compositor_->Shutdown();
    layer_tree_builder_->Clear();
    scroll_manager_->Clear();
    animation_bridge_->Clear();
    root_layer_.reset();

    initialized_ = false;
}

// =========================================================================
// 渲染
// =========================================================================

bool RenderPipelineV2::Render(RenderObject* root) {
    if (!initialized_ || !root) {
        return false;
    }

    frame_start_time_ = GetCurrentTimeMs();
    current_frame_stats_.Reset();

    // 1. 构建/更新层树
    double build_start = GetCurrentTimeMs();
    BuildLayerTree(root);
    current_frame_stats_.layer_tree_build_time = GetCurrentTimeMs() - build_start;

    // 2. 光栅化脏层
    double rasterize_start = GetCurrentTimeMs();
    RasterizeDirtyLayers();
    current_frame_stats_.rasterize_time = GetCurrentTimeMs() - rasterize_start;

    // 3. 合成到屏幕
    double composite_start = GetCurrentTimeMs();
    bool success = CompositeLayers();
    current_frame_stats_.composite_time = GetCurrentTimeMs() - composite_start;

    // 更新统计
    current_frame_stats_.total_time = GetCurrentTimeMs() - frame_start_time_;
    current_frame_stats_.using_gpu = compositor_->IsUsingGPU();
    last_frame_stats_ = current_frame_stats_;

    needs_render_ = false;
    return success;
}

bool RenderPipelineV2::RenderToCanvas(RenderObject* root, SkCanvas* canvas) {
    if (!initialized_ || !root || !canvas) {
        return false;
    }

    frame_start_time_ = GetCurrentTimeMs();
    current_frame_stats_.Reset();

    // 1. 构建/更新层树
    double build_start = GetCurrentTimeMs();
    BuildLayerTree(root);
    current_frame_stats_.layer_tree_build_time = GetCurrentTimeMs() - build_start;

    // 2. 光栅化脏层
    double rasterize_start = GetCurrentTimeMs();
    RasterizeDirtyLayers();
    current_frame_stats_.rasterize_time = GetCurrentTimeMs() - rasterize_start;

    // 3. 合成到 Canvas
    double composite_start = GetCurrentTimeMs();
    bool success = CompositeLayersToCanvas(canvas);
    current_frame_stats_.composite_time = GetCurrentTimeMs() - composite_start;

    // 更新统计
    current_frame_stats_.total_time = GetCurrentTimeMs() - frame_start_time_;
    current_frame_stats_.using_gpu = false;  // Canvas 模式不使用 GPU
    last_frame_stats_ = current_frame_stats_;

    needs_render_ = false;
    return success;
}

// =========================================================================
// 滚动处理
// =========================================================================

bool RenderPipelineV2::HandleScroll(RenderObject* container, float delta_x, float delta_y) {
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
        // 滚动成功，标记需要合成和重新渲染
        compositor_->MarkNeedsComposite();
        needs_render_ = true;
    }

    return scrolled;
}

bool RenderPipelineV2::ScrollTo(RenderObject* container, float scroll_x, float scroll_y) {
    if (!initialized_ || !container) {
        return false;
    }

    // 注册滚动容器（如果尚未注册）
    if (!scroll_manager_->IsScrollContainer(container)) {
        scroll_manager_->RegisterScrollContainer(container);
    }

    bool scrolled = scroll_manager_->ScrollTo(container, scroll_x, scroll_y);
    
    if (scrolled) {
        compositor_->MarkNeedsComposite();
    }

    return scrolled;
}

// =========================================================================
// 动画处理
// =========================================================================

void RenderPipelineV2::BeginAnimationFrame() {
    if (config_.enable_animation_optimization) {
        animation_bridge_->BeginAnimationUpdates();
    }
}

AnimationUpdateType RenderPipelineV2::UpdateAnimationProperty(RenderObject* object,
                                                               const std::string& property,
                                                               const std::string& value) {
    if (!config_.enable_animation_optimization) {
        return AnimationUpdateType::Paint;
    }

    return animation_bridge_->ApplyAnimationProperty(object, property, value);
}

bool RenderPipelineV2::EndAnimationFrame() {
    if (!config_.enable_animation_optimization) {
        return false;
    }

    bool has_layer_updates = animation_bridge_->EndAnimationUpdates();
    
    if (has_layer_updates) {
        // 有层属性更新，标记需要合成
        compositor_->MarkNeedsComposite();
    }

    return has_layer_updates;
}

void RenderPipelineV2::OnAnimationStart(RenderObject* object,
                                         const std::string& animation_name,
                                         const std::vector<std::string>& properties) {
    if (config_.enable_animation_optimization) {
        animation_bridge_->OnAnimationStart(object, animation_name, properties);
    }
}

void RenderPipelineV2::OnAnimationEnd(RenderObject* object, const std::string& animation_name) {
    if (config_.enable_animation_optimization) {
        animation_bridge_->OnAnimationEnd(object, animation_name);
    }
}

// =========================================================================
// 脏区域管理
// =========================================================================

void RenderPipelineV2::MarkDirty(RenderObject* object) {
    if (!object) {
        return;
    }

    // 获取对象关联的层
    auto layer = layer_tree_builder_->GetLayerForRenderObject(object);
    if (layer) {
        // 标记层的脏区域
        const auto& layout = object->GetLayoutInfo();
        SkRect bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
        layer->MarkDirty(bounds);
    }

    needs_render_ = true;
}

void RenderPipelineV2::MarkDirtyRegion(const SkRect& region) {
    if (root_layer_) {
        root_layer_->MarkDirty(region);
    }
    needs_render_ = true;
}

// =========================================================================
// 配置
// =========================================================================

void RenderPipelineV2::SetConfig(const RenderPipelineConfig& config) {
    config_ = config;

    // 应用配置
    layer_tree_builder_->SetLayerPromotionEnabled(config.enable_layer_promotion);
    rasterizer_->SetIncrementalEnabled(config.enable_incremental_rasterize);
    rasterizer_->SetScrollOptimizationEnabled(config.enable_scroll_optimization);
    compositor_->SetFrameSkipEnabled(config.enable_frame_skip);
    compositor_->SetShowLayerBorders(config.show_layer_borders);

    // GPU 合成状态变化
    if (config.enable_gpu_compositing && !compositor_->IsInitialized()) {
        compositor_->Initialize(viewport_width_, viewport_height_);
    }

    needs_render_ = true;
}

void RenderPipelineV2::SetShowLayerBorders(bool show) {
    config_.show_layer_borders = show;
    compositor_->SetShowLayerBorders(show);
}

void RenderPipelineV2::SetDpiScale(float scale) {
    if (scale <= 0) {
        scale = 1.0f;
    }
    dpi_scale_ = scale;
    
    // 传递给 LayerTreeBuilder
    if (layer_tree_builder_) {
        layer_tree_builder_->SetDpiScale(scale);
    }
    
    // 标记需要重新渲染
    needs_render_ = true;
}

// =========================================================================
// 统计
// =========================================================================

void RenderPipelineV2::ResetStats() {
    last_frame_stats_.Reset();
    current_frame_stats_.Reset();
    rasterizer_->ResetStats();
    compositor_->ResetStats();
}

// =========================================================================
// 私有方法
// =========================================================================

void RenderPipelineV2::BuildLayerTree(RenderObject* root) {
    // 关键修复：不要每帧都重建层树，这会导致性能问题和动画状态丢失
    // 只在首次或需要时重建
    if (!root_layer_ || needs_rebuild_layer_tree_) {
        root_layer_ = layer_tree_builder_->Build(root);
        needs_rebuild_layer_tree_ = false;
        
        // 关键：构建属性树并将状态设置到 RenderObject 上
        // 这样 AnimationApplicator 才能使用属性树优化
        if (use_property_tree_system_ && property_tree_builder_) {
            property_tree_builder_->Build(root);
        }
    } else {
        // 更新现有层的边界和脏区域
        UpdateLayerTreeBounds(root_layer_.get());
    }
    
    // 修复：移除无条件标记根层为脏的逻辑
    // 这是导致每帧全量光栅化的根本原因
    // 正确的做法是让各层自己追踪脏区域，只有内容真正变化时才标记
    // 动画只改变 transform/opacity 时，不需要重新光栅化
    // 
    // 旧代码（已移除）：
    // if (root_layer_ && CheckRenderObjectNeedsPaint(root)) {
    //     root_layer_->MarkFullDirty();
    // }
    
    current_frame_stats_.layers_built = static_cast<int>(layer_tree_builder_->GetLayerCount());

    // 注册滚动容器和固定元素
    RegisterScrollableElements(root);
}

void RenderPipelineV2::RasterizeDirtyLayers() {
    if (!root_layer_) {
        return;
    }

    // 光栅化所有脏层
    int rasterized = rasterizer_->RasterizeDirtyLayers(root_layer_.get());
    current_frame_stats_.layers_rasterized = rasterized;

    // 获取脏区域统计
    const auto& stats = rasterizer_->GetStats();
    current_frame_stats_.dirty_regions_count = stats.incremental_rasterizations;
}

bool RenderPipelineV2::CompositeLayers() {
    if (!root_layer_) {
        return false;
    }

    // 检查是否需要合成
    if (config_.enable_frame_skip && !compositor_->NeedsComposite(root_layer_.get())) {
        current_frame_stats_.frame_skipped = true;
        return true;
    }

    // 合成
    bool success = compositor_->Composite(root_layer_.get());
    
    // 更新统计
    const auto& stats = compositor_->GetStats();
    current_frame_stats_.layers_composited = stats.layers_composited;

    return success;
}

bool RenderPipelineV2::CompositeLayersToCanvas(SkCanvas* canvas) {
    if (!root_layer_ || !canvas) {
        return false;
    }

    return compositor_->CompositeToCanvas(root_layer_.get(), canvas);
}

void RenderPipelineV2::RegisterScrollableElements(RenderObject* root) {
    if (!root) {
        return;
    }

    RegisterScrollableElementsRecursive(root);
}

void RenderPipelineV2::RegisterScrollableElementsRecursive(RenderObject* obj) {
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

void RenderPipelineV2::UpdateLayerTreeBounds(CompositorLayer* layer) {
    if (!layer) {
        return;
    }

    // 获取关联的渲染对象
    RenderObject* render_obj = layer->GetRenderObject();
    if (render_obj) {
        // 使用 LayerTreeBuilder 更新层边界
        // 这会正确处理动画边界扩展
        layer_tree_builder_->UpdateLayerBounds(layer, render_obj);
        
        // 修复：检查层关联的渲染对象及其所有子对象的 NeedsPaint 标志
        // 因为动画元素可能是层内的子元素，不是层的直接关联对象
        if (CheckRenderObjectNeedsPaint(render_obj)) {
            layer->MarkFullDirty();
        }
    }

    // 递归更新子层
    for (const auto& child : layer->GetChildren()) {
        UpdateLayerTreeBounds(child.get());
    }
}

// 移除 ClearNeedsPaintRecursive，不再需要

bool RenderPipelineV2::CheckRenderObjectNeedsPaint(RenderObject* obj) {
    if (!obj) {
        return false;
    }
    
    // 修复：只检查 NeedsPaint 标志
    // 不再因为有动画配置就返回 true
    // 动画只改变 transform/opacity 时不需要重新光栅化内容
    // 这是分层合成的核心优化
    if (obj->NeedsPaint()) {
        return true;
    }
    
    // 修复：移除动画检测逻辑
    // 旧代码会导致每帧都返回 true，触发全量光栅化
    // 
    // 旧代码（已移除）：
    // const auto& style = obj->GetComputedStyle();
    // if (!style.animations.empty()) {
    //     for (const auto& anim : style.animations) {
    //         if (!anim.name.empty() && anim.name != "none") {
    //             return true;  // 这会导致每帧都重绘
    //         }
    //     }
    // }
    
    // 递归检查子对象
    for (const auto& child : obj->GetChildren()) {
        if (CheckRenderObjectNeedsPaint(child.get())) {
            return true;
        }
    }
    
    return false;
}

} // namespace lightui
