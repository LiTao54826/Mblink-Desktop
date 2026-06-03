/**
 * @file dirty_node_tracker.cpp
 * @brief 脏节点追踪器实现
 */

#include "dirty_node_tracker.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include <algorithm>

namespace mbink {

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
    // 优化结构变化：合并冗余操作
    std::vector<StructuralChange> optimized_structural;
    std::unordered_set<Node*> cancelled_nodes;
    std::unordered_set<Node*> moved_nodes;  // 记录被移动的节点

    // 分析每个节点的操作顺序
    // 记录每个节点的第一次操作类型和对应的变化
    std::unordered_map<Node*, StructuralChangeType> first_operation;
    std::unordered_map<Node*, StructuralChange> remove_changes;  // 记录 Remove 操作
    std::unordered_map<Node*, StructuralChange> add_changes;     // 记录 Add 操作

    for (const auto& change : structural_changes_) {
        Node* node_ptr = nullptr;
        if (change.type == StructuralChangeType::Added ||
            change.type == StructuralChangeType::Removed) {
            auto node = change.node;
            if (node) {
                node_ptr = node.get();
            }
        }

        if (node_ptr) {
            if (first_operation.find(node_ptr) == first_operation.end()) {
                first_operation[node_ptr] = change.type;
            }

            // 记录操作
            if (change.type == StructuralChangeType::Removed) {
                remove_changes[node_ptr] = change;
            } else if (change.type == StructuralChangeType::Added) {
                add_changes[node_ptr] = change;
            }
        }
    }

    // 判断哪些节点应该被取消或转换为移动操作
    for (const auto& change : structural_changes_) {
        if (change.type == StructuralChangeType::Added) {
            auto node = change.node;
            if (node && removed_nodes_.count(node.get())) {
                auto it = first_operation.find(node.get());
                if (it != first_operation.end()) {
                    if (it->second == StructuralChangeType::Added) {
                        // 先 Add 后 Remove，标记为取消
                        cancelled_nodes.insert(node.get());
                    } else if (it->second == StructuralChangeType::Removed) {
                        // 先 Remove 后 Add，这是移动操作
                        moved_nodes.insert(node.get());
                    }
                }
            }
        }
    }

    // 第二遍：过滤掉被取消的操作，合并移动操作
    std::vector<StructuralChange> added_changes;
    std::vector<StructuralChange> other_changes;

    for (const auto& change : structural_changes_) {
        Node* node_ptr = nullptr;

        if (change.type == StructuralChangeType::Added ||
            change.type == StructuralChangeType::Removed ||
            change.type == StructuralChangeType::Moved) {
            auto node = change.node;
            if (node) {
                node_ptr = node.get();
            }
        } else if (change.type == StructuralChangeType::Replaced) {
            // Replaced 操作不参与取消优化
            other_changes.push_back(change);
            continue;
        }

        // 跳过被取消的节点
        if (node_ptr && cancelled_nodes.count(node_ptr) > 0) {
            continue;
        }

        // 处理移动操作
        if (node_ptr && moved_nodes.count(node_ptr) > 0) {
            if (change.type == StructuralChangeType::Removed) {
                // 将 Remove + Add 合并为 Moved
                auto add_it = add_changes.find(node_ptr);
                if (add_it != add_changes.end()) {
                    StructuralChange moved_change;
                    moved_change.type = StructuralChangeType::Moved;
                    moved_change.node = change.node;
                    moved_change.old_parent = change.parent;  // Remove 的父节点
                    moved_change.parent = add_it->second.parent;  // Add 的父节点
                    moved_change.index = add_it->second.index;
                    other_changes.push_back(moved_change);
                }
            }
            // 跳过 Add 操作（已经合并到 Moved 中）
            continue;
        }

        // 正常处理
        if (node_ptr) {
            if (change.type == StructuralChangeType::Added) {
                added_changes.push_back(change);
            } else {
                other_changes.push_back(change);
            }
        }
    }

    // 对 Added 操作进行拓扑排序：父节点应该在子节点之前
    // 计算每个节点的深度
    auto get_depth = [](Node* node) -> int {
        int depth = 0;
        Node* current = node;
        while (current) {
            auto parent = current->GetParentNode();
            if (!parent) break;
            current = parent.get();
            depth++;
        }
        return depth;
    };

    // 按深度排序 Added 操作（深度小的先处理）
    std::sort(added_changes.begin(), added_changes.end(),
        [&get_depth](const StructuralChange& a, const StructuralChange& b) {
            auto node_a = a.node;
            auto node_b = b.node;
            if (!node_a || !node_b) return false;
            return get_depth(node_a.get()) < get_depth(node_b.get());
        });

    // 合并：先处理 Replaced，再处理 Added（按深度排序），最后处理其他操作
    // Replaced 操作应该先处理，因为它会创建新的渲染对象
    // 这样后续的 Added 操作才能正确计算插入位置
    std::vector<StructuralChange> replaced_changes;
    std::vector<StructuralChange> remaining_changes;
    
    for (auto& change : other_changes) {
        if (change.type == StructuralChangeType::Replaced) {
            replaced_changes.push_back(std::move(change));
        } else {
            remaining_changes.push_back(std::move(change));
        }
    }
    
    optimized_structural.reserve(replaced_changes.size() + added_changes.size() + remaining_changes.size());
    for (auto& change : replaced_changes) {
        optimized_structural.push_back(std::move(change));
    }
    for (auto& change : added_changes) {
        optimized_structural.push_back(std::move(change));
    }
    for (auto& change : remaining_changes) {
        optimized_structural.push_back(std::move(change));
    }

    structural_changes_ = std::move(optimized_structural);

    std::unordered_set<Node*> added_subtree_roots;
    std::unordered_set<Node*> removed_subtree_roots;
    for (const auto& change : structural_changes_) {
        switch (change.type) {
            case StructuralChangeType::Added:
                if (change.node) {
                    added_subtree_roots.insert(change.node.get());
                }
                break;
            case StructuralChangeType::Removed:
                if (change.node) {
                    removed_subtree_roots.insert(change.node.get());
                }
                break;
            case StructuralChangeType::Replaced:
                if (change.old_node) {
                    removed_subtree_roots.insert(change.old_node.get());
                }
                if (change.new_node) {
                    added_subtree_roots.insert(change.new_node.get());
                }
                break;
            case StructuralChangeType::Moved:
                break;
        }
    }

    auto is_covered_by_root = [](Node* node,
                                 const std::unordered_set<Node*>& roots,
                                 bool include_self) {
        if (!node || roots.empty()) {
            return false;
        }

        Node* current = include_self ? node : node->GetParentNode().get();
        while (current) {
            if (roots.find(current) != roots.end()) {
                return true;
            }
            auto parent = current->GetParentNode();
            current = parent.get();
        }
        return false;
    };

    if (!added_subtree_roots.empty() || !removed_subtree_roots.empty()) {
        std::vector<StructuralChange> subtree_pruned_structural;
        subtree_pruned_structural.reserve(structural_changes_.size());

        for (auto& change : structural_changes_) {
            bool covered = false;
            switch (change.type) {
                case StructuralChangeType::Added:
                    covered = change.node &&
                        is_covered_by_root(change.node.get(), added_subtree_roots, false);
                    break;
                case StructuralChangeType::Removed:
                    covered = change.node &&
                        is_covered_by_root(change.node.get(), removed_subtree_roots, false);
                    break;
                case StructuralChangeType::Moved:
                    covered = change.node &&
                        (is_covered_by_root(change.node.get(), added_subtree_roots, false) ||
                         is_covered_by_root(change.node.get(), removed_subtree_roots, false));
                    break;
                case StructuralChangeType::Replaced:
                    covered =
                        (change.old_node &&
                         is_covered_by_root(change.old_node.get(), removed_subtree_roots, false)) ||
                        (change.new_node &&
                         is_covered_by_root(change.new_node.get(), added_subtree_roots, false));
                    break;
            }

            if (!covered) {
                subtree_pruned_structural.push_back(std::move(change));
            }
        }

        structural_changes_ = std::move(subtree_pruned_structural);
    }

    // 优化样式变化：合并同一元素同一属性的多次变化
    std::unordered_map<Element*, std::unordered_map<std::string, size_t>> style_change_indices;
    std::vector<StyleChange> optimized_styles;

    for (size_t i = 0; i < style_changes_.size(); ++i) {
        auto& change = style_changes_[i];
        auto element = change.element.lock();
        if (!element) continue;
        if (is_covered_by_root(element.get(), added_subtree_roots, true) ||
            is_covered_by_root(element.get(), removed_subtree_roots, true)) {
            continue;
        }

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
        if (is_covered_by_root(node.get(), added_subtree_roots, true) ||
            is_covered_by_root(node.get(), removed_subtree_roots, true)) {
            continue;
        }

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
            if (auto node = change.node) {
                added_nodes_.insert(node.get());
            }
        } else if (change.type == StructuralChangeType::Removed) {
            if (auto node = change.node) {
                removed_nodes_.insert(node.get());
            }
        } else if (change.type == StructuralChangeType::Replaced) {
            if (auto old_node = change.old_node) {
                removed_nodes_.insert(old_node.get());
            }
            if (auto new_node = change.new_node) {
                added_nodes_.insert(new_node.get());
            }
        }
    }
}

} // namespace mbink
