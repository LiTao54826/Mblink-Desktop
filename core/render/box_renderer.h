/**
 * @file box_renderer.h
 * @brief CSS 盒模型渲染器
 * 
 * 功能：
 * - 渲染背景颜色
 * - 渲染边框
 * - 处理内边距和外边距
 * - 渲染盒模型
 */

#pragma once

#include "css_value.h"
#include "paint.h"
#include "shapes.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace lightui {

// 前向声明
class Element;

/**
 * @brief 盒模型定义
 */
struct Box {
    // 内容区域
    float content_x;
    float content_y;
    float content_width;
    float content_height;
    
    // 内边距
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;
    
    // 边框
    float border_top_width;
    float border_right_width;
    float border_bottom_width;
    float border_left_width;
    
    // 外边距
    float margin_top;
    float margin_right;
    float margin_bottom;
    float margin_left;
    
    Box() : content_x(0), content_y(0), content_width(0), content_height(0),
            padding_top(0), padding_right(0), padding_bottom(0), padding_left(0),
            border_top_width(0), border_right_width(0), border_bottom_width(0), border_left_width(0),
            margin_top(0), margin_right(0), margin_bottom(0), margin_left(0) {}
    
    /**
     * @brief 获取内边距盒子（content + padding）
     */
    SkRect GetPaddingBox() const {
        return SkRect::MakeXYWH(
            content_x - padding_left,
            content_y - padding_top,
            content_width + padding_left + padding_right,
            content_height + padding_top + padding_bottom
        );
    }
    
    /**
     * @brief 获取边框盒子（content + padding + border）
     */
    SkRect GetBorderBox() const {
        return SkRect::MakeXYWH(
            content_x - padding_left - border_left_width,
            content_y - padding_top - border_top_width,
            content_width + padding_left + padding_right + border_left_width + border_right_width,
            content_height + padding_top + padding_bottom + border_top_width + border_bottom_width
        );
    }
    
    /**
     * @brief 获取外边距盒子（content + padding + border + margin）
     */
    SkRect GetMarginBox() const {
        return SkRect::MakeXYWH(
            content_x - padding_left - border_left_width - margin_left,
            content_y - padding_top - border_top_width - margin_top,
            content_width + padding_left + padding_right + border_left_width + border_right_width + margin_left + margin_right,
            content_height + padding_top + padding_bottom + border_top_width + border_bottom_width + margin_top + margin_bottom
        );
    }
    
    /**
     * @brief 获取内容盒子
     */
    SkRect GetContentBox() const {
        return SkRect::MakeXYWH(content_x, content_y, content_width, content_height);
    }
};

/**
 * @brief 盒模型渲染器
 */
class BoxRenderer {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布
     */
    explicit BoxRenderer(SkCanvas* canvas);
    
    /**
     * @brief 渲染盒模型
     * @param box 盒模型定义
     * @param styles 样式映射（从 Element::GetStyle 获取）
     */
    void RenderBox(const Box& box, const std::unordered_map<std::string, std::string>& styles);
    
    /**
     * @brief 渲染背景颜色
     * @param box 盒模型定义
     * @param background_color 背景颜色字符串
     */
    void RenderBackground(const Box& box, const std::string& background_color);

    /**
     * @brief 渲染背景（支持颜色、渐变、图片）
     * @param box 盒模型定义
     * @param styles 样式映射
     * @param border_radius 圆角（可选）
     */
    void RenderBackgroundAdvanced(const Box& box,
                                  const std::unordered_map<std::string, std::string>& styles,
                                  const CSSBorderRadius* border_radius = nullptr);
    
    /**
     * @brief 渲染边框
     * @param box 盒模型定义
     * @param border_width 边框宽度字符串
     * @param border_style 边框样式字符串
     * @param border_color 边框颜色字符串
     */
    void RenderBorder(const Box& box, 
                      const std::string& border_width,
                      const std::string& border_style,
                      const std::string& border_color);
    
    /**
     * @brief 渲染单边边框
     * @param x1 起点 x
     * @param y1 起点 y
     * @param x2 终点 x
     * @param y2 终点 y
     * @param width 边框宽度
     * @param style 边框样式
     * @param color 边框颜色
     */
    void RenderBorderEdge(float x1, float y1, float x2, float y2,
                          float width, CSSBorderStyle style, SkColor color);

    /**
     * @brief 渲染圆角边框
     * @param box 盒模型定义
     * @param border_width 边框宽度字符串
     * @param border_style 边框样式字符串
     * @param border_color 边框颜色字符串
     * @param border_radius 圆角定义
     */
    void RenderRoundedBorder(const Box& box,
                            const std::string& border_width,
                            const std::string& border_style,
                            const std::string& border_color,
                            const CSSBorderRadius& border_radius);

    /**
     * @brief 渲染阴影
     * @param box 盒模型定义
     * @param shadows 阴影列表
     * @param border_radius 圆角（可选）
     */
    void RenderBoxShadow(const Box& box,
                        const std::vector<CSSBoxShadow>& shadows,
                        const CSSBorderRadius* border_radius = nullptr);
    
    /**
     * @brief 从样式映射计算盒模型
     * @param styles 样式映射
     * @param x 起始 x 坐标
     * @param y 起始 y 坐标
     * @param width 宽度
     * @param height 高度
     * @param parent_width 父元素宽度（用于百分比计算）
     * @param font_size 字体大小（用于 em 计算）
     * @return Box 对象
     */
    static Box ComputeBox(const std::unordered_map<std::string, std::string>& styles,
                          float x, float y, float width, float height,
                          float parent_width = 0.0f, float font_size = 16.0f);
    
private:
    SkCanvas* canvas_;
    Shapes shapes_;
};

} // namespace lightui

