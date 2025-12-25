/**
 * @file vertical_aligner.cpp
 * @brief 垂直对齐器实现
 */

#include "vertical_aligner.h"
#include "core/render/render_object.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// 调试开关
#define VA_DEBUG 0

namespace lightui {

// ========== 盒子度量 ==========

BoxVerticalMetrics VerticalAligner::GetBoxMetrics(const InlineBox& box) {
    BoxVerticalMetrics metrics;

    metrics.height = box.height;

    if (box.IsText()) {
        // 文本盒子：使用 Skia 测量的精确 ascent 和 descent
        float skia_ascent = box.skia_ascent;
        float skia_descent = box.skia_descent;

        if (skia_ascent <= 0 && skia_descent <= 0) {
            // 回退到估算值
            skia_ascent = box.height * 0.8f;
            skia_descent = box.height * 0.2f;
        }

        // 计算内容高度和 half-leading
        float content_height = skia_ascent + skia_descent;
        if (box.height > content_height) {
            // 有 line-height 导致的额外空间，分配 half-leading
            float half_leading = (box.height - content_height) / 2.0f;
            metrics.ascent = skia_ascent + half_leading;
            metrics.descent = skia_descent + half_leading;
        } else if (box.height < content_height) {
            // line-height 小于 Skia 测量的高度，按比例缩放
            // 这确保 ascent + descent = box.height
            float scale = box.height / content_height;
            metrics.ascent = skia_ascent * scale;
            metrics.descent = skia_descent * scale;
        } else {
            metrics.ascent = skia_ascent;
            metrics.descent = skia_descent;
        }
        metrics.baseline = metrics.ascent;
    } else if (box.IsAtomic()) {
        // 原子盒子（inline-block, 图片等）
        // 基线在底部（现代模式简化处理）
        // ✅ FIX: Include vertical margin in the metrics
        // For inline-block elements, vertical margin should affect line box height
        metrics.ascent = box.margin_top + box.height;
        metrics.descent = box.margin_bottom;
        metrics.baseline = box.margin_top + box.height;
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
    const std::vector<VerticalAlignInfo>& aligns,
    float container_line_height
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

    // CSS 规范：每行都有一个隐式的 "strut"（零宽度字符）
    // strut 的高度等于容器的 line-height，参与基线对齐
    // 这确保了即使行内只有 inline-block 元素，行高也至少为容器的 line-height
    //
    // 注意：container_line_height 现在使用浏览器风格的 line-height: normal
    // 而不是默认的 1.2 倍数，所以可以安全地使用它作为最小行高
    if (container_line_height > 0) {
        max_line_height = container_line_height;
    }

    for (size_t i = 0; i < boxes.size(); ++i) {
        const InlineBox* box = boxes[i];
        if (!box) continue;

        BoxVerticalMetrics box_metrics = GetBoxMetrics(*box);
        const VerticalAlignInfo& align = (i < aligns.size())
            ? aligns[i]
            : VerticalAlignInfo{};

        // 获取盒子的 line-height（使用盒子自身的高度，已包含 line-height 计算）
        // MeasureTextForIFC 已经根据 line-height: normal 或指定值计算了正确的高度
        float box_line_height = box->height;
        max_line_height = std::max(max_line_height, box_line_height);

        float font_size = 16.0f;
        if (box->style) {
            font_size = box->style->font_size;
        }
        max_font_size = std::max(max_font_size, font_size);

        // 根据对齐方式决定是否参与基线计算
        switch (align.type) {
            case VerticalAlignType::BASELINE:
            case VerticalAlignType::LENGTH:
                max_ascent = std::max(max_ascent, box_metrics.ascent);
                max_descent = std::max(max_descent, box_metrics.descent);
                break;

            case VerticalAlignType::SUPER:
                // 上标会向上偏移 0.4 * height，需要增加 ascent
                // 上标偏移后：新的顶部 = 原顶部 - 0.4 * height
                // 新的 ascent = 原 ascent + 0.4 * height
                {
                    float super_offset = box_metrics.height * 0.4f;
                    max_ascent = std::max(max_ascent, box_metrics.ascent + super_offset);
                    max_descent = std::max(max_descent, box_metrics.descent - super_offset);
                    // 确保 descent 不会变成负数
                    if (max_descent < 0.0f) max_descent = 0.0f;
                }
                break;

            case VerticalAlignType::SUB:
                // 下标会向下偏移 0.2 * height，需要增加 descent
                // 下标偏移后：新的底部 = 原底部 + 0.2 * height
                // 新的 descent = 原 descent + 0.2 * height
                {
                    float sub_offset = box_metrics.height * 0.2f;
                    max_ascent = std::max(max_ascent, box_metrics.ascent - sub_offset);
                    max_descent = std::max(max_descent, box_metrics.descent + sub_offset);
                    // 确保 ascent 不会变成负数
                    if (max_ascent < 0.0f) max_ascent = 0.0f;
                }
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

#if VA_DEBUG
    std::cout << "[VA] CalculateLineMetrics: max_ascent=" << max_ascent
              << ", max_descent=" << max_descent
              << ", max_line_height=" << max_line_height
              << ", content_height=" << content_height
              << ", line_height=" << metrics.line_height << std::endl;
#endif

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
    float line_y,
    float container_line_height
) {
    // 计算行度量，传入容器的 line-height
    LineVerticalMetrics line_metrics = CalculateLineMetrics(boxes, aligns, container_line_height);

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

#if VA_DEBUG
        if (box->IsAtomic()) {
            std::cout << "[VA] AlignBoxes ATOMIC: align_type=" << static_cast<int>(align.type)
                      << ", box_height=" << box_metrics.height
                      << ", line_height=" << line_metrics.line_height
                      << ", y_offset=" << y_offset
                      << ", final_y=" << box->y << std::endl;
        }
#endif
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

