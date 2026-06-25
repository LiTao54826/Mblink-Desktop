/**
 * @file html_button_element.cpp
 * @brief HTML Button元素实现
 */

#include "html_button_element.h"
#include "html_form_element.h"
#include "../event.h"
#include <algorithm>

namespace mblink {

// ========== 构造函数 ==========

HTMLButtonElement::HTMLButtonElement()
    : Element("button")
    , disabled_(false)
    , type_("submit") {

    // 注意：不能在构造函数中调用UpdatePseudoClasses()，
    // 因为它需要shared_from_this()，而此时对象还没有被shared_ptr管理
    // 伪类状态将在第一次SetDisabled()调用时更新
}

// ========== IDL属性实现 ==========

void HTMLButtonElement::SetDisabled(bool disabled) {
    if (disabled_ == disabled) {
        return;  // 无变化，直接返回
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
    
    // TODO: 如果禁用，移除焦点
    // if (disabled && IsFocused()) {
    //     Blur();
    // }
}

std::shared_ptr<HTMLFormElement> HTMLButtonElement::GetForm() const {
    return FindForm();
}

void HTMLButtonElement::SetName(const std::string& name) {
    if (name_ == name) {
        return;
    }
    
    name_ = name;
    Element::SetAttribute("name", name);
}

void HTMLButtonElement::SetType(const std::string& type) {
    // 验证type值，只接受 submit, reset, button
    std::string lower_type = type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
    
    if (lower_type != "submit" && lower_type != "reset" && lower_type != "button") {
        // 无效值，使用默认值 submit
        lower_type = "submit";
    }
    
    if (type_ == lower_type) {
        return;
    }
    
    type_ = lower_type;
    Element::SetAttribute("type", type_);
}

void HTMLButtonElement::SetValue(const std::string& value) {
    if (value_ == value) {
        return;
    }
    
    value_ = value;
    Element::SetAttribute("value", value);
}

// ========== 重写方法 ==========

void HTMLButtonElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法
    Element::SetAttribute(name, value);

    // 处理特殊属性
    if (name == "disabled") {
        // disabled是布尔属性，只要存在就是true
        disabled_ = true;
        UpdatePseudoClasses();
    } else if (name == "type") {
        SetType(value);
    } else if (name == "name") {
        name_ = value;
    } else if (name == "value") {
        value_ = value;
    }
}

void HTMLButtonElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法
    Element::RemoveAttribute(name);

    // 处理特殊属性
    if (name == "disabled") {
        disabled_ = false;
        UpdatePseudoClasses();
    } else if (name == "type") {
        type_ = "submit";  // 恢复默认值
    } else if (name == "name") {
        name_.clear();
    } else if (name == "value") {
        value_.clear();
    }
}

void HTMLButtonElement::HandleClick() {
    // 如果禁用，不处理点击
    if (disabled_) {
        return;
    }
    
    // 根据type执行不同操作
    if (type_ == "submit") {
        // 提交表单
        auto form = FindForm();
        if (form) {
            // TODO: 实现表单提交
            // form->Submit();
        }
    } else if (type_ == "reset") {
        // 重置表单
        auto form = FindForm();
        if (form) {
            // TODO: 实现表单重置
            // form->Reset();
        }
    }
    // type == "button" 不执行默认操作
    
    // 触发click事件
    // TODO: 调用基类方法或触发事件
    // Element::HandleClick();
}

// ========== 私有辅助方法 ==========

void HTMLButtonElement::UpdatePseudoClasses() {
    SetPseudoClass(":disabled", disabled_);
    SetPseudoClass(":enabled", !disabled_);
}

std::shared_ptr<HTMLFormElement> HTMLButtonElement::FindForm() const {
    // 向上查找最近的form元素
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

} // namespace mblink

