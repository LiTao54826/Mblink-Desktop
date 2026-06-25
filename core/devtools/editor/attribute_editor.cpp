/**
 * @file attribute_editor.cpp
 * @brief 属性编辑器实现
 */

#include "attribute_editor.h"
#include <algorithm>
#include <cctype>

namespace mblink {

AttributeEditor::AttributeEditor() = default;

AttributeEditor::~AttributeEditor() = default;

void AttributeEditor::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
}

bool AttributeEditor::SetAttribute(const std::string& name, const std::string& value) {
    auto element = element_.lock();
    if (!element) return false;

    if (!ValidateAttributeName(name)) {
        return false;
    }

    element->SetAttribute(name, value);

    if (on_attribute_changed_) {
        on_attribute_changed_(name, value);
    }

    return true;
}

bool AttributeEditor::AddAttribute(const std::string& name, const std::string& value) {
    return SetAttribute(name, value);
}

bool AttributeEditor::RemoveAttribute(const std::string& name) {
    auto element = element_.lock();
    if (!element) return false;

    element->RemoveAttribute(name);

    if (on_attribute_changed_) {
        on_attribute_changed_(name, "");
    }

    return true;
}

std::string AttributeEditor::GetAttribute(const std::string& name) const {
    auto element = element_.lock();
    if (!element) return "";

    return element->GetAttribute(name);
}

bool AttributeEditor::ValidateAttributeName(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    // 属性名必须以字母或下划线开头
    char first = name[0];
    if (!std::isalpha(first) && first != '_' && first != ':') {
        return false;
    }

    // 属性名只能包含字母、数字、连字符、下划线、冒号和点
    for (char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_' && c != ':' && c != '.') {
            return false;
        }
    }

    return true;
}

} // namespace mblink
