/**
 * @file selection.cpp
 * @brief DOM Selection API 实现
 *
 * 参考 Blink 实现和 W3C Selection API 规范
 */

#include "selection.h"
#include "range.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/event.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include "core/event/loop/task_scheduler.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <iostream>

namespace mbink {

Selection::Selection(std::shared_ptr<Document> document)
    : document_(document)
    , anchor_offset_(0)
    , focus_offset_(0)
    , is_directional_(false) {
}

Selection::~Selection() = default;

// ========== 锚点和焦点属性 ==========

std::shared_ptr<Node> Selection::GetAnchorNode() const {
    return anchor_node_.lock();
}

int Selection::GetAnchorOffset() const {
    return anchor_offset_;
}

std::shared_ptr<Node> Selection::GetFocusNode() const {
    return focus_node_.lock();
}

int Selection::GetFocusOffset() const {
    return focus_offset_;
}

// ========== 计算后的位置属性（用于光标渲染） ==========
// 参考 Blink 的 Position::ComputeContainerNode/ComputeOffsetInContainerNode

std::shared_ptr<Node> Selection::GetComputedAnchorNode() const {
    auto anchor = anchor_node_.lock();
    if (!anchor) {
        return nullptr;
    }
    
    // 如果是 Element 节点，解析为 Text 节点
    if (anchor->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto result = ResolveElementPosition(anchor, anchor_offset_);
        return result.first;
    }
    return anchor;
}

int Selection::GetComputedAnchorOffset() const {
    auto anchor = anchor_node_.lock();
    if (!anchor) {
        return 0;
    }
    
    // 如果是 Element 节点，解析为 Text 节点位置
    if (anchor->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto result = ResolveElementPosition(anchor, anchor_offset_);
        return result.second;
    }
    return anchor_offset_;
}

std::shared_ptr<Node> Selection::GetComputedFocusNode() const {
    auto focus = focus_node_.lock();
    if (!focus) {
        return nullptr;
    }
    
    // 如果是 Element 节点，解析为 Text 节点
    if (focus->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto result = ResolveElementPosition(focus, focus_offset_);
        return result.first;
    }
    return focus;
}

int Selection::GetComputedFocusOffset() const {
    auto focus = focus_node_.lock();
    if (!focus) {
        return 0;
    }
    
    // 如果是 Element 节点，解析为 Text 节点位置
    if (focus->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto result = ResolveElementPosition(focus, focus_offset_);
        return result.second;
    }
    return focus_offset_;
}

// ========== 起始和结束属性（文档顺序） ==========

std::shared_ptr<Node> Selection::GetStartNode() const {
    if (IsAnchorFirst()) {
        return anchor_node_.lock();
    }
    return focus_node_.lock();
}

int Selection::GetStartOffset() const {
    if (IsAnchorFirst()) {
        return anchor_offset_;
    }
    return focus_offset_;
}

std::shared_ptr<Node> Selection::GetEndNode() const {
    if (IsAnchorFirst()) {
        return focus_node_.lock();
    }
    return anchor_node_.lock();
}

int Selection::GetEndOffset() const {
    if (IsAnchorFirst()) {
        return focus_offset_;
    }
    return anchor_offset_;
}

// ========== 状态属性 ==========

bool Selection::IsCollapsed() const {
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        return true;
    }

    return anchor == focus && anchor_offset_ == focus_offset_;
}

int Selection::GetRangeCount() const {
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        return 0;
    }

    return 1;
}

SelectionType Selection::GetType() const {
    if (GetRangeCount() == 0) {
        return SelectionType::kNone;
    }
    if (IsCollapsed()) {
        return SelectionType::kCaret;
    }
    return SelectionType::kRange;
}

SelectionDirection Selection::GetDirection() const {
    if (IsCollapsed()) {
        return SelectionDirection::kNone;
    }
    if (IsAnchorFirst()) {
        return SelectionDirection::kForward;
    }
    return SelectionDirection::kBackward;
}

std::string Selection::GetDirectionString() const {
    switch (GetDirection()) {
        case SelectionDirection::kForward:
            return "forward";
        case SelectionDirection::kBackward:
            return "backward";
        default:
            return "none";
    }
}

// ========== 核心选择操作 ==========

void Selection::SetBaseAndExtent(
    std::shared_ptr<Node> anchor_node, int anchor_offset,
    std::shared_ptr<Node> focus_node, int focus_offset) {

    // 根据 W3C 规范，如果 anchor_node 或 focus_node 为 null，清除选择
    if (!anchor_node || !focus_node) {
        RemoveAllRanges();
        return;
    }

    // 验证偏移量
    int anchor_max = GetNodeLength(anchor_node);
    int focus_max = GetNodeLength(focus_node);

    if (anchor_offset < 0 || anchor_offset > anchor_max) {
        anchor_offset = std::clamp(anchor_offset, 0, anchor_max);
    }

    if (focus_offset < 0 || focus_offset > focus_max) {
        focus_offset = std::clamp(focus_offset, 0, focus_max);
    }

    // 参考 Blink：直接存储原始位置，不做转换
    // 转换在 UpdateRangeFromSelection 中进行
    anchor_node_ = anchor_node;
    anchor_offset_ = anchor_offset;
    focus_node_ = focus_node;
    focus_offset_ = focus_offset;
    is_directional_ = true;

    // 更新内部 Range
    UpdateRangeFromSelection();
}

void Selection::Collapse(std::shared_ptr<Node> node, int offset) {
    // 根据 W3C 规范，如果 node 为 null，等同于 removeAllRanges()
    if (!node) {
        RemoveAllRanges();
        return;
    }

    // 验证偏移量
    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        offset = std::clamp(offset, 0, max_offset);
    }

    // 参考 Blink：直接存储原始位置
    anchor_node_ = node;
    anchor_offset_ = offset;
    focus_node_ = node;
    focus_offset_ = offset;

    // 更新内部 Range
    UpdateRangeFromSelection();
}

void Selection::Extend(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        return;
    }

    auto anchor = anchor_node_.lock();
    if (!anchor) {
        // 如果没有锚点，不能扩展
        return;
    }

    // 验证偏移量
    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        offset = std::clamp(offset, 0, max_offset);
    }

    // 参考 Blink：直接存储原始位置
    focus_node_ = node;
    focus_offset_ = offset;
    is_directional_ = true;

    UpdateRangeFromSelection();
}

void Selection::SelectAllChildren(std::shared_ptr<Node> node) {
    if (!node) {
        return;
    }

    // 设置锚点为节点开始，焦点为节点结束
    anchor_node_ = node;
    anchor_offset_ = 0;
    focus_node_ = node;
    focus_offset_ = GetNodeLength(node);

    UpdateRangeFromSelection();
}

void Selection::CollapseToStart() {
    auto start_node = GetStartNode();
    if (start_node) {
        Collapse(start_node, GetStartOffset());
    }
}

void Selection::CollapseToEnd() {
    auto end_node = GetEndNode();
    if (end_node) {
        Collapse(end_node, GetEndOffset());
    }
}

// ========== Range 管理 ==========

void Selection::RemoveAllRanges() {
    ranges_.clear();
    anchor_node_.reset();
    focus_node_.reset();
    anchor_offset_ = 0;
    focus_offset_ = 0;
    is_directional_ = false;
}

void Selection::AddRange(std::shared_ptr<Range> range) {
    if (!range) {
        return;
    }

    // 如果已经有选择，根据规范忽略（浏览器通常只支持一个 Range）
    if (GetRangeCount() > 0) {
        return;
    }

    // 从 Range 设置锚点和焦点
    auto start_container = range->GetStartContainer();
    auto end_container = range->GetEndContainer();

    if (!start_container || !end_container) {
        return;
    }

    anchor_node_ = start_container;
    anchor_offset_ = range->GetStartOffset();
    focus_node_ = end_container;
    focus_offset_ = range->GetEndOffset();

    ranges_.clear();
    ranges_.push_back(range);
}

void Selection::RemoveRange(std::shared_ptr<Range> range) {
    if (!range) {
        return;
    }

    auto it = std::find(ranges_.begin(), ranges_.end(), range);
    if (it != ranges_.end()) {
        ranges_.erase(it);
        // 如果移除了唯一的 Range，清除选择
        if (ranges_.empty()) {
            anchor_node_.reset();
            focus_node_.reset();
            anchor_offset_ = 0;
            focus_offset_ = 0;
        }
    }
}

std::shared_ptr<Range> Selection::GetRangeAt(int index) const {
    if (index < 0 || index >= GetRangeCount()) {
        return nullptr;
    }

    // 如果有缓存的 Range，返回它
    if (!ranges_.empty() && index < static_cast<int>(ranges_.size())) {
        return ranges_[index];
    }

    // 否则从选择状态创建 Range
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        return nullptr;
    }

    auto doc = document_.lock();
    if (!doc) {
        return nullptr;
    }

    auto range = doc->CreateRange();
    if (!range) {
        return nullptr;
    }

    // 设置 Range 的起始和结束（按文档顺序）
    try {
        if (IsAnchorFirst()) {
            range->SetStart(anchor, anchor_offset_);
            range->SetEnd(focus, focus_offset_);
        } else {
            range->SetStart(focus, focus_offset_);
            range->SetEnd(anchor, anchor_offset_);
        }
    } catch (...) {
        return nullptr;
    }

    return range;
}

// ========== 其他方法 ==========

void Selection::Empty() {
    RemoveAllRanges();
}

void Selection::DeleteFromDocument() {
    auto range = GetRangeAt(0);
    if (range) {
        // TODO: 实现 Range::deleteContents()
    }
}

bool Selection::ContainsNode(std::shared_ptr<Node> node, bool allow_partial) const {
    if (!node) {
        return false;
    }

    auto range = GetRangeAt(0);
    if (!range) {
        return false;
    }

    // TODO: 实现完整的 containsNode 逻辑
    return false;
}

std::string Selection::ToString() const {
    auto range = GetRangeAt(0);
    if (!range) {
        return "";
    }

    return range->ToString();
}

// ========== 内部更新 ==========

void Selection::UpdateFromUserAction(
    std::shared_ptr<Node> anchor, int anchor_offset,
    std::shared_ptr<Node> focus, int focus_offset) {

    SetBaseAndExtent(anchor, anchor_offset, focus, focus_offset);
}

// ========== 私有方法 ==========

int Selection::ComparePositions(
    std::shared_ptr<Node> node_a, int offset_a,
    std::shared_ptr<Node> node_b, int offset_b) const {

    if (!node_a || !node_b) {
        return 0;
    }

    // 如果是同一个节点，直接比较偏移量
    if (node_a == node_b) {
        return offset_a - offset_b;
    }

    // 检查 node_a 是否是 node_b 的祖先
    auto current = node_b;
    while (current) {
        if (current == node_a) {
            // node_a 是 node_b 的祖先
            // 需要比较 offset_a 和 node_b 在 node_a 中的索引
            return -1;  // 简化处理
        }
        current = current->GetParentNode();
    }

    // 检查 node_b 是否是 node_a 的祖先
    current = node_a;
    while (current) {
        if (current == node_b) {
            return 1;  // 简化处理
        }
        current = current->GetParentNode();
    }

    // 找到共同祖先并比较
    // 简化实现：使用深度优先遍历顺序
    // TODO: 实现完整的位置比较算法

    return 0;
}

bool Selection::IsAnchorFirst() const {
    auto anchor = anchor_node_.lock();
    auto focus = focus_node_.lock();

    if (!anchor || !focus) {
        return true;
    }

    int cmp = ComparePositions(anchor, anchor_offset_, focus, focus_offset_);
    return cmp <= 0;
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
        if (range) {
            ranges_.push_back(range);
        }
    } else {
        range = ranges_[0];
    }

    if (!range) {
        return;
    }

    // 将 Element 节点位置解析为 Text 节点位置
    // 这样 Range 的 startContainer/endContainer 就是 Text 节点
    auto resolved_anchor = ResolveElementPosition(anchor, anchor_offset_);
    auto resolved_focus = ResolveElementPosition(focus, focus_offset_);

    auto start_node = resolved_anchor.first;
    int start_offset = resolved_anchor.second;
    auto end_node = resolved_focus.first;
    int end_offset = resolved_focus.second;

    // 设置 Range 的起始和结束（按文档顺序）
    try {
        if (IsAnchorFirst()) {
            range->SetStart(start_node, start_offset);
            range->SetEnd(end_node, end_offset);
        } else {
            range->SetStart(end_node, end_offset);
            range->SetEnd(start_node, start_offset);
        }
    } catch (...) {
        ranges_.clear();
    }

    // 分发 selectionchange 事件到 Document
    // CodeMirror 6 等编辑器依赖此事件同步选择状态
    DispatchSelectionChangeEvent();
}

bool Selection::ValidateNodeOffset(std::shared_ptr<Node> node, int offset) const {
    if (!node) {
        return false;
    }

    int max_offset = GetNodeLength(node);
    return offset >= 0 && offset <= max_offset;
}

int Selection::GetNodeLength(std::shared_ptr<Node> node) const {
    if (!node) {
        return 0;
    }

    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            return static_cast<int>(utf8::CharCount(text_node->GetData()));
        }
        return 0;
    }

    // 对于元素节点，返回子节点数量
    return static_cast<int>(node->GetChildNodes().size());
}

std::pair<std::shared_ptr<Node>, int> Selection::ResolveElementPosition(
    std::shared_ptr<Node> node, int offset) const {

    if (!node || node->GetNodeType() != NodeType::ELEMENT_NODE) {
        return {node, offset};
    }

    auto elem = std::dynamic_pointer_cast<Element>(node);
    if (!elem) {
        return {node, offset};
    }

    const auto& children = elem->GetChildNodes();

    // 辅助函数：查找第一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findFirstText;
    findFirstText = [&](std::shared_ptr<Node> n) -> std::shared_ptr<Text> {
        if (!n) return nullptr;
        if (n->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(n);
        }
        if (n->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto e = std::dynamic_pointer_cast<Element>(n);
            if (e) {
                for (auto& child : e->GetChildNodes()) {
                    auto result = findFirstText(child);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    // 辅助函数：查找最后一个文本节点
    std::function<std::shared_ptr<Text>(std::shared_ptr<Node>)> findLastText;
    findLastText = [&](std::shared_ptr<Node> n) -> std::shared_ptr<Text> {
        if (!n) return nullptr;
        if (n->GetNodeType() == NodeType::TEXT_NODE) {
            return std::dynamic_pointer_cast<Text>(n);
        }
        if (n->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto e = std::dynamic_pointer_cast<Element>(n);
            if (e) {
                auto ch = e->GetChildNodes();
                for (auto it = ch.rbegin(); it != ch.rend(); ++it) {
                    auto result = findLastText(*it);
                    if (result) return result;
                }
            }
        }
        return nullptr;
    };

    if (children.empty()) {
        // 没有子节点：找第一个文本节点
        auto text_node = findFirstText(node);
        if (text_node) {
            return {text_node, 0};
        }
        return {node, 0};
    }

    if (offset == 0) {
        // offset=0：定位到第一个文本节点的开头
        auto text_node = findFirstText(node);
        if (text_node) {
            return {text_node, 0};
        }
        return {node, 0};
    }

    if (offset >= static_cast<int>(children.size())) {
        // offset >= 子节点数量：定位到最后一个子节点的最后一个文本节点的末尾
        // 注意：要从最后一个子节点开始找，而不是从 node 本身
        auto last_child = children.back();
        auto text_node = findLastText(last_child);
        if (text_node) {
            int text_len = static_cast<int>(utf8::CharCount(text_node->GetData()));
            return {text_node, text_len};
        }
        return {node, offset};
    }

    // offset 在有效范围内 (0 < offset < children.size())
    // DOM 规范：offset 表示"在第 offset 个子节点之前"的位置
    // 即光标在 children[offset-1] 之后，children[offset] 之前
    //
    // 我们需要找到 children[offset-1] 的最后一个文本节点的末尾位置
    auto prev_child = children[offset - 1];
    auto text_node = findLastText(prev_child);
    if (text_node) {
        int text_len = static_cast<int>(utf8::CharCount(text_node->GetData()));
        return {text_node, text_len};
    }

    // 如果前一个子节点没有文本，尝试找下一个子节点的第一个文本
    auto next_child = children[offset];
    text_node = findFirstText(next_child);
    if (text_node) {
        return {text_node, 0};
    }

    return {node, offset};
}

void Selection::DispatchSelectionChangeEvent() {
    // 使用微任务延迟触发 selectionchange 事件
    // 这样可以避免在 CodeMirror 等编辑器的 updateSelection 过程中
    // 触发嵌套的 EditorView.update 调用
    // 
    // 参考：CodeMirror 6 的 ignore 机制会在 updateSelection 中调用
    // collapse/extend 等方法，如果同步触发 selectionchange 事件，
    // 会导致 "Calls to EditorView.update are not allowed while an update is in progress" 错误
    
    // 检查是否已经有待处理的 selectionchange 事件
    if (pending_selectionchange_) {
        return;
    }
    pending_selectionchange_ = true;

    std::weak_ptr<Selection> weak_self = shared_from_this();
    TaskScheduler::Instance().PostMicrotask([weak_self]() {
        auto self = weak_self.lock();
        if (!self) {
            return;
        }
        
        self->pending_selectionchange_ = false;
        
        auto doc = self->document_.lock();
        if (!doc) {
            return;
        }

        // 创建 selectionchange 事件
        // 根据规范，selectionchange 事件不冒泡，不可取消
        auto event = std::make_shared<Event>("selectionchange", false, false);

        // 获取 document 的 body 元素来分发事件
        // 因为 Document 没有 DispatchEvent，我们通过 body 分发
        auto body = doc->GetBody();
        if (body) {
            body->DispatchEvent(event);
        }
    });
}

} // namespace mbink
