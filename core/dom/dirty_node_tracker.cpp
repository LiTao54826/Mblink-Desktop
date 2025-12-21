/**
 * @file dirty_node_tracker.cpp
 * @brief 脏节点追踪器实现
 */

#include "dirty_node_tracker.h"
#include "node.h"
#include "element.h"
#include <algorithm>

namespace lightui {

void DirtyNodeTracker::RecordNodeAdded(std::shared_ptr<Node> node, 
                                       std::shared_ptr<Node> parent, 
                                       size_t index) {
    if (!node || !parent) return;
    
    StructuralChange change;
    change.type = StructuralChangeType::Added;
    change.node = node;
    change.parent = parent;
    change.index = index;
    
    structural_changes_.push_back(change);
    added_nodes_.insert(node.get());
}

void DirtyNodeTracker::RecordNodeRemoved(std::shared_ptr<Node> node, 
                                         std::shared_ptr<Node> parent, 
                                         size_t index) {
    if (!node || !parent) return;
    
    StructuralChange change;
    change.type = StructuralChangeType::Removed;
    change.node = node;
    change.parent = parent;
    change.index = index;
    
    structural_changes_.push_back(change);
    removed_nodes_.insert(node.get());
}

void DirtyNodeTracker::RecordNodeReplaced(std::shared_ptr<Node> old_node, 
                                          std::shared_ptr<Node> new_node, 
                                          std::shared_ptr<Node> parent, 
                                          size_t index) {
    if (!old_node || !new_node || !parent) return;
    
    StructuralChange change;
    change.type = StructuralChangeType::Replaced;
    change.old_node = old_node;
    change.new_node = new_node;
    change.parent = parent;
    change.index = index;
    
    structural_changes_.push_back(change);
    removed_nodes_.insert(old_node.get());
    added_nodes_.insert(new_node.get());
}

void DirtyNodeTracker::RecordNodeMoved(std::shared_ptr<Node> node,
                                       std::shared_ptr<Node> old_parent,
                                       std::shared_ptr<Node> new_parent,
                                       size_t new_index) {
    if (!node || !old_parent || !new_parent) return;
    
    StructuralChange change;
    change.type = StructuralChangeType::Moved;
    change.node = node;
    change.old_parent = old_parent;
    change.parent = new_parent;
    change.index = new_index;
    
    structural_changes_.push_back(change);
}

void DirtyNodeTracker::RecordStyleChanged(std::shared_ptr<Element> element,
                                          const std::string& property,
                                          const std::string& old_value,
                                          const std::string& new_value) {
    if (!element) return;
    
    StyleChange change;
    change.element = element;
    change.property = property;
    change.old_value = old_value;
    change.new_value = new_value;
    
    style_changes_.push_back(change);
}

void DirtyNodeTracker::RecordTextChanged(std::shared_ptr<Node> node,
                                         const std::string& old_text,
                                         const std::string& new_text) {
    if (!node) return;
    
    TextChange change;
    change.node = node;
    change.old_text = old_text;
    change.new_text = new_text;
    
    text_changes_.push_back(change);
}

bool DirtyNodeTracker::HasPendingChanges() const {
    return !structural_changes_.empty() || 
           !style_changes_.empty() || 
           !text_changes_.empty();
}

void DirtyNodeTracker::Clear() {
    structural_changes_.clear();
    style_changes_.clear();
    text_changes_.clear();
    added_nodes_.clear();
    removed_nodes_.clear();
}

void DirtyNodeTracker::Optimize() {
    // 优化结构变化：合并 add-then-remove 同一节点的操作
    std::vector<StructuralChange> optimized_structural;
    std::unordered_set<Node*> cancelled_nodes;
    
    // 第一遍：找出被添加后又被移除的节点
    for (const auto& change : structural_changes_) {
        if (change.type == StructuralChangeType::Added) {
            auto node = change.node.lock();
            if (node && removed_nodes_.count(node.get())) {
                // 这个节点被添加后又被移除，标记为取消
                cancelled_nodes.insert(node.get());
            }
        }
    }
    
    // 第二遍：过滤掉被取消的操作
    for (const auto& change : structural_changes_) {
        Node* node_ptr = nullptr;
        
        if (change.type == StructuralChangeType::Added || 
            change.type == StructuralChangeType::Removed ||
            change.type == StructuralChangeType::Moved) {
            auto node = change.node.lock();
            if (node) {
                node_ptr = node.get();
            }
        } else if (change.type == StructuralChangeType::Replaced) {
            // Replaced 操作不参与取消优化
            optimized_structural.push_back(change);
            continue;
        }
        
        // 如果节点不在取消列表中，保留这个变化
        if (node_ptr && cancelled_nodes.count(node_ptr) == 0) {
            optimized_structural.push_back(change);
        }
    }
    
    structural_changes_ = std::move(optimized_structural);
    
    // 优化样式变化：合并同一元素同一属性的多次变化
    std::unordered_map<Element*, std::unordered_map<std::string, size_t>> style_change_indices;
    std::vector<StyleChange> optimized_styles;
    
    for (size_t i = 0; i < style_changes_.size(); ++i) {
        auto& change = style_changes_[i];
        auto element = change.element.lock();
        if (!element) continue;
        
        auto& prop_indices = style_change_indices[element.get()];
        auto it = prop_indices.find(change.property);
        
        if (it != prop_indices.end()) {
            // 已有同一元素同一属性的变化，更新最终值
            optimized_styles[it->second].new_value = change.new_value;
        } else {
            // 新的变化
            prop_indices[change.property] = optimized_styles.size();
            optimized_styles.push_back(change);
        }
    }
    
    style_changes_ = std::move(optimized_styles);
    
    // 优化文本变化：合并同一节点的多次变化
    std::unordered_map<Node*, size_t> text_change_indices;
    std::vector<TextChange> optimized_texts;
    
    for (size_t i = 0; i < text_changes_.size(); ++i) {
        auto& change = text_changes_[i];
        auto node = change.node.lock();
        if (!node) continue;
        
        auto it = text_change_indices.find(node.get());
        
        if (it != text_change_indices.end()) {
            // 已有同一节点的变化，更新最终值
            optimized_texts[it->second].new_text = change.new_text;
        } else {
            // 新的变化
            text_change_indices[node.get()] = optimized_texts.size();
            optimized_texts.push_back(change);
        }
    }
    
    text_changes_ = std::move(optimized_texts);
    
    // 重建索引集合
    added_nodes_.clear();
    removed_nodes_.clear();
    
    for (const auto& change : structural_changes_) {
        if (change.type == StructuralChangeType::Added) {
            if (auto node = change.node.lock()) {
                added_nodes_.insert(node.get());
            }
        } else if (change.type == StructuralChangeType::Removed) {
            if (auto node = change.node.lock()) {
                removed_nodes_.insert(node.get());
            }
        } else if (change.type == StructuralChangeType::Replaced) {
            if (auto old_node = change.old_node.lock()) {
                removed_nodes_.insert(old_node.get());
            }
            if (auto new_node = change.new_node.lock()) {
                added_nodes_.insert(new_node.get());
            }
        }
    }
}

} // namespace lightui
