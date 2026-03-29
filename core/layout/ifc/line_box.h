/**
 * @file line_box.h
 * @brief 行盒结构定义
 * 
 * LineBox 表示内联格式化上下文中的一行。
 * 包含该行的所有内联盒、位置信息和对齐方法。
 * 
 * MBink 现代模式：
 * - 无 strut：行高完全由内容决定
 * - vertical-align: middle 真正居中
 */

#pragma once

#include <vector>
#include <string>
#include "inline_box.h"

namespace mbink {

/**
 * @brief 垂直对齐方式
 */
enum class VerticalAlign {
    BASELINE,       ///< 基线对齐（默认）
    TOP,            ///< 顶部对齐
    MIDDLE,         ///< 中线对齐（MBink: 真正居中）
    BOTTOM,         ///< 底部对齐
    TEXT_TOP,       ///< 文本顶部对齐
    TEXT_BOTTOM,    ///< 文本底部对齐
    SUPER,          ///< 上标
    SUB,            ///< 下标
    LENGTH,         ///< 相对基线偏移（像素）
    PERCENT         ///< 相对 line-height 偏移（百分比）
};

/**
 * @brief 行盒结构
 * 
 * 表示 IFC 中的一行，包含：
 * - 位置和尺寸信息
 * - 该行包含的所有内联盒
 * - 对齐和布局方法
 */
struct LineBox {
    // ========== 位置和尺寸 ==========
    
    /** @brief 行盒 X 坐标（相对于 IFC 容器） */
    float x = 0.0f;
    
    /** @brief 行盒 Y 坐标（相对于 IFC 容器） */
    float y = 0.0f;
    
    /** @brief 行盒宽度 */
    float width = 0.0f;
    
    /** @brief 行盒高度（由内容决定，无 strut） */
    float height = 0.0f;
    
    /** @brief 行盒基线位置（距离顶部） */
    float baseline = 0.0f;
    
    // ========== 内容 ==========
    
    /** @brief 此行包含的内联盒（非拥有指针） */
    std::vector<InlineBox*> boxes;
    
    // ========== 布局信息 ==========
    
    /** @brief 可用宽度（容器宽度） */
    float available_width = 0.0f;
    
    /** @brief 实际内容宽度 */
    float content_width = 0.0f;
    
    /** @brief 是否是第一行（用于 text-indent） */
    bool is_first_line = false;
    
    /** @brief 是否是最后一行（用于 text-align-last） */
    bool is_last_line = false;
    
    // ========== 构造函数 ==========
    
    /** @brief 默认构造函数 */
    LineBox() = default;
    
    /**
     * @brief 带可用宽度的构造函数
     * @param avail_width 可用宽度
     */
    explicit LineBox(float avail_width) : available_width(avail_width) {}
    
    // ========== 布局方法 ==========
    
    /**
     * @brief 添加内联盒到此行
     * @param box 要添加的内联盒（非拥有）
     */
    void AddBox(InlineBox* box);
    
    /**
     * @brief 计算行高（现代模式：由内容决定，无 strut）
     * 
     * 遍历所有盒子，计算最大 ascent 和 descent，
     * 行高 = max_ascent + max_descent
     */
    void CalculateHeight();
    
    /**
     * @brief 对齐内联盒（垂直对齐）
     * 
     * 根据每个盒子的 vertical-align 属性计算其 Y 偏移。
     * MBink 现代模式：middle 表示真正的垂直居中。
     */
    void AlignBoxes();
    
    /**
     * @brief 应用水平对齐（text-align）
     * @param align 对齐方式: "left", "right", "center", "justify"
     */
    void ApplyTextAlign(const std::string& align);
    
    /**
     * @brief 检查是否可以容纳指定宽度的内容
     * @param box_width 要添加的内容宽度
     * @return 如果可以容纳则返回 true
     */
    bool CanFit(float box_width) const {
        return content_width + box_width <= available_width;
    }
    
    /**
     * @brief 获取剩余可用宽度
     * @return 剩余宽度
     */
    float GetRemainingWidth() const {
        return available_width - content_width;
    }
    
    /**
     * @brief 检查行是否为空
     * @return 如果没有内联盒则返回 true
     */
    bool IsEmpty() const { return boxes.empty(); }
    
private:
    /**
     * @brief 应用垂直对齐到单个盒子
     * @param box 要对齐的盒子
     * @param align 对齐方式
     * @param offset 偏移值（用于 LENGTH 和 PERCENT 类型）
     */
    void ApplyVerticalAlign(InlineBox* box, VerticalAlign align, float offset = 0.0f);
    
    /**
     * @brief 分配额外空间（用于 justify 对齐）
     * @param extra_space 要分配的额外空间
     */
    void DistributeSpace(float extra_space);
};

} // namespace mbink

