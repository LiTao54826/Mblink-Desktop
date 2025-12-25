/**
 * @file render_pipeline_legacy.cpp
 * @brief 旧版渲染管线实现（已废弃，保留用于过渡）
 * @deprecated 请使用新的 RenderPipeline
 */

#include "render_pipeline_legacy.h"
#include "render_tree_synchronizer.h"
#include "render_object.h"
#include "../dom/dirty_node_tracker.h"
#include "../dom/document.h"
#include "../layout/native_layout_engine.h"
#include "include/core/SkCanvas.h"

namespace lightui {

RenderPipelineLegacy::RenderPipelineLegacy() = default;
RenderPipelineLegacy::~RenderPipelineLegacy() = default;

void RenderPipelineLegacy::SetDirtyTracker(DirtyNodeTracker* tracker) {
    dirty_tracker_ = tracker;
}

void RenderPipelineLegacy::SetRenderTree(std::shared_ptr<RenderObject> render_tree) {
    render_tree_ = render_tree;
}

void RenderPipelineLegacy::SetLayoutEngine(NativeLayoutEngine* layout_engine) {
    layout_engine_ = layout_engine;
}

void RenderPipelineLegacy::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;
}

void RenderPipelineLegacy::SetSynchronizer(std::shared_ptr<RenderTreeSynchronizer> synchronizer) {
    synchronizer_ = synchronizer;
}

void RenderPipelineLegacy::SetViewportSize(float width, float height) {
    if (viewport_width_ != width || viewport_height_ != height) {
        viewport_width_ = width;
        viewport_height_ = height;
        // 视口尺寸变化需要重新布局
        MarkNeedsLayout();
    }
}

void RenderPipelineLegacy::SetPaintCallback(std::function<void(SkCanvas*)> callback) {
    paint_callback_ = std::move(callback);
}

void RenderPipelineLegacy::MarkNeedsStyleRecalc() {
    needs_style_recalc_ = true;
    needs_layout_ = true;  // 样式变化通常需要重新布局
    needs_paint_ = true;
}

void RenderPipelineLegacy::MarkNeedsLayout() {
    needs_layout_ = true;
    needs_paint_ = true;  // 布局变化需要重新绘制
}

void RenderPipelineLegacy::MarkNeedsPaint() {
    needs_paint_ = true;
}

bool RenderPipelineLegacy::NeedsUpdate() const {
    // 检查是否有待处理的 DOM 变化
    if (dirty_tracker_ && dirty_tracker_->HasPendingChanges()) {
        return true;
    }

    return needs_style_recalc_ || needs_layout_ || needs_paint_;
}

void RenderPipelineLegacy::ProcessFrame(SkCanvas* canvas) {
    // 1. 渲染树同步阶段
    if (dirty_tracker_ && dirty_tracker_->HasPendingChanges()) {
        DoRenderTreeSync();
    }

    // 2. 样式重算阶段
    if (needs_style_recalc_) {
        DoStyleRecalc();
    }

    // 3. 布局阶段
    if (needs_layout_) {
        DoLayout();
    }

    // 4. 绘制阶段
    if (needs_paint_ && canvas) {
        DoPaint(canvas);
    }

    // 返回空闲状态
    lifecycle_ = RenderLifecycleLegacy::Idle;
}

void RenderPipelineLegacy::ForceFullUpdate() {
    needs_style_recalc_ = true;
    needs_layout_ = true;
    needs_paint_ = true;
}

void RenderPipelineLegacy::DoRenderTreeSync() {
    lifecycle_ = RenderLifecycleLegacy::RenderTreeSync;

    if (!synchronizer_ || !dirty_tracker_ || !render_tree_) {
        return;
    }

    // 同步渲染树
    bool changed = synchronizer_->Synchronize(*dirty_tracker_, render_tree_);

    if (changed) {
        // 渲染树变化后需要重新布局和绘制
        needs_layout_ = true;
        needs_paint_ = true;
    }
}

void RenderPipelineLegacy::DoStyleRecalc() {
    lifecycle_ = RenderLifecycleLegacy::StyleRecalc;

    // TODO: 实现样式重算逻辑
    // 当前样式在 DOM 操作时已经计算，这里主要处理级联样式变化

    needs_style_recalc_ = false;
}

void RenderPipelineLegacy::DoLayout() {
    lifecycle_ = RenderLifecycleLegacy::Layout;

    if (!layout_engine_ || !render_tree_) {
        needs_layout_ = false;
        return;
    }

    // 执行布局计算
    layout_engine_->ComputeLayout(viewport_width_, viewport_height_);

    needs_layout_ = false;
}

void RenderPipelineLegacy::DoPaint(SkCanvas* canvas) {
    lifecycle_ = RenderLifecycleLegacy::Paint;

    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    // 如果有自定义绘制回调，使用回调
    if (paint_callback_) {
        paint_callback_(canvas);
    } else if (render_tree_) {
        // 否则直接绘制渲染树
        render_tree_->Paint(canvas);
    }

    needs_paint_ = false;
}

} // namespace lightui
