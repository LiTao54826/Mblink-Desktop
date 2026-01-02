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
#include <vector>

namespace lightui {

// 前向声明
class RenderObject;
struct PendingLayerUpdate;

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

    /**
     * @brief 更新层的边界
     * @param layer 层
     * @param obj 关联的渲染对象
     * @note 会计算动画边界扩展
     */
    void UpdateLayerBounds(CompositorLayer* layer, RenderObject* obj);

    // =========================================================================
    // 增量更新接口（新增）
    // =========================================================================

    /**
     * @brief 查找 RenderObject 应该附加到的父层
     * @param obj RenderObject
     * @return 父层，如果是 fixed 元素返回根层
     *
     * 查找逻辑：
     * 1. 如果是 fixed 元素，返回根层
     * 2. 否则，向上遍历 RenderObject 树，找到第一个有层的祖先
     */
    CompositorLayer* FindParentLayerForObject(RenderObject* obj) const;

    /**
     * @brief 为 RenderObject 添加层（增量）
     * @param obj 需要层的 RenderObject
     * @param reason 层提升原因
     * @return 新创建的层，失败返回 nullptr
     *
     * 关键改进：
     * 1. 先找到正确的父层
     * 2. 创建层并附加到父层
     * 3. 附加后再计算边界（此时有父层信息）
     */
    std::shared_ptr<CompositorLayer> AddLayerForObject(
        RenderObject* obj, LayerPromotionReason reason);

    /**
     * @brief 移除 RenderObject 的层（增量）
     * @param obj 要移除层的 RenderObject
     * @return 是否成功移除
     *
     * 关键改进：
     * 1. 将子层转移到父层
     * 2. 从父层移除当前层
     * 3. 清理映射关系
     */
    bool RemoveLayerForObject(RenderObject* obj);

    /**
     * @brief 延迟更新层边界（在父层已知后调用）
     * @param layer 要更新的层
     * @param obj 关联的 RenderObject
     *
     * 与 UpdateLayerBounds() 的区别：
     * - 确保在层已附加到父层后调用
     * - 正确处理 fixed 元素的视口坐标
     */
    void UpdateLayerBoundsDeferred(CompositorLayer* layer, RenderObject* obj);

    /**
     * @brief 检查是否可以增量更新
     * @return 如果可以增量更新返回 true
     *
     * 不能增量更新的情况：
     * - 根层不存在
     * - 层树结构严重损坏
     * - 需要重新排序大量层
     */
    bool CanIncrementalUpdate() const;

    /**
     * @brief 获取层树版本号（用于检测变化）
     */
    uint64_t GetTreeVersion() const { return tree_version_; }

    /**
     * @brief 递增层树版本号
     */
    void IncrementTreeVersion() { ++tree_version_; }

    /**
     * @brief 获取根层
     */
    std::shared_ptr<CompositorLayer> GetRootLayer() const { return root_layer_; }

    /**
     * @brief 增量构建层树
     * @param root 渲染树根节点
     * @param pending_updates 待处理的更新列表
     * @return 是否成功
     *
     * 与 Build() 不同，IncrementalBuild() 不会清除现有层树，
     * 而是根据 pending_updates 进行增量修改。
     */
    bool IncrementalBuild(RenderObject* root,
                          const std::vector<PendingLayerUpdate>& pending_updates);

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

    // 层树版本号，每次修改递增
    uint64_t tree_version_ = 0;
};

} // namespace lightui
