/**
 * @file animation_controller.cpp
 * @brief CSS 动画控制器实现
 * @author MBink Development Team
 * @date 2025-11-14
 */

#include "animation_controller.h"
#include "easing_functions.h"
#include "property_interpolation.h"
#include "core/render/objects/render_object.h"
#include "core/dom/event.h"
#include "core/dom/element.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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

    // 标记为脏
    if (optimization_enabled_) {
        optimizer_.GetDirtyTracker().MarkDirty(object, animation.name);
    }
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
    // 如果启用了批量更新优化
    if (optimization_enabled_ && optimizer_.GetBatchUpdater().IsEnabled()) {
        // 收集所有需要更新的动画
        for (auto& anim : running_animations_) {
            if (anim.state != CSSAnimationState::PAUSED) {
                optimizer_.GetBatchUpdater().AddUpdateRequest(
                    anim.object, anim.config.name, current_time);
            }
        }

        // 批量执行更新
        auto requests = optimizer_.GetBatchUpdater().GetPendingRequests();
        optimizer_.GetBatchUpdater().Clear();

        // 处理每个请求
        for (const auto& req : requests) {
            UpdateSingleAnimation(req.object, req.animation_name, req.current_time);
        }
    } else {
        // 正常更新流程
        for (auto it = running_animations_.begin(); it != running_animations_.end(); ) {
            RunningAnimation& anim = *it;

            // 检查脏标记优化
            if (optimization_enabled_ &&
                !optimizer_.GetDirtyTracker().IsDirty(anim.object, anim.config.name)) {
                ++it;
                continue;
            }

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

            // 清除脏标记
            if (optimization_enabled_) {
                optimizer_.GetDirtyTracker().ClearDirty(anim.object, anim.config.name);
            }

            ++it;
        }
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
    
    // 处理 backwards 填充模式：在延迟期间应用第一帧样式
    if (anim.state == CSSAnimationState::DELAYED) {
        if (anim.config.fill_mode == AnimationFillMode::BACKWARDS ||
            anim.config.fill_mode == AnimationFillMode::BOTH) {
            // 返回第一帧的属性
            return ComputeCurrentFrame(anim, 0.0f);
        }
        // 延迟期间且不是 backwards/both，不返回任何属性
        return std::nullopt;
    }
    
    // 处理 forwards 填充模式：动画结束后保留最后一帧样式
    if (anim.state == CSSAnimationState::FINISHED) {
        if (anim.config.fill_mode == AnimationFillMode::FORWARDS ||
            anim.config.fill_mode == AnimationFillMode::BOTH) {
            // 返回最后一帧的属性
            return ComputeCurrentFrame(anim, 1.0f);
        }
        // 动画结束且不是 forwards/both，不返回任何属性
        return std::nullopt;
    }
    
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

    // 尝试从缓存获取
    if (optimization_enabled_) {
        auto cached = optimizer_.GetInterpolationCache().Get(anim.config.name, progress);
        if (cached.has_value()) {
            return cached.value();
        }
    }

    // 获取当前进度对应的关键帧
    auto [prev, next, factor] = anim.keyframes->GetKeyframesAt(progress);

    if (!prev || !next) {
        return {};
    }

    // 应用缓动函数
    float eased_factor = ApplyEasing(factor, anim.config.timing_function, anim.config.bezier);

    // 使用属性插值
    auto result = PropertyInterpolation::InterpolateProperties(prev->properties, next->properties, eased_factor);

    // 缓存结果
    if (optimization_enabled_) {
        optimizer_.GetInterpolationCache().Put(anim.config.name, progress, result);
    }

    return result;
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

    // 清除优化器状态
    if (optimization_enabled_) {
        optimizer_.Reset();
    }
}

void AnimationController::ClearRunningAnimations() {
    // 只清除运行中的动画，保留 @keyframes 规则
    running_animations_.clear();

    // 清除优化器状态
    if (optimization_enabled_) {
        optimizer_.Reset();
    }
}

const KeyframesRule* AnimationController::GetKeyframes(const std::string& name) const {
    auto it = keyframes_rules_.find(name);
    if (it != keyframes_rules_.end()) {
        return &it->second;
    }
    return nullptr;
}

void AnimationController::UpdateSingleAnimation(RenderObject* object,
                                                const std::string& name,
                                                double current_time) {
    auto it = FindAnimation(object, name);
    if (it == running_animations_.end()) {
        return;
    }

    RunningAnimation& anim = *it;

    // 初始化开始时间
    if (!anim.initialized) {
        anim.start_time = current_time;
        anim.initialized = true;
    }

    // 跳过暂停的动画
    if (anim.state == CSSAnimationState::PAUSED) {
        return;
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
            return;
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
        return;
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
            }
            return;
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

    // 清除脏标记
    if (optimization_enabled_) {
        optimizer_.GetDirtyTracker().ClearDirty(anim.object, anim.config.name);
    }
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

    // 获取关联的 DOM 节点
    auto node = anim.object->GetNode();
    if (!node) {
        return;
    }

    // 检查节点是否为 Element
    if (node->GetNodeType() != NodeType::ELEMENT_NODE) {
        return;
    }

    // 转换为 Element
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (!element) {
        return;
    }

    // 创建并分发 AnimationEvent
    auto event = std::make_shared<AnimationEvent>(
        event_type,
        anim.config.name,
        elapsed_time
    );
    element->DispatchEvent(event);
}

} // namespace lightui

