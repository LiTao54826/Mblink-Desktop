/**
 * @file node.cpp
 * @brief DOM节点基类实现
 */

#include "node.h"
#include "text.h"
#include "document.h"
#include "element.h"
#include "elements/html_style_element.h"
#include "elements/html_select_element.h"
#include "elements/html_option_element.h"
#include "observers/dom_observer.h"
#include "core/render/objects/render_object.h"
#include <algorithm>
#include <stdexcept>

namespace mbink {

namespace {

void NotifySelectOptionsChanged(const std::shared_ptr<Node>& parent,
                                const std::shared_ptr<Node>& child) {
    if (!parent || !child || child->GetNodeType() != NodeType::ELEMENT_NODE) {
        return;
    }

    auto parent_element = std::dynamic_pointer_cast<Element>(parent);
    if (!parent_element) {
        return;
    }

    auto tag_name = parent_element->GetTagName();
    bool affects_select = false;

    if (tag_name == "select") {
        affects_select = true;
    } else if (tag_name == "optgroup") {
        affects_select = true;
    }

    if (!affects_select) {
        return;
    }

    auto child_element = std::dynamic_pointer_cast<Element>(child);
    if (!child_element) {
        return;
    }

    auto child_tag_name = child_element->GetTagName();
    if (child_tag_name != "option" && child_tag_name != "optgroup") {
        return;
    }

    if (tag_name == "select") {
        if (auto select = std::dynamic_pointer_cast<HTMLSelectElement>(parent_element)) {
            select->OnOptionsChanged();
        }
        return;
    }

    auto select_parent = parent_element->GetParentNode();
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(select_parent);
    if (select) {
        select->OnOptionsChanged();
    }
}

void NotifyStyleElementTextChanged(const std::shared_ptr<Node>& node) {
    auto style = std::dynamic_pointer_cast<HTMLStyleElement>(node);
    if (style) {
        style->NotifyStyleUpdate();
    }
}

} // namespace

// ========== 构造函数和析构函数 ==========

Node::Node(NodeType type)
    : node_type_(type)
    , parent_node_()
    , child_nodes_()
    , is_dirty_(true) {
}

// ========== 节点关系 ==========

std::shared_ptr<Document> Node::GetOwnerDocument() const {
    // 如果当前节点就是 Document，返回 nullptr（Document 没有 owner document）
    if (node_type_ == NodeType::DOCUMENT_NODE) {
        return nullptr;
    }

    // 优先使用缓存的 owner_document_
    if (auto doc = owner_document_.lock()) {
        return doc;
    }

    // 如果没有缓存，向上遍历找到 Document 节点
    auto current = const_cast<Node*>(this)->shared_from_this();
    while (current) {
        if (current->GetNodeType() == NodeType::DOCUMENT_NODE) {
            auto doc = std::static_pointer_cast<Document>(current);
            // 缓存结果
            const_cast<Node*>(this)->owner_document_ = doc;
            return doc;
        }
        current = current->GetParentNode();
    }

    return nullptr;
}

bool Node::IsConnected() const {
    const Node* current = this;
    while (current) {
        if (current->GetNodeType() == NodeType::DOCUMENT_NODE) {
            return true;
        }
        auto parent = current->GetParentNode();
        current = parent.get();
    }
    return false;
}

uint32_t Node::CompareDocumentPosition(std::shared_ptr<Node> other) const {
    constexpr uint32_t kDisconnected = 0x01;
    constexpr uint32_t kPreceding = 0x02;
    constexpr uint32_t kFollowing = 0x04;
    constexpr uint32_t kContains = 0x08;
    constexpr uint32_t kContainedBy = 0x10;
    constexpr uint32_t kImplementationSpecific = 0x20;
    constexpr uint32_t kDisconnectedMask = kDisconnected | kImplementationSpecific | kPreceding;

    if (!other) {
        return kDisconnectedMask;
    }

    if (other.get() == this) {
        return 0;
    }

    auto build_chain = [](const Node* node) {
        std::vector<const Node*> chain;
        const Node* current = node;
        while (current) {
            chain.push_back(current);
            auto parent = current->GetParentNode();
            current = parent.get();
        }
        std::reverse(chain.begin(), chain.end());
        return chain;
    };

    auto this_chain = build_chain(this);
    auto other_chain = build_chain(other.get());
    if (this_chain.empty() || other_chain.empty() || this_chain.front() != other_chain.front()) {
        return kDisconnectedMask;
    }

    size_t shared_depth = 0;
    const size_t max_shared = std::min(this_chain.size(), other_chain.size());
    while (shared_depth < max_shared && this_chain[shared_depth] == other_chain[shared_depth]) {
        ++shared_depth;
    }

    if (shared_depth == this_chain.size()) {
        return kContainedBy | kFollowing;
    }

    if (shared_depth == other_chain.size()) {
        return kContains | kPreceding;
    }

    const Node* common_ancestor = this_chain[shared_depth - 1];
    const Node* this_child = this_chain[shared_depth];
    const Node* other_child = other_chain[shared_depth];

    for (const auto& child : common_ancestor->GetChildNodes()) {
        if (child.get() == other_child) {
            return kPreceding;
        }
        if (child.get() == this_child) {
            return kFollowing;
        }
    }

    return kDisconnectedMask;
}

// ========== 子节点访问 ==========

std::shared_ptr<Node> Node::GetFirstChild() const {
    if (child_nodes_.empty()) {
        return nullptr;
    }
    return child_nodes_.front();
}

std::shared_ptr<Node> Node::GetLastChild() const {
    if (child_nodes_.empty()) {
        return nullptr;
    }
    return child_nodes_.back();
}

// ========== 兄弟节点访问 ==========

std::shared_ptr<Node> Node::GetNextSibling() const {
    auto parent = parent_node_.lock();
    if (!parent) {
        return nullptr;
    }

    const auto& siblings = parent->child_nodes_;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::shared_ptr<Node>& node) {
            return node.get() == this;
        });

    if (it == siblings.end() || std::next(it) == siblings.end()) {
        return nullptr;
    }

    return *std::next(it);
}

std::shared_ptr<Node> Node::GetPreviousSibling() const {
    auto parent = parent_node_.lock();
    if (!parent) {
        return nullptr;
    }

    const auto& siblings = parent->child_nodes_;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::shared_ptr<Node>& node) {
            return node.get() == this;
        });

    if (it == siblings.end() || it == siblings.begin()) {
        return nullptr;
    }

    return *std::prev(it);
}

// ========== 子节点操作 ==========

std::shared_ptr<Node> Node::AppendChild(std::shared_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("Cannot append null child");
    }

    auto old_parent = child->GetParentNode();
    const bool is_move = static_cast<bool>(old_parent);

    // DOM move：不要走 RemoveChild()，否则会误报 removed。
    if (old_parent) {
        auto& old_siblings = old_parent->child_nodes_;
        auto old_it = std::find(old_siblings.begin(), old_siblings.end(), child);
        if (old_it == old_siblings.end()) {
            throw std::invalid_argument("Child not found in old parent");
        }
        old_siblings.erase(old_it);
        old_parent->MarkDirty();
        child->SetParentNode(nullptr);

        if (auto old_doc = old_parent->GetOwnerDocument()) {
            old_doc->MarkLexborDirty();
        }

        NotifySelectOptionsChanged(old_parent, child);
    }

    // 添加到子节点列表
    child_nodes_.push_back(child);
    child->SetParentNode(shared_from_this());

    // 传播 owner_document_ 给子节点（如果子节点没有的话）
    // 这确保通过 SetTextContent 等方法创建的节点也能正确获取 owner_document_
    auto doc = GetOwnerDocument();
    if (doc && !child->owner_document_.lock()) {
        child->owner_document_ = doc;
    }

    // 标记为脏
    MarkDirty();

    // 通知观察者和记录变化
    if (doc) {
        if (is_move) {
            doc->GetDirtyTracker().RecordNodeMoved(
                child, old_parent, shared_from_this(), child_nodes_.size() - 1);
        } else {
            doc->GetDirtyTracker().RecordNodeAdded(
                child, shared_from_this(), child_nodes_.size() - 1);
        }

        // move 场景仍需触发 added 路径，以便窗口进入重绘/布局流程
        doc->GetObserverManager().NotifyNodeAdded(child.get(), this);
        // 标记 Lexbor DOM 需要同步
        doc->MarkLexborDirty();
        if (auto childElement = std::dynamic_pointer_cast<Element>(child); childElement && childElement->IsConnected()) {
            doc->AutoMountNativeDeclarativeBindings(childElement);
        }
    }

    NotifySelectOptionsChanged(shared_from_this(), child);
    NotifyStyleElementTextChanged(shared_from_this());
    return child;
}

std::shared_ptr<Node> Node::InsertBefore(std::shared_ptr<Node> new_child,
                                          std::shared_ptr<Node> ref_child) {
    if (!new_child) {
        throw std::invalid_argument("Cannot insert null child");
    }

    // 如果ref_child为nullptr，等同于AppendChild
    if (!ref_child) {
        return AppendChild(new_child);
    }

    // DOM 语义：insertBefore(node, node) 等同于 no-op
    if (new_child == ref_child) {
        return new_child;
    }

    auto ref_it = std::find(child_nodes_.begin(), child_nodes_.end(), ref_child);
    if (ref_it == child_nodes_.end()) {
        throw std::invalid_argument("Reference child not found");
    }

    size_t index = std::distance(child_nodes_.begin(), ref_it);
    auto old_parent = new_child->GetParentNode();
    const bool is_move = static_cast<bool>(old_parent);

    if (old_parent && old_parent.get() == this) {
        auto existing_it = std::find(child_nodes_.begin(), child_nodes_.end(), new_child);
        if (existing_it != child_nodes_.end()) {
            size_t existing_index = std::distance(child_nodes_.begin(), existing_it);
            if (existing_index < index) {
                --index;
            }
        }
    }

    // DOM move：不要走 RemoveChild()，否则会误报 removed。
    if (old_parent) {
        auto& old_siblings = old_parent->child_nodes_;
        auto old_it = std::find(old_siblings.begin(), old_siblings.end(), new_child);
        if (old_it == old_siblings.end()) {
            throw std::invalid_argument("Child not found in old parent");
        }
        old_siblings.erase(old_it);
        old_parent->MarkDirty();
        new_child->SetParentNode(nullptr);

        if (auto old_doc = old_parent->GetOwnerDocument()) {
            old_doc->MarkLexborDirty();
        }

        NotifySelectOptionsChanged(old_parent, new_child);
    }

    if (index > child_nodes_.size()) {
        index = child_nodes_.size();
    }

    // 在 ref_child 前插入（重新按索引定位，避免移动后旧迭代器失效）
    child_nodes_.insert(child_nodes_.begin() + static_cast<std::ptrdiff_t>(index), new_child);
    new_child->SetParentNode(shared_from_this());

    // 传播 owner_document_ 给子节点（如果子节点没有的话）
    auto doc = GetOwnerDocument();
    if (doc && !new_child->owner_document_.lock()) {
        new_child->owner_document_ = doc;
    }

    // 标记为脏
    MarkDirty();

    // 通知观察者和记录变化
    if (doc) {
        if (is_move) {
            doc->GetDirtyTracker().RecordNodeMoved(new_child, old_parent, shared_from_this(), index);
        } else {
            doc->GetDirtyTracker().RecordNodeAdded(new_child, shared_from_this(), index);
        }

        doc->GetObserverManager().NotifyNodeAdded(new_child.get(), this);
        // 标记 Lexbor DOM 需要同步
        doc->MarkLexborDirty();
        if (auto childElement = std::dynamic_pointer_cast<Element>(new_child); childElement && childElement->IsConnected()) {
            doc->AutoMountNativeDeclarativeBindings(childElement);
        }
    }

    NotifySelectOptionsChanged(shared_from_this(), new_child);
    NotifyStyleElementTextChanged(shared_from_this());
    return new_child;
}

std::shared_ptr<Node> Node::RemoveChild(std::shared_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("Cannot remove null child");
    }

    // 查找child的位置
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), child);
    if (it == child_nodes_.end()) {
        throw std::invalid_argument("Child not found");
    }

    // 计算移除索引
    size_t index = std::distance(child_nodes_.begin(), it);

    // 通知观察者和记录变化（在移除之前）
    auto doc = GetOwnerDocument();
    if (doc) {
        // 记录到 DirtyNodeTracker（延迟处理）
        doc->GetDirtyTracker().RecordNodeRemoved(child, shared_from_this(), index);

        // 通知观察者（立即处理，用于兼容旧代码）
        doc->GetObserverManager().NotifyNodeRemoved(child.get(), this);

        // 如果被移除的是元素，清理其 ID 缓存（包括所有后代）
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::static_pointer_cast<Element>(child);
            doc->UnregisterElementAndDescendantIds(element);
            doc->UnmountNativeDeclarativeBindings(element);
        }
    }

    // 从子节点列表移除
    child_nodes_.erase(it);
    child->SetParentNode(nullptr);

    // 标记为脏
    MarkDirty();

    // 标记 Lexbor DOM 需要同步
    if (doc) {
        doc->MarkLexborDirty();
    }

    NotifySelectOptionsChanged(shared_from_this(), child);
    NotifyStyleElementTextChanged(shared_from_this());
    return child;
}

std::shared_ptr<Node> Node::ReplaceChild(std::shared_ptr<Node> new_child,
                                          std::shared_ptr<Node> old_child) {
    if (!new_child || !old_child) {
        throw std::invalid_argument("Cannot replace with/from null child");
    }

    if (new_child == old_child) {
        return old_child;
    }

    auto old_it = std::find(child_nodes_.begin(), child_nodes_.end(), old_child);
    if (old_it == child_nodes_.end()) {
        throw std::invalid_argument("Old child not found");
    }

    size_t index = std::distance(child_nodes_.begin(), old_it);
    auto old_parent_of_new_child = new_child->GetParentNode();

    if (old_parent_of_new_child && old_parent_of_new_child.get() == this) {
        auto existing_it = std::find(child_nodes_.begin(), child_nodes_.end(), new_child);
        if (existing_it != child_nodes_.end()) {
            size_t existing_index = std::distance(child_nodes_.begin(), existing_it);
            if (existing_index < index) {
                --index;
            }
        }
    }

    if (old_parent_of_new_child) {
        auto& old_siblings = old_parent_of_new_child->child_nodes_;
        auto old_new_child_it = std::find(old_siblings.begin(), old_siblings.end(), new_child);
        if (old_new_child_it == old_siblings.end()) {
            throw std::invalid_argument("New child not found in old parent");
        }
        old_siblings.erase(old_new_child_it);
        old_parent_of_new_child->MarkDirty();
        new_child->SetParentNode(nullptr);

        if (auto old_doc = old_parent_of_new_child->GetOwnerDocument()) {
            old_doc->MarkLexborDirty();
        }

        NotifySelectOptionsChanged(old_parent_of_new_child, new_child);
    }

    if (index >= child_nodes_.size()) {
        throw std::invalid_argument("Old child not found after reordering");
    }

    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetDirtyTracker().RecordNodeReplaced(old_child, new_child, shared_from_this(), index);
        doc->GetObserverManager().NotifyNodeRemoved(old_child.get(), this);
        if (old_child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto oldElement = std::static_pointer_cast<Element>(old_child);
            doc->UnregisterElementAndDescendantIds(oldElement);
            doc->UnmountNativeDeclarativeBindings(oldElement);
        }
    }

    child_nodes_[index] = new_child;
    old_child->SetParentNode(nullptr);
    new_child->SetParentNode(shared_from_this());

    if (doc) {
        doc->GetObserverManager().NotifyNodeAdded(new_child.get(), this);
    }

    MarkDirty();

    if (doc) {
        doc->MarkLexborDirty();
        if (auto childElement = std::dynamic_pointer_cast<Element>(new_child); childElement && childElement->IsConnected()) {
            doc->AutoMountNativeDeclarativeBindings(childElement);
        }
    }

    NotifySelectOptionsChanged(shared_from_this(), old_child);
    NotifySelectOptionsChanged(shared_from_this(), new_child);
    NotifyStyleElementTextChanged(shared_from_this());
    return old_child;
}

// ========== 其他操作 ==========

bool Node::Contains(std::shared_ptr<Node> other) const {
    if (!other) {
        return false;
    }

    // 遍历other的所有祖先节点
    auto current = other;
    while (current) {
        if (current.get() == this) {
            return true;
        }
        current = current->GetParentNode();
    }

    return false;
}

std::string Node::GetTextContent() const {
    std::string content;

    // 递归收集所有Text节点的内容
    for (const auto& child : child_nodes_) {
        content += child->GetTextContent();
    }

    return content;
}

void Node::SetTextContent(const std::string& content) {
    // 优化：如果只有一个 Text 子节点且内容非空，直接更新其内容
    // 这会触发 OnTextChanged，走增量更新路径，而不是删除重建
    // 参考 Blink 的增量更新机制
    if (child_nodes_.size() == 1 &&
        child_nodes_[0]->GetNodeType() == NodeType::TEXT_NODE &&
        !content.empty()) {
        auto text_node = std::static_pointer_cast<Text>(child_nodes_[0]);
        // 只有内容真正改变时才更新
        if (text_node->GetData() != content) {
            text_node->SetData(content);  // 这会触发 OnTextChanged
        }
        return;
    }

    // 如果内容为空且只有一个 Text 子节点，需要移除它
    // 如果有多个子节点或子节点不是 Text，需要重建

    // 移除所有子节点
    RemoveAllChildren();

    // 创建新的Text节点
    if (!content.empty()) {
        // 优先使用 Document::CreateTextNode 以确保 owner_document_ 被正确设置
        auto doc = GetOwnerDocument();
        std::shared_ptr<Text> text_node;
        if (doc) {
            text_node = doc->CreateTextNode(content);
        } else {
            // 回退：直接创建 Text 节点（没有 owner_document_）
            text_node = std::make_shared<Text>(content);
        }
        AppendChild(text_node);
    }
}

void Node::MarkDirty(DirtyType type) {
    // 更新脏标记标志
    dirty_flags_ |= static_cast<uint32_t>(type);

    // 兼容旧代码
    is_dirty_ = true;

    // 4.9 布局隔离回滚：
    // 恢复为标准的增量布局模式。
    // 即便对于 absolute/fixed 元素，也允许 LAYOUT 标记向上传播。
    // 这会触发 Root 的增量布局过程，利用 Cache 避免非必要的重排。
    // 这虽然不如完全隔离高效，但能保证绝对正确性，并能解决 Popup 不显示的问题。
    DirtyType propagate_type = type;

    // 向上传播脏标记
    if (auto parent = parent_node_.lock()) {
        parent->MarkDirty(propagate_type);
    }
}

void Node::ClearDirty(DirtyType type) {
    // 清除指定类型的脏标记
    dirty_flags_ &= ~static_cast<uint32_t>(type);

    // 如果所有脏标记都清除了，更新兼容标志
    if (dirty_flags_ == 0) {
        is_dirty_ = false;
    }

    // 如果清除了绘制标记，也清除脏矩形
    if ((static_cast<uint32_t>(type) & static_cast<uint32_t>(DirtyType::PAINT)) != 0) {
        dirty_rect_ = SkRect::MakeEmpty();
    }
}

// ========== 增量更新样式重算 ==========

void Node::MarkAncestorsWithChildNeedsStyleRecalc() {
    // 向上遍历祖先链，只设置 ChildNeedsStyleRecalc 标志
    // 不修改祖先的 StyleChangeType
    auto parent = parent_node_.lock();
    while (parent) {
        // 如果祖先已经有 ChildNeedsStyleRecalc 标志，停止遍历
        // 因为更上层的祖先肯定也已经被标记过了
        if (parent->ChildNeedsStyleRecalc()) {
            break;
        }

        // 只设置 ChildNeedsStyleRecalc 标志
        parent->SetChildNeedsStyleRecalc();

        // 继续向上遍历
        parent = parent->GetParentNode();
    }
}

void Node::SetNeedsStyleRecalc(StyleChangeType change_type) {
    // 如果请求的是 kNoStyleChange，直接返回
    if (change_type == StyleChangeType::kNoStyleChange) {
        return;
    }

    // 如果当前的 StyleChangeType 已经是更高级别的变化，不需要降级
    // kSubtreeStyleChange > kLocalStyleChange > kNoStyleChange
    StyleChangeType current_type = GetStyleChangeType();
    if (static_cast<uint32_t>(current_type) >= static_cast<uint32_t>(change_type)) {
        return;
    }

    // 设置节点的 StyleChangeType
    SetStyleChange(change_type);

    // 标记祖先链
    MarkAncestorsWithChildNeedsStyleRecalc();
}

// ========== 增量更新布局 ==========

void Node::MarkAncestorsWithChildNeedsLayout() {
    // 向上遍历祖先链，只设置 ChildNeedsLayout 标志
    // 不修改祖先的 NeedsLayout 标志
    auto parent = parent_node_.lock();
    while (parent) {
        // 如果祖先已经有 ChildNeedsLayout 标志，停止遍历
        // 因为更上层的祖先肯定也已经被标记过了
        if (parent->ChildNeedsLayout()) {
            break;
        }

        // 只设置 ChildNeedsLayout 标志
        parent->SetChildNeedsLayout();

        // 继续向上遍历
        parent = parent->GetParentNode();
    }
}

void Node::SetNeedsLayout() {
    // 如果已经标记为需要布局，直接返回
    if (NeedsLayoutFlag()) {
        return;
    }

    // 设置节点的 NeedsLayout 标志
    SetNeedsLayoutFlag();

    // 标记祖先链
    MarkAncestorsWithChildNeedsLayout();
}

// ========== Protected方法 ==========

void Node::SetParentNode(std::shared_ptr<Node> parent) {
    parent_node_ = parent;
}

void Node::RemoveAllChildren() {
    if (child_nodes_.empty()) {
        return;
    }

    auto self = shared_from_this();
    auto removed_children = child_nodes_;
    auto doc = GetOwnerDocument();

    // 通知观察者和记录变化（在移除之前）
    if (doc) {
        for (size_t i = 0; i < removed_children.size(); ++i) {
            auto& child = removed_children[i];
            doc->GetDirtyTracker().RecordNodeRemoved(child, self, i);
            doc->GetObserverManager().NotifyNodeRemoved(child.get(), this);

            if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(child);
                doc->UnregisterElementAndDescendantIds(element);
                doc->UnmountNativeDeclarativeBindings(element);
            }
        }
    }

    // 清除所有子节点的父节点引用
    for (auto& child : removed_children) {
        child->SetParentNode(nullptr);
        NotifySelectOptionsChanged(self, child);
    }

    // 清空子节点列表
    child_nodes_.clear();

    // 标记为脏
    MarkDirty();

    // 标记 Lexbor DOM 需要同步
    if (doc) {
        doc->MarkLexborDirty();
    }
}

// ========== RenderObject 双向绑定 ==========

void Node::SetRenderObject(std::shared_ptr<RenderObject> render_obj) {
    render_object_ = render_obj;
}

std::shared_ptr<RenderObject> Node::GetRenderObject() const {
    return render_object_.lock();
}

} // namespace mbink
