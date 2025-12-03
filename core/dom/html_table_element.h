/**
 * @file html_table_element.h
 * @brief HTML Table元素类
 * 
 * 功能：
 * - 实现<table>元素
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

/**
 * @brief HTML Table元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/tables.html#the-table-element
 * 
 * IDL定义：
 * interface HTMLTableElement : HTMLElement {
 *   attribute HTMLTableCaptionElement? caption;
 *   attribute HTMLTableSectionElement? tHead;
 *   attribute HTMLTableSectionElement? tFoot;
 *   readonly attribute HTMLCollection tBodies;
 *   readonly attribute HTMLCollection rows;
 * };
 */
class HTMLTableElement : public Element {
public:
    HTMLTableElement();
    ~HTMLTableElement() override = default;
};

/**
 * @brief HTML TableSection元素类（thead, tbody, tfoot）
 */
class HTMLTableSectionElement : public Element {
public:
    explicit HTMLTableSectionElement(const std::string& tag_name);
    ~HTMLTableSectionElement() override = default;
};

/**
 * @brief HTML TableRow元素类（tr）
 */
class HTMLTableRowElement : public Element {
public:
    HTMLTableRowElement();
    ~HTMLTableRowElement() override = default;

    /**
     * @brief 获取行在表格中的索引
     */
    int GetRowIndex() const;

    /**
     * @brief 获取行在所属section中的索引
     */
    int GetSectionRowIndex() const;
};

/**
 * @brief HTML TableCell元素类（td, th）
 */
class HTMLTableCellElement : public Element {
public:
    explicit HTMLTableCellElement(const std::string& tag_name);
    ~HTMLTableCellElement() override = default;

    /**
     * @brief 获取单元格的列跨度
     */
    int GetColSpan() const;

    /**
     * @brief 设置单元格的列跨度
     */
    void SetColSpan(int span);

    /**
     * @brief 获取单元格的行跨度
     */
    int GetRowSpan() const;

    /**
     * @brief 设置单元格的行跨度
     */
    void SetRowSpan(int span);

    /**
     * @brief 获取单元格在行中的索引
     */
    int GetCellIndex() const;
};

/**
 * @brief HTML TableCaption元素类（caption）
 */
class HTMLTableCaptionElement : public Element {
public:
    HTMLTableCaptionElement();
    ~HTMLTableCaptionElement() override = default;
};

/**
 * @brief HTML TableCol元素类（col, colgroup）
 */
class HTMLTableColElement : public Element {
public:
    explicit HTMLTableColElement(const std::string& tag_name);
    ~HTMLTableColElement() override = default;

    /**
     * @brief 获取列跨度
     */
    int GetSpan() const;

    /**
     * @brief 设置列跨度
     */
    void SetSpan(int span);
};

} // namespace lightui

