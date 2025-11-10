/**
 * @file dirty_region.h
 * @brief 脏区域检测系统
 *
 * 功能：
 * - 跟踪需要重绘的区域
 * - 合并相邻脏区域
 * - 优化重绘范围
 * - 减少不必要的渲染
 */

#pragma once

#include "include/core/SkRect.h"
#include <vector>
#include <optional>

namespace lightui {

/**
 * @brief 脏区域管理器
 *
 * 管理需要重绘的区域，优化渲染性能
 */
class DirtyRegion {
public:
    /**
     * @brief 构造函数
     */
    DirtyRegion() = default;

    /**
     * @brief 添加脏区域
     * @param rect 脏区域矩形
     */
    void AddRect(const SkRect& rect);

    /**
     * @brief 添加脏区域（坐标形式）
     * @param x X 坐标
     * @param y Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void AddRect(float x, float y, float width, float height);

    /**
     * @brief 标记整个区域为脏
     * @param width 区域宽度
     * @param height 区域高度
     */
    void MarkAll(float width, float height);

    /**
     * @brief 清空所有脏区域
     */
    void Clear();

    /**
     * @brief 检查是否有脏区域
     * @return 是否有脏区域
     */
    bool IsDirty() const { return !regions_.empty(); }

    /**
     * @brief 获取合并后的脏区域
     * @return 合并后的矩形，如果没有脏区域则返回空
     */
    std::optional<SkRect> GetBoundingRect() const;

    /**
     * @brief 获取所有脏区域
     * @return 脏区域列表
     */
    const std::vector<SkRect>& GetRegions() const { return regions_; }

    /**
     * @brief 合并相邻的脏区域
     *
     * 将距离较近的脏区域合并，减少绘制次数
     */
    void Optimize();

    /**
     * @brief 检查矩形是否与脏区域相交
     * @param rect 要检查的矩形
     * @return 是否相交
     */
    bool Intersects(const SkRect& rect) const;

private:
    /**
     * @brief 检查两个矩形是否应该合并
     * @param a 矩形 A
     * @param b 矩形 B
     * @return 是否应该合并
     */
    bool ShouldMerge(const SkRect& a, const SkRect& b) const;

    /**
     * @brief 合并两个矩形
     * @param a 矩形 A
     * @param b 矩形 B
     * @return 合并后的矩形
     */
    SkRect Merge(const SkRect& a, const SkRect& b) const;

    std::vector<SkRect> regions_;  ///< 脏区域列表
    
    // 合并阈值：如果两个矩形的距离小于此值，则合并
    static constexpr float kMergeThreshold = 10.0f;
};

/**
 * @brief 脏标记接口
 *
 * 可以被标记为脏的对象需要实现此接口
 */
class IDirtyMarkable {
public:
    virtual ~IDirtyMarkable() = default;

    /**
     * @brief 标记为需要布局
     */
    virtual void MarkNeedsLayout() = 0;

    /**
     * @brief 标记为需要绘制
     */
    virtual void MarkNeedsPaint() = 0;

    /**
     * @brief 检查是否需要布局
     * @return 是否需要布局
     */
    virtual bool NeedsLayout() const = 0;

    /**
     * @brief 检查是否需要绘制
     * @return 是否需要绘制
     */
    virtual bool NeedsPaint() const = 0;
};

} // namespace lightui

