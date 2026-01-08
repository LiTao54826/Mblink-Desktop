/**
 * @file layer_tree_manager.h
 * @brief 层树管理器 - 协调增量层树更新
 *
 * LayerTreeManager 是增量层树更新系统的核心组件，负责：
 * - 管理增量更新队列
 * - 维护滚动状态的单一数据源（SSOT）
 * - 提供统一的坐标转换
 * - 协调 LayerTreeBuilder、Rasterizer、Compositor
 */

#pragma once

#include "layer_tree_types.h"
#include "animation/animation_bounds_calculator.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace lightui {

// 前向声明
class LayerTreeBuilder;
class Rasterizer;
class Compositor;
class CompositorLayer;

/**
 * @brief 层树管理器
 *
 * 协调层树更新的中央管理器，整合增量更新、滚动状态和坐标系统。
 */
class LayerTreeManager {
public:
    LayerTreeManager();
    ~LayerTreeManager();

    // 禁止拷贝
    LayerTreeManager(const LayerTreeManager&) = delete;
    LayerTreeManager& operator=(const LayerTreeManager&) = delete;

    // =========================================================================
    // 初始化和配置
    // =========================================================================

    /**
     * @brief 初始化管理器
     * @param builder 层树构建器
     * @param rasterizer 光栅化器
     * @param compositor 合成器
     */
    void Initialize(LayerTreeBuilder* builder,
                    Rasterizer* rasterizer,
                    Compositor* compositor);

    /**
     * @brief 设置视口信息
     * @param width 视口宽度
     * @param height 视口高度
     * @param dpi_scale DPI 缩放比
     */
    void SetViewport(float width, float height, float dpi_scale);

    /**
     * @brief 获取视口宽度
     */
    float GetViewportWidth() const { return viewport_width_; }

    /**
     * @brief 获取视口高度
     */
    float GetViewportHeight() const { return viewport_height_; }

    /**
     * @brief 获取 DPI 缩放比
     */
    float GetDpiScale() const { return dpi_scale_; }

    // =========================================================================
    // 增量更新接口
    // =========================================================================

    /**
     * @brief 请求添加层（增量）
     * @param obj 需要层的 RenderObject
     * @param reason 层提升原因
     */
    void RequestAddLayer(RenderObject* obj, LayerPromotionReason reason);

    /**
     * @brief 请求移除层（增量）
     * @param obj 要移除层的 RenderObject
     */
    void RequestRemoveLayer(RenderObject* obj);

    /**
     * @brief 请求更新层边界
     * @param obj 需要更新边界的 RenderObject
     */
    void RequestUpdateBounds(RenderObject* obj);

    /**
     * @brief 请求重新附加父层
     * @param obj 需要重新附加的 RenderObject
     */
    void RequestReparent(RenderObject* obj);

    /**
     * @brief 请求更新 z-index
     * @param obj 需要更新 z-index 的 RenderObject
     * @param z_index 新的 z-index 值
     */
    void RequestUpdateZIndex(RenderObject* obj, int z_index);

    /**
     * @brief 应用所有待处理的更新
     * @return 是否有更新被应用
     */
    bool ApplyPendingUpdates();

    /**
     * @brief 检查是否有待处理的更新
     */
    bool HasPendingUpdates() const { return !pending_updates_.empty(); }

    /**
     * @brief 获取待处理更新数量
     */
    size_t GetPendingUpdateCount() const { return pending_updates_.size(); }

    /**
     * @brief 检查是否需要完整重建
     */
    bool NeedsFullRebuild() const { return needs_full_rebuild_; }

    /**
     * @brief 强制完整重建（仅在必要时使用）
     * @param reason 重建原因（用于调试）
     */
    void ForceFullRebuild(const std::string& reason);

    /**
     * @brief 清除完整重建标记
     */
    void ClearFullRebuildFlag() { needs_full_rebuild_ = false; }

    // =========================================================================
    // 滚动状态管理（SSOT）
    // =========================================================================

    /**
     * @brief 注册滚动容器
     * @param container 滚动容器的 RenderObject
     * @return 是否成功注册
     */
    bool RegisterScrollContainer(RenderObject* container);

    /**
     * @brief 注销滚动容器
     * @param container 滚动容器
     */
    void UnregisterScrollContainer(RenderObject* container);

    /**
     * @brief 检查是否是已注册的滚动容器
     * @param container 要检查的 RenderObject
     */
    bool IsScrollContainer(RenderObject* container) const;

    /**
     * @brief 获取滚动状态（只读）
     * @param container 滚动容器
     * @return 滚动状态指针，如果未注册返回 nullptr
     */
    const ScrollState* GetScrollState(RenderObject* container) const;

    /**
     * @brief 设置滚动位置（唯一的写入入口）
     * @param container 滚动容器
     * @param x X 滚动位置
     * @param y Y 滚动位置
     * @return 是否成功设置
     */
    bool SetScrollPosition(RenderObject* container, float x, float y);

    /**
     * @brief 滚动指定距离
     * @param container 滚动容器
     * @param dx X 方向增量
     * @param dy Y 方向增量
     * @return 是否成功滚动
     */
    bool ScrollBy(RenderObject* container, float dx, float dy);

    /**
     * @brief 更新滚动容器的内容尺寸
     * @param container 滚动容器
     */
    void UpdateScrollContentSize(RenderObject* container);

    /**
     * @brief 添加滚动变化监听器
     * @param listener 监听器回调
     * @return 监听器 ID
     */
    uint32_t AddScrollListener(ScrollListener listener);

    /**
     * @brief 移除滚动变化监听器
     * @param id 监听器 ID
     */
    void RemoveScrollListener(uint32_t id);

    /**
     * @brief 获取滚动容器数量
     */
    size_t GetScrollContainerCount() const { return scroll_states_.size(); }

    // =========================================================================
    // 坐标转换
    // =========================================================================

    /**
     * @brief 转换点坐标
     * @param point 输入点
     * @param from 源坐标系
     * @param to 目标坐标系
     * @param layer 相关层（用于 Layer 坐标系）
     * @return 转换后的点
     */
    SkPoint ConvertPoint(const SkPoint& point,
                         CoordinateSpace from,
                         CoordinateSpace to,
                         CompositorLayer* layer = nullptr) const;

    /**
     * @brief 转换矩形坐标
     * @param rect 输入矩形
     * @param from 源坐标系
     * @param to 目标坐标系
     * @param layer 相关层（用于 Layer 坐标系）
     * @return 转换后的矩形
     */
    SkRect ConvertRect(const SkRect& rect,
                       CoordinateSpace from,
                       CoordinateSpace to,
                       CompositorLayer* layer = nullptr) const;

    /**
     * @brief 计算层在文档坐标系中的边界
     * @param obj RenderObject
     * @param parent_layer 父层
     * @return 文档坐标系中的边界
     */
    SkRect CalculateLayerBoundsInDocument(RenderObject* obj,
                                          CompositorLayer* parent_layer) const;

    /**
     * @brief 计算 fixed 元素在视口坐标系中的边界
     * @param obj RenderObject
     * @return 视口坐标系中的边界
     */
    SkRect CalculateFixedLayerBounds(RenderObject* obj) const;

    // =========================================================================
    // 文档滚动
    // =========================================================================

    /**
     * @brief 设置文档滚动位置
     * @param x X 滚动位置
     * @param y Y 滚动位置
     */
    void SetDocumentScroll(float x, float y);

    /**
     * @brief 获取文档 X 滚动位置
     */
    float GetDocumentScrollX() const { return document_scroll_x_; }

    /**
     * @brief 获取文档 Y 滚动位置
     */
    float GetDocumentScrollY() const { return document_scroll_y_; }

    // =========================================================================
    // 层树版本和统计
    // =========================================================================

    /**
     * @brief 获取层树版本号
     */
    uint64_t GetTreeVersion() const { return tree_version_; }

    /**
     * @brief 递增层树版本号
     */
    void IncrementTreeVersion() { ++tree_version_; }

    /**
     * @brief 获取层数量
     */
    size_t GetLayerCount() const;

    // =========================================================================
    // 调试支持
    // =========================================================================

    /**
     * @brief 启用/禁用调试日志
     * @param enabled 是否启用
     */
    void SetDebugLogging(bool enabled) { debug_logging_ = enabled; }

    /**
     * @brief 检查调试日志是否启用
     */
    bool IsDebugLoggingEnabled() const { return debug_logging_; }

    /**
     * @brief 获取最后一次完整重建的原因
     */
    const std::string& GetLastRebuildReason() const { return last_rebuild_reason_; }

    // =========================================================================
    // 脏标记优化
    // =========================================================================

    /**
     * @brief 标记层内容变化（只影响指定层）
     * @param obj 内容变化的 RenderObject
     */
    void MarkContentDirty(RenderObject* obj);

    /**
     * @brief 标记 transform 变化（不触发光栅化）
     * @param obj transform 变化的 RenderObject
     */
    void MarkTransformDirty(RenderObject* obj);

    /**
     * @brief 标记 opacity 变化（不触发光栅化）
     * @param obj opacity 变化的 RenderObject
     */
    void MarkOpacityDirty(RenderObject* obj);

    /**
     * @brief 标记边界变化
     * @param obj 边界变化的 RenderObject
     */
    void MarkBoundsDirty(RenderObject* obj);

    // =========================================================================
    // 动画状态保持
    // =========================================================================

    /**
     * @brief 保存层的动画状态
     * @param layer 要保存状态的层
     * @return 动画状态数据
     */
    struct AnimationStateData {
        SkMatrix transform;
        float opacity;
        bool has_animation_bounds;
        AnimationBounds animation_bounds;
    };
    
    AnimationStateData SaveAnimationState(CompositorLayer* layer) const;

    /**
     * @brief 恢复层的动画状态
     * @param layer 要恢复状态的层
     * @param state 动画状态数据
     */
    void RestoreAnimationState(CompositorLayer* layer, const AnimationStateData& state);

    /**
     * @brief 转移动画状态到新层
     * @param old_layer 旧层
     * @param new_layer 新层
     */
    void TransferAnimationState(CompositorLayer* old_layer, CompositorLayer* new_layer);

    // =========================================================================
    // 调试支持
    // =========================================================================

    /**
     * @brief 层检查信息
     */
    struct LayerInspectionInfo {
        uint64_t layer_identity;
        uint32_t layer_id;
        std::string debug_name;
        LayerPromotionReason promotion_reason;
        CoordinateSpace coordinate_space;
        SkRect bounds;
        int z_index;
        int tree_depth;
        bool has_scroll_offset;
        float scroll_x;
        float scroll_y;
        uint64_t parent_identity;
        size_t children_count;
    };

    /**
     * @brief 获取层检查信息
     * @param layer 要检查的层
     * @return 层检查信息
     */
    LayerInspectionInfo InspectLayer(CompositorLayer* layer) const;

    /**
     * @brief 获取所有层的检查信息
     * @return 所有层的检查信息列表
     */
    std::vector<LayerInspectionInfo> InspectAllLayers() const;

    /**
     * @brief 打印层树结构（调试用）
     */
    void DumpLayerTree() const;

private:
    // =========================================================================
    // 私有方法
    // =========================================================================

    /**
     * @brief 应用单个更新操作
     * @param update 更新操作
     * @return 是否成功应用
     */
    bool ApplyUpdate(const PendingLayerUpdate& update);

    /**
     * @brief 通知滚动监听器
     * @param container 滚动容器
     * @param state 滚动状态
     */
    void NotifyScrollListeners(RenderObject* container, const ScrollState& state);

    /**
     * @brief 计算滚动范围
     * @param state 滚动状态（输出）
     * @param container 滚动容器
     */
    void CalculateScrollBounds(ScrollState& state, RenderObject* container);

    /**
     * @brief 限制滚动位置在有效范围内
     * @param state 滚动状态
     */
    void ClampScrollPosition(ScrollState& state);

    /**
     * @brief 记录调试日志
     * @param message 日志消息
     */
    void LogDebug(const std::string& message) const;

    // =========================================================================
    // 成员变量
    // =========================================================================

    // 关联组件
    LayerTreeBuilder* builder_ = nullptr;
    Rasterizer* rasterizer_ = nullptr;
    Compositor* compositor_ = nullptr;

    // 待处理的更新
    std::vector<PendingLayerUpdate> pending_updates_;

    // 滚动状态（SSOT）
    std::unordered_map<RenderObject*, ScrollState> scroll_states_;

    // 滚动监听器
    std::unordered_map<uint32_t, ScrollListener> scroll_listeners_;
    uint32_t next_listener_id_ = 1;

    // 视口信息
    float viewport_width_ = 0.0f;
    float viewport_height_ = 0.0f;
    float dpi_scale_ = 1.0f;

    // 文档滚动
    float document_scroll_x_ = 0.0f;
    float document_scroll_y_ = 0.0f;

    // 状态
    bool needs_full_rebuild_ = true;
    std::string last_rebuild_reason_ = "initial";
    uint64_t tree_version_ = 0;

    // 调试
    bool debug_logging_ = false;
};

} // namespace lightui
