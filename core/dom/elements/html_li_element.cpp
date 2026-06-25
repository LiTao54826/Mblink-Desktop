/**
 * @file html_li_element.cpp
 * @brief HTML LI元素实现
 */

#include "html_li_element.h"

namespace mblink {

HTMLLIElement::HTMLLIElement()
    : Element("li") {
    // li是列表项元素，默认样式：
    // - display: list-item (在本项目中使用 block)
    // 这里不设置样式，由CSS引擎处理默认样式
}

int HTMLLIElement::GetValue() const {
    std::string value_str = GetAttribute("value");
    if (value_str.empty()) {
        return 0;  // 默认值
    }
    try {
        return std::stoi(value_str);
    } catch (...) {
        return 0;
    }
}

void HTMLLIElement::SetValue(int value) {
    SetAttribute("value", std::to_string(value));
}

} // namespace mblink

