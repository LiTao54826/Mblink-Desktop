/**
 * @file render_pipeline.cpp
 * @brief 统一渲染管线实现
 *
 * 合并 RenderPipelineLegacy (V1) 和 RenderPipelineV2 的代码。
 * 所有代码直接从已验证的 V1 和 V2 复制，不重新实现。
 */

#include "render_pipeline.h"
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"  // RenderTreeBuilder 在这里定义
#include "render_tree_synchronizer.h"
#include "core/dom/document.h"
#include "core/dom/node.h"
#include "core/dom/observers/dirty_node_tracker.h"
#include "core/dom/style/incremental_style_recalc.h"
#include "core/layout/native_layout_engine.h"
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <cstdlib>


// 全局变量：用于控制调试日志输出（放在全局命名空间，方便其他编译单元访问）
int g_debug_frames_remaining = 0;

namespace mbink {

// =========================================================================
// 辅助函数（复制自 V2）
// =========================================================================

namespace {
    double GetCurrentTimeMs() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    bool IsAnimFrameDebugEnabled() {
        static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_FRAME") != nullptr);
        return enabled;
    }

    bool IsLayerRebuildDebugEnabled() {
        static const bool enabled = (std::getenv("MBINK_DEBUG_LAYER_REBUILD") != nullptr);
        return enabled;
    }

    bool IsBaselineFrameStatsEnabled() {
        static const bool enabled = (std::getenv("MBINK_BASELINE_FRAME_STATS") != nullptr);
        return enabled;
    }

    bool IsNearlyFullLayerRect(const SkRect& rect, const SkRect& layer_bounds) {
        if (rect.isEmpty() || layer_bounds.isEmpty()) {
            return false;
        }

        const float layer_area = layer_bounds.width() * layer_bounds.height();
        if (layer_area <= 0.0f) {
            return false;
        }

        const float rect_area = rect.width() * rect.height();
        return rect_area >= layer_area * 0.90f;
    }

    bool IsTightNativeTextElement(const RenderObject* obj) {
        if (!obj) {
            return false;
        }
        auto node = obj->GetNode();
        if (!node || node->GetNodeType() != NodeType::ELEMENT_NODE) {
            return false;
        }

        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) {
            return false;
        }
        const std::string tag_name = element->GetTagName();
        return tag_name == "terminal" || tag_name == "logview";
    }

    float DirtyOutsetForRenderObject(const RenderObject* obj) {
        constexpr float kTightOutset = 4.0f;
        constexpr float kConservativeOutset = 50.0f;
        if (!obj) {
            return kConservativeOutset;
        }
        if (IsTightNativeTextElement(obj)) {
            return kTightOutset;
        }

        const auto& style = obj->GetComputedStyle();
        const bool has_visual_overflow =
            !style.box_shadow.empty() ||
            !style.text_shadow.empty() ||
            style.filter.has_value() ||
            style.backdrop_filter.has_value() ||
            (style.outline_style != "none" && !style.outline_width.IsZero());
        return has_visual_overflow ? kConservativeOutset : kTightOutset;
    }

    int RetainedPresentBlockingScrollFallbacks(const ScrollInvalidationStats& stats) {
        return static_cast<int>(
            IncrementalEligibleScrollFallbacks(stats) + ConservativeScrollFallbacks(stats));
    }

    void ClearRenderedTreePaintDirtyFlags(RenderObject* obj) {
        if (!obj || !obj->IsDirtyForPaint()) {
            return;
        }

        const bool has_dirty_children = obj->ChildNeedsPaint();
        obj->ClearNeedsPaint();
        obj->ClearChildNeedsPaint();

        if (!has_dirty_children) {
            return;
        }

        for (const auto& child : obj->GetChildren()) {
            ClearRenderedTreePaintDirtyFlags(child.get());
        }
    }

    bool AccumulatePaintScrollOffset(const RenderObject* obj,
                                     const RenderObject* ancestor,
                                     SkPoint& offset) {
        offset = SkPoint::Make(0.0f, 0.0f);
        if (!obj || !ancestor || obj == ancestor) {
            return true;
        }

        auto parent = obj->GetParent();
        while (parent) {
            offset.offset(parent->GetScrollX(), parent->GetScrollY());
            if (parent.get() == ancestor) {
                return true;
            }
            parent = parent->GetParent();
        }

        offset = SkPoint::Make(0.0f, 0.0f);
        return false;
    }

    void MoveToLayerPaintSpace(SkRect& rect,
                               const RenderObject* obj,
                               const RenderObject* layer_render_obj) {
        SkPoint scroll_offset;
        if (AccumulatePaintScrollOffset(obj, layer_render_obj, scroll_offset) &&
            (scroll_offset.fX != 0.0f || scroll_offset.fY != 0.0f)) {
            rect.offset(-scroll_offset.fX, -scroll_offset.fY);
        }
    }
}

// =========================================================================
// 构造和析构
// =========================================================================

RenderPipeline::RenderPipeline()
    // 初始化 V2 组件（复制自 RenderPipelineV2 构造函数）
    : layer_tree_builder_(std::make_unique<LayerTreeBuilder>())
    , layer_tree_manager_(std::make_unique<LayerTreeManager>())
    , rasterizer_(std::make_unique<Rasterizer>())
    , compositor_(std::make_unique<Compositor>())
    , animation_bridge_(std::make_unique<AnimationLayerBridge>())
    , scroll_manager_(std::make_unique<ScrollLayerManager>())
    , property_trees_(std::make_unique<PropertyTrees>())
    , property_tree_builder_(std::make_unique<PropertyTreeBuilder>(*property_trees_))
    , paint_artifact_compositor_(std::make_unique<PaintArtifactCompositor>()) {

    // 连接组件（复制自 V2）
    animation_bridge_->SetLayerTreeBuilder(layer_tree_builder_.get());
    scroll_manager_->SetLayerTreeBuilder(layer_tree_builder_.get());
    scroll_manager_->SetRasterizer(rasterizer_.get());

    // 初始化 LayerTreeManager
    layer_tree_manager_->Initialize(
        layer_tree_builder_.get(),
        rasterizer_.get(),
        compositor_.get()
    );

    // 设置属性树系统（复制自 V2）
    paint_artifact_compositor_->SetPropertyTrees(property_trees_.get());
    scroll_manager_->SetPaintArtifactCompositor(paint_artifact_compositor_.get());
    scroll_manager_->SetPropertyTrees(property_trees_.get());
}

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

// =========================================================================
// 初始化（复制自 V2）
// =========================================================================

bool RenderPipeline::Initialize(int width, int height, const UnifiedPipelineConfig& config) {
    if (initialized_) {
        return true;
    }

    viewport_width_ = width;
    viewport_height_ = height;
    config_ = config;

    // 应用配置（复制自 V2）
    layer_tree_builder_->SetLayerPromotionEnabled(config.enable_layer_promotion);
    rasterizer_->SetIncrementalEnabled(config.enable_incremental_rasterize);
    rasterizer_->SetScrollOptimizationEnabled(config.enable_scroll_optimization);
    rasterizer_->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
    compositor_->SetFrameSkipEnabled(config.enable_frame_skip);
    compositor_->SetShowLayerBorders(config.show_layer_borders);

    // 初始化 LayerTreeManager 视口
    layer_tree_manager_->SetViewport(
        static_cast<float>(width),
        static_cast<float>(height),
        dpi_scale_
    );

    // 初始化合成器（复制自 V2）
    if (config.enable_gpu_compositing) {
        if (!compositor_->Initialize(width, height)) {
            config_.enable_gpu_compositing = false;
        }
    }

    initialized_ = true;
    needs_render_ = true;
    last_observed_layer_tree_scroll_stats_.Reset();
    last_observed_scroll_manager_stats_.Reset();
    pending_scroll_invalidation_reason_ = ScrollInvalidationReason::None;
    return true;
}

void RenderPipeline::Shutdown() {
    if (!initialized_) {
        return;
    }

    // 复制自 V2
    compositor_->Shutdown();
    layer_tree_builder_->Clear();
    scroll_manager_->Clear();
    layer_tree_manager_->ResetInvalidationStats();
    last_observed_layer_tree_scroll_stats_.Reset();
    last_observed_scroll_manager_stats_.Reset();
    pending_scroll_invalidation_reason_ = ScrollInvalidationReason::None;
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

    // 复制自 V2
    if (compositor_->IsInitialized()) {
        compositor_->Resize(width, height);
    }

    // 同步 Rasterizer 视口尺寸（用于 fixed 元素 clip rect）
    rasterizer_->SetViewportSize(static_cast<float>(width), static_cast<float>(height));

    // 更新 LayerTreeManager 视口
    layer_tree_manager_->SetViewport(
        static_cast<float>(width),
        static_cast<float>(height),
        dpi_scale_
    );

    // 标记需要重新渲染和重建层树
    needs_render_ = true;
    needs_layer_tree_rebuild_ = true;
    needs_layout_ = true;  // V1: 视口变化需要重新布局
}

// =========================================================================
// 配置
// =========================================================================

void RenderPipeline::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;
    needs_render_ = true;
    needs_layer_tree_rebuild_ = true;
}

void RenderPipeline::SetConfig(const UnifiedPipelineConfig& config) {
    config_ = config;

    // 复制自 V2
    layer_tree_builder_->SetLayerPromotionEnabled(config.enable_layer_promotion);
    rasterizer_->SetIncrementalEnabled(config.enable_incremental_rasterize);
    rasterizer_->SetScrollOptimizationEnabled(config.enable_scroll_optimization);
    compositor_->SetFrameSkipEnabled(config.enable_frame_skip);
    compositor_->SetShowLayerBorders(config.show_layer_borders);

    if (config.enable_gpu_compositing && !compositor_->IsInitialized()) {
        compositor_->Initialize(viewport_width_, viewport_height_);
    }

    needs_render_ = true;
}

void RenderPipeline::SetDpiScale(float scale) {
    if (scale <= 0) {
        scale = 1.0f;
    }
    dpi_scale_ = scale;

    // 复制自 V2
    if (layer_tree_builder_) {
        layer_tree_builder_->SetDpiScale(scale);
    }

    needs_render_ = true;
}

void RenderPipeline::SetShowLayerBorders(bool show) {
    config_.show_layer_borders = show;
    compositor_->SetShowLayerBorders(show);
}

void RenderPipeline::SetRenderTree(std::shared_ptr<RenderObject> tree) {
    if (render_tree_ != tree) {
        // 关键修复：只要 RenderTree 根指针变化，就必须完整重建层树。
        // 旧逻辑只在 render_tree_ 由 nullptr -> 非空时重建，
        // 会导致渲染树已重建但层树仍绑定旧 RenderObject（出现“双实例/动画打到旧对象”）。
        render_tree_ = tree;
        needs_layer_tree_rebuild_ = true;

        // 主动丢弃旧层树，避免同一帧内误用旧 root_layer_。
        if (layer_tree_builder_) {
            layer_tree_builder_->Clear();
        }
        if (scroll_manager_) {
            scroll_manager_->Clear();
        }
        if (layer_tree_manager_) {
            layer_tree_manager_->ClearScrollState();
        }
        root_layer_.reset();

        needs_render_ = true;
    }
}

// =========================================================================
// 脏标记（复制自 V1）
// =========================================================================

void RenderPipeline::MarkNeedsStyleRecalc() {
    needs_style_recalc_ = true;
    needs_layout_ = true;  // 样式变化通常需要重新布局
    needs_paint_ = true;
    needs_render_ = true;
}

void RenderPipeline::MarkNeedsLayout() {
    needs_layout_ = true;
    needs_paint_ = true;  // 布局变化需要重新绘制
    needs_render_ = true;
}

void RenderPipeline::MarkNeedsPaint() {
    needs_paint_ = true;
    needs_render_ = true;
}

void RenderPipeline::ForceFullUpdate() {
    needs_style_recalc_ = true;
    needs_layout_ = true;
    needs_paint_ = true;
    needs_render_ = true;
    needs_layer_tree_rebuild_ = true;
}

// =========================================================================
// 层树管理（复制自 V2）
// =========================================================================

void RenderPipeline::InvalidateLayerTree() {
    needs_layer_tree_rebuild_ = true;
    needs_render_ = true;
}

void RenderPipeline::ForceRasterize() {
    if (root_layer_) {
        MarkAllLayersDirty(root_layer_.get());
    }
    needs_render_ = true;
}

void RenderPipeline::MarkDirty(RenderObject* object) {
    if (!object) {
        return;
    }

    // 复制自 V2
    auto layer = layer_tree_builder_->GetLayerForRenderObject(object);
    if (layer) {
        const auto& layout = object->GetLayoutInfo();
        SkRect bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
        layer->MarkDirty(bounds);
    }

    needs_render_ = true;
}

void RenderPipeline::MarkDirtyRegion(const SkRect& region) {
    if (root_layer_) {
        root_layer_->MarkDirty(region);
    }
    needs_render_ = true;
}

// =========================================================================
// 检查是否需要更新
// =========================================================================

bool RenderPipeline::NeedsUpdate() const {
    return needs_style_recalc_ || needs_layout_ || needs_paint_ || needs_render_;
}

bool RenderPipeline::HasPendingScrollFullDirtyFallback() const {
    const auto& layer_tree_scroll_stats = layer_tree_manager_->GetInvalidationStats();
    const auto& scroll_manager_stats = scroll_manager_->GetInvalidationStats();
    return layer_tree_scroll_stats.full_dirty_scrolls >
               last_observed_layer_tree_scroll_stats_.full_dirty_scrolls ||
           scroll_manager_stats.full_dirty_scrolls >
               last_observed_scroll_manager_stats_.full_dirty_scrolls;
}

bool RenderPipeline::HasPendingScrollRetainedPresentBlockingFallback() const {
    const auto& layer_tree_scroll_stats = layer_tree_manager_->GetInvalidationStats();
    const auto& scroll_manager_stats = scroll_manager_->GetInvalidationStats();
    return RetainedPresentBlockingScrollFallbacks(layer_tree_scroll_stats) >
               RetainedPresentBlockingScrollFallbacks(last_observed_layer_tree_scroll_stats_) ||
           RetainedPresentBlockingScrollFallbacks(scroll_manager_stats) >
               RetainedPresentBlockingScrollFallbacks(last_observed_scroll_manager_stats_);
}

// =========================================================================
// 主渲染入口
// =========================================================================

bool RenderPipeline::ProcessFrame(SkCanvas* canvas,
                                  const SkRect* logical_clip,
                                  const std::vector<SkRect>* raster_dirty_rects) {
    if (!initialized_ || !canvas) {
        return false;
    }

    const bool debug_anim_frame = IsAnimFrameDebugEnabled();
    static uint64_t frame_seq = 0;
    ++frame_seq;

    frame_start_time_ = GetCurrentTimeMs();
    current_frame_stats_.Reset();
    current_stage_ = RenderStage::Idle;
    external_root_dirty_rects_ = raster_dirty_rects;

    // 注意：渲染树由 Window::EnsureRenderTree() 构建和管理
    // RenderPipeline 不再自己构建渲染树，而是使用外部设置的渲染树
    // 如果没有渲染树，直接返回
    if (!render_tree_) {
        if (IsBaselineFrameStatsEnabled()) {
            std::cout << "[MBINK_BASELINE_FRAME]"
                      << " boundary=pipeline"
                      << " frame=" << frame_seq
                      << " class=no_render_tree"
                      << "\n";
        }
        if (debug_anim_frame && (frame_seq <= 120 || (frame_seq % 60 == 0))) {
            std::cout << "[ANIM_FRAME_PIPELINE] frame=" << frame_seq
                      << " earlyReturn=no_render_tree"
                      << "\n";
        }
        external_root_dirty_rects_ = nullptr;
        return false;
    }

    // =========================================================================
    // 阶段 1-3: 来自 V1 的流程
    // =========================================================================

    // 1. DOM 同步
    double stage_start = GetCurrentTimeMs();
    DoDOMSync();
    current_frame_stats_.dom_sync_time = GetCurrentTimeMs() - stage_start;

    // 2. 样式重算
    if (needs_style_recalc_) {
        stage_start = GetCurrentTimeMs();
        DoStyleRecalc();
        current_frame_stats_.style_time = GetCurrentTimeMs() - stage_start;
    }

    // 3. 布局（由 Window::EnsureRenderTree() 完成，这里跳过）
    // 注意：布局在 Window 中完成，这里只清除标记
    if (needs_layout_) {
        needs_layout_ = false;
    }

    // =========================================================================
    // 阶段 4-6: 来自 V2 的流程
    // =========================================================================

    // 4. 层树构建
    stage_start = GetCurrentTimeMs();
    DoLayerTreeBuild();
    current_frame_stats_.layer_tree_time = GetCurrentTimeMs() - stage_start;

    // 5. 光栅化
    stage_start = GetCurrentTimeMs();
    DoRasterize();
    current_frame_stats_.rasterize_time = GetCurrentTimeMs() - stage_start;

    // 6. 合成
    stage_start = GetCurrentTimeMs();
    DoComposite(canvas, logical_clip);
    current_frame_stats_.composite_time = GetCurrentTimeMs() - stage_start;

    // 更新统计
    current_frame_stats_.total_time = GetCurrentTimeMs() - frame_start_time_;
    current_frame_stats_.using_gpu = compositor_->IsUsingGPU();
    const auto& layer_tree_scroll_stats = layer_tree_manager_->GetInvalidationStats();
    const auto& scroll_manager_stats = scroll_manager_->GetInvalidationStats();
    auto diff_counter = [](std::uint64_t current, std::uint64_t previous) {
        return static_cast<int>(current - previous);
    };
    const int layer_tree_scrolls_handled = diff_counter(
        layer_tree_scroll_stats.scrolls_handled,
        last_observed_layer_tree_scroll_stats_.scrolls_handled);
    const int scroll_manager_scrolls_handled = diff_counter(
        scroll_manager_stats.scrolls_handled,
        last_observed_scroll_manager_stats_.scrolls_handled);
    current_frame_stats_.scrolls_handled =
        layer_tree_scrolls_handled + scroll_manager_scrolls_handled;
    current_frame_stats_.scroll_full_dirty_fallbacks =
        diff_counter(layer_tree_scroll_stats.full_dirty_scrolls,
                     last_observed_layer_tree_scroll_stats_.full_dirty_scrolls) +
        diff_counter(scroll_manager_stats.full_dirty_scrolls,
                     last_observed_scroll_manager_stats_.full_dirty_scrolls);
    current_frame_stats_.scroll_clip_layer_full_dirty_fallbacks =
        diff_counter(layer_tree_scroll_stats.clip_layer_full_dirty_scrolls,
                     last_observed_layer_tree_scroll_stats_.clip_layer_full_dirty_scrolls) +
        diff_counter(scroll_manager_stats.clip_layer_full_dirty_scrolls,
                     last_observed_scroll_manager_stats_.clip_layer_full_dirty_scrolls);
    current_frame_stats_.scroll_ancestor_layer_full_dirty_fallbacks =
        diff_counter(layer_tree_scroll_stats.ancestor_layer_full_dirty_scrolls,
                     last_observed_layer_tree_scroll_stats_.ancestor_layer_full_dirty_scrolls) +
        diff_counter(scroll_manager_stats.ancestor_layer_full_dirty_scrolls,
                     last_observed_scroll_manager_stats_.ancestor_layer_full_dirty_scrolls);
    current_frame_stats_.scroll_missing_layer_target_fallbacks =
        diff_counter(layer_tree_scroll_stats.missing_layer_target_scrolls,
                     last_observed_layer_tree_scroll_stats_.missing_layer_target_scrolls) +
        diff_counter(scroll_manager_stats.missing_layer_target_scrolls,
                     last_observed_scroll_manager_stats_.missing_layer_target_scrolls);
    ScrollInvalidationStats frame_scroll_stats;
    frame_scroll_stats.clip_layer_full_dirty_scrolls =
        current_frame_stats_.scroll_clip_layer_full_dirty_fallbacks;
    frame_scroll_stats.ancestor_layer_full_dirty_scrolls =
        current_frame_stats_.scroll_ancestor_layer_full_dirty_fallbacks;
    frame_scroll_stats.missing_layer_target_scrolls =
        current_frame_stats_.scroll_missing_layer_target_fallbacks;
    current_frame_stats_.scroll_incremental_eligible_fallbacks =
        static_cast<int>(IncrementalEligibleScrollFallbacks(frame_scroll_stats));
    current_frame_stats_.scroll_conservative_fallbacks =
        static_cast<int>(ConservativeScrollFallbacks(frame_scroll_stats));
    current_frame_stats_.scroll_retained_present_blocking_fallbacks =
        current_frame_stats_.scroll_incremental_eligible_fallbacks +
        current_frame_stats_.scroll_conservative_fallbacks;
    current_frame_stats_.last_scroll_invalidation_reason =
        (current_frame_stats_.scrolls_handled > 0)
            ? pending_scroll_invalidation_reason_
            : ScrollInvalidationReason::None;
    if (current_frame_stats_.scrolls_handled > 0 &&
        current_frame_stats_.last_scroll_invalidation_reason == ScrollInvalidationReason::None) {
        current_frame_stats_.last_scroll_invalidation_reason =
            (scroll_manager_scrolls_handled > 0)
                ? scroll_manager_stats.last_reason
                : layer_tree_scroll_stats.last_reason;
    }
    last_observed_layer_tree_scroll_stats_ = layer_tree_scroll_stats;
    last_observed_scroll_manager_stats_ = scroll_manager_stats;
    pending_scroll_invalidation_reason_ = ScrollInvalidationReason::None;
    last_frame_stats_ = current_frame_stats_;

    if (IsBaselineFrameStatsEnabled()) {
        std::cout << "[MBINK_BASELINE_FRAME]"
                  << " boundary=pipeline"
                  << " frame=" << frame_seq
                  << " class=rendered"
                  << " total_ms=" << current_frame_stats_.total_time
                  << " dom_ms=" << current_frame_stats_.dom_sync_time
                  << " style_ms=" << current_frame_stats_.style_time
                  << " layer_ms=" << current_frame_stats_.layer_tree_time
                  << " raster_ms=" << current_frame_stats_.rasterize_time
                  << " composite_ms=" << current_frame_stats_.composite_time
                  << " layers_built=" << current_frame_stats_.layers_built
                  << " layers_rasterized=" << current_frame_stats_.layers_rasterized
                  << " layers_composited=" << current_frame_stats_.layers_composited
                  << " full_rasterizations=" << current_frame_stats_.full_rasterizations
                  << " incremental_rasterizations=" << current_frame_stats_.incremental_rasterizations
                  << " pixels_rasterized=" << current_frame_stats_.pixels_rasterized
                  << " pixels_skipped=" << current_frame_stats_.pixels_skipped
                  << " textures_uploaded=" << current_frame_stats_.textures_uploaded
                  << " frames_composited=" << current_frame_stats_.frames_composited
                  << " frames_skipped=" << current_frame_stats_.frames_skipped
                  << " compositor_composite_ms=" << current_frame_stats_.compositor_composite_time_ms
                  << " scrolls_handled=" << current_frame_stats_.scrolls_handled
                  << " scroll_full_dirty_fallbacks=" << current_frame_stats_.scroll_full_dirty_fallbacks
                  << " scroll_clip_layer_full_dirty_fallbacks="
                  << current_frame_stats_.scroll_clip_layer_full_dirty_fallbacks
                  << " scroll_ancestor_layer_full_dirty_fallbacks="
                  << current_frame_stats_.scroll_ancestor_layer_full_dirty_fallbacks
                  << " scroll_missing_layer_target_fallbacks="
                  << current_frame_stats_.scroll_missing_layer_target_fallbacks
                  << " scroll_incremental_eligible_fallbacks="
                  << current_frame_stats_.scroll_incremental_eligible_fallbacks
                  << " scroll_conservative_fallbacks="
                  << current_frame_stats_.scroll_conservative_fallbacks
                  << " scroll_retained_present_blocking_fallbacks="
                  << current_frame_stats_.scroll_retained_present_blocking_fallbacks
                  << " scroll_last_reason="
                  << static_cast<int>(current_frame_stats_.last_scroll_invalidation_reason)
                  << " using_gpu=" << (current_frame_stats_.using_gpu ? 1 : 0)
                  << "\n";
    }

    if (debug_anim_frame && (frame_seq <= 120 || (frame_seq % 60 == 0))) {
        std::cout << "[ANIM_FRAME_PIPELINE] frame=" << frame_seq
                  << " ok=1"
                  << " layersBuilt=" << current_frame_stats_.layers_built
                  << " layersRasterized=" << current_frame_stats_.layers_rasterized
                  << " layersComposited=" << current_frame_stats_.layers_composited
                  << " totalMs=" << current_frame_stats_.total_time
                  << "\n";
    }

    // 清除脏标记
    needs_render_ = false;
    needs_paint_ = false;
    ClearRenderedTreePaintDirtyFlags(render_tree_.get());
    external_root_dirty_rects_ = nullptr;
    current_stage_ = RenderStage::Idle;

    return true;
}


// =========================================================================
// 渲染阶段实现 - 来自 V1
// =========================================================================

void RenderPipeline::EnsureRenderTree() {
    // 注意：渲染树现在由 Window::EnsureRenderTree() 构建和管理
    // 通过 SetRenderTree() 传入，这个方法保留为空实现以保持接口兼容
    // 不再在这里构建渲染树，避免重复构建导致内存问题
}

void RenderPipeline::DoDOMSync() {
    current_stage_ = RenderStage::DOMSync;

    // 注意：渲染树同步现在由 Window::EnsureRenderTree() 处理
    // Window 在调用 ProcessFrame 之前会确保渲染树是最新的
    // 这里只需要清除脏标记，不需要再次同步

    auto doc = document_.lock();
    if (!doc) {
        return;
    }

    // 清除脏标记（因为 Window::EnsureRenderTree() 已经处理了）
    DirtyNodeTracker& dirty_tracker = doc->GetDirtyTracker();
    if (dirty_tracker.HasPendingChanges()) {
        current_frame_stats_.dirty_nodes = static_cast<int>(
            dirty_tracker.GetStructuralChangeCount() +
            dirty_tracker.GetStyleChangeCount() +
            dirty_tracker.GetTextChangeCount()
        );
        dirty_tracker.Clear();
    }
}

void RenderPipeline::DoStyleRecalc() {
    current_stage_ = RenderStage::StyleRecalc;

    auto doc = document_.lock();
    if (!doc) {
        needs_style_recalc_ = false;
        return;
    }

    // 使用增量样式重算
    IncrementalStyleRecalc style_recalc;
    style_recalc.RecalcStyle(doc.get());

    // 更新统计信息
    current_frame_stats_.style_nodes_visited = style_recalc.GetNodesVisited();
    current_frame_stats_.style_nodes_recalculated = style_recalc.GetNodesRecalculated();
    current_frame_stats_.style_subtrees_skipped = style_recalc.GetSubtreesSkipped();

    // 计算优化比率
    int total_nodes = current_frame_stats_.style_nodes_visited + current_frame_stats_.style_subtrees_skipped;
    if (total_nodes > 0) {
        current_frame_stats_.optimization_ratio =
            static_cast<double>(current_frame_stats_.style_subtrees_skipped) / total_nodes;
        current_frame_stats_.used_incremental_update =
            (current_frame_stats_.style_subtrees_skipped > 0);
    }

    // 更新 DirtyNodeTracker 统计
    DirtyNodeTracker& dirty_tracker = doc->GetDirtyTracker();
    current_frame_stats_.text_changes_count = static_cast<int>(dirty_tracker.GetTextChangeCount());
    current_frame_stats_.structural_changes_count = static_cast<int>(dirty_tracker.GetStructuralChangeCount());

    needs_style_recalc_ = false;
}

void RenderPipeline::DoLayout() {
    current_stage_ = RenderStage::Layout;

    // 复制自 V1 的 DoLayout
    if (!layout_engine_ || !render_tree_) {
        needs_layout_ = false;
        return;
    }

    // 执行布局计算
    layout_engine_->ComputeLayout(
        static_cast<float>(viewport_width_),
        static_cast<float>(viewport_height_)
    );

    needs_layout_ = false;
    needs_paint_ = true;  // 布局后需要重绘
}

// =========================================================================
// 渲染阶段实现 - 来自 V2
// =========================================================================

void RenderPipeline::DoLayerTreeBuild() {
    current_stage_ = RenderStage::LayerTreeBuild;

    if (!render_tree_) {
        return;
    }

    // 关键修复：在构建层树前，设置 DPI 缩放
    layer_tree_builder_->SetDpiScale(dpi_scale_);

    // 检查是否可以使用增量更新
    bool use_incremental = config_.enable_incremental_layer_tree &&
                           root_layer_ &&
                           !needs_layer_tree_rebuild_ &&
                           layer_tree_builder_->CanIncrementalUpdate() &&
                           layer_tree_manager_->HasPendingUpdates();

    if (IsLayerRebuildDebugEnabled()) {
        std::cout << "[LAYER_BUILD_DECISION]"
                  << " incremental_enabled=" << (config_.enable_incremental_layer_tree ? 1 : 0)
                  << " has_root=" << (root_layer_ ? 1 : 0)
                  << " needs_rebuild=" << (needs_layer_tree_rebuild_ ? 1 : 0)
                  << " can_incremental=" << (layer_tree_builder_->CanIncrementalUpdate() ? 1 : 0)
                  << " has_pending=" << (layer_tree_manager_->HasPendingUpdates() ? 1 : 0)
                  << " use_incremental=" << (use_incremental ? 1 : 0)
                  << "\n";
    }

    static bool debug_layer_build = std::getenv("MBINK_DEBUG_LAYER_BUILD") != nullptr;

    // 递减调试帧计数器
    if (g_debug_frames_remaining > 0) {
        if (debug_layer_build) {
        }
    }

    if (use_incremental) {
        // 增量更新路径
        if (IsLayerRebuildDebugEnabled()) {
            std::cout << "[LAYER_BUILD_PATH] mode=incremental\n";
        }
        layer_tree_manager_->ApplyPendingUpdates();

        // 更新现有层的边界和脏区域
        UpdateLayerTreeBounds(root_layer_.get());
    } else if (!root_layer_ || needs_layer_tree_rebuild_) {
        // 完整重建路径
        if (IsLayerRebuildDebugEnabled()) {
            std::cout << "[LAYER_BUILD_PATH] mode=full_rebuild"
                      << " reason_no_root=" << (!root_layer_ ? 1 : 0)
                      << " reason_needs_rebuild=" << (needs_layer_tree_rebuild_ ? 1 : 0)
                      << "\n";
        }
        root_layer_ = layer_tree_builder_->Build(render_tree_.get());
        needs_layer_tree_rebuild_ = false;
        layer_tree_manager_->ClearFullRebuildFlag();

        // 关键修复：根层边界应该使用视口尺寸，而不是渲染树的布局尺寸
        // 因为渲染树的布局尺寸可能小于视口（例如内容不足以填满视口）
        if (root_layer_) {
            root_layer_->SetBounds(SkRect::MakeWH(
                static_cast<float>(viewport_width_),
                static_cast<float>(viewport_height_)));

            root_layer_->MarkFullDirty();

            // 关键修复：层树重建后，立即恢复滚动偏移
            // 注意：不能依赖 IsScrollable()，因为新创建的 RenderObject 可能
            // IsScrollable() 返回 false，但 GetScrollY() 已经有值（从 DOM 同步过来）
            RenderObject* root_obj = root_layer_->GetRenderObject();
            if (root_obj) {
                float scroll_x = root_obj->GetScrollX();
                float scroll_y = root_obj->GetScrollY();
                if (scroll_x != 0 || scroll_y != 0) {
                    root_layer_->SetScrollOffset(SkPoint::Make(scroll_x, scroll_y));
                }
            }

            // 🐛 修复：完整重建后，恢复所有可滚动层的滚动偏移
            // 不仅仅是 root 层，中间的可滚动容器也需要恢复
            RestoreScrollOffsetsAfterRebuild(root_layer_.get());
        }

        // 构建属性树（复制自 V2）
        if (config_.enable_property_trees && property_tree_builder_) {
            property_tree_builder_->Build(render_tree_.get());
        }
    } else {
        // 更新边界路径：先检测并创建新层，再更新边界和收集脏区域
        // 同时检测并删除孤立层（对应的 RenderObject 已被删除）

        // 1. 检测并删除孤立层
        RemoveOrphanedLayers(root_layer_.get());

        // 2. 检测并创建新层
        DetectAndCreateNewLayers(render_tree_.get());

        // 3. 更新现有层的边界和脏区域
        UpdateLayerTreeBounds(root_layer_.get());
    }

    current_frame_stats_.layers_built = static_cast<int>(layer_tree_builder_->GetLayerCount());

    // 注册滚动容器
    RegisterScrollableElements(render_tree_.get());

    // 递减调试帧计数器
    if (g_debug_frames_remaining > 0) {
        g_debug_frames_remaining--;
        if (debug_layer_build) {
        }
    }
}

void RenderPipeline::DoRasterize() {
    current_stage_ = RenderStage::Rasterize;

    if (!root_layer_) {
        return;
    }

    const RasterizeStats stats_before = rasterizer_->GetStats();

    // 复制自 V2 的 RasterizeDirtyLayers
    int rasterized = rasterizer_->RasterizeDirtyLayers(root_layer_.get());
    current_frame_stats_.layers_rasterized = rasterized;

    const RasterizeStats stats_after = rasterizer_->GetStats();
    current_frame_stats_.full_rasterizations =
        stats_after.full_rasterizations - stats_before.full_rasterizations;
    current_frame_stats_.incremental_rasterizations =
        stats_after.incremental_rasterizations - stats_before.incremental_rasterizations;
    current_frame_stats_.pixels_rasterized =
        stats_after.pixels_rasterized - stats_before.pixels_rasterized;
    current_frame_stats_.pixels_skipped =
        stats_after.pixels_skipped - stats_before.pixels_skipped;
    current_frame_stats_.dirty_regions_count = current_frame_stats_.incremental_rasterizations;

    // if (frame_full_rasterizations > 0 || frame_incremental_rasterizations > 0) {
    //     const char* render_mode = frame_full_rasterizations > 0 ? "full" : "incremental";
    //     std::cout << "[RENDER_MODE] mode=" << render_mode
    //               << " full=" << frame_full_rasterizations
    //               << " incremental=" << frame_incremental_rasterizations
    //               << " layers=" << rasterized
    //               << "\n";
    // }
}

void RenderPipeline::DoComposite(SkCanvas* canvas, const SkRect* logical_clip) {
    current_stage_ = RenderStage::Composite;

    if (!canvas) {
        return;
    }

    // 优先使用层合成（如果层树已构建）
    if (root_layer_) {
        const CompositeStats stats_before = compositor_->GetStats();
        compositor_->CompositeToCanvas(root_layer_.get(), canvas, logical_clip);
        const CompositeStats stats_after = compositor_->GetStats();
        current_frame_stats_.frames_composited =
            stats_after.frames_composited - stats_before.frames_composited;
        current_frame_stats_.frames_skipped =
            stats_after.frames_skipped - stats_before.frames_skipped;
        current_frame_stats_.layers_composited =
            stats_after.layers_composited - stats_before.layers_composited;
        current_frame_stats_.textures_uploaded =
            stats_after.textures_uploaded - stats_before.textures_uploaded;
        current_frame_stats_.compositor_composite_time_ms =
            stats_after.composite_time_ms - stats_before.composite_time_ms;
        current_frame_stats_.frame_skipped = (current_frame_stats_.frames_skipped > 0);
        return;
    }

    // 回退：直接绘制渲染树（层树未构建时）
    if (render_tree_) {
        render_tree_->Paint(canvas);
    }
}

// =========================================================================
// 辅助方法（复制自 V2）
// =========================================================================

void RenderPipeline::UpdateLayerTreeBounds(CompositorLayer* layer) {
    if (!layer) {
        return;
    }

    RenderObject* render_obj = layer->GetRenderObject();
    if (render_obj) {
        layer_tree_builder_->UpdateLayerBounds(layer, render_obj);

        // 关键修复：更新滚动偏移
        // 每帧都需要同步滚动偏移，确保动画层能正确跟随滚动
        // 注意：不能依赖 IsScrollable()，因为 root 元素等可能
        // IsScrollable() 返回 false，但 GetScrollX/Y 已经有值
        {
            float scroll_x = render_obj->GetScrollX();
            float scroll_y = render_obj->GetScrollY();
            if (scroll_x != 0 || scroll_y != 0) {
                layer->SetScrollOffset(SkPoint::Make(scroll_x, scroll_y));
            }
        }

        // GPU 增量渲染优化：使用精确的脏矩形而不是整层标记
        // 只标记需要重绘的 RenderObject 的边界区域
        CollectDirtyRectsForLayer(render_obj, layer);
    }

    for (const auto& child : layer->GetChildren()) {
        UpdateLayerTreeBounds(child.get());
    }
}

void RenderPipeline::CollectDirtyRectsForLayer(RenderObject* obj, CompositorLayer* layer) {
    if (!obj || !layer) {
        return;
    }

    // 🐛 hover bug 调试日志
    static bool debug_hover = std::getenv("MBINK_DEBUG_HOVER_BUG") != nullptr;

    auto rect_differs = [](const SkRect& a, const SkRect& b) {
        const float eps = 0.01f;
        return std::abs(a.left() - b.left()) > eps ||
               std::abs(a.top() - b.top()) > eps ||
               std::abs(a.right() - b.right()) > eps ||
               std::abs(a.bottom() - b.bottom()) > eps;
    };

    // 如果当前节点需要重绘，或位置/尺寸相对上次绘制发生变化，也要标记其边界为脏
    bool force_dirty_by_bounds_change = false;
    if (obj->HasPreviousPaintBounds()) {
        if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer) {
            force_dirty_by_bounds_change = rect_differs(obj->GetViewportBoundingRect(),
                                                       obj->GetPreviousViewportPaintBounds());
        } else {
            RenderObject* layer_render_obj = layer->GetRenderObject();
            if (layer_render_obj) {
                force_dirty_by_bounds_change = rect_differs(obj->GetBoundingRectRelativeTo(layer_render_obj),
                                                           obj->GetPreviousPaintBounds());
            }
        }
    }

    if (obj->NeedsPaint() || force_dirty_by_bounds_change) {
        SkRect bounds;
        SkRect previous_bounds;
        bool has_previous_bounds = false;

        // 获取元素信息用于调试
        std::string tag_name = "unknown";
        std::string element_id = "";
        if (auto node = obj->GetNode()) {
            if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                tag_name = element->GetTagName();
                element_id = element->GetAttribute("id");
            } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
                tag_name = "#text";
            }
        }

        // 对于根层，使用视口坐标系的边界（已经考虑了所有祖先的滚动偏移）
        if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer) {
            bounds = obj->GetViewportBoundingRect();
            if (obj->HasPreviousPaintBounds()) {
                previous_bounds = obj->GetPreviousViewportPaintBounds();
                has_previous_bounds = !previous_bounds.isEmpty();
            }

            // 🐛 hover bug 调试日志
            if (debug_hover) {
            }

            // 关键调试：在新层创建后的几帧内，输出所有导致根层被标记为脏的元素
            if (g_debug_frames_remaining > 0) {
                const auto& style = obj->GetComputedStyle();
                if (!element_id.empty()) {
                }
            }
        } else {
            // 对于非根层，区分两种情况：
            // 1. obj == layer_render_obj：层自身 → 全层标记为脏
            // 2. obj 是子元素：使用 GetBoundingRectRelativeTo 计算相对位置
            //    （与 LayerTreeBuilder::UpdateLayerBounds 的坐标累加逻辑一致）
            //
            // 性能优化：小尺寸层（≤200px）直接全层标记，避免精确计算开销

            RenderObject* layer_render_obj = layer->GetRenderObject();
            const SkRect& layer_bounds = layer->GetBounds();

            // 小尺寸层阈值（可通过环境变量配置）
            static float small_layer_threshold = []() {
                const char* env = std::getenv("MBINK_SMALL_LAYER_THRESHOLD");
                return env ? std::atof(env) : 200.0f;
            }();

            bool is_fixed_layer = layer->GetPromotionReason() == LayerPromotionReason::PositionFixed;
            bool is_small_layer = !is_fixed_layer &&
                                  (layer_bounds.width() <= small_layer_threshold &&
                                   layer_bounds.height() <= small_layer_threshold);

            if (obj == layer_render_obj || is_small_layer) {
                bounds = SkRect::MakeWH(layer_bounds.width(), layer_bounds.height());
            } else {
                bounds = obj->GetBoundingRectRelativeTo(layer_render_obj);
                MoveToLayerPaintSpace(bounds, obj, layer_render_obj);
                if (obj->HasPreviousPaintBounds()) {
                    previous_bounds = obj->GetPreviousPaintBounds();
                    if (!previous_bounds.isEmpty()) {
                        previous_bounds.offset(-layer_render_obj->GetBoundingRect().x(),
                                               -layer_render_obj->GetBoundingRect().y());
                        MoveToLayerPaintSpace(previous_bounds, obj, layer_render_obj);
                        has_previous_bounds = true;
                    }
                }
            }
        }

        if (has_previous_bounds) {
            bounds.join(previous_bounds);
        }

        // 扩展边界以包含阴影、outline 等
        const float dirty_outset = DirtyOutsetForRenderObject(obj);
        bounds.outset(dirty_outset, dirty_outset);

        if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer &&
            external_root_dirty_rects_ &&
            !external_root_dirty_rects_->empty() &&
            IsNearlyFullLayerRect(bounds, layer->GetBounds())) {
            for (const auto& dirty_rect : *external_root_dirty_rects_) {
                if (!dirty_rect.isEmpty()) {
                    layer->MarkDirty(dirty_rect);
                }
            }
        } else {
            layer->MarkDirty(bounds);
        }
        obj->ClearPreviousPaintBounds();
    }

    // 优化：如果子节点不需要重绘，跳过整个子树
    if (!obj->ChildNeedsPaint()) {
        return;
    }

    // 递归处理子节点
    for (const auto& child : obj->GetChildren()) {
        // 跳过有独立层的子节点（它们会在自己的层中处理）
        if (child->HasOwnCompositorLayer()) {
            continue;
        }
        CollectDirtyRectsForLayer(child.get(), layer);
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

    // 复制自 V2
    if (obj->IsScrollable()) {
        scroll_manager_->RegisterScrollContainer(obj);
        // 关键修复：每次渲染时更新内容尺寸
        // 这确保当 overflow 变化导致布局宽度变化时，滚动条能正确更新
        scroll_manager_->UpdateContentSize(obj);
    }

    const auto& style = obj->GetComputedStyle();
    if (style.position == "fixed") {
        scroll_manager_->RegisterFixedElement(obj);
    }

    for (const auto& child : obj->GetChildren()) {
        RegisterScrollableElementsRecursive(child.get());
    }
}

bool RenderPipeline::CheckRenderObjectNeedsPaint(RenderObject* obj) {
    if (!obj) {
        return false;
    }

    // 复制自 V2
    if (obj->NeedsPaint()) {
        return true;
    }

    for (const auto& child : obj->GetChildren()) {
        if (CheckRenderObjectNeedsPaint(child.get())) {
            return true;
        }
    }

    return false;
}

void RenderPipeline::DetectAndCreateNewLayers(RenderObject* root) {
    if (!root || !root_layer_ || !layer_tree_builder_) {
        return;
    }
    DetectAndCreateNewLayersRecursive(root);
}

void RenderPipeline::DetectAndCreateNewLayersRecursive(RenderObject* obj) {
    if (!obj) {
        return;
    }

    // 检查这个元素是否需要层但还没有层
    if (!obj->HasOwnCompositorLayer()) {
        LayerPromotionReason reason = layer_tree_builder_->ShouldPromote(obj);
        if (reason != LayerPromotionReason::None) {
            // 获取元素信息
            std::string tag_name = "unknown";
            std::string element_id = "";
            if (auto node = obj->GetNode()) {
                if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto element = std::static_pointer_cast<Element>(node);
                    tag_name = element->GetTagName();
                    element_id = element->GetAttribute("id");
                }
            }

            // 只对 position: fixed 元素输出详细日志（默认关闭）
            bool is_fixed = (reason == LayerPromotionReason::PositionFixed);
            static bool debug_layers = std::getenv("MBINK_DEBUG_LAYERS") != nullptr ||
                                       std::getenv("MBINK_DEBUG_DIRTY") != nullptr;
            if (is_fixed && debug_layers) {
                if (!element_id.empty()) {
                }
                const auto& layout = obj->GetLayoutInfo();
            }

            // 创建层
            auto new_layer = layer_tree_builder_->AddLayerForObject(obj, reason);
            if (new_layer) {
                new_layer->MarkFullDirty();

                // 关键修复：标记父层为脏，确保父层重新光栅化
                // 当子元素被提升为独立层后，父层的 bitmap 中还残留该元素的旧内容
                // 必须重新光栅化父层，使其 Paint 时跳过已有独立层的子元素
                auto parent_layer = new_layer->GetParent();
                if (parent_layer) {
                    parent_layer->MarkFullDirty();
                }

                if (is_fixed && debug_layers) {
                    // 启用接下来3帧的详细日志
                    g_debug_frames_remaining = 3;
                }
            } else if (is_fixed && debug_layers) {
            }
        }
    }

    // 递归处理子节点
    for (const auto& child : obj->GetChildren()) {
        DetectAndCreateNewLayersRecursive(child.get());
    }
}

void RenderPipeline::RemoveOrphanedLayers(CompositorLayer* layer) {
    if (!layer || !render_tree_) {
        return;
    }

    // 收集渲染树中所有有效的 RenderObject 指针
    std::unordered_set<RenderObject*> valid_objects;
    std::function<void(RenderObject*)> collect = [&](RenderObject* obj) {
        if (!obj) return;
        valid_objects.insert(obj);
        for (const auto& child : obj->GetChildren()) {
            collect(child.get());
        }
    };
    collect(render_tree_.get());

    // 调试：输出收集到的对象数量
    static bool debug_orphan = std::getenv("MBINK_DEBUG_ORPHAN") != nullptr;
    if (debug_orphan) {
    }

    // 收集需要删除的子层
    std::vector<std::shared_ptr<CompositorLayer>> layers_to_remove;

    // 检查所有子层
    for (const auto& child : layer->GetChildren()) {
        // 跳过根层
        if (child->GetPromotionReason() == LayerPromotionReason::RootLayer) {
            continue;
        }

        // 跳过 ScrollableContent 层 - 这些层故意不设置 RenderObject
        // 它们是容器层，用于组织子层并应用滚动偏移
        if (child->GetPromotionReason() == LayerPromotionReason::ScrollableContent) {
            // 递归检查其子层
            RemoveOrphanedLayers(child.get());
            continue;
        }

        // 检查层对应的 RenderObject 是否还在渲染树中
        RenderObject* render_obj = child->GetRenderObject();

        // 调试：输出层信息
        if (debug_orphan && render_obj) {
            std::string tag_name = "unknown";
            if (auto node = render_obj->GetNode()) {
                if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto element = std::static_pointer_cast<Element>(node);
                    tag_name = element->GetTagName();
                }
            }
            bool is_valid = (valid_objects.find(render_obj) != valid_objects.end());
        }

        if (!render_obj || valid_objects.find(render_obj) == valid_objects.end()) {
            // RenderObject 已被删除或不在渲染树中
            layers_to_remove.push_back(child);

            std::string tag_name = "unknown";
            if (render_obj) {
                if (auto node = render_obj->GetNode()) {
                    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
                        auto element = std::static_pointer_cast<Element>(node);
                        tag_name = element->GetTagName();
                    }
                }
            }

        } else {
            // 递归检查子层的子层
            RemoveOrphanedLayers(child.get());
        }
    }

    // 删除孤立层
    for (const auto& orphan : layers_to_remove) {
        // 清除 RenderObject 的层引用
        if (orphan->GetRenderObject()) {
            orphan->GetRenderObject()->SetCompositorLayer(nullptr);
        }
        // 从父层移除
        layer->RemoveChild(orphan.get());
    }
}

void RenderPipeline::RestoreScrollOffsetsAfterRebuild(CompositorLayer* layer) {
    if (!layer) return;

    // 恢复当前层的滚动偏移
    // 注意：不能依赖 IsScrollable()，因为新创建的 RenderObject 可能
    // IsScrollable() 返回 false，但 GetScrollX/Y 已经有值
    RenderObject* obj = layer->GetRenderObject();
    if (obj) {
        float sx = obj->GetScrollX();
        float sy = obj->GetScrollY();
        if (sx != 0 || sy != 0) {
            layer->SetScrollOffset(SkPoint::Make(sx, sy));
        }
    }

    // 递归处理子层
    for (const auto& child : layer->GetChildren()) {
        RestoreScrollOffsetsAfterRebuild(child.get());
    }
}

void RenderPipeline::MarkAllLayersDirty(CompositorLayer* layer) {
    if (!layer) {
        return;
    }

    // 标记当前层为脏
    layer->MarkFullDirty();

    // 递归标记所有子层
    for (const auto& child : layer->GetChildren()) {
        MarkAllLayersDirty(child.get());
    }
}

// =========================================================================
// 滚动处理（复制自 V2）
// =========================================================================

bool RenderPipeline::HandleScroll(RenderObject* container, float delta_x, float delta_y) {
    if (!initialized_ || !container) {
        return false;
    }

    if (!scroll_manager_->IsScrollContainer(container)) {
        scroll_manager_->RegisterScrollContainer(container);
    }

    // 关键修复：在处理滚动前更新内容尺寸
    // 这确保页面切换后滚动范围被正确更新
    scroll_manager_->UpdateContentSize(container);

    bool scrolled = false;

    // 如果启用了增量层树更新，使用 LayerTreeManager 处理滚动
    if (config_.enable_incremental_layer_tree) {
        // 注册到 LayerTreeManager（如果尚未注册）
        if (!layer_tree_manager_->IsScrollContainer(container)) {
            layer_tree_manager_->RegisterScrollContainer(container);
        }
        // 关键修复：在滚动前更新 LayerTreeManager 的滚动范围
        // 之前只更新了 scroll_manager_ 的内容尺寸，但走 layer_tree_manager_ 路径时
        // 没有更新其 max_scroll_y，导致使用注册时的旧值，鼠标滚轮无法滚到底部
        layer_tree_manager_->UpdateScrollContentSize(container);
        scrolled = layer_tree_manager_->ScrollBy(container, delta_x, delta_y);
    } else {
        scrolled = scroll_manager_->HandleScroll(container, delta_x, delta_y);
    }

    if (scrolled) {
        const auto& scroll_stats = config_.enable_incremental_layer_tree
            ? layer_tree_manager_->GetInvalidationStats()
            : scroll_manager_->GetInvalidationStats();
        pending_scroll_invalidation_reason_ = scroll_stats.last_reason;
        compositor_->MarkNeedsComposite();
        needs_render_ = true;
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

    bool scrolled = false;

    // 如果启用了增量层树更新，使用 LayerTreeManager 处理滚动
    if (config_.enable_incremental_layer_tree) {
        // 注册到 LayerTreeManager（如果尚未注册）
        if (!layer_tree_manager_->IsScrollContainer(container)) {
            layer_tree_manager_->RegisterScrollContainer(container);
        }
        scrolled = layer_tree_manager_->SetScrollPosition(container, scroll_x, scroll_y);
    } else {
        scrolled = scroll_manager_->ScrollTo(container, scroll_x, scroll_y);
    }

    if (scrolled) {
        const auto& scroll_stats = config_.enable_incremental_layer_tree
            ? layer_tree_manager_->GetInvalidationStats()
            : scroll_manager_->GetInvalidationStats();
        pending_scroll_invalidation_reason_ = scroll_stats.last_reason;
        compositor_->MarkNeedsComposite();
        needs_render_ = true;
    }

    return scrolled;
}

// =========================================================================
// 动画处理（复制自 V2）
// =========================================================================

void RenderPipeline::BeginAnimationFrame() {
    if (config_.enable_animation_optimization) {
        animation_bridge_->BeginAnimationUpdates();
    }
}

AnimationUpdateType RenderPipeline::UpdateAnimationProperty(RenderObject* object,
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

void RenderPipeline::OnAnimationStart(RenderObject* object,
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

} // namespace mbink
