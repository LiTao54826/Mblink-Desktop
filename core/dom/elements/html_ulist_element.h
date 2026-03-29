/**
 * @file html_ulist_element.h
 * @brief HTML UList元素类
 * 
 * 功能：
 * - 实现<ul>元素（无序列表）
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mbink {

/**
 * @brief HTML UList元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/grouping-content.html#the-ul-element
 * 
 * IDL定义：
 * interface HTMLUListElement : HTMLElement {
 *   // 无特殊属性（type属性已废弃）
 * };
 * 
 * <ul>元素表示无序列表。
 * 默认样式：
 * - display: block
 * - list-style-type: disc
 * - margin: 1em 0
 * - padding-left: 40px
 */
class HTMLUListElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLUListElement();

    /**
     * @brief 析构函数
     */
    ~HTMLUListElement() override = default;
};

} // namespace mbink

