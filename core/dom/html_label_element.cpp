/**
 * @file html_label_element.cpp
 * @brief HTML Label元素实现
 */

#include "html_label_element.h"
#include "html_form_element.h"
#include "html_input_element.h"
#include "html_button_element.h"
#include "html_select_element.h"
#include "html_textarea_element.h"
#include "document.h"
#include "event.h"

namespace lightui {

HTMLLabelElement::HTMLLabelElement()
    : Element("label")
    , html_for_("") {
}

// ========== IDL属性 ==========

std::shared_ptr<HTMLFormElement> HTMLLabelElement::GetForm() const {
    return FindForm();
}

void HTMLLabelElement::SetHtmlFor(const std::string& html_for) {
    html_for_ = html_for;
    SetAttribute("for", html_for);
}

std::shared_ptr<Element> HTMLLabelElement::GetControl() const {
    // 如果有for属性，通过ID查找
    if (!html_for_.empty()) {
        return FindControlById();
    }
    
    // 否则查找第一个可标签化的后代元素
    return FindLabelableDescendant();
}

// ========== 重写方法 ==========

void HTMLLabelElement::SetAttribute(const std::string& name, const std::string& value) {
    Element::SetAttribute(name, value);
    
    if (name == "for") {
        html_for_ = value;
    }
}

void HTMLLabelElement::RemoveAttribute(const std::string& name) {
    Element::RemoveAttribute(name);
    
    if (name == "for") {
        html_for_ = "";
    }
}

void HTMLLabelElement::HandleClick() {
    // 触发click事件
    auto event = std::make_shared<Event>("click");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());
    DispatchEvent(event);
    
    // 聚焦并激活关联的控件
    FocusControl();
    ActivateControl();
}

// ========== 私有方法 ==========

std::shared_ptr<HTMLFormElement> HTMLLabelElement::FindForm() const {
    // 查找关联的控件
    auto control = GetControl();
    if (!control) {
        return nullptr;
    }

    // 如果控件是表单控件，获取其关联的表单
    auto tag = control->GetTagName();
    if (tag == "button") {
        auto button = std::dynamic_pointer_cast<HTMLButtonElement>(control);
        return button ? button->GetForm() : nullptr;
    } else if (tag == "select") {
        auto select = std::dynamic_pointer_cast<HTMLSelectElement>(control);
        return select ? select->GetForm() : nullptr;
    }

    // HTMLInputElement和HTMLTextAreaElement目前没有GetForm()方法
    // 需要通过查找父级form元素来获取
    auto current = control->GetParentNode();
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(current);
            if (elem && elem->GetTagName() == "form") {
                return std::dynamic_pointer_cast<HTMLFormElement>(elem);
            }
        }
        current = current->GetParentNode();
    }

    return nullptr;
}

std::shared_ptr<Element> HTMLLabelElement::FindControlById() const {
    if (html_for_.empty()) {
        return nullptr;
    }
    
    // 通过document查找ID
    auto doc = GetOwnerDocument();
    if (!doc) {
        return nullptr;
    }
    
    return doc->GetElementById(html_for_);
}

std::shared_ptr<Element> HTMLLabelElement::FindLabelableDescendant() const {
    // 递归查找第一个可标签化的后代元素
    std::function<std::shared_ptr<Element>(std::shared_ptr<Node>)> find_labelable;
    find_labelable = [&](std::shared_ptr<Node> node) -> std::shared_ptr<Element> {
        if (!node) {
            return nullptr;
        }
        
        // 如果是元素节点，检查是否可标签化
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(node);
            if (elem && IsLabelable(elem)) {
                return elem;
            }
            
            // 递归查找子节点
            for (const auto& child : node->GetChildNodes()) {
                auto result = find_labelable(child);
                if (result) {
                    return result;
                }
            }
        }
        
        return nullptr;
    };
    
    // 从当前label的子节点开始查找
    for (const auto& child : GetChildNodes()) {
        auto result = find_labelable(child);
        if (result) {
            return result;
        }
    }
    
    return nullptr;
}

bool HTMLLabelElement::IsLabelable(std::shared_ptr<Element> element) const {
    if (!element) {
        return false;
    }

    auto tag = element->GetTagName();

    // 可标签化的元素
    if (tag == "button" || tag == "select" || tag == "textarea") {
        return true;
    }

    // input元素（除了type="hidden"）
    if (tag == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (input) {
            return input->GetInputType() != InputType::Hidden;
        }
    }

    // 其他可标签化的元素（meter, output, progress）
    // 目前未实现，暂时返回false
    if (tag == "meter" || tag == "output" || tag == "progress") {
        return true;
    }

    return false;
}

void HTMLLabelElement::FocusControl() {
    auto control = GetControl();
    if (!control) {
        return;
    }
    
    // 触发focus事件
    auto event = std::make_shared<Event>("focus");
    event->SetTarget(control);
    event->SetCurrentTarget(control);
    control->DispatchEvent(event);
}

void HTMLLabelElement::ActivateControl() {
    auto control = GetControl();
    if (!control) {
        return;
    }

    auto tag = control->GetTagName();

    // 如果是checkbox或radio，切换选中状态
    if (tag == "input") {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(control);
        if (input) {
            auto type = input->GetInputType();
            if (type == InputType::Checkbox) {
                // 切换checkbox选中状态
                input->SetChecked(!input->GetChecked(), true);
            } else if (type == InputType::Radio) {
                // radio只能选中，不能取消选中
                if (!input->GetChecked()) {
                    input->SetChecked(true, true);
                }
            }
        }
    }
}

} // namespace lightui

