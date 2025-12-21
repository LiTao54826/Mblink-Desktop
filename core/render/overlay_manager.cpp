/**
 * @file overlay_manager.cpp
 * @brief Overlay 管理器实现
 */

#include "overlay_manager.h"
#include <algorithm>
#include <iostream>

namespace lightui {

OverlayManager& OverlayManager::Instance() {
    static OverlayManager instance;
    return instance;
}

void OverlayManager::BeginFrame() {
    overlays_.clear();
}

void OverlayManager::AddOverlay(std::shared_ptr<RenderObject> render_obj, const SkMatrix& transform, int z_index) {
    if (!render_obj) return;
    
    OverlayItem item;
    item.render_obj = render_obj;
    item.transform = transform;
    item.z_index = z_index;
    overlays_.push_back(item);
}

bool OverlayManager::ShouldDeferPaint(const RenderObject* render_obj) const {
    if (!render_obj) return false;
    
    // 如果正在绘制 overlay，不要再延迟
    if (painting_overlay_) return false;
    
    const auto& style = render_obj->GetComputedStyle();
    
    // 检查是否是 positioned 元素且 z-index >= 阈值
    bool is_positioned = (style.position == "absolute" || 
                          style.position == "fixed" || 
                          style.position == "relative");
    
    return is_positioned && style.z_index >= z_index_threshold_;
}

void OverlayManager::PaintOverlays(SkCanvas* canvas) {
    if (overlays_.empty() || !canvas) return;
    
    // 设置标志，防止递归延迟
    painting_overlay_ = true;
    
    // 按 z-index 排序
    std::sort(overlays_.begin(), overlays_.end(),
        [](const OverlayItem& a, const OverlayItem& b) {
            return a.z_index < b.z_index;
        });
    
    // 绘制所有 overlay 元素
    for (const auto& item : overlays_) {
        canvas->save();
        canvas->setMatrix(item.transform);
        item.render_obj->Paint(canvas);
        canvas->restore();
    }
    
    // 清除标志
    painting_overlay_ = false;
}

} // namespace lightui
