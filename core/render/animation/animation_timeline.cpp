#include "animation_timeline.h"
#include "easing_functions.h"
#include "core/render/objects/render_object.h"
#include "core/dom/element.h"
#include <algorithm>
#include <cmath>

namespace mblink {

// ============================================================================
// RunningTransition Implementation
// ============================================================================

RenderObject* RunningTransition::GetRenderObject() const {
    if (auto elem = element.lock()) {
        auto render_obj = elem->GetRenderObject();
        return render_obj.get();
    }
    return nullptr;
}

float RunningTransition::GetProgress() const {
    if (state != AnimationState::RUNNING) {
        return 0.0f;
    }
    
    double elapsed = current_time - start_time;
    double duration_ms = transition.duration * 1000.0;
    
    if (duration_ms <= 0.0) {
        return 1.0f;
    }
    
    float progress = static_cast<float>(elapsed / duration_ms);
    return std::max(0.0f, std::min(1.0f, progress));
}

TransitionValue RunningTransition::GetCurrentValue() const {
    float progress = GetProgress();
    
    // 应用缓动函数
    float eased_progress = EasingFunctions::Apply(progress, 
                                                   transition.timing_function, 
                                                   transition.bezier);
    
    // 插值
    return AnimationTimeline::Interpolate(start_value, end_value, eased_progress);
}

// ============================================================================
// AnimationTimeline Implementation
// ============================================================================

AnimationTimeline::AnimationTimeline() {
}

AnimationTimeline::~AnimationTimeline() {
}


// ============================================================================
// 过渡控制 - Element 版本
// ============================================================================

void AnimationTimeline::StartTransition(std::shared_ptr<Element> element,
                                       const std::string& property,
                                       const CSSTransition& transition,
                                       const TransitionValue& start_value,
                                       const TransitionValue& end_value) {
    if (!element) {
        return;
    }
    
    // 停止已存在的相同属性的过渡
    StopTransition(element, property);
    
    // 创建新的过渡
    RunningTransition trans;
    trans.element = element;  // 使用 Element 引用
    trans.property = property;
    trans.transition = transition;
    trans.start_value = start_value;
    trans.end_value = end_value;
    trans.start_time = -1.0;  // 标记为未初始化
    trans.current_time = 0;

    // 如果有延迟，设置为 DELAYED 状态
    if (transition.delay > 0) {
        trans.state = AnimationState::DELAYED;
    } else {
        trans.state = AnimationState::RUNNING;
    }

    running_transitions_.push_back(trans);
}

// ============================================================================
// 过渡控制 - RenderObject 兼容版本
// ============================================================================

void AnimationTimeline::StartTransition(RenderObject* object,
                                       const std::string& property,
                                       const CSSTransition& transition,
                                       const TransitionValue& start_value,
                                       const TransitionValue& end_value) {
    if (!object) {
        return;
    }
    
    // 从 RenderObject 提取 Element
    auto element = ExtractElement(object);
    if (!element) {
        return;
    }
    
    // 调用 Element 版本
    StartTransition(element, property, transition, start_value, end_value);
}

void AnimationTimeline::Update(double current_time) {
    // 首先清理无效过渡（Element 已销毁）
    CleanupInvalidTransitions();
    
    for (auto& trans : running_transitions_) {
        // 检查过渡是否有效
        if (!trans.IsValid()) {
            continue;
        }
        
        trans.current_time = current_time;

        // 初始化开始时间
        if (trans.start_time < 0) {
            trans.start_time = current_time;
        }

        // 处理延迟状态
        if (trans.state == AnimationState::DELAYED) {
            double elapsed = current_time - trans.start_time;
            double delay_ms = trans.transition.delay * 1000.0;

            if (elapsed >= delay_ms) {
                // 延迟结束，开始运行
                trans.state = AnimationState::RUNNING;
                trans.start_time = current_time;  // 重置开始时间
            }
            continue;
        }

        // 处理运行状态
        if (trans.state == AnimationState::RUNNING) {
            float progress = trans.GetProgress();

            if (progress >= 1.0f) {
                // 过渡完成
                trans.state = AnimationState::FINISHED;

                // TODO: 触发 transitionend 事件
            }
        }
    }

    // 移除已完成的过渡
    RemoveFinishedTransitions();
}

std::optional<TransitionValue> AnimationTimeline::GetCurrentValue(std::shared_ptr<Element> element,
                                                                   const std::string& property) const {
    if (!element) {
        return std::nullopt;
    }
    
    Element* elem_ptr = element.get();
    for (const auto& trans : running_transitions_) {
        auto trans_elem = trans.GetElement();
        if (trans_elem && trans_elem.get() == elem_ptr && trans.property == property) {
            if (trans.state == AnimationState::RUNNING) {
                return trans.GetCurrentValue();
            }
        }
    }
    
    return std::nullopt;
}

std::optional<TransitionValue> AnimationTimeline::GetCurrentValue(RenderObject* object,
                                                                   const std::string& property) const {
    auto element = ExtractElement(object);
    if (element) {
        return GetCurrentValue(element, property);
    }
    return std::nullopt;
}

void AnimationTimeline::StopTransition(std::shared_ptr<Element> element, const std::string& property) {
    if (!element) {
        return;
    }
    
    Element* elem_ptr = element.get();
    auto it = std::remove_if(running_transitions_.begin(), running_transitions_.end(),
        [elem_ptr, &property](const RunningTransition& trans) {
            auto trans_elem = trans.GetElement();
            return trans_elem && trans_elem.get() == elem_ptr && trans.property == property;
        });
    
    running_transitions_.erase(it, running_transitions_.end());
}

void AnimationTimeline::StopTransition(RenderObject* object, const std::string& property) {
    auto element = ExtractElement(object);
    if (element) {
        StopTransition(element, property);
    }
}

void AnimationTimeline::StopAllTransitions(std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }
    
    Element* elem_ptr = element.get();
    auto it = std::remove_if(running_transitions_.begin(), running_transitions_.end(),
        [elem_ptr](const RunningTransition& trans) {
            auto trans_elem = trans.GetElement();
            return trans_elem && trans_elem.get() == elem_ptr;
        });
    
    running_transitions_.erase(it, running_transitions_.end());
}

void AnimationTimeline::StopAllTransitions(RenderObject* object) {
    auto element = ExtractElement(object);
    if (element) {
        StopAllTransitions(element);
    }
}

void AnimationTimeline::StopAll() {
    running_transitions_.clear();
}

bool AnimationTimeline::HasRunningTransitions() const {
    return !running_transitions_.empty();
}

size_t AnimationTimeline::GetRunningTransitionCount() const {
    return running_transitions_.size();
}

void AnimationTimeline::RemoveFinishedTransitions() {
    auto it = std::remove_if(running_transitions_.begin(), running_transitions_.end(),
        [](const RunningTransition& trans) {
            return trans.state == AnimationState::FINISHED || !trans.IsValid();
        });
    
    running_transitions_.erase(it, running_transitions_.end());
}

void AnimationTimeline::CleanupInvalidTransitions() {
    running_transitions_.erase(
        std::remove_if(running_transitions_.begin(), running_transitions_.end(),
            [](const RunningTransition& trans) {
                return !trans.IsValid();
            }),
        running_transitions_.end()
    );
}

std::shared_ptr<Element> AnimationTimeline::ExtractElement(RenderObject* object) const {
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
    return std::dynamic_pointer_cast<Element>(node);
}


// ============================================================================
// 插值函数
// ============================================================================

TransitionValue AnimationTimeline::Interpolate(const TransitionValue& start,
                                               const TransitionValue& end,
                                               float progress) {
    // 检查类型是否匹配
    if (start.index() != end.index()) {
        // 类型不匹配，返回结束值
        return end;
    }
    
    // 根据类型进行插值
    if (std::holds_alternative<float>(start)) {
        float start_val = std::get<float>(start);
        float end_val = std::get<float>(end);
        return InterpolateFloat(start_val, end_val, progress);
    }
    else if (std::holds_alternative<SkColor>(start)) {
        SkColor start_val = std::get<SkColor>(start);
        SkColor end_val = std::get<SkColor>(end);
        return InterpolateColor(start_val, end_val, progress);
    }
    else if (std::holds_alternative<CSSTransform>(start)) {
        const CSSTransform& start_val = std::get<CSSTransform>(start);
        const CSSTransform& end_val = std::get<CSSTransform>(end);
        return InterpolateTransform(start_val, end_val, progress);
    }
    
    return end;
}

float AnimationTimeline::InterpolateFloat(float start, float end, float progress) {
    return start + (end - start) * progress;
}

SkColor AnimationTimeline::InterpolateColor(SkColor start, SkColor end, float progress) {
    // 在 RGBA 空间中插值
    uint8_t r = static_cast<uint8_t>(
        SkColorGetR(start) + (SkColorGetR(end) - SkColorGetR(start)) * progress
    );
    uint8_t g = static_cast<uint8_t>(
        SkColorGetG(start) + (SkColorGetG(end) - SkColorGetG(start)) * progress
    );
    uint8_t b = static_cast<uint8_t>(
        SkColorGetB(start) + (SkColorGetB(end) - SkColorGetB(start)) * progress
    );
    uint8_t a = static_cast<uint8_t>(
        SkColorGetA(start) + (SkColorGetA(end) - SkColorGetA(start)) * progress
    );
    
    return SkColorSetARGB(a, r, g, b);
}

CSSTransform AnimationTimeline::InterpolateTransform(const CSSTransform& start,
                                                     const CSSTransform& end,
                                                     float progress) {
    // 简化实现：如果 transform 数量不同，直接返回结束值
    if (start.transforms.size() != end.transforms.size()) {
        return end;
    }
    
    CSSTransform result;
    
    for (size_t i = 0; i < start.transforms.size(); ++i) {
        const auto& start_trans = start.transforms[i];
        const auto& end_trans = end.transforms[i];
        
        // 类型必须相同
        if (start_trans.type != end_trans.type) {
            result.transforms.push_back(end_trans);
            continue;
        }
        
        // 插值各个值
        Transform interpolated(start_trans.type);
        size_t value_count = std::min(start_trans.values.size(), end_trans.values.size());
        
        for (size_t j = 0; j < value_count; ++j) {
            float value = InterpolateFloat(start_trans.values[j], end_trans.values[j], progress);
            interpolated.values.push_back(value);
        }
        
        result.transforms.push_back(interpolated);
    }
    
    return result;
}

} // namespace mblink
