/**
 * @file html_select_element.cpp
 * @brief HTML Select元素实现
 */

#include "html_select_element.h"
#include "html_form_element.h"
#include "../event.h"
#include <algorithm>

namespace mbink {

// ========== 构造函数 ==========

HTMLSelectElement::HTMLSelectElement()
    : Element("select")
    , disabled_(false)
    , multiple_(false)
    , required_(false)
    , size_(0) {
}

// ========== IDL属性实现 ==========

void HTMLSelectElement::SetDisabled(bool disabled) {
    if (disabled_ == disabled) {
        return;
    }
    
    disabled_ = disabled;
    
    // 更新属性
    if (disabled) {
        Element::SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
    
    // 更新伪类
    UpdatePseudoClasses();
}

std::shared_ptr<HTMLFormElement> HTMLSelectElement::GetForm() const {
    return FindForm();
}

void HTMLSelectElement::SetMultiple(bool multiple) {
    if (multiple_ == multiple) {
        return;
    }
    
    multiple_ = multiple;
    
    // 更新属性
    if (multiple) {
        Element::SetAttribute("multiple", "");
    } else {
        RemoveAttribute("multiple");
    }
}

void HTMLSelectElement::SetName(const std::string& name) {
    if (name_ == name) {
        return;
    }
    
    name_ = name;
    Element::SetAttribute("name", name);
}

void HTMLSelectElement::SetRequired(bool required) {
    if (required_ == required) {
        return;
    }
    
    required_ = required;
    
    // 更新属性
    if (required) {
        Element::SetAttribute("required", "");
    } else {
        RemoveAttribute("required");
    }
}

void HTMLSelectElement::SetSize(unsigned long size) {
    if (size_ == size) {
        return;
    }
    
    size_ = size;
    Element::SetAttribute("size", std::to_string(size));
}

std::string HTMLSelectElement::GetType() const {
    return multiple_ ? "select-multiple" : "select-one";
}

std::vector<std::shared_ptr<HTMLOptionElement>> HTMLSelectElement::GetOptions() const {
    std::vector<std::shared_ptr<HTMLOptionElement>> options;
    
    // 递归收集所有option子元素
    std::function<void(std::shared_ptr<Node>)> collect_options = [&](std::shared_ptr<Node> node) {
        auto children = node->GetChildNodes();
        for (const auto& child : children) {
            if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::static_pointer_cast<Element>(child);
                if (elem->GetTagName() == "option") {
                    auto option = std::static_pointer_cast<HTMLOptionElement>(elem);
                    options.push_back(option);
                } else if (elem->GetTagName() == "optgroup") {
                    // optgroup中也可以有option
                    collect_options(elem);
                }
            }
        }
    };
    
    collect_options(std::static_pointer_cast<Node>(const_cast<HTMLSelectElement*>(this)->shared_from_this()));
    
    return options;
}

unsigned long HTMLSelectElement::GetLength() const {
    return static_cast<unsigned long>(GetOptions().size());
}

std::vector<std::shared_ptr<HTMLOptionElement>> HTMLSelectElement::GetSelectedOptions() const {
    std::vector<std::shared_ptr<HTMLOptionElement>> selected;
    auto options = GetOptions();
    
    for (const auto& option : options) {
        if (option->GetSelected()) {
            selected.push_back(option);
        }
    }
    
    return selected;
}

long HTMLSelectElement::GetSelectedIndex() const {
    auto options = GetOptions();
    
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i]->GetSelected()) {
            return static_cast<long>(i);
        }
    }
    
    return -1;
}

void HTMLSelectElement::SetSelectedIndex(long index) {
    auto options = GetOptions();
    
    // 如果是单选，先取消所有选中
    if (!multiple_) {
        for (auto& option : options) {
            option->SetSelected(false);
        }
    }
    
    // 选中指定索引
    if (index >= 0 && index < static_cast<long>(options.size())) {
        options[index]->SetSelected(true);
    }
}

std::string HTMLSelectElement::GetValue() const {
    auto selected_options = GetSelectedOptions();
    if (!selected_options.empty()) {
        return selected_options[0]->GetValue();
    }
    return "";
}

void HTMLSelectElement::SetValue(const std::string& value) {
    auto options = GetOptions();
    
    // 如果是单选，先取消所有选中
    if (!multiple_) {
        for (auto& option : options) {
            option->SetSelected(false);
        }
    }
    
    // 选中匹配value的第一个option
    for (auto& option : options) {
        if (option->GetValue() == value) {
            option->SetSelected(true);
            if (!multiple_) {
                break;  // 单选只选中第一个
            }
        }
    }
}

// ========== 验证方法 ==========

bool HTMLSelectElement::CheckValidity() const {
    // 如果有自定义验证消息，无效
    if (!custom_validity_.empty()) {
        return false;
    }
    
    // 如果required，必须有选中项
    if (required_) {
        auto selected = GetSelectedOptions();
        if (selected.empty()) {
            return false;
        }
        // 如果选中的是空值option，也无效
        if (selected[0]->GetValue().empty()) {
            return false;
        }
    }
    
    return true;
}

bool HTMLSelectElement::ReportValidity() const {
    bool valid = CheckValidity();
    
    // TODO: 显示验证消息UI
    // 目前只返回验证结果
    
    return valid;
}

// ========== 重写方法 ==========

void HTMLSelectElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法
    Element::SetAttribute(name, value);
    
    // 处理特殊属性
    if (name == "disabled") {
        disabled_ = true;
        UpdatePseudoClasses();
    } else if (name == "multiple") {
        multiple_ = true;
    } else if (name == "required") {
        required_ = true;
    } else if (name == "name") {
        name_ = value;
    } else if (name == "size") {
        try {
            size_ = std::stoul(value);
        } catch (...) {
            size_ = 0;
        }
    }
}

void HTMLSelectElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法
    Element::RemoveAttribute(name);
    
    // 处理特殊属性
    if (name == "disabled") {
        disabled_ = false;
        UpdatePseudoClasses();
    } else if (name == "multiple") {
        multiple_ = false;
    } else if (name == "required") {
        required_ = false;
    } else if (name == "name") {
        name_.clear();
    } else if (name == "size") {
        size_ = 0;
    }
}

// ========== 内部方法 ==========

void HTMLSelectElement::OnOptionSelectionChanged(std::shared_ptr<HTMLOptionElement> option) {
    // 如果是单选模式，取消其他option的选中状态
    if (!multiple_ && option->GetSelected()) {
        auto options = GetOptions();
        for (auto& opt : options) {
            if (opt != option && opt->GetSelected()) {
                opt->SetSelected(false);
            }
        }
    }
    
    // 触发change事件
    TriggerChangeEvent();
}

// ========== 辅助方法 ==========

void HTMLSelectElement::UpdatePseudoClasses() {
    SetPseudoClass(":disabled", disabled_);
    SetPseudoClass(":enabled", !disabled_);
}

std::shared_ptr<HTMLFormElement> HTMLSelectElement::FindForm() const {
    auto parent = GetParentNode();
    while (parent) {
        auto elem = std::dynamic_pointer_cast<Element>(parent);
        if (elem && elem->GetTagName() == "form") {
            return std::dynamic_pointer_cast<HTMLFormElement>(elem);
        }
        parent = parent->GetParentNode();
    }
    return nullptr;
}

void HTMLSelectElement::TriggerChangeEvent() {
    // 创建change事件
    auto event = std::make_shared<Event>("change");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());

    // 触发事件
    DispatchEvent(event);
}

void HTMLSelectElement::HandleClick() {
    // 如果禁用，不处理
    if (disabled_) {
        return;
    }

    // 切换下拉菜单状态
    is_dropdown_open_ = !is_dropdown_open_;

    // 打开下拉菜单时，设置悬停索引为当前选中的索引
    if (is_dropdown_open_) {
        hovered_index_ = GetSelectedIndex();
    }
}

void HTMLSelectElement::SelectHoveredOption() {
    if (hovered_index_ >= 0) {
        auto options = GetOptions();
        if (hovered_index_ < static_cast<long>(options.size())) {
            if (!options[hovered_index_]->GetDisabled()) {
                SetSelectedIndex(hovered_index_);
            }
        }
    }
    is_dropdown_open_ = false;
    hovered_index_ = -1;
}

void HTMLSelectElement::SelectNextOption() {
    auto options = GetOptions();
    if (options.empty()) return;

    long current = GetSelectedIndex();
    long next = current + 1;

    // 循环到第一个
    if (next >= static_cast<long>(options.size())) {
        next = 0;
    }

    // 跳过禁用的选项
    long start = next;
    while (options[next]->GetDisabled()) {
        next = (next + 1) % static_cast<long>(options.size());
        if (next == start) {
            // 所有选项都禁用，不做任何事
            return;
        }
    }

    SetSelectedIndex(next);
}

void HTMLSelectElement::SelectPreviousOption() {
    auto options = GetOptions();
    if (options.empty()) return;

    long current = GetSelectedIndex();
    long prev = current - 1;

    // 循环到最后一个
    if (prev < 0) {
        prev = static_cast<long>(options.size()) - 1;
    }

    // 跳过禁用的选项
    long start = prev;
    while (options[prev]->GetDisabled()) {
        prev = prev - 1;
        if (prev < 0) {
            prev = static_cast<long>(options.size()) - 1;
        }
        if (prev == start) {
            // 所有选项都禁用，不做任何事
            return;
        }
    }

    SetSelectedIndex(prev);
}

} // namespace mbink

