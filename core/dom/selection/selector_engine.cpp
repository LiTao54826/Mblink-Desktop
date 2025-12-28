/**
 * @file selector_engine.cpp
 * @brief CSS 选择器引擎实现 - 使用 Lexbor
 *
 * 参考：RmlUi/Source/Core/Element.cpp - QuerySelector实现
 */

#include "selector_engine.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include <algorithm>
#include <cctype>
#include <iostream>

// Lexbor头文件
#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

namespace lightui {

// ========== Lexbor上下文管理 ==========

SelectorEngine::LexborContext::LexborContext()
    : document(nullptr)
    , css_parser(nullptr)
    , selectors(nullptr) {

    // 创建HTML文档
    document = lxb_html_document_create();
    if (!document) {
        return;
    }

    // 注意：不需要调用lxb_html_document_init，因为我们手动构建DOM树

    // 创建CSS解析器
    css_parser = lxb_css_parser_create();
    if (!css_parser) {
        lxb_html_document_destroy(document);
        document = nullptr;
        return;
    }

    lxb_status_t status = lxb_css_parser_init(css_parser, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(css_parser, true);
        lxb_html_document_destroy(document);
        document = nullptr;
        css_parser = nullptr;
        return;
    }

    // 创建选择器引擎
    selectors = lxb_selectors_create();
    if (!selectors) {
        lxb_css_parser_destroy(css_parser, true);
        lxb_html_document_destroy(document);
        document = nullptr;
        css_parser = nullptr;
        return;
    }

    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
        lxb_selectors_destroy(selectors, true);
        lxb_css_parser_destroy(css_parser, true);
        lxb_html_document_destroy(document);
        document = nullptr;
        css_parser = nullptr;
        selectors = nullptr;
        return;
    }
}

SelectorEngine::LexborContext::~LexborContext() {
    if (selectors) {
        lxb_selectors_destroy(selectors, true);
    }
    if (css_parser) {
        lxb_css_parser_destroy(css_parser, true);
    }
    if (document) {
        lxb_html_document_destroy(document);
    }
}

SelectorEngine::LexborContext& SelectorEngine::GetContext() {
    // 使用线程局部存储，避免多线程问题
    thread_local LexborContext context;
    return context;
}

// ========== Element到Lexbor DOM转换 ==========

lxb_dom_element* SelectorEngine::ConvertToLexborDOM(
    std::shared_ptr<Element> element,
    lxb_dom_node* lexbor_parent,
    std::unordered_map<lxb_dom_element*, std::shared_ptr<Element>>& element_map) {

    if (!element) {
        return nullptr;
    }

    LexborContext& ctx = GetContext();
    if (!ctx.document) {
        return nullptr;
    }

    // 创建Lexbor元素
    // 注意：必须将 tag_name 存储在局部变量中，避免临时对象被销毁
    lxb_dom_document_t* doc = lxb_dom_interface_document(ctx.document);
    std::string tag_name_str = element->GetTagName();
    const lxb_char_t* tag_name = reinterpret_cast<const lxb_char_t*>(tag_name_str.c_str());
    size_t tag_len = tag_name_str.length();

    lxb_dom_element_t* lexbor_elem = lxb_dom_document_create_element(
        doc, tag_name, tag_len, nullptr);

    if (!lexbor_elem) {
        return lexbor_elem;
    }

    // 添加到父节点
    if (lexbor_parent) {
        lxb_dom_node_insert_child(lexbor_parent, lxb_dom_interface_node(lexbor_elem));
    }

    // 复制属性
    for (const auto& [attr_name, attr_value] : element->GetAllAttributes()) {
        const lxb_char_t* name_ptr = reinterpret_cast<const lxb_char_t*>(attr_name.c_str());
        const lxb_char_t* value_ptr = reinterpret_cast<const lxb_char_t*>(attr_value.c_str());

        lxb_dom_element_set_attribute(
            lexbor_elem,
            name_ptr, attr_name.length(),
            value_ptr, attr_value.length());
    }

    // 建立映射
    element_map[lexbor_elem] = element;

    // 递归转换子节点
    for (const auto& child : element->GetChildNodes()) {
        auto child_element = std::dynamic_pointer_cast<Element>(child);
        if (child_element) {
            ConvertToLexborDOM(child_element, lxb_dom_interface_node(lexbor_elem), element_map);
        }
        // 注意：暂时忽略文本节点，因为选择器通常不需要它们
    }

    return lexbor_elem;
}

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

    if (!root || selector.empty()) {
        return nullptr;
    }

    LexborContext& ctx = GetContext();
    if (!ctx.document || !ctx.css_parser || !ctx.selectors) {
        // Lexbor初始化失败，使用回退实现
        std::shared_ptr<Node> root_node = std::static_pointer_cast<Node>(root);
        for (const auto& child : root_node->GetChildNodes()) {
            auto result = QuerySelectorRecursive(child, selector);
            if (result) {
                return result;
            }
        }
        return nullptr;
    }

    // 1. 将Element树转换为Lexbor DOM树
    std::unordered_map<lxb_dom_element*, std::shared_ptr<Element>> element_map;
    lxb_dom_element* lexbor_root = ConvertToLexborDOM(root, nullptr, element_map);

    if (!lexbor_root) {
        return nullptr;
    }

    // 2. 解析CSS选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        ctx.css_parser,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );

    if (!list || ctx.css_parser->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_root));
        return nullptr;
    }

    // 3. 使用Lexbor选择器查找
    lxb_dom_element_t* result_elem = nullptr;

    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t spec, void* ctx) -> lxb_status_t {
        lxb_dom_element_t** result_ptr = static_cast<lxb_dom_element_t**>(ctx);
        *result_ptr = lxb_dom_interface_element(node);
        return LXB_STATUS_STOP;  // 只需要第一个
    };

    lxb_dom_node_t* search_root = lxb_dom_interface_node(lexbor_root);
    lxb_selectors_find(ctx.selectors, search_root, list, callback, &result_elem);

    lxb_css_selector_list_destroy_memory(list);

    // 4. 映射回LightUI Element
    std::shared_ptr<Element> result = nullptr;
    if (result_elem) {
        auto it = element_map.find(result_elem);
        if (it != element_map.end()) {
            result = it->second;
        }
    }

    // 5. 清理Lexbor DOM树
    lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_root));

    return result;
}

std::vector<std::shared_ptr<Element>> SelectorEngine::QuerySelectorAll(
    std::shared_ptr<Element> root,
    const std::string& selector) {

    std::vector<std::shared_ptr<Element>> results;

    if (!root || selector.empty()) {
        return results;
    }

    LexborContext& ctx = GetContext();
    if (!ctx.document || !ctx.css_parser || !ctx.selectors) {
        // Lexbor初始化失败，使用回退实现
        std::shared_ptr<Node> root_node = std::static_pointer_cast<Node>(root);
        for (const auto& child : root_node->GetChildNodes()) {
            QuerySelectorAllRecursive(child, selector, results);
        }
        return results;
    }

    // 1. 将Element树转换为Lexbor DOM树
    std::unordered_map<lxb_dom_element*, std::shared_ptr<Element>> element_map;
    lxb_dom_element* lexbor_root = ConvertToLexborDOM(root, nullptr, element_map);

    if (!lexbor_root) {
        return results;
    }

    // 2. 解析CSS选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        ctx.css_parser,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );

    if (!list || ctx.css_parser->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        // 清理Lexbor DOM树
        lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_root));
        return results;
    }

    // 3. 使用Lexbor选择器查找所有匹配元素
    std::vector<lxb_dom_element_t*> lexbor_results;

    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t spec, void* ctx) -> lxb_status_t {
        auto* results_ptr = static_cast<std::vector<lxb_dom_element_t*>*>(ctx);
        results_ptr->push_back(lxb_dom_interface_element(node));
        return LXB_STATUS_OK;
    };

    lxb_dom_node_t* search_root = lxb_dom_interface_node(lexbor_root);
    lxb_selectors_find(ctx.selectors, search_root, list, callback, &lexbor_results);

    lxb_css_selector_list_destroy_memory(list);

    // 4. 映射回LightUI Element
    for (auto* lexbor_elem : lexbor_results) {
        auto it = element_map.find(lexbor_elem);
        if (it != element_map.end()) {
            results.push_back(it->second);
        }
    }

    // 5. 清理Lexbor DOM树
    lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_root));

    return results;
}

bool SelectorEngine::Matches(
    std::shared_ptr<Element> element,
    const std::string& selector) {

    if (!element || selector.empty()) {
        return false;
    }

    LexborContext& ctx = GetContext();
    if (!ctx.document || !ctx.css_parser || !ctx.selectors) {
        // Lexbor初始化失败，使用回退实现
        return MatchesSimpleSelector(element.get(), selector);
    }

    // 1. 将Element转换为Lexbor DOM节点
    std::unordered_map<lxb_dom_element*, std::shared_ptr<Element>> element_map;
    lxb_dom_element* lexbor_elem = ConvertToLexborDOM(element, nullptr, element_map);

    if (!lexbor_elem) {
        return false;
    }

    // 2. 解析CSS选择器
    lxb_css_selector_list_t* list = lxb_css_selectors_parse(
        ctx.css_parser,
        reinterpret_cast<const lxb_char_t*>(selector.c_str()),
        selector.length()
    );

    if (!list || ctx.css_parser->status != LXB_STATUS_OK) {
        if (list) {
            lxb_css_selector_list_destroy_memory(list);
        }
        lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_elem));
        return false;
    }

    // 3. 使用Lexbor选择器匹配节点
    bool matched = false;

    auto callback = [](lxb_dom_node_t* node, lxb_css_selector_specificity_t spec, void* ctx) -> lxb_status_t {
        bool* matched_ptr = static_cast<bool*>(ctx);
        *matched_ptr = true;
        return LXB_STATUS_STOP;
    };

    lxb_dom_node_t* node = lxb_dom_interface_node(lexbor_elem);
    lxb_selectors_match_node(ctx.selectors, node, list, callback, &matched);

    lxb_css_selector_list_destroy_memory(list);

    // 4. 清理Lexbor DOM树
    lxb_dom_node_destroy_deep(lxb_dom_interface_node(lexbor_elem));

    return matched;
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

