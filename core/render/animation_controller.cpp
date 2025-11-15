/**
 * @file animation_controller.cpp
 * @brief CSS 动画控制器实现
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "animation_controller.h"
#include "easing_functions.h"
#include "property_interpolation.h"
#include "render_object.h"
#include "core/dom/event.h"
#include "core/dom/element.h"
#include <algorithm>
#include <cmath>

namespace lightui {

// ============================================================================
// 构造/析构
// ============================================================================

AnimationController::AnimationController() {
}

AnimationController::~AnimationController() {
}

// ============================================================================
// @keyframes 管理
// ============================================================================

void AnimationController::RegisterKeyframes(const KeyframesRule& rule) {
    if (rule.IsValid()) {
        keyframes_rules_[rule.name] = rule;
    }
}

// ============================================================================
// 动画控制
// ============================================================================

void AnimationController::StartAnimation(RenderObject* object, const CSSAnimation& animation) {
    if (!object || !animation.IsValid()) {
        return;
    }
    
    // 查找 @keyframes 规则
    auto it = keyframes_rules_.find(animation.name);
    if (it == keyframes_rules_.end()) {
        return;  // 找不到对应的 @keyframes
    }
    
    // 停止已存在的同名动画
    StopAnimation(object, animation.name);
    
    // 创建新的运行中动画
    RunningAnimation anim;
    anim.object = object;
    anim.config = animation;
    anim.keyframes = &it->second;
    anim.state = animation.delay > 0 ? CSSAnimationState::DELAYED : CSSAnimationState::RUNNING;
    anim.start_time = 0;  // 将在第一次 Update 时设置
    anim.current_time = 0;
    anim.current_iteration = 0;

    if (animation.paused) {
        anim.state = CSSAnimationState::PAUSED;
    }
    
    running_animations_.push_back(anim);
}

void AnimationController::StopAnimation(RenderObject* object, const std::string& name) {
    auto it = FindAnimation(object, name);
    if (it != running_animations_.end()) {
        running_animations_.erase(it);
    }
}

void AnimationController::StopAllAnimations(RenderObject* object) {
    running_animations_.erase(
        std::remove_if(running_animations_.begin(), running_animations_.end(),
            [object](const RunningAnimation& anim) {
                return anim.object == object;
            }),
        running_animations_.end()
    );
}

void AnimationController::PauseAnimation(RenderObject* object, const std::string& name) {
    auto it = FindAnimation(object, name);
    if (it != running_animations_.end()) {
        it->state = CSSAnimationState::PAUSED;
    }
}

void AnimationController::ResumeAnimation(RenderObject* object, const std::string& name) {
    auto it = FindAnimation(object, name);
    if (it != running_animations_.end() && it->state == CSSAnimationState::PAUSED) {
        it->state = CSSAnimationState::RUNNING;
    }
}

// ============================================================================
// 动画更新
// ============================================================================

void AnimationController::Update(double current_time) {
    for (auto it = running_animations_.begin(); it != running_animations_.end(); ) {
        RunningAnimation& anim = *it;

        // 初始化开始时间
        if (!anim.initialized) {
            anim.start_time = current_time;
            anim.initialized = true;
        }

        // 跳过暂停的动画
        if (anim.state == CSSAnimationState::PAUSED) {
            ++it;
            continue;
        }

        // 计算从开始到现在的总时间
        double total_elapsed = current_time - anim.start_time;

        // 处理延迟
        if (anim.state == CSSAnimationState::DELAYED) {
            if (total_elapsed >= anim.config.delay) {
                anim.state = CSSAnimationState::RUNNING;

                // 触发 animationstart 事件
                if (!anim.start_event_fired) {
                    FireAnimationEvent(anim, "animationstart", 0.0f);
                    anim.start_event_fired = true;
                }
            } else {
                ++it;
                continue;
            }
        }

        // 计算动画实际运行时间（减去延迟）
        if (anim.config.delay > 0) {
            anim.current_time = total_elapsed - anim.config.delay;
        } else {
            anim.current_time = total_elapsed;
        }

        // 计算动画进度
        double elapsed = anim.current_time;
        double duration = anim.config.duration;

        if (duration <= 0) {
            ++it;
            continue;
        }

        // 检查是否完成
        if (anim.config.iteration_count > 0) {
            // 有限次迭代
            double total_duration = duration * anim.config.iteration_count;
            if (elapsed >= total_duration) {
                anim.state = CSSAnimationState::FINISHED;

                // 触发 animationend 事件
                if (!anim.end_event_fired) {
                    FireAnimationEvent(anim, "animationend", static_cast<float>(elapsed));
                    anim.end_event_fired = true;
                }

                // 根据 fill-mode 决定是否保留
                if (anim.config.fill_mode == AnimationFillMode::FORWARDS ||
                    anim.config.fill_mode == AnimationFillMode::BOTH) {
                    // 保留最后一帧
                    anim.current_iteration = anim.config.iteration_count - 1;
                    ++it;
                } else {
                    // 移除动画
                    it = running_animations_.erase(it);
                }
                continue;
            }
        }

        // 计算当前迭代
        int new_iteration = static_cast<int>(elapsed / duration);

        // 检测迭代变化，触发 animationiteration 事件
        if (new_iteration > anim.last_iteration && anim.last_iteration >= 0) {
            // 迭代次数增加，触发事件
            FireAnimationEvent(anim, "animationiteration", static_cast<float>(elapsed));
        }

        anim.current_iteration = new_iteration;
        anim.last_iteration = new_iteration;

        ++it;
    }
}

// ============================================================================
// 属性计算
// ============================================================================

std::optional<std::map<std::string, std::string>> 
AnimationController::GetCurrentProperties(RenderObject* object, const std::string& name) const {
    auto it = FindAnimation(object, name);
    if (it == running_animations_.end()) {
        return std::nullopt;
    }
    
    const RunningAnimation& anim = *it;
    
    // 计算进度
    float progress = ComputeProgress(anim, anim.current_time + anim.start_time);
    
    // 计算当前帧属性
    return ComputeCurrentFrame(anim, progress);
}

std::map<std::string, std::string> AnimationController::ComputeCurrentFrame(
    const RunningAnimation& anim, float progress) const {

    if (!anim.keyframes || anim.keyframes->keyframes.empty()) {
        return {};
    }

    // 获取当前进度对应的关键帧
    auto [prev, next, factor] = anim.keyframes->GetKeyframesAt(progress);

    if (!prev || !next) {
        return {};
    }

    // 应用缓动函数
    float eased_factor = ApplyEasing(factor, anim.config.timing_function, anim.config.bezier);

    // 使用属性插值
    return PropertyInterpolation::InterpolateProperties(prev->properties, next->properties, eased_factor);
}

float AnimationController::ComputeProgress(const RunningAnimation& anim, double current_time) const {
    double elapsed = current_time - anim.start_time;
    double duration = anim.config.duration;
    
    if (duration <= 0) {
        return 1.0f;
    }
    
    // 计算当前迭代内的进度
    double iteration_progress = std::fmod(elapsed, duration) / duration;
    
    // 处理方向
    int iteration = static_cast<int>(elapsed / duration);
    bool reverse = false;
    
    switch (anim.config.direction) {
        case AnimationDirection::ANIM_NORMAL:
            reverse = false;
            break;
        case AnimationDirection::ANIM_REVERSE:
            reverse = true;
            break;
        case AnimationDirection::ANIM_ALTERNATE:
            reverse = (iteration % 2 == 1);
            break;
        case AnimationDirection::ANIM_ALTERNATE_REVERSE:
            reverse = (iteration % 2 == 0);
            break;
    }
    
    if (reverse) {
        iteration_progress = 1.0 - iteration_progress;
    }
    
    return static_cast<float>(iteration_progress);
}

float AnimationController::ApplyEasing(float progress, TimingFunction timing_function,
                                      const CubicBezier& bezier) const {
    switch (timing_function) {
        case TimingFunction::LINEAR:
            return EasingFunctions::Linear(progress);
        case TimingFunction::EASE:
            return EasingFunctions::Ease(progress);
        case TimingFunction::EASE_IN:
            return EasingFunctions::EaseIn(progress);
        case TimingFunction::EASE_OUT:
            return EasingFunctions::EaseOut(progress);
        case TimingFunction::EASE_IN_OUT:
            return EasingFunctions::EaseInOut(progress);
        case TimingFunction::CUBIC_BEZIER:
            return EasingFunctions::CubicBezierEasing(progress, bezier.x1, bezier.y1, bezier.x2, bezier.y2);
        default:
            return progress;
    }
}

// ============================================================================
// 辅助函数
// ============================================================================

std::vector<RunningAnimation>::iterator 
AnimationController::FindAnimation(RenderObject* object, const std::string& name) {
    return std::find_if(running_animations_.begin(), running_animations_.end(),
        [object, &name](const RunningAnimation& anim) {
            return anim.object == object && anim.config.name == name;
        });
}

std::vector<RunningAnimation>::const_iterator 
AnimationController::FindAnimation(RenderObject* object, const std::string& name) const {
    return std::find_if(running_animations_.begin(), running_animations_.end(),
        [object, &name](const RunningAnimation& anim) {
            return anim.object == object && anim.config.name == name;
        });
}

void AnimationController::Clear() {
    running_animations_.clear();
    keyframes_rules_.clear();
}

// ============================================================================
// 事件触发
// ============================================================================

void AnimationController::FireAnimationEvent(const RunningAnimation& anim,
                                             const std::string& event_type,
                                             float elapsed_time) {
    // 获取 RenderObject 关联的 DOM 元素
    if (!anim.object) {
        return;
    }

    // TODO: 需要在 RenderObject 中添加获取关联 Element 的方法
    // 目前暂时跳过事件触发
    //
    // 预期的实现：
    // auto element = anim.object->GetElement();
    // if (element) {
    //     auto event = std::make_shared<AnimationEvent>(
    //         event_type,
    //         anim.config.name,
    //         elapsed_time
    //     );
    //     element->DispatchEvent(event);
    // }
}

} // namespace lightui

