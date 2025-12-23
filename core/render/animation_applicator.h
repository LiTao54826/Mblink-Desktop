/**
 * @file animation_applicator.h
 * @brief 动画应用器 - 将动画属性值应用到渲染对象
 * 
 * AnimationApplicator 是 AnimationController 和 RenderObject 之间的桥梁，
 * 负责：
 * - 根据 ComputedStyle.animations 启动动画
 * - 获取当前动画属性值并应用到 RenderObject
 * - 处理动画播放状态变化
 */

#ifndef LIGHTUI_CORE_RENDER_ANIMATION_APPLICATOR_H_
#define LIGHTUI_CORE_RENDER_ANIMATION_APPLICATOR_H_

#include "animation_controller.h"
#include "render_object.h"
#include <string>
#include <set>
#include <map>

namespace lightui {

/**
 * @brief 动画应用器
 * 
 * 将 AnimationController 计算的动画属性值应用到 RenderObject 的 ComputedStyle。
 * 
 * 使用示例:
 * @code
 * AnimationController controller;
 * AnimationApplicator applicator(controller);
 * 
 * // 在样式解析后启动动画
 * applicator.StartAnimationsForObject(render_object);
 * 
 * // 每帧应用动画值
 * applicator.ApplyAnimationValues(render_object);
 * @endcode
 */
class AnimationApplicator {
public:
    /**
     * @brief 构造函数
     * @param controller 动画控制器引用
     */
    explicit AnimationApplicator(AnimationController& controller);
    
    /**
     * @brief 析构函数
     */
    ~AnimationApplicator();
    
    /**
     * @brief 根据 ComputedStyle 启动对象的动画
     * 
     * 读取 RenderObject 的 ComputedStyle.animations，
     * 为每个动画调用 AnimationController::StartAnimation()。
     * 
     * @param object 渲染对象
     * 
     * @note 已启动的同名动画不会重复启动
     */
    void StartAnimationsForObject(RenderObject* object);
    
    /**
     * @brief 应用当前动画值到渲染对象
     * 
     * 获取所有运行中动画的当前属性值，
     * 并将它们应用到 RenderObject 的 ComputedStyle。
     * 
     * @param object 渲染对象
     * 
     * @note 如果有属性被修改，会标记对象需要重绘
     */
    void ApplyAnimationValues(RenderObject* object);
    
    /**
     * @brief 停止对象的所有动画
     * 
     * @param object 渲染对象
     */
    void StopAnimationsForObject(RenderObject* object);
    
    /**
     * @brief 更新动画播放状态
     * 
     * 根据 ComputedStyle.animation_play_state 暂停或恢复动画。
     * 
     * @param object 渲染对象
     * @param paused 是否暂停
     */
    void SetAnimationsPaused(RenderObject* object, bool paused);
    
    /**
     * @brief 检查对象是否有活动动画
     * 
     * @param object 渲染对象
     * @return true 如果有活动动画
     */
    bool HasActiveAnimations(RenderObject* object) const;
    
    /**
     * @brief 获取对象的活动动画名称列表
     * 
     * @param object 渲染对象
     * @return 活动动画名称集合
     */
    std::set<std::string> GetActiveAnimationNames(RenderObject* object) const;
    
    /**
     * @brief 清理所有动画状态
     * 
     * 在渲染树重建前调用，清除所有已启动动画的跟踪信息。
     * 这可以防止悬空指针问题。
     */
    void Clear();

private:
    AnimationController& controller_;
    
    /// 跟踪每个对象已启动的动画名称
    std::map<RenderObject*, std::set<std::string>> started_animations_;
    
    /**
     * @brief 将单个属性值应用到 ComputedStyle
     * 
     * @param style 要修改的 ComputedStyle
     * @param property 属性名
     * @param value 属性值
     * @return true 如果属性被成功应用
     */
    bool ApplyPropertyToStyle(ComputedStyle& style,
                              const std::string& property,
                              const std::string& value);
    
    /**
     * @brief 解析数值和单位
     * @param str 数值字符串 (如 "100px", "50%")
     * @return {value, unit}
     */
    std::pair<float, std::string> ParseNumberWithUnit(const std::string& str) const;
    
    /**
     * @brief 解析颜色值
     * @param str 颜色字符串
     * @return SkColor，如果解析失败返回 SK_ColorTRANSPARENT
     */
    SkColor ParseColor(const std::string& str) const;
};

} // namespace lightui

#endif // LIGHTUI_CORE_RENDER_ANIMATION_APPLICATOR_H_
