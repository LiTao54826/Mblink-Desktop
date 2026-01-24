/**
 * @file incremental_style_recalc.cpp
 * @brief 增量样式重算实现
 */

#include "incremental_style_recalc.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/node.h"
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_object.h"

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
            // 关键修复：必须设置 StyleManager，否则 CSS 规则不会被应用
            StyleResolver resolver;
            if (current_document_ && current_document_->GetStyleManager()) {
                resolver.SetStyleManager(current_document_->GetStyleManager());
            }
            ComputedStyle new_style = resolver.ResolveStyle(element, parent_style);

            // 保存旧样式用于比较
            const ComputedStyle& old_style = render_obj->GetComputedStyle();

            // 检查是否有布局相关属性变化
            // 只有布局属性变化时才标记需要布局，纯视觉属性变化只需要重绘
            bool layout_changed =
                old_style.display != new_style.display ||
                old_style.position != new_style.position ||
                old_style.width != new_style.width ||
                old_style.height != new_style.height ||
                old_style.min_width != new_style.min_width ||
                old_style.min_height != new_style.min_height ||
                old_style.max_width != new_style.max_width ||
                old_style.max_height != new_style.max_height ||
                old_style.padding_top != new_style.padding_top ||
                old_style.padding_right != new_style.padding_right ||
                old_style.padding_bottom != new_style.padding_bottom ||
                old_style.padding_left != new_style.padding_left ||
                old_style.margin_top != new_style.margin_top ||
                old_style.margin_right != new_style.margin_right ||
                old_style.margin_bottom != new_style.margin_bottom ||
                old_style.margin_left != new_style.margin_left ||
                old_style.border_top_width != new_style.border_top_width ||
                old_style.border_right_width != new_style.border_right_width ||
                old_style.border_bottom_width != new_style.border_bottom_width ||
                old_style.border_left_width != new_style.border_left_width ||
                old_style.flex_direction != new_style.flex_direction ||
                old_style.flex_wrap != new_style.flex_wrap ||
                old_style.flex_grow != new_style.flex_grow ||
                old_style.flex_shrink != new_style.flex_shrink ||
                old_style.flex_basis != new_style.flex_basis ||
                old_style.justify_content != new_style.justify_content ||
                old_style.align_items != new_style.align_items ||
                old_style.align_content != new_style.align_content ||
                old_style.gap != new_style.gap ||
                old_style.overflow != new_style.overflow;

            // 更新 RenderObject 的样式
            render_obj->SetComputedStyle(new_style);

            // 更新布局样式
            render_obj->UpdateLayoutStyle();

            // 只有布局属性变化时才标记需要布局
            if (layout_changed) {
                render_obj->MarkNeedsLayout();
            }

            // 总是标记需要重绘（样式变化至少需要重绘）
            render_obj->MarkNeedsPaint();

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
