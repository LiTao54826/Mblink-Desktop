/**
 * @file background_painter.h
 * @brief 背景绘制器 - 负责渲染元素背景
 * 
 * 功能：
 * - 渲染纯色背景
 * - 渲染线性渐变背景
 * - 渲染径向渐变背景
 * - 渲染背景图片
 * - 支持圆角裁剪
 * 
 * 从 render_object.cpp 和 box_renderer.cpp 提取的背景绘制逻辑
 */

#pragma once

#include "box_renderer.h"
#include "core/render/css/css_value.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkColor.h"
#include <string>
#include <unordered_map>
#include <optional>

namespace mbink {

// 前向声明
class RenderObject;
struct ComputedStyle;
struct PaintCache;

/**
 * @brief 背景绘制器
 * 
 * 负责渲染元素的背景，包括：
 * - 纯色背景
 * - 线性渐变
 * - 径向渐变
 * - 背景图片
 */
class BackgroundPainter {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布
     */
    explicit BackgroundPainter(SkCanvas* canvas);

    /**
     * @brief 绘制背景
     * 
     * 根据样式绘制背景，优先级：
     * 1. 线性渐变
     * 2. 径向渐变
     * 3. 背景图片
     * 4. 纯色背景
     * 
     * @param box 盒模型定义
     * @param style 计算后的样式
     * @param cache 绘制缓存
     */
    void Paint(const Box& box, 
               const ComputedStyle& style, 
               const PaintCache& cache);

    /**
     * @brief 绘制纯色背景
     * @param rect 绘制区域
     * @param color 背景颜色
     * @param border_radius 圆角（可选）
     */
    void PaintSolidColor(const SkRect& rect, 
                         SkColor color,
                         const CSSBorderRadius* border_radius = nullptr);

    /**
     * @brief 绘制线性渐变背景
     * @param rect 绘制区域
     * @param gradient 线性渐变定义
     * @param border_radius 圆角（可选）
     */
    void PaintLinearGradient(const SkRect& rect,
                             const CSSLinearGradient& gradient,
                             const CSSBorderRadius* border_radius = nullptr);

    /**
     * @brief 绘制径向渐变背景
     * @param rect 绘制区域
     * @param gradient 径向渐变定义
     * @param border_radius 圆角（可选）
     */
    void PaintRadialGradient(const SkRect& rect,
                             const CSSRadialGradient& gradient,
                             const CSSBorderRadius* border_radius = nullptr);

    /**
     * @brief 绘制背景图片
     * @param rect 绘制区域
     * @param image_url 图片 URL
     * @param repeat 重复模式
     * @param size 背景尺寸
     * @param border_radius 圆角（可选）
     */
    void PaintBackgroundImage(const SkRect& rect,
                              const std::string& image_url,
                              CSSBackgroundRepeat repeat,
                              const CSSBackgroundSize& size,
                              const CSSBorderRadius* border_radius = nullptr);

    /**
     * @brief 使用样式映射绘制背景（兼容旧接口）
     * @param box 盒模型定义
     * @param styles 样式映射
     * @param border_radius 圆角（可选）
     */
    void PaintWithStyles(const Box& box,
                         const std::unordered_map<std::string, std::string>& styles,
                         const CSSBorderRadius* border_radius = nullptr);

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

    SkCanvas* canvas_;
};

} // namespace mbink
