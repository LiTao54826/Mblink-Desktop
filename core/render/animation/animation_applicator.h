/**
 * @file animation_applicator.h
 * @brief 动画应用器 - 将动画属性值应用到渲染对象
 * 
 * AnimationApplicator 是 AnimationController 和 RenderObject 之间的桥梁，
 * 负责：
 * - 根据 ComputedStyle.animations 启动动画
 * - 获取当前动画属性值并应用到 RenderObject
 * - 处理动画播放状态变化
 * - 对于 transform/opacity 动画，通过层合成系统优化更新
 * - 支持属性树系统的直接属性更新（不触发光栅化）
 */

#ifndef LIGHTUI_CORE_RENDER_ANIMATION_APPLICATOR_H_
#define LIGHTUI_CORE_RENDER_ANIMATION_APPLICATOR_H_

#include "animation_controller.h"
#include "core/render/objects/render_object.h"
#include <string>
#include <set>
#include <map>
#include <memory>

namespace lightui {

// 前向声明
class PaintArtifactCompositor;
class PropertyTrees;
class AnimationLayerBridge;
class Element;

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
 * // 设置合成器适配器（用于层优化）
 * applicator.SetCompositorAdapter(compositor_adapter);
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
     * @brief 设置动画层桥接器
     * 
     * 用于 transform/opacity 动画的层优化。
     * 当设置了动画层桥接器时，transform/opacity 动画会直接更新层属性，
     * 而不是触发完整重绘。
     * 
     * @param bridge 动画层桥接器指针（不拥有所有权）
     */
    void SetAnimationBridge(AnimationLayerBridge* bridge) {
        animation_bridge_ = bridge;
    }
    
    /**
     * @brief 设置绘制产物合成器（属性树系统）
     * 
     * 当使用属性树系统时，transform/opacity 动画会通过
     * PaintArtifactCompositor 的 DirectlyUpdate 方法更新，
     * 完全避免光栅化。
     * 
     * @param compositor 绘制产物合成器指针（不拥有所有权）
     */
    void SetPaintArtifactCompositor(PaintArtifactCompositor* compositor) {
        paint_artifact_compositor_ = compositor;
    }
    
    /**
     * @brief 设置属性树集合
     * @param trees 属性树集合指针（不拥有所有权）
     */
    void SetPropertyTrees(PropertyTrees* trees) {
        property_trees_ = trees;
    }
    
    /**
     * @brief 检查是否使用属性树系统
     */
    bool IsUsingPropertyTreeSystem() const {
        return paint_artifact_compositor_ != nullptr && property_trees_ != nullptr;
    }
    
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
     * 对于 transform/opacity 属性，如果对象有独立层，
     * 会尝试通过层合成系统直接更新，避免重新光栅化。
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

    /// 动画层桥接器（用于层优化）
    AnimationLayerBridge* animation_bridge_ = nullptr;

    /// 绘制产物合成器（属性树系统）
    PaintArtifactCompositor* paint_artifact_compositor_ = nullptr;

    /// 属性树集合
    PropertyTrees* property_trees_ = nullptr;

    /// 跟踪每个 Element 已启动的动画名称
    /// 使用 Element* 作为键，跨渲染树重建时可保持动画启动状态
    std::map<Element*, std::set<std::string>> started_animations_;

    /**
     * @brief 清理 started_animations_ 中已失效的 Element 键
     *
     * AnimationController 使用 weak_ptr<Element> 维护运行态，
     * 这里按运行态反查，移除 applicator 中陈旧的裸指针键，避免状态污染。
     */
    void PruneStaleStartedAnimations();

    /**
     * @brief 是否启用动画调试日志（兼容两个环境变量）
     */
    bool IsDebugAnimationEnabled() const;

    /**
     * @brief 从 controller 运行态判断某元素动画是否已真正启动
     */
    bool IsAnimationRunningForElement(Element* element, const std::string& animation_name) const;

    /**
     * @brief 从 RenderObject 提取关联的 Element
     * @param object 渲染对象
     * @return Element 指针，如果无法提取返回 nullptr
     */
    Element* ExtractElement(RenderObject* object) const;
    
    /**
     * @brief 尝试通过属性树系统直接更新属性
     * 
     * 对于 transform/opacity 属性，如果对象有属性树状态，
     * 直接更新属性树节点而不触发光栅化。
     * 
     * @param object 渲染对象
     * @param property 属性名
     * @param value 属性值
     * @return true 如果成功通过属性树系统应用
     */
    bool TryApplyViaPropertyTree(RenderObject* object,
                                  const std::string& property,
                                  const std::string& value);
    
    /**
     * @brief 尝试通过层合成系统应用属性
     * 
     * 对于 transform/opacity 属性，如果对象有独立层，
     * 直接更新层属性而不触发重绘。
     * 
     * @param object 渲染对象
     * @param property 属性名
     * @param value 属性值
     * @return true 如果成功通过层系统应用
     */
    bool TryApplyViaCompositor(RenderObject* object,
                               const std::string& property,
                               const std::string& value);
    
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
