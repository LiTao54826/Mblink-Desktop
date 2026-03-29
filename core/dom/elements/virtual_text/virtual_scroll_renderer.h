/**
 * @file virtual_scroll_renderer.h
 * @brief 虚拟滚动渲染基类
 *
 * 提供虚拟滚动渲染的基础设施，只渲染可见区域以优化性能。
 */

#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkTypeface.h"

namespace mbink {

/**
 * @brief 虚拟滚动渲染基类
 *
 * VirtualScrollRenderer 提供虚拟滚动的基础功能：
 * - 字体度量计算
 * - 滚动偏移管理
 * - 可见行数计算
 * - 命中测试
 *
 * 子类需要实现 Render() 方法来渲染具体内容。
 *
 * @note 这是一个抽象基类
 */
class VirtualScrollRenderer {
public:
    VirtualScrollRenderer();
    virtual ~VirtualScrollRenderer() = default;

    // === 字体配置 ===

    /**
     * @brief 设置字体
     * @param typeface 字体
     * @param size 字号
     */
    void SetFont(sk_sp<SkTypeface> typeface, float size);

    /**
     * @brief 设置行高
     * @param height 行高（像素）
     */
    void SetLineHeight(float height);

    /**
     * @brief 设置内边距
     * @param padding 内边距（像素）
     */
    void SetPadding(float padding);

    // === 滚动控制 ===

    /**
     * @brief 设置滚动偏移（行号）
     * @param line_offset 第一个可见行的索引
     */
    void SetScrollOffset(int line_offset);

    /**
     * @brief 获取滚动偏移
     * @return 第一个可见行的索引
     */
    int scroll_offset() const { return scroll_offset_; }

    /**
     * @brief 滚动到指定行
     * @param line 目标行号
     */
    void ScrollTo(int line);

    /**
     * @brief 相对滚动
     * @param delta 滚动行数（正数向下，负数向上）
     */
    void ScrollBy(int delta);

    /**
     * @brief 设置总行数
     * @param total 总行数
     */
    void SetTotalLines(int total);

    // === 尺寸信息 ===

    /**
     * @brief 获取可见行数
     * @return 可见行数
     */
    int visible_lines() const { return visible_lines_; }

    /**
     * @brief 获取总行数
     * @return 总行数
     */
    int total_lines() const { return total_lines_; }

    /**
     * @brief 获取最大滚动偏移
     * @return 最大滚动偏移值
     */
    int max_scroll_offset() const {
        if (total_lines_ <= visible_lines_) return 0;
        return total_lines_ - visible_lines_;
    }

    /**
     * @brief 获取行高
     * @return 行高（像素）
     */
    float line_height() const { return line_height_; }

    /**
     * @brief 获取字符宽度（等宽字体）
     * @return 字符宽度（像素）
     */
    float cell_width() const { return cell_width_; }

    /**
     * @brief 更新视图尺寸
     * @param view_height 视图高度（像素）
     */
    void UpdateMetrics(float view_height);

    // === 命中测试 ===

    /**
     * @brief 根据 Y 坐标计算行号
     * @param y Y 坐标（相对于视图顶部）
     * @return 行号（相对于缓冲区）
     */
    int HitTestLine(float y) const;

    /**
     * @brief 根据 X 坐标计算列号
     * @param x X 坐标（相对于视图左侧）
     * @return 列号
     */
    int HitTestColumn(float x) const;

    // === 渲染（子类实现）===

    /**
     * @brief 渲染内容
     * @param canvas Skia 画布
     * @param bounds 渲染区域
     */
    virtual void Render(SkCanvas* canvas, const SkRect& bounds) = 0;

protected:
    sk_sp<SkTypeface> typeface_;
    float font_size_ = 14.0f;
    float line_height_ = 0;
    float cell_width_ = 0;
    float padding_ = 4.0f;
    int scroll_offset_ = 0;
    int visible_lines_ = 0;
    int total_lines_ = 0;

    /**
     * @brief 更新字符度量
     *
     * 根据当前字体计算 cell_width_ 和 line_height_。
     */
    void UpdateCellMetrics();

    /**
     * @brief 获取指定行的渲染区域
     * @param line_index 行索引（相对于可见区域，0 为第一个可见行）
     * @param bounds 视图边界
     * @return 行的渲染区域
     */
    SkRect GetLineRect(int line_index, const SkRect& bounds) const;

    /**
     * @brief 限制滚动偏移在有效范围内
     */
    void ClampScrollOffset();
};

}  // namespace mbink
