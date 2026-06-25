/**
 * @file clip_tree_node.h
 * @brief 裁剪树节点
 *
 * 裁剪树节点管理元素的裁剪属性：
 * - 裁剪矩形
 * - 圆角半径
 * - clip-path 路径裁剪
 * - 关联的变换节点
 *
 * 参考 Chromium Blink: cc/trees/clip_node.h
 */

#pragma once

#include "core/compositor/property_tree/nodes/property_tree_node.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPath.h"
#include <memory>

namespace mblink {

// 前向声明
class RenderObject;
class TransformTreeNode;

/**
 * @brief 裁剪类型
 */
enum class ClipType {
    kNone,              // 无裁剪
    kRect,              // 矩形裁剪
    kRoundedRect,       // 圆角矩形裁剪
    kPath,              // 路径裁剪（clip-path）
};

/**
 * @brief 裁剪树节点
 *
 * 存储元素的裁剪信息，支持：
 * - 矩形裁剪（overflow: hidden）
 * - 圆角裁剪（border-radius）
 * - 路径裁剪（clip-path）
 * - 累积裁剪区域计算
 */
class ClipTreeNode : public PropertyTreeNode<ClipTreeNode> {
public:
    ClipTreeNode();
    ~ClipTreeNode() override = default;

    // =========================================================================
    // 裁剪类型
    // =========================================================================

    /**
     * @brief 获取裁剪类型
     */
    ClipType GetClipType() const { return type_; }

    /**
     * @brief 是否有裁剪
     */
    bool HasClip() const { return type_ != ClipType::kNone; }

    // =========================================================================
    // 矩形裁剪
    // =========================================================================

    /**
     * @brief 获取裁剪矩形（在关联变换空间中）
     */
    const SkRect& GetClipRect() const { return clip_rect_; }

    /**
     * @brief 设置裁剪矩形
     * @param rect 裁剪矩形
     */
    void SetClipRect(const SkRect& rect);

    /**
     * @brief 清除裁剪
     */
    void ClearClip();

    // =========================================================================
    // 圆角裁剪
    // =========================================================================

    /**
     * @brief 获取圆角半径
     * @return 四个角的圆角半径数组 [左上, 右上, 右下, 左下]
     */
    const SkVector* GetRadii() const { return radii_; }

    /**
     * @brief 设置圆角半径
     * @param radii 四个角的圆角半径
     */
    void SetRadii(const SkVector radii[4]);

    /**
     * @brief 设置统一圆角半径
     * @param radius 所有角的圆角半径
     */
    void SetUniformRadius(float radius);

    /**
     * @brief 是否有圆角
     */
    bool HasRoundedCorners() const;

    /**
     * @brief 获取圆角矩形
     */
    SkRRect GetRoundedRect() const;

    // =========================================================================
    // 路径裁剪
    // =========================================================================

    /**
     * @brief 获取裁剪路径
     */
    const SkPath* GetClipPath() const { return clip_path_.get(); }

    /**
     * @brief 设置裁剪路径
     * @param path 裁剪路径
     */
    void SetClipPath(std::unique_ptr<SkPath> path);

    /**
     * @brief 设置裁剪路径（拷贝）
     */
    void SetClipPath(const SkPath& path);

    /**
     * @brief 清除裁剪路径
     */
    void ClearClipPath();

    // =========================================================================
    // 变换关联
    // =========================================================================

    /**
     * @brief 获取关联的变换节点
     * @note 裁剪区域定义在此变换空间中
     */
    TransformTreeNode* GetTransformNode() const { return transform_node_; }

    /**
     * @brief 设置关联的变换节点
     */
    void SetTransformNode(TransformTreeNode* node) { transform_node_ = node; }

    // =========================================================================
    // 裁剪区域计算
    // =========================================================================

    /**
     * @brief 计算在目标变换空间中的裁剪区域
     * @param target_space 目标变换空间
     * @return 变换后的裁剪矩形
     */
    SkRect GetClipRectInSpace(const TransformTreeNode* target_space) const;

    /**
     * @brief 计算累积裁剪区域（考虑所有祖先）
     * @param target_space 目标变换空间
     * @return 累积裁剪矩形
     */
    SkRect GetAccumulatedClipRect(const TransformTreeNode* target_space) const;

    /**
     * @brief 使累积裁剪缓存失效
     */
    void InvalidateAccumulatedClipCache();

    // =========================================================================
    // RenderObject 关联
    // =========================================================================

    /**
     * @brief 获取关联的 RenderObject
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief 设置关联的 RenderObject
     */
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }

private:
    // 裁剪类型
    ClipType type_ = ClipType::kNone;
    
    // 裁剪矩形
    SkRect clip_rect_ = SkRect::MakeEmpty();
    
    // 圆角半径 [左上, 右上, 右下, 左下]
    SkVector radii_[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    
    // 裁剪路径
    std::unique_ptr<SkPath> clip_path_;
    
    // 关联的变换节点
    TransformTreeNode* transform_node_ = nullptr;
    
    // 关联的 RenderObject
    RenderObject* render_object_ = nullptr;
    
    // 缓存的累积裁剪
    mutable SkRect cached_accumulated_clip_;
    mutable const TransformTreeNode* cached_target_space_ = nullptr;
    mutable bool accumulated_clip_valid_ = false;
};

} // namespace mblink
