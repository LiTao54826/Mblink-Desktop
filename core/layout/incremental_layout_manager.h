/**
 * @file incremental_layout_manager.h
 * @brief 增量布局管理器
 *
 * 管理增量布局更新，避免全量重建。
 * 通过识别布局边界，将布局变化限制在边界内部。
 */

#pragma once

#include <memory>
#include <unordered_set>
#include <vector>
#include <utility>

namespace mbink {

// 前向声明
class Window;
class Element;
class Node;
class RenderObject;

/**
 * @brief 增量布局管理器
 *
 * 管理增量布局更新，避免全量重建。
 * 主要功能：
 * 1. 处理脱离文档流元素的增删（不触发全量重建）
 * 2. 标记布局边界需要重新布局
 * 3. 更新滚动容器的滚动尺寸
 */
class IncrementalLayoutManager {
public:
    /**
     * @brief 构造函数
     * @param window 关联的窗口
     */
    explicit IncrementalLayoutManager(Window* window);

    /**
     * @brief 析构函数
     */
    ~IncrementalLayoutManager() = default;

    /**
     * @brief 处理脱离文档流元素的添加
     *
     * 直接创建 RenderObject 并添加到渲染树，
     * 不触发 InvalidateRenderTree。
     *
     * @param element 要添加的元素
     * @param parent 父节点
     * @return 如果成功处理返回 true
     */
    bool AddOutOfFlowElement(Element* element, Node* parent);

    /**
     * @brief 处理脱离文档流元素的移除
     *
     * 直接从渲染树移除 RenderObject，
     * 不触发 InvalidateRenderTree。
     *
     * @param element 要移除的元素
     * @return 如果成功处理返回 true
     */
    bool RemoveOutOfFlowElement(Element* element);

    /**
     * @brief 标记布局边界需要重新布局
     *
     * 只标记边界及其子树需要重新布局，
     * 不影响边界外的元素。
     *
     * @param boundary 布局边界元素
     */
    void MarkBoundaryNeedsLayout(Element* boundary);

    /**
     * @brief 标记 RenderObject 布局边界需要重新布局
     *
     * @param boundary 布局边界渲染对象
     */
    void MarkBoundaryNeedsLayout(RenderObject* boundary);

    /**
     * @brief 更新滚动容器的滚动尺寸
     *
     * @param scroll_container 滚动容器元素
     */
    void UpdateScrollContainerSize(Element* scroll_container);

    /**
     * @brief 检查是否有待处理的增量更新
     * @return 如果有待处理的更新返回 true
     */
    bool HasPendingUpdates() const;

    /**
     * @brief 清除所有待处理的更新
     */
    void ClearPendingUpdates();

    /**
     * @brief 获取需要重新布局的边界数量
     */
    size_t GetDirtyBoundaryCount() const { return dirty_boundaries_.size(); }

private:
    /// 关联的窗口
    Window* window_;

    /// 需要重新布局的边界列表（使用原始指针，因为 Element 生命周期由 DOM 管理）
    std::unordered_set<Element*> dirty_boundaries_;

    /// 需要重新布局的 RenderObject 边界列表
    std::unordered_set<RenderObject*> dirty_render_boundaries_;
};

}  // namespace mbink
