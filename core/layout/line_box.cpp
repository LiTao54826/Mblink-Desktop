/**
 * @file line_box.cpp
 * @brief LineBox 结构的实现
 * 
 * MBink 现代模式实现：
 * - 无 strut：行高完全由内容决定
 * - vertical-align: middle 真正居中
 */

#include "line_box.h"
#include <algorithm>
#include <cmath>

namespace lightui {

void LineBox::AddBox(InlineBox* box) {
    if (!box) return;
    
    boxes.push_back(box);
    content_width += box->GetTotalWidth();
}

void LineBox::CalculateHeight() {
    if (boxes.empty()) {
        height = 0;
        baseline = 0;
        return;
    }
    
    // MBink 现代模式：行高完全由内容决定，无 strut
    // 计算最大 ascent（基线以上）和 descent（基线以下）
    float max_ascent = 0.0f;
    float max_descent = 0.0f;
    
    for (auto* box : boxes) {
        if (!box) continue;
        
        // ascent = 基线位置（距离盒子顶部）
        float ascent = box->baseline;
        // descent = 盒子高度 - 基线位置
        float descent = box->height - box->baseline;
        
        max_ascent = std::max(max_ascent, ascent);
        max_descent = std::max(max_descent, descent);
    }
    
    height = max_ascent + max_descent;
    baseline = max_ascent;
}

void LineBox::AlignBoxes() {
    if (boxes.empty()) return;
    
    // 首先计算行高
    CalculateHeight();
    
    // 然后对齐每个盒子
    float current_x = x;
    
    for (auto* box : boxes) {
        if (!box) continue;
        
        // 水平位置
        box->x = current_x + box->GetLeftSpace();
        current_x += box->GetTotalWidth();
        
        // 垂直对齐 - 默认基线对齐
        // TODO: 从样式中读取 vertical-align
        VerticalAlign align = VerticalAlign::BASELINE;
        ApplyVerticalAlign(box, align);
    }
}

void LineBox::ApplyVerticalAlign(InlineBox* box, VerticalAlign align, float offset) {
    if (!box) return;
    
    switch (align) {
        case VerticalAlign::BASELINE:
            // 基线对齐：盒子的基线与行盒的基线对齐
            box->y = y + (baseline - box->baseline);
            break;
            
        case VerticalAlign::TOP:
            // 顶部对齐：盒子顶部与行盒顶部对齐
            box->y = y;
            break;
            
        case VerticalAlign::BOTTOM:
            // 底部对齐：盒子底部与行盒底部对齐
            box->y = y + height - box->height;
            break;
            
        case VerticalAlign::MIDDLE:
            // MBink 现代模式：真正的垂直居中
            // 盒子中心与行盒中心对齐
            box->y = y + (height - box->height) / 2.0f;
            break;
            
        case VerticalAlign::TEXT_TOP:
            // 文本顶部对齐（简化实现：与顶部对齐相同）
            box->y = y;
            break;
            
        case VerticalAlign::TEXT_BOTTOM:
            // 文本底部对齐（简化实现：与底部对齐相同）
            box->y = y + height - box->height;
            break;
            
        case VerticalAlign::SUPER:
            // 上标：基线对齐后向上偏移
            box->y = y + (baseline - box->baseline) - box->height * 0.4f;
            break;
            
        case VerticalAlign::SUB:
            // 下标：基线对齐后向下偏移
            box->y = y + (baseline - box->baseline) + box->height * 0.2f;
            break;
            
        case VerticalAlign::LENGTH:
            // 相对基线偏移（像素）
            box->y = y + (baseline - box->baseline) - offset;
            break;
            
        case VerticalAlign::PERCENT:
            // 相对 line-height 偏移（百分比）
            box->y = y + (baseline - box->baseline) - (height * offset / 100.0f);
            break;
    }
}

void LineBox::ApplyTextAlign(const std::string& align) {
    float extra_space = available_width - content_width;
    if (extra_space <= 0) return;
    
    if (align == "left" || align == "start") {
        // 左对齐：默认，无需调整
        return;
    }
    
    if (align == "right" || align == "end") {
        // 右对齐：所有盒子右移
        for (auto* box : boxes) {
            if (box) box->x += extra_space;
        }
        return;
    }
    
    if (align == "center") {
        // 居中对齐：所有盒子右移一半额外空间
        float offset = extra_space / 2.0f;
        for (auto* box : boxes) {
            if (box) box->x += offset;
        }
        return;
    }
    
    if (align == "justify") {
        // 两端对齐：分配额外空间到单词之间
        // 最后一行不做两端对齐
        if (!is_last_line) {
            DistributeSpace(extra_space);
        }
        return;
    }
}

void LineBox::DistributeSpace(float extra_space) {
    if (boxes.size() < 2) return;
    
    // 计算可分配的间隙数量（空白盒子的数量）
    int gap_count = 0;
    for (auto* box : boxes) {
        if (box && box->IsText()) {
            for (const auto& run : box->text_runs) {
                if (run.is_whitespace) {
                    gap_count++;
                }
            }
        }
    }
    
    if (gap_count == 0) {
        // 如果没有空白，则在盒子之间均匀分配
        gap_count = static_cast<int>(boxes.size()) - 1;
        if (gap_count <= 0) return;
        
        float gap_space = extra_space / gap_count;
        float accumulated_offset = 0.0f;
        
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i]) {
                boxes[i]->x += accumulated_offset;
                if (i < boxes.size() - 1) {
                    accumulated_offset += gap_space;
                }
            }
        }
    } else {
        // 在空白处分配额外空间
        float space_per_gap = extra_space / gap_count;
        float accumulated_offset = 0.0f;
        
        for (auto* box : boxes) {
            if (!box) continue;
            
            box->x += accumulated_offset;
            
            if (box->IsText()) {
                for (auto& run : box->text_runs) {
                    if (run.is_whitespace) {
                        run.width += space_per_gap;
                        accumulated_offset += space_per_gap;
                    }
                }
            }
        }
    }
}

} // namespace lightui

