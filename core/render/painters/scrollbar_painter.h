/**
 * @file scrollbar_painter.h
 * @brief 滚动条绘制器 - 负责渲染元素滚动条
 * 
 * 功能：
 * - 渲染水平滚动条
 * - 渲染垂直滚动条
 * - 渲染滚动条角落
 * - 支持滚动条轨道和滑块样式
 * 
 * 从 render_object.cpp 提取的滚动条绘制逻辑
 */

#pragma once

#include "box_renderer.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"

namespace lightui {

// 前向声明
class RenderObject;
struct ComputedStyle;

/**
 * @brief 滚动条绘制参数
 * 
 * 封装滚动条绘制所需的所有参数
 */
struct ScrollbarPaintParams {
    // 可见区域尺寸
    float visible_width = 0.0f;
    float visible_height = 0.0f;
    
    // 内容尺寸
    float content_width = 0.0f;
    float content_height = 0.0f;
    
    // 滚动位置
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    
    // 边框宽度
    float border_left = 0.0f;
    float border_right = 0.0f;
    float border_top = 0.0f;
    float border_bottom = 0.0f;
    
    // 有效区域尺寸（用于滚动条定位）
    float effective_width = 0.0f;
    float effective_height = 0.0f;
    
    // 是否需要滚动条
    bool needs_h_scroll = false;
    bool needs_v_scroll = false;

    // CSS scrollbar-color 自定义颜色
    bool scrollbar_color_auto = true;
    SkColor scrollbar_thumb_color = SK_ColorTRANSPARENT;
    SkColor scrollbar_track_color = SK_ColorTRANSPARENT;
};

/**
 * @brief 滚动条绘制器
 * 
 * 负责渲染元素的滚动条，包括：
 * - 水平滚动条轨道和滑块
 * - 垂直滚动条轨道和滑块
 * - 滚动条角落（两个滚动条都存在时）
 */
class ScrollbarPainter {
public:
    /// 滚动条宽度常量
    static constexpr float kScrollbarWidth = 12.0f;
    
    /// 滚动条边距
    static constexpr float kScrollbarMargin = 2.0f;
    
    /// 滑块圆角半径
    static constexpr float kCornerRadius = 4.0f;
    
    /// 最小滑块尺寸
    static constexpr float kMinThumbSize = 30.0f;

    /**
     * @brief 构造函数
     * @param canvas Skia 画布
     */
    explicit ScrollbarPainter(SkCanvas* canvas);

    /**
     * @brief 绘制滚动条
     * 
     * 根据参数绘制水平和/或垂直滚动条
     * 
     * @param params 滚动条绘制参数
     */
    void Paint(const ScrollbarPaintParams& params);

    /**
     * @brief 绘制水平滚动条
     * @param params 滚动条绘制参数
     */
    void PaintHorizontalScrollbar(const ScrollbarPaintParams& params);

    /**
     * @brief 绘制垂直滚动条
     * @param params 滚动条绘制参数
     */
    void PaintVerticalScrollbar(const ScrollbarPaintParams& params);

    /**
     * @brief 绘制滚动条角落
     * 
     * 当水平和垂直滚动条都存在时，绘制右下角的空白区域
     * 
     * @param params 滚动条绘制参数
     */
    void PaintScrollbarCorner(const ScrollbarPaintParams& params);

    /**
     * @brief 设置轨道颜色
     * @param color 轨道颜色
     */
    void SetTrackColor(SkColor color) { track_color_ = color; }

    /**
     * @brief 设置滑块颜色
     * @param color 滑块颜色
     */
    void SetThumbColor(SkColor color) { thumb_color_ = color; }

    /**
     * @brief 从 RenderObject 创建绘制参数
     * 
     * 辅助方法，从 RenderObject 提取滚动条绘制所需的参数
     * 
     * @param render_object 渲染对象
     * @param box 盒模型
     * @param style 计算后的样式
     * @return 滚动条绘制参数
     */
    static ScrollbarPaintParams CreateParams(
        const RenderObject& render_object,
        const Box& box,
        const ComputedStyle& style);

private:
    /**
     * @brief 绘制轨道
     * @param rect 轨道区域
     */
    void PaintTrack(const SkRect& rect);

    /**
     * @brief 绘制滑块
     * @param rect 滑块区域
     */
    void PaintThumb(const SkRect& rect);

    /**
     * @brief 计算滑块位置和尺寸
     * @param track_size 轨道尺寸
     * @param content_size 内容尺寸
     * @param visible_size 可见区域尺寸
     * @param scroll_pos 滚动位置
     * @param out_thumb_pos 输出：滑块位置
     * @param out_thumb_size 输出：滑块尺寸
     */
    void CalculateThumbMetrics(
        float track_size,
        float content_size,
        float visible_size,
        float scroll_pos,
        float& out_thumb_pos,
        float& out_thumb_size);

    SkCanvas* canvas_;
    
    // 滚动条颜色（可自定义）
    SkColor track_color_ = SkColorSetRGB(241, 241, 241);  // 浅灰色轨道
    SkColor thumb_color_ = SkColorSetRGB(193, 193, 193);  // 深灰色滑块
};

} // namespace lightui
