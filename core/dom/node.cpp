/**
 * @file node.cpp
 * @brief DOM节点基类实现
 */

#include "node.h"
#include "text.h"
#include "document.h"
#include "element.h"
#include "dom_observer.h"
#include "core/render/render_object.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace lightui {

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

    // 如果child已有父节点，先从原父节点移除
    if (auto parent = child->GetParentNode()) {
        parent->RemoveChild(child);
    }

    // 添加到子节点列表
    child_nodes_.push_back(child);
    child->SetParentNode(shared_from_this());

    // 标记为脏
    MarkDirty();

    // 通知观察者
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyNodeAdded(child.get(), this);
        // 标记 Lexbor DOM 需要同步
        doc->MarkLexborDirty();
    }

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

    // 查找ref_child的位置
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), ref_child);
    if (it == child_nodes_.end()) {
        throw std::invalid_argument("Reference child not found");
    }

    // 如果new_child已有父节点，先从原父节点移除
    if (auto parent = new_child->GetParentNode()) {
        parent->RemoveChild(new_child);
    }

    // 在ref_child前插入
    child_nodes_.insert(it, new_child);
    new_child->SetParentNode(shared_from_this());

    // 标记为脏
    MarkDirty();

    // 通知观察者
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyNodeAdded(new_child.get(), this);
        // 标记 Lexbor DOM 需要同步
        doc->MarkLexborDirty();
    }

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

    // 通知观察者（在移除之前）
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyNodeRemoved(child.get(), this);
        
        // 如果被移除的是元素，清理其 ID 缓存（包括所有后代）
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::static_pointer_cast<Element>(child);
            doc->UnregisterElementAndDescendantIds(element);
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

    return child;
}

std::shared_ptr<Node> Node::ReplaceChild(std::shared_ptr<Node> new_child,
                                          std::shared_ptr<Node> old_child) {
    if (!new_child || !old_child) {
        throw std::invalid_argument("Cannot replace with/from null child");
    }

    // 查找old_child的位置
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), old_child);
    if (it == child_nodes_.end()) {
        throw std::invalid_argument("Old child not found");
    }

    // 如果new_child已有父节点，先从原父节点移除
    if (auto parent = new_child->GetParentNode()) {
        parent->RemoveChild(new_child);
    }

    // 通知观察者：旧节点被移除
    auto doc = GetOwnerDocument();
    if (doc) {
        doc->GetObserverManager().NotifyNodeRemoved(old_child.get(), this);
    }

    // 替换节点
    *it = new_child;
    old_child->SetParentNode(nullptr);
    new_child->SetParentNode(shared_from_this());

    // 通知观察者：新节点被添加
    if (doc) {
        doc->GetObserverManager().NotifyNodeAdded(new_child.get(), this);
    }

    // 标记为脏
    MarkDirty();

    // 标记 Lexbor DOM 需要同步
    if (doc) {
        doc->MarkLexborDirty();
    }

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
    // 移除所有子节点
    RemoveAllChildren();

    // 创建新的Text节点
    if (!content.empty()) {
        auto text_node = std::make_shared<Text>(content);
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

// ========== Protected方法 ==========

void Node::SetParentNode(std::shared_ptr<Node> parent) {
    parent_node_ = parent;
}

void Node::RemoveAllChildren() {
    // 通知观察者（在移除之前）
    auto doc = GetOwnerDocument();
    if (doc) {
        for (auto& child : child_nodes_) {
            doc->GetObserverManager().NotifyNodeRemoved(child.get(), this);
        }
    }

    // 清除所有子节点的父节点引用
    for (auto& child : child_nodes_) {
        child->SetParentNode(nullptr);
    }

    // 清空子节点列表
    child_nodes_.clear();

    // 标记为脏
    MarkDirty();
}

// ========== RenderObject 双向绑定 ==========

void Node::SetRenderObject(std::shared_ptr<RenderObject> render_obj) {
    render_object_ = render_obj;
}

std::shared_ptr<RenderObject> Node::GetRenderObject() const {
    return render_object_.lock();
}

} // namespace lightui
