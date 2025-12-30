/**
 * @file ansi_parser.h
 * @brief ANSI 转义序列解析器
 *
 * 实现 ANSI/VT100 转义序列的状态机解析，支持颜色、样式和光标控制。
 */

#pragma once

#include "core/dom/elements/virtual_text/text_style.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace lightui {

/**
 * @brief 解析器状态
 */
enum class ParserState {
    Ground,          ///< 普通文本
    Escape,          ///< 收到 ESC (0x1B)
    CsiEntry,        ///< 收到 ESC [
    CsiParam,        ///< CSI 参数
    CsiIntermediate, ///< CSI 中间字节
    OscString,       ///< OSC 字符串 (ESC ])
    DcsEntry,        ///< DCS 入口 (ESC P)
    DcsString,       ///< DCS 字符串
};

/**
 * @brief 清除模式
 */
enum class ClearMode {
    ToEnd = 0,    ///< 从光标到末尾
    ToStart = 1,  ///< 从开头到光标
    All = 2,      ///< 全部
    Scrollback = 3 ///< 包括回滚缓冲区
};

/**
 * @brief 终端单元格
 */
struct Cell {
    char32_t codepoint = ' ';  ///< Unicode 码点
    TextStyle style;           ///< 样式
    uint8_t width = 1;         ///< 字符宽度 (1 或 2)

    Cell() = default;
    Cell(char32_t cp, const TextStyle& s) : codepoint(cp), style(s), width(1) {}
    Cell(char32_t cp, const TextStyle& s, uint8_t w) : codepoint(cp), style(s), width(w) {}
    
    /**
     * @brief 判断字符是否为宽字符（占用两个单元格）
     * @param cp Unicode 码点
     * @return true 如果是宽字符
     */
    static bool IsWideChar(char32_t cp) {
        // CJK 字符范围（简化版本）
        // CJK Unified Ideographs: U+4E00 - U+9FFF
        // CJK Unified Ideographs Extension A: U+3400 - U+4DBF
        // CJK Compatibility Ideographs: U+F900 - U+FAFF
        // Hangul Syllables: U+AC00 - U+D7AF
        // Fullwidth Forms: U+FF00 - U+FFEF
        // CJK Symbols and Punctuation: U+3000 - U+303F
        // Hiragana: U+3040 - U+309F
        // Katakana: U+30A0 - U+30FF
        // Bopomofo: U+3100 - U+312F
        if (cp >= 0x3000 && cp <= 0x303F) return true;  // CJK 符号和标点
        if (cp >= 0x3040 && cp <= 0x309F) return true;  // 平假名
        if (cp >= 0x30A0 && cp <= 0x30FF) return true;  // 片假名
        if (cp >= 0x3100 && cp <= 0x312F) return true;  // 注音符号
        if (cp >= 0x3400 && cp <= 0x4DBF) return true;  // CJK 扩展 A
        if (cp >= 0x4E00 && cp <= 0x9FFF) return true;  // CJK 统一汉字
        if (cp >= 0xAC00 && cp <= 0xD7AF) return true;  // 韩文音节
        if (cp >= 0xF900 && cp <= 0xFAFF) return true;  // CJK 兼容汉字
        if (cp >= 0xFF00 && cp <= 0xFFEF) return true;  // 全角形式
        if (cp >= 0x20000 && cp <= 0x2A6DF) return true; // CJK 扩展 B
        if (cp >= 0x2A700 && cp <= 0x2B73F) return true; // CJK 扩展 C
        if (cp >= 0x2B740 && cp <= 0x2B81F) return true; // CJK 扩展 D
        return false;
    }
};

/**
 * @brief ANSI 转义序列解析器
 *
 * 使用状态机解析 ANSI 转义序列，支持：
 * - SGR (Select Graphic Rendition): 颜色和样式
 * - CSI (Control Sequence Introducer): 光标控制
 * - OSC (Operating System Command): 标题设置等
 *
 * @note 线程不安全
 */
class AnsiParser {
public:
    /// 输出回调：每个字符调用一次
    using OutputCallback = std::function<void(const Cell& cell)>;
    /// 光标回调：光标移动时调用
    using CursorCallback = std::function<void(int row, int col)>;
    /// 清除回调：清屏时调用
    using ClearCallback = std::function<void(ClearMode mode)>;
    /// 换行回调
    using NewLineCallback = std::function<void()>;
    /// 回车回调
    using CarriageReturnCallback = std::function<void()>;
    /// 标题回调
    using TitleCallback = std::function<void(const std::string& title)>;

    AnsiParser();

    // === 回调设置 ===
    void SetOutputCallback(OutputCallback cb) { output_cb_ = std::move(cb); }
    void SetCursorCallback(CursorCallback cb) { cursor_cb_ = std::move(cb); }
    void SetClearCallback(ClearCallback cb) { clear_cb_ = std::move(cb); }
    void SetNewLineCallback(NewLineCallback cb) { newline_cb_ = std::move(cb); }
    void SetCarriageReturnCallback(CarriageReturnCallback cb) { cr_cb_ = std::move(cb); }
    void SetTitleCallback(TitleCallback cb) { title_cb_ = std::move(cb); }

    /**
     * @brief 解析输入数据
     * @param data 输入数据
     * @param length 数据长度
     */
    void Parse(const char* data, size_t length);

    /**
     * @brief 解析字符串
     * @param str 输入字符串
     */
    void Parse(const std::string& str) {
        Parse(str.data(), str.size());
    }

    /**
     * @brief 重置解析器状态
     */
    void Reset();

    /**
     * @brief 获取当前样式
     * @return 当前文本样式
     */
    const TextStyle& current_style() const { return current_style_; }

private:
    ParserState state_ = ParserState::Ground;
    TextStyle current_style_;
    std::vector<int> csi_params_;
    std::string osc_string_;
    std::string utf8_buffer_;

    // 回调
    OutputCallback output_cb_;
    CursorCallback cursor_cb_;
    ClearCallback clear_cb_;
    NewLineCallback newline_cb_;
    CarriageReturnCallback cr_cb_;
    TitleCallback title_cb_;

    /**
     * @brief 处理单个字节
     * @param byte 输入字节
     */
    void ProcessByte(uint8_t byte);

    /**
     * @brief 处理 Ground 状态
     */
    void HandleGround(uint8_t byte);

    /**
     * @brief 处理 Escape 状态
     */
    void HandleEscape(uint8_t byte);

    /**
     * @brief 处理 CSI 状态
     */
    void HandleCsi(uint8_t byte);

    /**
     * @brief 处理 OSC 状态
     */
    void HandleOsc(uint8_t byte);

    /**
     * @brief 执行 CSI 命令
     * @param final_byte 终止字节
     */
    void ExecuteCsi(char final_byte);

    /**
     * @brief 执行 SGR (Select Graphic Rendition)
     */
    void ExecuteSgr();

    /**
     * @brief 执行 OSC 命令
     */
    void ExecuteOsc();

    /**
     * @brief 输出字符
     * @param codepoint Unicode 码点
     */
    void OutputChar(char32_t codepoint);

    /**
     * @brief 处理无效序列
     */
    void HandleInvalidSequence();

    /**
     * @brief 获取 CSI 参数
     * @param index 参数索引
     * @param default_value 默认值
     * @return 参数值
     */
    int GetCsiParam(size_t index, int default_value = 0) const;

    /**
     * @brief 检查是否为 UTF-8 起始字节
     */
    static bool IsUtf8Start(uint8_t byte);

    /**
     * @brief 检查是否为 UTF-8 后续字节
     */
    static bool IsUtf8Continuation(uint8_t byte);

    /**
     * @brief 解码 UTF-8 序列
     * @return 解码后的码点，失败返回 0xFFFD
     */
    char32_t DecodeUtf8();
};

}  // namespace lightui
