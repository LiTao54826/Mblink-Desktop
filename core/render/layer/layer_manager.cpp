/**
 * @file layer_manager.cpp
 * @brief LayerManager 类实现
 */

#include "layer_manager.h"
#include "core/dom/element.h"
#include "core/event/input/hit_testing.h"
#include <iostream>
#include <cstdlib>

namespace lightui {

// 调试模式：设置环境变量 LIGHTUI_DEBUG_LAYERS=1 启用
static bool IsDebugLayersEnabled() {
    static bool checked = false;
    static bool enabled = false;
    if (!checked) {
        const char* env = std::getenv("LIGHTUI_DEBUG_LAYERS");
        enabled = (env != nullptr && std::string(env) == "1");
        checked = true;
    }
    return enabled;
}

LayerManager& LayerManager::Instance() {
    static LayerManager instance;
    return instance;
}

LayerManager::LayerManager() {
    // 创建三个 Layer：Base, Overlay, Modal
    // Layer 0: Base (z-index < 100)
    layers_.push_back(std::make_unique<Layer>(0, overlay_threshold_ - 1));
    // Layer 1: Overlay (z-index 100-999)
    layers_.push_back(std::make_unique<Layer>(overlay_threshold_, modal_threshold_ - 1));
    // Layer 2: Modal (z-index >= 1000)
    layers_.push_back(std::make_unique<Layer>(modal_threshold_, -1));
}

void LayerManager::BeginFrame() {
    // 关键修复：不在这里清空 Layer
    // Layer 的内容会在下一次 Collect 时被替换
    // 这样 HitTest 可以使用上一帧收集的元素，直到新的渲染完成
    
    // 只设置一个标志，表示新帧开始
    // 实际清空在 PaintLayers 之前进行
    frame_started_ = true;
}

LayerLevel LayerManager::GetLayerLevel(int z_index) const {
    if (z_index >= modal_threshold_) {
        return LayerLevel::Modal;
    } else if (z_index >= overlay_threshold_) {
        return LayerLevel::Overlay;
    }
    return LayerLevel::Base;
}

bool LayerManager::ShouldCollect(const RenderObject* render_obj) const {
    if (!render_obj) return false;
    
    // 如果正在绘制 Layer，不要再收集
    if (painting_layers_) return false;
    
    const auto& style = render_obj->GetComputedStyle();
    
    // 检查是否是 positioned 元素且 z-index >= 阈值
    bool is_positioned = (style.position == "absolute" || 
                          style.position == "fixed" || 
                          style.position == "relative");
    
    bool should = is_positioned && style.z_index >= overlay_threshold_;
    
    return should;
}

void LayerManager::Collect(std::shared_ptr<RenderObject> render_obj, const SkMatrix& transform, int z_index) {
    if (!render_obj) return;
    
    // 延迟清空：在新帧第一次收集时清空所有 Layer
    // 这样 HitTest 可以使用上一帧的元素，直到新的渲染开始收集
    if (frame_started_) {
        for (auto& layer : layers_) {
            layer->Clear();
        }
        frame_started_ = false;
    }
    
    LayerLevel level = GetLayerLevel(z_index);
    int layer_index = static_cast<int>(level);
    
    if (layer_index >= 0 && layer_index < static_cast<int>(layers_.size())) {
        layers_[layer_index]->AddItem(render_obj, transform, z_index);
    }
}


void LayerManager::PaintLayers(SkCanvas* canvas) {
    if (!canvas) return;
    
    // 设置标志，防止递归收集
    painting_layers_ = true;
    
    // 调试输出
    if (IsDebugLayersEnabled()) {
        int overlay_count = layers_[1]->GetItemCount();
        int modal_count = layers_[2]->GetItemCount();
        if (overlay_count > 0 || modal_count > 0) {
            std::cout << "[LayerManager] PaintLayers: Overlay=" << overlay_count 
                      << ", Modal=" << modal_count << std::endl;
        }
    }
    
    // 按 Base -> Overlay -> Modal 顺序绘制
    for (auto& layer : layers_) {
        layer->Paint(canvas);
    }
    
    // 清除标志
    painting_layers_ = false;
}

bool LayerManager::HitTest(float x, float y, HitTestResult& result) {
    // 从最高 Layer (Modal) 到最低 Layer (Overlay) 测试
    // 注意：Base 层不在这里测试，由原有的 HitTesting 处理
    for (int i = static_cast<int>(layers_.size()) - 1; i >= 1; --i) {
        if (layers_[i]->HitTest(x, y, result)) {
            return true;
        }
    }
    return false;
}

bool LayerManager::HandleWheel(float x, float y, float delta_x, float delta_y) {
    // 从最高 Layer (Modal) 到最低 Layer (Overlay) 处理
    // 注意：Base 层不在这里处理
    for (int i = static_cast<int>(layers_.size()) - 1; i >= 1; --i) {
        if (layers_[i]->HandleWheel(x, y, delta_x, delta_y)) {
            return true;  // 事件被处理，阻止传递
        }
    }
    return false;  // 事件未被处理，传递到 Base 层
}

bool LayerManager::HasOverlays() const {
    // 检查 Overlay 层和 Modal 层是否有元素
    for (size_t i = 1; i < layers_.size(); ++i) {
        if (!layers_[i]->IsEmpty()) {
            return true;
        }
    }
    return false;
}

Layer* LayerManager::GetLayer(LayerLevel level) {
    int index = static_cast<int>(level);
    if (index >= 0 && index < static_cast<int>(layers_.size())) {
        return layers_[index].get();
    }
    return nullptr;
}

} // namespace lightui
