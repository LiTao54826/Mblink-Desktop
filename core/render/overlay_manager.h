/**
 * @file overlay_manager.h
 * @brief Overlay 管理器 - 处理高 z-index positioned 元素的绘制
 * 
 * 解决 CSS stacking context 问题：让高 z-index 的 positioned 元素
 * 能够覆盖其他元素，即使它们不是同级的。
 */

#pragma once

#include "render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include <vector>
#include <memory>

namespace lightui {

/**
 * @brief Overlay 元素信息
 */
struct OverlayItem {
    std::shared_ptr<RenderObject> render_obj;  // 渲染对象
    SkMatrix transform;                         // 累积的变换矩阵
    int z_index;                                // z-index 值
};

/**
 * @brief Overlay 管理器（单例）
 * 
 * 在 Paint 过程中收集高 z-index 的 positioned 元素，
 * 然后在所有普通元素绘制完成后统一绘制这些 overlay 元素。
 */
class OverlayManager {
public:
    /**
     * @brief 获取单例实例
     */
    static OverlayManager& Instance();

    /**
     * @brief 开始新的渲染帧
     * 清除上一帧收集的 overlay 元素
     */
    void BeginFrame();

    /**
     * @brief 添加 overlay 元素
     * @param render_obj 渲染对象
     * @param transform 当前的变换矩阵
     * @param z_index z-index 值
     */
    void AddOverlay(std::shared_ptr<RenderObject> render_obj, const SkMatrix& transform, int z_index);

    /**
     * @brief 检查是否应该延迟绘制该元素
     * @param render_obj 渲染对象
     * @return 如果应该延迟绘制返回 true
     */
    bool ShouldDeferPaint(const RenderObject* render_obj) const;

    /**
     * @brief 绘制所有 overlay 元素
     * @param canvas 画布
     */
    void PaintOverlays(SkCanvas* canvas);

    /**
     * @brief 检查是否有 overlay 元素
     */
    bool HasOverlays() const { return !overlays_.empty(); }

    /**
     * @brief 设置 z-index 阈值
     * z-index >= 阈值的 positioned 元素会被收集为 overlay
     */
    void SetZIndexThreshold(int threshold) { z_index_threshold_ = threshold; }

    /**
     * @brief 检查是否正在绘制 overlay
     */
    bool IsPaintingOverlay() const { return painting_overlay_; }

private:
    OverlayManager() = default;
    ~OverlayManager() = default;
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;

    std::vector<OverlayItem> overlays_;
    int z_index_threshold_ = 100;  // 阈值：z-index >= 100 的 positioned 元素作为 overlay
    bool painting_overlay_ = false;  // 是否正在绘制 overlay
};

} // namespace lightui
