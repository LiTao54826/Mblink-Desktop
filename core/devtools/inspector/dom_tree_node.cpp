/**
 * @file dom_tree_node.cpp
 * @brief DOM 树节点渲染辅助类实现
 */

#include "dom_tree_node.h"
#include "core/dom/text.h"
#include <algorithm>

namespace lightui {

std::string DOMTreeNode::FormatNode(std::shared_ptr<Node> node) {
    if (!node) return "";

    switch (node->GetNodeType()) {
        case NodeType::ELEMENT_NODE: {
            auto element = std::dynamic_pointer_cast<Element>(node);
            return FormatOpenTag(element);
        }
        case NodeType::TEXT_NODE: {
            auto text = std::dynamic_pointer_cast<Text>(node);
            if (text) {
                return "\"" + TruncateText(text->GetData()) + "\"";
            }
            break;
        }
        default:
            break;
    }

    // 返回节点类型名称
    switch (node->GetNodeType()) {
        case NodeType::ELEMENT_NODE: return "#element";
        case NodeType::TEXT_NODE: return "#text";
        case NodeType::DOCUMENT_NODE: return "#document";
        default: return "#node";
    }
}

std::string DOMTreeNode::FormatOpenTag(std::shared_ptr<Element> element) {
    if (!element) return "";

    std::string result = "<" + element->GetTagName();
    result += FormatAttributes(element);

    if (IsSelfClosingTag(element->GetTagName())) {
        result += " />";
    } else {
        result += ">";
    }

    return result;
}

std::string DOMTreeNode::FormatCloseTag(std::shared_ptr<Element> element) {
    if (!element || IsSelfClosingTag(element->GetTagName())) {
        return "";
    }
    return "</" + element->GetTagName() + ">";
}

std::string DOMTreeNode::FormatAttributes(std::shared_ptr<Element> element) {
    if (!element) return "";

    std::string result;

    // 优先显示 id
    std::string id = element->GetAttribute("id");
    if (!id.empty()) {
        result += " id=\"" + id + "\"";
    }

    // 然后显示 class
    std::string cls = element->GetAttribute("class");
    if (!cls.empty()) {
        result += " class=\"" + cls + "\"";
    }

    // 其他属性（限制数量）
    int attr_count = 0;
    const int max_attrs = 3;

    // TODO: 遍历其他属性
    // 这里需要 Element 提供获取所有属性的接口

    return result;
}

std::string DOMTreeNode::TruncateText(const std::string& text, size_t max_length) {
    std::string result = text;

    // 替换换行符和多余空白
    for (char& c : result) {
        if (c == '\n' || c == '\r' || c == '\t') {
            c = ' ';
        }
    }

    // 压缩连续空格
    auto new_end = std::unique(result.begin(), result.end(),
        [](char a, char b) { return a == ' ' && b == ' '; });
    result.erase(new_end, result.end());

    // 去除首尾空格
    size_t start = result.find_first_not_of(' ');
    size_t end = result.find_last_not_of(' ');
    if (start != std::string::npos && end != std::string::npos) {
        result = result.substr(start, end - start + 1);
    }

    // 截断
    if (result.length() > max_length) {
        result = result.substr(0, max_length - 3) + "...";
    }

    return result;
}

bool DOMTreeNode::IsSelfClosingTag(const std::string& tag_name) {
    static const std::vector<std::string> self_closing = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };

    std::string lower_tag = tag_name;
    std::transform(lower_tag.begin(), lower_tag.end(), lower_tag.begin(), ::tolower);

    return std::find(self_closing.begin(), self_closing.end(), lower_tag) != self_closing.end();
}

} // namespace lightui
