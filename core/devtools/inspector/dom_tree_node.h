/**
 * @file dom_tree_node.h
 * @brief DOM 树节点渲染辅助类
 */

#pragma once

#include <string>
#include <memory>
#include "core/dom/node.h"
#include "core/dom/element.h"

namespace lightui {

/**
 * @brief DOM 树节点格式化工具
 */
class DOMTreeNode {
public:
    /**
     * @brief 格式化节点为显示字符串
     */
    static std::string FormatNode(std::shared_ptr<Node> node);

    /**
     * @brief 格式化元素开始标签
     */
    static std::string FormatOpenTag(std::shared_ptr<Element> element);

    /**
     * @brief 格式化元素结束标签
     */
    static std::string FormatCloseTag(std::shared_ptr<Element> element);

    /**
     * @brief 格式化属性
     */
    static std::string FormatAttributes(std::shared_ptr<Element> element);

    /**
     * @brief 截断文本内容
     */
    static std::string TruncateText(const std::string& text, size_t max_length = 50);

    /**
     * @brief 检查元素是否为自闭合标签
     */
    static bool IsSelfClosingTag(const std::string& tag_name);
};

} // namespace lightui
