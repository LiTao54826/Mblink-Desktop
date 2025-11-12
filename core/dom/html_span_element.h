/**
 * @file html_span_element.h
 * @brief HTML Span元素类
 * 
 * 功能：
 * - 实现<span>元素
 * - 通用内联容器元素
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief HTML Span元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-span-element
 * 
 * IDL定义：
 * interface HTMLSpanElement : HTMLElement {
 *   // 无特殊属性
 * };
 * 
 * <span>是通用的内联容器元素，用于组织和样式化内联内容。
 * 默认样式：display: inline
 */
class HTMLSpanElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLSpanElement();

    /**
     * @brief 析构函数
     */
    ~HTMLSpanElement() override = default;
};

} // namespace lightui

