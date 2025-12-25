/**
 * @file inline_box.h
 * @brief 内联盒结构定义
 * 
 * InlineBox 表示内联格式化上下文中的一个盒子。
 * 支持文本内容、原子内联元素（如 img, inline-block）和内联元素的开始/结束标记。
 */

#pragma once

#include <vector>
#include <memory>
#include "text_run.h"

// Forward declarations
namespace lightui {
class RenderObject;
struct ComputedStyle;
}

namespace lightui {

/**
 * @brief 内联盒类型
 */
enum class InlineBoxType {
    /** @brief 文本内容 */
    TEXT,
    
    /** @brief 原子内联元素 (img, inline-block, svg, 表单控件等) */
    ATOMIC,
    
    /** @brief 内联元素开始标记 (如 <span> 开始) */
    INLINE_START,
    
    /** @brief 内联元素结束标记 (如 </span> 结束) */
    INLINE_END
};

/**
 * @brief 内联盒结构
 * 
 * 表示内联格式化上下文中的一个盒子，可以是：
 * - TEXT: 文本内容，包含一个或多个 TextRun
 * - ATOMIC: 原子内联元素，有固定尺寸
 * - INLINE_START: 内联元素的开始，有 margin/padding/border-left
 * - INLINE_END: 内联元素的结束，有 margin/padding/border-right
 */
struct InlineBox {
    // ========== 类型和来源 ==========
    
    /** @brief 盒子类型 */
    InlineBoxType type = InlineBoxType::TEXT;
    
    /** @brief 关联的渲染对象（非拥有指针） */
    RenderObject* render_object = nullptr;
    
    /** @brief 关联的样式（非拥有指针） */
    const ComputedStyle* style = nullptr;
    
    // ========== 尺寸信息 ==========

    /** @brief 盒子宽度（像素） */
    float width = 0.0f;

    /** @brief 盒子高度（像素） */
    float height = 0.0f;

    /** @brief 基线位置（距离盒子顶部的像素） */
    float baseline = 0.0f;

    /** @brief 行高倍数（CSS line-height 属性） */
    float line_height_multiplier = 1.2f;

    /** @brief Skia 测量的 ascent（从基线向上的距离，正值） */
    float skia_ascent = 0.0f;

    /** @brief Skia 测量的 descent（从基线向下的距离，正值） */
    float skia_descent = 0.0f;
    
    // ========== 位置信息（布局后填充） ==========
    
    /** @brief X 坐标（相对于行盒） */
    float x = 0.0f;
    
    /** @brief Y 坐标（相对于行盒） */
    float y = 0.0f;
    
    // ========== 边距/内边距/边框 ==========
    // 水平方向：用于 INLINE_START、INLINE_END 和 ATOMIC 类型
    // 垂直方向：仅用于 ATOMIC 类型（inline-block等）
    
    /** @brief 左边距 */
    float margin_left = 0.0f;
    
    /** @brief 右边距 */
    float margin_right = 0.0f;
    
    /** @brief 上边距（仅用于ATOMIC类型，如inline-block） */
    float margin_top = 0.0f;
    
    /** @brief 下边距（仅用于ATOMIC类型，如inline-block） */
    float margin_bottom = 0.0f;
    
    /** @brief 左内边距 */
    float padding_left = 0.0f;
    
    /** @brief 右内边距 */
    float padding_right = 0.0f;
    
    /** @brief 左边框宽度 */
    float border_left = 0.0f;
    
    /** @brief 右边框宽度 */
    float border_right = 0.0f;
    
    // ========== 文本内容 ==========
    // 仅用于 TEXT 类型
    
    /** @brief 文本片段列表 */
    std::vector<TextRun> text_runs;
    
    // ========== 构造函数 ==========
    
    /** @brief 默认构造函数 */
    InlineBox() = default;
    
    /**
     * @brief 带类型的构造函数
     * @param t 盒子类型
     */
    explicit InlineBox(InlineBoxType t) : type(t) {}
    
    /**
     * @brief 创建文本类型的内联盒
     * @param render_obj 关联的渲染对象
     * @return 文本类型的内联盒
     */
    static InlineBox CreateTextBox(RenderObject* render_obj);
    
    /**
     * @brief 创建原子内联元素的内联盒
     * @param render_obj 关联的渲染对象
     * @param w 宽度
     * @param h 高度
     * @param bl 基线位置
     * @return 原子类型的内联盒
     */
    static InlineBox CreateAtomicBox(RenderObject* render_obj, float w, float h, float bl);
    
    /**
     * @brief 创建内联开始标记
     * @param render_obj 关联的渲染对象
     * @return 开始标记类型的内联盒
     */
    static InlineBox CreateInlineStart(RenderObject* render_obj);
    
    /**
     * @brief 创建内联结束标记
     * @param render_obj 关联的渲染对象
     * @return 结束标记类型的内联盒
     */
    static InlineBox CreateInlineEnd(RenderObject* render_obj);
    
    // ========== 辅助方法 ==========
    
    /** @brief 检查是否是文本类型 */
    bool IsText() const { return type == InlineBoxType::TEXT; }
    
    /** @brief 检查是否是原子类型 */
    bool IsAtomic() const { return type == InlineBoxType::ATOMIC; }
    
    /** @brief 检查是否是内联开始标记 */
    bool IsInlineStart() const { return type == InlineBoxType::INLINE_START; }
    
    /** @brief 检查是否是内联结束标记 */
    bool IsInlineEnd() const { return type == InlineBoxType::INLINE_END; }
    
    /** @brief 获取左侧额外空间（margin + padding + border） */
    float GetLeftSpace() const {
        return margin_left + padding_left + border_left;
    }
    
    /** @brief 获取右侧额外空间（margin + padding + border） */
    float GetRightSpace() const {
        return margin_right + padding_right + border_right;
    }
    
    /** @brief 获取总宽度（包括边距等） */
    float GetTotalWidth() const {
        return width + GetLeftSpace() + GetRightSpace();
    }
};

} // namespace lightui

