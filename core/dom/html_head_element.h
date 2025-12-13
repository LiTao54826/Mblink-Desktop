/**
 * @file html_head_element.h
 * @brief HTMLHeadElement 类 - <head> 元素
 */

#pragma once

#include "element.h"

namespace lightui {

/**
 * @brief HTMLHeadElement 类
 * 
 * 表示 HTML <head> 元素，包含文档的元数据
 */
class HTMLHeadElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLHeadElement() : Element("head") {}

    /**
     * @brief 析构函数
     */
    ~HTMLHeadElement() override = default;
};

} // namespace lightui
