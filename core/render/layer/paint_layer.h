/**
 * @file paint_layer.h
 * @brief PaintLayer - 统一的绘制层系统
 *
 * PaintLayer 是 MBlink 统一层系统的核心类，整合了：
 * - Stacking Context 管理
 * - Z-index 排序
 * - Compositing 判断
 * - 绘制 (Paint)
 * - Hit Testing
 *
 * 设计原则：
 * - 单一 PaintLayer 类整合所有功能，减少类数量
 * - 复用现有 CompositorLayer 进行 GPU 纹理管理
 * - 删除 LayerManager，其功能由 PaintLayer 的 stacking context 机制替代
 *
 * 参考：Blink 的 PaintLayer 设计
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"

namespace mblink {

// 前向声明
class RenderObject;
class CompositorLayer;
struct HitTestResult;
enum class LayerPromotionReason;

/**
 * @brief 统一的绘制层类
 *
 * 每个需要特殊绘制处理的 RenderObject 都有一个 PaintLayer。
 * PaintLayer 负责：
 * - 管理 Stacking Context
 * - 维护 z-order 列表
 * - 决定是否需要 CompositorLayer (GPU 加速)
 * - 执行绘制和 Hit Testing
 */
class PaintLayer {
public:
    /**
     * @brief 构造函数
     * @param render_object 关联的 RenderObject
     */
    explicit PaintLayer(RenderObject* render_object);

    /**
     * @brief 析构函数
     */
    ~PaintLayer();

    static size_t GetLiveLayerCount();

    // 禁止拷贝
    PaintLayer(const PaintLayer&) = delete;
    PaintLayer& operator=(const PaintLayer&) = delete;

    // =========================================================================
    // 基本属性
    // =========================================================================

    /**
     * @brief 获取关联的 RenderObject
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief 获取父 PaintLayer
     */
    PaintLayer* Parent() const { return parent_; }

    /**
     * @brief 获取子 PaintLayer 列表
     */
    const std::vector<PaintLayer*>& Children() const { return children_; }

    // =========================================================================
    // 树操作
    // =========================================================================

    /**
     * @brief 添加子层
     * @param child 子 PaintLayer
     */
    void AddChild(PaintLayer* child);

    /**
     * @brief 移除子层
     * @param child 子 PaintLayer
     */
    void RemoveChild(PaintLayer* child);

    /**
     * @brief 在指定位置前插入子层
     * @param child 要插入的子层
     * @param before 插入位置（在此之前插入），nullptr 表示插入到末尾
     */
    void InsertBefore(PaintLayer* child, PaintLayer* before);

    /**
     * @brief 移除所有子层
     */
    void RemoveAllChildren();

    // =========================================================================
    // Stacking Context
    // =========================================================================

    /**
     * @brief 检查是否是 stacking context
     *
     * 创建 stacking context 的条件（CSS 规范）：
     * - position: absolute/relative/fixed/sticky 且 z-index != auto
     * - opacity < 1
     * - transform != none
     * - filter != none
     * - will-change: transform/opacity
     * - isolation: isolate
     *
     * @return true 如果是 stacking context
     */
    bool IsStackingContext() const;

    /**
     * @brief 获取 stacking context 祖先
     * @return 最近的 stacking context 祖先，如果没有则返回 nullptr
     */
    PaintLayer* StackingContext() const;

    /**
     * @brief 获取 z-index 值
     * @return z-index 值，auto 返回 0
     */
    int ZIndex() const;

    // =========================================================================
    // Z-Order 列表
    // =========================================================================

    /**
     * @brief 获取正 z-index 子层列表（z-index >= 0，按升序）
     */
    const std::vector<PaintLayer*>& PosZOrderList() const { return pos_z_order_list_; }

    /**
     * @brief 获取负 z-index 子层列表（z-index < 0，按升序）
     */
    const std::vector<PaintLayer*>& NegZOrderList() const { return neg_z_order_list_; }

    /**
     * @brief 更新 z-order 列表（只在 stacking context 上有效）
     */
    void UpdateZOrderLists();

    /**
     * @brief 标记 z-order 列表需要更新
     */
    void DirtyZOrderLists() { z_order_dirty_ = true; }

    /**
     * @brief 检查 z-order 列表是否需要更新
     */
    bool ZOrderListsDirty() const { return z_order_dirty_; }

    // =========================================================================
    // Compositing
    // =========================================================================

    /**
     * @brief 检查是否需要独立的 CompositorLayer
     *
     * 提升条件：
     * - will-change: transform/opacity
     * - position: fixed
     * - 活动的 transform/opacity 动画
     * - 可滚动容器
     *
     * @return true 如果需要 CompositorLayer
     */
    bool NeedsCompositing() const;

    /**
     * @brief 获取提升原因
     * @return 层提升原因
     */
    LayerPromotionReason GetPromotionReason() const;

    /**
     * @brief 确保有 CompositorLayer（按需创建）
     */
    void EnsureCompositorLayer();

    /**
     * @brief 获取关联的 CompositorLayer
     * @return CompositorLayer 指针，如果没有则返回 nullptr
     */
    CompositorLayer* GetCompositedLayer() const;

    /**
     * @brief 检查是否有独立的 CompositorLayer
     */
    bool HasCompositedLayer() const { return compositor_layer_ != nullptr; }

    // =========================================================================
    // 绘制
    // =========================================================================

    /**
     * @brief 绘制层及其子层
     *
     * 按 CSS stacking context 规则绘制：
     * 1. 背景和边框
     * 2. 负 z-index 子层
     * 3. 正常流内容
     * 4. 正 z-index 子层
     *
     * @param canvas Skia 画布
     */
    void Paint(SkCanvas* canvas);

    /**
     * @brief 绘制层自身内容（不包括子层）
     * @param canvas Skia 画布
     */
    void PaintContents(SkCanvas* canvas);

    // =========================================================================
    // Hit Testing
    // =========================================================================

    /**
     * @brief Hit Testing（按 z-order 逆序测试）
     *
     * 从高 z-index 到低 z-index 测试，先测试正 z-index 子层，
     * 再测试自身，最后测试负 z-index 子层。
     *
     * @param x 鼠标 X 坐标（视口坐标）
     * @param y 鼠标 Y 坐标（视口坐标）
     * @param result 输出结果
     * @return true 如果命中了元素
     */
    bool HitTest(float x, float y, HitTestResult& result);

    // =========================================================================
    // 滚动支持
    // =========================================================================

    /**
     * @brief 处理滚轮事件
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param delta_x 水平滚动量
     * @param delta_y 垂直滚动量
     * @return true 如果事件被处理
     */
    bool HandleWheel(float x, float y, float delta_x, float delta_y);

    // =========================================================================
    // Position: Fixed 支持
    // =========================================================================

    /**
     * @brief 检查是否是 fixed 定位
     */
    bool IsFixedPositioned() const;

    /**
     * @brief 获取用于 hit testing 的绝对坐标
     *
     * 对于 fixed 元素：直接使用 layout.x/y（视口坐标）
     * 对于其他元素：累加父元素偏移
     *
     * @param abs_x 输出绝对 X 坐标
     * @param abs_y 输出绝对 Y 坐标
     */
    void GetAbsolutePosition(float& abs_x, float& abs_y) const;

    // =========================================================================
    // 调试
    // =========================================================================

    /**
     * @brief 转换为调试字符串
     * @return 包含层信息的字符串
     */
    std::string ToDebugString() const;

    /**
     * @brief 打印层树结构
     * @param indent 缩进级别
     */
    void DumpTree(int indent = 0) const;

private:
    // 关联的 RenderObject
    RenderObject* render_object_;

    // 树结构
    PaintLayer* parent_ = nullptr;
    std::vector<PaintLayer*> children_;

    // Z-order 列表（只有 stacking context 使用）
    std::vector<PaintLayer*> pos_z_order_list_;  // z-index >= 0
    std::vector<PaintLayer*> neg_z_order_list_;  // z-index < 0
    bool z_order_dirty_ = true;

    // Compositing
    std::shared_ptr<CompositorLayer> compositor_layer_;
    mutable LayerPromotionReason promotion_reason_;

    // 缓存
    mutable PaintLayer* cached_stacking_context_ = nullptr;
    mutable bool stacking_context_dirty_ = true;

    // =========================================================================
    // 私有方法
    // =========================================================================

    /**
     * @brief 收集需要参与 z-order 排序的子层
     */
    void CollectZOrderLayers();

    /**
     * @brief 对 z-order 列表排序
     */
    void SortZOrderLists();

    /**
     * @brief 绘制负 z-index 子层
     */
    void PaintNegativeZOrderChildren(SkCanvas* canvas);

    /**
     * @brief 绘制正 z-index 子层
     */
    void PaintPositiveZOrderChildren(SkCanvas* canvas);

    /**
     * @brief 递归 Hit Test 子层
     */
    bool HitTestChildren(float x, float y, HitTestResult& result);

    /**
     * @brief 递归 Hit Test 单个 RenderObject
     */
    bool HitTestRenderObject(
        RenderObject* render_obj,
        float x, float y,
        float offset_x, float offset_y,
        HitTestResult& result,
        bool is_root = false);
};

} // namespace mblink
