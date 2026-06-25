/**
 * @file render_flex.h
 * @brief Flex渲染对象（块级flex容器）
 * 
 * 实现 display: flex 的渲染对象
 * 与 RenderInlineFlex 的区别：
 * - RenderFlex 是块级元素（独占一行）
 * - RenderInlineFlex 是内联元素（在行内流中）
 * - 但内部flex布局逻辑相同
 */

#pragma once

#include "render_object.h"

namespace mblink {

// Forward declaration
class RenderFlex;

/**
 * @brief Flex渲染对象（块级flex容器）
 * 
 * CSS标准行为：
 * 1. 块级元素，独占一行
 * 2. 内部按照flex容器布局子元素
 * 3. 默认宽度占满父容器（除非显式设置width）
 * 4. 支持flex-direction, justify-content, align-items等flex属性
 */
class RenderFlex : public RenderObject {
public:
    RenderFlex() : RenderObject(RenderObjectType::FLEX) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

private:
    /**
     * @brief 执行flex布局逻辑
     * @param parent_width 父容器宽度
     * @param parent_height 父容器高度
     * 
     * 这个方法实现了完整的flex布局算法，包括：
     * - flex-direction (row/column)
     * - justify-content (主轴对齐)
     * - align-items (交叉轴对齐)
     * - flex-wrap (换行，暂未实现)
     */
    void LayoutAsFlex(float parent_width, float parent_height);
};

} // namespace mblink

