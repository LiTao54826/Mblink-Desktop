/**
 * @file animation_applicator.cpp
 * @brief 动画应用器实现
 */

#include "animation_applicator.h"
#include "core/render/utils/color.h"
#include "core/render/utils/transform.h"
#include "core/compositor/animation/animation_layer_bridge.h"
#include "core/compositor/property_tree/paint/paint_artifact_compositor.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include "core/compositor/property_tree/nodes/effect_tree_node.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "core/dom/element.h"
#include "include/core/SkM44.h"
#include <regex>
#include <sstream>
#include <cmath>
#include <iostream>

namespace lightui {

// ============================================================================
// 构造/析构
// ============================================================================

AnimationApplicator::AnimationApplicator(AnimationController& controller)
    : controller_(controller) {
}

AnimationApplicator::~AnimationApplicator() {
}

// ============================================================================
// 动画启动
// ============================================================================

void AnimationApplicator::StartAnimationsForObject(RenderObject* object) {
    if (!object) {
        return;
    }

    // 从 RenderObject 提取 Element
    Element* element = ExtractElement(object);
    if (!element) {
        return;
    }

    const auto& style = object->GetComputedStyle();
    auto& started = started_animations_[element];  // 使用 Element* 作为键

    // 调试日志
    static bool debug_anim = std::getenv("LIGHTUI_DEBUG_ANIM") != nullptr;

    // 遍历 ComputedStyle 中定义的所有动画
    for (const auto& anim : style.animations) {
        if (!anim.IsValid()) {
            if (debug_anim && !anim.name.empty()) {
                std::cout << "[AnimationApplicator] Invalid animation: " << anim.name << std::endl;
            }
            continue;
        }

        // 检查是否已启动
        if (started.find(anim.name) != started.end()) {
            continue;
        }

        if (debug_anim) {
            std::cout << "[AnimationApplicator] Starting animation: " << anim.name
                      << " duration=" << anim.duration << "s"
                      << " iteration=" << anim.iteration_count
                      << std::endl;
        }

        // 启动动画
        controller_.StartAnimation(object, anim);
        started.insert(anim.name);
    }
    
    // 清理不再需要的动画
    std::set<std::string> current_names;
    for (const auto& anim : style.animations) {
        if (anim.IsValid()) {
            current_names.insert(anim.name);
        }
    }
    
    // 停止不在当前样式中的动画
    std::vector<std::string> to_remove;
    for (const auto& name : started) {
        if (current_names.find(name) == current_names.end()) {
            controller_.StopAnimation(object, name);
            to_remove.push_back(name);
        }
    }
    
    for (const auto& name : to_remove) {
        started.erase(name);
    }
}

// ============================================================================
// 动画值应用
// ============================================================================

void AnimationApplicator::ApplyAnimationValues(RenderObject* object) {
    if (!object) {
        return;
    }
    
    // 从 RenderObject 提取 Element
    Element* element = ExtractElement(object);
    if (!element) {
        return;
    }
    
    auto& style = object->GetComputedStyle();
    bool modified = false;
    bool needs_paint = false;  // 是否需要重绘（非层优化的属性）
    
    // 检测 play-state 变化并更新动画状态
    bool should_pause = (style.animation_play_state == "paused");
    auto& started = started_animations_[element];  // 使用 Element* 作为键
    
    if (started.empty()) {
        return;  // 没有已启动的动画，直接返回
    }
    
    for (const auto& anim_name : started) {
        // 检查当前动画的暂停状态
        const auto& running_anims = controller_.GetRunningAnimations();
        
        for (const auto& running : running_anims) {
            // 使用 GetRenderObject() 获取当前 RenderObject
            if (running.GetRenderObject() == object && running.config.name == anim_name) {
                bool is_paused = (running.state == CSSAnimationState::PAUSED);
                if (should_pause && !is_paused) {
                    controller_.PauseAnimation(object, anim_name);
                } else if (!should_pause && is_paused) {
                    controller_.ResumeAnimation(object, anim_name);
                }
                break;
            }
        }
        
        // 获取当前动画属性值
        auto props = controller_.GetCurrentProperties(object, anim_name);
        if (!props) {
            continue;
        }
        
        // 应用每个属性
        for (const auto& [property, value] : *props) {
            // 优先级 1：尝试通过属性树系统直接更新（最高效，不触发光栅化）
            if (property == "transform" || property == "opacity") {
                if (TryApplyViaPropertyTree(object, property, value)) {
                    ApplyPropertyToStyle(style, property, value);
                    modified = true;
                    needs_paint = true;
                    continue;
                }
            }
            
            // 优先级 2：尝试通过层合成系统更新（次优，可能触发部分更新）
            if (property == "transform" || property == "opacity") {
                if (TryApplyViaCompositor(object, property, value)) {
                    ApplyPropertyToStyle(style, property, value);
                    modified = true;
                    needs_paint = true;
                    continue;
                }
            }
            
            // 回退：通过传统方式应用属性
            if (ApplyPropertyToStyle(style, property, value)) {
                modified = true;
                needs_paint = true;
            }
        }
    }
    
    // 如果有属性被修改，标记需要重绘
    if (modified && needs_paint) {
        // 对于 transform 动画，需要扩展脏区域以覆盖变换前后的区域
        // 简单的解决方案：标记父元素也需要重绘，这样可以确保整个区域被正确重绘
        object->MarkNeedsPaint();
        object->InvalidatePaintCache();

        // 如果有 transform，也标记父元素需要重绘
        // 这确保了变换前的位置也会被清除
        if ((style.transform.has_value() && !style.transform->IsEmpty()) || !style.transform_str.empty()) {
            if (auto parent = object->GetParent()) {
                parent->MarkNeedsPaint();
            }
        }
    }
}

// ============================================================================
// 动画控制
// ============================================================================

void AnimationApplicator::StopAnimationsForObject(RenderObject* object) {
    if (!object) {
        return;
    }
    
    // 从 RenderObject 提取 Element
    Element* element = ExtractElement(object);
    if (!element) {
        return;
    }
    
    controller_.StopAllAnimations(object);
    started_animations_.erase(element);  // 使用 Element* 作为键
}

void AnimationApplicator::SetAnimationsPaused(RenderObject* object, bool paused) {
    if (!object) {
        return;
    }
    
    // 从 RenderObject 提取 Element
    Element* element = ExtractElement(object);
    if (!element) {
        return;
    }
    
    auto it = started_animations_.find(element);  // 使用 Element* 作为键
    if (it == started_animations_.end()) {
        return;
    }
    
    for (const auto& name : it->second) {
        if (paused) {
            controller_.PauseAnimation(object, name);
        } else {
            controller_.ResumeAnimation(object, name);
        }
    }
}

bool AnimationApplicator::HasActiveAnimations(RenderObject* object) const {
    if (!object) {
        return false;
    }
    
    // 从 RenderObject 提取 Element
    Element* element = const_cast<AnimationApplicator*>(this)->ExtractElement(object);
    if (!element) {
        return false;
    }
    
    auto it = started_animations_.find(element);  // 使用 Element* 作为键
    return it != started_animations_.end() && !it->second.empty();
}

std::set<std::string> AnimationApplicator::GetActiveAnimationNames(RenderObject* object) const {
    if (!object) {
        return {};
    }
    
    // 从 RenderObject 提取 Element
    Element* element = const_cast<AnimationApplicator*>(this)->ExtractElement(object);
    if (!element) {
        return {};
    }
    
    auto it = started_animations_.find(element);  // 使用 Element* 作为键
    if (it != started_animations_.end()) {
        return it->second;
    }
    return {};
}

void AnimationApplicator::Clear() {
    // 清理所有已启动动画的跟踪信息
    // 注意：不再清除 AnimationController 中的动画
    // 因为动画现在通过 Element 引用关联，渲染树重建不会影响动画
    started_animations_.clear();
}

Element* AnimationApplicator::ExtractElement(RenderObject* object) const {
    if (!object) {
        return nullptr;
    }
    
    // 获取关联的 DOM 节点
    auto node = object->GetNode();
    if (!node) {
        return nullptr;
    }
    
    // 检查节点是否为 Element
    if (node->GetNodeType() != NodeType::ELEMENT_NODE) {
        return nullptr;
    }
    
    // 转换为 Element
    auto element = std::dynamic_pointer_cast<Element>(node);
    return element.get();
}

// ============================================================================
// 属性树系统优化
// ============================================================================

bool AnimationApplicator::TryApplyViaPropertyTree(RenderObject* object,
                                                   const std::string& property,
                                                   const std::string& value) {
    // 检查是否有属性树系统
    if (!paint_artifact_compositor_ || !property_trees_) {
        static bool first_warning = true;
        if (first_warning) {
            std::cout << "[AnimationApplicator] No property tree system available" << std::endl;
            first_warning = false;
        }
        return false;
    }
    
    // 检查对象是否有属性树状态
    PropertyTreeState* state = object->GetPropertyTreeState();
    if (!state) {
        // 调试：输出为什么没有属性树状态
        static bool first_warning = true;
        if (first_warning) {
            std::cout << "[AnimationApplicator] Warning: RenderObject has no PropertyTreeState, "
                      << "falling back to traditional rendering path" << std::endl;
            first_warning = false;
        }
        return false;
    }
    
    // 检查对象是否可以直接更新
    if (property == "transform") {
        if (!object->CanDirectlyUpdateTransform()) {
            static bool first_warning = true;
            if (first_warning) {
                const auto& style = object->GetComputedStyle();
                std::cout << "[AnimationApplicator] Cannot directly update transform. "
                          << "HasOwnCompositorLayer=" << object->HasOwnCompositorLayer()
                          << ", will_change='" << style.will_change << "'" << std::endl;
                first_warning = false;
            }
            return false;
        }
        
        TransformTreeNode* transform_node = state->Transform();
        if (!transform_node) {
            static bool first_warning = true;
            if (first_warning) {
                std::cout << "[AnimationApplicator] No transform node in PropertyTreeState" << std::endl;
                first_warning = false;
            }
            return false;
        }
        
        // 检查合成器是否支持直接更新此节点
        if (!paint_artifact_compositor_->CanDirectlyUpdateTransform(transform_node)) {
            static bool first_warning = true;
            if (first_warning) {
                std::cout << "[AnimationApplicator] Compositor cannot directly update transform node" << std::endl;
                first_warning = false;
            }
            return false;
        }
        
        // 解析 transform 值并转换为 SkM44
        std::optional<CSSTransform> css_transform_opt = CSSTransform::Parse(value);
        if (!css_transform_opt.has_value() || css_transform_opt->IsEmpty()) {
            // 空变换 = 单位矩阵
            return paint_artifact_compositor_->DirectlyUpdateTransform(
                transform_node, SkM44());
        }
        
        const CSSTransform& css_transform = css_transform_opt.value();
        
        // 获取对象的布局信息用于计算变换原点
        const auto& layout = object->GetLayoutInfo();
        SkRect rect = SkRect::MakeXYWH(layout.x, layout.y, layout.width, layout.height);
        
        // 获取变换原点
        const auto& style = object->GetComputedStyle();
        const TransformOrigin& origin = style.transform_origin;
        
        // 计算 2D 变换矩阵
        SkMatrix matrix2d = css_transform.ToSkMatrix(rect, origin);
        
        // 转换为 4x4 矩阵
        SkM44 matrix = SkM44(matrix2d);
        
        return paint_artifact_compositor_->DirectlyUpdateTransform(transform_node, matrix);
    }
    else if (property == "opacity") {
        if (!object->CanDirectlyUpdateOpacity()) {
            return false;
        }
        
        EffectTreeNode* effect_node = state->Effect();
        if (!effect_node) {
            return false;
        }
        
        // 检查合成器是否支持直接更新此节点
        if (!paint_artifact_compositor_->CanDirectlyUpdateOpacity(effect_node)) {
            return false;
        }
        
        // 解析 opacity 值
        float opacity = 1.0f;
        try {
            opacity = std::stof(value);
            opacity = std::max(0.0f, std::min(1.0f, opacity));  // 限制在 [0, 1]
        } catch (...) {
            return false;
        }
        
        return paint_artifact_compositor_->DirectlyUpdateOpacity(effect_node, opacity);
    }
    
    return false;
}

// ============================================================================
// 层合成优化
// ============================================================================

bool AnimationApplicator::TryApplyViaCompositor(RenderObject* object,
                                                 const std::string& property,
                                                 const std::string& value) {
    // 检查是否有动画层桥接器
    if (!animation_bridge_) {
        return false;
    }
    
    // 检查对象是否有独立的合成层
    if (!object->HasOwnCompositorLayer()) {
        return false;
    }
    
    // 通过动画层桥接器更新属性
    auto update_type = animation_bridge_->ApplyAnimationProperty(object, property, value);
    
    // 检查是否成功通过层系统更新
    return (update_type == AnimationUpdateType::Transform ||
            update_type == AnimationUpdateType::Opacity);
}

// ============================================================================
// 属性应用
// ============================================================================

bool AnimationApplicator::ApplyPropertyToStyle(ComputedStyle& style,
                                               const std::string& property,
                                               const std::string& value) {
    // opacity
    if (property == "opacity") {
        try {
            style.opacity = std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // transform
    if (property == "transform") {
        style.transform_str = value;
        // 解析 transform 到 CSSTransform
        style.transform = CSSTransform::Parse(value);
        return true;
    }
    
    // color
    if (property == "color") {
        style.color = value;
        return true;
    }
    
    // background-color
    if (property == "background-color") {
        style.background_color = value;
        return true;
    }
    
    // width
    if (property == "width") {
        auto [val, unit] = ParseNumberWithUnit(value);
        if (unit == "px") {
            style.width = CSSLength(val, CSSUnit::PX);
        } else if (unit == "%") {
            style.width = CSSLength(val, CSSUnit::PERCENT);
        } else if (unit == "em") {
            style.width = CSSLength(val, CSSUnit::EM);
        } else if (unit == "rem") {
            style.width = CSSLength(val, CSSUnit::REM);
        } else {
            style.width = CSSLength(val, CSSUnit::PX);
        }
        return true;
    }
    
    // height
    if (property == "height") {
        auto [val, unit] = ParseNumberWithUnit(value);
        if (unit == "px") {
            style.height = CSSLength(val, CSSUnit::PX);
        } else if (unit == "%") {
            style.height = CSSLength(val, CSSUnit::PERCENT);
        } else if (unit == "em") {
            style.height = CSSLength(val, CSSUnit::EM);
        } else if (unit == "rem") {
            style.height = CSSLength(val, CSSUnit::REM);
        } else {
            style.height = CSSLength(val, CSSUnit::PX);
        }
        return true;
    }
    
    // margin
    if (property == "margin-top") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.margin_top = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "margin-right") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.margin_right = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "margin-bottom") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.margin_bottom = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "margin-left") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.margin_left = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    
    // padding
    if (property == "padding-top") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.padding_top = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "padding-right") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.padding_right = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "padding-bottom") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.padding_bottom = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "padding-left") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.padding_left = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    
    // border-width
    if (property == "border-top-width") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_top_width = val;
        return true;
    }
    if (property == "border-right-width") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_right_width = val;
        return true;
    }
    if (property == "border-bottom-width") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_bottom_width = val;
        return true;
    }
    if (property == "border-left-width") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_left_width = val;
        return true;
    }
    
    // border-color
    if (property == "border-top-color") {
        style.border_top_color = ParseColor(value);
        return true;
    }
    if (property == "border-right-color") {
        style.border_right_color = ParseColor(value);
        return true;
    }
    if (property == "border-bottom-color") {
        style.border_bottom_color = ParseColor(value);
        return true;
    }
    if (property == "border-left-color") {
        style.border_left_color = ParseColor(value);
        return true;
    }
    
    // border-radius
    if (property == "border-top-left-radius") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_radius.top_left = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "border-top-right-radius") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_radius.top_right = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "border-bottom-left-radius") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_radius.bottom_left = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "border-bottom-right-radius") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.border_radius.bottom_right = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    
    // font-size
    if (property == "font-size") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.font_size = val;
        return true;
    }
    
    // line-height
    if (property == "line-height") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.line_height = val;
        return true;
    }
    
    // letter-spacing
    if (property == "letter-spacing") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.letter_spacing = CSSLength(val, CSSUnit::PX);
        return true;
    }
    
    // top, right, bottom, left (positioning)
    if (property == "top") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.top = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "right") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.right = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "bottom") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.bottom = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    if (property == "left") {
        auto [val, unit] = ParseNumberWithUnit(value);
        style.left = CSSLength(val, unit == "%" ? CSSUnit::PERCENT : CSSUnit::PX);
        return true;
    }
    
    // z-index
    if (property == "z-index") {
        try {
            style.z_index = std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // flex properties
    if (property == "flex-grow") {
        try {
            style.flex_grow = std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    if (property == "flex-shrink") {
        try {
            style.flex_shrink = std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // 未知属性
    return false;
}

// ============================================================================
// 辅助函数
// ============================================================================

std::pair<float, std::string> AnimationApplicator::ParseNumberWithUnit(const std::string& str) const {
    std::regex number_regex(R"(^([-+]?[0-9]*\.?[0-9]+)([a-z%]*)$)");
    std::smatch match;
    
    if (std::regex_match(str, match, number_regex)) {
        float value = std::stof(match[1].str());
        std::string unit = match[2].str();
        return {value, unit};
    }
    
    return {0.0f, ""};
}

SkColor AnimationApplicator::ParseColor(const std::string& str) const {
    // 使用 Color 类解析
    return Color::Parse(str);
}

} // namespace lightui
