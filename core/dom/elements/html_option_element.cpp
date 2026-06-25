/**
 * @file html_option_element.cpp
 * @brief HTML Option元素实现
 */

#include "html_option_element.h"
#include "html_select_element.h"
#include "html_form_element.h"
#include "../document.h"
#include "../text.h"
#include <algorithm>

namespace mblink {

// ========== 构造函数 ==========

HTMLOptionElement::HTMLOptionElement()
    : Element("option")
    , disabled_(false)
    , default_selected_(false)
    , selected_(false) {
}

// ========== IDL属性实现 ==========

void HTMLOptionElement::SetDisabled(bool disabled) {
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

std::string HTMLOptionElement::GetLabel() const {
    std::string label = GetAttribute("label");
    if (!label.empty()) {
        return label;
    }
    // 如果没有label属性，返回text内容
    return GetText();
}

void HTMLOptionElement::SetLabel(const std::string& label) {
    Element::SetAttribute("label", label);
}

void HTMLOptionElement::SetDefaultSelected(bool selected) {
    if (default_selected_ == selected) {
        return;
    }
    
    default_selected_ = selected;
    
    // 更新属性
    if (selected) {
        Element::SetAttribute("selected", "");
    } else {
        RemoveAttribute("selected");
    }
}

void HTMLOptionElement::SetSelected(bool selected) {
    if (selected_ == selected) {
        return;
    }
    
    selected_ = selected;
    
    // 通知父级select元素
    auto select = FindSelectElement();
    if (select) {
        // select会处理选择变化
        select->OnOptionSelectionChanged(std::static_pointer_cast<HTMLOptionElement>(shared_from_this()));
    }
}

std::string HTMLOptionElement::GetValue() const {
    // 如果有value属性，返回value属性
    std::string value = GetAttribute("value");
    if (!value.empty()) {
        return value;
    }
    // 否则返回text内容
    return GetText();
}

void HTMLOptionElement::SetValue(const std::string& value) {
    value_ = value;
    Element::SetAttribute("value", value);

    auto select = FindSelectElement();
    if (select) {
        select->OnOptionsChanged();
    }
}

std::string HTMLOptionElement::GetText() const {
    // 收集所有文本子节点的内容
    std::string text;
    auto children = GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::static_pointer_cast<Text>(child);
            text += text_node->GetData();
        }
    }
    return text;
}

void HTMLOptionElement::SetText(const std::string& text) {
    // 清除所有子节点
    while (GetFirstChild()) {
        RemoveChild(GetFirstChild());
    }
    
    // 创建新的文本节点
    auto doc = GetOwnerDocument();
    if (doc) {
        auto text_node = doc->CreateTextNode(text);
        AppendChild(text_node);
    }
}

int HTMLOptionElement::GetIndex() const {
    auto select = FindSelectElement();
    if (!select) {
        return -1;
    }
    
    auto options = select->GetOptions();
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i].get() == this) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

std::shared_ptr<HTMLFormElement> HTMLOptionElement::GetForm() const {
    auto select = FindSelectElement();
    if (select) {
        return select->GetForm();
    }
    return nullptr;
}

// ========== 重写方法 ==========

void HTMLOptionElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法
    Element::SetAttribute(name, value);

    // 处理特殊属性
    if (name == "disabled") {
        disabled_ = true;
        UpdatePseudoClasses();
    } else if (name == "selected") {
        default_selected_ = true;
        // 如果还没有用户交互，也设置selected状态
        if (!selected_) {
            SetSelected(true);
        }
    } else if (name == "value") {
        value_ = value;
        auto select = FindSelectElement();
        if (select) {
            select->OnOptionsChanged();
        }
    }
}

void HTMLOptionElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法
    Element::RemoveAttribute(name);

    // 处理特殊属性
    if (name == "disabled") {
        disabled_ = false;
        UpdatePseudoClasses();
    } else if (name == "selected") {
        default_selected_ = false;
    } else if (name == "value") {
        value_.clear();
        auto select = FindSelectElement();
        if (select) {
            select->OnOptionsChanged();
        }
    }
}

// ========== 辅助方法 ==========

void HTMLOptionElement::UpdatePseudoClasses() {
    SetPseudoClass(":disabled", disabled_);
    SetPseudoClass(":enabled", !disabled_);
}

std::shared_ptr<HTMLSelectElement> HTMLOptionElement::FindSelectElement() const {
    auto parent = GetParentNode();
    while (parent) {
        auto elem = std::dynamic_pointer_cast<Element>(parent);
        if (elem) {
            if (elem->GetTagName() == "select") {
                return std::dynamic_pointer_cast<HTMLSelectElement>(elem);
            }
            // option也可以在optgroup中
            if (elem->GetTagName() == "optgroup") {
                parent = parent->GetParentNode();
                continue;
            }
        }
        break;
    }
    return nullptr;
}

} // namespace mblink

