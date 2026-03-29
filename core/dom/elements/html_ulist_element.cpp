/**
 * @file html_ulist_element.cpp
 * @brief HTML UList元素实现
 */

#include "html_ulist_element.h"

namespace mbink {

HTMLUListElement::HTMLUListElement()
    : Element("ul") {
    // ul是块级元素，默认样式：
    // - display: block
    // - list-style-type: disc
    // - margin: 1em 0
    // - padding-left: 40px
    // 这里不设置样式，由CSS引擎处理默认样式
}

} // namespace mbink

