/**
 * @file selector_engine.h
 * @brief CSS 选择器引擎 - 使用 Lexbor 实现
 * 
 * 功能：
 * - 使用 Lexbor 库进行 CSS 选择器匹配
 * - 支持完整的 CSS3 选择器语法
 * - 提供 querySelector, querySelectorAll, matches 等功能
 */

#pragma once

#include "element.h"
#include <string>
#include <vector>
#include <memory>

namespace lightui {

/**
 * @brief CSS 选择器引擎（使用 Lexbor）
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

