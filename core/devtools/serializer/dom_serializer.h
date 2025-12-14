/**
 * @file dom_serializer.h
 * @brief DOM 序列化器
 */

#pragma once

#include <memory>
#include <string>
#include "core/dom/document.h"
#include "core/dom/node.h"
#include "core/dom/element.h"

namespace lightui {

/**
 * @brief 序列化选项
 */
struct SerializeOptions {
    int indent_size = 2;            // 缩进空格数
    bool include_text_nodes = true; // 是否包含文本节点
    bool pretty_print = true;       // 是否格式化输出
    bool escape_special_chars = true; // 是否转义特殊字符
};

/**
 * @brief DOM 序列化器
 */
class DOMSerializer {
public:
    /**
     * @brief 序列化节点为 HTML 字符串
     */
    static std::string Serialize(std::shared_ptr<Node> node,
                                  const SerializeOptions& options = {});

    /**
     * @brief 反序列化 HTML 字符串为 Document
     */
    static std::shared_ptr<Document> Deserialize(const std::string& html);

    /**
     * @brief 检查两个节点是否结构等价
     */
    static bool IsEquivalent(std::shared_ptr<Node> a, std::shared_ptr<Node> b);

private:
    static void SerializeNode(std::shared_ptr<Node> node,
                               std::string& output,
                               int depth,
                               const SerializeOptions& options);

    static std::string EscapeHTML(const std::string& text);
    static std::string UnescapeHTML(const std::string& text);
    static std::string FormatAttributes(std::shared_ptr<Element> element);
    static std::string GetIndent(int depth, int indent_size);
    static bool IsSelfClosingTag(const std::string& tag_name);
};

} // namespace lightui
