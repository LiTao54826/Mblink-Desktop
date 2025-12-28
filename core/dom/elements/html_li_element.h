/**
 * @file html_li_element.h
 * @brief HTML LI元素类
 * 
 * 功能：
 * - 实现<li>元素（列表项）
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief HTML LI元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/grouping-content.html#the-li-element
 * 
 * IDL定义：
 * interface HTMLLIElement : HTMLElement {
 *   attribute long value;
 * };
 * 
 * <li>元素表示列表项。
 * 默认样式：
 * - display: list-item (在本项目中使用 block)
 */
class HTMLLIElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLLIElement();

    /**
     * @brief 析构函数
     */
    ~HTMLLIElement() override = default;

    /**
     * @brief 获取列表项的值
     * @return 列表项的序号值
     */
    int GetValue() const;

    /**
     * @brief 设置列表项的值
     * @param value 列表项的序号值
     */
    void SetValue(int value);
};

} // namespace lightui

