/**
 * @file scroll_layer_manager.h
 * @brief 滚动层管理器 - 管理滚动容器和固定元素的层
 *
 * ScrollLayerManager 负责：
 * - 检测可滚动容器，创建滚动内容层
 * - 实现无需重新光栅化的滚动（只更新层偏移）
 * - 处理 position: fixed 元素（滚动时保持静止）
 * - 支持属性树系统的直接滚动偏移更新
 *
 * 设计原则：
 * - 滚动时只更新层偏移，不重新光栅化
 * - 固定元素在独立层，不受滚动影响
 * - 支持嵌套滚动容器
 */

#pragma once

#include "compositor_layer.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mbink {

// 前向声明
class RenderObject;
class LayerTreeBuilder;
class Rasterizer;
class PaintArtifactCompositor;
class PropertyTrees;

/**
 * @brief 滚动容器信息
 */
struct ScrollContainerInfo {
    RenderObject* container = nullptr;           // 滚动容器
    std::shared_ptr<CompositorLayer> content_layer;  // 滚动内容层
    std::shared_ptr<CompositorLayer> fixed_layer;    // 固定元素层（可选）
    
    // 滚动状态
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    float max_scroll_x = 0.0f;
    float max_scroll_y = 0.0f;
    
    // 内容尺寸
    float content_width = 0.0f;
    float content_height = 0.0f;
    
    // 视口尺寸
    float viewport_width = 0.0f;
    float viewport_height = 0.0f;
    
    // 上次布局宽度（用于检测宽度变化）
    float last_layout_width = 0.0f;
};

/**
 * @brief 固定元素信息
 */
struct FixedElementInfo {
    RenderObject* element = nullptr;
    std::shared_ptr<CompositorLayer> layer;
    
    // 固定位置（相对于视口）
    float fixed_x = 0.0f;
    float fixed_y = 0.0f;
};

enum class ScrollInvalidationReason {
    None,
    ClipLayerFullDirty,
    AncestorLayerFullDirty,
    MissingLayerTarget
};

struct ScrollInvalidationStats {
    std::uint64_t scrolls_handled = 0;
    std::uint64_t full_dirty_scrolls = 0;
    std::uint64_t clip_layer_full_dirty_scrolls = 0;
    std::uint64_t ancestor_layer_full_dirty_scrolls = 0;
    std::uint64_t missing_layer_target_scrolls = 0;
    ScrollInvalidationReason last_reason = ScrollInvalidationReason::None;

    void Reset() {
        scrolls_handled = 0;
        full_dirty_scrolls = 0;
        clip_layer_full_dirty_scrolls = 0;
        ancestor_layer_full_dirty_scrolls = 0;
        missing_layer_target_scrolls = 0;
        last_reason = ScrollInvalidationReason::None;
    }
};

/**
 * @brief 滚动层管理器
 *
 * 管理滚动容器和固定元素的合成层。
 */
class ScrollLayerManager {
public:
    ScrollLayerManager();
    ~ScrollLayerManager();

    // 禁止拷贝
    ScrollLayerManager(const ScrollLayerManager&) = delete;
    ScrollLayerManager& operator=(const ScrollLayerManager&) = delete;

    /**
     * @brief 设置层树构建器
     */
    void SetLayerTreeBuilder(LayerTreeBuilder* builder) { layer_tree_builder_ = builder; }

    /**
     * @brief 设置光栅化器
     */
    void SetRasterizer(Rasterizer* rasterizer) { rasterizer_ = rasterizer; }

    /**
     * @brief 设置绘制产物合成器（属性树系统）
     * @param compositor 绘制产物合成器指针（不拥有所有权）
     */
    void SetPaintArtifactCompositor(PaintArtifactCompositor* compositor) {
        paint_artifact_compositor_ = compositor;
    }

    /**
     * @brief 设置属性树集合
     * @param trees 属性树集合指针（不拥有所有权）
     */
    void SetPropertyTrees(PropertyTrees* trees) {
        property_trees_ = trees;
    }

    /**
     * @brief 检查是否使用属性树系统
     */
    bool IsUsingPropertyTreeSystem() const {
        return paint_artifact_compositor_ != nullptr && property_trees_ != nullptr;
    }

    // =========================================================================
    // 滚动容器管理
    // =========================================================================

    /**
     * @brief 注册滚动容器
     * @param container 滚动容器的渲染对象
     * @return true 如果注册成功
     */
    bool RegisterScrollContainer(RenderObject* container);

    /**
     * @brief 注销滚动容器
     * @param container 滚动容器
     */
    void UnregisterScrollContainer(RenderObject* container);

    /**
     * @brief 检查是否是已注册的滚动容器
     */
    bool IsScrollContainer(RenderObject* obj) const;

    /**
     * @brief 获取滚动容器信息
     */
    ScrollContainerInfo* GetScrollContainerInfo(RenderObject* container);
    const ScrollContainerInfo* GetScrollContainerInfo(RenderObject* container) const;

    /**
     * @brief 获取所有滚动容器
     */
    const std::unordered_map<RenderObject*, ScrollContainerInfo>& GetScrollContainers() const {
        return scroll_containers_;
    }

    // =========================================================================
    // 滚动处理
    // =========================================================================

    /**
     * @brief 处理滚动事件（无需重新光栅化）
     * @param container 滚动容器
     * @param delta_x X 方向滚动增量
     * @param delta_y Y 方向滚动增量
     * @return true 如果滚动成功
     */
    bool HandleScroll(RenderObject* container, float delta_x, float delta_y);

    /**
     * @brief 滚动到指定位置
     * @param container 滚动容器
     * @param scroll_x 目标 X 滚动位置
     * @param scroll_y 目标 Y 滚动位置
     * @return true 如果滚动成功
     */
    bool ScrollTo(RenderObject* container, float scroll_x, float scroll_y);

    const ScrollInvalidationStats& GetInvalidationStats() const { return invalidation_stats_; }
    void ResetInvalidationStats() { invalidation_stats_.Reset(); }

    /**
     * @brief 更新滚动容器的内容尺寸
     * @param container 滚动容器
     */
    void UpdateContentSize(RenderObject* container);

    /**
     * @brief 检查滚动后是否需要光栅化新区域
     * @param container 滚动容器
     * @param old_scroll_x 旧滚动位置 X
     * @param old_scroll_y 旧滚动位置 Y
     * @return true 如果需要光栅化
     */
    bool NeedsRasterizeAfterScroll(RenderObject* container,
                                    float old_scroll_x, float old_scroll_y);

    // =========================================================================
    // 固定元素管理
    // =========================================================================

    /**
     * @brief 注册固定元素
     * @param element 固定元素的渲染对象
     * @return true 如果注册成功
     */
    bool RegisterFixedElement(RenderObject* element);

    /**
     * @brief 注销固定元素
     * @param element 固定元素
     */
    void UnregisterFixedElement(RenderObject* element);

    /**
     * @brief 检查是否是已注册的固定元素
     */
    bool IsFixedElement(RenderObject* obj) const;

    /**
     * @brief 获取固定元素信息
     */
    FixedElementInfo* GetFixedElementInfo(RenderObject* element);
    const FixedElementInfo* GetFixedElementInfo(RenderObject* element) const;

    /**
     * @brief 获取所有固定元素
     */
    const std::unordered_map<RenderObject*, FixedElementInfo>& GetFixedElements() const {
        return fixed_elements_;
    }

    /**
     * @brief 更新固定元素位置（滚动后调用）
     * @note 固定元素的层位置不随滚动改变
     */
    void UpdateFixedElementPositions();

    // =========================================================================
    // 层创建
    // =========================================================================

    /**
     * @brief 为滚动容器创建内容层
     * @param container 滚动容器
     * @return 创建的内容层
     */
    std::shared_ptr<CompositorLayer> CreateScrollContentLayer(RenderObject* container);

    /**
     * @brief 为固定元素创建层
     * @param element 固定元素
     * @return 创建的层
     */
    std::shared_ptr<CompositorLayer> CreateFixedElementLayer(RenderObject* element);

    // =========================================================================
    // 查询
    // =========================================================================

    /**
     * @brief 获取滚动容器数量
     */
    size_t GetScrollContainerCount() const { return scroll_containers_.size(); }

    /**
     * @brief 获取固定元素数量
     */
    size_t GetFixedElementCount() const { return fixed_elements_.size(); }

    /**
     * @brief 查找包含指定点的滚动容器
     * @param x 屏幕 X 坐标
     * @param y 屏幕 Y 坐标
     * @return 滚动容器，如果没有则返回 nullptr
     */
    RenderObject* FindScrollContainerAt(float x, float y) const;

    // =========================================================================
    // 清理
    // =========================================================================

    /**
     * @brief 清除所有状态
     */
    void Clear();

private:
    /**
     * @brief 检查渲染对象是否可滚动
     */
    bool IsScrollable(RenderObject* obj) const;

    /**
     * @brief 检查渲染对象是否是 position: fixed
     */
    bool IsPositionFixed(RenderObject* obj) const;

    /**
     * @brief 计算滚动范围
     */
    void CalculateScrollBounds(ScrollContainerInfo& info);

    /**
     * @brief 限制滚动位置在有效范围内
     */
    void ClampScrollPosition(ScrollContainerInfo& info);

    // 关联的组件
    LayerTreeBuilder* layer_tree_builder_ = nullptr;
    Rasterizer* rasterizer_ = nullptr;
    
    // 属性树系统组件
    PaintArtifactCompositor* paint_artifact_compositor_ = nullptr;
    PropertyTrees* property_trees_ = nullptr;

    // 滚动容器映射
    std::unordered_map<RenderObject*, ScrollContainerInfo> scroll_containers_;

    // 固定元素映射
    std::unordered_map<RenderObject*, FixedElementInfo> fixed_elements_;
    ScrollInvalidationStats invalidation_stats_;

    // 层 ID 计数器
    static uint32_t next_layer_id_;
};

} // namespace mbink
