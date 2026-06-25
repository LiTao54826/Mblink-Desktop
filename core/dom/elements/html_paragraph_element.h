/**
 * @file html_paragraph_element.h
 * @brief HTML Paragraph元素类
 * 
 * 功能：
 * - 实现<p>元素
 * - 段落元素
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mblink {

/**
 * @brief HTML Paragraph元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/grouping-content.html#the-p-element
 * 
 * IDL定义：
 * interface HTMLParagraphElement : HTMLElement {
 *   // 无特殊属性
 * };
 * 
 * <p>元素表示一个段落。
 * 默认样式：display: block, margin-top: 1em, margin-bottom: 1em
 */
class HTMLParagraphElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLParagraphElement();

    /**
     * @brief 析构函数
     */
    ~HTMLParagraphElement() override = default;
};

} // namespace mblink

