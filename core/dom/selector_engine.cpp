/**
 * @file selector_engine.cpp
 * @brief CSS 选择器引擎实现 - 使用 Lexbor
 */

#include "selector_engine.h"
#include "node.h"
#include <algorithm>
#include <cctype>

// 注意：Lexbor 集成将在后续实现
// 当前使用简化的选择器匹配作为回退

namespace lightui {

// ========== 简单选择器匹配（回退实现） ==========

bool SelectorEngine::MatchesSimpleSelector(const Element* element, const std::string& selector) {
    if (!element || selector.empty()) {
        return false;
    }

    // 通配符
    if (selector == "*") {
        return true;
    }

    // ID 选择器 (#id)
    if (selector[0] == '#') {
        std::string id = selector.substr(1);
        return element->GetAttribute("id") == id;
    }

    // 类选择器 (.class)
    if (selector[0] == '.') {
        std::string class_name = selector.substr(1);
        return element->HasClass(class_name);
    }

    // 属性选择器 ([name="value"])
    if (selector[0] == '[') {
        size_t end = selector.find(']');
        if (end == std::string::npos) {
            return false;
        }

        std::string attr_expr = selector.substr(1, end - 1);
        size_t eq_pos = attr_expr.find('=');

        if (eq_pos == std::string::npos) {
            // [name] - 只检查属性存在
            return element->HasAttribute(attr_expr);
        } else {
            // [name="value"] - 检查属性值
            std::string attr_name = attr_expr.substr(0, eq_pos);
            std::string attr_value = attr_expr.substr(eq_pos + 1);

            // 移除引号
            if (!attr_value.empty() &&
                (attr_value.front() == '"' || attr_value.front() == '\'')) {
                attr_value = attr_value.substr(1, attr_value.length() - 2);
            }

            return element->GetAttribute(attr_name) == attr_value;
        }
    }

    // 标签选择器 (div, span, etc.)
    return element->GetTagName() == selector;
}

std::shared_ptr<Element> SelectorEngine::QuerySelectorRecursive(
    std::shared_ptr<Node> node,
    const std::string& selector) {

    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element && MatchesSimpleSelector(element.get(), selector)) {
        return element;
    }

    for (const auto& child : node->GetChildNodes()) {
        auto result = QuerySelectorRecursive(child, selector);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

void SelectorEngine::QuerySelectorAllRecursive(
    std::shared_ptr<Node> node,
    const std::string& selector,
    std::vector<std::shared_ptr<Element>>& results) {

    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element && MatchesSimpleSelector(element.get(), selector)) {
        results.push_back(element);
    }

    for (const auto& child : node->GetChildNodes()) {
        QuerySelectorAllRecursive(child, selector, results);
    }
}

// ========== 公共 API ==========

std::shared_ptr<Element> SelectorEngine::QuerySelector(
    std::shared_ptr<Element> root,
    const std::string& selector) {

    if (!root) {
        return nullptr;
    }

    // TODO: 使用 Lexbor 实现
    // 当前使用简单实现
    std::shared_ptr<Node> root_node = std::static_pointer_cast<Node>(root);
    for (const auto& child : root_node->GetChildNodes()) {
        auto result = QuerySelectorRecursive(child, selector);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Element>> SelectorEngine::QuerySelectorAll(
    std::shared_ptr<Element> root,
    const std::string& selector) {

    std::vector<std::shared_ptr<Element>> results;

    if (!root) {
        return results;
    }

    // TODO: 使用 Lexbor 实现
    // 当前使用简单实现
    std::shared_ptr<Node> root_node = std::static_pointer_cast<Node>(root);
    for (const auto& child : root_node->GetChildNodes()) {
        QuerySelectorAllRecursive(child, selector, results);
    }

    return results;
}

bool SelectorEngine::Matches(
    std::shared_ptr<Element> element,
    const std::string& selector) {
    
    if (!element) {
        return false;
    }

    // TODO: 使用 Lexbor 实现
    // 当前使用简单实现
    return MatchesSimpleSelector(element.get(), selector);
}

std::shared_ptr<Element> SelectorEngine::Closest(
    std::shared_ptr<Element> element,
    const std::string& selector) {
    
    auto current = element;

    while (current) {
        if (Matches(current, selector)) {
            return current;
        }

        auto parent = current->GetParentNode();
        current = std::dynamic_pointer_cast<Element>(parent);
    }

    return nullptr;
}

} // namespace lightui

