/**
 * @file animation_controller.h
 * @brief CSS 动画控制器
 * @author MBink Development Team
 * @date 2025-11-14
 */

#ifndef LIGHTUI_CORE_RENDER_ANIMATION_CONTROLLER_H_
#define LIGHTUI_CORE_RENDER_ANIMATION_CONTROLLER_H_

#include "animation.h"
#include "keyframes.h"
#include "animation_optimizer.h"
#include <map>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace lightui {

// 前向声明
class RenderObject;
class Element;

/**
 * @brief CSS 动画状态
 */
enum class CSSAnimationState {
    IDLE,      ///< 未开始
    DELAYED,   ///< 延迟中
    RUNNING,   ///< 运行中
    PAUSED,    ///< 暂停
    FINISHED   ///< 已完成
};

/**
 * @brief 运行中的动画
 * 
 * 表示一个正在运行的动画实例，包含动画配置、状态和时间信息。
 * 动画通过 Element 引用关联，而不是 RenderObject 指针，
 * 这样渲染树重建时动画不会丢失。
 */
struct RunningAnimation {
    std::weak_ptr<Element> element; ///< 关联的 DOM 元素（弱引用避免循环引用）
    CSSAnimation config;            ///< 动画配置
    const KeyframesRule* keyframes; ///< 关键帧规则
    CSSAnimationState state;        ///< 当前状态
    double start_time;              ///< 开始时间 (秒)
    double current_time;            ///< 当前时间 (秒)
    int current_iteration;          ///< 当前迭代次数 (从0开始)
    int last_iteration;             ///< 上一次的迭代次数 (用于检测迭代变化)
    bool initialized;               ///< 是否已初始化开始时间
    bool start_event_fired;         ///< 是否已触发 animationstart 事件
    bool end_event_fired;           ///< 是否已触发 animationend 事件

    /**
     * @brief 默认构造函数
     */
    RunningAnimation()
        : keyframes(nullptr)
        , state(CSSAnimationState::IDLE)
        , start_time(0)
        , current_time(0)
        , current_iteration(0)
        , last_iteration(-1)
        , initialized(false)
        , start_event_fired(false)
        , end_event_fired(false) {}
    
    /**
     * @brief 获取当前关联的 RenderObject
     * @return RenderObject 指针，如果 Element 已销毁或无 RenderObject 则返回 nullptr
     */
    RenderObject* GetRenderObject() const;
    
    /**
     * @brief 获取关联的 Element
     * @return Element 共享指针，如果已销毁返回 nullptr
     */
    std::shared_ptr<Element> GetElement() const {
        return element.lock();
    }
    
    /**
     * @brief 检查动画是否仍然有效（Element 未销毁）
     * @return true 如果 Element 仍然存在
     */
    bool IsValid() const {
        return !element.expired();
    }
};

/**
 * @brief 动画控制器
 * 
 * 管理所有运行中的 CSS 动画，负责动画的启动、停止、暂停、恢复和更新。
 * 
 * 主要功能:
 * - 注册和管理 @keyframes 规则
 * - 启动和停止动画
 * - 暂停和恢复动画
 * - 每帧更新动画状态
 * - 计算当前帧的属性值
 * 
 * 使用示例:
 * @code
 * AnimationController controller;
 * 
 * // 注册 @keyframes
 * KeyframesRule rule = KeyframesRule::Parse("@keyframes slide { ... }");
 * controller.RegisterKeyframes(rule);
 * 
 * // 启动动画
 * CSSAnimation anim;
 * anim.name = "slide";
 * anim.duration = 1.0f;
 * controller.StartAnimation(render_object, anim);
 * 
 * // 每帧更新
 * controller.Update(current_time);
 * 
 * // 获取当前属性
 * auto props = controller.GetCurrentProperties(render_object, "slide");
 * @endcode
 */
class AnimationController {
public:
    /**
     * @brief 构造函数
     */
    AnimationController();
    
    /**
     * @brief 析构函数
     */
    ~AnimationController();
    
    /**
     * @brief 注册 @keyframes 规则
     * 
     * 将 @keyframes 规则注册到控制器中，以便动画可以引用它。
     * 
     * @param rule @keyframes 规则
     * 
     * @note 如果已存在同名规则，将被覆盖
     */
    void RegisterKeyframes(const KeyframesRule& rule);
    
    /**
     * @brief 启动动画 (Element 版本，推荐使用)
     * 
     * 为指定的 DOM 元素启动一个动画。
     * 
     * @param element DOM 元素
     * @param animation 动画配置
     * 
     * @note 如果元素已有同名动画，将被替换
     * @note 如果找不到对应的 @keyframes 规则，动画不会启动
     */
    void StartAnimation(std::shared_ptr<Element> element, const CSSAnimation& animation);
    
    /**
     * @brief 启动动画 (RenderObject 版本，兼容接口)
     * 
     * 为指定的渲染对象启动一个动画。内部会提取关联的 Element。
     * 
     * @param object 渲染对象
     * @param animation 动画配置
     * 
     * @note 如果对象已有同名动画，将被替换
     * @note 如果找不到对应的 @keyframes 规则，动画不会启动
     * @deprecated 推荐使用 Element 版本
     */
    void StartAnimation(RenderObject* object, const CSSAnimation& animation);
    
    /**
     * @brief 停止动画 (Element 版本)
     * 
     * 停止指定元素的指定动画。
     * 
     * @param element DOM 元素
     * @param name 动画名称
     */
    void StopAnimation(std::shared_ptr<Element> element, const std::string& name);
    
    /**
     * @brief 停止动画 (RenderObject 版本，兼容接口)
     * 
     * 停止指定对象的指定动画。
     * 
     * @param object 渲染对象
     * @param name 动画名称
     */
    void StopAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 停止所有动画 (Element 版本)
     * 
     * 停止指定元素的所有动画。
     * 
     * @param element DOM 元素
     */
    void StopAllAnimations(std::shared_ptr<Element> element);
    
    /**
     * @brief 停止所有动画 (RenderObject 版本，兼容接口)
     * 
     * 停止指定对象的所有动画。
     * 
     * @param object 渲染对象
     */
    void StopAllAnimations(RenderObject* object);
    
    /**
     * @brief 暂停动画 (Element 版本)
     * 
     * 暂停指定元素的指定动画。
     * 
     * @param element DOM 元素
     * @param name 动画名称
     */
    void PauseAnimation(std::shared_ptr<Element> element, const std::string& name);
    
    /**
     * @brief 暂停动画 (RenderObject 版本，兼容接口)
     * 
     * 暂停指定对象的指定动画。
     * 
     * @param object 渲染对象
     * @param name 动画名称
     */
    void PauseAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 恢复动画 (Element 版本)
     * 
     * 恢复指定元素的指定动画。
     * 
     * @param element DOM 元素
     * @param name 动画名称
     */
    void ResumeAnimation(std::shared_ptr<Element> element, const std::string& name);
    
    /**
     * @brief 恢复动画 (RenderObject 版本，兼容接口)
     * 
     * 恢复指定对象的指定动画。
     * 
     * @param object 渲染对象
     * @param name 动画名称
     */
    void ResumeAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 更新所有动画 (每帧调用)
     * 
     * 更新所有运行中的动画状态，计算当前进度和属性值。
     * 
     * @param current_time 当前时间 (秒)
     * 
     * @note 应该在每帧渲染前调用
     */
    void Update(double current_time);
    
    /**
     * @brief 获取当前动画属性值 (Element 版本)
     * 
     * 获取指定元素的指定动画在当前时刻的属性值。
     * 
     * @param element DOM 元素
     * @param name 动画名称
     * @return 属性映射 (属性名 -> 属性值)，如果动画不存在返回 std::nullopt
     */
    std::optional<std::map<std::string, std::string>> 
        GetCurrentProperties(std::shared_ptr<Element> element, const std::string& name) const;
    
    /**
     * @brief 获取当前动画属性值 (RenderObject 版本，兼容接口)
     * 
     * 获取指定对象的指定动画在当前时刻的属性值。
     * 
     * @param object 渲染对象
     * @param name 动画名称
     * @return 属性映射 (属性名 -> 属性值)，如果动画不存在返回 std::nullopt
     */
    std::optional<std::map<std::string, std::string>> 
        GetCurrentProperties(RenderObject* object, const std::string& name) const;
    
    /**
     * @brief 获取所有运行中的动画
     * @return 运行中的动画列表
     */
    const std::vector<RunningAnimation>& GetRunningAnimations() const {
        return running_animations_;
    }
    
    /**
     * @brief 清除所有动画和 @keyframes 规则
     */
    void Clear();
    
    /**
     * @brief 只清除运行中的动画，保留 @keyframes 规则
     * 
     * 在渲染树重建时使用，避免丢失 @keyframes 定义
     */
    void ClearRunningAnimations();

    /**
     * @brief 获取已注册的 @keyframes 规则
     * @param name 动画名称
     * @return 规则指针，如果不存在返回 nullptr
     */
    const KeyframesRule* GetKeyframes(const std::string& name) const;

    /**
     * @brief 获取动画优化器
     * @return 动画优化器引用
     */
    AnimationOptimizer& GetOptimizer() { return optimizer_; }

    /**
     * @brief 获取动画优化器 (const 版本)
     * @return 动画优化器引用
     */
    const AnimationOptimizer& GetOptimizer() const { return optimizer_; }

    /**
     * @brief 启用/禁用性能优化
     * @param enabled 是否启用
     */
    void SetOptimizationEnabled(bool enabled) { optimization_enabled_ = enabled; }

    /**
     * @brief 检查性能优化是否启用
     * @return 是否启用
     */
    bool IsOptimizationEnabled() const { return optimization_enabled_; }

private:
    std::map<std::string, KeyframesRule> keyframes_rules_;  ///< @keyframes 规则映射
    std::vector<RunningAnimation> running_animations_;      ///< 运行中的动画列表
    AnimationOptimizer optimizer_;                          ///< 动画优化器
    bool optimization_enabled_ = true;                      ///< 是否启用性能优化
    
    /**
     * @brief 计算当前帧的属性值
     * 
     * 根据动画进度和关键帧规则，计算当前帧的属性值。
     * 
     * @param anim 运行中的动画
     * @param progress 动画进度 (0.0 - 1.0)
     * @return 属性映射 (属性名 -> 属性值)
     */
    std::map<std::string, std::string> ComputeCurrentFrame(
        const RunningAnimation& anim, float progress) const;
    
    /**
     * @brief 计算动画进度
     * 
     * 根据动画配置和当前时间，计算动画进度。
     * 
     * @param anim 运行中的动画
     * @param current_time 当前时间 (秒)
     * @return 动画进度 (0.0 - 1.0)
     */
    float ComputeProgress(const RunningAnimation& anim, double current_time) const;
    
    /**
     * @brief 应用缓动函数
     * 
     * 将线性进度转换为缓动进度。
     * 
     * @param progress 线性进度 (0.0 - 1.0)
     * @param timing_function 缓动函数类型
     * @param bezier 自定义贝塞尔曲线
     * @return 缓动进度 (0.0 - 1.0)
     */
    float ApplyEasing(float progress, TimingFunction timing_function, 
                     const CubicBezier& bezier) const;
    
    /**
     * @brief 查找运行中的动画 (Element 版本)
     * 
     * @param element DOM 元素
     * @param name 动画名称
     * @return 动画迭代器，如果未找到返回 end()
     */
    std::vector<RunningAnimation>::iterator FindAnimation(
        std::shared_ptr<Element> element, const std::string& name);
    
    /**
     * @brief 查找运行中的动画 (Element 版本，const)
     */
    std::vector<RunningAnimation>::const_iterator FindAnimation(
        std::shared_ptr<Element> element, const std::string& name) const;
    
    /**
     * @brief 查找运行中的动画 (RenderObject 版本，兼容接口)
     * 
     * @param object 渲染对象
     * @param name 动画名称
     * @return 动画迭代器，如果未找到返回 end()
     */
    std::vector<RunningAnimation>::iterator FindAnimation(
        RenderObject* object, const std::string& name);
    
    /**
     * @brief 查找运行中的动画 (RenderObject 版本，const)
     */
    std::vector<RunningAnimation>::const_iterator FindAnimation(
        RenderObject* object, const std::string& name) const;
    
    /**
     * @brief 从 RenderObject 提取关联的 Element
     * @param object 渲染对象
     * @return Element 共享指针，如果无法提取返回 nullptr
     */
    std::shared_ptr<Element> ExtractElement(RenderObject* object) const;
    
    /**
     * @brief 清理无效动画（Element 已销毁）
     */
    void CleanupInvalidAnimations();

    /**
     * @brief 触发动画事件
     *
     * 在 RenderObject 关联的 DOM 元素上触发动画事件。
     *
     * @param anim 运行中的动画
     * @param event_type 事件类型 ("animationstart", "animationend", "animationiteration")
     * @param elapsed_time 动画已运行时间（秒）
     */
    void FireAnimationEvent(const RunningAnimation& anim,
                           const std::string& event_type,
                           float elapsed_time);

    /**
     * @brief 更新单个动画（用于批量更新）
     *
     * @param element DOM 元素
     * @param name 动画名称
     * @param current_time 当前时间
     */
    void UpdateSingleAnimation(std::shared_ptr<Element> element, const std::string& name, double current_time);
};

} // namespace lightui

#endif // LIGHTUI_CORE_RENDER_ANIMATION_CONTROLLER_H_

