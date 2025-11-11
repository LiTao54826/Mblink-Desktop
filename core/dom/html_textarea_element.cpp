/**
 * @file html_textarea_element.cpp
 * @brief HTML TextArea元素类实现
 */

#include "html_textarea_element.h"
#include "event.h"
#include <algorithm>

namespace lightui {

HTMLTextAreaElement::HTMLTextAreaElement()
    : Element("textarea")
    , value_("")
    , selection_start_(0)
    , selection_end_(0) {
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
    selection_end_ = static_cast<int>(value_.length());
}

void HTMLTextAreaElement::SetSelectionRange(int start, int end) {
    int len = static_cast<int>(value_.length());
    selection_start_ = std::max(0, std::min(start, len));
    selection_end_ = std::max(selection_start_, std::min(end, len));
}

void HTMLTextAreaElement::HandleTextInput(const std::string& text) {
    // 检查是否可编辑
    if (IsDisabled() || IsReadOnly()) {
        return;
    }
    
    // 在光标位置插入文本
    std::string new_value = value_;
    if (selection_start_ != selection_end_) {
        // 有选中文本，替换选中部分
        new_value = value_.substr(0, selection_start_) + 
                   text + 
                   value_.substr(selection_end_);
    } else {
        // 无选中文本，在光标位置插入
        new_value = value_.substr(0, selection_start_) + 
                   text + 
                   value_.substr(selection_start_);
    }
    
    // 检查maxlength
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(new_value.length()) > max_length) {
        return;  // 超过最大长度，忽略输入
    }
    
    value_ = new_value;
    selection_start_ += static_cast<int>(text.length());
    selection_end_ = selection_start_;
    
    // 触发input事件
    TriggerInputEvent();
}

void HTMLTextAreaElement::HandleKeyPress(const std::string& key, bool ctrl_key) {
    // 检查是否可编辑
    if (IsDisabled() || IsReadOnly()) {
        return;
    }
    
    // 处理特殊按键
    if (key == "Backspace") {
        if (selection_start_ != selection_end_) {
            // 删除选中文本
            value_ = value_.substr(0, selection_start_) + value_.substr(selection_end_);
            selection_end_ = selection_start_;
        } else if (selection_start_ > 0) {
            // 删除光标前一个字符
            value_ = value_.substr(0, selection_start_ - 1) + value_.substr(selection_start_);
            selection_start_--;
            selection_end_ = selection_start_;
        }
        TriggerInputEvent();
        
    } else if (key == "Delete") {
        if (selection_start_ != selection_end_) {
            // 删除选中文本
            value_ = value_.substr(0, selection_start_) + value_.substr(selection_end_);
            selection_end_ = selection_start_;
        } else if (selection_start_ < static_cast<int>(value_.length())) {
            // 删除光标后一个字符
            value_ = value_.substr(0, selection_start_) + value_.substr(selection_start_ + 1);
        }
        TriggerInputEvent();
        
    } else if (key == "Enter") {
        // TextArea支持换行
        HandleTextInput("\n");
        
    } else if (ctrl_key && key == "a") {
        // Ctrl+A 全选
        Select();
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

} // namespace lightui

