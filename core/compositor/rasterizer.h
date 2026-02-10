/**
 * @file rasterizer.h
 * @brief 光栅化器 - 将渲染对象绘制到合成层的 CPU 位图
 *
 * Rasterizer 负责：
 * - 完整层光栅化（首次绘制或大范围变化）
 * - 增量光栅化（只重绘脏区域）
 * - 滚动感知光栅化（滚动时只绘制新可见区域）
 *
 * 设计原则：
 * - CPU 光栅化，GPU 合成（业界标准架构）
 * - 最小化重绘区域
 * - 保持像素精确性
 */

#pragma once

#include "compositor_layer.h"
#include "core/render/objects/render_object.h"
#include <memory>
#include <unordered_map>
#include <vector>

// 前向声明 Skia 类
class SkCanvas;
class SkRegion;

namespace lightui {

/**
 * @brief 光栅化统计信息
 */
struct RasterizeStats {
    int layers_rasterized = 0;      // 光栅化的层数
    int full_rasterizations = 0;    // 完整光栅化次数
    int incremental_rasterizations = 0;  // 增量光栅化次数
    int pixels_rasterized = 0;      // 光栅化的像素数
    int pixels_skipped = 0;         // 跳过的像素数（增量优化）
    double rasterize_time_ms = 0.0; // 光栅化耗时（毫秒）

    void Reset() {
        layers_rasterized = 0;
        full_rasterizations = 0;
        incremental_rasterizations = 0;
        pixels_rasterized = 0;
        pixels_skipped = 0;
        rasterize_time_ms = 0.0;
    }
};

/**
 * @brief 光栅化器类
 *
 * 将渲染对象绘制到合成层的 CPU 位图。
 * 支持完整光栅化和增量光栅化。
 */
class Rasterizer {
public:
    Rasterizer();
    ~Rasterizer();

    // 禁止拷贝
    Rasterizer(const Rasterizer&) = delete;
    Rasterizer& operator=(const Rasterizer&) = delete;

    // =========================================================================
    // 完整光栅化
    // =========================================================================

    /**
     * @brief 光栅化整个层
     * @param layer 目标合成层
     * @return true 如果光栅化成功
     */
    bool RasterizeLayer(CompositorLayer* layer);

    /**
     * @brief 光栅化层树中的所有脏层
     * @param root 层树根节点
     * @return 光栅化的层数
     */
    int RasterizeDirtyLayers(CompositorLayer* root);

    // =========================================================================
    // 增量光栅化
    // =========================================================================

    /**
     * @brief 只光栅化层的脏区域
     * @param layer 目标合成层
     * @return true 如果光栅化成功
     */
    bool RasterizeDirtyRegions(CompositorLayer* layer);

    /**
     * @brief 光栅化指定区域
     * @param layer 目标合成层
     * @param region 要光栅化的区域（层坐标系）
     * @return true 如果光栅化成功
     */
    bool RasterizeRegion(CompositorLayer* layer, const SkIRect& region);

    // =========================================================================
    // 滚动优化
    // =========================================================================

    /**
     * @brief 处理滚动，只光栅化新可见区域
     * @param layer 滚动层
     * @param old_offset 旧滚动偏移
     * @param new_offset 新滚动偏移
     * @return true 如果处理成功
     */
    bool HandleScroll(CompositorLayer* layer,
                      const SkPoint& old_offset,
                      const SkPoint& new_offset);

    /**
     * @brief 计算滚动后需要光栅化的新区域
     * @param layer 滚动层
     * @param scroll_delta 滚动增量
     * @param[out] new_regions 需要光栅化的新区域
     */
    void CalculateScrollDirtyRegions(CompositorLayer* layer,
                                     const SkPoint& scroll_delta,
                                     std::vector<SkIRect>& new_regions);

    // =========================================================================
    // 统计和调试
    // =========================================================================

    /**
     * @brief 获取光栅化统计信息
     */
    const RasterizeStats& GetStats() const { return stats_; }

    /**
     * @brief 重置统计信息
     */
    void ResetStats() { stats_.Reset(); }

    /**
     * @brief 设置是否启用增量光栅化
     */
    void SetIncrementalEnabled(bool enabled) { incremental_enabled_ = enabled; }

    /**
     * @brief 检查增量光栅化是否启用
     */
    bool IsIncrementalEnabled() const { return incremental_enabled_; }

    /**
     * @brief 设置是否启用滚动优化
     */
    void SetScrollOptimizationEnabled(bool enabled) { scroll_optimization_enabled_ = enabled; }

    /**
     * @brief 检查滚动优化是否启用
     */
    bool IsScrollOptimizationEnabled() const { return scroll_optimization_enabled_; }

    /**
     * @brief 设置视口尺寸（用于 fixed 元素的 clip rect）
     * @param width 视口宽度
     * @param height 视口高度
     */
    void SetViewportSize(float width, float height) {
        viewport_width_ = width;
        viewport_height_ = height;
    }

private:
    /**
     * @brief 绘制渲染对象到 Canvas
     * @param canvas 目标 Canvas
     * @param obj 渲染对象
     * @param clip_rect 裁剪区域（可选）
     */
    void PaintRenderObject(SkCanvas* canvas, RenderObject* obj, const SkIRect* clip_rect = nullptr);

    /**
     * @brief 递归绘制渲染对象及其子对象
     * @param canvas 目标 Canvas
     * @param obj 渲染对象
     * @param clip_rect 裁剪区域（可选）
     */
    void PaintRenderObjectRecursive(SkCanvas* canvas, RenderObject* obj, const SkIRect* clip_rect);

    /**
     * @brief 应用非根层的 canvas 偏移补偿
     *
     * 统一处理非根层的 layout 位置抵消、fixed 元素 transform 偏移、
     * 动画边界偏移和静态变换偏移的补偿逻辑。
     *
     * @param canvas 目标 Canvas
     * @param layer 合成层
     * @param render_obj 关联的渲染对象
     * @param layout 渲染对象的布局信息
     */
    void ApplyLayerCanvasOffset(SkCanvas* canvas, CompositorLayer* layer,
                                RenderObject* render_obj, const LayoutInfo& layout);

    /**
     * @brief 清除区域为透明
     * @param canvas 目标 Canvas
     * @param region 要清除的区域
     */
    void ClearRegion(SkCanvas* canvas, const SkIRect& region);

    /**
     * @brief 位图像素复制（用于滚动优化）
     * @param layer 目标层
     * @param src_rect 源区域
     * @param dst_x 目标 X
     * @param dst_y 目标 Y
     */
    void CopyPixels(CompositorLayer* layer,
                    const SkIRect& src_rect,
                    int dst_x, int dst_y);

    // 统计信息
    RasterizeStats stats_;

    // 配置选项
    bool incremental_enabled_ = true;
    bool scroll_optimization_enabled_ = true;

    // 视口尺寸（用于 fixed 元素的 clip rect，替代 hardcoded 10000x10000）
    float viewport_width_ = 0.0f;
    float viewport_height_ = 0.0f;

    // 滚动状态缓存（用于检测滚动方向和距离）
    std::unordered_map<uint32_t, SkPoint> last_scroll_offsets_;
};

} // namespace lightui
