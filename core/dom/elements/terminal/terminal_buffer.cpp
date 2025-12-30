/**
 * @file terminal_buffer.cpp
 * @brief 终端缓冲区实现
 */

#include "terminal_buffer.h"

#include <algorithm>
#include <iostream>

namespace lightui {

const Cell TerminalBuffer::kDefaultCell = Cell(' ', TextStyle{});

TerminalBuffer::TerminalBuffer(int cols, int scrollback_lines)
    : cols_(cols)
    , scrollback_lines_(scrollback_lines)
    , lines_(scrollback_lines) {
    // 初始化可见区域的行
    for (int i = 0; i < visible_rows_; ++i) {
        lines_.Emplace(cols_, kDefaultCell);
    }
}

void TerminalBuffer::PutCell(const Cell& cell) {
    int buffer_row = GetBufferRow(cursor_row_);
    EnsureLine(buffer_row);
    
    auto& line = GetWritableLine(buffer_row);
    
    // 确保列存在
    if (cursor_col_ >= static_cast<int>(line.size())) {
        line.resize(cursor_col_ + 1, kDefaultCell);
    }
    
    line[cursor_col_] = cell;
    cursor_col_++;
    
    // 如果是宽字符，占用下一个单元格
    if (cell.width == 2) {
        if (cursor_col_ >= static_cast<int>(line.size())) {
            line.resize(cursor_col_ + 1, kDefaultCell);
        }
        // 下一个单元格标记为宽字符的后半部分（空字符，宽度为0）
        Cell placeholder;
        placeholder.codepoint = 0;  // 空字符表示这是宽字符的后半部分
        placeholder.width = 0;
        line[cursor_col_] = placeholder;
        cursor_col_++;
    }
    
    // 自动换行
    if (cursor_col_ >= cols_) {
        NewLine();
    }
}

void TerminalBuffer::NewLine() {
    cursor_col_ = 0;
    cursor_row_++;
    
    // 如果超出可见区域，滚动
    if (cursor_row_ >= visible_rows_) {
        ScrollUp();
        cursor_row_ = visible_rows_ - 1;
    }
    
    int buffer_row = GetBufferRow(cursor_row_);
    EnsureLine(buffer_row);
}

void TerminalBuffer::CarriageReturn() {
    cursor_col_ = 0;
}

void TerminalBuffer::Clear(ClearMode mode) {
    switch (mode) {
        case ClearMode::ToEnd: {
            // 清除从光标到屏幕末尾
            int buffer_row = GetBufferRow(cursor_row_);
            EnsureLine(buffer_row);
            auto& line = GetWritableLine(buffer_row);
            for (int c = cursor_col_; c < static_cast<int>(line.size()); ++c) {
                line[c] = kDefaultCell;
            }
            // 清除后续行（在可见区域内）
            for (int r = cursor_row_ + 1; r < visible_rows_; ++r) {
                int br = GetBufferRow(r);
                if (br < total_lines()) {
                    auto& l = GetWritableLine(br);
                    std::fill(l.begin(), l.end(), kDefaultCell);
                }
            }
            break;
        }
        
        case ClearMode::ToStart: {
            // 清除从屏幕开头到光标
            for (int r = 0; r < cursor_row_; ++r) {
                int br = GetBufferRow(r);
                if (br < total_lines()) {
                    auto& l = GetWritableLine(br);
                    std::fill(l.begin(), l.end(), kDefaultCell);
                }
            }
            int buffer_row = GetBufferRow(cursor_row_);
            EnsureLine(buffer_row);
            auto& line = GetWritableLine(buffer_row);
            for (int c = 0; c <= cursor_col_ && c < static_cast<int>(line.size()); ++c) {
                line[c] = kDefaultCell;
            }
            break;
        }
        
        case ClearMode::All:
        case ClearMode::Scrollback: {
            // 清除整个屏幕
            lines_.Clear();
            scroll_top_ = 0;
            for (int i = 0; i < visible_rows_; ++i) {
                lines_.Emplace(cols_, kDefaultCell);
            }
            cursor_row_ = 0;
            cursor_col_ = 0;
            break;
        }
    }
}

void TerminalBuffer::SetCursor(int row, int col) {
    cursor_row_ = row;
    cursor_col_ = col;
    ClampCursor();
}

void TerminalBuffer::MoveCursor(int delta_row, int delta_col) {
    cursor_row_ += delta_row;
    cursor_col_ += delta_col;
    ClampCursor();
}

const Cell& TerminalBuffer::GetCell(int row, int col) const {
    if (row < 0 || row >= total_lines()) {
        return kDefaultCell;
    }
    
    const auto& line = lines_[row];
    if (col < 0 || col >= static_cast<int>(line.size())) {
        return kDefaultCell;
    }
    
    return line[col];
}

std::string TerminalBuffer::Serialize() const {
    std::string result;
    
    for (int row = 0; row < total_lines(); ++row) {
        result += GetLineText(row);
        if (row < total_lines() - 1) {
            result += '\n';
        }
    }
    
    return result;
}

std::string TerminalBuffer::GetText(int start_row, int start_col,
                                     int end_row, int end_col) const {
    std::string result;
    
    // 规范化范围
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        std::swap(start_row, end_row);
        std::swap(start_col, end_col);
    }
    
    for (int row = start_row; row <= end_row && row < total_lines(); ++row) {
        const auto& line = lines_[row];
        
        int col_start = (row == start_row) ? start_col : 0;
        int col_end = (row == end_row) ? end_col : static_cast<int>(line.size());
        
        for (int col = col_start; col < col_end && col < static_cast<int>(line.size()); ++col) {
            char32_t cp = line[col].codepoint;
            // 简单的 UTF-8 编码
            if (cp < 0x80) {
                result += static_cast<char>(cp);
            } else if (cp < 0x800) {
                result += static_cast<char>(0xC0 | (cp >> 6));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (cp >> 18));
                result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        
        if (row < end_row) {
            result += '\n';
        }
    }
    
    return result;
}

std::string TerminalBuffer::GetLineText(int row) const {
    if (row < 0 || row >= total_lines()) {
        return "";
    }
    
    const auto& line = lines_[row];
    std::string result;
    
    // 找到最后一个非空字符
    int last_non_space = -1;
    for (int i = static_cast<int>(line.size()) - 1; i >= 0; --i) {
        if (line[i].codepoint != ' ') {
            last_non_space = i;
            break;
        }
    }
    
    for (int col = 0; col <= last_non_space; ++col) {
        char32_t cp = line[col].codepoint;
        // UTF-8 编码
        if (cp < 0x80) {
            result += static_cast<char>(cp);
        } else if (cp < 0x800) {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (cp >> 18));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    
    return result;
}

void TerminalBuffer::EnsureLine(int row) {
    while (total_lines() <= row) {
        lines_.Emplace(cols_, kDefaultCell);
    }
}

std::vector<Cell>& TerminalBuffer::GetWritableLine(int row) {
    EnsureLine(row);
    return lines_[row];
}

void TerminalBuffer::ScrollUp() {
    // 添加新行到缓冲区末尾
    lines_.Emplace(cols_, kDefaultCell);
    // 更新滚动顶部位置
    scroll_top_++;
}

void TerminalBuffer::ClampCursor() {
    cursor_row_ = std::max(0, std::min(cursor_row_, visible_rows_ - 1));
    cursor_col_ = std::max(0, std::min(cursor_col_, cols_ - 1));
}

int TerminalBuffer::GetBufferRow(int screen_row) const {
    // 将屏幕行号转换为缓冲区行号
    return scroll_top_ + screen_row;
}

}  // namespace lightui
