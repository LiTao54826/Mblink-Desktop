/**
 * @file vertical_aligner.h
 * @brief 垂直对齐器定义
 * 
 * VerticalAligner 负责处理行内元素的垂直对齐。
 * 采用现代模式设计，vertical-align: middle 表示真正居中。
 */

#pragma once

#include <vector>
#include <cstdint>
#include "inline_box.h"

namespace lightui {

/**
 * @brief 垂直对齐方式（现代模式）
 * 
 * 与浏览器行为的主要差异：
 * - MIDDLE: 真正的垂直居中（不是 x-height 中点）
 * - 无 strut 概念，行高完全由内容决定
 */
enum class VerticalAlignType {
    /** @brief 基线对齐（默认） */
    BASELINE,
    
    /** @brief 顶部对齐（与行顶对齐） */
    TOP,
    
    /** @brief 底部对齐（与行底对齐） */
    BOTTOM,
    
    /** @brief 真正的垂直居中（现代模式） */
    MIDDLE,
    
    /** @brief 文字顶部 */
    TEXT_TOP,
    
    /** @brief 文字底部 */
    TEXT_BOTTOM,
    
    /** @brief 上标 */
    SUPER,
    
    /** @brief 下标 */
    SUB,
    
    /** @brief 长度值偏移 */
    LENGTH
};

/**
 * @brief 垂直对齐信息
 */
struct VerticalAlignInfo {
    /** @brief 对齐类型 */
    VerticalAlignType type = VerticalAlignType::BASELINE;
    
    /** @brief 长度值（仅当 type == LENGTH 时有效） */
    float length_value = 0.0f;
};

/**
 * @brief 行内盒的垂直度量信息
 */
struct BoxVerticalMetrics {
    /** @brief 盒子高度 */
    float height = 0.0f;
    
    /** @brief 基线位置（从盒子顶部算起） */
    float baseline = 0.0f;
    
    /** @brief 上升部分（基线以上） */
    float ascent = 0.0f;
    
    /** @brief 下降部分（基线以下） */
    float descent = 0.0f;
};

/**
 * @brief 行的垂直度量信息
 */
struct LineVerticalMetrics {
    /** @brief 行高 */
    float line_height = 0.0f;
    
    /** @brief 基线位置（从行顶算起） */
    float baseline = 0.0f;
    
    /** @brief 最大上升部分 */
    float max_ascent = 0.0f;
    
    /** @brief 最大下降部分 */
    float max_descent = 0.0f;
    
    /** @brief 文字区域顶部（用于 text-top 对齐） */
    float text_top = 0.0f;
    
    /** @brief 文字区域底部（用于 text-bottom 对齐） */
    float text_bottom = 0.0f;
};

/**
 * @brief 垂直对齐器
 * 
 * 负责计算行内元素的垂直位置。采用两阶段算法：
 * 1. 计算行的垂直度量（基线、行高等）
 * 2. 根据每个盒子的 vertical-align 计算其 Y 位置
 * 
 * 现代模式特点：
 * - 无 strut，行高由实际内容决定
 * - middle 是真正的垂直居中
 */
class VerticalAligner {
public:
    /** @brief 默认构造函数 */
    VerticalAligner() = default;
    
    /**
     * @brief 计算行的垂直度量
     * @param boxes 行内的盒子列表
     * @param aligns 每个盒子的对齐信息
     * @param container_line_height 容器指定的 line-height（可选，0 表示自动）
     * @return 行的垂直度量
     */
    LineVerticalMetrics CalculateLineMetrics(
        const std::vector<InlineBox*>& boxes,
        const std::vector<VerticalAlignInfo>& aligns,
        float container_line_height = 0.0f
    );
    
    /**
     * @brief 计算盒子的 Y 偏移量
     * @param box 盒子
     * @param box_metrics 盒子的垂直度量
     * @param line_metrics 行的垂直度量
     * @param align 对齐信息
     * @return Y 偏移量（相对于行顶）
     */
    float CalculateBoxYOffset(
        const InlineBox& box,
        const BoxVerticalMetrics& box_metrics,
        const LineVerticalMetrics& line_metrics,
        const VerticalAlignInfo& align
    );
    
    /**
     * @brief 对齐行内所有盒子
     * @param boxes 盒子列表（会修改 y 值）
     * @param aligns 对齐信息列表
     * @param line_y 行的 Y 位置
     * @param container_line_height 容器指定的 line-height（可选，0 表示自动）
     */
    void AlignBoxes(
        std::vector<InlineBox*>& boxes,
        const std::vector<VerticalAlignInfo>& aligns,
        float line_y,
        float container_line_height = 0.0f
    );
    
    /**
     * @brief 获取盒子的垂直度量
     * @param box 盒子
     * @return 垂直度量
     */
    BoxVerticalMetrics GetBoxMetrics(const InlineBox& box);
    
    /**
     * @brief 从 CSS 值解析垂直对齐信息
     * @param value CSS vertical-align 值
     * @param font_size 当前字体大小（用于百分比计算）
     * @return 对齐信息
     */
    static VerticalAlignInfo ParseVerticalAlign(
        const std::string& value,
        float font_size
    );
    
private:
    /**
     * @brief 计算基线对齐偏移
     * @param box_metrics 盒子度量
     * @param line_metrics 行度量
     * @return Y 偏移量
     */
    float CalculateBaselineOffset(
        const BoxVerticalMetrics& box_metrics,
        const LineVerticalMetrics& line_metrics
    );
    
    /**
     * @brief 计算顶部对齐偏移
     * @param box_metrics 盒子度量
     * @param line_metrics 行度量
     * @return Y 偏移量
     */
    float CalculateTopOffset(
        const BoxVerticalMetrics& box_metrics,
        const LineVerticalMetrics& line_metrics
    );
    
    /**
     * @brief 计算底部对齐偏移
     * @param box_metrics 盒子度量
     * @param line_metrics 行度量
     * @return Y 偏移量
     */
    float CalculateBottomOffset(
        const BoxVerticalMetrics& box_metrics,
        const LineVerticalMetrics& line_metrics
    );
    
    /**
     * @brief 计算居中对齐偏移（现代模式：真正居中）
     * @param box_metrics 盒子度量
     * @param line_metrics 行度量
     * @return Y 偏移量
     */
    float CalculateMiddleOffset(
        const BoxVerticalMetrics& box_metrics,
        const LineVerticalMetrics& line_metrics
    );
};

} // namespace lightui

