/**
 * @file inline_formatting_context.h
 * @brief 内联格式化上下文 (IFC) 定义
 * 
 * InlineFormattingContext 是 IFC 的主类，负责：
 * - 收集内联内容
 * - 执行断行
 * - 计算行盒布局
 * - 定位内联盒
 * 
 * MBink 现代模式：
 * - 无 strut：行高由内容决定
 * - 简化空白处理
 * - vertical-align: middle 真正居中
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "text_run.h"
#include "inline_box.h"
#include "line_box.h"

// Forward declarations
namespace lightui {
class RenderObject;
class RenderText;
struct ComputedStyle;
}

namespace lightui {

/**
 * @brief 内联格式化上下文
 * 
 * 管理块级容器内的内联内容布局。
 * 
 * 使用方式：
 * ```cpp
 * InlineFormattingContext ifc(container);
 * ifc.Layout(available_width);
 * float height = ifc.GetContentHeight();
 * ```
 */
class InlineFormattingContext {
public:
    /**
     * @brief 构造函数
     * @param container 包含内联内容的块级容器
     */
    explicit InlineFormattingContext(RenderObject* container);
    
    /** @brief 析构函数 */
    ~InlineFormattingContext();
    
    // 禁止拷贝
    InlineFormattingContext(const InlineFormattingContext&) = delete;
    InlineFormattingContext& operator=(const InlineFormattingContext&) = delete;
    
    // 允许移动
    InlineFormattingContext(InlineFormattingContext&&) noexcept = default;
    InlineFormattingContext& operator=(InlineFormattingContext&&) noexcept = default;
    
    // ========== 主布局方法 ==========
    
    /**
     * @brief 执行布局
     * @param available_width 可用宽度
     * 
     * 布局步骤：
     * 1. 收集所有内联盒
     * 2. 断行
     * 3. 布局每行
     * 4. 定位盒子
     */
    void Layout(float available_width);
    
    // ========== 结果获取 ==========
    
    /**
     * @brief 获取内容高度
     * @return 所有行盒的总高度
     */
    float GetContentHeight() const;
    
    /**
     * @brief 获取内容宽度（最宽行的宽度）
     * @return 最大行宽
     */
    float GetContentWidth() const;
    
    /**
     * @brief 获取行盒列表
     * @return 行盒列表的常量引用
     */
    const std::vector<LineBox>& GetLineBoxes() const { return line_boxes_; }
    
    /**
     * @brief 获取行数
     * @return 行盒数量
     */
    size_t GetLineCount() const { return line_boxes_.size(); }
    
    // ========== 调试 ==========
    
    /**
     * @brief 打印布局信息（调试用）
     */
    void DebugPrint() const;
    
private:
    // ========== 成员变量 ==========
    
    /** @brief 容器渲染对象 */
    RenderObject* container_;
    
    /** @brief 可用宽度 */
    float available_width_ = 0.0f;
    
    /** @brief 行盒列表 */
    std::vector<LineBox> line_boxes_;
    
    /** @brief 内联盒列表（拥有所有权） */
    std::vector<InlineBox> inline_boxes_;
    
    // ========== 布局步骤 ==========
    
    /**
     * @brief 步骤1: 收集所有内联盒
     * 
     * 遍历容器的子元素，为每个内联内容创建 InlineBox。
     * - TEXT 节点 -> TEXT 类型的 InlineBox
     * - inline 元素 -> INLINE_START + 子内容 + INLINE_END
     * - inline-block/img 等 -> ATOMIC 类型的 InlineBox
     */
    void CollectInlineBoxes();
    
    /**
     * @brief 步骤2: 断行
     * @param width 可用宽度
     * 
     * 将内联盒分配到行盒中。
     * 当一行放不下时，在允许的断行点换行。
     */
    void BreakIntoLines(float width);
    
    /**
     * @brief 步骤3: 布局每行
     * 
     * 计算每行的高度和基线。
     */
    void LayoutLines();
    
    /**
     * @brief 步骤4: 定位盒子
     * 
     * 应用 text-align 和 vertical-align，
     * 计算每个内联盒的最终位置。
     */
    void PositionBoxes();
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 递归收集内联内容
     * @param render_obj 当前渲染对象
     */
    void CollectInlineContent(RenderObject* render_obj);
    
    /**
     * @brief 处理文本节点
     * @param render_text 文本渲染对象
     */
    void ProcessTextNode(RenderText* render_text);
    
    /**
     * @brief 处理内联元素
     * @param render_obj 内联渲染对象
     */
    void ProcessInlineElement(RenderObject* render_obj);
    
    /**
     * @brief 处理原子内联元素（img, inline-block 等）
     * @param render_obj 原子内联渲染对象
     */
    void ProcessAtomicInline(RenderObject* render_obj);
    
    /**
     * @brief 创建新行
     * @return 新行盒的引用
     */
    LineBox& CreateNewLine();
    
    /**
     * @brief 获取容器的样式属性
     * @param property 属性名
     * @return 属性值
     */
    std::string GetContainerStyle(const std::string& property) const;
};

} // namespace lightui

