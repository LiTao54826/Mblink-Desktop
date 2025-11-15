#pragma once

#include "transition.h"
#include "transform.h"
#include "css_value.h"
#include "include/core/SkColor.h"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 动画状态
 */
enum class AnimationState {
    IDLE,       // 空闲
    DELAYED,    // 延迟中
    RUNNING,    // 运行中
    FINISHED    // 已完成
};

/**
 * @brief 可过渡的属性值类型
 */
using TransitionValue = std::variant<
    float,              // 数值属性 (width, height, opacity, etc.)
    SkColor,            // 颜色属性 (color, background-color, etc.)
    CSSTransform        // Transform 属性
>;

/**
 * @brief 运行中的过渡动画
 */
struct RunningTransition {
    RenderObject* object;           // 目标对象
    std::string property;           // 属性名
    CSSTransition transition;       // 过渡配置
    AnimationState state;           // 当前状态
    double start_time;              // 开始时间 (毫秒)
    double current_time;            // 当前时间 (毫秒)
    
    // 起始值和目标值
    TransitionValue start_value;
    TransitionValue end_value;
    
    RunningTransition()
        : object(nullptr)
        , state(AnimationState::IDLE)
        , start_time(0)
        , current_time(0)
        , start_value(0.0f)
        , end_value(0.0f) {}
    
    /**
     * @brief 计算当前进度 (0.0 到 1.0)
     */
    float GetProgress() const;
    
    /**
     * @brief 获取当前插值后的值
     */
    TransitionValue GetCurrentValue() const;
};

/**
 * @brief 动画时间轴管理器
 * 
 * 管理所有运行中的过渡动画，每帧更新动画状态
 */
class AnimationTimeline {
public:
    AnimationTimeline();
    ~AnimationTimeline();
    
    /**
     * @brief 启动一个过渡动画
     * @param object 目标对象
     * @param property 属性名
     * @param transition 过渡配置
     * @param start_value 起始值
     * @param end_value 目标值
     */
    void StartTransition(RenderObject* object,
                        const std::string& property,
                        const CSSTransition& transition,
                        const TransitionValue& start_value,
                        const TransitionValue& end_value);
    
    /**
     * @brief 更新时间轴 (每帧调用)
     * @param current_time 当前时间 (毫秒)
     */
    void Update(double current_time);
    
    /**
     * @brief 获取属性的当前值
     * @param object 目标对象
     * @param property 属性名
     * @return 当前值 (如果有运行中的过渡)
     */
    std::optional<TransitionValue> GetCurrentValue(RenderObject* object, 
                                                    const std::string& property) const;
    
    /**
     * @brief 停止指定对象的指定属性的过渡
     * @param object 目标对象
     * @param property 属性名
     */
    void StopTransition(RenderObject* object, const std::string& property);
    
    /**
     * @brief 停止指定对象的所有过渡
     * @param object 目标对象
     */
    void StopAllTransitions(RenderObject* object);
    
    /**
     * @brief 停止所有过渡
     */
    void StopAll();
    
    /**
     * @brief 是否有运行中的过渡
     */
    bool HasRunningTransitions() const;
    
    /**
     * @brief 获取运行中的过渡数量
     */
    size_t GetRunningTransitionCount() const;

    /**
     * @brief 插值两个值
     * @param start 起始值
     * @param end 目标值
     * @param progress 进度 (0.0 到 1.0)
     * @return 插值结果
     */
    static TransitionValue Interpolate(const TransitionValue& start,
                                       const TransitionValue& end,
                                       float progress);

private:
    std::vector<RunningTransition> running_transitions_;

    /**
     * @brief 移除已完成的过渡
     */
    void RemoveFinishedTransitions();
    
    /**
     * @brief 插值两个浮点数
     */
    static float InterpolateFloat(float start, float end, float progress);
    
    /**
     * @brief 插值两个颜色
     */
    static SkColor InterpolateColor(SkColor start, SkColor end, float progress);
    
    /**
     * @brief 插值两个 Transform
     */
    static CSSTransform InterpolateTransform(const CSSTransform& start,
                                            const CSSTransform& end,
                                            float progress);
};

} // namespace lightui

