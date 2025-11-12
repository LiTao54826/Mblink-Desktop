/**
 * @file html_input_element.cpp
 * @brief HTML Input元素类实现
 */

#include "html_input_element.h"
#include "event.h"
#include <algorithm>

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
    
    // 更新value属性
    SetAttribute("value", new_value);
    
    // 触发事件
    if (trigger_events && old_value != new_value) {
        TriggerInputEvent();
        TriggerChangeEvent();
    }
}

bool HTMLInputElement::GetChecked() const {
    // 如果checked_已经被程序设置过，使用程序设置的值
    // 否则检查HTML属性
    if (checked_) {
        return true;
    }

    // 检查HTML属性中是否有checked
    return HasAttribute("checked");
}

void HTMLInputElement::SetChecked(bool checked, bool trigger_events) {
    if (input_type_ != InputType::Checkbox && input_type_ != InputType::Radio) {
        return;  // 只有checkbox和radio支持checked
    }

    bool old_checked = checked_;
    checked_ = checked;

    // 更新checked属性
    if (checked) {
        SetAttribute("checked", "");
    } else {
        RemoveAttribute("checked");
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
    selection_end_ = static_cast<int>(value_.length());
}

void HTMLInputElement::SetSelectionRange(int start, int end) {
    int len = static_cast<int>(value_.length());
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
    
    // 更新value属性
    SetAttribute("value", value_);
    
    // 触发input事件
    TriggerInputEvent();
}

void HTMLInputElement::HandleKeyPress(const std::string& key, bool ctrl_key) {
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
        SetAttribute("value", value_);
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
        SetAttribute("value", value_);
        TriggerInputEvent();
        
    } else if (key == "Enter") {
        // Enter键触发change事件
        TriggerChangeEvent();
        
    } else if (ctrl_key && key == "a") {
        // Ctrl+A 全选
        Select();
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

} // namespace lightui

