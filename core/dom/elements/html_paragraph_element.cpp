/**
 * @file html_paragraph_element.cpp
 * @brief HTML Paragraph元素实现
 */

#include "html_paragraph_element.h"

namespace lightui {

HTMLParagraphElement::HTMLParagraphElement()
    : Element("p") {
    // p是块级元素，默认display: block, margin-top: 1em, margin-bottom: 1em
    // 这里不设置样式，由CSS引擎处理默认样式
}

} // namespace lightui

