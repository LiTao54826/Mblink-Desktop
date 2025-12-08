/**
 * @file html_input_element.cpp
 * @brief HTML Input元素类实现
 */

#include "html_input_element.h"
#include "event.h"
#include "../utils/utf8_utils.h"
#include "../window/window_manager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <SDL3/SDL.h>

namespace lightui {

HTMLInputElement::HTMLInputElement()
    : Element("input")
    , input_type_(InputType::Text)
    , value_("")
    , checked_(false)
    , selection_start_(0)
    , selection_end_(0) {
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
        value_ = value;
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
    
    // 某些类型切换时需要重置value
    if (type == InputType::Checkbox || type == InputType::Radio) {
        // checkbox和radio不使用value作为显示值
        checked_ = false;
    }
}

void HTMLInputElement::SetValue(const std::string& value, bool trigger_events) {
    // 检查maxlength限制
    int max_length = GetMaxLength();
    std::string new_value = value;
    if (max_length > 0 && static_cast<int>(value.length()) > max_length) {
        new_value = value.substr(0, max_length);
    }

    std::string old_value = value_;
    value_ = new_value;

    // 调整选择范围，确保不越界
    int new_length = static_cast<int>(new_value.length());
    if (selection_start_ > new_length) {
        selection_start_ = new_length;
    }
    if (selection_end_ > new_length) {
        selection_end_ = new_length;
    }

    // 注意：不更新value属性，value属性保持为默认值
    // 这符合HTML标准：value属性是默认值，value_是当前值

    // 触发事件
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
            if (value_.empty()) {
                return false;
            }
        }
    }
    
    // 检查maxlength
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(value_.length()) > max_length) {
        return false;
    }
    
    // 检查pattern（如果有）
    std::string pattern = GetAttribute("pattern");
    if (!pattern.empty() && !value_.empty()) {
        // TODO: 实现正则表达式验证
        // 需要引入regex库
    }
    
    // 检查type特定的验证
    if (input_type_ == InputType::Email) {
        // 简单的邮箱验证
        if (!value_.empty() && value_.find('@') == std::string::npos) {
            return false;
        }
    } else if (input_type_ == InputType::Url) {
        // 简单的URL验证
        if (!value_.empty() && 
            value_.find("http://") != 0 && 
            value_.find("https://") != 0) {
            return false;
        }
    }
    
    return true;
}

std::string HTMLInputElement::GetValidationMessage() const {
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
    
    if (input_type_ == InputType::Email && value_.find('@') == std::string::npos) {
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
        return;  // 只有文本类型支持选择
    }

    selection_start_ = 0;
    selection_end_ = static_cast<int>(utf8::CharCount(value_));
}

void HTMLInputElement::SetSelectionRange(int start, int end) {
    int len = static_cast<int>(utf8::CharCount(value_));
    selection_start_ = std::max(0, std::min(start, len));
    selection_end_ = std::max(selection_start_, std::min(end, len));
}

void HTMLInputElement::HandleTextInput(const std::string& text) {
    // 检查是否可编辑
    if (IsDisabled() || IsReadOnly()) {
        return;
    }

    // 只有文本类型支持文本输入
    if (input_type_ != InputType::Text &&
        input_type_ != InputType::Password &&
        input_type_ != InputType::Search &&
        input_type_ != InputType::Email &&
        input_type_ != InputType::Tel &&
        input_type_ != InputType::Url &&
        input_type_ != InputType::Number) {
        return;
    }

    // 使用 UTF-8 工具处理文本插入
    std::string new_value = value_;
    size_t text_char_count = utf8::CharCount(text);

    if (selection_start_ != selection_end_) {
        // 有选中文本，替换选中部分
        // 先删除选中区域
        size_t start_byte = utf8::CharPosToBytePos(value_, selection_start_);
        size_t end_byte = utf8::CharPosToBytePos(value_, selection_end_);
        new_value = value_.substr(0, start_byte) + text + value_.substr(end_byte);
        selection_end_ = selection_start_;  // 重置选择
    } else {
        // 无选中文本，在光标位置插入
        size_t byte_pos = utf8::CharPosToBytePos(value_, selection_start_);
        new_value = value_.substr(0, byte_pos) + text + value_.substr(byte_pos);
    }

    // 检查maxlength（按字符数检查，不是字节数）
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(utf8::CharCount(new_value)) > max_length) {
        return;  // 超过最大长度，忽略输入
    }

    value_ = new_value;
    selection_start_ += static_cast<int>(text_char_count);
    selection_end_ = selection_start_;

    // 注意：不调用 SetAttribute("value", value_)
    // 因为这会触发 DOM 观察者，导致 Preact 等框架重新渲染整个组件
    // value 属性保持为默认值，value_ 是当前输入值（符合 HTML 标准）

    // 触发input事件
    TriggerInputEvent();
}

void HTMLInputElement::HandleKeyPress(const std::string& key, bool ctrl_key) {
    // 检查是否可编辑（箭头键等导航键不需要可编辑）
    bool is_navigation_key = (key == "ArrowLeft" || key == "ArrowRight" ||
                              key == "Home" || key == "End");
    bool is_clipboard_read = ctrl_key && (key == "c" || key == "C");  // 复制不需要可编辑

    if (!is_navigation_key && !is_clipboard_read && (IsDisabled() || IsReadOnly())) {
        return;
    }

    size_t char_count = utf8::CharCount(value_);

    // 处理特殊按键
    if (key == "Backspace") {
        if (selection_start_ != selection_end_) {
            // 删除选中文本（使用 UTF-8 字符位置）
            size_t start_byte = utf8::CharPosToBytePos(value_, selection_start_);
            size_t end_byte = utf8::CharPosToBytePos(value_, selection_end_);
            value_ = value_.substr(0, start_byte) + value_.substr(end_byte);
            selection_end_ = selection_start_;
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
            size_t start_byte = utf8::CharPosToBytePos(value_, selection_start_);
            size_t end_byte = utf8::CharPosToBytePos(value_, selection_end_);
            value_ = value_.substr(0, start_byte) + value_.substr(end_byte);
            selection_end_ = selection_start_;
        } else if (selection_start_ < static_cast<int>(char_count)) {
            // 删除光标后一个字符（正确处理多字节UTF-8字符）
            size_t curr_byte = utf8::CharPosToBytePos(value_, selection_start_);
            size_t next_byte = utf8::CharPosToBytePos(value_, selection_start_ + 1);
            value_ = value_.substr(0, curr_byte) + value_.substr(next_byte);
        }
        TriggerInputEvent();

    } else if (key == "ArrowLeft") {
        // 左箭头：光标左移一个字符
        if (selection_start_ > 0) {
            selection_start_--;
            selection_end_ = selection_start_;
        }
        // 触发重绘以更新光标位置
        auto& window_manager = WindowManager::Instance();
        for (auto& window : window_manager.GetAllWindows()) {
            window->SetNeedsRepaint();
        }

    } else if (key == "ArrowRight") {
        // 右箭头：光标右移一个字符
        if (selection_start_ < static_cast<int>(char_count)) {
            selection_start_++;
            selection_end_ = selection_start_;
        }
        // 触发重绘以更新光标位置
        auto& window_manager = WindowManager::Instance();
        for (auto& window : window_manager.GetAllWindows()) {
            window->SetNeedsRepaint();
        }

    } else if (key == "Home") {
        // Home：光标移到开头
        selection_start_ = 0;
        selection_end_ = 0;
        // 触发重绘以更新光标位置
        auto& window_manager = WindowManager::Instance();
        for (auto& window : window_manager.GetAllWindows()) {
            window->SetNeedsRepaint();
        }

    } else if (key == "End") {
        // End：光标移到末尾
        selection_start_ = static_cast<int>(char_count);
        selection_end_ = selection_start_;
        // 触发重绘以更新光标位置
        auto& window_manager = WindowManager::Instance();
        for (auto& window : window_manager.GetAllWindows()) {
            window->SetNeedsRepaint();
        }

    } else if (key == "Enter") {
        // Enter键触发change事件
        TriggerChangeEvent();

    } else if (ctrl_key && (key == "a" || key == "A")) {
        // Ctrl+A 全选
        Select();
        // 触发重绘以显示选中状态
        auto& window_manager = WindowManager::Instance();
        for (auto& window : window_manager.GetAllWindows()) {
            window->SetNeedsRepaint();
        }

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
                // 使用 HandleTextInput 来插入文本（它会处理选中区域的替换）
                HandleTextInput(clipboard_text);
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
    std::cout << "[HTMLInputElement::TriggerInputEvent] START this=" << this << std::endl;
    auto input_event = std::make_shared<Event>("input");
    std::cout << "[HTMLInputElement::TriggerInputEvent] About to DispatchEvent" << std::endl;
    DispatchEvent(input_event);
    std::cout << "[HTMLInputElement::TriggerInputEvent] END" << std::endl;
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
    // 确保位置在有效范围内
    size_t char_count = utf8::CharCount(value_);
    if (char_pos < 0) {
        char_pos = 0;
    } else if (char_pos > static_cast<int>(char_count)) {
        char_pos = static_cast<int>(char_count);
    }

    selection_start_ = char_pos;
    selection_end_ = char_pos;

    // 触发重绘
    auto& window_manager = WindowManager::Instance();
    for (auto& window : window_manager.GetAllWindows()) {
        window->SetNeedsRepaint();
    }
}

void HTMLInputElement::SetSelection(int start, int end) {
    size_t char_count = utf8::CharCount(value_);

    // 确保start <= end
    if (start > end) {
        std::swap(start, end);
    }

    // 确保范围在有效范围内
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > static_cast<int>(char_count)) start = static_cast<int>(char_count);
    if (end > static_cast<int>(char_count)) end = static_cast<int>(char_count);

    selection_start_ = start;
    selection_end_ = end;

    // 触发重绘
    auto& window_manager = WindowManager::Instance();
    for (auto& window : window_manager.GetAllWindows()) {
        window->SetNeedsRepaint();
    }
}

void HTMLInputElement::StepUp() {
    if (input_type_ != InputType::Number) {
        return;
    }

    // 获取当前值
    double current_value = 0.0;
    try {
        if (!value_.empty()) {
            current_value = std::stod(value_);
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
        if (!value_.empty()) {
            current_value = std::stod(value_);
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
    if (value_.empty()) {
        // range 类型默认值为 (min + max) / 2
        if (input_type_ == InputType::Range) {
            return (GetMin() + GetMax()) / 2.0;
        }
        return 0.0;
    }
    try {
        return std::stod(value_);
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

} // namespace lightui

