/**
 * @file paint_artifact_compositor.cpp
 * @brief 绘制产物合成器实现
 */

#include "paint_artifact_compositor.h"
#include "core/compositor/compositor_layer.h"
#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include "core/compositor/property_tree/nodes/effect_tree_node.h"
#include "core/compositor/property_tree/nodes/scroll_tree_node.h"
#include "core/compositor/property_tree/paint/paint_chunk.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkPaint.h"
#include "include/core/SkImage.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace {
inline bool IsAnimFinalDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_FINAL") != nullptr);
    return enabled;
}

inline bool IsLayerReuseDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_LAYER_REUSE") != nullptr);
    return enabled;
}

inline bool IsAnimMapDebugEnabled() {
    static const bool enabled = (std::getenv("MBINK_DEBUG_ANIM_MAP") != nullptr);
    return enabled;
}

inline bool ShouldLogSetTransformChanged(int layer_id, const SkMatrix& transform) {
    struct LastTransform {
        float m00 = 0.0f;
        float m01 = 0.0f;
        float m02 = 0.0f;
        float m10 = 0.0f;
        float m11 = 0.0f;
        float m12 = 0.0f;
    };

    static std::unordered_map<int, LastTransform> last_values;
    LastTransform current;
    current.m00 = transform[SkMatrix::kMScaleX];
    current.m01 = transform[SkMatrix::kMSkewX];
    current.m02 = transform[SkMatrix::kMTransX];
    current.m10 = transform[SkMatrix::kMSkewY];
    current.m11 = transform[SkMatrix::kMScaleY];
    current.m12 = transform[SkMatrix::kMTransY];

    auto it = last_values.find(layer_id);
    if (it != last_values.end()) {
        const LastTransform& prev = it->second;
        const float eps = 0.0001f;
        bool same = std::fabs(prev.m00 - current.m00) < eps &&
                    std::fabs(prev.m01 - current.m01) < eps &&
                    std::fabs(prev.m02 - current.m02) < eps &&
                    std::fabs(prev.m10 - current.m10) < eps &&
                    std::fabs(prev.m11 - current.m11) < eps &&
                    std::fabs(prev.m12 - current.m12) < eps;
        if (same) {
            return false;
        }
    }

    last_values[layer_id] = current;
    return true;
}

inline bool ShouldLogSetOpacityChanged(int layer_id, float opacity) {
    static std::unordered_map<int, float> last_opacity;
    auto it = last_opacity.find(layer_id);
    if (it != last_opacity.end()) {
        if (std::fabs(it->second - opacity) < 0.0001f) {
            return false;
        }
    }
    last_opacity[layer_id] = opacity;
    return true;
}
}

namespace mbink {

PaintArtifactCompositor::PaintArtifactCompositor() = default;

PaintArtifactCompositor::~PaintArtifactCompositor() = default;

// =========================================================================
// 初始化和配置
// =========================================================================

void PaintArtifactCompositor::SetPropertyTrees(PropertyTrees* trees) {
    property_trees_ = trees;
    
    if (trees) {
        geometry_mapper_ = std::make_unique<GeometryMapper>(*trees);
        layerizer_ = std::make_unique<Layerizer>(*trees, *geometry_mapper_);
        raster_invalidator_ = std::make_unique<RasterInvalidator>(*geometry_mapper_);
    } else {
        geometry_mapper_.reset();
        layerizer_.reset();
        raster_invalidator_.reset();
    }
    
    needs_full_update_ = true;
}

// =========================================================================
// 更新方法
// =========================================================================

UpdateType PaintArtifactCompositor::Update(const PaintArtifact& artifact) {
    if (!property_trees_ || !layerizer_) {
        return UpdateType::kNone;
    }
    
    // 执行层化
    std::vector<PendingLayer> pending_layers = layerizer_->Layerize(artifact);
    
    // 匹配新旧层
    MatchLayers(pending_layers);
    
    // 更新统计
    statistics_.full_updates++;
    statistics_.total_layers = layers_.size();
    
    // 计算需要光栅化的层数
    statistics_.layers_needing_raster = 0;
    for (const auto& layer : layers_) {
        if (layer->NeedsRasterization()) {
            statistics_.layers_needing_raster++;
        }
    }
    
    // 保存当前状态用于下一帧比较
    previous_artifact_ = std::make_unique<PaintArtifact>();
    // 复制必要的数据用于比较
    for (const auto& chunk : artifact.GetPaintChunks()) {
        previous_artifact_->GetPaintChunks().push_back(chunk);
    }
    previous_pending_layers_ = std::move(pending_layers);
    
    needs_full_update_ = false;
    last_update_type_ = UpdateType::kFullUpdate;
    
    return UpdateType::kFullUpdate;
}

bool PaintArtifactCompositor::TryFastPathUpdate(const PaintArtifact& artifact) {
    if (needs_full_update_ || !previous_artifact_) {
        return false;
    }
    
    // 检查绘制块数量是否相同
    if (artifact.GetPaintChunks().size() != previous_artifact_->GetPaintChunks().size()) {
        return false;
    }
    
    // 检查每个绘制块的属性树状态是否兼容
    const auto& new_chunks = artifact.GetPaintChunks();
    const auto& old_chunks = previous_artifact_->GetPaintChunks();
    
    for (size_t i = 0; i < new_chunks.size(); ++i) {
        const auto& new_state = new_chunks[i].GetState();
        const auto& old_state = old_chunks[i].GetState();
        
        // 如果状态不能合并，需要完整更新
        if (!new_state.CanMergeWith(old_state)) {
            return false;
        }
    }
    
    // 快速路径成功 - 只需要计算光栅化失效区域
    if (raster_invalidator_) {
        PropertyTreeState root_state = property_trees_->GetRootState();
        auto invalidation = raster_invalidator_->ComputeInvalidation(
            *previous_artifact_, artifact, root_state);
        
        // 将失效区域应用到相应的层
        for (const auto& rect : invalidation.rects) {
            // 简化处理：将失效区域应用到所有层
            for (auto& layer : layers_) {
                layer->AddRasterDirtyRect(rect);
            }
        }
    }
    
    // 更新统计
    statistics_.fast_path_updates++;
    
    // 保存当前状态
    previous_artifact_ = std::make_unique<PaintArtifact>();
    // 复制必要的数据用于比较
    for (const auto& chunk : artifact.GetPaintChunks()) {
        previous_artifact_->GetPaintChunks().push_back(chunk);
    }
    
    last_update_type_ = UpdateType::kRepaint;
    return true;
}

// =========================================================================
// 直接属性更新
// =========================================================================

bool PaintArtifactCompositor::DirectlyUpdateTransform(
    TransformTreeNode* transform_node,
    const SkM44& new_matrix) {
    
    if (!transform_node || !CanDirectlyUpdateTransform(transform_node)) {
        return false;
    }
    
    // 更新属性树节点
    transform_node->SetMatrix(new_matrix);
    
    // 查找关联的层并更新其变换
    CompositorLayer* layer = FindLayerForTransformNode(transform_node);
    if (IsAnimMapDebugEnabled()) {
        std::cout << "[ANIM_MAP_DIRECT]"
                  << " transform_node=" << transform_node
                  << " hit_layer=" << (layer ? layer->GetId() : 0)
                  << " hit_ro=" << (layer ? layer->GetRenderObject() : nullptr)
                  << " tx=" << new_matrix.rc(0, 3)
                  << " ty=" << new_matrix.rc(1, 3)
                  << "\n";
    }
    if (layer) {
        // 将 SkM44 转换为 SkMatrix（2D 变换）
        SkMatrix matrix;
        matrix.setAll(
            new_matrix.rc(0, 0), new_matrix.rc(0, 1), new_matrix.rc(0, 3),
            new_matrix.rc(1, 0), new_matrix.rc(1, 1), new_matrix.rc(1, 3),
            new_matrix.rc(3, 0), new_matrix.rc(3, 1), new_matrix.rc(3, 3)
        );
        layer->SetTransform(matrix);

        if (IsAnimFinalDebugEnabled()) {
            const SkMatrix& applied = layer->GetTransform();
            if (ShouldLogSetTransformChanged(layer->GetId(), applied)) {
                std::cout << "[ANIM_FINAL_SET] path=property_tree property=transform"
                          << " layer_id=" << layer->GetId()
                          << " m00=" << applied[SkMatrix::kMScaleX]
                          << " m01=" << applied[SkMatrix::kMSkewX]
                          << " m02=" << applied[SkMatrix::kMTransX]
                          << " m10=" << applied[SkMatrix::kMSkewY]
                          << " m11=" << applied[SkMatrix::kMScaleY]
                          << " m12=" << applied[SkMatrix::kMTransY]
                          << "\n";
            }
        }
    }

    // 更新统计
    statistics_.direct_updates++;
    
    last_update_type_ = UpdateType::kDirectUpdate;
    return true;
}

bool PaintArtifactCompositor::DirectlyUpdateOpacity(
    EffectTreeNode* effect_node,
    float new_opacity) {
    
    if (!effect_node || !CanDirectlyUpdateOpacity(effect_node)) {
        return false;
    }
    
    // 更新属性树节点
    effect_node->SetOpacity(new_opacity);
    
    // 查找关联的层并更新其透明度
    CompositorLayer* layer = FindLayerForEffectNode(effect_node);
    if (layer) {
        layer->SetOpacity(new_opacity);

        if (IsAnimFinalDebugEnabled()) {
            const float opacity = layer->GetOpacity();
            if (ShouldLogSetOpacityChanged(layer->GetId(), opacity)) {
                std::cout << "[ANIM_FINAL_SET] path=property_tree property=opacity"
                          << " layer_id=" << layer->GetId()
                          << " opacity=" << opacity
                          << "\n";
            }
        }
    }

    // 更新统计
    statistics_.direct_updates++;
    
    last_update_type_ = UpdateType::kDirectUpdate;
    return true;
}

bool PaintArtifactCompositor::DirectlyUpdateScrollOffset(
    ScrollTreeNode* scroll_node,
    const SkPoint& new_offset) {
    
    if (!scroll_node || !CanDirectlyUpdateScrollOffset(scroll_node)) {
        return false;
    }
    
    // 更新属性树节点
    scroll_node->SetScrollOffset(new_offset);
    
    // 更新关联的变换节点
    TransformTreeNode* scroll_transform = scroll_node->GetScrollTransformNode();
    if (scroll_transform) {
        // 创建滚动变换矩阵
        SkM44 scroll_matrix = SkM44::Translate(-new_offset.fX, -new_offset.fY, 0);
        scroll_transform->SetMatrix(scroll_matrix);
    }
    
    // 查找关联的层并更新其滚动偏移
    CompositorLayer* layer = FindLayerForScrollNode(scroll_node);
    if (layer) {
        layer->SetScrollOffset(new_offset);
    }
    
    // 更新统计
    statistics_.direct_updates++;
    
    last_update_type_ = UpdateType::kDirectUpdate;
    return true;
}

bool PaintArtifactCompositor::CanDirectlyUpdateTransform(
    const TransformTreeNode* node) const {
    
    if (!node) {
        return false;
    }
    
    // 检查节点是否支持直接更新
    return node->CanDirectlyUpdate();
}

bool PaintArtifactCompositor::CanDirectlyUpdateOpacity(
    const EffectTreeNode* node) const {
    
    if (!node) {
        return false;
    }
    
    // 检查节点是否支持直接更新透明度
    return node->CanDirectlyUpdateOpacity();
}

bool PaintArtifactCompositor::CanDirectlyUpdateScrollOffset(
    const ScrollTreeNode* node) const {
    
    if (!node) {
        return false;
    }
    
    // 检查节点是否支持合成器线程滚动
    return node->CanCompositorScroll();
}


// =========================================================================
// 层管理
// =========================================================================

std::shared_ptr<CompositorLayer> PaintArtifactCompositor::GetLayerForRenderObject(
    RenderObject* obj) const {
    
    for (const auto& layer : layers_) {
        if (layer->GetRenderObject() == obj) {
            return layer;
        }
    }
    return nullptr;
}

std::shared_ptr<CompositorLayer> PaintArtifactCompositor::GetLayerForState(
    const PropertyTreeState& state) const {
    
    for (const auto& layer : layers_) {
        if (layer->GetPropertyTreeState() == state) {
            return layer;
        }
    }
    return nullptr;
}

void PaintArtifactCompositor::ClearLayers() {
    layers_.clear();
    transform_to_layer_.clear();
    effect_to_layer_.clear();
    scroll_to_layer_.clear();
    previous_artifact_.reset();
    previous_pending_layers_.clear();
    needs_full_update_ = true;
}

// =========================================================================
// 光栅化和合成
// =========================================================================

std::vector<CompositorLayer*> PaintArtifactCompositor::GetLayersNeedingRasterization() const {
    std::vector<CompositorLayer*> result;
    for (const auto& layer : layers_) {
        if (layer->NeedsRasterization()) {
            result.push_back(layer.get());
        }
    }
    return result;
}

bool PaintArtifactCompositor::HasLayersNeedingRasterization() const {
    for (const auto& layer : layers_) {
        if (layer->NeedsRasterization()) {
            return true;
        }
    }
    return false;
}

int PaintArtifactCompositor::RasterizeDirtyLayers(SkCanvas* /*canvas*/) {
    int count = 0;
    
    for (auto& layer : layers_) {
        if (!layer->NeedsRasterization()) {
            continue;
        }
        
        // 确保位图已分配
        if (!layer->EnsureBitmap()) {
            continue;
        }
        
        SkCanvas* layer_canvas = layer->GetCanvas();
        if (!layer_canvas) {
            continue;
        }
        
        // 检查是否需要完整光栅化
        if (layer->NeedsFullRaster()) {
            // 清除整个位图
            layer_canvas->clear(SK_ColorTRANSPARENT);
            
            // 绘制所有绘制块
            RasterizeLayerChunks(layer.get());
            
            // 标记纹理需要更新
            layer->MarkTextureDirty(SkIRect::MakeWH(
                static_cast<int>(layer->GetBounds().width()),
                static_cast<int>(layer->GetBounds().height())
            ));
            
            layer->ClearNeedsFullRaster();
        } else {
            // 增量光栅化 - 只重绘脏区域
            const auto& dirty_rects = layer->GetRasterDirtyRects();
            for (const auto& rect : dirty_rects) {
                layer_canvas->save();
                layer_canvas->clipRect(rect);
                
                // 清除区域
                SkPaint clear_paint;
                clear_paint.setColor(SK_ColorTRANSPARENT);
                clear_paint.setBlendMode(SkBlendMode::kSrc);
                layer_canvas->drawRect(rect, clear_paint);
                
                // 重绘该区域的绘制块
                RasterizeLayerChunks(layer.get());
                
                layer_canvas->restore();
                
                // 标记纹理脏区域
                layer->MarkTextureDirty(rect.roundOut());
            }
        }
        
        // 清除光栅化脏区域
        layer->ClearRasterDirtyRects();
        layer->ClearDirtyRegions();
        
        count++;
    }
    
    return count;
}

void PaintArtifactCompositor::CompositeToCanvas(SkCanvas* canvas, const SkRect& viewport) {
    if (!canvas) {
        return;
    }
    
    // 按顺序合成所有层
    for (const auto& layer : layers_) {
        // 检查层是否在视口内
        const SkRect& bounds = layer->GetBounds();
        if (!bounds.intersects(viewport)) {
            continue;
        }
        
        // 保存 Canvas 状态
        canvas->save();
        
        // 应用层变换
        const SkMatrix& transform = layer->GetTransform();
        if (!transform.isIdentity()) {
            canvas->concat(transform);
        }
        
        // 应用滚动偏移
        const SkPoint& scroll = layer->GetScrollOffset();
        if (scroll.fX != 0 || scroll.fY != 0) {
            canvas->translate(-scroll.fX, -scroll.fY);
        }
        
        // 应用透明度
        float opacity = layer->GetOpacity();
        SkPaint paint;
        if (opacity < 1.0f) {
            paint.setAlphaf(opacity);
        }
        
        // 绘制层位图
        const SkBitmap& bitmap = layer->GetBitmap();
        if (!bitmap.isNull()) {
            canvas->drawImage(bitmap.asImage(), bounds.left(), bounds.top(), 
                             SkSamplingOptions(), opacity < 1.0f ? &paint : nullptr);
        }
        
        // 恢复 Canvas 状态
        canvas->restore();
    }
}

bool PaintArtifactCompositor::CompositeToGPU() {
    // GPU 合成需要 OpenGL 上下文
    // 这里只是一个框架，实际实现需要 GPU 渲染代码
    
    for (const auto& layer : layers_) {
        // 确保纹理已创建
        if (!layer->HasTexture()) {
            if (!layer->CreateTexture()) {
                continue;
            }
        }
        
        // 上传脏区域
        if (layer->IsTextureDirty()) {
            layer->UploadDirtyRegions();
        }
    }
    
    // 实际的 GPU 合成在这里进行
    // 需要设置着色器、绑定纹理、绘制四边形等
    
    return true;
}

int PaintArtifactCompositor::UploadDirtyTextures() {
    int count = 0;
    
    for (auto& layer : layers_) {
        if (!layer->IsTextureDirty()) {
            continue;
        }
        
        // 确保纹理已创建
        if (!layer->HasTexture()) {
            if (!layer->CreateTexture()) {
                continue;
            }
        }
        
        // 上传脏区域
        if (layer->UploadDirtyRegions()) {
            count++;
        }
    }
    
    return count;
}

// =========================================================================
// 调试支持
// =========================================================================

void PaintArtifactCompositor::ResetStatistics() {
    statistics_ = Statistics{};
}

// =========================================================================
// 内部方法
// =========================================================================

std::shared_ptr<CompositorLayer> PaintArtifactCompositor::CreateLayerFromPendingLayer(
    const PendingLayer& pending_layer) {
    
    auto layer = CreateCompositorLayer();
    
    // 设置属性树状态
    layer->SetPropertyTreeState(pending_layer.GetState());
    
    // 设置合成原因
    layer->SetCompositingReasons(pending_layer.GetCompositingReasons());
    
    // 设置边界
    layer->SetBounds(pending_layer.GetBounds());
    
    // 设置绘制块引用
    std::vector<const PaintChunk*> chunks;
    for (const auto* chunk : pending_layer.GetChunks()) {
        chunks.push_back(chunk);
    }

    // 关键：绑定层与 RenderObject，供后续稳定复用（MatchLayers fallback）
    RenderObject* render_object = nullptr;
    if (!chunks.empty() && chunks[0]) {
        render_object = chunks[0]->GetRenderObject();
    }
    layer->SetRenderObject(render_object);

    layer->SetPaintChunks(std::move(chunks));
    
    // 标记需要完整光栅化
    layer->MarkNeedsFullRaster();
    
    // 建立节点到层的映射
    const auto& state = pending_layer.GetState();
    if (state.Transform()) {
        transform_to_layer_[state.Transform()] = layer.get();
    }
    if (state.Effect()) {
        effect_to_layer_[state.Effect()] = layer.get();
    }
    if (state.Scroll()) {
        scroll_to_layer_[state.Scroll()] = layer.get();
    }
    
    return layer;
}

void PaintArtifactCompositor::UpdateExistingLayer(
    CompositorLayer* layer,
    const PendingLayer& pending_layer) {
    
    if (!layer) {
        return;
    }
    
    // 更新属性树状态
    layer->SetPropertyTreeState(pending_layer.GetState());
    
    // 更新合成原因
    layer->SetCompositingReasons(pending_layer.GetCompositingReasons());
    
    // 检查边界是否变化
    const SkRect& old_bounds = layer->GetBounds();
    const SkRect& new_bounds = pending_layer.GetBounds();
    
    if (old_bounds != new_bounds) {
        layer->SetBounds(new_bounds);
        // 边界变化需要重新光栅化
        layer->MarkNeedsFullRaster();
    }
    
    // 更新绘制块引用
    std::vector<const PaintChunk*> chunks;
    for (const auto* chunk : pending_layer.GetChunks()) {
        chunks.push_back(chunk);
    }

    // 关键：同步更新 RenderObject 关联，避免 fallback 复用键失效
    RenderObject* render_object = nullptr;
    if (!chunks.empty() && chunks[0]) {
        render_object = chunks[0]->GetRenderObject();
    }
    layer->SetRenderObject(render_object);

    layer->SetPaintChunks(std::move(chunks));
}

void PaintArtifactCompositor::MatchLayers(
    const std::vector<PendingLayer>& new_pending_layers) {

    // 清除旧的映射
    transform_to_layer_.clear();
    effect_to_layer_.clear();
    scroll_to_layer_.clear();

    // 构建旧层索引，尽量按稳定键复用，避免 layer id 持续增长
    std::unordered_map<const TransformTreeNode*, std::shared_ptr<CompositorLayer>> old_by_transform;
    std::unordered_map<const EffectTreeNode*, std::shared_ptr<CompositorLayer>> old_by_effect;
    std::unordered_map<const ScrollTreeNode*, std::shared_ptr<CompositorLayer>> old_by_scroll;
    std::unordered_map<RenderObject*, std::shared_ptr<CompositorLayer>> old_by_render_object;

    for (const auto& layer : layers_) {
        if (!layer) continue;
        const auto& old_state = layer->GetPropertyTreeState();
        if (old_state.Transform()) old_by_transform[old_state.Transform()] = layer;
        if (old_state.Effect()) old_by_effect[old_state.Effect()] = layer;
        if (old_state.Scroll()) old_by_scroll[old_state.Scroll()] = layer;

        if (RenderObject* obj = layer->GetRenderObject()) {
            old_by_render_object[obj] = layer;
        }
    }

    std::unordered_set<CompositorLayer*> used_layers;
    std::vector<std::shared_ptr<CompositorLayer>> new_layers;
    new_layers.reserve(new_pending_layers.size());

    for (const auto& pending_layer : new_pending_layers) {
        const auto& state = pending_layer.GetState();
        std::shared_ptr<CompositorLayer> matched;
        const char* match_stage = "none";

        // 1) 优先按属性树节点复用
        if (!matched && state.Transform()) {
            auto it = old_by_transform.find(state.Transform());
            if (it != old_by_transform.end() && !used_layers.count(it->second.get())) {
                matched = it->second;
                match_stage = "transform_node";
            }
        }
        if (!matched && state.Effect()) {
            auto it = old_by_effect.find(state.Effect());
            if (it != old_by_effect.end() && !used_layers.count(it->second.get())) {
                matched = it->second;
                match_stage = "effect_node";
            }
        }
        if (!matched && state.Scroll()) {
            auto it = old_by_scroll.find(state.Scroll());
            if (it != old_by_scroll.end() && !used_layers.count(it->second.get())) {
                matched = it->second;
                match_stage = "scroll_node";
            }
        }

        // 2) 回退：按第一个 chunk 的 RenderObject 复用
        if (!matched) {
            const auto& chunks = pending_layer.GetChunks();
            if (!chunks.empty() && chunks[0]) {
                RenderObject* obj = chunks[0]->GetRenderObject();
                if (obj) {
                    auto it = old_by_render_object.find(obj);
                    if (it != old_by_render_object.end() && !used_layers.count(it->second.get())) {
                        matched = it->second;
                        match_stage = "render_object";
                    }
                }
            }
        }

        // 3) 回退：按几何特征 + compositing reasons 复用（应对 RenderObject/节点重建）
        if (!matched) {
            const SkRect& new_bounds = pending_layer.GetBounds();
            const auto new_reasons = pending_layer.GetCompositingReasons();

            float best_score = -1.0f;
            std::shared_ptr<CompositorLayer> best_layer;

            for (const auto& old_layer : layers_) {
                if (!old_layer || used_layers.count(old_layer.get())) {
                    continue;
                }

                // 优先约束：合成原因一致
                if (old_layer->GetCompositingReasons() != new_reasons) {
                    continue;
                }

                const SkRect& old_bounds = old_layer->GetBounds();

                // 先做快速尺寸过滤
                const float w_diff = std::fabs(old_bounds.width() - new_bounds.width());
                const float h_diff = std::fabs(old_bounds.height() - new_bounds.height());
                if (w_diff > 1.0f || h_diff > 1.0f) {
                    continue;
                }

                // 计算重叠率作为评分
                SkRect intersection;
                if (!intersection.intersect(old_bounds, new_bounds)) {
                    continue;
                }

                const float inter_area = intersection.width() * intersection.height();
                const float old_area = std::max(1.0f, old_bounds.width() * old_bounds.height());
                const float new_area = std::max(1.0f, new_bounds.width() * new_bounds.height());
                const float score = inter_area / std::max(old_area, new_area);

                if (score > best_score) {
                    best_score = score;
                    best_layer = old_layer;
                }
            }

            if (best_layer && best_score > 0.6f) {
                matched = best_layer;
                match_stage = "geometry";
            }
        }

        if (matched) {
            UpdateExistingLayer(matched.get(), pending_layer);
            used_layers.insert(matched.get());
            new_layers.push_back(matched);
        } else {
            new_layers.push_back(CreateLayerFromPendingLayer(pending_layer));

            if (IsLayerReuseDebugEnabled()) {
                const auto& chunks = pending_layer.GetChunks();
                RenderObject* obj = (!chunks.empty() && chunks[0]) ? chunks[0]->GetRenderObject() : nullptr;
                const SkRect& b = pending_layer.GetBounds();
                std::cout << "[LAYER_REUSE_MISS]"
                          << " reason=create_new"
                          << " t=" << (state.Transform() ? 1 : 0)
                          << " e=" << (state.Effect() ? 1 : 0)
                          << " s=" << (state.Scroll() ? 1 : 0)
                          << " ro=" << (obj ? 1 : 0)
                          << " chunk_count=" << chunks.size()
                          << " comp_reasons=" << static_cast<uint32_t>(pending_layer.GetCompositingReasons())
                          << " bounds=" << b.left() << "," << b.top() << "," << b.width() << "x" << b.height()
                          << "\n";
            }
        }

        if (IsLayerReuseDebugEnabled() && matched) {
            std::cout << "[LAYER_REUSE_HIT]"
                      << " stage=" << match_stage
                      << " layer_id=" << matched->GetId()
                      << "\n";
        }

        // 重建映射（使用 new_layers 最后一个）
        CompositorLayer* layer_ptr = new_layers.back().get();
        if (state.Transform()) {
            auto it = transform_to_layer_.find(state.Transform());
            if (IsAnimMapDebugEnabled() && it != transform_to_layer_.end() && it->second != layer_ptr) {
                std::cout << "[ANIM_MAP_OVERWRITE]"
                          << " kind=transform"
                          << " node=" << state.Transform()
                          << " old_layer=" << (it->second ? it->second->GetId() : 0)
                          << " old_ro=" << (it->second ? it->second->GetRenderObject() : nullptr)
                          << " new_layer=" << layer_ptr->GetId()
                          << " new_ro=" << layer_ptr->GetRenderObject()
                          << "\n";
            }
            transform_to_layer_[state.Transform()] = layer_ptr;
        }
        if (state.Effect()) effect_to_layer_[state.Effect()] = layer_ptr;
        if (state.Scroll()) scroll_to_layer_[state.Scroll()] = layer_ptr;
    }

    layers_ = std::move(new_layers);
}

void PaintArtifactCompositor::ComputeLayerInvalidation(
    CompositorLayer* layer,
    const PendingLayer& pending_layer) {
    
    if (!layer || !raster_invalidator_) {
        return;
    }
    
    // 获取层的属性树状态
    const PropertyTreeState& layer_state = layer->GetPropertyTreeState();
    
    // 计算每个绘制块的失效区域
    for (const auto* chunk : pending_layer.GetChunks()) {
        if (!chunk) continue;
        
        // 获取绘制块的光栅化失效区域
        const auto& invalidation_rects = chunk->GetRasterInvalidationRects();
        for (const auto& rect : invalidation_rects) {
            // 将失效区域转换到层坐标系
            if (geometry_mapper_) {
                SkRect mapped_rect = geometry_mapper_->MapRect(
                    rect, chunk->GetState(), layer_state);
                layer->AddRasterDirtyRect(mapped_rect);
            } else {
                layer->AddRasterDirtyRect(rect);
            }
        }
    }
}

CompositorLayer* PaintArtifactCompositor::FindLayerForTransformNode(
    const TransformTreeNode* node) const {
    
    auto it = transform_to_layer_.find(node);
    if (it != transform_to_layer_.end()) {
        return it->second;
    }
    return nullptr;
}

CompositorLayer* PaintArtifactCompositor::FindLayerForEffectNode(
    const EffectTreeNode* node) const {
    
    auto it = effect_to_layer_.find(node);
    if (it != effect_to_layer_.end()) {
        return it->second;
    }
    return nullptr;
}

CompositorLayer* PaintArtifactCompositor::FindLayerForScrollNode(
    const ScrollTreeNode* node) const {
    
    auto it = scroll_to_layer_.find(node);
    if (it != scroll_to_layer_.end()) {
        return it->second;
    }
    return nullptr;
}

void PaintArtifactCompositor::RasterizeLayerChunks(CompositorLayer* layer) {
    if (!layer) {
        return;
    }
    
    SkCanvas* canvas = layer->GetCanvas();
    if (!canvas) {
        return;
    }
    
    const auto& chunks = layer->GetPaintChunks();
    const PropertyTreeState& layer_state = layer->GetPropertyTreeState();
    
    for (const auto* chunk : chunks) {
        if (!chunk) {
            continue;
        }
        
        // 保存 Canvas 状态
        canvas->save();
        
        // 计算从绘制块状态到层状态的变换
        if (geometry_mapper_) {
            // 关键修复：GetClipRect(source, target) 返回的是 target(层)坐标系中的裁剪区域
            // 当前 canvas 尚未 concat 时正处于层坐标系，因此必须先 clip 再 concat。
            // 否则会把 layer 坐标系的 clip 当作 chunk 坐标系使用，导致布局变化时裁剪异常。
            SkRect clip_rect = geometry_mapper_->GetClipRect(
                chunk->GetState(), layer_state);
            if (!clip_rect.isEmpty()) {
                canvas->clipRect(clip_rect);
            }

            SkM44 transform = geometry_mapper_->GetTransformMatrix(
                chunk->GetState(), layer_state);

            // 应用变换（转换为 SkMatrix）
            SkMatrix matrix;
            matrix.setAll(
                transform.rc(0, 0), transform.rc(0, 1), transform.rc(0, 3),
                transform.rc(1, 0), transform.rc(1, 1), transform.rc(1, 3),
                transform.rc(3, 0), transform.rc(3, 1), transform.rc(3, 3)
            );
            canvas->concat(matrix);
        }
        
        // 绘制绘制块关联的 RenderObject
        RenderObject* render_obj = chunk->GetRenderObject();
        if (render_obj) {
            render_obj->Paint(canvas);
        }
        
        // 恢复 Canvas 状态
        canvas->restore();
    }
}

} // namespace mbink
