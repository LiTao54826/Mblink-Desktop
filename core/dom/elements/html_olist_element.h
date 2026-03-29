/**
 * @file html_olist_element.h
 * @brief HTML OList元素类
 * 
 * 功能：
 * - 实现<ol>元素（有序列表）
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace mbink {

/**
 * @brief HTML OList元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/grouping-content.html#the-ol-element
 * 
 * IDL定义：
 * interface HTMLOListElement : HTMLElement {
 *   attribute boolean reversed;
 *   attribute long start;
 *   attribute DOMString type;
 * };
 * 
 * <ol>元素表示有序列表。
 * 默认样式：
 * - display: block
 * - list-style-type: decimal
 * - margin: 1em 0
 * - padding-left: 40px
 */
class HTMLOListElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLOListElement();

    /**
     * @brief 析构函数
     */
    ~HTMLOListElement() override = default;

    /**
     * @brief 获取是否反向排序
     * @return 是否反向排序
     */
    bool GetReversed() const;

    /**
     * @brief 设置是否反向排序
     * @param reversed 是否反向排序
     */
    void SetReversed(bool reversed);

    /**
     * @brief 获取起始序号
     * @return 起始序号
     */
    int GetStart() const;

    /**
     * @brief 设置起始序号
     * @param start 起始序号
     */
    void SetStart(int start);

    /**
     * @brief 获取列表类型
     * @return 列表类型 ("1", "a", "A", "i", "I")
     */
    std::string GetType() const;

    /**
     * @brief 设置列表类型
     * @param type 列表类型 ("1", "a", "A", "i", "I")
     */
    void SetType(const std::string& type);
};

} // namespace mbink

