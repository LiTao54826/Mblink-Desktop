/**
 * @file property_tree_builder.h
 * @brief 属性树构建器
 *
 * 从 RenderObject 树构建属性树：
 * - 遍历 RenderObject 树
 * - 根据样式创建属性树节点
 * - 维护 RenderObject 到属性树状态的映射
 *
 * 参考 Chromium Blink: platform/graphics/paint/property_tree_builder.h
 */

#pragma once

#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include <unordered_map>
#include <memory>

namespace mblink {

// 前向声明
class RenderObject;

/**
 * @brief 属性树构建器
 *
 * 负责从 RenderObject 树构建四棵属性树。
 * 支持完整构建和增量更新。
 */
class PropertyTreeBuilder {
public:
    /**
     * @brief 构造函数
     * @param trees 属性树集合
     */
    explicit PropertyTreeBuilder(PropertyTrees& trees);
    
    ~PropertyTreeBuilder() = default;

    // 禁止拷贝
    PropertyTreeBuilder(const PropertyTreeBuilder&) = delete;
    PropertyTreeBuilder& operator=(const PropertyTreeBuilder&) = delete;

    // =========================================================================
    // 构建方法
    // =========================================================================

    /**
     * @brief 完整构建属性树
     * @param root RenderObject 树的根节点
     */
    void Build(RenderObject* root);

    /**
     * @brief 增量更新属性树
     * @param changed_node 发生变化的 RenderObject
     */
    void Update(RenderObject* changed_node);

    /**
     * @brief 清除所有构建状态
     */
    void Clear();

    // =========================================================================
    // 状态查询
    // =========================================================================

    /**
     * @brief 获取 RenderObject 的属性树状态
     * @param obj RenderObject
     * @return 属性树状态
     */
    PropertyTreeState GetStateForRenderObject(RenderObject* obj) const;

    /**
     * @brief 检查 RenderObject 是否有属性树状态
     */
    bool HasStateForRenderObject(RenderObject* obj) const;

private:
    // =========================================================================
    // 递归构建
    // =========================================================================

    /**
     * @brief 递归构建属性树
     * @param obj 当前 RenderObject
     * @param parent_state 父节点的属性树状态
     */
    void BuildRecursive(RenderObject* obj, const PropertyTreeState& parent_state);

    // =========================================================================
    // 节点创建判断
    // =========================================================================

    /**
     * @brief 判断是否需要创建变换节点
     */
    bool NeedsTransformNode(RenderObject* obj) const;

    /**
     * @brief 判断是否需要创建裁剪节点
     */
    bool NeedsClipNode(RenderObject* obj) const;

    /**
     * @brief 判断是否需要创建效果节点
     */
    bool NeedsEffectNode(RenderObject* obj) const;

    /**
     * @brief 判断是否需要创建滚动节点
     */
    bool NeedsScrollNode(RenderObject* obj) const;

    // =========================================================================
    // 节点创建
    // =========================================================================

    /**
     * @brief 创建变换节点（如果需要）
     * @param obj RenderObject
     * @param parent 父变换节点
     * @return 新创建的节点或父节点
     */
    TransformTreeNode* CreateTransformNodeIfNeeded(
        RenderObject* obj,
        TransformTreeNode* parent);

    /**
     * @brief 创建裁剪节点（如果需要）
     * @param obj RenderObject
     * @param parent 父裁剪节点
     * @param transform 关联的变换节点
     * @return 新创建的节点或父节点
     */
    ClipTreeNode* CreateClipNodeIfNeeded(
        RenderObject* obj,
        ClipTreeNode* parent,
        TransformTreeNode* transform);

    /**
     * @brief 创建效果节点（如果需要）
     * @param obj RenderObject
     * @param parent 父效果节点
     * @param transform 关联的变换节点
     * @param clip 关联的裁剪节点
     * @return 新创建的节点或父节点
     */
    EffectTreeNode* CreateEffectNodeIfNeeded(
        RenderObject* obj,
        EffectTreeNode* parent,
        TransformTreeNode* transform,
        ClipTreeNode* clip);

    /**
     * @brief 创建滚动节点（如果需要）
     * @param obj RenderObject
     * @param parent 父滚动节点
     * @param transform 关联的变换节点
     * @return 新创建的节点或父节点
     */
    ScrollTreeNode* CreateScrollNodeIfNeeded(
        RenderObject* obj,
        ScrollTreeNode* parent,
        TransformTreeNode* transform);

    // =========================================================================
    // 辅助方法
    // =========================================================================

    /**
     * @brief 从 ComputedStyle 创建变换矩阵
     */
    SkM44 CreateTransformMatrix(RenderObject* obj) const;

    /**
     * @brief 从 ComputedStyle 获取变换原点
     */
    SkV3 GetTransformOrigin(RenderObject* obj) const;

    /**
     * @brief 检查是否有活动的 transform 动画
     */
    bool HasActiveTransformAnimation(RenderObject* obj) const;

    /**
     * @brief 检查是否有活动的 opacity 动画
     */
    bool HasActiveOpacityAnimation(RenderObject* obj) const;

    /**
     * @brief 检查 will-change 属性
     */
    bool HasWillChangeTransform(RenderObject* obj) const;
    bool HasWillChangeOpacity(RenderObject* obj) const;

    /**
     * @brief 将构建的属性树状态应用到 RenderObject
     * 
     * 这是关键步骤：将 render_object_states_ 中的状态设置到对应的 RenderObject 上，
     * 这样 AnimationApplicator 才能通过 object->GetPropertyTreeState() 获取状态，
     * 从而启用直接属性更新优化（不触发光栅化）。
     */
    void ApplyStatesToRenderObjects();

    // =========================================================================
    // 成员变量
    // =========================================================================

    PropertyTrees& trees_;
    
    // RenderObject 到属性树状态的映射
    std::unordered_map<RenderObject*, PropertyTreeState> render_object_states_;
};

} // namespace mblink
