/**
 * @file html_olist_element.cpp
 * @brief HTML OList元素实现
 */

#include "html_olist_element.h"

namespace mblink {

HTMLOListElement::HTMLOListElement()
    : Element("ol") {
    // ol是块级元素，默认样式：
    // - display: block
    // - list-style-type: decimal
    // - margin: 1em 0
    // - padding-left: 40px
    // 这里不设置样式，由CSS引擎处理默认样式
}

bool HTMLOListElement::GetReversed() const {
    return HasAttribute("reversed");
}

void HTMLOListElement::SetReversed(bool reversed) {
    if (reversed) {
        SetAttribute("reversed", "");
    } else {
        RemoveAttribute("reversed");
    }
}

int HTMLOListElement::GetStart() const {
    std::string start_str = GetAttribute("start");
    if (start_str.empty()) {
        return 1;  // 默认从1开始
    }
    try {
        return std::stoi(start_str);
    } catch (...) {
        return 1;
    }
}

void HTMLOListElement::SetStart(int start) {
    SetAttribute("start", std::to_string(start));
}

std::string HTMLOListElement::GetType() const {
    std::string type = GetAttribute("type");
    if (type.empty()) {
        return "1";  // 默认为数字
    }
    return type;
}

void HTMLOListElement::SetType(const std::string& type) {
    SetAttribute("type", type);
}

} // namespace mblink

