/**
 * @file selection.cpp
 * @brief DOM Selection API 实现
 */

#include "selection.h"
#include "range.h"
#include "node.h"
#include "document.h"
#include "text.h"
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

    // 设置锚点为节点的第一个位置
    anchor_node_ = node;
    anchor_offset_ = 0;

    // 设置焦点为节点的最后一个位置
    focus_node_ = node;
    
    // 计算节点的长度
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
