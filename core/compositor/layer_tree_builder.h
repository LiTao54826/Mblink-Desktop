/**
 * @file layer_tree_builder.h
 * @brief 层树构建器 - 从渲染树构建合成层树
 *
 * LayerTreeBuilder 负责：
 * - 遍历渲染树，决定哪些元素需要独立层
 * - 构建层树的父子关系
 * - 增量更新层树
 *
 * 层提升条件（参考 Chrome Blink）：
 * - will-change: transform/opacity
 * - position: fixed
 * - CSS transform/opacity 动画
 * - 可滚动容器
 */

#pragma once

#include "compositor_layer.h"
#include <memory>
#include <unordered_map>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 层树构建器
 *
 * 从渲染树构建合成层树，决定哪些元素需要独立层。
 */
class LayerTreeBuilder {
public:
    LayerTreeBuilder();
    ~LayerTreeBuilder();

    // 禁止拷贝
    LayerTreeBuilder(const LayerTreeBuilder&) = delete;
    LayerTreeBuilder& operator=(const LayerTreeBuilder&) = delete;

    /**
     * @brief 从渲染树构建层树
     * @param root 渲染树根节点
     * @return 层树根节点
     */
    std::shared_ptr<CompositorLayer> Build(RenderObject* root);

    /**
     * @brief 增量更新层树
     * @param changed_node 发生变化的渲染节点
     */
    void Update(RenderObject* changed_node);

    /**
     * @brief 检查元素是否需要独立层
     * @param obj 渲染对象
     * @return 层提升原因（None 表示不需要独立层）
     */
    LayerPromotionReason ShouldPromote(RenderObject* obj) const;

    /**
     * @brief 获取渲染对象关联的层
     * @param obj 渲染对象
     * @return 关联的层，如果没有则返回 nullptr
     */
    std::shared_ptr<CompositorLayer> GetLayerForRenderObject(RenderObject* obj) const;

    /**
     * @brief 获取层数量
     */
    size_t GetLayerCount() const { return layer_count_; }

    /**
     * @brief 清除所有层
     */
    void Clear();

    /**
     * @brief 设置是否启用层提升
     * @param enabled 是否启用
     * @note 禁用时所有内容渲染到根层
     */
    void SetLayerPromotionEnabled(bool enabled) { layer_promotion_enabled_ = enabled; }

    /**
     * @brief 检查层提升是否启用
     */
    bool IsLayerPromotionEnabled() const { return layer_promotion_enabled_; }

    /**
     * @brief 设置 DPI 缩放比
     * @param scale DPI 缩放比
     */
    void SetDpiScale(float scale) { dpi_scale_ = scale; }

    /**
     * @brief 获取 DPI 缩放比
     */
    float GetDpiScale() const { return dpi_scale_; }

private:
    /**
     * @brief 递归构建层树
     * @param obj 当前渲染对象
     * @param parent_layer 父层
     */
    void BuildRecursive(RenderObject* obj, CompositorLayer* parent_layer);

    /**
     * @brief 为渲染对象创建层
     * @param obj 渲染对象
     * @param reason 提升原因
     * @return 新创建的层
     */
    std::shared_ptr<CompositorLayer> CreateLayer(RenderObject* obj, LayerPromotionReason reason);

    /**
     * @brief 更新层的边界
     * @param layer 层
     * @param obj 关联的渲染对象
     */
    void UpdateLayerBounds(CompositorLayer* layer, RenderObject* obj);

    /**
     * @brief 检查是否有 will-change: transform
     */
    bool HasWillChangeTransform(RenderObject* obj) const;

    /**
     * @brief 检查是否有 will-change: opacity
     */
    bool HasWillChangeOpacity(RenderObject* obj) const;

    /**
     * @brief 检查是否有 position: fixed
     */
    bool HasPositionFixed(RenderObject* obj) const;

    /**
     * @brief 检查是否有 transform 动画
     */
    bool HasTransformAnimation(RenderObject* obj) const;

    /**
     * @brief 检查是否有 opacity 动画
     */
    bool HasOpacityAnimation(RenderObject* obj) const;

    /**
     * @brief 检查是否是可滚动容器
     */
    bool IsScrollableContainer(RenderObject* obj) const;

    // 根层
    std::shared_ptr<CompositorLayer> root_layer_;

    // 渲染对象到层的映射
    std::unordered_map<RenderObject*, std::shared_ptr<CompositorLayer>> render_object_to_layer_;

    // 层数量
    size_t layer_count_ = 0;

    // 是否启用层提升
    bool layer_promotion_enabled_ = true;

    // DPI 缩放比
    float dpi_scale_ = 1.0f;
};

} // namespace lightui
