/**
 * @file html_form_controls.cpp
 * @brief HTML 表单增强控件元素实现
 */

#include "html_form_controls.h"

namespace mblink {

// ========== HTMLFieldSetElement ==========

HTMLFieldSetElement::HTMLFieldSetElement()
    : Element("fieldset") {
}

bool HTMLFieldSetElement::GetDisabled() const {
    return HasAttribute("disabled");
}

void HTMLFieldSetElement::SetDisabled(bool disabled) {
    if (disabled) {
        SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
}

std::shared_ptr<Element> HTMLFieldSetElement::GetForm() const {
    // 向上查找 form 元素
    auto parent = GetParentNode();
    while (parent) {
        auto element = std::dynamic_pointer_cast<Element>(parent);
        if (element && element->GetTagName() == "form") {
            return element;
        }
        parent = parent->GetParentNode();
    }
    return nullptr;
}

std::string HTMLFieldSetElement::GetName() const {
    return GetAttribute("name");
}

void HTMLFieldSetElement::SetName(const std::string& name) {
    SetAttribute("name", name);
}

// ========== HTMLLegendElement ==========

HTMLLegendElement::HTMLLegendElement()
    : Element("legend") {
}

std::shared_ptr<HTMLFieldSetElement> HTMLLegendElement::GetForm() const {
    auto parent = GetParentNode();
    if (parent) {
        return std::dynamic_pointer_cast<HTMLFieldSetElement>(parent);
    }
    return nullptr;
}

// ========== HTMLOptGroupElement ==========

HTMLOptGroupElement::HTMLOptGroupElement()
    : Element("optgroup") {
}

bool HTMLOptGroupElement::GetDisabled() const {
    return HasAttribute("disabled");
}

void HTMLOptGroupElement::SetDisabled(bool disabled) {
    if (disabled) {
        SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
}

std::string HTMLOptGroupElement::GetLabel() const {
    return GetAttribute("label");
}

void HTMLOptGroupElement::SetLabel(const std::string& label) {
    SetAttribute("label", label);
}

// ========== HTMLDataListElement ==========

HTMLDataListElement::HTMLDataListElement()
    : Element("datalist") {
}

std::vector<std::shared_ptr<Element>> HTMLDataListElement::GetOptions() const {
    std::vector<std::shared_ptr<Element>> options;
    const auto& children = GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        auto element = std::dynamic_pointer_cast<Element>(children[i]);
        if (element && element->GetTagName() == "option") {
            options.push_back(element);
        }
    }
    return options;
}

// ========== HTMLOutputElement ==========

HTMLOutputElement::HTMLOutputElement()
    : Element("output") {
}

std::string HTMLOutputElement::GetDefaultValue() const {
    return GetAttribute("defaultvalue");
}

void HTMLOutputElement::SetDefaultValue(const std::string& value) {
    SetAttribute("defaultvalue", value);
}

std::string HTMLOutputElement::GetValue() const {
    // 返回 textContent
    return GetTextContent();
}

void HTMLOutputElement::SetValue(const std::string& value) {
    SetTextContent(value);
}

std::string HTMLOutputElement::GetName() const {
    return GetAttribute("name");
}

void HTMLOutputElement::SetName(const std::string& name) {
    SetAttribute("name", name);
}

std::string HTMLOutputElement::GetHtmlFor() const {
    return GetAttribute("for");
}

void HTMLOutputElement::SetHtmlFor(const std::string& value) {
    SetAttribute("for", value);
}

// ========== HTMLProgressElement ==========

HTMLProgressElement::HTMLProgressElement()
    : Element("progress") {
}

double HTMLProgressElement::GetValue() const {
    std::string value_str = GetAttribute("value");
    if (value_str.empty()) return -1.0; // 不确定状态
    try {
        return std::stod(value_str);
    } catch (...) {
        return -1.0;
    }
}

void HTMLProgressElement::SetValue(double value) {
    SetAttribute("value", std::to_string(value));
}

double HTMLProgressElement::GetMax() const {
    std::string max_str = GetAttribute("max");
    if (max_str.empty()) return 1.0; // 默认最大值
    try {
        double max = std::stod(max_str);
        return (max > 0) ? max : 1.0;
    } catch (...) {
        return 1.0;
    }
}

void HTMLProgressElement::SetMax(double max) {
    if (max > 0) {
        SetAttribute("max", std::to_string(max));
    }
}

double HTMLProgressElement::GetPosition() const {
    double value = GetValue();
    if (value < 0) return -1.0; // 不确定状态
    double max = GetMax();
    return value / max;
}

// ========== HTMLMeterElement ==========

HTMLMeterElement::HTMLMeterElement()
    : Element("meter") {
}

double HTMLMeterElement::GetValue() const {
    std::string value_str = GetAttribute("value");
    if (value_str.empty()) return 0.0;
    try {
        return std::stod(value_str);
    } catch (...) {
        return 0.0;
    }
}

void HTMLMeterElement::SetValue(double value) {
    SetAttribute("value", std::to_string(value));
}

double HTMLMeterElement::GetMin() const {
    std::string min_str = GetAttribute("min");
    if (min_str.empty()) return 0.0;
    try {
        return std::stod(min_str);
    } catch (...) {
        return 0.0;
    }
}

void HTMLMeterElement::SetMin(double min) {
    SetAttribute("min", std::to_string(min));
}

double HTMLMeterElement::GetMax() const {
    std::string max_str = GetAttribute("max");
    if (max_str.empty()) return 1.0;
    try {
        return std::stod(max_str);
    } catch (...) {
        return 1.0;
    }
}

void HTMLMeterElement::SetMax(double max) {
    SetAttribute("max", std::to_string(max));
}

double HTMLMeterElement::GetLow() const {
    std::string low_str = GetAttribute("low");
    if (low_str.empty()) return GetMin();
    try {
        return std::stod(low_str);
    } catch (...) {
        return GetMin();
    }
}

void HTMLMeterElement::SetLow(double low) {
    SetAttribute("low", std::to_string(low));
}

double HTMLMeterElement::GetHigh() const {
    std::string high_str = GetAttribute("high");
    if (high_str.empty()) return GetMax();
    try {
        return std::stod(high_str);
    } catch (...) {
        return GetMax();
    }
}

void HTMLMeterElement::SetHigh(double high) {
    SetAttribute("high", std::to_string(high));
}

double HTMLMeterElement::GetOptimum() const {
    std::string optimum_str = GetAttribute("optimum");
    if (optimum_str.empty()) {
        // 默认为 low 和 high 的中点
        return (GetLow() + GetHigh()) / 2.0;
    }
    try {
        return std::stod(optimum_str);
    } catch (...) {
        return (GetLow() + GetHigh()) / 2.0;
    }
}

void HTMLMeterElement::SetOptimum(double optimum) {
    SetAttribute("optimum", std::to_string(optimum));
}

// ========== HTMLDialogElement ==========

HTMLDialogElement::HTMLDialogElement()
    : Element("dialog") {
}

bool HTMLDialogElement::GetOpen() const {
    return HasAttribute("open");
}

void HTMLDialogElement::SetOpen(bool open) {
    if (open) {
        SetAttribute("open", "");
    } else {
        RemoveAttribute("open");
    }
}

std::string HTMLDialogElement::GetReturnValue() const {
    return return_value_;
}

void HTMLDialogElement::SetReturnValue(const std::string& value) {
    return_value_ = value;
}

void HTMLDialogElement::Show() {
    SetOpen(true);
}

void HTMLDialogElement::ShowModal() {
    // TODO: 实现模态对话框逻辑（需要设置 backdrop 等）
    SetOpen(true);
}

void HTMLDialogElement::Close(const std::string& returnValue) {
    if (!returnValue.empty()) {
        return_value_ = returnValue;
    }
    SetOpen(false);
    // TODO: 触发 close 事件
}

} // namespace mblink

