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
    auto parent = GetParentElementOf(this);
    if (!parent) return -1;

    std::shared_ptr<Element> table = parent;
    if (parent->GetTagName() == "thead" ||
        parent->GetTagName() == "tbody" ||
        parent->GetTagName() == "tfoot") {
        table = GetParentElementOf(parent.get());
    }

    if (!table || table->GetTagName() != "table") {
        return -1;
    }

    int index = 0;
    const auto& children = table->GetChildNodes();
    auto count_rows_in_section = [&](const std::shared_ptr<Element>& section) -> bool {
        const auto& section_children = section->GetChildNodes();
        for (const auto& section_child : section_children) {
            auto row = std::dynamic_pointer_cast<Element>(section_child);
            if (row && row->GetTagName() == "tr") {
                if (row.get() == this) return true;
                index++;
            }
        }
        return false;
    };

    for (const auto& child : children) {
        auto element = std::dynamic_pointer_cast<Element>(child);
        if (element && element->GetTagName() == "thead" && count_rows_in_section(element)) {
            return index;
        }
    }

    for (const auto& child : children) {
        auto element = std::dynamic_pointer_cast<Element>(child);
        if (!element) continue;

        const std::string tag_name = element->GetTagName();
        if (tag_name == "tr") {
            if (element.get() == this) return index;
            index++;
        } else if (tag_name == "tbody" && count_rows_in_section(element)) {
            return index;
        }
    }

    for (const auto& child : children) {
        auto element = std::dynamic_pointer_cast<Element>(child);
        if (element && element->GetTagName() == "tfoot" && count_rows_in_section(element)) {
            return index;
        }
    }

    return -1;
}

int HTMLTableRowElement::GetSectionRowIndex() const {
    auto parent = GetParentElementOf(this);
    if (!parent) return -1;

    const std::string parent_tag_name = parent->GetTagName();
    if (parent_tag_name == "table") {
        return GetRowIndex();
    }

    if (parent_tag_name != "thead" &&
        parent_tag_name != "tbody" &&
        parent_tag_name != "tfoot") {
        return -1;
    }

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

