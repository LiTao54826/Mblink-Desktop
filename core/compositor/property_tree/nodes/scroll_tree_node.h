/**
 * @file scroll_tree_node.h
 * @brief 滚动树节点
 *
 * 滚动树节点管理滚动容器的属性：
 * - 容器尺寸（可视区域）
 * - 内容尺寸
 * - 滚动偏移
 * - 滚动方向
 *
 * 参考 Chromium Blink: cc/trees/scroll_node.h
 */

#pragma once

#include "core/compositor/property_tree/nodes/property_tree_node.h"
#include "include/core/SkSize.h"
#include "include/core/SkPoint.h"

namespace mblink {

// 前向声明
class RenderObject;
class TransformTreeNode;

/**
 * @brief 滚动方向
 */
enum class ScrollDirection : uint8_t {
    kNone = 0,
    kHorizontal = 1 << 0,
    kVertical = 1 << 1,
    kBoth = kHorizontal | kVertical,
};

inline ScrollDirection operator|(ScrollDirection a, ScrollDirection b) {
    return static_cast<ScrollDirection>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ScrollDirection operator&(ScrollDirection a, ScrollDirection b) {
    return static_cast<ScrollDirection>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool HasDirection(ScrollDirection dirs, ScrollDirection dir) {
    return (dirs & dir) != ScrollDirection::kNone;
}

/**
 * @brief 滚动树节点
 *
 * 存储滚动容器的信息，支持：
 * - 滚动范围计算
 * - 滚动偏移直接更新（不触发重新光栅化）
 * - 合成器线程滚动
 */
class ScrollTreeNode : public PropertyTreeNode<ScrollTreeNode> {
public:
    ScrollTreeNode();
    ~ScrollTreeNode() override = default;

    // =========================================================================
    // 容器尺寸
    // =========================================================================

    /**
     * @brief 获取容器尺寸（可视区域）
     */
    const SkSize& GetContainerSize() const { return container_size_; }

    /**
     * @brief 设置容器尺寸
     */
    void SetContainerSize(const SkSize& size);

    /**
     * @brief 设置容器尺寸
     */
    void SetContainerSize(float width, float height) {
        SetContainerSize(SkSize::Make(width, height));
    }

    // =========================================================================
    // 内容尺寸
    // =========================================================================

    /**
     * @brief 获取内容尺寸
     */
    const SkSize& GetContentSize() const { return content_size_; }

    /**
     * @brief 设置内容尺寸
     */
    void SetContentSize(const SkSize& size);

    /**
     * @brief 设置内容尺寸
     */
    void SetContentSize(float width, float height) {
        SetContentSize(SkSize::Make(width, height));
    }

    // =========================================================================
    // 滚动偏移
    // =========================================================================

    /**
     * @brief 获取当前滚动偏移
     */
    const SkPoint& GetScrollOffset() const { return scroll_offset_; }

    /**
     * @brief 设置滚动偏移
     * @param offset 新的滚动偏移
     * @note 会自动限制在有效范围内
     */
    void SetScrollOffset(const SkPoint& offset);

    /**
     * @brief 设置滚动偏移
     */
    void SetScrollOffset(float x, float y) {
        SetScrollOffset(SkPoint::Make(x, y));
    }

    /**
     * @brief 滚动指定距离
     * @param dx 水平滚动距离
     * @param dy 垂直滚动距离
     */
    void ScrollBy(float dx, float dy);

    /**
     * @brief 滚动到指定位置
     */
    void ScrollTo(float x, float y) {
        SetScrollOffset(x, y);
    }

    // =========================================================================
    // 滚动范围
    // =========================================================================

    /**
     * @brief 获取最大滚动偏移
     * @return 最大滚动偏移（内容尺寸 - 容器尺寸，不小于 0）
     */
    SkPoint GetMaxScrollOffset() const;

    /**
     * @brief 获取最小滚动偏移（通常为 0）
     */
    SkPoint GetMinScrollOffset() const { return SkPoint::Make(0, 0); }

    /**
     * @brief 检查是否可以滚动
     */
    bool CanScroll() const;

    // =========================================================================
    // 滚动方向
    // =========================================================================

    /**
     * @brief 获取可滚动方向
     */
    ScrollDirection GetScrollDirection() const;

    /**
     * @brief 是否可以水平滚动
     */
    bool CanScrollHorizontally() const;

    /**
     * @brief 是否可以垂直滚动
     */
    bool CanScrollVertically() const;

    // =========================================================================
    // 变换关联
    // =========================================================================

    /**
     * @brief 获取关联的滚动变换节点
     * @note 滚动偏移会应用到此变换节点
     */
    TransformTreeNode* GetScrollTransformNode() const { return scroll_transform_node_; }

    /**
     * @brief 设置关联的滚动变换节点
     */
    void SetScrollTransformNode(TransformTreeNode* node) { scroll_transform_node_ = node; }

    // =========================================================================
    // 根滚动器
    // =========================================================================

    /**
     * @brief 是否是根滚动器
     */
    bool IsRootScroller() const { return is_root_scroller_; }

    /**
     * @brief 设置是否是根滚动器
     */
    void SetIsRootScroller(bool is_root) { is_root_scroller_ = is_root; }

    // =========================================================================
    // 合成器线程滚动
    // =========================================================================

    /**
     * @brief 是否可以在合成器线程滚动
     * @note 如果滚动不会触发 JavaScript 事件处理，可以在合成器线程滚动
     */
    bool CanCompositorScroll() const { return can_compositor_scroll_; }

    /**
     * @brief 设置是否可以在合成器线程滚动
     */
    void SetCanCompositorScroll(bool can) { can_compositor_scroll_ = can; }

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
    /**
     * @brief 限制滚动偏移在有效范围内
     */
    SkPoint ClampScrollOffset(const SkPoint& offset) const;

    // 容器尺寸
    SkSize container_size_ = {0, 0};
    
    // 内容尺寸
    SkSize content_size_ = {0, 0};
    
    // 滚动偏移
    SkPoint scroll_offset_ = {0, 0};
    
    // 关联的滚动变换节点
    TransformTreeNode* scroll_transform_node_ = nullptr;
    
    // 是否是根滚动器
    bool is_root_scroller_ = false;
    
    // 是否可以在合成器线程滚动
    bool can_compositor_scroll_ = true;
    
    // 关联的 RenderObject
    RenderObject* render_object_ = nullptr;
};

} // namespace mblink
