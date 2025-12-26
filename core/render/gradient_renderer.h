/**
 * @file gradient_renderer.h
 * @brief CSS 渐变渲染器
 * 
 * 功能：
 * - 渲染 linear-gradient（线性渐变）
 * - 渲染 radial-gradient（径向渐变）
 * - 支持多色停止点
 * - 支持角度和方向
 */

#pragma once

#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkShader.h"
#include "include/effects/SkGradientShader.h"
#include "css/css_value.h"

namespace lightui {

/**
 * @brief 渐变渲染器
 * 
 * 负责渲染 CSS linear-gradient 和 radial-gradient
 */
class GradientRenderer {
public:
    /**
     * @brief 渲染线性渐变
     * @param canvas Skia 画布
     * @param rect 渲染区域
     * @param gradient 线性渐变定义
     * 
     * 注意：
     * - 角度从上方（0度）顺时针旋转
     * - 支持多个颜色停止点
     */
    static void RenderLinearGradient(SkCanvas* canvas,
                                     const SkRect& rect,
                                     const CSSLinearGradient& gradient);

    /**
     * @brief 渲染径向渐变
     * @param canvas Skia 画布
     * @param rect 渲染区域
     * @param gradient 径向渐变定义
     * 
     * 注意：
     * - 支持圆形和椭圆形
     * - 中心点坐标为相对位置（0.0-1.0）
     */
    static void RenderRadialGradient(SkCanvas* canvas,
                                     const SkRect& rect,
                                     const CSSRadialGradient& gradient);

private:
    /**
     * @brief 计算线性渐变的起点和终点
     * @param rect 渲染区域
     * @param angle 角度（度数）
     * @param start 输出起点
     * @param end 输出终点
     */
    static void CalculateLinearGradientPoints(const SkRect& rect,
                                              float angle,
                                              SkPoint& start,
                                              SkPoint& end);

    /**
     * @brief 从 CSSGradientStop 创建 Skia 颜色和位置数组
     * @param stops CSS 渐变停止点
     * @param colors 输出颜色数组
     * @param positions 输出位置数组
     */
    static void ExtractColorsAndPositions(const std::vector<CSSGradientStop>& stops,
                                         std::vector<SkColor>& colors,
                                         std::vector<SkScalar>& positions);
};

} // namespace lightui

