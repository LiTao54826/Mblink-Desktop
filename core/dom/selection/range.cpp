/**
 * @file range.cpp
 * @brief DOM Range API 实现
 */

#include "range.h"
#include "core/dom/node.h"
#include "core/dom/document.h"
#include "core/dom/text.h"
#include "core/dom/element.h"
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace lightui {

Range::Range(std::shared_ptr<Document> owner_document)
    : owner_document_(owner_document)
    , start_offset_(0)
    , end_offset_(0) {
    // 默认情况下，Range 的边界设置为文档本身
    if (owner_document) {
        start_container_ = owner_document;
        end_container_ = owner_document;
    }
}

Range::~Range() = default;

// ========== 边界设置方法 ==========

void Range::SetStart(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        throw std::out_of_range("Offset out of range");
    }

    start_container_ = node;
    start_offset_ = offset;

    // 如果起始位置在结束位置之后，将结束位置设置为起始位置
    auto end_node = end_container_.lock();
    if (!end_node) {
        end_container_ = node;
        end_offset_ = offset;
    }
}

void Range::SetEnd(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        throw std::out_of_range("Offset out of range");
    }

    end_container_ = node;
    end_offset_ = offset;

    // 如果结束位置在起始位置之前，将起始位置设置为结束位置
    auto start_node = start_container_.lock();
    if (!start_node) {
        start_container_ = node;
        start_offset_ = offset;
    }
}

void Range::SetStartBefore(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetStart(parent, index);
}

void Range::SetStartAfter(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetStart(parent, index + 1);
}

void Range::SetEndBefore(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetEnd(parent, index);
}

void Range::SetEndAfter(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetEnd(parent, index + 1);
}

// ========== 选择方法 ==========

void Range::SelectNode(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    start_container_ = parent;
    start_offset_ = index;
    end_container_ = parent;
    end_offset_ = index + 1;
}

void Range::SelectNodeContents(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    start_container_ = node;
    start_offset_ = 0;
    end_container_ = node;
    end_offset_ = GetNodeLength(node);
}

void Range::Collapse(bool to_start) {
    if (to_start) {
        end_container_ = start_container_;
        end_offset_ = start_offset_;
    } else {
        start_container_ = end_container_;
        start_offset_ = end_offset_;
    }
}

// ========== 属性访问 ==========

bool Range::IsCollapsed() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return true;
    }

    return start_node == end_node && start_offset_ == end_offset_;
}

std::shared_ptr<Node> Range::GetCommonAncestorContainer() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return nullptr;
    }

    if (start_node == end_node) {
        return start_node;
    }

    return FindCommonAncestor(start_node, end_node);
}

// ========== 克隆和转换 ==========

std::shared_ptr<Range> Range::CloneRange() const {
    auto doc = owner_document_.lock();
    auto cloned = std::make_shared<Range>(doc);

    cloned->start_container_ = start_container_;
    cloned->start_offset_ = start_offset_;
    cloned->end_container_ = end_container_;
    cloned->end_offset_ = end_offset_;

    return cloned;
}

std::string Range::ToString() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return "";
    }

    // 如果起始和结束在同一个文本节点
    if (start_node == end_node && start_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(start_node);
        if (text_node) {
            std::string content = text_node->GetData();
            int start = std::max(0, start_offset_);
            int end = std::min(static_cast<int>(content.length()), end_offset_);
            if (start < end) {
                return content.substr(start, end - start);
            }
        }
        return "";
    }

    // 复杂情况：跨越多个节点
    std::string result;
    bool in_range = false;
    auto common_ancestor = GetCommonAncestorContainer();
    if (common_ancestor) {
        CollectText(common_ancestor, result, in_range);
    }

    return result;
}

// ========== 私有辅助方法 ==========

int Range::GetNodeIndex(std::shared_ptr<Node> node) const {
    if (!node) {
        return -1;
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        return -1;
    }

    const auto& children = parent->GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i] == node) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::shared_ptr<Node> Range::FindCommonAncestor(
    std::shared_ptr<Node> node1,
    std::shared_ptr<Node> node2) const {

    if (!node1 || !node2) {
        return nullptr;
    }

    // 收集 node1 的所有祖先
    std::vector<std::shared_ptr<Node>> ancestors1;
    auto current = node1;
    while (current) {
        ancestors1.push_back(current);
        current = current->GetParentNode();
    }

    // 从 node2 向上查找，找到第一个在 ancestors1 中的节点
    current = node2;
    while (current) {
        for (const auto& ancestor : ancestors1) {
            if (current == ancestor) {
                return current;
            }
        }
        current = current->GetParentNode();
    }

    return nullptr;
}

int Range::GetNodeLength(std::shared_ptr<Node> node) const {
    if (!node) {
        return 0;
    }

    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            return static_cast<int>(text_node->GetData().length());
        }
        return 0;
    }

    // 对于元素节点，返回子节点数量
    return static_cast<int>(node->GetChildNodes().size());
}

void Range::CollectText(std::shared_ptr<Node> node, std::string& result, bool& in_range) const {
    if (!node) {
        return;
    }

    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    // 检查是否进入 Range
    if (node == start_node) {
        in_range = true;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                std::string content = text_node->GetData();
                if (start_node == end_node) {
                    // 起始和结束在同一节点
                    int start = std::max(0, start_offset_);
                    int end = std::min(static_cast<int>(content.length()), end_offset_);
                    if (start < end) {
                        result += content.substr(start, end - start);
                    }
                    in_range = false;
                } else {
                    // 只取起始偏移之后的部分
                    int start = std::max(0, start_offset_);
                    if (start < static_cast<int>(content.length())) {
                        result += content.substr(start);
                    }
                }
            }
        }
    } else if (node == end_node) {
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                std::string content = text_node->GetData();
                int end = std::min(static_cast<int>(content.length()), end_offset_);
                if (end > 0) {
                    result += content.substr(0, end);
                }
            }
        }
        in_range = false;
    } else if (in_range && node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            result += text_node->GetData();
        }
    }

    // 递归处理子节点
    for (const auto& child : node->GetChildNodes()) {
        if (!in_range && child == end_node) {
            break;
        }
        CollectText(child, result, in_range);
        if (!in_range) {
            break;
        }
    }
}

} // namespace lightui
