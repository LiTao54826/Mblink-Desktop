/**
 * @file selector_engine.h
 * @brief CSS 选择器引擎 - 使用 Lexbor 实现
 *
 * 功能：
 * - 使用 Lexbor 库进行 CSS 选择器匹配
 * - 支持完整的 CSS3 选择器语法
 * - 提供 querySelector, querySelectorAll, matches 等功能
 *
 * 参考：RmlUi/Source/Core/Element.cpp - QuerySelector实现
 */

#pragma once

#include "element.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Lexbor前向声明
struct lxb_html_document;
struct lxb_dom_element;
struct lxb_dom_node;
struct lxb_css_parser;
struct lxb_selectors;

namespace lightui {

/**
 * @brief CSS 选择器引擎（使用 Lexbor）
 *
 * 实现策略：
 * 1. 将LightUI Element树转换为Lexbor DOM树
 * 2. 使用Lexbor的CSS选择器引擎进行查询
 * 3. 将Lexbor节点映射回LightUI Element
 */
class SelectorEngine {
public:
    /**
     * @brief 查询第一个匹配的元素
     * @param root 根元素
     * @param selector CSS 选择器
     * @return 第一个匹配的元素，如果没有则返回 nullptr
     */
    static std::shared_ptr<Element> QuerySelector(
        std::shared_ptr<Element> root,
        const std::string& selector);

    /**
     * @brief 查询所有匹配的元素
     * @param root 根元素
     * @param selector CSS 选择器
     * @return 所有匹配的元素列表
     */
    static std::vector<std::shared_ptr<Element>> QuerySelectorAll(
        std::shared_ptr<Element> root,
        const std::string& selector);

    /**
     * @brief 检查元素是否匹配选择器
     * @param element 要检查的元素
     * @param selector CSS 选择器
     * @return 是否匹配
     */
    static bool Matches(
        std::shared_ptr<Element> element,
        const std::string& selector);

    /**
     * @brief 查找最近的匹配祖先元素（包括自身）
     * @param element 起始元素
     * @param selector CSS 选择器
     * @return 最近的匹配元素，如果没有则返回 nullptr
     */
    static std::shared_ptr<Element> Closest(
        std::shared_ptr<Element> element,
        const std::string& selector);

private:
    /**
     * @brief Lexbor选择器上下文（线程局部，避免重复创建）
     */
    struct LexborContext {
        lxb_html_document* document;
        lxb_css_parser* css_parser;
        lxb_selectors* selectors;

        LexborContext();
        ~LexborContext();

        // 禁止拷贝
        LexborContext(const LexborContext&) = delete;
        LexborContext& operator=(const LexborContext&) = delete;
    };

    /**
     * @brief 获取线程局部的Lexbor上下文
     */
    static LexborContext& GetContext();

    /**
     * @brief 将LightUI Element树转换为Lexbor DOM树
     * @param element LightUI元素
     * @param lexbor_parent Lexbor父节点
     * @param element_map Element到Lexbor节点的映射
     * @return Lexbor DOM节点
     */
    static lxb_dom_element* ConvertToLexborDOM(
        std::shared_ptr<Element> element,
        lxb_dom_node* lexbor_parent,
        std::unordered_map<lxb_dom_element*, std::shared_ptr<Element>>& element_map);

    /**
     * @brief 简单选择器匹配（回退实现，当 Lexbor 不可用时使用）
     * @param element 要检查的元素
     * @param selector CSS 选择器
     * @return 是否匹配
     */
    static bool MatchesSimpleSelector(
        const Element* element,
        const std::string& selector);

    /**
     * @brief 递归查询第一个匹配的元素（简单实现）
     */
    static std::shared_ptr<Element> QuerySelectorRecursive(
        std::shared_ptr<Node> node,
        const std::string& selector);

    /**
     * @brief 递归查询所有匹配的元素（简单实现）
     */
    static void QuerySelectorAllRecursive(
        std::shared_ptr<Node> node,
        const std::string& selector,
        std::vector<std::shared_ptr<Element>>& results);
};

} // namespace lightui

