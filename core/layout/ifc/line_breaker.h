/**
 * @file line_breaker.h
 * @brief 断行器定义
 * 
 * LineBreaker 负责在内联内容中查找断行点，并将内容分割成多行。
 * 支持 Unicode 断行算法（简化版）和 CSS white-space、word-break 等属性。
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "inline_box.h"
#include "line_box.h"

namespace mbink {

/**
 * @brief 断行类型
 */
enum class BreakType {
    /** @brief 正常断行点（空格、标点后） */
    NORMAL,
    
    /** @brief 强制断行（word-break: break-all） */
    WORD_BREAK,
    
    /** @brief 溢出换行（overflow-wrap: break-word） */
    OVERFLOW_WRAP,
    
    /** @brief 强制换行（<br> 或 \n） */
    FORCED
};

/**
 * @brief 空白处理模式（white-space 属性）
 */
enum class WhiteSpaceMode {
    /** @brief normal: 合并空白，允许换行 */
    NORMAL,
    
    /** @brief nowrap: 合并空白，禁止换行 */
    NOWRAP,
    
    /** @brief pre: 保留空白，禁止换行（除了 \n） */
    PRE,
    
    /** @brief pre-wrap: 保留空白，允许换行 */
    PRE_WRAP,
    
    /** @brief pre-line: 合并空白，保留换行符 */
    PRE_LINE
};

/**
 * @brief 断词模式（word-break 属性）
 */
enum class WordBreakMode {
    /** @brief normal: 正常断词规则 */
    NORMAL,
    
    /** @brief break-all: 可在任意字符间断行 */
    BREAK_ALL,
    
    /** @brief keep-all: CJK 文本不断行 */
    KEEP_ALL
};

/**
 * @brief 溢出换行模式（overflow-wrap 属性）
 */
enum class OverflowWrapMode {
    /** @brief normal: 只在允许的断行点换行 */
    NORMAL,
    
    /** @brief break-word: 单词溢出时可断开 */
    BREAK_WORD,
    
    /** @brief anywhere: 任意位置都可断行 */
    ANYWHERE
};

/**
 * @brief 断行机会
 */
struct BreakOpportunity {
    /** @brief 断行位置（在 inline_boxes 中的索引） */
    size_t box_index = 0;
    
    /** @brief 文本内断行位置（字节偏移，仅对 TEXT 类型有效） */
    size_t text_offset = 0;
    
    /** @brief 断行前的累计宽度 */
    float width_before = 0.0f;
    
    /** @brief 断行类型 */
    BreakType type = BreakType::NORMAL;
};

/**
 * @brief 断行器
 * 
 * 负责在内联内容中查找断行点，并根据可用宽度将内容分割成多行。
 * 
 * 使用方式：
 * ```cpp
 * LineBreaker breaker;
 * breaker.SetWhiteSpace(WhiteSpaceMode::NORMAL);
 * auto lines = breaker.BreakIntoLines(boxes, available_width);
 * ```
 */
class LineBreaker {
public:
    /** @brief 默认构造函数 */
    LineBreaker() = default;
    
    // ========== 配置 ==========
    
    /** @brief 设置空白处理模式 */
    void SetWhiteSpace(WhiteSpaceMode mode) { white_space_ = mode; }
    
    /** @brief 设置断词模式 */
    void SetWordBreak(WordBreakMode mode) { word_break_ = mode; }
    
    /** @brief 设置溢出换行模式 */
    void SetOverflowWrap(OverflowWrapMode mode) { overflow_wrap_ = mode; }

    /** @brief 设置首行缩进 */
    void SetTextIndent(float indent) { text_indent_ = indent; }

    /** @brief 设置字符间距 */
    void SetLetterSpacing(float spacing) { letter_spacing_ = spacing; }

    /** @brief 设置单词间距 */
    void SetWordSpacing(float spacing) { word_spacing_ = spacing; }

    // ========== 主要方法 ==========
    
    /**
     * @brief 查找所有断行机会
     * @param boxes 内联盒列表
     * @param available_width 可用宽度
     * @return 断行机会列表
     */
    std::vector<BreakOpportunity> FindBreakOpportunities(
        const std::vector<InlineBox>& boxes,
        float available_width
    );
    
    /**
     * @brief 执行断行，将内联盒分配到行盒中
     * @param boxes 内联盒列表
     * @param available_width 可用宽度
     * @return 行盒列表
     */
    std::vector<LineBox> BreakIntoLines(
        std::vector<InlineBox>& boxes,
        float available_width
    );
    
    /**
     * @brief 处理空白（根据 white-space 属性）
     * @param text 原始文本
     * @return 处理后的文本
     */
    std::string ProcessWhitespace(const std::string& text);
    
private:
    // ========== 配置 ==========
    WhiteSpaceMode white_space_ = WhiteSpaceMode::NORMAL;
    WordBreakMode word_break_ = WordBreakMode::NORMAL;
    OverflowWrapMode overflow_wrap_ = OverflowWrapMode::NORMAL;

    // 文本间距配置
    float text_indent_ = 0.0f;      // 首行缩进
    float letter_spacing_ = 0.0f;   // 字符间距
    float word_spacing_ = 0.0f;     // 单词间距

    // ========== Unicode 断行辅助 ==========
    
    /**
     * @brief 检查两个字符之间是否可以断行
     * @param prev_char 前一个字符（Unicode 码点）
     * @param next_char 后一个字符（Unicode 码点）
     * @return 如果可以断行则返回 true
     */
    bool CanBreakBetween(uint32_t prev_char, uint32_t next_char);
    
    /**
     * @brief 检查是否是可断行的空白
     * @param ch Unicode 码点
     * @return 如果是可断行空白则返回 true
     */
    bool IsBreakableWhitespace(uint32_t ch);
    
    /**
     * @brief 检查是否是 CJK 字符
     * @param ch Unicode 码点
     * @return 如果是 CJK 字符则返回 true
     */
    bool IsCJK(uint32_t ch);
    
    /**
     * @brief 检查是否是行首禁止字符（如中文句号）
     * @param ch Unicode 码点
     * @return 如果是行首禁止字符则返回 true
     */
    bool IsLineStartProhibited(uint32_t ch);
    
    /**
     * @brief 检查是否是行尾禁止字符（如左括号）
     * @param ch Unicode 码点
     * @return 如果是行尾禁止字符则返回 true
     */
    bool IsLineEndProhibited(uint32_t ch);
    
    /**
     * @brief 解码 UTF-8 字符
     * @param str 字符串
     * @param pos 当前位置（会被更新）
     * @return Unicode 码点
     */
    uint32_t DecodeUTF8(const std::string& str, size_t& pos);
};

} // namespace mbink

