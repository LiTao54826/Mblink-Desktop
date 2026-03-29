/**
 * @file html_span_element.cpp
 * @brief HTML Span元素实现
 */

#include "html_span_element.h"

namespace mbink {

HTMLSpanElement::HTMLSpanElement()
    : Element("span") {
    // span是内联元素，默认display: inline
    // 这里不设置样式，由CSS引擎处理默认样式
}

} // namespace mbink

