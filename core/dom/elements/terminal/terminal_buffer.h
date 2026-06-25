/**
 * @file terminal_buffer.h
 * @brief 终端缓冲区
 *
 * 存储终端内容，支持光标操作和回滚。
 */

#pragma once

#include "ansi_parser.h"
#include "core/dom/elements/virtual_text/virtual_buffer.h"

#include <string>
#include <vector>

namespace mblink {

/**
 * @brief 终端缓冲区
 *
 * 存储终端的单元格数据，支持：
 * - 光标移动和定位
 * - 行操作（换行、回车）
 * - 清屏操作
 * - 回滚缓冲区
 */
class TerminalBuffer {
public:
    /**
     * @brief 构造函数
     * @param cols 列数
     * @param scrollback_lines 回滚缓冲区行数
     */
    TerminalBuffer(int cols, int scrollback_lines = 10000);

    // === 写入操作 ===

    /**
     * @brief 写入单元格
     * @param cell 单元格数据
     */
    void PutCell(const Cell& cell);

    /**
     * @brief 换行
     */
    void NewLine();

    /**
     * @brief 回车
     */
    void CarriageReturn();

    /**
     * @brief 清除
     * @param mode 清除模式
     */
    void Clear(ClearMode mode);

    // === 光标操作 ===

    /**
     * @brief 设置光标位置（绝对）
     * @param row 行号
     * @param col 列号
     */
    void SetCursor(int row, int col);

    /**
     * @brief 移动光标（相对）
     * @param delta_row 行偏移
     * @param delta_col 列偏移
     */
    void MoveCursor(int delta_row, int delta_col);

    /**
     * @brief 获取光标行（屏幕行号）
     */
    int cursor_row() const { return cursor_row_; }

    /**
     * @brief 获取光标的缓冲区行号
     */
    int cursor_buffer_row() const { return scroll_top_ + cursor_row_; }

    /**
     * @brief 获取光标列
     */
    int cursor_col() const { return cursor_col_; }

    // === 读取操作 ===

    /**
     * @brief 获取单元格
     * @param row 行号（包含回滚）
     * @param col 列号
     * @return 单元格引用
     */
    const Cell& GetCell(int row, int col) const;

    /**
     * @brief 获取总行数（包含回滚）
     */
    int total_lines() const { return static_cast<int>(lines_.size()); }

    /**
     * @brief 获取包含实际内容或光标的显示行数
     */
    int display_line_count() const;

    /**
     * @brief 获取列数
     */
    int cols() const { return cols_; }

    /**
     * @brief 获取包含实际内容或光标的最大列数
     */
    int max_content_columns() const;

    /**
     * @brief 获取可见行数
     */
    int visible_rows() const { return visible_rows_; }

    /**
     * @brief 设置可见行数
     */
    void set_visible_rows(int rows);

    // === 序列化 ===

    /**
     * @brief 序列化为纯文本
     * @return 纯文本内容
     */
    std::string Serialize() const;

    /**
     * @brief 获取指定范围的文本
     * @param start_row 起始行
     * @param start_col 起始列
     * @param end_row 结束行
     * @param end_col 结束列
     * @return 文本内容
     */
    std::string GetText(int start_row, int start_col, 
                        int end_row, int end_col) const;

    /**
     * @brief 获取指定行的文本
     * @param row 行号
     * @return 行文本
     */
    std::string GetLineText(int row) const;

private:
    int cols_;
    int scrollback_lines_;
    int visible_rows_ = 24;
    int cursor_row_ = 0;  // 屏幕行号 (0 到 visible_rows_-1)
    int cursor_col_ = 0;
    int scroll_top_ = 0;  // 滚动顶部在缓冲区中的位置

    // 使用 VirtualBuffer 存储行
    VirtualBuffer<std::vector<Cell>> lines_;

    // 默认单元格
    static const Cell kDefaultCell;

    mutable bool content_metrics_dirty_ = true;
    mutable int cached_display_line_count_ = 1;
    mutable int cached_max_content_columns_ = 1;

    /**
     * @brief 将屏幕行号转换为缓冲区行号
     * @param screen_row 屏幕行号 (0 到 visible_rows_-1)
     * @return 缓冲区行号
     */
    int GetBufferRow(int screen_row) const;

    /**
     * @brief 确保行存在
     * @param row 行号
     */
    void EnsureLine(int row);

    /**
     * @brief 获取可写行
     * @param row 行号
     * @return 行引用
     */
    std::vector<Cell>& GetWritableLine(int row);

    /**
     * @brief 滚动屏幕
     */
    void ScrollUp();

    /**
     * @brief 限制光标在有效范围内
     */
    void ClampCursor();

    void MarkContentMetricsDirty() { content_metrics_dirty_ = true; }
    void RecomputeContentMetrics() const;
};

}  // namespace mblink
