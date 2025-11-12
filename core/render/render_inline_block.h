/**
 * @file render_inline_block.h
 * @brief Inline-Block渲染对象 - 符合CSS标准的inline-block实现
 * 
 * 功能：
 * - 在行内水平排列（像inline元素）
 * - 可以设置宽高（像block元素）
 * - 支持shrink-to-fit宽度计算
 * - 用于button, input, select, textarea等表单控件
 */

#pragma once

#include "render_object.h"

namespace lightui {

/**
 * @brief Inline-Block渲染对象
 * 
 * CSS标准行为:
 * 1. 在行内流中参与布局（不会独占一行）
 * 2. 可以设置width和height
 * 3. 如果width为auto，使用shrink-to-fit算法
 * 4. 内部按照block容器布局子元素
 */
class RenderInlineBlock : public RenderObject {
public:
    RenderInlineBlock() : RenderObject(RenderObjectType::INLINE_BLOCK) {}

    void Layout(float parent_width, float parent_height) override;
    void Paint(SkCanvas* canvas) override;

private:
    /**
     * @brief 计算shrink-to-fit宽度
     * @param available_width 可用宽度
     * @return 计算出的宽度
     * 
     * Shrink-to-fit算法 (CSS 2.1规范):
     * min(max(preferred minimum width, available width), preferred width)
     */
    float CalculateShrinkToFitWidth(float available_width);
    
    /**
     * @brief 计算首选最小宽度（内容不换行的最小宽度）
     */
    float CalculatePreferredMinimumWidth();
    
    /**
     * @brief 计算首选宽度（内容自然布局的宽度）
     */
    float CalculatePreferredWidth();
    
    /**
     * @brief 渲染input元素的特定内容
     */
    void PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box);
    
    /**
     * @brief 渲染textarea元素的特定内容
     */
    void PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box);
    
    /**
     * @brief 渲染select元素的特定内容
     */
    void PaintSelectElement(SkCanvas* canvas, Element* select, const Box& box);
};

} // namespace lightui

