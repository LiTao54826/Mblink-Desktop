/**
 * @file selection_manager.cpp
 * @brief 文本选择管理器实现
 */

#include "selection_manager.h"

#include <algorithm>
#include <cctype>

namespace lightui {
namespace virtual_text {

// === SelectionRange 实现 ===

bool SelectionRange::Contains(int line, int col) const {
    SelectionRange norm = Normalized();
    
    if (line < norm.start_line || line > norm.end_line) {
        return false;
    }
    
    if (line == norm.start_line && col < norm.start_col) {
        return false;
    }
    
    if (line == norm.end_line && col >= norm.end_col) {
        return false;
    }
    
    return true;
}

void SelectionRange::Normalize() {
    if (start_line > end_line || 
        (start_line == end_line && start_col > end_col)) {
        std::swap(start_line, end_line);
        std::swap(start_col, end_col);
    }
}

SelectionRange SelectionRange::Normalized() const {
    SelectionRange result = *this;
    result.Normalize();
    return result;
}

// === SelectionManager 实现 ===

void SelectionManager::StartSelection(int line, int col) {
    selection_.start_line = line;
    selection_.start_col = col;
    selection_.end_line = line;
    selection_.end_col = col;
    is_selecting_ = true;
    has_selection_ = false;
}

void SelectionManager::UpdateSelection(int line, int col) {
    if (!is_selecting_) {
        return;
    }
    
    selection_.end_line = line;
    selection_.end_col = col;
    
    // 只要起点和终点不同，就有选择
    has_selection_ = !selection_.IsEmpty();
}

void SelectionManager::EndSelection() {
    is_selecting_ = false;
}

void SelectionManager::ClearSelection() {
    selection_ = SelectionRange{};
    is_selecting_ = false;
    has_selection_ = false;
}

void SelectionManager::SelectWord(int line, int col, const std::string& line_text) {
    if (line_text.empty() || col < 0 || col >= static_cast<int>(line_text.size())) {
        ClearSelection();
        return;
    }
    
    // 如果点击位置不是单词字符，不选择
    if (!IsWordChar(line_text[col])) {
        ClearSelection();
        return;
    }
    
    // 向左找单词起始
    int start = col;
    while (start > 0 && IsWordChar(line_text[start - 1])) {
        --start;
    }
    
    // 向右找单词结束
    int end = col;
    while (end < static_cast<int>(line_text.size()) && IsWordChar(line_text[end])) {
        ++end;
    }
    
    selection_.start_line = line;
    selection_.start_col = start;
    selection_.end_line = line;
    selection_.end_col = end;
    is_selecting_ = false;
    has_selection_ = true;
}

void SelectionManager::SelectLine(int line, int line_length) {
    selection_.start_line = line;
    selection_.start_col = 0;
    selection_.end_line = line;
    selection_.end_col = line_length;
    is_selecting_ = false;
    has_selection_ = line_length > 0;
}

void SelectionManager::SelectAll(int total_lines, int last_line_length) {
    if (total_lines <= 0) {
        ClearSelection();
        return;
    }
    
    selection_.start_line = 0;
    selection_.start_col = 0;
    selection_.end_line = total_lines - 1;
    selection_.end_col = last_line_length;
    is_selecting_ = false;
    has_selection_ = true;
}

bool SelectionManager::HasSelection() const {
    // 正在选择中或已有选择都返回 true
    return (is_selecting_ || has_selection_) && !selection_.IsEmpty();
}

SelectionRange SelectionManager::GetSelection() const {
    return selection_.Normalized();
}

bool SelectionManager::IsSelected(int line, int col) const {
    if (!HasSelection()) {
        return false;
    }
    return selection_.Contains(line, col);
}

bool SelectionManager::IsWordChar(char c) {
    // 字母、数字、下划线视为单词字符
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

}  // namespace virtual_text
}  // namespace lightui
