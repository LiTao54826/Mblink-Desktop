/**
 * @file dirty_region_collector.cpp
 * @brief 脏区域收集器实现
 */

#include "dirty_region_collector.h"
#include "core/dom/element.h"
#include "core/render/render_object.h"

namespace lightui {

bool DirtyRegionCollector::CollectFromDOM(Node* root, DirtyRegion& dirty_region) {
    if (!root) {
        return false;
    }

    // 清空之前的脏区域
    dirty_region.Clear();

    // 递归收集脏节点
    bool has_dirty = CollectDirtyNodes(root, dirty_region, false);

    // 优化脏区域（合并相邻区域）
    if (has_dirty) {
        dirty_region.Optimize();
    }

    return has_dirty;
}

bool DirtyRegionCollector::CollectDirtyNodes(Node* node, DirtyRegion& dirty_region, bool parent_dirty) {
    if (!node || !ShouldCollectNode(node)) {
        return false;
    }

    bool has_dirty = false;

    // 检查当前节点是否脏
    bool node_dirty = node->IsDirty();
    bool is_layout_dirty = node->IsLayoutDirty();
    bool is_paint_dirty = node->IsPaintDirty();

    // 如果父节点脏，子节点也需要重绘
    bool should_collect = node_dirty || parent_dirty;

    if (should_collect) {
        // 计算节点的屏幕空间边界
        SkRect bounds = ComputeNodeBounds(node);

        // 如果节点有有效边界，添加到脏区域
        if (!bounds.isEmpty()) {
            // 如果节点有自己的脏矩形，使用它；否则使用计算的边界
            SkRect dirty_rect = node->GetDirtyRect();
            if (!dirty_rect.isEmpty()) {
                dirty_region.AddRect(dirty_rect);
            } else {
                dirty_region.AddRect(bounds);
            }
            has_dirty = true;
        }
    }

    // 递归处理子节点
    // 注意：如果当前节点布局脏，所有子节点都需要重新布局和绘制
    bool propagate_dirty = parent_dirty || is_layout_dirty;

    auto child = node->GetFirstChild();
    while (child) {
        if (CollectDirtyNodes(child.get(), dirty_region, propagate_dirty)) {
            has_dirty = true;
        }
        child = child->GetNextSibling();
    }

    return has_dirty;
}

bool DirtyRegionCollector::ShouldCollectNode(Node* node) const {
    if (!node) {
        return false;
    }

    // 文本节点不单独收集（由父元素处理）
    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        return false;
    }

    // 元素节点需要收集
    return node->GetNodeType() == NodeType::ELEMENT_NODE;
}

SkRect DirtyRegionCollector::ComputeNodeBounds(Node* node) const {
    if (!node) {
        return SkRect::MakeEmpty();
    }

    // 尝试从节点获取布局信息
    float x = 0, y = 0, width = 0, height = 0;
    if (!GetNodeLayout(node, x, y, width, height)) {
        return SkRect::MakeEmpty();
    }

    // 创建屏幕空间矩形
    SkRect bounds = SkRect::MakeXYWH(x, y, width, height);

    // 裁剪到视口范围
    if (viewport_width_ > 0 && viewport_height_ > 0) {
        SkRect viewport = SkRect::MakeWH(viewport_width_, viewport_height_);
        if (!bounds.intersect(viewport)) {
            // 完全在视口外，返回空矩形
            return SkRect::MakeEmpty();
        }
    }

    return bounds;
}

bool DirtyRegionCollector::GetNodeLayout(Node* node, float& x, float& y, float& width, float& height) const {
    if (!node) {
        return false;
    }

    // 仅对 Element 节点尝试获取布局
    if (node->GetNodeType() != NodeType::ELEMENT_NODE) {
        return false;
    }

    // 从 RenderObject 获取真实的布局信息
    auto render_obj = node->GetRenderObject();
    if (!render_obj) {
        // 节点没有关联的 RenderObject（可能还未构建渲染树）
        return false;
    }

    // 使用 GetBoundingRect() 获取绝对坐标边界框
    // 该方法已考虑父子偏移和滚动
    SkRect bounds = render_obj->GetBoundingRect();
    if (bounds.isEmpty()) {
        // 节点没有有效的布局信息
        return false;
    }

    x      = bounds.left();
    y      = bounds.top();
    width  = bounds.width();
    height = bounds.height();
    return true;
}

void DirtyRegionCollector::SetViewportSize(float width, float height) {
    viewport_width_ = width;
    viewport_height_ = height;
}

} // namespace lightui

