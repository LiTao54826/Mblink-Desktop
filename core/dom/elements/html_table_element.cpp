/**
 * @file html_table_element.cpp
 * @brief HTML Table元素实现
 */

#include "html_table_element.h"

namespace mbink {

// 辅助函数：获取父元素
static std::shared_ptr<Element> GetParentElementOf(const Element* elem) {
    auto parent_node = elem->GetParentNode();
    if (!parent_node) return nullptr;
    return std::dynamic_pointer_cast<Element>(parent_node);
}

// ========== HTMLTableElement ==========

HTMLTableElement::HTMLTableElement()
    : Element("table") {
}

// ========== HTMLTableSectionElement ==========

HTMLTableSectionElement::HTMLTableSectionElement(const std::string& tag_name)
    : Element(tag_name) {
}

// ========== HTMLTableRowElement ==========

HTMLTableRowElement::HTMLTableRowElement()
    : Element("tr") {
}

int HTMLTableRowElement::GetRowIndex() const {
    // 获取行在整个表格中的索引
    auto parent = GetParentElementOf(this);
    if (!parent) return -1;

    // 如果父元素是 section (thead/tbody/tfoot)，需要查找 table
    std::shared_ptr<Element> table_or_section = parent;
    if (parent->GetTagName() == "thead" ||
        parent->GetTagName() == "tbody" ||
        parent->GetTagName() == "tfoot") {
        table_or_section = GetParentElementOf(parent.get());
    }

    if (!table_or_section || table_or_section->GetTagName() != "table") {
        return -1;
    }

    // 遍历所有行计算索引
    int index = 0;
    const auto& children = table_or_section->GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        auto element = std::dynamic_pointer_cast<Element>(children[i]);
        if (!element) continue;

        if (element->GetTagName() == "tr") {
            if (element.get() == this) return index;
            index++;
        } else if (element->GetTagName() == "thead" ||
                   element->GetTagName() == "tbody" ||
                   element->GetTagName() == "tfoot") {
            const auto& section_children = element->GetChildNodes();
            for (size_t j = 0; j < section_children.size(); ++j) {
                auto row = std::dynamic_pointer_cast<Element>(section_children[j]);
                if (row && row->GetTagName() == "tr") {
                    if (row.get() == this) return index;
                    index++;
                }
            }
        }
    }
    return -1;
}

int HTMLTableRowElement::GetSectionRowIndex() const {
    auto parent = GetParentElementOf(this);
    if (!parent) return -1;

    int index = 0;
    const auto& children = parent->GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        auto element = std::dynamic_pointer_cast<Element>(children[i]);
        if (element && element->GetTagName() == "tr") {
            if (element.get() == this) return index;
            index++;
        }
    }
    return -1;
}

// ========== HTMLTableCellElement ==========

HTMLTableCellElement::HTMLTableCellElement(const std::string& tag_name)
    : Element(tag_name) {
}

int HTMLTableCellElement::GetColSpan() const {
    std::string span_str = GetAttribute("colspan");
    if (span_str.empty()) return 1;
    try {
        int span = std::stoi(span_str);
        return (span > 0) ? span : 1;
    } catch (...) {
        return 1;
    }
}

void HTMLTableCellElement::SetColSpan(int span) {
    if (span < 1) span = 1;
    SetAttribute("colspan", std::to_string(span));
}

int HTMLTableCellElement::GetRowSpan() const {
    std::string span_str = GetAttribute("rowspan");
    if (span_str.empty()) return 1;
    try {
        int span = std::stoi(span_str);
        return (span >= 0) ? span : 1;  // 0 means span all rows
    } catch (...) {
        return 1;
    }
}

void HTMLTableCellElement::SetRowSpan(int span) {
    if (span < 0) span = 1;
    SetAttribute("rowspan", std::to_string(span));
}

int HTMLTableCellElement::GetCellIndex() const {
    auto parent = GetParentElementOf(this);
    if (!parent || parent->GetTagName() != "tr") return -1;

    int index = 0;
    const auto& children = parent->GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        auto element = std::dynamic_pointer_cast<Element>(children[i]);
        if (element && (element->GetTagName() == "td" || element->GetTagName() == "th")) {
            if (element.get() == this) return index;
            index++;
        }
    }
    return -1;
}

// ========== HTMLTableCaptionElement ==========

HTMLTableCaptionElement::HTMLTableCaptionElement()
    : Element("caption") {
}

// ========== HTMLTableColElement ==========

HTMLTableColElement::HTMLTableColElement(const std::string& tag_name)
    : Element(tag_name) {
}

int HTMLTableColElement::GetSpan() const {
    std::string span_str = GetAttribute("span");
    if (span_str.empty()) return 1;
    try {
        int span = std::stoi(span_str);
        return (span > 0) ? span : 1;
    } catch (...) {
        return 1;
    }
}

void HTMLTableColElement::SetSpan(int span) {
    if (span < 1) span = 1;
    SetAttribute("span", std::to_string(span));
}

} // namespace mbink

