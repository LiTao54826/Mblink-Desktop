/**
 * @file animation_layer_bridge.cpp
 * @brief 动画层桥接器实现
 */

#include "animation_layer_bridge.h"
#include "../compositor_layer.h"
#include "../layer_tree_builder.h"
#include "core/render/objects/render_object.h"
#include "core/render/utils/transform.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include <algorithm>
#include <cmath>
#include <regex>

namespace mblink {

AnimationLayerBridge::AnimationLayerBridge() = default;
AnimationLayerBridge::~AnimationLayerBridge() = default;

// =========================================================================
// 动画生命周期
// =========================================================================

void AnimationLayerBridge::OnAnimationStart(RenderObject* object,
                                             const std::string& animation_name,
                                             const std::vector<std::string>& properties) {
    if (!object) {
        return;
    }

    auto& state = animation_states_[object];
    bool needs_promotion = false;

    for (const auto& prop : properties) {
        if (prop == "transform") {
            state.transform_animations.insert(animation_name);
            needs_promotion = true;
        } else if (prop == "opacity") {
            state.opacity_animations.insert(animation_name);
            needs_promotion = true;
        } else {
            state.other_animations.insert(animation_name);
        }
    }

    // 如果有 transform/opacity 动画，请求层提升
    if (needs_promotion && !IsPromotedForAnimation(object)) {
        RequestLayerPromotion(object, "animation");
    }
}

void AnimationLayerBridge::OnAnimationEnd(RenderObject* object, const std::string& animation_name) {
    if (!object) {
        return;
    }

    auto it = animation_states_.find(object);
    if (it == animation_states_.end()) {
        return;
    }

    auto& state = it->second;
    state.transform_animations.erase(animation_name);
    state.opacity_animations.erase(animation_name);
    state.other_animations.erase(animation_name);

    // 如果没有更多的 transform/opacity 动画，考虑降级
    if (state.transform_animations.empty() && state.opacity_animations.empty()) {
        if (IsPromotedForAnimation(object)) {
            RequestLayerDemotion(object);
        }
    }

    // 如果没有任何动画，清理状态
    if (state.transform_animations.empty() &&
        state.opacity_animations.empty() &&
        state.other_animations.empty()) {
        animation_states_.erase(it);
    }
}

// =========================================================================
// 动画更新
// =========================================================================

void AnimationLayerBridge::BeginAnimationUpdates() {
    in_batch_ = true;
    has_layer_updates_ = false;
    pending_updates_.clear();
}

AnimationUpdateType AnimationLayerBridge::ApplyAnimationProperty(RenderObject* object,
                                                                   const std::string& property,
                                                                   const std::string& value) {
    if (!object) {
        return AnimationUpdateType::None;
    }

    // 检查是否可以使用层优化
    bool use_layer_opt = ShouldUseLayerOptimization(object);

    if (property == "transform") {
        if (use_layer_opt && UpdateLayerTransform(object, value)) {
            has_layer_updates_ = true;
            return AnimationUpdateType::Transform;
        }
        // 回退到传统方式
        return AnimationUpdateType::Paint;
    }

    if (property == "opacity") {
        try {
            float opacity = std::stof(value);
            if (use_layer_opt && UpdateLayerOpacity(object, opacity)) {
                has_layer_updates_ = true;
                return AnimationUpdateType::Opacity;
            }
        } catch (...) {
            // 解析失败
        }
        return AnimationUpdateType::Paint;
    }

    // 布局相关属性
    if (property == "width" || property == "height" ||
        property == "margin-top" || property == "margin-right" ||
        property == "margin-bottom" || property == "margin-left" ||
        property == "padding-top" || property == "padding-right" ||
        property == "padding-bottom" || property == "padding-left" ||
        property == "top" || property == "right" ||
        property == "bottom" || property == "left") {
        return AnimationUpdateType::Layout;
    }

    // 其他绘制属性
    return AnimationUpdateType::Paint;
}

bool AnimationLayerBridge::EndAnimationUpdates() {
    in_batch_ = false;
    return has_layer_updates_;
}

// =========================================================================
// 查询
// =========================================================================

bool AnimationLayerBridge::HasTransformAnimation(RenderObject* object) const {
    auto it = animation_states_.find(object);
    if (it == animation_states_.end()) {
        return false;
    }
    return !it->second.transform_animations.empty();
}

bool AnimationLayerBridge::HasOpacityAnimation(RenderObject* object) const {
    auto it = animation_states_.find(object);
    if (it == animation_states_.end()) {
        return false;
    }
    return !it->second.opacity_animations.empty();
}

bool AnimationLayerBridge::IsPromotedForAnimation(RenderObject* object) const {
    return animation_promoted_objects_.find(object) != animation_promoted_objects_.end();
}

// =========================================================================
// 清理
// =========================================================================

void AnimationLayerBridge::Clear() {
    animation_states_.clear();
    animation_promoted_objects_.clear();
    pending_updates_.clear();
    in_batch_ = false;
    has_layer_updates_ = false;
}

// =========================================================================
// 私有方法
// =========================================================================

bool AnimationLayerBridge::ShouldUseLayerOptimization(RenderObject* object) const {
    if (!object) {
        return false;
    }

    // 检查对象是否有独立的合成层
    return object->HasOwnCompositorLayer();
}

bool AnimationLayerBridge::UpdateLayerTransform(RenderObject* object, const std::string& transform_str) {
    if (!object) {
        return false;
    }

    auto layer = object->GetCompositorLayer();
    if (!layer) {
        return false;
    }

    // 解析 transform 字符串
    auto css_transform = CSSTransform::Parse(transform_str);
    if (!css_transform.has_value()) {
        return false;
    }

    // 获取元素尺寸用于百分比计算
    const auto& layout = object->GetLayoutInfo();
    const auto& style = object->GetComputedStyle();
    float width = layout.width;
    float height = layout.height;

    // 转换为 SkMatrix
    SkRect bounds = SkRect::MakeWH(width, height);
    SkMatrix matrix = css_transform->ToSkMatrix(bounds, style.transform_origin);

    // 更新层变换
    layer->SetTransform(matrix);

    // 记录更新
    if (in_batch_) {
        AnimationUpdate update;
        update.object = object;
        update.type = AnimationUpdateType::Transform;
        update.has_transform = true;
        for (int i = 0; i < 9; i++) {
            update.transform_values[i] = matrix[i];
        }
        pending_updates_.push_back(update);
    }

    return true;
}

bool AnimationLayerBridge::UpdateLayerOpacity(RenderObject* object, float opacity) {
    if (!object) {
        return false;
    }

    auto layer = object->GetCompositorLayer();
    if (!layer) {
        return false;
    }

    // 限制范围
    opacity = std::max(0.0f, std::min(1.0f, opacity));

    // 更新层透明度
    layer->SetOpacity(opacity);

    // 记录更新
    if (in_batch_) {
        AnimationUpdate update;
        update.object = object;
        update.type = AnimationUpdateType::Opacity;
        update.has_opacity = true;
        update.opacity = opacity;
        pending_updates_.push_back(update);
    }

    return true;
}

void AnimationLayerBridge::RequestLayerPromotion(RenderObject* object, const std::string& reason) {
    if (!object || !layer_tree_builder_) {
        return;
    }

    // 标记对象需要独立层
    auto& layer_info = object->GetLayerInfo();
    layer_info.force_own_layer = true;

    if (reason == "animation") {
        // 根据动画类型设置提升原因
        if (HasTransformAnimation(object)) {
            layer_info.promotion_reason = LayerPromotionReason::TransformAnimation;
        } else if (HasOpacityAnimation(object)) {
            layer_info.promotion_reason = LayerPromotionReason::OpacityAnimation;
        }
    }

    animation_promoted_objects_.insert(object);

    // 触发层树重建
    // 注意：实际的层创建会在下一次 LayerTreeBuilder::Build() 时发生
}

void AnimationLayerBridge::RequestLayerDemotion(RenderObject* object) {
    if (!object) {
        return;
    }

    // 清除强制层标记
    auto& layer_info = object->GetLayerInfo();
    layer_info.force_own_layer = false;

    // 如果提升原因是动画，清除它
    if (layer_info.promotion_reason == LayerPromotionReason::TransformAnimation ||
        layer_info.promotion_reason == LayerPromotionReason::OpacityAnimation) {
        layer_info.promotion_reason = LayerPromotionReason::None;
    }

    animation_promoted_objects_.erase(object);

    // 触发层树重建
    // 注意：实际的层移除会在下一次 LayerTreeBuilder::Build() 时发生
}

} // namespace mblink
