/**
 * @file html_div_element.cpp
 * @brief HTML Div元素实现
 */

#include "html_div_element.h"

namespace mbink {

HTMLDivElement::HTMLDivElement()
    : Element("div") {
    // div是块级元素，默认display: block
    // 这里不设置样式，由CSS引擎处理默认样式
}

} // namespace mbink

