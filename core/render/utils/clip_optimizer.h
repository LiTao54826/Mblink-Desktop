/**
 * @file clip_optimizer.h
 * @brief 裁剪优化系统
 *
 * 功能：
 * - 裁剪不可见区域
 * - 减少绘制量
 * - 实现视口裁剪
 * - 优化性能
 */

#pragma once

#include "include/core/SkRect.h"
#include "include/core/SkCanvas.h"
#include <memory>
#include <vector>

namespace mbink {

// 前向声明
class RenderObject;

/**
 * @brief 裁剪优化器
 *
 * 优化渲染时的裁剪操作，减少不必要的绘制
 */
class ClipOptimizer {
public:
    /**
     * @brief 构造函数
     */
    ClipOptimizer() = default;

    /**
     * @brief 析构函数
     */
    ~ClipOptimizer() = default;

    // ========== 视口裁剪 ==========

    /**
     * @brief 设置视口
     * @param viewport 视口矩形
     */
    void SetViewport(const SkRect& viewport) { viewport_ = viewport; }

    /**
     * @brief 获取视口
     */
    const SkRect& GetViewport() const { return viewport_; }

    /**
     * @brief 检查矩形是否在视口内
     * @param rect 矩形
     * @return 是否在视口内（包括部分相交）
     */
    bool IsInViewport(const SkRect& rect) const;

    /**
     * @brief 检查矩形是否完全在视口外
     * @param rect 矩形
     * @return 是否完全在视口外
     */
    bool IsOutsideViewport(const SkRect& rect) const;

    /**
     * @brief 裁剪矩形到视口
     * @param rect 矩形
     * @return 裁剪后的矩形
     */
    SkRect ClipToViewport(const SkRect& rect) const;

    // ========== 渲染对象裁剪 ==========

    /**
     * @brief 检查渲染对象是否可见
     * @param render_object 渲染对象
     * @return 是否可见
     */
    bool IsVisible(const RenderObject* render_object) const;

    /**
     * @brief 过滤可见的渲染对象
     * @param objects 渲染对象列表
     * @return 可见的渲染对象列表
     */
    std::vector<std::shared_ptr<RenderObject>> FilterVisible(
        const std::vector<std::shared_ptr<RenderObject>>& objects);

    // ========== 裁剪区域管理 ==========

    /**
     * @brief 添加裁剪区域
     * @param clip_rect 裁剪矩形
     */
    void PushClipRect(const SkRect& clip_rect);

    /**
     * @brief 移除最后添加的裁剪区域
     */
    void PopClipRect();

    /**
     * @brief 获取当前裁剪区域
     * @return 当前裁剪矩形
     */
    SkRect GetCurrentClipRect() const;

    /**
     * @brief 清空所有裁剪区域
     */
    void ClearClipRects();

    // ========== 裁剪优化 ==========

    /**
     * @brief 优化裁剪区域
     *
     * 合并重叠的裁剪区域，减少裁剪操作
     */
    void OptimizeClipRects();

    /**
     * @brief 应用裁剪到画布
     * @param canvas 画布
     */
    void ApplyClip(SkCanvas* canvas) const;

    // ========== 统计信息 ==========

    /**
     * @brief 获取裁剪的对象数量
     */
    size_t GetClippedObjectCount() const { return clipped_object_count_; }

    /**
     * @brief 获取总对象数量
     */
    size_t GetTotalObjectCount() const { return total_object_count_; }

    /**
     * @brief 获取裁剪率
     */
    float GetClipRate() const {
        return total_object_count_ > 0 
            ? static_cast<float>(clipped_object_count_) / total_object_count_ 
            : 0.0f;
    }

    /**
     * @brief 重置统计信息
     */
    void ResetStats() {
        clipped_object_count_ = 0;
        total_object_count_ = 0;
    }

private:
    SkRect viewport_;                      // 视口
    std::vector<SkRect> clip_rects_;       // 裁剪区域栈
    size_t clipped_object_count_;          // 裁剪的对象数量
    size_t total_object_count_;            // 总对象数量
};

/**
 * @brief 视口裁剪器
 *
 * 简化的视口裁剪工具
 */
class ViewportClipper {
public:
    /**
     * @brief 构造函数
     * @param viewport 视口矩形
     */
    explicit ViewportClipper(const SkRect& viewport) : viewport_(viewport) {}

    /**
     * @brief 检查点是否在视口内
     */
    bool Contains(float x, float y) const {
        return viewport_.contains(x, y);
    }

    /**
     * @brief 检查矩形是否与视口相交
     */
    bool Intersects(const SkRect& rect) const {
        return SkRect::Intersects(viewport_, rect);
    }

    /**
     * @brief 检查矩形是否完全在视口内
     */
    bool ContainsRect(const SkRect& rect) const {
        return viewport_.contains(rect);
    }

    /**
     * @brief 裁剪矩形到视口
     */
    SkRect Clip(const SkRect& rect) const {
        SkRect result;
        if (result.intersect(viewport_, rect)) {
            return result;
        }
        return SkRect::MakeEmpty();
    }

    /**
     * @brief 获取视口
     */
    const SkRect& GetViewport() const { return viewport_; }

private:
    SkRect viewport_;
};

/**
 * @brief 裁剪区域 RAII 辅助类
 *
 * 自动管理裁剪区域的压入和弹出
 */
class ScopedClip {
public:
    /**
     * @brief 构造函数
     * @param optimizer 裁剪优化器
     * @param clip_rect 裁剪矩形
     */
    ScopedClip(ClipOptimizer& optimizer, const SkRect& clip_rect)
        : optimizer_(optimizer) {
        optimizer_.PushClipRect(clip_rect);
    }

    /**
     * @brief 析构函数
     */
    ~ScopedClip() {
        optimizer_.PopClipRect();
    }

    // 禁止拷贝和移动
    ScopedClip(const ScopedClip&) = delete;
    ScopedClip& operator=(const ScopedClip&) = delete;
    ScopedClip(ScopedClip&&) = delete;
    ScopedClip& operator=(ScopedClip&&) = delete;

private:
    ClipOptimizer& optimizer_;
};

} // namespace mbink

