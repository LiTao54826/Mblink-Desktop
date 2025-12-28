/**
 * @file property_tree_state.h
 * @brief 属性树状态
 *
 * 属性树状态描述一个绘制块的渲染上下文，包含：
 * - 变换树节点引用
 * - 裁剪树节点引用
 * - 效果树节点引用
 * - 滚动树节点引用
 *
 * 用于层化决策和坐标转换。
 *
 * 参考 Chromium Blink: platform/graphics/paint/property_tree_state.h
 */

#pragma once

#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include "core/compositor/property_tree/nodes/clip_tree_node.h"
#include "core/compositor/property_tree/nodes/effect_tree_node.h"
#include "core/compositor/property_tree/nodes/scroll_tree_node.h"

namespace lightui {

/**
 * @brief 属性树状态差异
 */
struct PropertyTreeStateDifference {
    bool transform_changed = false;
    bool clip_changed = false;
    bool effect_changed = false;
    bool scroll_changed = false;
    
    /**
     * @brief 是否有任何变化
     */
    bool HasAnyChange() const {
        return transform_changed || clip_changed || effect_changed || scroll_changed;
    }
    
    /**
     * @brief 是否只有变换变化
     */
    bool OnlyTransformChanged() const {
        return transform_changed && !clip_changed && !effect_changed && !scroll_changed;
    }
    
    /**
     * @brief 是否只有效果变化
     */
    bool OnlyEffectChanged() const {
        return !transform_changed && !clip_changed && effect_changed && !scroll_changed;
    }
    
    /**
     * @brief 是否只有滚动变化
     */
    bool OnlyScrollChanged() const {
        return !transform_changed && !clip_changed && !effect_changed && scroll_changed;
    }
};

/**
 * @brief 属性树状态
 *
 * 描述一个绘制块的完整渲染上下文。
 * 两个具有相同属性树状态的绘制块可以合并到同一层。
 */
class PropertyTreeState {
public:
    PropertyTreeState() = default;
    
    /**
     * @brief 构造函数
     */
    PropertyTreeState(TransformTreeNode* transform,
                      ClipTreeNode* clip,
                      EffectTreeNode* effect,
                      ScrollTreeNode* scroll = nullptr);

    // =========================================================================
    // 节点访问
    // =========================================================================

    /**
     * @brief 获取变换节点
     */
    TransformTreeNode* Transform() const { return transform_; }

    /**
     * @brief 获取裁剪节点
     */
    ClipTreeNode* Clip() const { return clip_; }

    /**
     * @brief 获取效果节点
     */
    EffectTreeNode* Effect() const { return effect_; }

    /**
     * @brief 获取滚动节点
     */
    ScrollTreeNode* Scroll() const { return scroll_; }

    // =========================================================================
    // 节点设置
    // =========================================================================

    /**
     * @brief 设置变换节点
     */
    void SetTransform(TransformTreeNode* node) { transform_ = node; }

    /**
     * @brief 设置裁剪节点
     */
    void SetClip(ClipTreeNode* node) { clip_ = node; }

    /**
     * @brief 设置效果节点
     */
    void SetEffect(EffectTreeNode* node) { effect_ = node; }

    /**
     * @brief 设置滚动节点
     */
    void SetScroll(ScrollTreeNode* node) { scroll_ = node; }

    // =========================================================================
    // 比较
    // =========================================================================

    /**
     * @brief 相等比较
     */
    bool operator==(const PropertyTreeState& other) const;

    /**
     * @brief 不等比较
     */
    bool operator!=(const PropertyTreeState& other) const {
        return !(*this == other);
    }

    // =========================================================================
    // 合并判断
    // =========================================================================

    /**
     * @brief 是否可以与另一个状态合并到同一层
     * @param other 另一个状态
     * @return true 如果可以合并
     *
     * 合并条件：
     * - 变换节点相同或兼容
     * - 裁剪节点相同或兼容
     * - 效果节点相同或兼容
     * - 滚动节点相同或兼容
     */
    bool CanMergeWith(const PropertyTreeState& other) const;

    // =========================================================================
    // 差异计算
    // =========================================================================

    /**
     * @brief 计算与另一个状态的差异
     */
    PropertyTreeStateDifference ComputeDifference(const PropertyTreeState& other) const;

    // =========================================================================
    // 有效性检查
    // =========================================================================

    /**
     * @brief 是否有效（至少有一个非空节点）
     */
    bool IsValid() const {
        return transform_ != nullptr || clip_ != nullptr || 
               effect_ != nullptr || scroll_ != nullptr;
    }

    /**
     * @brief 是否完整（所有节点都非空）
     */
    bool IsComplete() const {
        return transform_ != nullptr && clip_ != nullptr && effect_ != nullptr;
    }

    // =========================================================================
    // 静态方法
    // =========================================================================

    /**
     * @brief 获取根状态
     * @note 需要在 PropertyTrees 初始化后调用
     */
    static PropertyTreeState Root();

    /**
     * @brief 创建无效状态
     */
    static PropertyTreeState Invalid() {
        return PropertyTreeState();
    }

private:
    TransformTreeNode* transform_ = nullptr;
    ClipTreeNode* clip_ = nullptr;
    EffectTreeNode* effect_ = nullptr;
    ScrollTreeNode* scroll_ = nullptr;
};

} // namespace lightui
