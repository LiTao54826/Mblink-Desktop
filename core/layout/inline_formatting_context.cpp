/**
 * @file inline_formatting_context.cpp
 * @brief InlineFormattingContext 的实现
 */

#include "inline_formatting_context.h"
#include "render/render_object.h"
#include <iostream>
#include <algorithm>

namespace lightui {

// ========== 构造和析构 ==========

InlineFormattingContext::InlineFormattingContext(RenderObject* container)
    : container_(container)
    , available_width_(0.0f) {
}

InlineFormattingContext::~InlineFormattingContext() = default;

// ========== 主布局方法 ==========

void InlineFormattingContext::Layout(float available_width) {
    available_width_ = available_width;
    
    // 清空之前的布局结果
    line_boxes_.clear();
    inline_boxes_.clear();
    
    if (!container_) {
        return;
    }
    
    // 步骤1: 收集所有内联盒
    CollectInlineBoxes();
    
    if (inline_boxes_.empty()) {
        return;
    }
    
    // 步骤2: 断行
    BreakIntoLines(available_width);
    
    // 步骤3: 布局每行
    LayoutLines();
    
    // 步骤4: 定位盒子
    PositionBoxes();
}

// ========== 结果获取 ==========

float InlineFormattingContext::GetContentHeight() const {
    if (line_boxes_.empty()) {
        return 0.0f;
    }
    
    // 最后一行的 y + 高度
    const auto& last_line = line_boxes_.back();
    return last_line.y + last_line.height;
}

float InlineFormattingContext::GetContentWidth() const {
    float max_width = 0.0f;
    for (const auto& line : line_boxes_) {
        max_width = std::max(max_width, line.content_width);
    }
    return max_width;
}

// ========== 布局步骤实现 ==========

void InlineFormattingContext::CollectInlineBoxes() {
    if (!container_) return;
    
    // 遍历容器的所有子元素
    for (auto& child : container_->GetChildren()) {
        CollectInlineContent(child.get());
    }
}

void InlineFormattingContext::CollectInlineContent(RenderObject* render_obj) {
    if (!render_obj) return;
    
    RenderObjectType type = render_obj->GetType();
    
    switch (type) {
        case RenderObjectType::TEXT: {
            // 文本节点
            auto* render_text = static_cast<RenderText*>(render_obj);
            ProcessTextNode(render_text);
            break;
        }
        
        case RenderObjectType::INLINE: {
            // 内联元素
            ProcessInlineElement(render_obj);
            break;
        }
        
        case RenderObjectType::INLINE_BLOCK: {
            // 原子内联元素（inline-block, img 等）
            ProcessAtomicInline(render_obj);
            break;
        }
        
        case RenderObjectType::BLOCK:
        case RenderObjectType::FLEX:
        case RenderObjectType::GRID: {
            // 块级元素不应该出现在 IFC 中
            // 这表示需要创建匿名块盒，但这超出了当前实现范围
            break;
        }
        
        default:
            // 其他类型递归处理子元素
            for (auto& child : render_obj->GetChildren()) {
                CollectInlineContent(child.get());
            }
            break;
    }
}

void InlineFormattingContext::ProcessTextNode(RenderText* render_text) {
    if (!render_text) return;
    
    std::string text = render_text->GetText();
    if (text.empty()) return;
    
    // 创建文本类型的内联盒
    InlineBox box = InlineBox::CreateTextBox(render_text);
    
    // 创建单个 TextRun（后续可以优化为按空白分割）
    TextRun run(text);
    run.render_text = render_text;
    run.style = &render_text->GetComputedStyle();
    
    // TODO: 测量文本尺寸（需要 TextRenderer）
    // 暂时使用估算值
    const auto& style = render_text->GetComputedStyle();
    float font_size = style.font_size;
    float char_count = static_cast<float>(run.CharacterCount());
    
    run.width = char_count * font_size * 0.5f;  // 粗略估算
    run.height = font_size * style.line_height;
    run.baseline = font_size * 0.8f;  // 假设基线在 80% 处
    
    // 设置断行信息
    run.can_break_after = true;
    run.is_whitespace = run.IsOnlyWhitespace();
    
    box.text_runs.push_back(run);
    box.width = run.width;
    box.height = run.height;
    box.baseline = run.baseline;
    
    inline_boxes_.push_back(std::move(box));
}

void InlineFormattingContext::ProcessInlineElement(RenderObject* render_obj) {
    if (!render_obj) return;
    
    // 添加开始标记
    inline_boxes_.push_back(InlineBox::CreateInlineStart(render_obj));
    
    // 递归处理子元素
    for (auto& child : render_obj->GetChildren()) {
        CollectInlineContent(child.get());
    }
    
    // 添加结束标记
    inline_boxes_.push_back(InlineBox::CreateInlineEnd(render_obj));
}

void InlineFormattingContext::ProcessAtomicInline(RenderObject* render_obj) {
    if (!render_obj) return;
    
    const auto& layout = render_obj->GetLayoutInfo();
    float width = layout.width;
    float height = layout.height;
    
    // 原子内联元素的基线：
    // MBink 现代模式：有文本内容用文本基线，否则用底部
    float baseline = height;  // 默认基线在底部
    
    // TODO: 如果有文本内容，使用文本基线
    
    InlineBox box = InlineBox::CreateAtomicBox(render_obj, width, height, baseline);
    inline_boxes_.push_back(std::move(box));
}

void InlineFormattingContext::BreakIntoLines(float width) {
    if (inline_boxes_.empty()) return;
    
    // 创建第一行
    CreateNewLine();
    
    for (size_t i = 0; i < inline_boxes_.size(); ++i) {
        InlineBox& box = inline_boxes_[i];
        LineBox& current_line = line_boxes_.back();
        
        // 检查是否能放入当前行
        float box_width = box.GetTotalWidth();
        
        if (current_line.IsEmpty() || current_line.CanFit(box_width)) {
            // 可以放入当前行
            current_line.AddBox(&box);
        } else {
            // 需要换行
            // TODO: 处理文本内部的断行
            CreateNewLine();
            line_boxes_.back().AddBox(&box);
        }
        
        // 检查强制换行
        if (box.IsText() && !box.text_runs.empty()) {
            const auto& last_run = box.text_runs.back();
            if (last_run.is_forced_break) {
                CreateNewLine();
            }
        }
    }
}

void InlineFormattingContext::LayoutLines() {
    float current_y = 0.0f;
    
    for (size_t i = 0; i < line_boxes_.size(); ++i) {
        LineBox& line = line_boxes_[i];
        
        // 设置行的位置
        line.x = 0;
        line.y = current_y;
        
        // 设置行的标记
        line.is_first_line = (i == 0);
        line.is_last_line = (i == line_boxes_.size() - 1);
        
        // 计算行高
        line.CalculateHeight();
        
        // 更新 Y 位置
        current_y += line.height;
    }
}

void InlineFormattingContext::PositionBoxes() {
    // 获取 text-align 属性
    std::string text_align = GetContainerStyle("text-align");
    if (text_align.empty()) {
        text_align = "left";  // 默认左对齐
    }
    
    for (auto& line : line_boxes_) {
        // 对齐内联盒（垂直对齐）
        line.AlignBoxes();
        
        // 应用水平对齐
        line.ApplyTextAlign(text_align);
    }
}

// ========== 辅助方法 ==========

LineBox& InlineFormattingContext::CreateNewLine() {
    line_boxes_.emplace_back(available_width_);
    return line_boxes_.back();
}

std::string InlineFormattingContext::GetContainerStyle(const std::string& property) const {
    if (!container_) return "";
    
    const auto& style = container_->GetComputedStyle();
    
    if (property == "text-align") {
        return style.text_align;
    }
    
    return "";
}

// ========== 调试 ==========

void InlineFormattingContext::DebugPrint() const {
    std::cout << "=== InlineFormattingContext ===" << std::endl;
    std::cout << "Available width: " << available_width_ << std::endl;
    std::cout << "Inline boxes: " << inline_boxes_.size() << std::endl;
    std::cout << "Lines: " << line_boxes_.size() << std::endl;
    std::cout << "Content height: " << GetContentHeight() << std::endl;
    std::cout << "Content width: " << GetContentWidth() << std::endl;
    
    for (size_t i = 0; i < line_boxes_.size(); ++i) {
        const auto& line = line_boxes_[i];
        std::cout << "  Line " << i << ": y=" << line.y 
                  << " h=" << line.height 
                  << " boxes=" << line.boxes.size() << std::endl;
    }
}

} // namespace lightui

