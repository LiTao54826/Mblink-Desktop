/**
 * @file text_run.h
 * @brief 文本片段结构定义
 * 
 * TextRun 表示文本流中的一个片段，是 IFC (Inline Formatting Context) 的基本单元。
 * 每个 TextRun 包含文本内容、渲染尺寸和断行信息。
 */

#pragma once

#include <string>
#include <cstddef>

// Forward declarations
namespace mbink {
class RenderText;
struct ComputedStyle;
}

namespace mbink {

/**
 * @brief 文本片段结构
 * 
 * 表示一个连续的文本片段，包含以下信息：
 * - 文本内容及其在原始文本中的位置
 * - 渲染尺寸（宽度、高度、基线）
 * - 来源信息（关联的 RenderText 和样式）
 * - 换行信息（断行点）
 */
struct TextRun {
    // ========== 文本内容 ==========
    
    /** @brief 文本内容 */
    std::string text;
    
    /** @brief 在原始文本中的起始位置（字节偏移） */
    size_t start_offset = 0;
    
    /** @brief 在原始文本中的结束位置（字节偏移） */
    size_t end_offset = 0;
    
    // ========== 渲染尺寸 ==========
    
    /** @brief 渲染宽度（像素） */
    float width = 0.0f;
    
    /** @brief 高度（由 line-height 决定） */
    float height = 0.0f;
    
    /** @brief 基线位置（距离顶部的像素） */
    float baseline = 0.0f;
    
    // ========== 来源信息 ==========
    
    /** @brief 关联的 RenderText 对象（非拥有指针） */
    RenderText* render_text = nullptr;
    
    /** @brief 样式引用（非拥有指针） */
    const ComputedStyle* style = nullptr;
    
    // ========== 换行信息 ==========
    
    /** @brief 此片段前可以换行 */
    bool can_break_before = false;
    
    /** @brief 此片段后可以换行 */
    bool can_break_after = false;
    
    /** @brief 是否是空白字符 */
    bool is_whitespace = false;
    
    /** @brief 是否是强制换行符（\n 或 <br>） */
    bool is_forced_break = false;
    
    // ========== 构造函数 ==========
    
    /** @brief 默认构造函数 */
    TextRun() = default;
    
    /** 
     * @brief 带文本的构造函数
     * @param txt 文本内容
     */
    explicit TextRun(const std::string& txt)
        : text(txt)
        , start_offset(0)
        , end_offset(txt.size()) {}
    
    /**
     * @brief 完整构造函数
     * @param txt 文本内容
     * @param start 起始偏移
     * @param end 结束偏移
     */
    TextRun(const std::string& txt, size_t start, size_t end)
        : text(txt)
        , start_offset(start)
        , end_offset(end) {}
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 检查是否为空
     * @return 如果文本为空则返回 true
     */
    bool IsEmpty() const { return text.empty(); }
    
    /**
     * @brief 获取文本长度（字节数）
     * @return 文本字节长度
     */
    size_t Length() const { return text.size(); }
    
    /**
     * @brief 获取文本的 UTF-8 字符数
     * @return UTF-8 字符数
     */
    size_t CharacterCount() const;
    
    /**
     * @brief 检查是否只包含空白字符
     * @return 如果只包含空白则返回 true
     */
    bool IsOnlyWhitespace() const;
    
    /**
     * @brief 检查是否可以在指定位置断行
     * @param byte_offset 字节偏移位置
     * @return 如果可以断行则返回 true
     */
    bool CanBreakAt(size_t byte_offset) const;
};

} // namespace mbink

