/**
 * @file html_heading_element.h
 * @brief HTML Heading元素类
 * 
 * 功能：
 * - 实现<h1>-<h6>元素
 * - 标题元素
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "../element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief HTML Heading元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/sections.html#the-h1,-h2,-h3,-h4,-h5,-and-h6-elements
 * 
 * IDL定义：
 * interface HTMLHeadingElement : HTMLElement {
 *   // 无特殊属性
 * };
 * 
 * <h1>-<h6>元素表示不同级别的标题。
 * 默认样式：
 * - display: block
 * - font-weight: bold
 * - font-size: 根据级别不同（h1最大，h6最小）
 * - margin-top, margin-bottom: 根据级别不同
 */
class HTMLHeadingElement : public Element {
public:
    /**
     * @brief 构造函数
     * @param level 标题级别（1-6）
     */
    explicit HTMLHeadingElement(int level);

    /**
     * @brief 析构函数
     */
    ~HTMLHeadingElement() override = default;

    /**
     * @brief 获取标题级别
     * @return 标题级别（1-6）
     */
    int GetLevel() const { return level_; }

private:
    int level_;  ///< 标题级别（1-6）
};

} // namespace lightui

