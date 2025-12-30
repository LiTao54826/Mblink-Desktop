/**
 * @file document_fragment.h
 * @brief DocumentFragment 类定义
 * 
 * DocumentFragment 是一个轻量级的文档片段容器，
 * 用于高效地批量操作 DOM 节点。
 */

#pragma once

#include "node.h"

namespace lightui {

/**
 * @brief DocumentFragment 类
 * 
 * DocumentFragment 是一个没有父节点的最小文档对象。
 * 它被用作一个轻量级的 Document 版本，用于存储由节点组成的文档结构片段。
 * 
 * 主要用途：
 * - 批量添加节点到 DOM 树（避免多次重排）
 * - 作为临时容器存储节点
 */
class DocumentFragment : public Node {
public:
    DocumentFragment();
    ~DocumentFragment() override = default;

    /**
     * @brief 克隆节点
     * @param deep 是否深度克隆（包含子节点）
     * @return 克隆的节点
     */
    std::shared_ptr<Node> CloneNode(bool deep) override;
};

} // namespace lightui
