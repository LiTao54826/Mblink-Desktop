/**
 * @file dom_serializer.cpp
 * @brief DOM 序列化器实现
 */

#include "dom_serializer.h"
#include "core/dom/text.h"
#include "core/lexbor/lexbor_document.h"
#include <algorithm>
#include <sstream>

namespace lightui {

std::string DOMSerializer::Serialize(std::shared_ptr<Node> node,
                                      const SerializeOptions& options) {
    if (!node) return "";

    std::string output;
    SerializeNode(node, output, 0, options);
    return output;
}

std::shared_ptr<Document> DOMSerializer::Deserialize(const std::string& html) {
    if (html.empty()) return nullptr;

    // 使用 Document 的 LoadHTML 方法解析
    auto doc = std::make_shared<Document>();
    if (!doc->LoadHTML(html)) {
        return nullptr;
    }

    return doc;
}

bool DOMSerializer::IsEquivalent(std::shared_ptr<Node> a, std::shared_ptr<Node> b) {
    if (!a && !b) return true;
    if (!a || !b) return false;

    // 检查节点类型
    if (a->GetNodeType() != b->GetNodeType()) {
        return false;
    }

    // 对于元素节点，检查属性
    if (a->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem_a = std::dynamic_pointer_cast<Element>(a);
        auto elem_b = std::dynamic_pointer_cast<Element>(b);

        if (!elem_a || !elem_b) return false;

        // 检查标签名
        if (elem_a->GetTagName() != elem_b->GetTagName()) {
            return false;
        }

        // TODO: 检查所有属性是否相同
        // 需要 Element 提供获取所有属性的接口
    }

    // 对于文本节点，检查内容
    if (a->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_a = std::dynamic_pointer_cast<Text>(a);
        auto text_b = std::dynamic_pointer_cast<Text>(b);

        if (!text_a || !text_b) return false;

        // 比较文本内容（忽略空白差异）
        std::string data_a = text_a->GetData();
        std::string data_b = text_b->GetData();

        // 简单的空白规范化
        auto normalize = [](const std::string& s) {
            std::string result;
            bool last_was_space = true;
            for (char c : s) {
                if (std::isspace(c)) {
                    if (!last_was_space) {
                        result += ' ';
                        last_was_space = true;
                    }
                } else {
                    result += c;
                    last_was_space = false;
                }
            }
            // 去除尾部空格
            while (!result.empty() && result.back() == ' ') {
                result.pop_back();
            }
            return result;
        };

        if (normalize(data_a) != normalize(data_b)) {
            return false;
        }
    }

    // 检查子节点数量
    const auto& children_a = a->GetChildNodes();
    const auto& children_b = b->GetChildNodes();

    if (children_a.size() != children_b.size()) {
        return false;
    }

    // 递归检查子节点
    for (size_t i = 0; i < children_a.size(); ++i) {
        if (!IsEquivalent(children_a[i], children_b[i])) {
            return false;
        }
    }

    return true;
}

void DOMSerializer::SerializeNode(std::shared_ptr<Node> node,
                                   std::string& output,
                                   int depth,
                                   const SerializeOptions& options) {
    if (!node) return;

    std::string indent = options.pretty_print ? GetIndent(depth, options.indent_size) : "";
    std::string newline = options.pretty_print ? "\n" : "";

    switch (node->GetNodeType()) {
        case NodeType::ELEMENT_NODE: {
            auto element = std::dynamic_pointer_cast<Element>(node);
            if (!element) break;

            std::string tag = element->GetTagName();
            std::string attrs = FormatAttributes(element);

            // 开始标签
            output += indent + "<" + tag + attrs;

            if (IsSelfClosingTag(tag)) {
                output += " />" + newline;
            } else {
                output += ">";

                // 子节点
                const auto& children = node->GetChildNodes();
                bool has_element_children = false;
                for (const auto& child : children) {
                    if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                        has_element_children = true;
                        break;
                    }
                }

                if (has_element_children && options.pretty_print) {
                    output += newline;
                }

                for (const auto& child : children) {
                    SerializeNode(child, output, depth + 1, options);
                }

                // 结束标签
                if (has_element_children && options.pretty_print) {
                    output += indent;
                }
                output += "</" + tag + ">" + newline;
            }
            break;
        }

        case NodeType::TEXT_NODE: {
            if (!options.include_text_nodes) break;

            auto text = std::dynamic_pointer_cast<Text>(node);
            if (!text) break;

            std::string content = text->GetData();

            // 跳过纯空白文本节点
            bool is_whitespace_only = true;
            for (char c : content) {
                if (!std::isspace(c)) {
                    is_whitespace_only = false;
                    break;
                }
            }

            if (!is_whitespace_only) {
                if (options.escape_special_chars) {
                    content = EscapeHTML(content);
                }
                output += content;
            }
            break;
        }

        // 注意：NodeType 枚举中没有 COMMENT_NODE，暂时跳过注释节点处理

        default:
            break;
    }
}

std::string DOMSerializer::EscapeHTML(const std::string& text) {
    std::string result;
    result.reserve(text.length() * 1.1);

    for (char c : text) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default:   result += c; break;
        }
    }

    return result;
}

std::string DOMSerializer::UnescapeHTML(const std::string& text) {
    std::string result = text;

    // 简单的实体替换
    auto replace_all = [](std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(result, "&amp;", "&");
    replace_all(result, "&lt;", "<");
    replace_all(result, "&gt;", ">");
    replace_all(result, "&quot;", "\"");
    replace_all(result, "&#39;", "'");

    return result;
}

std::string DOMSerializer::FormatAttributes(std::shared_ptr<Element> element) {
    if (!element) return "";

    std::string result;

    // 获取常见属性
    std::string id = element->GetAttribute("id");
    if (!id.empty()) {
        result += " id=\"" + EscapeHTML(id) + "\"";
    }

    std::string cls = element->GetAttribute("class");
    if (!cls.empty()) {
        result += " class=\"" + EscapeHTML(cls) + "\"";
    }

    std::string style = element->GetAttribute("style");
    if (!style.empty()) {
        result += " style=\"" + EscapeHTML(style) + "\"";
    }

    // TODO: 遍历所有其他属性
    // 需要 Element 提供获取所有属性的接口

    return result;
}

std::string DOMSerializer::GetIndent(int depth, int indent_size) {
    return std::string(depth * indent_size, ' ');
}

bool DOMSerializer::IsSelfClosingTag(const std::string& tag_name) {
    static const std::vector<std::string> self_closing = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };

    std::string lower_tag = tag_name;
    std::transform(lower_tag.begin(), lower_tag.end(), lower_tag.begin(), ::tolower);

    return std::find(self_closing.begin(), self_closing.end(), lower_tag) != self_closing.end();
}

} // namespace lightui
