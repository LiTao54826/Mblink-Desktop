/**
 * @file vertical_aligner.cpp
 * @brief 垂直对齐器实现
 */

#include "vertical_aligner.h"
#include "core/render/render_object.h"
#include <algorithm>
#include <cmath>

namespace lightui {

// ========== 盒子度量 ==========

BoxVerticalMetrics VerticalAligner::GetBoxMetrics(const InlineBox& box) {
    BoxVerticalMetrics metrics;
    
    metrics.height = box.height;
    
    if (box.IsText()) {
        // 文本盒子：使用字体度量
        // 典型情况：ascent ≈ 80% height, descent ≈ 20% height
        metrics.ascent = box.height * 0.8f;
        metrics.descent = box.height * 0.2f;
        metrics.baseline = metrics.ascent;
    } else if (box.IsAtomic()) {
        // 原子盒子（inline-block, 图片等）
        // 基线在底部（现代模式简化处理）
        metrics.ascent = box.height;
        metrics.descent = 0.0f;
        metrics.baseline = box.height;
    } else {
        // 其他（inline-start, inline-end）
        // 使用父元素的度量
        metrics.ascent = box.height * 0.8f;
        metrics.descent = box.height * 0.2f;
        metrics.baseline = metrics.ascent;
    }
    
    return metrics;
}

// ========== 行度量计算 ==========

LineVerticalMetrics VerticalAligner::CalculateLineMetrics(
    const std::vector<InlineBox*>& boxes,
    const std::vector<VerticalAlignInfo>& aligns
) {
    LineVerticalMetrics metrics;

    if (boxes.empty()) {
        return metrics;
    }

    // 第一遍：收集基线对齐盒子的度量
    float max_ascent = 0.0f;
    float max_descent = 0.0f;
    float max_text_ascent = 0.0f;
    float max_text_descent = 0.0f;
    float max_line_height = 0.0f;  // 最大 CSS line-height
    float max_font_size = 0.0f;    // 最大字体大小

    for (size_t i = 0; i < boxes.size(); ++i) {
        const InlineBox* box = boxes[i];
        if (!box) continue;

        BoxVerticalMetrics box_metrics = GetBoxMetrics(*box);
        const VerticalAlignInfo& align = (i < aligns.size())
            ? aligns[i]
            : VerticalAlignInfo{};

        // 获取盒子的 line-height 和 font-size
        float font_size = 16.0f;  // 默认字体大小
        float line_height_multiplier = box->line_height_multiplier;
        if (box->style) {
            font_size = box->style->font_size;
        }

        // 计算 CSS line-height（font-size * multiplier）
        float css_line_height = font_size * line_height_multiplier;
        max_line_height = std::max(max_line_height, css_line_height);
        max_font_size = std::max(max_font_size, font_size);

        // 根据对齐方式决定是否参与基线计算
        switch (align.type) {
            case VerticalAlignType::BASELINE:
            case VerticalAlignType::SUPER:
            case VerticalAlignType::SUB:
            case VerticalAlignType::LENGTH:
                max_ascent = std::max(max_ascent, box_metrics.ascent);
                max_descent = std::max(max_descent, box_metrics.descent);
                break;

            case VerticalAlignType::TEXT_TOP:
            case VerticalAlignType::TEXT_BOTTOM:
                max_text_ascent = std::max(max_text_ascent, box_metrics.ascent);
                max_text_descent = std::max(max_text_descent, box_metrics.descent);
                break;

            case VerticalAlignType::TOP:
            case VerticalAlignType::BOTTOM:
            case VerticalAlignType::MIDDLE:
                // 这些不参与基线计算，但会影响最终行高
                break;
        }
    }

    // 设置基线相关度量
    metrics.max_ascent = max_ascent;
    metrics.max_descent = max_descent;
    metrics.baseline = max_ascent;
    metrics.text_top = 0.0f;
    metrics.text_bottom = max_text_ascent + max_text_descent;

    // 内容高度（基于基线对齐元素）
    float content_height = max_ascent + max_descent;

    // 应用 CSS line-height：取内容高度和 CSS line-height 的较大值
    // 如果 CSS line-height 更大，则添加半行距（half-leading）
    float base_line_height = std::max(content_height, max_line_height);

    // 如果 line-height 大于内容高度，调整基线位置（添加半行距）
    if (max_line_height > content_height) {
        float half_leading = (max_line_height - content_height) / 2.0f;
        metrics.baseline = max_ascent + half_leading;
    }

    // 第二遍：调整行高以容纳 top/bottom/middle 对齐的盒子
    float line_top = 0.0f;
    float line_bottom = base_line_height;

    for (size_t i = 0; i < boxes.size(); ++i) {
        const InlineBox* box = boxes[i];
        if (!box) continue;

        BoxVerticalMetrics box_metrics = GetBoxMetrics(*box);
        const VerticalAlignInfo& align = (i < aligns.size())
            ? aligns[i]
            : VerticalAlignInfo{};

        float box_top = 0.0f;
        float box_bottom = 0.0f;

        switch (align.type) {
            case VerticalAlignType::TOP:
                box_top = 0.0f;
                box_bottom = box_metrics.height;
                break;

            case VerticalAlignType::BOTTOM:
                box_bottom = base_line_height;
                box_top = base_line_height - box_metrics.height;
                break;

            case VerticalAlignType::MIDDLE:
                // 现代模式：真正居中
                {
                    float center = base_line_height / 2.0f;
                    box_top = center - box_metrics.height / 2.0f;
                    box_bottom = center + box_metrics.height / 2.0f;
                }
                break;

            default:
                continue;
        }

        line_top = std::min(line_top, box_top);
        line_bottom = std::max(line_bottom, box_bottom);
    }

    // 最终行高
    metrics.line_height = line_bottom - line_top;

    // 如果有负的 line_top，调整基线位置
    if (line_top < 0) {
        metrics.baseline = metrics.baseline - line_top;
    }

    return metrics;
}

// ========== 偏移计算 ==========

float VerticalAligner::CalculateBaselineOffset(
    const BoxVerticalMetrics& box_metrics,
    const LineVerticalMetrics& line_metrics
) {
    // 基线对齐：盒子基线对齐到行基线
    return line_metrics.baseline - box_metrics.baseline;
}

float VerticalAligner::CalculateTopOffset(
    const BoxVerticalMetrics& box_metrics,
    const LineVerticalMetrics& line_metrics
) {
    // 顶部对齐：盒子顶部对齐到行顶
    (void)box_metrics;
    (void)line_metrics;
    return 0.0f;
}

float VerticalAligner::CalculateBottomOffset(
    const BoxVerticalMetrics& box_metrics,
    const LineVerticalMetrics& line_metrics
) {
    // 底部对齐：盒子底部对齐到行底
    return line_metrics.line_height - box_metrics.height;
}

float VerticalAligner::CalculateMiddleOffset(
    const BoxVerticalMetrics& box_metrics,
    const LineVerticalMetrics& line_metrics
) {
    // 现代模式：真正的垂直居中
    return (line_metrics.line_height - box_metrics.height) / 2.0f;
}

float VerticalAligner::CalculateBoxYOffset(
    const InlineBox& box,
    const BoxVerticalMetrics& box_metrics,
    const LineVerticalMetrics& line_metrics,
    const VerticalAlignInfo& align
) {
    (void)box; // 当前未使用
    
    switch (align.type) {
        case VerticalAlignType::BASELINE:
            return CalculateBaselineOffset(box_metrics, line_metrics);
            
        case VerticalAlignType::TOP:
            return CalculateTopOffset(box_metrics, line_metrics);
            
        case VerticalAlignType::BOTTOM:
            return CalculateBottomOffset(box_metrics, line_metrics);
            
        case VerticalAlignType::MIDDLE:
            return CalculateMiddleOffset(box_metrics, line_metrics);
            
        case VerticalAlignType::TEXT_TOP:
            return line_metrics.text_top;
            
        case VerticalAlignType::TEXT_BOTTOM:
            return line_metrics.text_bottom - box_metrics.height;
            
        case VerticalAlignType::SUPER:
            // 上标：基线对齐后向上偏移
            return CalculateBaselineOffset(box_metrics, line_metrics) 
                   - box_metrics.height * 0.4f;
            
        case VerticalAlignType::SUB:
            // 下标：基线对齐后向下偏移
            return CalculateBaselineOffset(box_metrics, line_metrics) 
                   + box_metrics.height * 0.2f;
            
        case VerticalAlignType::LENGTH:
            // 长度偏移：基线对齐后加上指定偏移
            return CalculateBaselineOffset(box_metrics, line_metrics) 
                   - align.length_value;
            
        default:
            return 0.0f;
    }
}

// ========== 对齐应用 ==========

void VerticalAligner::AlignBoxes(
    std::vector<InlineBox*>& boxes,
    const std::vector<VerticalAlignInfo>& aligns,
    float line_y
) {
    // 计算行度量
    LineVerticalMetrics line_metrics = CalculateLineMetrics(boxes, aligns);
    
    // 应用到每个盒子
    for (size_t i = 0; i < boxes.size(); ++i) {
        InlineBox* box = boxes[i];
        if (!box) continue;
        
        BoxVerticalMetrics box_metrics = GetBoxMetrics(*box);
        const VerticalAlignInfo& align = (i < aligns.size()) 
            ? aligns[i] 
            : VerticalAlignInfo{};
        
        float y_offset = CalculateBoxYOffset(*box, box_metrics, line_metrics, align);
        box->y = line_y + y_offset;
    }
}

// ========== CSS 解析 ==========

VerticalAlignInfo VerticalAligner::ParseVerticalAlign(
    const std::string& value,
    float font_size
) {
    VerticalAlignInfo info;
    
    if (value.empty() || value == "baseline") {
        info.type = VerticalAlignType::BASELINE;
    } else if (value == "top") {
        info.type = VerticalAlignType::TOP;
    } else if (value == "bottom") {
        info.type = VerticalAlignType::BOTTOM;
    } else if (value == "middle") {
        info.type = VerticalAlignType::MIDDLE;
    } else if (value == "text-top") {
        info.type = VerticalAlignType::TEXT_TOP;
    } else if (value == "text-bottom") {
        info.type = VerticalAlignType::TEXT_BOTTOM;
    } else if (value == "super") {
        info.type = VerticalAlignType::SUPER;
    } else if (value == "sub") {
        info.type = VerticalAlignType::SUB;
    } else {
        // 尝试解析长度值
        info.type = VerticalAlignType::LENGTH;
        
        // 简单解析：支持 px 和 %
        size_t pos = 0;
        try {
            float num = std::stof(value, &pos);
            std::string unit = value.substr(pos);
            
            if (unit == "%" || unit.empty()) {
                info.length_value = num * font_size / 100.0f;
            } else if (unit == "px") {
                info.length_value = num;
            } else if (unit == "em") {
                info.length_value = num * font_size;
            } else {
                // 未知单位，当作 px
                info.length_value = num;
            }
        } catch (...) {
            // 解析失败，回退到基线对齐
            info.type = VerticalAlignType::BASELINE;
        }
    }
    
    return info;
}

} // namespace lightui

