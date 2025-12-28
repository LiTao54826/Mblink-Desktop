/**
 * @file html_heading_element.cpp
 * @brief HTML Heading元素实现
 */

#include "html_heading_element.h"
#include <algorithm>

namespace lightui {

HTMLHeadingElement::HTMLHeadingElement(int level)
    : Element("h" + std::to_string(std::clamp(level, 1, 6)))
    , level_(std::clamp(level, 1, 6)) {
    // h1-h6是块级元素，默认样式：
    // - display: block
    // - font-weight: bold
    // - font-size: 根据级别不同
    // - margin: 根据级别不同
    // 这里不设置样式，由CSS引擎处理默认样式
}

} // namespace lightui

