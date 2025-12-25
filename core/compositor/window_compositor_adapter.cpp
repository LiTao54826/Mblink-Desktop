/**
 * @file window_compositor_adapter.cpp
 * @brief 窗口合成器适配器实现
 */

#include "window_compositor_adapter.h"
#include "../render/render_object.h"
#include "core/compositor/property_tree/transform_tree_node.h"
#include "core/compositor/property_tree/effect_tree_node.h"
#include "core/compositor/property_tree/scroll_tree_node.h"

namespace lightui {

WindowCompositorAdapter::WindowCompositorAdapter()
    : pipeline_(std::make_unique<RenderPipelineV2>()) {
    // 属性树系统由 RenderPipelineV2 管理，不需要在这里初始化
}

WindowCompositorAdapter::~WindowCompositorAdapter() {
    Shutdown();
}

// =========================================================================
// 初始化
// =========================================================================

bool WindowCompositorAdapter::Initialize(int width, int height) {
    viewport_width_ = width;
    viewport_height_ = height;

    // 配置管线
    RenderPipelineConfig config;
    config.enable_gpu_compositing = true;
    config.enable_layer_promotion = true;
    config.enable_incremental_rasterize = true;
    config.enable_scroll_optimization = true;
    config.enable_animation_optimization = true;
    config.enable_frame_skip = true;
    config.show_layer_borders = false;

    return pipeline_->Initialize(width, height, config);
}

void WindowCompositorAdapter::Resize(int width, int height) {
    if (width == viewport_width_ && height == viewport_height_) {
        return;
    }

    viewport_width_ = width;
    viewport_height_ = height;

    if (pipeline_->IsInitialized()) {
        pipeline_->Resize(width, height);
    }
}

void WindowCompositorAdapter::Shutdown() {
    if (pipeline_) {
        pipeline_->Shutdown();
    }
}

bool WindowCompositorAdapter::IsInitialized() const {
    return pipeline_ && pipeline_->IsInitialized();
}

// =========================================================================
// 模式切换
// =========================================================================

void WindowCompositorAdapter::SetUseLayerCompositing(bool use_layer_compositing) {
    use_layer_compositing_ = use_layer_compositing;

    // 如果启用分层合成但管线未初始化，初始化它
    if (use_layer_compositing && !pipeline_->IsInitialized() && 
        viewport_width_ > 0 && viewport_height_ > 0) {
        Initialize(viewport_width_, viewport_height_);
    }
}

// =========================================================================
// 渲染
// =========================================================================

bool WindowCompositorAdapter::Render(RenderObject* render_tree, SkCanvas* canvas) {
    if (!use_layer_compositing_) {
        return false;  // 让 Window 使用旧渲染路径
    }

    if (!pipeline_->IsInitialized() || !render_tree || !canvas) {
        return false;
    }

    return pipeline_->RenderToCanvas(render_tree, canvas);
}

bool WindowCompositorAdapter::NeedsRender() const {
    if (!use_layer_compositing_ || !pipeline_->IsInitialized()) {
        return true;  // 旧模式总是需要检查
    }
    return pipeline_->NeedsRender();
}

void WindowCompositorAdapter::MarkNeedsRender() {
    if (pipeline_->IsInitialized()) {
        pipeline_->MarkNeedsRender();
    }
}

void WindowCompositorAdapter::ForceRasterize() {
    if (pipeline_->IsInitialized()) {
        pipeline_->ForceRasterize();
    }
}

// =========================================================================
// 滚动处理
// =========================================================================

bool WindowCompositorAdapter::HandleScroll(RenderObject* container, float delta_x, float delta_y) {
    if (!use_layer_compositing_ || !pipeline_->IsInitialized()) {
        return false;
    }
    return pipeline_->HandleScroll(container, delta_x, delta_y);
}

// =========================================================================
// 动画处理
// =========================================================================

void WindowCompositorAdapter::BeginAnimationFrame() {
    if (use_layer_compositing_ && pipeline_->IsInitialized()) {
        pipeline_->BeginAnimationFrame();
    }
}

AnimationUpdateType WindowCompositorAdapter::UpdateAnimationProperty(RenderObject* object,
                                                                       const std::string& property,
                                                                       const std::string& value) {
    if (!use_layer_compositing_ || !pipeline_->IsInitialized()) {
        return AnimationUpdateType::Paint;
    }
    return pipeline_->UpdateAnimationProperty(object, property, value);
}

bool WindowCompositorAdapter::EndAnimationFrame() {
    if (!use_layer_compositing_ || !pipeline_->IsInitialized()) {
        return false;
    }
    return pipeline_->EndAnimationFrame();
}

void WindowCompositorAdapter::OnAnimationStart(RenderObject* object,
                                                const std::string& animation_name,
                                                const std::vector<std::string>& properties) {
    if (use_layer_compositing_ && pipeline_->IsInitialized()) {
        pipeline_->OnAnimationStart(object, animation_name, properties);
    }
}

void WindowCompositorAdapter::OnAnimationEnd(RenderObject* object, const std::string& animation_name) {
    if (use_layer_compositing_ && pipeline_->IsInitialized()) {
        pipeline_->OnAnimationEnd(object, animation_name);
    }
}

// =========================================================================
// 脏区域管理
// =========================================================================

void WindowCompositorAdapter::MarkDirty(RenderObject* object) {
    if (use_layer_compositing_ && pipeline_->IsInitialized()) {
        pipeline_->MarkDirty(object);
    }
}

void WindowCompositorAdapter::MarkDirtyRegion(const SkRect& region) {
    if (use_layer_compositing_ && pipeline_->IsInitialized()) {
        pipeline_->MarkDirtyRegion(region);
    }
}

void WindowCompositorAdapter::InvalidateLayerTree() {
    if (pipeline_->IsInitialized()) {
        pipeline_->InvalidateLayerTree();
    }
}

// =========================================================================
// 配置
// =========================================================================

void WindowCompositorAdapter::SetShowLayerBorders(bool show) {
    if (pipeline_->IsInitialized()) {
        pipeline_->SetShowLayerBorders(show);
    }
}

void WindowCompositorAdapter::SetDpiScale(float scale) {
    if (scale <= 0) {
        scale = 1.0f;
    }
    dpi_scale_ = scale;
    
    if (pipeline_) {
        pipeline_->SetDpiScale(scale);
    }
}

// =========================================================================
// 统计
// =========================================================================

const RenderFrameStats& WindowCompositorAdapter::GetLastFrameStats() const {
    static RenderFrameStats empty_stats;
    if (!pipeline_->IsInitialized()) {
        return empty_stats;
    }
    return pipeline_->GetLastFrameStats();
}

// =========================================================================
// 属性树系统
// =========================================================================

void WindowCompositorAdapter::SetUsePropertyTreeSystem(bool use_property_tree) {
    // 设置 RenderPipelineV2 的属性树系统状态
    if (pipeline_) {
        pipeline_->SetUsePropertyTreeSystem(use_property_tree);
    }
}

bool WindowCompositorAdapter::RenderWithPropertyTrees(RenderObject* render_tree, SkCanvas* canvas) {
    if (!render_tree || !canvas || !pipeline_) {
        return false;
    }
    
    auto* property_trees = pipeline_->GetPropertyTrees();
    auto* property_tree_builder = pipeline_->GetPropertyTreeBuilder();
    auto* paint_artifact_compositor = pipeline_->GetPaintArtifactCompositor();
    
    if (!property_trees || !property_tree_builder || !paint_artifact_compositor) {
        return false;
    }
    
    // 1. 构建属性树
    property_tree_builder->Build(render_tree);
    
    // 2. 生成绘制产物（简化版本 - 实际需要 PaintController）
    PaintArtifact artifact;
    // TODO: 使用 PaintController 生成完整的绘制产物
    
    // 3. 更新合成器
    paint_artifact_compositor->Update(artifact);
    
    // 4. 光栅化脏层
    paint_artifact_compositor->RasterizeDirtyLayers();
    
    // 5. 合成到 Canvas
    SkRect viewport = SkRect::MakeWH(
        static_cast<float>(viewport_width_),
        static_cast<float>(viewport_height_)
    );
    paint_artifact_compositor->CompositeToCanvas(canvas, viewport);
    
    return true;
}

bool WindowCompositorAdapter::DirectlyUpdateTransform(RenderObject* object, const SkM44& matrix) {
    if (!object || !pipeline_) {
        return false;
    }
    
    auto* paint_artifact_compositor = pipeline_->GetPaintArtifactCompositor();
    if (!paint_artifact_compositor) {
        return false;
    }
    
    // 获取对象的变换节点
    PropertyTreeState* state = object->GetPropertyTreeState();
    if (!state) {
        return false;
    }
    
    TransformTreeNode* transform_node = state->Transform();
    if (!transform_node) {
        return false;
    }
    
    // 直接更新变换
    return paint_artifact_compositor->DirectlyUpdateTransform(transform_node, matrix);
}

bool WindowCompositorAdapter::DirectlyUpdateOpacity(RenderObject* object, float opacity) {
    if (!object || !pipeline_) {
        return false;
    }
    
    auto* paint_artifact_compositor = pipeline_->GetPaintArtifactCompositor();
    if (!paint_artifact_compositor) {
        return false;
    }
    
    // 获取对象的效果节点
    PropertyTreeState* state = object->GetPropertyTreeState();
    if (!state) {
        return false;
    }
    
    EffectTreeNode* effect_node = state->Effect();
    if (!effect_node) {
        return false;
    }
    
    // 直接更新透明度
    return paint_artifact_compositor->DirectlyUpdateOpacity(effect_node, opacity);
}

bool WindowCompositorAdapter::DirectlyUpdateScrollOffset(RenderObject* object, const SkPoint& offset) {
    if (!object || !pipeline_) {
        return false;
    }
    
    auto* paint_artifact_compositor = pipeline_->GetPaintArtifactCompositor();
    if (!paint_artifact_compositor) {
        return false;
    }
    
    // 获取对象的滚动节点
    PropertyTreeState* state = object->GetPropertyTreeState();
    if (!state) {
        return false;
    }
    
    ScrollTreeNode* scroll_node = state->Scroll();
    if (!scroll_node) {
        return false;
    }
    
    // 直接更新滚动偏移
    return paint_artifact_compositor->DirectlyUpdateScrollOffset(scroll_node, offset);
}

} // namespace lightui
