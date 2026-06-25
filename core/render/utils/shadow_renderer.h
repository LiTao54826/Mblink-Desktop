/**
 * @file shadow_renderer.h
 * @brief CSS 阴影渲染器
 * 
 * 功能：
 * - 渲染 box-shadow（盒子阴影）
 * - 渲染 text-shadow（文本阴影）
 * - 支持多重阴影
 * - 支持 inset 阴影
 * - 支持模糊效果
 */

#pragma once

#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkFont.h"
#include "include/core/SkMaskFilter.h"
#include "css/css_value.h"

namespace mblink {

// 前向声明
class TextRenderer;

/**
 * @brief 阴影渲染器
 *
 * 负责渲染 CSS box-shadow 和 text-shadow
 */
class ShadowRenderer {
public:
    /**
     * @brief 渲染盒子阴影
     * @param canvas Skia 画布
     * @param rect 元素矩形
     * @param shadows 阴影列表（支持多重阴影）
     * @param border_radius 圆角半径（可选）
     *
     * 注意：
     * - 阴影从后往前渲染（最后的阴影在最底层）
     * - 支持 inset 和 outset 阴影
     * - 支持模糊效果
     */
    static void RenderBoxShadow(SkCanvas* canvas,
                               const SkRect& rect,
                               const std::vector<CSSBoxShadow>& shadows,
                               float border_radius = 0.0f);

    /**
     * @brief 渲染带阴影的文本
     * @param canvas Skia 画布
     * @param text 文本内容
     * @param font 字体
     * @param x X 坐标
     * @param y Y 坐标
     * @param text_color 文本颜色
     * @param shadows 阴影列表（支持多重阴影）
     * @param text_renderer 文本渲染器（用于支持emoji）
     *
     * 注意：
     * - 先渲染阴影，再渲染文本
     * - 阴影从后往前渲染
     * - 支持emoji正确渲染
     */
    static void RenderTextWithShadow(SkCanvas* canvas,
                                    const std::string& text,
                                    const SkFont& font,
                                    float x, float y,
                                    SkColor text_color,
                                    const std::vector<CSSTextShadow>& shadows,
                                    TextRenderer& text_renderer);

private:
    /**
     * @brief 渲染外阴影
     * @param canvas Skia 画布
     * @param rect 元素矩形
     * @param shadow 阴影定义
     * @param border_radius 圆角半径
     */
    static void RenderOutsetShadow(SkCanvas* canvas,
                                  const SkRect& rect,
                                  const CSSBoxShadow& shadow,
                                  float border_radius);

    /**
     * @brief 渲染内阴影
     * @param canvas Skia 画布
     * @param rect 元素矩形
     * @param shadow 阴影定义
     * @param border_radius 圆角半径
     */
    static void RenderInsetShadow(SkCanvas* canvas,
                                 const SkRect& rect,
                                 const CSSBoxShadow& shadow,
                                 float border_radius);

    /**
     * @brief 创建模糊滤镜
     * @param blur_radius 模糊半径
     * @return SkMaskFilter 智能指针
     */
    static sk_sp<SkMaskFilter> CreateBlurFilter(float blur_radius);
};

} // namespace mblink

