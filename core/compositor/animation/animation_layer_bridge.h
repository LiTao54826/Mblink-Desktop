/**
 * @file animation_layer_bridge.h
 * @brief 动画层桥接器 - 连接动画系统和合成层系统
 *
 * AnimationLayerBridge 负责：
 * - 检测动画开始/结束，触发层提升/降级
 * - 对于有独立层的元素，直接更新层变换/透明度
 * - 批量处理动画更新，减少合成器调用
 *
 * 优化原理：
 * - transform/opacity 动画不需要重新光栅化
 * - 直接更新层属性，由合成器处理
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 前向声明
class SkMatrix;

namespace mbink {

// 前向声明
class RenderObject;
class CompositorLayer;
class LayerTreeBuilder;
class AnimationController;

/**
 * @brief 动画更新类型
 */
enum class AnimationUpdateType {
    None,           // 无更新
    Transform,      // 变换更新（不需要重新光栅化）
    Opacity,        // 透明度更新（不需要重新光栅化）
    Layout,         // 布局属性更新（需要重新布局和光栅化）
    Paint           // 绘制属性更新（需要重新光栅化）
};

/**
 * @brief 动画更新信息
 */
struct AnimationUpdate {
    RenderObject* object = nullptr;
    AnimationUpdateType type = AnimationUpdateType::None;
    
    // Transform 更新数据
    bool has_transform = false;
    float transform_values[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};  // 3x3 矩阵
    
    // Opacity 更新数据
    bool has_opacity = false;
    float opacity = 1.0f;
};

/**
 * @brief 动画层桥接器
 *
 * 连接动画系统和合成层系统，实现高效的动画渲染。
 */
class AnimationLayerBridge {
public:
    AnimationLayerBridge();
    ~AnimationLayerBridge();

    // 禁止拷贝
    AnimationLayerBridge(const AnimationLayerBridge&) = delete;
    AnimationLayerBridge& operator=(const AnimationLayerBridge&) = delete;

    /**
     * @brief 设置层树构建器
     */
    void SetLayerTreeBuilder(LayerTreeBuilder* builder) { layer_tree_builder_ = builder; }

    /**
     * @brief 设置动画控制器
     */
    void SetAnimationController(AnimationController* controller) { animation_controller_ = controller; }

    // =========================================================================
    // 动画生命周期
    // =========================================================================

    /**
     * @brief 通知动画开始
     * @param object 动画目标对象
     * @param animation_name 动画名称
     * @param properties 动画属性列表
     *
     * 如果动画包含 transform/opacity，会触发层提升
     */
    void OnAnimationStart(RenderObject* object,
                          const std::string& animation_name,
                          const std::vector<std::string>& properties);

    /**
     * @brief 通知动画结束
     * @param object 动画目标对象
     * @param animation_name 动画名称
     *
     * 如果没有其他动画，可能触发层降级
     */
    void OnAnimationEnd(RenderObject* object, const std::string& animation_name);

    // =========================================================================
    // 动画更新
    // =========================================================================

    /**
     * @brief 开始动画更新批处理
     *
     * 在帧开始时调用，准备收集动画更新
     */
    void BeginAnimationUpdates();

    /**
     * @brief 应用动画属性值
     * @param object 渲染对象
     * @param property 属性名
     * @param value 属性值
     * @return 更新类型
     *
     * 对于 transform/opacity，直接更新层属性
     * 对于其他属性，返回需要的更新类型
     */
    AnimationUpdateType ApplyAnimationProperty(RenderObject* object,
                                                const std::string& property,
                                                const std::string& value);

    /**
     * @brief 结束动画更新批处理
     * @return 是否有层属性更新（需要合成器更新）
     *
     * 在帧结束时调用，处理所有收集的更新
     */
    bool EndAnimationUpdates();

    /**
     * @brief 获取待处理的动画更新
     */
    const std::vector<AnimationUpdate>& GetPendingUpdates() const { return pending_updates_; }

    // =========================================================================
    // 查询
    // =========================================================================

    /**
     * @brief 检查对象是否有 transform 动画
     */
    bool HasTransformAnimation(RenderObject* object) const;

    /**
     * @brief 检查对象是否有 opacity 动画
     */
    bool HasOpacityAnimation(RenderObject* object) const;

    /**
     * @brief 检查对象是否因动画而被提升
     */
    bool IsPromotedForAnimation(RenderObject* object) const;

    /**
     * @brief 获取因动画提升的对象数量
     */
    size_t GetAnimationPromotedCount() const { return animation_promoted_objects_.size(); }

    // =========================================================================
    // 清理
    // =========================================================================

    /**
     * @brief 清除所有状态
     */
    void Clear();

private:
    /**
     * @brief 检查是否应该使用层优化
     * @param object 渲染对象
     * @return true 如果对象有独立层
     */
    bool ShouldUseLayerOptimization(RenderObject* object) const;

    /**
     * @brief 更新层变换
     * @param object 渲染对象
     * @param transform_str transform 字符串
     * @return true 如果成功
     */
    bool UpdateLayerTransform(RenderObject* object, const std::string& transform_str);

    /**
     * @brief 更新层透明度
     * @param object 渲染对象
     * @param opacity 透明度值
     * @return true 如果成功
     */
    bool UpdateLayerOpacity(RenderObject* object, float opacity);

    /**
     * @brief 请求层提升
     * @param object 渲染对象
     * @param reason 提升原因
     */
    void RequestLayerPromotion(RenderObject* object, const std::string& reason);

    /**
     * @brief 请求层降级
     * @param object 渲染对象
     */
    void RequestLayerDemotion(RenderObject* object);

    // 关联的组件
    LayerTreeBuilder* layer_tree_builder_ = nullptr;
    AnimationController* animation_controller_ = nullptr;

    // 动画状态跟踪
    struct AnimationState {
        std::unordered_set<std::string> transform_animations;
        std::unordered_set<std::string> opacity_animations;
        std::unordered_set<std::string> other_animations;
    };
    std::unordered_map<RenderObject*, AnimationState> animation_states_;

    // 因动画而提升的对象
    std::unordered_set<RenderObject*> animation_promoted_objects_;

    // 待处理的更新
    std::vector<AnimationUpdate> pending_updates_;

    // 是否在批处理中
    bool in_batch_ = false;

    // 是否有层属性更新
    bool has_layer_updates_ = false;
};

} // namespace mbink
