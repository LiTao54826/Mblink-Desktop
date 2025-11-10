/**
 * @file layer.cpp
 * @brief 渲染层级系统实现
 */

#include "layer.h"
#include "render_object.h"
#include "include/core/SkPaint.h"
#include <algorithm>

namespace lightui {

// ========== Layer 实现 ==========

Layer::Layer(LayerType type, int z_index)
    : type_(type)
    , z_index_(z_index)
    , bounds_(SkRect::MakeEmpty())
    , needs_repaint_(true)
    , visible_(true)
    , opacity_(1.0f)
    , clip_rect_(SkRect::MakeEmpty())
    , has_clip_(false) {
}

void Layer::AddRenderObject(std::shared_ptr<RenderObject> render_object) {
    if (!render_object) return;
    
    render_objects_.push_back(render_object);
    MarkNeedsRepaint();
}

void Layer::RemoveRenderObject(std::shared_ptr<RenderObject> render_object) {
    if (!render_object) return;
    
    auto it = std::find(render_objects_.begin(), render_objects_.end(), render_object);
    if (it != render_objects_.end()) {
        render_objects_.erase(it);
        MarkNeedsRepaint();
    }
}

void Layer::ClearRenderObjects() {
    render_objects_.clear();
    MarkNeedsRepaint();
}

void Layer::UpdateBounds() {
    if (render_objects_.empty()) {
        bounds_ = SkRect::MakeEmpty();
        return;
    }

    // 计算所有渲染对象的边界
    bounds_ = SkRect::MakeEmpty();
    for (const auto& obj : render_objects_) {
        const auto& layout = obj->GetLayoutInfo();
        if (layout.is_laid_out) {
            SkRect obj_bounds = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
            bounds_.join(obj_bounds);
        }
    }
}

void Layer::CreateSurface(int width, int height) {
    if (width <= 0 || height <= 0) return;
    
    surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));
    MarkNeedsRepaint();
}

void Layer::ReleaseSurface() {
    surface_ = nullptr;
}

void Layer::Paint(SkCanvas* canvas) {
    if (!visible_ || render_objects_.empty()) {
        return;
    }

    canvas->save();

    // 应用裁剪
    if (has_clip_) {
        canvas->clipRect(clip_rect_);
    }

    // 应用透明度
    if (opacity_ < 1.0f) {
        SkPaint paint;
        paint.setAlphaf(opacity_);
        canvas->saveLayer(nullptr, &paint);
    }

    // 如果有缓存的表面，直接绘制
    if (surface_ && !needs_repaint_) {
        auto image = surface_->makeImageSnapshot();
        canvas->drawImage(image, bounds_.left(), bounds_.top());
    } else {
        // 否则直接绘制渲染对象
        for (const auto& obj : render_objects_) {
            obj->Paint(canvas);
        }
    }

    // 恢复透明度
    if (opacity_ < 1.0f) {
        canvas->restore();
    }

    canvas->restore();
}

void Layer::PaintToSurface() {
    if (!surface_ || render_objects_.empty()) {
        return;
    }

    auto canvas = surface_->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    // 平移画布，使渲染对象绘制在正确的位置
    canvas->save();
    canvas->translate(-bounds_.left(), -bounds_.top());

    // 绘制所有渲染对象
    for (const auto& obj : render_objects_) {
        obj->Paint(canvas);
    }

    canvas->restore();

    needs_repaint_ = false;
}

// ========== LayerManager 实现 ==========

std::shared_ptr<Layer> LayerManager::CreateLayer(LayerType type, int z_index) {
    auto layer = std::make_shared<Layer>(type, z_index);
    AddLayer(layer);
    return layer;
}

void LayerManager::AddLayer(std::shared_ptr<Layer> layer) {
    if (!layer) return;
    
    layers_.push_back(layer);
    SortLayers();
}

void LayerManager::RemoveLayer(std::shared_ptr<Layer> layer) {
    if (!layer) return;
    
    auto it = std::find(layers_.begin(), layers_.end(), layer);
    if (it != layers_.end()) {
        layers_.erase(it);
    }
}

std::shared_ptr<Layer> LayerManager::FindLayerById(const std::string& id) {
    for (const auto& layer : layers_) {
        if (layer->GetId() == id) {
            return layer;
        }
    }
    return nullptr;
}

void LayerManager::ClearLayers() {
    layers_.clear();
}

void LayerManager::SortLayers() {
    // 根据 z-index 排序（从小到大）
    std::sort(layers_.begin(), layers_.end(), [](const auto& a, const auto& b) {
        return a->GetZIndex() < b->GetZIndex();
    });
}

void LayerManager::Composite(SkCanvas* canvas) {
    // 按 z-index 顺序绘制所有层级
    for (const auto& layer : layers_) {
        if (!layer->IsVisible()) {
            continue;
        }

        // 如果层级需要重绘且有表面，先绘制到表面
        if (layer->NeedsRepaint() && layer->HasSurface()) {
            layer->PaintToSurface();
        }

        // 绘制层级到画布
        layer->Paint(canvas);
    }
}

void LayerManager::CompositeRegion(SkCanvas* canvas, const SkRect& region) {
    // 只绘制与指定区域相交的层级
    for (const auto& layer : layers_) {
        if (!layer->IsVisible()) {
            continue;
        }

        // 检查层级边界是否与区域相交
        if (!SkRect::Intersects(layer->GetBounds(), region)) {
            continue;
        }

        // 如果层级需要重绘且有表面，先绘制到表面
        if (layer->NeedsRepaint() && layer->HasSurface()) {
            layer->PaintToSurface();
        }

        // 绘制层级到画布
        layer->Paint(canvas);
    }
}

} // namespace lightui

