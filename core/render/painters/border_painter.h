/**
 * @file border_painter.h
 * @brief 边框绘制器 - 负责渲染元素边框
 * 
 * 功能：
 * - 渲染实线边框
 * - 渲染虚线边框
 * - 渲染点线边框
 * - 支持圆角边框
 * - 支持每边独立样式
 * 
 * 从 render_object.cpp 和 box_renderer.cpp 提取的边框绘制逻辑
 */

#pragma once

#include "box_renderer.h"
#include "core/render/css/css_value.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkColor.h"
#include "include/core/SkPath.h"
#include <string>
#include <vector>

namespace lightui {

// 前向声明
class RenderObject;
struct ComputedStyle;
struct PaintCache;

/**
 * @brief 边框绘制器
 * 
 * 负责渲染元素的边框，包括：
 * - 实线边框 (solid)
 * - 虚线边框 (dashed)
 * - 点线边框 (dotted)
 * - 圆角边框
 * - 每边独立样式
 */
class BorderPainter {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布
     */
    explicit BorderPainter(SkCanvas* canvas);

    /**
     * @brief 绘制边框
     * 
     * 根据样式绘制边框，支持：
     * - 四边统一样式
     * - 每边独立样式
     * - 圆角边框
     * 
     * @param box 盒模型定义
     * @param style 计算后的样式
     * @param cache 绘制缓存
     * @param children 子渲染对象（用于 fieldset legend 处理）
     */
    void Paint(const Box& box, 
               const ComputedStyle& style, 
               const PaintCache& cache,
               const std::vector<std::shared_ptr<RenderObject>>* children = nullptr);

    /**
     * @brief 绘制单边边框
     * @param x1 起点 x
     * @param y1 起点 y
     * @param x2 终点 x
     * @param y2 终点 y
     * @param width 边框宽度
     * @param style 边框样式
     * @param color 边框颜色
     */
    void PaintBorderEdge(float x1, float y1, float x2, float y2,
                         float width, CSSBorderStyle style, SkColor color);

    /**
     * @brief 绘制圆角边框（四边统一样式）
     * @param box 盒模型定义
     * @param border_width 边框宽度
     * @param border_style 边框样式
     * @param border_color 边框颜色
     * @param border_radius 圆角定义
     */
    void PaintRoundedBorder(const Box& box,
                            float border_width,
                            CSSBorderStyle border_style,
                            SkColor border_color,
                            const CSSBorderRadius& border_radius);

    /**
     * @brief 绘制圆角边框（每边独立样式）
     * 
     * 如果四边属性完全相同，使用 SkRRect 绘制完整圆角矩形。
     * 如果四边属性不同，回退到分段绘制直边（不绘制圆角部分）。
     * 
     * @param box 盒模型定义
     * @param border_widths 四边宽度 [top, right, bottom, left]
     * @param border_styles 四边样式 [top, right, bottom, left]
     * @param border_colors 四边颜色 [top, right, bottom, left]
     * @param border_radius 圆角定义
     */
    void PaintRoundedBorderAdvanced(const Box& box,
                                    const float border_widths[4],
                                    const CSSBorderStyle border_styles[4],
                                    const SkColor border_colors[4],
                                    const CSSBorderRadius& border_radius);

private:
    /**
     * @brief 创建带圆角的路径
     * @param rect 矩形区域
     * @param border_radius 圆角定义
     * @return Skia 路径
     */
    SkPath CreateRoundedPath(const SkRect& rect, 
                             const CSSBorderRadius& border_radius);

    /**
     * @brief 计算圆角半径（处理百分比）
     * @param border_radius 圆角定义
     * @param width 元素宽度
     * @param height 元素高度
     * @param out_radii 输出的四个角半径 [TL, TR, BR, BL]
     */
    void CalculateRadii(const CSSBorderRadius& border_radius,
                        float width, float height,
                        SkVector out_radii[4]);

    /**
     * @brief 检查四边属性是否相同
     * @param widths 四边宽度
     * @param styles 四边样式
     * @param colors 四边颜色
     * @return 如果四边属性完全相同返回 true
     */
    bool AreAllSidesEqual(const float widths[4],
                          const CSSBorderStyle styles[4],
                          const SkColor colors[4]) const;

    /**
     * @brief 设置画笔的边框样式（实线、虚线、点线）
     * @param paint 画笔
     * @param style 边框样式
     * @param width 边框宽度
     */
    void SetBorderStyle(SkPaint& paint, CSSBorderStyle style, float width);

    /**
     * @brief 绘制分段边框（四边属性不同时使用）
     * @param box 盒模型定义
     * @param border_widths 四边宽度 [top, right, bottom, left]
     * @param border_styles 四边样式 [top, right, bottom, left]
     * @param border_colors 四边颜色 [top, right, bottom, left]
     * @param outer_radii 外圆角半径 [TL, TR, BR, BL]
     * @param border_box 边框盒子矩形
     * @param has_top 是否有上边框
     * @param has_right 是否有右边框
     * @param has_bottom 是否有下边框
     * @param has_left 是否有左边框
     */
    void PaintSegmentedBorder(const Box& box,
                              const float border_widths[4],
                              const CSSBorderStyle border_styles[4],
                              const SkColor border_colors[4],
                              const SkVector outer_radii[4],
                              const SkRect& border_box,
                              bool has_top, bool has_right, 
                              bool has_bottom, bool has_left);

    SkCanvas* canvas_;
};

} // namespace lightui
