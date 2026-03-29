/**
 * @file render_inline_flex.h
 * @brief Inline-Flex渲染对象 - 符合CSS标准的inline-flex实现
 * 
 * 功能：
 * - 在行内水平排列（像inline元素）
 * - 内部使用flex布局（像flex容器）
 * - 宽度自适应内容（shrink-to-fit）
 * - 用于 display: inline-flex 元素
 */

#pragma once

#include "render_object.h"

namespace mbink {

// Forward declaration
class RenderInlineFlex;

/**
 * @brief Inline-Flex渲染对象
 * 
 * CSS标准行为:
 * 1. 在行内流中参与布局（不会独占一行）
 * 2. 内部按照flex容器布局子元素
 * 3. 如果width为auto，宽度自适应内容
 * 4. 支持flex-direction, justify-content, align-items等flex属性
 */
class RenderInlineFlex : public RenderObject {
public:
    RenderInlineFlex() : RenderObject(RenderObjectType::INLINE_FLEX) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

    /**
     * @brief 计算元素的固有尺寸（用于 IFC 布局）
     * @param available_width 可用宽度
     * @return 包含宽度和高度的尺寸
     */
    std::pair<float, float> MeasureIntrinsicSize(float available_width);

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

} // namespace mbink

