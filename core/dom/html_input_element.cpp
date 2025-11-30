/**
 * @file html_input_element.cpp
 * @brief HTML Input元素类实现
 */

#include "html_input_element.h"
#include "event.h"
#include <algorithm>
#include <iostream>

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
    std::cerr << "[HTMLInputElement::SetValue] START value='" << value << "' trigger_events=" << trigger_events << " this=" << this << std::endl;
    std::cerr << "[HTMLInputElement::SetValue] BEFORE: value_='" << value_ << "' selection_start_=" << selection_start_ << " selection_end_=" << selection_end_ << std::endl;
    std::cerr.flush();

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

    std::cerr << "[HTMLInputElement::SetValue] AFTER: value_='" << value_ << "' selection_start_=" << selection_start_ << " selection_end_=" << selection_end_ << std::endl;
    std::cerr.flush();

    // 注意：不更新value属性，value属性保持为默认值
    // 这符合HTML标准：value属性是默认值，value_是当前值

    // 触发事件
    if (trigger_events && old_value != new_value) {
        TriggerInputEvent();
        TriggerChangeEvent();
    }

    std::cerr << "[HTMLInputElement::SetValue] END" << std::endl;
    std::cerr.flush();
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
    selection_end_ = static_cast<int>(value_.length());
}

void HTMLInputElement::SetSelectionRange(int start, int end) {
    int len = static_cast<int>(value_.length());
    selection_start_ = std::max(0, std::min(start, len));
    selection_end_ = std::max(selection_start_, std::min(end, len));
}

void HTMLInputElement::HandleTextInput(const std::string& text) {
    std::cerr << "[HTMLInputElement::HandleTextInput] START text='" << text << "' this=" << this << std::endl;
    std::cerr.flush();

    std::cerr << "[HTMLInputElement::HandleTextInput] Checking IsDisabled" << std::endl;
    std::cerr.flush();

    // 检查是否可编辑
    bool disabled = IsDisabled();
    std::cerr << "[HTMLInputElement::HandleTextInput] IsDisabled=" << disabled << std::endl;
    std::cerr.flush();

    bool readonly = IsReadOnly();
    std::cerr << "[HTMLInputElement::HandleTextInput] IsReadOnly=" << readonly << std::endl;
    std::cerr.flush();

    if (disabled || readonly) {
        std::cerr << "[HTMLInputElement::HandleTextInput] Disabled or readonly, returning" << std::endl;
        return;
    }

    std::cerr << "[HTMLInputElement::HandleTextInput] Checking input_type_=" << static_cast<int>(input_type_) << std::endl;
    std::cerr.flush();

    // 只有文本类型支持文本输入
    if (input_type_ != InputType::Text &&
        input_type_ != InputType::Password &&
        input_type_ != InputType::Search &&
        input_type_ != InputType::Email &&
        input_type_ != InputType::Tel &&
        input_type_ != InputType::Url &&
        input_type_ != InputType::Number) {
        std::cerr << "[HTMLInputElement::HandleTextInput] Wrong input type, returning" << std::endl;
        return;
    }

    std::cerr << "[HTMLInputElement::HandleTextInput] About to build new_value, value_='" << value_ << "'" << std::endl;
    std::cerr.flush();

    // 在光标位置插入文本
    std::string new_value = value_;
    std::cerr << "[HTMLInputElement::HandleTextInput] selection_start_=" << selection_start_ << " selection_end_=" << selection_end_ << std::endl;
    std::cerr.flush();

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

    std::cerr << "[HTMLInputElement::HandleTextInput] new_value='" << new_value << "'" << std::endl;
    std::cerr.flush();

    // 检查maxlength
    int max_length = GetMaxLength();
    if (max_length > 0 && static_cast<int>(new_value.length()) > max_length) {
        return;  // 超过最大长度，忽略输入
    }

    value_ = new_value;
    selection_start_ += static_cast<int>(text.length());
    selection_end_ = selection_start_;

    // 注意：不调用 SetAttribute("value", value_)
    // 因为这会触发 DOM 观察者，导致 Preact 等框架重新渲染整个组件
    // value 属性保持为默认值，value_ 是当前输入值（符合 HTML 标准）

    std::cerr << "[HTMLInputElement::HandleTextInput] About to TriggerInputEvent" << std::endl;
    std::cerr.flush();
    // 触发input事件
    TriggerInputEvent();
    std::cerr << "[HTMLInputElement::HandleTextInput] END" << std::endl;
    std::cerr.flush();
}

void HTMLInputElement::HandleKeyPress(const std::string& key, bool ctrl_key) {
    // 检查是否可编辑（箭头键等导航键不需要可编辑）
    bool is_navigation_key = (key == "ArrowLeft" || key == "ArrowRight" ||
                              key == "Home" || key == "End");

    if (!is_navigation_key && (IsDisabled() || IsReadOnly())) {
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
        // 注意：不调用 SetAttribute，避免触发 DOM 观察者导致 Preact 重新渲染
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
        // 注意：不调用 SetAttribute，避免触发 DOM 观察者导致 Preact 重新渲染
        TriggerInputEvent();

    } else if (key == "ArrowLeft") {
        // 左箭头：光标左移
        if (selection_start_ > 0) {
            selection_start_--;
            selection_end_ = selection_start_;
        }

    } else if (key == "ArrowRight") {
        // 右箭头：光标右移
        if (selection_start_ < static_cast<int>(value_.length())) {
            selection_start_++;
            selection_end_ = selection_start_;
        }

    } else if (key == "Home") {
        // Home：光标移到开头
        selection_start_ = 0;
        selection_end_ = 0;

    } else if (key == "End") {
        // End：光标移到末尾
        selection_start_ = static_cast<int>(value_.length());
        selection_end_ = selection_start_;

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

} // namespace lightui

