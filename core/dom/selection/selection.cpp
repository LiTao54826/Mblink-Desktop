/**
 * @file selection.cpp
 * @brief DOM Selection API 实现
 */

#include "selection.h"
#include "range.h"
#include "core/dom/node.h"
#include "core/dom/document.h"
#include "core/dom/text.h"
#include <algorithm>

namespace lightui {

Selection::Selection(std::shared_ptr<Document> document)
    : document_(document)
    , anchor_offset_(0)
    , focus_offset_(0) {
}

Selection::~Selection() = default;

// ========== 属性访问 ==========

bool Selection::IsCollapsed() const {
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        return true;
    }

    return anchor == focus && anchor_offset_ == focus_offset_;
}

// ========== 选择操作 ==========

void Selection::Collapse(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        // 如果 node 为 null，清除选择
        anchor_node_.reset();
        focus_node_.reset();
        anchor_offset_ = 0;
        focus_offset_ = 0;
        ranges_.clear();
        return;
    }

    anchor_node_ = node;
    focus_node_ = node;
    anchor_offset_ = offset;
    focus_offset_ = offset;

    UpdateRangeFromSelection();
}

void Selection::Extend(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        return;
    }

    // 保持锚点不变，只更新焦点
    focus_node_ = node;
    focus_offset_ = offset;

    UpdateRangeFromSelection();
}

void Selection::SelectAllChildren(std::shared_ptr<Node> node) {
    if (!node) {
        return;
    }

    // 辅助函数：查找第一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstTextNode;
    findFirstTextNode = [&](std::shared_ptr<Node> n) -> std::shared_ptr<Text> {
        if (!n) return nullptr;
        if (n->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(n);
        }
        if (n->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(n);
            if (elem) {
                for (auto& child : elem->GetChildNodes()) {
                    auto result = findFirstTextNode(child);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 辅助函数：查找最后一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findLastTextNode;
    findLastTextNode = [&](std::shared_ptr<Node> n) -> std::shared_ptr<Text> {
        if (!n) return nullptr;
        if (n->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(n);
        }
        if (n->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(n);
            if (elem) {
                auto children = elem->GetChildNodes();
                for (auto it = children.rbegin(); it != children.rend(); ++it) {
                    auto result = findLastTextNode(*it);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    auto first_text = findFirstTextNode(node);
    auto last_text = findLastTextNode(node);

    if (first_text && last_text) {
        // 设置锚点为第一个文本节点的开头
        anchor_node_ = first_text;
        anchor_offset_ = 0;

        // 设置焦点为最后一个文本节点的末尾
        focus_node_ = last_text;
        focus_offset_ = static_cast<int>(last_text->GetData().length());
    } else {
        // 没有文本节点，回退到原来的行为
        anchor_node_ = node;
        anchor_offset_ = 0;
        focus_node_ = node;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                focus_offset_ = static_cast<int>(text_node->GetData().length());
            } else {
                focus_offset_ = 0;
            }
        } else {
            focus_offset_ = static_cast<int>(node->GetChildNodes().size());
        }
    }

    UpdateRangeFromSelection();
}

void Selection::CollapseToStart() {
    auto anchor = anchor_node_.lock();
    if (anchor) {
        Collapse(anchor, anchor_offset_);
    }
}

void Selection::CollapseToEnd() {
    auto focus = focus_node_.lock();
    if (focus) {
        Collapse(focus, focus_offset_);
    }
}

// ========== Range 管理 ==========

void Selection::RemoveAllRanges() {
    ranges_.clear();
    anchor_node_.reset();
    focus_node_.reset();
    anchor_offset_ = 0;
    focus_offset_ = 0;
}

void Selection::AddRange(std::shared_ptr<Range> range) {
    if (!range) {
        return;
    }

    // 浏览器通常只支持一个 Range
    ranges_.clear();
    ranges_.push_back(range);

    // 从 Range 更新锚点和焦点
    anchor_node_ = range->GetStartContainer();
    anchor_offset_ = range->GetStartOffset();
    focus_node_ = range->GetEndContainer();
    focus_offset_ = range->GetEndOffset();
}

std::shared_ptr<Range> Selection::GetRangeAt(int index) const {
    if (index < 0 || index >= static_cast<int>(ranges_.size())) {
        return nullptr;
    }
    return ranges_[index];
}

// ========== 转换 ==========

std::string Selection::ToString() const {
    if (ranges_.empty()) {
        return "";
    }

    return ranges_[0]->ToString();
}

// ========== 内部更新 ==========

void Selection::UpdateFromUserAction(
    std::shared_ptr<Node> anchor, int anchor_offset,
    std::shared_ptr<Node> focus, int focus_offset) {

    anchor_node_ = anchor;
    anchor_offset_ = anchor_offset;
    focus_node_ = focus;
    focus_offset_ = focus_offset;

    UpdateRangeFromSelection();
}

void Selection::UpdateRangeFromSelection() {
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        ranges_.clear();
        return;
    }

    auto doc = document_.lock();
    if (!doc) {
        ranges_.clear();
        return;
    }

    // 创建或更新 Range
    std::shared_ptr<Range> range;
    if (ranges_.empty()) {
        range = doc->CreateRange();
        ranges_.push_back(range);
    } else {
        range = ranges_[0];
    }

    // 确定 Range 的起始和结束位置
    // 注意：Selection 的锚点可能在焦点之后（反向选择）
    // 但 Range 的 start 必须在 end 之前
    
    // 简化处理：假设锚点在焦点之前
    // TODO: 实现正确的位置比较
    try {
        range->SetStart(anchor, anchor_offset_);
        range->SetEnd(focus, focus_offset_);
    } catch (...) {
        // 如果设置失败（例如位置无效），尝试反向设置
        try {
            range->SetStart(focus, focus_offset_);
            range->SetEnd(anchor, anchor_offset_);
        } catch (...) {
            // 如果仍然失败，清除 Range
            ranges_.clear();
        }
    }
}

} // namespace lightui
