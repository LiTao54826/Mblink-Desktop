/**
 * @file render_tree_synchronizer.cpp
 * @brief 渲染树同步器实现
 */

#include "render_tree_synchronizer.h"
#include "render_object.h"
#include "style_resolver.h"  // RenderTreeBuilder 在这里定义
#include "../dom/dirty_node_tracker.h"
#include "../dom/node.h"
#include "../dom/element.h"
#include "../dom/text.h"
#include "../dom/document.h"
#include "../layout/layout_engine.h"
#include <algorithm>

namespace lightui {

RenderTreeSynchronizer::RenderTreeSynchronizer() = default;
RenderTreeSynchronizer::~RenderTreeSynchronizer() = default;

void RenderTreeSynchronizer::SetDocument(std::shared_ptr<Document> doc) {
    document_ = doc;
}

void RenderTreeSynchronizer::SetLayoutEngine(std::shared_ptr<LayoutEngine> engine) {
    layout_engine_ = engine;
}

void RenderTreeSynchronizer::SetRenderTreeBuilder(std::shared_ptr<RenderTreeBuilder> builder) {
    render_tree_builder_ = builder;
}

bool RenderTreeSynchronizer::Synchronize(DirtyNodeTracker& tracker, 
                                          std::shared_ptr<RenderObject> render_tree) {
    if (!tracker.HasPendingChanges()) {
        return false;
    }
    
    render_tree_ = render_tree;
    
    // 优化变化列表（合并冗余操作）
    tracker.Optimize();
    
    // 判断是否需要子树重建
    if (NeedsSubtreeRebuild(tracker)) {
        // 收集受影响的根节点
        std::unordered_set<Node*> affected_roots;
        
        for (const auto& change : tracker.GetStructuralChanges()) {
            if (auto parent = change.parent.lock()) {
                affected_roots.insert(parent.get());
            }
        }
        
        // 重建受影响的子树
        for (Node* root : affected_roots) {
            RebuildSubtree(root);
        }
    } else {
        // 增量更新
        ProcessStructuralChanges(tracker);
    }
    
    // 处理样式和文本变化
    ProcessStyleChanges(tracker);
    ProcessTextChanges(tracker);
    
    // 清除追踪器
    tracker.Clear();
    
    render_tree_ = nullptr;
    
    return true;
}

bool RenderTreeSynchronizer::NeedsSubtreeRebuild(const DirtyNodeTracker& tracker) const {
    const auto& changes = tracker.GetStructuralChanges();
    
    // 规则 1：计算变化区域的总面积
    // 如果变化区域超过视口面积的 50%，使用全量重建更高效
    float total_change_area = 0.0f;
    float viewport_width = 800.0f;  // 默认值
    float viewport_height = 600.0f;
    
    // 尝试从第一个有 RenderObject 的节点获取视口大小
    for (const auto& change : changes) {
        if (auto node = change.node.lock()) {
            if (auto render_obj = node->GetRenderObject()) {
                viewport_width = render_obj->GetViewportWidth();
                viewport_height = render_obj->GetViewportHeight();
                if (viewport_width > 0 && viewport_height > 0) break;
            }
        }
    }
    
    float viewport_area = viewport_width * viewport_height;
    
    for (const auto& change : changes) {
        // 尝试获取变化节点的渲染对象来计算面积
        if (auto node = change.node.lock()) {
            if (auto render_obj = node->GetRenderObject()) {
                const auto& layout = render_obj->GetLayoutInfo();
                total_change_area += layout.width * layout.height;
            } else {
                // 没有渲染对象，估算一个默认大小
                total_change_area += 100.0f * 50.0f;
            }
        }
        // 对于被移除的旧节点
        if (auto old_node = change.old_node.lock()) {
            if (auto render_obj = old_node->GetRenderObject()) {
                const auto& layout = render_obj->GetLayoutInfo();
                total_change_area += layout.width * layout.height;
            }
        }
    }
    
    // 如果变化区域超过视口的 50%，使用全量重建
    if (viewport_area > 0 && total_change_area > viewport_area * 0.5f) {
        return true;
    }
    
    // 规则 2：有 Replaced 操作且涉及复杂子树
    for (const auto& change : changes) {
        if (change.type == DirtyNodeTracker::StructuralChangeType::Replaced) {
            if (auto old_node = change.old_node.lock()) {
                if (old_node->GetChildNodes().size() > replaced_children_threshold_) {
                    return true;
                }
            }
        }
    }
    
    // 规则 3：同一父节点下有多个变化
    std::unordered_map<Node*, size_t> parent_change_count;
    for (const auto& change : changes) {
        if (auto parent = change.parent.lock()) {
            if (++parent_change_count[parent.get()] > parent_changes_threshold_) {
                return true;
            }
        }
    }
    
    return false;
}

void RenderTreeSynchronizer::ProcessStructuralChanges(DirtyNodeTracker& tracker) {
    for (const auto& change : tracker.GetStructuralChanges()) {
        switch (change.type) {
            case DirtyNodeTracker::StructuralChangeType::Added: {
                auto node = change.node.lock();
                auto parent = change.parent.lock();
                if (node && parent) {
                    InsertRenderObject(node.get(), parent.get(), change.index);
                }
                break;
            }
            
            case DirtyNodeTracker::StructuralChangeType::Removed: {
                auto node = change.node.lock();
                if (node) {
                    RemoveRenderObject(node.get());
                }
                break;
            }
            
            case DirtyNodeTracker::StructuralChangeType::Replaced: {
                auto old_node = change.old_node.lock();
                auto new_node = change.new_node.lock();
                auto parent = change.parent.lock();
                if (old_node && new_node && parent) {
                    ReplaceRenderObject(old_node.get(), new_node.get(), parent.get(), change.index);
                }
                break;
            }
            
            case DirtyNodeTracker::StructuralChangeType::Moved: {
                auto node = change.node.lock();
                auto new_parent = change.parent.lock();
                if (node && new_parent) {
                    // 移动 = 移除 + 插入
                    RemoveRenderObject(node.get());
                    InsertRenderObject(node.get(), new_parent.get(), change.index);
                }
                break;
            }
        }
    }
}

void RenderTreeSynchronizer::ProcessStyleChanges(DirtyNodeTracker& tracker) {
    auto doc = document_.lock();
    if (!doc) return;
    
    for (const auto& change : tracker.GetStyleChanges()) {
        auto element = change.element.lock();
        if (!element) continue;
        
        auto render_obj = element->GetRenderObject();
        if (!render_obj) continue;
        
        // 重新解析样式
        // TODO: 使用 StyleResolver 重新计算样式
        
        // 标记需要重新布局和绘制，并向上传播到祖先
        InvalidateAncestorLayout(render_obj.get());
        render_obj->MarkNeedsPaint();
        render_obj->InvalidatePaintCache();
    }
}

void RenderTreeSynchronizer::ProcessTextChanges(DirtyNodeTracker& tracker) {
    for (const auto& change : tracker.GetTextChanges()) {
        auto node = change.node.lock();
        if (!node) continue;
        
        // 使用增量更新系统的脏标记
        node->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
        node->SetNeedsLayout();
        
        auto render_obj = node->GetRenderObject();
        if (!render_obj) continue;
        
        // 更新文本内容
        if (render_obj->GetType() == RenderObjectType::TEXT) {
            auto render_text = std::dynamic_pointer_cast<RenderText>(render_obj);
            if (render_text) {
                render_text->SetText(change.new_text);
            }
        }
        
        // 标记需要重新布局和绘制，并向上传播到祖先
        // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
        InvalidateAncestorLayout(render_obj.get());
        render_obj->MarkNeedsPaint();
    }
}

void RenderTreeSynchronizer::RebuildSubtree(Node* root) {
    if (!root) return;
    
    auto render_obj = root->GetRenderObject();
    if (!render_obj) return;
    
    // 移除所有子渲染对象
    render_obj->RemoveAllChildren();
    
    // 重新创建子树
    for (const auto& child : root->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }
    
    // 标记需要重新布局，并向上传播到所有祖先
    // 这确保滚动容器等祖先节点的 content_height_ 缓存被清除
    InvalidateAncestorLayout(render_obj.get());
}

void RenderTreeSynchronizer::UpdateNode(Node* node) {
    if (!node) return;
    
    auto render_obj = node->GetRenderObject();
    if (render_obj) {
        render_obj->MarkNeedsLayout();
        render_obj->MarkNeedsPaint();
    }
}

std::shared_ptr<RenderObject> RenderTreeSynchronizer::InsertRenderObject(
    Node* node, Node* parent, size_t index) {
    
    if (!node || !parent) return nullptr;
    
    auto parent_ro = parent->GetRenderObject();
    if (!parent_ro) return nullptr;
    
    // 创建渲染对象
    auto render_obj = CreateRenderObjectForNode(node);
    if (!render_obj) return nullptr;
    
    // 查找插入位置
    size_t insert_pos = FindInsertPosition(parent_ro.get(), index, parent);
    
    // 插入到父渲染对象
    auto& children = parent_ro->GetChildrenMutable();
    if (insert_pos >= children.size()) {
        parent_ro->AppendChild(render_obj);
    } else {
        children.insert(children.begin() + insert_pos, render_obj);
        render_obj->SetParent(parent_ro);
    }
    
    // 递归创建子树
    for (const auto& child : node->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }
    
    // 使祖先布局失效
    InvalidateAncestorLayout(render_obj.get());
    
    return render_obj;
}

void RenderTreeSynchronizer::RemoveRenderObject(Node* node) {
    if (!node) return;
    
    auto render_obj = node->GetRenderObject();
    if (!render_obj) return;
    
    auto parent_ro = render_obj->GetParent();
    if (parent_ro) {
        // 使祖先布局失效（在移除前）
        InvalidateAncestorLayout(render_obj.get());
        
        // 从父节点移除
        parent_ro->RemoveChild(render_obj);
    }
    
    // 清除 DOM 节点与渲染对象的关联
    node->SetRenderObject(nullptr);
}

void RenderTreeSynchronizer::ReplaceRenderObject(
    Node* old_node, Node* new_node, Node* parent, size_t index) {
    
    if (!old_node || !new_node || !parent) return;
    
    auto parent_ro = parent->GetRenderObject();
    if (!parent_ro) return;
    
    auto old_ro = old_node->GetRenderObject();
    
    // 创建新的渲染对象
    auto new_ro = CreateRenderObjectForNode(new_node);
    
    if (old_ro) {
        // 找到旧渲染对象在父节点中的位置
        auto& children = parent_ro->GetChildrenMutable();
        auto it = std::find(children.begin(), children.end(), old_ro);
        
        if (it != children.end()) {
            if (new_ro) {
                // 原子替换：直接替换，不经过中间状态
                *it = new_ro;
                new_ro->SetParent(parent_ro);
            } else {
                // 新节点是 display: none，直接移除旧节点
                children.erase(it);
            }
        }
        
        // 清除旧节点的关联
        old_node->SetRenderObject(nullptr);
    } else if (new_ro) {
        // 旧节点没有渲染对象（可能是 display: none），直接插入新节点
        size_t insert_pos = FindInsertPosition(parent_ro.get(), index, parent);
        auto& children = parent_ro->GetChildrenMutable();
        if (insert_pos >= children.size()) {
            parent_ro->AppendChild(new_ro);
        } else {
            children.insert(children.begin() + insert_pos, new_ro);
            new_ro->SetParent(parent_ro);
        }
    }
    
    // 递归创建新节点的子树
    if (new_ro) {
        for (const auto& child : new_node->GetChildNodes()) {
            CreateRenderSubtree(child.get(), new_ro.get());
        }
    }
    
    // 使祖先布局失效
    if (new_ro) {
        InvalidateAncestorLayout(new_ro.get());
    } else {
        InvalidateAncestorLayout(parent_ro.get());
    }
}

std::shared_ptr<RenderObject> RenderTreeSynchronizer::CreateRenderObjectForNode(Node* node) {
    if (!node) return nullptr;
    
    auto builder = render_tree_builder_.lock();
    if (!builder) return nullptr;
    
    std::shared_ptr<RenderObject> render_obj;
    
    // 根据节点类型使用不同的创建方法
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::dynamic_pointer_cast<Element>(node->shared_from_this());
        if (element) {
            render_obj = builder->CreateRenderObjectForElement(element.get());
        }
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<Text>(node->shared_from_this());
        if (text) {
            render_obj = builder->CreateRenderObjectForText(text.get());
        }
    }
    
    if (render_obj) {
        // 建立双向关联
        node->SetRenderObject(render_obj);
        render_obj->SetNode(node->shared_from_this());
    }
    
    return render_obj;
}

void RenderTreeSynchronizer::CreateRenderSubtree(Node* node, RenderObject* parent_ro) {
    if (!node || !parent_ro) return;
    
    auto render_obj = CreateRenderObjectForNode(node);
    if (!render_obj) return;
    
    parent_ro->AppendChild(render_obj);
    
    // 递归处理子节点
    for (const auto& child : node->GetChildNodes()) {
        CreateRenderSubtree(child.get(), render_obj.get());
    }
}

size_t RenderTreeSynchronizer::FindInsertPosition(
    RenderObject* parent_ro, size_t dom_index, Node* parent_node) {
    
    if (!parent_ro || !parent_node) return 0;
    
    const auto& dom_children = parent_node->GetChildNodes();
    const auto& ro_children = parent_ro->GetChildren();
    
    // 遍历 DOM 子节点，找到在 dom_index 之前有多少个有渲染对象的节点
    size_t ro_index = 0;
    for (size_t i = 0; i < dom_index && i < dom_children.size(); ++i) {
        if (dom_children[i]->GetRenderObject()) {
            ++ro_index;
        }
    }
    
    return std::min(ro_index, ro_children.size());
}

void RenderTreeSynchronizer::InvalidateAncestorLayout(RenderObject* obj) {
    while (obj) {
        obj->MarkNeedsLayout();
        obj = obj->GetParent().get();
    }
}

} // namespace lightui
