/**
 * @file html_textarea_element.cpp
 * @brief HTML TextArea元素类实现
 */

#include "html_textarea_element.h"
#include "event.h"
#include "../utils/utf8_utils.h"
#include "../render/text_renderer.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <SDL3/SDL.h>
#include <include/core/SkFont.h>
#include <include/core/SkTextBlob.h>

namespace lightui {

HTMLTextAreaElement::HTMLTextAreaElement()
    : Element("textarea")
    , value_("")
    , value_initialized_(false)
    , selection_start_(0)
    , selection_end_(0)
    , is_dragging_selection_(false)
    , drag_start_pos_(0)
    , scroll_top_(0.0f)
    , scroll_left_(0.0f)
    , needs_scroll_to_cursor_(false)
    , scrollbar_drag_type_(ScrollbarType::NONE)
    , scrollbar_drag_start_pos_(0.0f)
    , scrollbar_drag_start_scroll_(0.0f) {
}

std::string HTMLTextAreaElement::GetValue() const {
    // 如果 value_ 尚未初始化，从子文本节点获取初始值
    // 符合浏览器行为：<textarea>text</textarea> 中的文本会成为初始值
    if (!value_initialized_ && value_.empty()) {
        // 获取子节点的文本内容
        std::string text_content = Element::GetTextContent();
        if (!text_content.empty()) {
            value_ = text_content;
            value_initialized_ = true;
        }
    }
    return value_;
}

void HTMLTextAreaElement::SetValue(const std::string& value, bool trigger_events) {
    // 检查maxlength限制
    int max_length = GetMaxLength();
    std::string new_value = value;
    if (max_length > 0 && static_cast<int>(value.length()) > max_length) {
        new_value = value.substr(0, max_length);
    }

    std::string old_value = value_;
    value_ = new_value;
    value_initialized_ = true;  // 标记 value 已经被显式设置

    // 调整选择范围，确保不越界
    int new_length = static_cast<int>(new_value.length());
    if (selection_start_ > new_length) {
        selection_start_ = new_length;
    }
    if (selection_end_ > new_length) {
        selection_end_ = new_length;
    }

    // 触发事件
    if (trigger_events && old_value != new_value) {
        TriggerInputEvent();
        TriggerChangeEvent();
    }
}

int HTMLTextAreaElement::GetMaxLength() const {
    std::string max_length_str = GetAttribute("maxlength");
    if (max_length_str.empty()) {
        return -1;  // 无限制
    }
    
    try {
        return std::stoi(max_length_str);
    } catch (...) {
        return -1;
    }
}

void HTMLTextAreaElement::SetMaxLength(int max_length) {
    if (max_length < 0) {
        RemoveAttribute("maxlength");
    } else {
        SetAttribute("maxlength", std::to_string(max_length));
    }
}

int HTMLTextAreaElement::GetRows() const {
    std::string rows_str = GetAttribute("rows");
    if (rows_str.empty()) {
        return 2;  // 默认2行
    }
    
    try {
        return std::stoi(rows_str);
    } catch (...) {
        return 2;
    }
}

void HTMLTextAreaElement::SetRows(int rows) {
    if (rows > 0) {
        SetAttribute("rows", std::to_string(rows));
    }
}

int HTMLTextAreaElement::GetCols() const {
    std::string cols_str = GetAttribute("cols");
    if (cols_str.empty()) {
        return 20;  // 默认20列
    }
    
    try {
        return std::stoi(cols_str);
    } catch (...) {
        return 20;
    }
}

void HTMLTextAreaElement::SetCols(int cols) {
    if (cols > 0) {
        SetAttribute("cols", std::to_string(cols));
    }
}

void HTMLTextAreaElement::SetDisabled(bool disabled) {
    if (disabled) {
        SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
}

void HTMLTextAreaElement::SetReadOnly(bool readonly) {
    if (readonly) {
        SetAttribute("readonly", "");
    } else {
        RemoveAttribute("readonly");
    }
}

void HTMLTextAreaElement::SetRequired(bool required) {
    if (required) {
        SetAttribute("required", "");
    } else {
        RemoveAttribute("required");
    }
}

bool HTMLTextAreaElement::CheckValidity() const {
    // 检查required
    if (IsRequired() && value_.empty()) {
        return false;
    }
    
    // 检查maxlength
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(value_.length()) > max_length) {
        return false;
    }
    
    return true;
}

std::string HTMLTextAreaElement::GetValidationMessage() const {
    if (CheckValidity()) {
        return "";
    }
    
    if (IsRequired() && value_.empty()) {
        return "Please fill out this field.";
    }
    
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(value_.length()) > max_length) {
        return "Please use no more than " + std::to_string(max_length) + " characters.";
    }
    
    return "Invalid value.";
}

void HTMLTextAreaElement::Select() {
    selection_start_ = 0;
    selection_end_ = static_cast<int>(utf8::CharCount(value_));
}

void HTMLTextAreaElement::SetSelectionRange(int start, int end) {
    int len = static_cast<int>(utf8::CharCount(value_));
    selection_start_ = std::max(0, std::min(start, len));
    selection_end_ = std::max(selection_start_, std::min(end, len));
}

void HTMLTextAreaElement::HandleTextInput(const std::string& text) {
    // 检查是否可编辑
    if (IsDisabled() || IsReadOnly()) {
        return;
    }

    // 将 Windows 换行符 CRLF (\r\n) 转换为 LF (\n)
    std::string normalized_text = text;
    size_t pos = 0;
    while ((pos = normalized_text.find("\r\n", pos)) != std::string::npos) {
        normalized_text.replace(pos, 2, "\n");
        pos += 1;
    }
    // 移除独立的 \r
    pos = 0;
    while ((pos = normalized_text.find('\r', pos)) != std::string::npos) {
        normalized_text.erase(pos, 1);
    }

    // 使用 UTF-8 工具处理文本插入
    std::string new_value = value_;
    size_t text_char_count = utf8::CharCount(normalized_text);

    if (selection_start_ != selection_end_) {
        // 有选中文本，替换选中部分
        int start = std::min(selection_start_, selection_end_);
        int end = std::max(selection_start_, selection_end_);
        size_t start_byte = utf8::CharPosToBytePos(value_, start);
        size_t end_byte = utf8::CharPosToBytePos(value_, end);
        new_value = value_.substr(0, start_byte) + normalized_text + value_.substr(end_byte);
        selection_start_ = start;
        selection_end_ = start;
    } else {
        // 无选中文本，在光标位置插入
        size_t byte_pos = utf8::CharPosToBytePos(value_, selection_start_);
        new_value = value_.substr(0, byte_pos) + normalized_text + value_.substr(byte_pos);
    }

    // 检查maxlength（按字符数检查，不是字节数）
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(utf8::CharCount(new_value)) > max_length) {
        return;  // 超过最大长度，忽略输入
    }

    value_ = new_value;
    selection_start_ += static_cast<int>(text_char_count);
    selection_end_ = selection_start_;

    // 标记需要滚动到光标位置
    needs_scroll_to_cursor_ = true;

    // 触发input事件
    TriggerInputEvent();
}

void HTMLTextAreaElement::HandleKeyPress(const std::string& key, bool ctrl_key, bool shift_key) {
    // 检查是否可编辑（导航键和复制不需要可编辑）
    bool is_navigation_key = (key == "ArrowLeft" || key == "ArrowRight" ||
                              key == "ArrowUp" || key == "ArrowDown" ||
                              key == "Home" || key == "End");
    bool is_clipboard_read = ctrl_key && (key == "c" || key == "C");

    if (!is_navigation_key && !is_clipboard_read && (IsDisabled() || IsReadOnly())) {
        return;
    }

    size_t char_count = utf8::CharCount(value_);
    int old_cursor = selection_end_;  // 保存当前光标位置

    // 处理特殊按键
    if (key == "Backspace") {
        if (selection_start_ != selection_end_) {
            // 删除选中文本
            int start = std::min(selection_start_, selection_end_);
            int end = std::max(selection_start_, selection_end_);
            size_t start_byte = utf8::CharPosToBytePos(value_, start);
            size_t end_byte = utf8::CharPosToBytePos(value_, end);
            value_ = value_.substr(0, start_byte) + value_.substr(end_byte);
            selection_start_ = start;
            selection_end_ = start;
        } else if (selection_start_ > 0) {
            // 删除光标前一个字符（正确处理多字节UTF-8字符）
            size_t prev_byte = utf8::CharPosToBytePos(value_, selection_start_ - 1);
            size_t curr_byte = utf8::CharPosToBytePos(value_, selection_start_);
            value_ = value_.substr(0, prev_byte) + value_.substr(curr_byte);
            selection_start_--;
            selection_end_ = selection_start_;
        }
        TriggerInputEvent();

    } else if (key == "Delete") {
        if (selection_start_ != selection_end_) {
            // 删除选中文本
            int start = std::min(selection_start_, selection_end_);
            int end = std::max(selection_start_, selection_end_);
            size_t start_byte = utf8::CharPosToBytePos(value_, start);
            size_t end_byte = utf8::CharPosToBytePos(value_, end);
            value_ = value_.substr(0, start_byte) + value_.substr(end_byte);
            selection_start_ = start;
            selection_end_ = start;
        } else if (selection_start_ < static_cast<int>(char_count)) {
            // 删除光标后一个字符
            size_t curr_byte = utf8::CharPosToBytePos(value_, selection_start_);
            size_t next_byte = utf8::CharPosToBytePos(value_, selection_start_ + 1);
            value_ = value_.substr(0, curr_byte) + value_.substr(next_byte);
        }
        TriggerInputEvent();

    } else if (key == "ArrowLeft") {
        int new_pos = selection_end_ > 0 ? selection_end_ - 1 : 0;
        if (shift_key) {
            // Shift+Left: 扩展选择
            selection_end_ = new_pos;
        } else {
            // 普通Left: 移动光标，清除选择
            if (selection_start_ != selection_end_) {
                new_pos = std::min(selection_start_, selection_end_);
            }
            selection_start_ = new_pos;
            selection_end_ = new_pos;
        }

    } else if (key == "ArrowRight") {
        int new_pos = selection_end_ < static_cast<int>(char_count) ? selection_end_ + 1 : static_cast<int>(char_count);
        if (shift_key) {
            // Shift+Right: 扩展选择
            selection_end_ = new_pos;
        } else {
            // 普通Right: 移动光标，清除选择
            if (selection_start_ != selection_end_) {
                new_pos = std::max(selection_start_, selection_end_);
            }
            selection_start_ = new_pos;
            selection_end_ = new_pos;
        }

    } else if (key == "ArrowUp") {
        // 上键: 移动到上一行的相同列位置
        int line, col;
        GetLineAndColumn(selection_end_, line, col);
        if (line > 0) {
            int new_pos = GetCharPosFromLineColumn(line - 1, col);
            if (shift_key) {
                selection_end_ = new_pos;
            } else {
                selection_start_ = new_pos;
                selection_end_ = new_pos;
            }
        } else if (!shift_key) {
            // 已在第一行，移动到行首
            selection_start_ = 0;
            selection_end_ = 0;
        }

    } else if (key == "ArrowDown") {
        // 下键: 移动到下一行的相同列位置
        int line, col;
        GetLineAndColumn(selection_end_, line, col);
        int line_count = GetLineCount();
        if (line < line_count - 1) {
            int new_pos = GetCharPosFromLineColumn(line + 1, col);
            if (shift_key) {
                selection_end_ = new_pos;
            } else {
                selection_start_ = new_pos;
                selection_end_ = new_pos;
            }
        } else if (!shift_key) {
            // 已在最后一行，移动到行尾
            selection_start_ = static_cast<int>(char_count);
            selection_end_ = selection_start_;
        }

    } else if (key == "Home") {
        // Home: 移动到当前行开头
        int line, col;
        GetLineAndColumn(selection_end_, line, col);
        int line_start, line_end;
        GetLineRange(line, line_start, line_end);
        if (shift_key) {
            selection_end_ = line_start;
        } else {
            selection_start_ = line_start;
            selection_end_ = line_start;
        }

    } else if (key == "End") {
        // End: 移动到当前行结尾
        int line, col;
        GetLineAndColumn(selection_end_, line, col);
        int line_start, line_end;
        GetLineRange(line, line_start, line_end);
        if (shift_key) {
            selection_end_ = line_end;
        } else {
            selection_start_ = line_end;
            selection_end_ = line_end;
        }

    } else if (key == "Enter") {
        // TextArea支持换行
        HandleTextInput("\n");

    } else if (ctrl_key && (key == "a" || key == "A")) {
        // Ctrl+A 全选
        Select();

    } else if (ctrl_key && (key == "c" || key == "C")) {
        // Ctrl+C 复制
        if (selection_start_ != selection_end_) {
            int start = std::min(selection_start_, selection_end_);
            int end = std::max(selection_start_, selection_end_);
            std::string selected_text = utf8::SubstrByChar(value_, start, end);
            SDL_SetClipboardText(selected_text.c_str());
        }

    } else if (ctrl_key && (key == "x" || key == "X")) {
        // Ctrl+X 剪切
        if (selection_start_ != selection_end_ && !IsReadOnly()) {
            int start = std::min(selection_start_, selection_end_);
            int end = std::max(selection_start_, selection_end_);
            std::string selected_text = utf8::SubstrByChar(value_, start, end);
            SDL_SetClipboardText(selected_text.c_str());

            // 删除选中文本
            size_t start_byte = utf8::CharPosToBytePos(value_, start);
            size_t end_byte = utf8::CharPosToBytePos(value_, end);
            value_ = value_.substr(0, start_byte) + value_.substr(end_byte);
            selection_start_ = start;
            selection_end_ = start;
            TriggerInputEvent();
        }

    } else if (ctrl_key && (key == "v" || key == "V")) {
        // Ctrl+V 粘贴
        if (!IsReadOnly()) {
            char* clipboard_text = SDL_GetClipboardText();
            if (clipboard_text && clipboard_text[0] != '\0') {
                HandleTextInput(clipboard_text);
            }
            SDL_free(clipboard_text);
        }
    }

    // 如果光标位置发生变化，标记需要滚动到光标位置
    if (selection_end_ != old_cursor) {
        needs_scroll_to_cursor_ = true;
    }
}

void HTMLTextAreaElement::TriggerChangeEvent() {
    auto change_event = std::make_shared<Event>("change");
    DispatchEvent(change_event);
}

void HTMLTextAreaElement::TriggerInputEvent() {
    auto input_event = std::make_shared<Event>("input");
    DispatchEvent(input_event);
}

// ========== 鼠标交互方法 ==========

void HTMLTextAreaElement::HandleMouseDown(float local_x, float local_y) {
    is_dragging_selection_ = true;
}

void HTMLTextAreaElement::HandleMouseMove(float local_x, float local_y) {
    // 字符位置计算在渲染层完成
}

void HTMLTextAreaElement::HandleMouseUp() {
    is_dragging_selection_ = false;
}

void HTMLTextAreaElement::SetCursorPosition(int char_pos) {
    size_t char_count = utf8::CharCount(value_);
    if (char_pos < 0) {
        char_pos = 0;
    } else if (char_pos > static_cast<int>(char_count)) {
        char_pos = static_cast<int>(char_count);
    }
    selection_start_ = char_pos;
    selection_end_ = char_pos;
}

void HTMLTextAreaElement::SetSelection(int start, int end) {
    size_t char_count = utf8::CharCount(value_);
    selection_start_ = std::max(0, std::min(start, static_cast<int>(char_count)));
    selection_end_ = std::max(0, std::min(end, static_cast<int>(char_count)));
}

// ========== 辅助方法 ==========

void HTMLTextAreaElement::GetLineAndColumn(int char_pos, int& out_line, int& out_col) const {
    out_line = 0;
    out_col = 0;

    int current_pos = 0;
    int line_start = 0;

    // 遍历字符，计算行号和列号
    for (size_t i = 0; i < value_.size(); ) {
        if (current_pos >= char_pos) {
            break;
        }

        // 获取当前UTF-8字符的字节数
        unsigned char c = static_cast<unsigned char>(value_[i]);
        size_t char_bytes = 1;
        if ((c & 0x80) == 0) {
            char_bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_bytes = 4;
        }

        // 检查是否是换行符
        if (c == '\n') {
            out_line++;
            line_start = current_pos + 1;
        }

        i += char_bytes;
        current_pos++;
    }

    out_col = char_pos - line_start;
}

int HTMLTextAreaElement::GetCharPosFromLineColumn(int line, int col) const {
    int current_line = 0;
    int current_pos = 0;
    int line_start = 0;

    // 找到目标行的起始位置
    for (size_t i = 0; i < value_.size() && current_line < line; ) {
        unsigned char c = static_cast<unsigned char>(value_[i]);
        size_t char_bytes = 1;
        if ((c & 0x80) == 0) {
            char_bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_bytes = 4;
        }

        if (c == '\n') {
            current_line++;
            line_start = current_pos + 1;
        }

        i += char_bytes;
        current_pos++;
    }

    // 如果没找到目标行，返回文本末尾
    if (current_line < line) {
        return static_cast<int>(utf8::CharCount(value_));
    }

    // 在目标行中移动到指定列
    int target_pos = line_start;
    int col_count = 0;
    for (size_t i = utf8::CharPosToBytePos(value_, line_start); i < value_.size() && col_count < col; ) {
        unsigned char c = static_cast<unsigned char>(value_[i]);
        if (c == '\n') {
            break;  // 行尾
        }

        size_t char_bytes = 1;
        if ((c & 0x80) == 0) {
            char_bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_bytes = 4;
        }

        i += char_bytes;
        target_pos++;
        col_count++;
    }

    return target_pos;
}

void HTMLTextAreaElement::GetLineRange(int line, int& out_start, int& out_end) const {
    out_start = 0;
    out_end = 0;

    int current_line = 0;
    int current_pos = 0;
    int line_start = 0;

    for (size_t i = 0; i < value_.size(); ) {
        unsigned char c = static_cast<unsigned char>(value_[i]);
        size_t char_bytes = 1;
        if ((c & 0x80) == 0) {
            char_bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_bytes = 4;
        }

        if (current_line == line) {
            if (c == '\n') {
                out_start = line_start;
                out_end = current_pos;
                return;
            }
        } else if (c == '\n') {
            current_line++;
            line_start = current_pos + 1;
        }

        i += char_bytes;
        current_pos++;
    }

    // 最后一行或目标行
    if (current_line == line) {
        out_start = line_start;
        out_end = current_pos;
    }
}

int HTMLTextAreaElement::GetLineCount() const {
    int count = 1;
    for (char c : value_) {
        if (c == '\n') {
            count++;
        }
    }
    return count;
}

// ========== 滚动相关方法 ==========

void HTMLTextAreaElement::SetScrollTop(float scroll_top) {
    scroll_top_ = std::max(0.0f, scroll_top);
}

void HTMLTextAreaElement::HandleMouseWheel(float delta_y, float line_height, float visible_height) {
    // delta_y > 0 表示向下滚动（scroll_top 增加），< 0 表示向上滚动（scroll_top 减少）
    float scroll_amount = delta_y * line_height * 3;  // 每次滚动3行
    float content_height = GetContentHeight(line_height);
    float max_scroll = std::max(0.0f, content_height - visible_height);

    float old_scroll_top = scroll_top_;
    scroll_top_ = std::clamp(scroll_top_ + scroll_amount, 0.0f, max_scroll);

    // std::cout << "[HandleMouseWheel] delta_y=" << delta_y
    //           << " line_height=" << line_height
    //           << " visible_height=" << visible_height
    //           << " content_height=" << content_height
    //           << " line_count=" << GetLineCount()
    //           << " max_scroll=" << max_scroll
    //           << " old_scroll=" << old_scroll_top
    //           << " new_scroll=" << scroll_top_ << std::endl;
}

void HTMLTextAreaElement::SetScrollLeft(float scroll_left) {
    scroll_left_ = std::max(0.0f, scroll_left);
}

void HTMLTextAreaElement::HandleMouseWheelHorizontal(float delta_x, float visible_width, const SkFont& font) {
    // 计算最长行的宽度
    float max_line_width = 0.0f;
    std::istringstream stream(value_);
    std::string line;
    while (std::getline(stream, line)) {
        float line_width = font.measureText(line.c_str(), line.size(), SkTextEncoding::kUTF8);
        if (line_width > max_line_width) {
            max_line_width = line_width;
        }
    }

    float max_scroll = std::max(0.0f, max_line_width - visible_width);
    float scroll_amount = delta_x * 30.0f;  // 每次滚动30像素
    scroll_left_ = std::clamp(scroll_left_ + scroll_amount, 0.0f, max_scroll);
}

void HTMLTextAreaElement::EnsureCursorVisible(float line_height, float visible_height, float visible_width, const SkFont& font) {
    // 计算光标所在行
    int cursor_line = 0;
    int cursor_pos = selection_end_;

    size_t byte_pos = utf8::CharPosToBytePos(value_, cursor_pos);
    std::string text_before = value_.substr(0, byte_pos);
    for (char c : text_before) {
        if (c == '\n') {
            cursor_line++;
        }
    }

    // 计算光标的Y位置
    float cursor_y = cursor_line * line_height;

    // 检查是否需要垂直滚动
    if (cursor_y < scroll_top_) {
        // 光标在可见区域上方，向上滚动
        scroll_top_ = cursor_y;
    } else if (cursor_y + line_height > scroll_top_ + visible_height) {
        // 光标在可见区域下方，向下滚动
        scroll_top_ = cursor_y + line_height - visible_height;
    }

    // 确保垂直滚动位置有效
    float content_height = GetContentHeight(line_height);
    float max_scroll_y = std::max(0.0f, content_height - visible_height);
    scroll_top_ = std::clamp(scroll_top_, 0.0f, max_scroll_y);

    // 计算光标的X位置（当前行中光标前的文本宽度）
    size_t last_newline = text_before.rfind('\n');
    std::string current_line_before_cursor;
    if (last_newline != std::string::npos) {
        current_line_before_cursor = text_before.substr(last_newline + 1);
    } else {
        current_line_before_cursor = text_before;
    }

    // 使用支持 CJK/Emoji 的测量方法（与渲染一致）
    TextRenderer text_renderer(nullptr);
    float cursor_x = 0.0f;
    if (!current_line_before_cursor.empty()) {
        cursor_x = text_renderer.MeasureTextWidthWithEmoji(current_line_before_cursor, font);
    }

    // 检查是否需要横向滚动
    if (cursor_x < scroll_left_) {
        // 光标在可见区域左边，向左滚动
        scroll_left_ = cursor_x;
    } else if (cursor_x > scroll_left_ + visible_width - 2.0f) {
        // 光标在可见区域右边，向右滚动（留2像素给光标）
        scroll_left_ = cursor_x - visible_width + 2.0f;
    }

    // 确保横向滚动位置有效
    scroll_left_ = std::max(0.0f, scroll_left_);
}

float HTMLTextAreaElement::GetContentHeight(float line_height) const {
    int line_count = GetLineCount();
    return line_count * line_height;
}

float HTMLTextAreaElement::GetMaxLineWidth(const SkFont& font) const {
    // 使用支持 CJK/Emoji 的测量方法（与渲染一致）
    TextRenderer text_renderer(nullptr);
    float max_width = 0.0f;
    std::istringstream stream(value_);
    std::string line;
    while (std::getline(stream, line)) {
        float w = text_renderer.MeasureTextWidthWithEmoji(line, font);
        if (w > max_width) max_width = w;
    }
    return max_width;
}

void HTMLTextAreaElement::StartScrollbarDrag(ScrollbarType type, float mouse_pos) {
    scrollbar_drag_type_ = type;
    scrollbar_drag_start_pos_ = mouse_pos;
    if (type == ScrollbarType::VERTICAL) {
        scrollbar_drag_start_scroll_ = scroll_top_;
    } else {
        scrollbar_drag_start_scroll_ = scroll_left_;
    }
}

void HTMLTextAreaElement::UpdateScrollbarDrag(float mouse_pos, float track_size, float content_size, float visible_size) {
    if (scrollbar_drag_type_ == ScrollbarType::NONE) return;

    // 计算滑块尺寸
    float thumb_ratio = visible_size / content_size;
    float thumb_size = std::max(20.0f, track_size * thumb_ratio);

    // 计算可移动的范围
    float movable_range = track_size - thumb_size;
    if (movable_range <= 0) return;

    // 计算鼠标移动量
    float delta = mouse_pos - scrollbar_drag_start_pos_;

    // 将鼠标移动量转换为滚动距离
    float max_scroll = content_size - visible_size;
    float scroll_delta = (delta / movable_range) * max_scroll;

    // 更新滚动位置
    float new_scroll = scrollbar_drag_start_scroll_ + scroll_delta;
    new_scroll = std::clamp(new_scroll, 0.0f, max_scroll);

    if (scrollbar_drag_type_ == ScrollbarType::VERTICAL) {
        scroll_top_ = new_scroll;
    } else {
        scroll_left_ = new_scroll;
    }
}

void HTMLTextAreaElement::EndScrollbarDrag() {
    scrollbar_drag_type_ = ScrollbarType::NONE;
}

} // namespace lightui

