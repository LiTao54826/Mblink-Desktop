/**
 * @file dirty_region_collector.h
 * @brief 脏区域收集器
 *
 * 功能：
 * - 从DOM树收集脏节点
 * - 计算节点的屏幕空间边界
 * - 生成优化的脏区域列表
 * - 支持增量渲染
 */

#pragma once

#include "dirty_region.h"
#include "core/dom/node.h"
#include "include/core/SkRect.h"
#include <memory>

namespace lightui {

/**
 * @brief 脏区域收集器
 *
 * 遍历DOM树，收集所有脏节点的屏幕空间边界，
 * 生成优化的脏区域列表用于增量渲染
 */
class DirtyRegionCollector {
public:
    /**
     * @brief 构造函数
     */
    DirtyRegionCollector() = default;

    /**
     * @brief 从DOM树收集脏区域
     * @param root 根节点
     * @param dirty_region 输出的脏区域
     * @return 是否有脏区域
     */
    bool CollectFromDOM(Node* root, DirtyRegion& dirty_region);

    /**
     * @brief 计算节点的屏幕空间边界
     * @param node 节点
     * @return 屏幕空间矩形，如果节点不可见则返回空矩形
     */
    SkRect ComputeNodeBounds(Node* node) const;

    /**
     * @brief 设置视口尺寸（用于边界计算）
     * @param width 视口宽度
     * @param height 视口高度
     */
    void SetViewportSize(float width, float height);

private:
    /**
     * @brief 递归收集脏节点
     * @param node 当前节点
     * @param dirty_region 脏区域累积器
     * @param parent_dirty 父节点是否脏
     * @return 是否有脏节点
     */
    bool CollectDirtyNodes(Node* node, DirtyRegion& dirty_region, bool parent_dirty);

    /**
     * @brief 检查节点是否需要收集
     * @param node 节点
     * @return 是否需要收集
     */
    bool ShouldCollectNode(Node* node) const;

    /**
     * @brief 获取节点的布局信息
     * @param node 节点
     * @param x 输出X坐标
     * @param y 输出Y坐标
     * @param width 输出宽度
     * @param height 输出高度
     * @return 是否成功获取布局信息
     */
    bool GetNodeLayout(Node* node, float& x, float& y, float& width, float& height) const;

    float viewport_width_ = 0.0f;   ///< 视口宽度
    float viewport_height_ = 0.0f;  ///< 视口高度
};

} // namespace lightui

