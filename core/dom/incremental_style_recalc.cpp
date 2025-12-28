/**
 * @file incremental_style_recalc.cpp
 * @brief 增量样式重算实现
 */

#include "incremental_style_recalc.h"
#include "document.h"
#include "element.h"
#include "text.h"
#include "node.h"
#include "core/render/css/style_resolver.h"
#include "core/render/render_object.h"

namespace lightui {

IncrementalStyleRecalc::IncrementalStyleRecalc() = default;

void IncrementalStyleRecalc::RecalcStyle(Document* document) {
    if (!document) {
        return;
    }
    
    // 重置统计信息
    stats_.Reset();
    current_document_ = document;
    
    // 获取文档元素（html）
    auto doc_element = document->GetDocumentElement();
    if (!doc_element) {
        return;
    }
    
    // 检查是否需要样式重算
    // 如果文档元素本身不脏且没有脏子节点，直接返回
    if (!doc_element->IsDirtyForStyleRecalc()) {
        return;
    }
    
    // 从文档元素开始递归处理
    RecalcStyleForNode(doc_element, nullptr, false);
    
    current_document_ = nullptr;
}

void IncrementalStyleRecalc::RecalcStyleForNode(std::shared_ptr<Node> node,
                                                 const ComputedStyle* parent_style,
                                                 bool force_recalc) {
    if (!node) {
        return;
    }
    
    stats_.nodes_visited++;
    
    // 如果不是强制重算，且节点干净，跳过整个子树
    if (!force_recalc && !node->IsDirtyForStyleRecalc()) {
        stats_.subtrees_skipped++;
        return;
    }
    
    // 根据节点类型处理
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        RecalcStyleForElement(element, parent_style, force_recalc);
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        // 文本节点不需要样式重算，但需要清除标志
        node->ClearNeedsStyleRecalc();
    } else {
        // 其他节点类型（如 Document），处理子节点
        RecalcStyleForChildren(node, parent_style, force_recalc);
        node->ClearNeedsStyleRecalc();
    }
}

void IncrementalStyleRecalc::RecalcStyleForElement(std::shared_ptr<Element> element,
                                                    const ComputedStyle* parent_style,
                                                    bool force_recalc) {
    if (!element) {
        return;
    }
    
    // 获取当前的样式变化类型
    StyleChangeType change_type = element->GetStyleChangeType();
    
    // 判断是否需要重算此节点的样式
    bool needs_recalc = force_recalc || 
                        change_type != StyleChangeType::kNoStyleChange;
    
    // 判断是否需要强制重算子树
    bool force_children = force_recalc || 
                          change_type == StyleChangeType::kSubtreeStyleChange;
    
    const ComputedStyle* style_for_children = parent_style;
    
    if (needs_recalc) {
        stats_.nodes_recalculated++;
        
        // 获取关联的 RenderObject
        auto render_obj = element->GetRenderObject();
        if (render_obj) {
            // 使用 StyleResolver 重新计算样式
            // 注意：这里需要访问 Document 的 RenderTreeBuilder 来获取 StyleResolver
            // 为了简化，我们直接创建一个临时的 StyleResolver
            StyleResolver resolver;
            ComputedStyle new_style = resolver.ResolveStyle(element, parent_style);
            
            // 更新 RenderObject 的样式
            render_obj->SetComputedStyle(new_style);
            
            // 更新布局样式
            render_obj->UpdateLayoutStyle();
            
            // 标记需要布局（样式变化可能影响布局）
            render_obj->MarkNeedsLayout();
            
            // 使用新样式作为子节点的父样式
            style_for_children = &render_obj->GetComputedStyle();
        }
    } else if (element->GetRenderObject()) {
        // 节点本身不需要重算，但可能有脏子节点
        // 使用当前节点的样式作为子节点的父样式
        style_for_children = &element->GetRenderObject()->GetComputedStyle();
    }
    
    // 处理子节点
    if (element->ChildNeedsStyleRecalc() || force_children) {
        RecalcStyleForChildren(element, style_for_children, force_children);
    } else if (!element->GetChildNodes().empty()) {
        // 有子节点但不需要处理，统计跳过的子树
        stats_.subtrees_skipped++;
    }
    
    // 清除样式重算标志
    element->ClearNeedsStyleRecalc();
}

void IncrementalStyleRecalc::RecalcStyleForChildren(std::shared_ptr<Node> node,
                                                     const ComputedStyle* parent_style,
                                                     bool force_recalc) {
    if (!node) {
        return;
    }
    
    for (const auto& child : node->GetChildNodes()) {
        RecalcStyleForNode(child, parent_style, force_recalc);
    }
    
    // 清除 ChildNeedsStyleRecalc 标志
    node->ClearChildNeedsStyleRecalc();
}

} // namespace lightui
