/**
 * @file html_textarea_element.cpp
 * @brief HTML TextArea元素类实现
 */

#include "html_textarea_element.h"
#include "../document.h"
#include "../event.h"
#include "../utils/utf8_utils.h"
#include "../../render/input/text_edit_metrics.h"
#include "../render/text/text_renderer.h"
#include "core/editing/textarea_editing_controller.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/window/window.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <SDL3/SDL.h>
#include <include/core/SkFont.h>
#include <include/core/SkTextBlob.h>

namespace mbink {

HTMLTextAreaElement::HTMLTextAreaElement()
    : Element("textarea")
    , value_("")
    , value_initialized_(false)
    , edit_state_(CreateTextAreaEditState())
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
    if (edit_state_) {
        value_ = edit_state_->text;
    }

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
    int max_length = GetMaxLength();
    std::string new_value = value;
    if (max_length > 0 && static_cast<int>(utf8::CharCount(new_value)) > max_length) {
        new_value = utf8::SubstrByChar(new_value, 0, max_length);
    }

    std::string old_value = GetValue();
    value_ = new_value;
    value_initialized_ = true;

    if (edit_state_) {
        edit_state_->SetText(new_value);
        int new_length = static_cast<int>(utf8::CharCount(new_value));
        int anchor = std::min(edit_state_->selection_anchor, new_length);
        int focus = std::min(edit_state_->selection_focus, new_length);
        edit_state_->SetSelection(anchor, focus);
    }

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
    const std::string value = GetValue();

    if (IsRequired() && value.empty()) {
        return false;
    }

    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(utf8::CharCount(value)) > max_length) {
        return false;
    }

    return true;
}

std::string HTMLTextAreaElement::GetValidationMessage() const {
    const std::string value = GetValue();
    if (CheckValidity()) {
        return "";
    }

    if (IsRequired() && value.empty()) {
        return "Please fill out this field.";
    }

    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(utf8::CharCount(value)) > max_length) {
        return "Please use no more than " + std::to_string(max_length) + " characters.";
    }

    return "Invalid value.";
}

void HTMLTextAreaElement::Select() {
    if (!edit_state_) {
        return;
    }
    edit_state_->SetSelection(0, static_cast<int>(utf8::CharCount(GetValue())));
    RequestTextAreaRepaint();
}

void HTMLTextAreaElement::SetSelectionRange(int start, int end) {
    if (!edit_state_) {
        return;
    }
    int len = static_cast<int>(utf8::CharCount(GetValue()));
    edit_state_->SetSelection(std::max(0, std::min(start, len)),
                              std::max(0, std::min(end, len)));
    RequestTextAreaRepaint();
}

int HTMLTextAreaElement::GetSelectionStart() const {
    return edit_state_ ? edit_state_->GetSelectionStart() : 0;
}

int HTMLTextAreaElement::GetSelectionEnd() const {
    return edit_state_ ? edit_state_->GetSelectionEnd() : 0;
}

void HTMLTextAreaElement::HandleTextInput(const std::string& text) {
    if (IsDisabled() || IsReadOnly() || !edit_state_) {
        return;
    }

    int max_length = GetMaxLength();
    if (max_length > 0) {
        std::string candidate = edit_state_->text;
        int start = edit_state_->GetSelectionStart();
        int end = edit_state_->GetSelectionEnd();
        size_t start_byte = utf8::CharPosToBytePos(candidate, start);
        size_t end_byte = utf8::CharPosToBytePos(candidate, end);

        std::string normalized_text = text;
        size_t pos = 0;
        while ((pos = normalized_text.find("\r\n", pos)) != std::string::npos) {
            normalized_text.replace(pos, 2, "\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = normalized_text.find('\r', pos)) != std::string::npos) {
            normalized_text.erase(pos, 1);
        }

        candidate = candidate.substr(0, start_byte) + normalized_text + candidate.substr(end_byte);
        if (static_cast<int>(utf8::CharCount(candidate)) > max_length) {
            return;
        }
    }

    TextAreaEditingController controller(this, edit_state_);
    controller.ReplaceSelectionText(text);
}

void HTMLTextAreaElement::HandleKeyPress(const std::string& key, bool ctrl_key, bool shift_key) {
    bool is_navigation_key = (key == "ArrowLeft" || key == "ArrowRight" ||
                              key == "ArrowUp" || key == "ArrowDown" ||
                              key == "Home" || key == "End");
    bool is_clipboard_read = ctrl_key && (key == "c" || key == "C");

    if (!is_navigation_key && !is_clipboard_read && (IsDisabled() || IsReadOnly())) {
        return;
    }
    if (!edit_state_) {
        return;
    }

    TextAreaEditingController controller(this, edit_state_);
    const std::string current_value = GetValue();
    int char_count = static_cast<int>(utf8::CharCount(current_value));
    int old_cursor = edit_state_->caret_position;

    if (key == "Backspace") {
        controller.DeleteBackward();
    } else if (key == "Delete") {
        controller.DeleteForward();
    } else if (key == "ArrowLeft") {
        int new_pos = edit_state_->GetSelectionEnd() > 0 ? edit_state_->GetSelectionEnd() - 1 : 0;
        if (shift_key) {
            controller.SetSelection(edit_state_->selection_anchor, new_pos);
        } else {
            if (edit_state_->HasSelection()) {
                new_pos = edit_state_->GetSelectionStart();
            }
            controller.SetCaret(new_pos);
        }
    } else if (key == "ArrowRight") {
        int new_pos = edit_state_->GetSelectionEnd() < char_count ? edit_state_->GetSelectionEnd() + 1 : char_count;
        if (shift_key) {
            controller.SetSelection(edit_state_->selection_anchor, new_pos);
        } else {
            if (edit_state_->HasSelection()) {
                new_pos = edit_state_->GetSelectionEnd();
            }
            controller.SetCaret(new_pos);
        }
    } else if (key == "ArrowUp") {
        int line, col;
        GetLineAndColumn(edit_state_->GetSelectionEnd(), line, col);
        if (line > 0) {
            int new_pos = GetCharPosFromLineColumn(line - 1, col);
            shift_key ? controller.SetSelection(edit_state_->selection_anchor, new_pos)
                      : controller.SetCaret(new_pos);
        } else if (!shift_key) {
            controller.SetCaret(0);
        }
    } else if (key == "ArrowDown") {
        int line, col;
        GetLineAndColumn(edit_state_->GetSelectionEnd(), line, col);
        int line_count = GetLineCount();
        if (line < line_count - 1) {
            int new_pos = GetCharPosFromLineColumn(line + 1, col);
            shift_key ? controller.SetSelection(edit_state_->selection_anchor, new_pos)
                      : controller.SetCaret(new_pos);
        } else if (!shift_key) {
            controller.SetCaret(char_count);
        }
    } else if (key == "Home") {
        int line, col;
        GetLineAndColumn(edit_state_->GetSelectionEnd(), line, col);
        int line_start, line_end;
        GetLineRange(line, line_start, line_end);
        shift_key ? controller.SetSelection(edit_state_->selection_anchor, line_start)
                  : controller.SetCaret(line_start);
    } else if (key == "End") {
        int line, col;
        GetLineAndColumn(edit_state_->GetSelectionEnd(), line, col);
        int line_start, line_end;
        GetLineRange(line, line_start, line_end);
        shift_key ? controller.SetSelection(edit_state_->selection_anchor, line_end)
                  : controller.SetCaret(line_end);
    } else if (key == "Enter") {
        HandleTextInput("\n");
    } else if (ctrl_key && (key == "a" || key == "A")) {
        Select();
        RequestTextAreaRepaint();
    } else if (ctrl_key && (key == "c" || key == "C")) {
        if (edit_state_->HasSelection()) {
            std::string selected_text = utf8::SubstrByChar(current_value,
                                                           edit_state_->GetSelectionStart(),
                                                           edit_state_->GetSelectionEnd());
            SDL_SetClipboardText(selected_text.c_str());
        }
    } else if (ctrl_key && (key == "x" || key == "X")) {
        if (edit_state_->HasSelection() && !IsReadOnly()) {
            std::string selected_text = utf8::SubstrByChar(current_value,
                                                           edit_state_->GetSelectionStart(),
                                                           edit_state_->GetSelectionEnd());
            SDL_SetClipboardText(selected_text.c_str());
            controller.ReplaceSelectionText("");
        }
    } else if (ctrl_key && (key == "v" || key == "V")) {
        if (!IsReadOnly()) {
            char* clipboard_text = SDL_GetClipboardText();
            if (clipboard_text && clipboard_text[0] != '\0') {
                HandleTextInput(clipboard_text);
            }
            SDL_free(clipboard_text);
        }
    }

    if (edit_state_->caret_position != old_cursor) {
        MarkScrollToCursor();
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
    if (!edit_state_) {
        return;
    }
    int char_count = static_cast<int>(utf8::CharCount(GetValue()));
    char_pos = std::clamp(char_pos, 0, char_count);
    edit_state_->SetCaretPosition(char_pos);
}

void HTMLTextAreaElement::SetSelection(int start, int end) {
    if (!edit_state_) {
        return;
    }
    int char_count = static_cast<int>(utf8::CharCount(GetValue()));
    edit_state_->SetSelection(std::clamp(start, 0, char_count),
                              std::clamp(end, 0, char_count));
}

void HTMLTextAreaElement::RequestTextAreaRepaint() {
    auto doc = GetOwnerDocument();
    if (!doc) {
        return;
    }

    Window* window = doc->GetWindow();
    if (!window) {
        return;
    }

    SkRect dirty_rect = SkRect::MakeEmpty();
    if (auto render_obj = GetRenderObject()) {
        render_obj->MarkNeedsPaint();
        render_obj->InvalidatePaintCache();

        const auto& bounds = render_obj->GetViewportBounds();
        if (bounds.valid && bounds.width > 0.0f && bounds.height > 0.0f) {
            dirty_rect = SkRect::MakeXYWH(bounds.x, bounds.y, bounds.width, bounds.height);
        } else {
            dirty_rect = render_obj->GetViewportBoundingRect();
        }
    }

    if (!dirty_rect.isEmpty()) {
        SetDirtyRect(dirty_rect);
        window->AddDirtyRect(dirty_rect);

        if (auto* pipeline = window->GetRenderPipeline()) {
            pipeline->MarkDirtyRegion(dirty_rect);
            pipeline->MarkNeedsPaint();
        }
    } else if (auto* pipeline = window->GetRenderPipeline()) {
        pipeline->MarkNeedsPaint();
    }

    window->SetNeedsRepaintFor(RepaintReason::KeyboardInput);
    if (auto* pipeline = window->GetRenderPipeline()) {
        pipeline->ForceRasterize();
    }
}

// ========== 辅助方法 ==========

void HTMLTextAreaElement::GetLineAndColumn(int char_pos, int& out_line, int& out_col) const {
    const std::string value = GetValue();
    int total_chars = static_cast<int>(utf8::CharCount(value));
    char_pos = std::clamp(char_pos, 0, total_chars);

    out_line = 0;
    out_col = 0;

    int current_pos = 0;
    int line_start = 0;

    for (size_t i = 0; i < value.size(); ) {
        if (current_pos >= char_pos) {
            break;
        }

        unsigned char c = static_cast<unsigned char>(value[i]);
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
            out_line++;
            line_start = current_pos + 1;
        }

        i += char_bytes;
        current_pos++;
    }

    out_col = char_pos - line_start;
}

int HTMLTextAreaElement::GetCharPosFromLineColumn(int line, int col) const {
    const std::string value = GetValue();
    int current_line = 0;
    int current_pos = 0;
    int line_start = 0;

    for (size_t i = 0; i < value.size() && current_line < line; ) {
        unsigned char c = static_cast<unsigned char>(value[i]);
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

    if (current_line < line) {
        return static_cast<int>(utf8::CharCount(value));
    }

    int target_pos = line_start;
    int col_count = 0;
    for (size_t i = utf8::CharPosToBytePos(value, line_start); i < value.size() && col_count < col; ) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        if (c == '\n') {
            break;
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
    const std::string value = GetValue();
    out_start = 0;
    out_end = 0;

    int current_line = 0;
    int current_pos = 0;
    int line_start = 0;

    for (size_t i = 0; i < value.size(); ) {
        unsigned char c = static_cast<unsigned char>(value[i]);
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

    if (current_line == line) {
        out_start = line_start;
        out_end = current_pos;
    }
}

int HTMLTextAreaElement::GetLineCount() const {
    const std::string value = GetValue();
    int count = 1;
    for (char c : value) {
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
    float max_line_width = GetMaxLineWidth(font);
    float max_scroll = std::max(0.0f, max_line_width - visible_width);
    float scroll_amount = delta_x * 30.0f;
    scroll_left_ = std::clamp(scroll_left_ + scroll_amount, 0.0f, max_scroll);
}

void HTMLTextAreaElement::EnsureCursorVisible(float line_height, float visible_height, float visible_width, const SkFont& font) {
    const std::string value = GetValue();
    int cursor_pos = edit_state_ ? edit_state_->caret_position : 0;
    size_t byte_pos = utf8::CharPosToBytePos(value, cursor_pos);
    std::string text_before = value.substr(0, byte_pos);

    int cursor_line = 0;
    for (char c : text_before) {
        if (c == '\n') {
            cursor_line++;
        }
    }

    float cursor_y = cursor_line * line_height;
    if (cursor_y < scroll_top_) {
        scroll_top_ = cursor_y;
    } else if (cursor_y + line_height > scroll_top_ + visible_height) {
        scroll_top_ = cursor_y + line_height - visible_height;
    }

    float content_height = GetContentHeight(line_height);
    float max_scroll_y = std::max(0.0f, content_height - visible_height);
    scroll_top_ = std::clamp(scroll_top_, 0.0f, max_scroll_y);

    size_t last_newline = text_before.rfind('\n');
    std::string current_line_before_cursor = (last_newline != std::string::npos)
        ? text_before.substr(last_newline + 1)
        : text_before;

    float cursor_x = text_edit_metrics::MeasureTextWidth(current_line_before_cursor, font, false);
    if (cursor_x < scroll_left_) {
        scroll_left_ = cursor_x;
    } else if (cursor_x > scroll_left_ + visible_width - 2.0f) {
        scroll_left_ = cursor_x - visible_width + 2.0f;
    }

    float max_scroll_x = std::max(0.0f, GetMaxLineWidth(font) - visible_width);
    scroll_left_ = std::clamp(scroll_left_, 0.0f, max_scroll_x);
}

float HTMLTextAreaElement::GetContentHeight(float line_height) const {
    int line_count = GetLineCount();
    return line_count * line_height;
}

float HTMLTextAreaElement::GetMaxLineWidth(const SkFont& font) const {
    const std::string value = GetValue();
    float max_width = 0.0f;
    std::istringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) {
        float w = text_edit_metrics::MeasureTextWidth(line, font, false);
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

} // namespace mbink

