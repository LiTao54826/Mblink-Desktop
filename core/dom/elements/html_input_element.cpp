/**
 * @file html_input_element.cpp
 * @brief HTML Input元素类实现
 */

#include "html_input_element.h"
#include "../event.h"
#include "../document.h"
#include "../utils/utf8_utils.h"
#include "core/editing/input_edit_state.h"
#include "core/editing/input_editing_controller.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "include/core/SkRect.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <SDL3/SDL.h>

namespace mbink {

HTMLInputElement::HTMLInputElement()
    : Element("input")
    , input_type_(InputType::Text)
    , edit_state_(CreateInputEditState())
    , checked_(false) {
}

void HTMLInputElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法设置属性
    Element::SetAttribute(name, value);

    // 如果是type属性，同步更新input_type_
    if (name == "type") {
        input_type_ = StringToInputType(value);
    }
    // 如果是checked属性，同步更新checked_（这是默认值）
    else if (name == "checked") {
        if (input_type_ == InputType::Checkbox || input_type_ == InputType::Radio) {
            checked_ = true;  // 有checked属性表示默认选中
        }
    }
    // 如果是value属性，同步更新value_（这是默认值）
    else if (name == "value") {
        if (edit_state_) {
            edit_state_->SetText(value);
            edit_state_->SetCaretPosition(static_cast<int>(utf8::CharCount(value)));
            edit_state_->ClearDirtyFlags();
        }
    }
}

void HTMLInputElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法移除属性
    Element::RemoveAttribute(name);

    // 如果是checked属性，同步更新checked_
    if (name == "checked") {
        if (input_type_ == InputType::Checkbox || input_type_ == InputType::Radio) {
            checked_ = false;  // 移除checked属性表示默认不选中
        }
    }
}

void HTMLInputElement::SetInputType(InputType type) {
    input_type_ = type;
    SetAttribute("type", InputTypeToString(type));

    if (type == InputType::Checkbox || type == InputType::Radio) {
        checked_ = false;
    }
}

bool HTMLInputElement::SupportsTextEditing() const {
    return input_type_ == InputType::Text ||
           input_type_ == InputType::Password ||
           input_type_ == InputType::Search ||
           input_type_ == InputType::Email ||
           input_type_ == InputType::Tel ||
           input_type_ == InputType::Url ||
           input_type_ == InputType::Number;
}

void HTMLInputElement::RequestInputRepaint() {
    MarkDirty(DirtyType::PAINT);

    auto doc = GetOwnerDocument();
    if (!doc) {
        return;
    }

    Window* window = doc->GetWindow();
    if (!window) {
        return;
    }

    auto rect = GetBoundingClientRect();
    if (rect.width > 0.0f && rect.height > 0.0f) {
        SkRect dirty_rect = SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height);
        window->AddDirtyRect(dirty_rect);

        if (auto* pipeline = window->GetRenderPipeline()) {
            pipeline->MarkDirtyRegion(dirty_rect);
            pipeline->MarkNeedsPaint();
        }
    }

    window->SetNeedsRepaint();
}


bool HTMLInputElement::ApplyEditCommand(const InputEditCommand& command) {
    if (!edit_state_) {
        return false;
    }

    InputEditingController controller(this, edit_state_);
    return controller.ApplyCommand(command);
}

void HTMLInputElement::SetValue(const std::string& value, bool trigger_events) {
    if (!edit_state_) {
        return;
    }

    int max_length = GetMaxLength();
    std::string new_value = value;
    if (max_length > 0 && static_cast<int>(utf8::CharCount(new_value)) > max_length) {
        new_value = utf8::SubstrByChar(new_value, 0, max_length);
    }

    std::string old_value = edit_state_->text;
    edit_state_->SetText(new_value);

    int new_length = static_cast<int>(utf8::CharCount(new_value));
    int anchor = std::min(edit_state_->selection_anchor, new_length);
    int focus = std::min(edit_state_->selection_focus, new_length);
    edit_state_->SetSelection(anchor, focus);

    if (old_value != new_value) {
        RequestInputRepaint();
    }

    if (trigger_events && old_value != new_value) {
        TriggerInputEvent();
        TriggerChangeEvent();
    }
}

bool HTMLInputElement::GetChecked() const {
    // 返回当前checked状态
    // checked_在SetAttribute("checked")时被设置为默认值
    // 在SetChecked()时被设置为当前值
    return checked_;
}

void HTMLInputElement::SetChecked(bool checked, bool trigger_events) {
    if (input_type_ != InputType::Checkbox && input_type_ != InputType::Radio) {
        return;  // 只有checkbox和radio支持checked
    }

    bool old_checked = checked_;
    checked_ = checked;

    // 注意：不更新checked属性，checked属性保持为默认值
    // 这符合HTML标准：checked属性是默认值，checked_是当前值

    // 标记需要重绘（checkbox 视觉状态改变）
    if (old_checked != checked) {
        RequestInputRepaint();
    }

    // 触发change事件
    if (trigger_events && old_checked != checked) {
        TriggerChangeEvent();
    }
}

int HTMLInputElement::GetMaxLength() const {
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

void HTMLInputElement::SetMaxLength(int max_length) {
    if (max_length < 0) {
        RemoveAttribute("maxlength");
    } else {
        SetAttribute("maxlength", std::to_string(max_length));
    }
}

void HTMLInputElement::SetDisabled(bool disabled) {
    if (disabled) {
        SetAttribute("disabled", "");
        // 禁用时移除焦点
        // TODO: 调用Blur()方法
    } else {
        RemoveAttribute("disabled");
    }
}

void HTMLInputElement::SetReadOnly(bool readonly) {
    if (readonly) {
        SetAttribute("readonly", "");
    } else {
        RemoveAttribute("readonly");
    }
}

void HTMLInputElement::SetRequired(bool required) {
    if (required) {
        SetAttribute("required", "");
    } else {
        RemoveAttribute("required");
    }
}

bool HTMLInputElement::CheckValidity() const {
    // 检查required
    if (IsRequired()) {
        if (input_type_ == InputType::Checkbox || input_type_ == InputType::Radio) {
            if (!checked_) {
                return false;
            }
        } else {
            if (edit_state_ && edit_state_->text.empty()) {
                return false;
            }
        }
    }

    // 检查maxlength
    int max_length = GetMaxLength();
    if (max_length > 0 && edit_state_ && static_cast<int>(utf8::CharCount(edit_state_->text)) > max_length) {
        return false;
    }

    // 检查pattern（如果有）
    std::string pattern = GetAttribute("pattern");
    const std::string value = GetValue();
    if (!pattern.empty() && !value.empty()) {
        // TODO: 实现正则表达式验证
        // 需要引入regex库
    }

    if (input_type_ == InputType::Email) {
        if (!value.empty() && value.find('@') == std::string::npos) {
            return false;
        }
    } else if (input_type_ == InputType::Url) {
        if (!value.empty() &&
            value.find("http://") != 0 &&
            value.find("https://") != 0) {
            return false;
        }
    }

    return true;
}

std::string HTMLInputElement::GetValidationMessage() const {
    if (CheckValidity()) {
        return "";
    }

    const std::string value = GetValue();
    if (IsRequired() && value.empty()) {
        return "Please fill out this field.";
    }

    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(utf8::CharCount(value)) > max_length) {
        return "Please use no more than " + std::to_string(max_length) + " characters.";
    }

    if (input_type_ == InputType::Email && value.find('@') == std::string::npos) {
        return "Please include an '@' in the email address.";
    }

    if (input_type_ == InputType::Url) {
        return "Please enter a URL.";
    }

    return "Invalid value.";
}

void HTMLInputElement::Select() {
    if (input_type_ != InputType::Text &&
        input_type_ != InputType::Password &&
        input_type_ != InputType::Search &&
        input_type_ != InputType::Email &&
        input_type_ != InputType::Tel &&
        input_type_ != InputType::Url) {
        return;
    }

    if (!edit_state_) {
        return;
    }

    edit_state_->SetSelection(0, static_cast<int>(utf8::CharCount(edit_state_->text)));
}

void HTMLInputElement::SetSelectionRange(int start, int end) {
    if (!edit_state_) {
        return;
    }

    int len = static_cast<int>(utf8::CharCount(edit_state_->text));
    start = std::max(0, std::min(start, len));
    end = std::max(0, std::min(end, len));
    edit_state_->SetSelection(start, end);
}

void HTMLInputElement::HandleTextInput(const std::string& text) {
    if (IsDisabled() || IsReadOnly() || !edit_state_ || !SupportsTextEditing()) {
        return;
    }

    int max_length = GetMaxLength();
    if (max_length > 0) {
        std::string candidate = edit_state_->text;
        int start = edit_state_->GetSelectionStart();
        int end = edit_state_->GetSelectionEnd();
        size_t start_byte = utf8::CharPosToBytePos(candidate, start);
        size_t end_byte = utf8::CharPosToBytePos(candidate, end);
        candidate = candidate.substr(0, start_byte) + text + candidate.substr(end_byte);
        if (static_cast<int>(utf8::CharCount(candidate)) > max_length) {
            return;
        }
    }

    ApplyEditCommand(InputEditCommand::InsertText(text));
}

bool HTMLInputElement::ExecuteEditCommand(const InputEditCommand& command) {
    if (IsDisabled()) {
        return false;
    }

    bool is_read_only_command = command.type == InputEditCommandType::MoveCaretLeft ||
                                command.type == InputEditCommandType::MoveCaretRight ||
                                command.type == InputEditCommandType::MoveCaretToStart ||
                                command.type == InputEditCommandType::MoveCaretToEnd ||
                                command.type == InputEditCommandType::ExtendSelectionLeft ||
                                command.type == InputEditCommandType::ExtendSelectionRight ||
                                command.type == InputEditCommandType::ExtendSelectionToStart ||
                                command.type == InputEditCommandType::ExtendSelectionToEnd ||
                                command.type == InputEditCommandType::SelectAll ||
                                command.type == InputEditCommandType::SetSelection ||
                                command.type == InputEditCommandType::SetCaret;

    if (!is_read_only_command && IsReadOnly()) {
        return false;
    }

    if ((command.type == InputEditCommandType::InsertText ||
         command.type == InputEditCommandType::PasteText) && edit_state_) {
        int max_length = GetMaxLength();
        if (max_length > 0) {
            std::string text = std::get<InsertTextCommandData>(command.payload).text;
            std::string candidate = edit_state_->text;
            int start = edit_state_->GetSelectionStart();
            int end = edit_state_->GetSelectionEnd();
            size_t start_byte = utf8::CharPosToBytePos(candidate, start);
            size_t end_byte = utf8::CharPosToBytePos(candidate, end);
            candidate = candidate.substr(0, start_byte) + text + candidate.substr(end_byte);
            if (static_cast<int>(utf8::CharCount(candidate)) > max_length) {
                return false;
            }
        }
    }

    return ApplyEditCommand(command);
}


void HTMLInputElement::HandleKeyPress(const std::string& key, bool ctrl_key) {
    bool is_navigation_key = (key == "ArrowLeft" || key == "ArrowRight" ||
                              key == "Home" || key == "End");
    bool is_clipboard_read = ctrl_key && (key == "c" || key == "C");

    if (!is_navigation_key && !is_clipboard_read && (IsDisabled() || IsReadOnly())) {
        return;
    }

    if (!edit_state_ || !SupportsTextEditing()) {
        return;
    }

    if (key == "Backspace") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::DeleteBackward));
    } else if (key == "Delete") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::DeleteForward));
    } else if (key == "ArrowLeft") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::MoveCaretLeft));
    } else if (key == "ArrowRight") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::MoveCaretRight));
    } else if (key == "Home") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::MoveCaretToStart));
    } else if (key == "End") {
        ApplyEditCommand(MakeCommand(InputEditCommandType::MoveCaretToEnd));
    } else if (key == "Enter") {
        TriggerChangeEvent();
    } else if (ctrl_key && (key == "a" || key == "A")) {
        ApplyEditCommand(MakeCommand(InputEditCommandType::SelectAll));
    } else if (ctrl_key && (key == "c" || key == "C")) {
        if (edit_state_->HasSelection()) {
            std::string selected_text = utf8::SubstrByChar(edit_state_->text,
                                                           edit_state_->GetSelectionStart(),
                                                           edit_state_->GetSelectionEnd());
            SDL_SetClipboardText(selected_text.c_str());
        }
    } else if (ctrl_key && (key == "x" || key == "X")) {
        if (!IsReadOnly()) {
            ApplyEditCommand(MakeCommand(InputEditCommandType::CutSelection));
        }
    } else if (ctrl_key && (key == "v" || key == "V")) {
        if (!IsReadOnly()) {
            char* clipboard_text = SDL_GetClipboardText();
            if (clipboard_text && clipboard_text[0] != '\0') {
                ApplyEditCommand(InputEditCommand::PasteText(clipboard_text));
            }
            SDL_free(clipboard_text);
        }
    }
}

void HTMLInputElement::TriggerChangeEvent() {
    auto change_event = std::make_shared<Event>("change");
    DispatchEvent(change_event);
}

void HTMLInputElement::TriggerInputEvent() {
    auto input_event = std::make_shared<Event>("input");
    DispatchEvent(input_event);
}

std::string HTMLInputElement::InputTypeToString(InputType type) {
    switch (type) {
        case InputType::Text: return "text";
        case InputType::Password: return "password";
        case InputType::Checkbox: return "checkbox";
        case InputType::Radio: return "radio";
        case InputType::Button: return "button";
        case InputType::Submit: return "submit";
        case InputType::Reset: return "reset";
        case InputType::Hidden: return "hidden";
        case InputType::Number: return "number";
        case InputType::Email: return "email";
        case InputType::Tel: return "tel";
        case InputType::Url: return "url";
        case InputType::Search: return "search";
        case InputType::Date: return "date";
        case InputType::Time: return "time";
        case InputType::Color: return "color";
        case InputType::Range: return "range";
        case InputType::File: return "file";
        default: return "text";
    }
}

InputType HTMLInputElement::StringToInputType(const std::string& type_str) {
    if (type_str == "password") return InputType::Password;
    if (type_str == "checkbox") return InputType::Checkbox;
    if (type_str == "radio") return InputType::Radio;
    if (type_str == "button") return InputType::Button;
    if (type_str == "submit") return InputType::Submit;
    if (type_str == "reset") return InputType::Reset;
    if (type_str == "hidden") return InputType::Hidden;
    if (type_str == "number") return InputType::Number;
    if (type_str == "email") return InputType::Email;
    if (type_str == "tel") return InputType::Tel;
    if (type_str == "url") return InputType::Url;
    if (type_str == "search") return InputType::Search;
    if (type_str == "date") return InputType::Date;
    if (type_str == "time") return InputType::Time;
    if (type_str == "color") return InputType::Color;
    if (type_str == "range") return InputType::Range;
    if (type_str == "file") return InputType::File;
    return InputType::Text;
}

void HTMLInputElement::HandleMouseDown(float local_x, float local_y) {
    // 只有文本类型支持鼠标选择
    if (input_type_ != InputType::Text &&
        input_type_ != InputType::Password &&
        input_type_ != InputType::Search &&
        input_type_ != InputType::Email &&
        input_type_ != InputType::Tel &&
        input_type_ != InputType::Url &&
        input_type_ != InputType::Number) {
        return;
    }

    // 开始拖动选择
    is_dragging_selection_ = true;

    // 计算字符位置需要字体信息，这里只存储鼠标位置
    // 实际的字符位置计算在渲染层（EventLoop）中完成
    // 通过 SetCursorPositionFromX 方法设置光标位置
}

void HTMLInputElement::HandleMouseMove(float local_x, float local_y) {
    if (!is_dragging_selection_) {
        return;
    }
    // 字符位置计算在渲染层完成
}

void HTMLInputElement::HandleMouseUp() {
    is_dragging_selection_ = false;
}

void HTMLInputElement::SetCursorPosition(int char_pos) {
    if (!edit_state_) {
        return;
    }

    size_t char_count = utf8::CharCount(edit_state_->text);
    if (char_pos < 0) {
        char_pos = 0;
    } else if (char_pos > static_cast<int>(char_count)) {
        char_pos = static_cast<int>(char_count);
    }

    edit_state_->SetCaretPosition(char_pos);
    RequestInputRepaint();
}

void HTMLInputElement::SetSelection(int start, int end) {
    if (!edit_state_) {
        return;
    }

    size_t char_count = utf8::CharCount(edit_state_->text);
    if (start > end) {
        std::swap(start, end);
    }

    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > static_cast<int>(char_count)) start = static_cast<int>(char_count);
    if (end > static_cast<int>(char_count)) end = static_cast<int>(char_count);

    edit_state_->SetSelection(start, end);
    RequestInputRepaint();
}

void HTMLInputElement::StepUp() {
    if (input_type_ != InputType::Number) {
        return;
    }

    // 获取当前值
    double current_value = 0.0;
    try {
        const std::string value = GetValue();
        if (!value.empty()) {
            current_value = std::stod(value);
        }
    } catch (...) {
        current_value = 0.0;
    }

    // 获取 step 属性（默认为 1）
    double step = 1.0;
    std::string step_attr = GetAttribute("step");
    if (!step_attr.empty() && step_attr != "any") {
        try {
            step = std::stod(step_attr);
        } catch (...) {
            step = 1.0;
        }
    }

    // 增加值
    current_value += step;

    // 检查 max 限制
    std::string max_attr = GetAttribute("max");
    if (!max_attr.empty()) {
        try {
            double max_val = std::stod(max_attr);
            if (current_value > max_val) {
                current_value = max_val;
            }
        } catch (...) {}
    }

    // 更新值
    // 如果是整数，去掉小数点
    if (step == std::floor(step) && current_value == std::floor(current_value)) {
        SetValue(std::to_string(static_cast<long long>(current_value)));
    } else {
        SetValue(std::to_string(current_value));
    }
}

void HTMLInputElement::StepDown() {
    if (input_type_ != InputType::Number) {
        return;
    }

    // 获取当前值
    double current_value = 0.0;
    try {
        const std::string value = GetValue();
        if (!value.empty()) {
            current_value = std::stod(value);
        }
    } catch (...) {
        current_value = 0.0;
    }

    // 获取 step 属性（默认为 1）
    double step = 1.0;
    std::string step_attr = GetAttribute("step");
    if (!step_attr.empty() && step_attr != "any") {
        try {
            step = std::stod(step_attr);
        } catch (...) {
            step = 1.0;
        }
    }

    // 减少值
    current_value -= step;

    // 检查 min 限制
    std::string min_attr = GetAttribute("min");
    if (!min_attr.empty()) {
        try {
            double min_val = std::stod(min_attr);
            if (current_value < min_val) {
                current_value = min_val;
            }
        } catch (...) {}
    }

    // 更新值
    // 如果是整数，去掉小数点
    if (step == std::floor(step) && current_value == std::floor(current_value)) {
        SetValue(std::to_string(static_cast<long long>(current_value)));
    } else {
        SetValue(std::to_string(current_value));
    }
}

double HTMLInputElement::GetMin() const {
    std::string min_attr = GetAttribute("min");
    if (!min_attr.empty()) {
        try {
            return std::stod(min_attr);
        } catch (...) {}
    }
    // range 类型默认 min 为 0
    if (input_type_ == InputType::Range) {
        return 0.0;
    }
    return 0.0;
}

double HTMLInputElement::GetMax() const {
    std::string max_attr = GetAttribute("max");
    if (!max_attr.empty()) {
        try {
            return std::stod(max_attr);
        } catch (...) {}
    }
    // range 类型默认 max 为 100
    if (input_type_ == InputType::Range) {
        return 100.0;
    }
    return 100.0;
}

double HTMLInputElement::GetValueAsNumber() const {
    const std::string value = GetValue();
    if (value.empty()) {
        if (input_type_ == InputType::Range) {
            return (GetMin() + GetMax()) / 2.0;
        }
        return 0.0;
    }
    try {
        return std::stod(value);
    } catch (...) {
        if (input_type_ == InputType::Range) {
            return (GetMin() + GetMax()) / 2.0;
        }
        return 0.0;
    }
}

// ========== Range 滑块拖动实现 ==========

void HTMLInputElement::StartRangeDrag(float track_width) {
    if (input_type_ != InputType::Range) return;
    is_dragging_range_ = true;
}

void HTMLInputElement::UpdateRangeDrag(float local_x, float track_width) {
    if (input_type_ != InputType::Range || !is_dragging_range_) return;
    if (track_width <= 0) return;

    // 计算位置比例 (0.0 - 1.0)
    float position = std::clamp(local_x / track_width, 0.0f, 1.0f);

    // 转换为值
    double min_val = GetMin();
    double max_val = GetMax();
    double new_value = min_val + position * (max_val - min_val);

    // 应用 step（如果有）
    std::string step_attr = GetAttribute("step");
    if (!step_attr.empty() && step_attr != "any") {
        try {
            double step = std::stod(step_attr);
            if (step > 0) {
                // 四舍五入到最近的 step
                new_value = min_val + std::round((new_value - min_val) / step) * step;
            }
        } catch (...) {}
    }

    // 限制在范围内
    new_value = std::clamp(new_value, min_val, max_val);

    // 设置新值
    // 对于整数 step，保持整数格式
    double step = 1.0;
    if (!step_attr.empty() && step_attr != "any") {
        try { step = std::stod(step_attr); } catch (...) {}
    }

    if (step == std::floor(step) && new_value == std::floor(new_value)) {
        SetValue(std::to_string(static_cast<long long>(new_value)));
    } else {
        SetValue(std::to_string(new_value));
    }

    // 触发 input 事件
    TriggerInputEvent();
}

void HTMLInputElement::EndRangeDrag() {
    if (is_dragging_range_) {
        is_dragging_range_ = false;
        // 触发 change 事件
        TriggerChangeEvent();
    }
}

} // namespace mbink

