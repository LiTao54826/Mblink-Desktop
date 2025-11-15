/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation using Taffy
 */

#include "layout_engine.h"
#include "dom/element.h"
#include "render/render_object.h"
#include <cmath>

// Taffy C API
extern "C" {
#include "taffy.h"
}

namespace lightui {

LayoutEngine::LayoutEngine()
    : taffy_tree_(nullptr)
    , has_root_(false) {
    // Initialize Taffy tree
    taffy_tree_ = TaffyTree_New();
}

LayoutEngine::~LayoutEngine() {
    Clear();
    // Free Taffy tree
    if (taffy_tree_) {
        TaffyTree_Free(taffy_tree_);
        taffy_tree_ = nullptr;
    }
}

void LayoutEngine::BuildLayoutTree(Element* root) {
    if (!root || !taffy_tree_) {
        return;
    }

    Clear();

    // Build tree from root
    TaffyNodeId invalid_parent;
    invalid_parent._0 = 0;
    BuildSubtree(root, invalid_parent);
}

void LayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (!has_root_ || !taffy_tree_) {
        return;
    }

    // Call Taffy layout computation
    TaffyTree_ComputeLayout(taffy_tree_, root_node_, available_width, available_height);

    // Read layout results for all elements
    for (auto& [element, node] : element_to_node_) {
        ReadLayoutResults(element);
    }
}

LayoutInfo LayoutEngine::GetLayoutInfo(Element* element) const {
    LayoutInfo info;

    if (!taffy_tree_) {
        return info;
    }

    auto it = element_to_node_.find(element);
    if (it != element_to_node_.end()) {
        TaffyResult_TaffyLayout result = TaffyTree_GetLayout(taffy_tree_, it->second);
        if (result.return_code == TAFFY_RETURN_CODE_OK) {
            info.x = result.value.x;
            info.y = result.value.y;
            info.width = result.value.width;
            info.height = result.value.height;
            info.is_laid_out = true;
        }
    }

    return info;
}

void LayoutEngine::UpdateStyle(Element* element, const ComputedStyle& style) {
    auto it = element_to_node_.find(element);
    if (it != element_to_node_.end()) {
        ApplyStyle(it->second, style);
    }
}

void LayoutEngine::AddElement(Element* element, Element* parent) {
    if (!element || HasElement(element) || !taffy_tree_) {
        return;
    }

    TaffyNodeId parent_node;
    parent_node._0 = 0;

    if (parent) {
        auto it = element_to_node_.find(parent);
        if (it != element_to_node_.end()) {
            parent_node = it->second;
        }
    }

    TaffyNodeId node = CreateNode(element);
    element_to_node_[element] = node;
    node_to_element_[node._0] = element;

    // Add to parent in Taffy tree
    if (parent_node._0 != 0) {
        TaffyTree_AppendChild(taffy_tree_, parent_node, node);
    }

    if (!has_root_ && !parent) {
        root_node_ = node;
        has_root_ = true;
    }
}

void LayoutEngine::RemoveElement(Element* element) {
    auto it = element_to_node_.find(element);
    if (it == element_to_node_.end() || !taffy_tree_) {
        return;
    }

    TaffyNodeId node = it->second;

    // Remove from Taffy tree
    TaffyTree_RemoveNode(taffy_tree_, node);

    node_to_element_.erase(node._0);
    element_to_node_.erase(it);

    if (has_root_ && root_node_._0 == node._0) {
        has_root_ = false;
    }
}

void LayoutEngine::Clear() {
    element_to_node_.clear();
    node_to_element_.clear();
    has_root_ = false;

    // TODO: Clear Taffy tree
    // if (taffy_tree_) {
    //     TaffyTree_Clear(taffy_tree_);
    // }
}

bool LayoutEngine::HasElement(Element* element) const {
    return element_to_node_.find(element) != element_to_node_.end();
}

TaffyNodeId LayoutEngine::CreateNode(Element* element) {
    TaffyNodeId node;
    node._0 = 0;

    if (!taffy_tree_) {
        return node;
    }

    // Create Taffy node
    TaffyNodeIdResult result = TaffyTree_NewNode(taffy_tree_);
    if (result.return_code == TAFFY_RETURN_CODE_OK) {
        node = result.value;

        // Apply element's computed style if available
        if (element) {
            auto render_obj = element->GetRenderObject();
            if (render_obj) {
                ApplyStyle(node, render_obj->GetComputedStyle());
            }
        }
    }

    return node;
}

void LayoutEngine::ApplyStyle(TaffyNodeId node, const ComputedStyle& style) {
    if (!taffy_tree_) {
        return;
    }

    // Get mutable style reference from Taffy
    TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
    if (style_result.return_code != TAFFY_RETURN_CODE_OK) {
        return;
    }

    TaffyStyleMutRef taffy_style = style_result.value;

    // Map display type
    TaffyDisplay display = TAFFY_DISPLAY_BLOCK;
    if (style.display == RenderObjectType::FLEX) {
        display = TAFFY_DISPLAY_FLEX;
    } else if (style.display == RenderObjectType::NONE) {
        display = TAFFY_DISPLAY_NONE;
    } else if (style.display == RenderObjectType::BLOCK) {
        display = TAFFY_DISPLAY_BLOCK;
    }
    TaffyStyle_SetDisplay(taffy_style, display);

    // Apply sizing
    auto apply_dimension = [](TaffyStyleMutRef style_ref, const CSSLength& css_len,
                              auto setter_func) {
        if (css_len.unit == CSSUnit::PX) {
            setter_func(style_ref, css_len.value, TAFFY_UNIT_LENGTH);
        } else if (css_len.unit == CSSUnit::PERCENT) {
            setter_func(style_ref, css_len.value, TAFFY_UNIT_PERCENT);
        } else if (css_len.unit == CSSUnit::AUTO) {
            setter_func(style_ref, 0.0f, TAFFY_UNIT_AUTO);
        }
    };

    apply_dimension(taffy_style, style.width, TaffyStyle_SetWidth);
    apply_dimension(taffy_style, style.height, TaffyStyle_SetHeight);
    apply_dimension(taffy_style, style.min_width, TaffyStyle_SetMinWidth);
    apply_dimension(taffy_style, style.max_width, TaffyStyle_SetMaxWidth);
    apply_dimension(taffy_style, style.min_height, TaffyStyle_SetMinHeight);
    apply_dimension(taffy_style, style.max_height, TaffyStyle_SetMaxHeight);

    // Apply margin
    apply_dimension(taffy_style, style.margin.top, TaffyStyle_SetMarginTop);
    apply_dimension(taffy_style, style.margin.right, TaffyStyle_SetMarginRight);
    apply_dimension(taffy_style, style.margin.bottom, TaffyStyle_SetMarginBottom);
    apply_dimension(taffy_style, style.margin.left, TaffyStyle_SetMarginLeft);

    // Apply padding
    apply_dimension(taffy_style, style.padding.top, TaffyStyle_SetPaddingTop);
    apply_dimension(taffy_style, style.padding.right, TaffyStyle_SetPaddingRight);
    apply_dimension(taffy_style, style.padding.bottom, TaffyStyle_SetPaddingBottom);
    apply_dimension(taffy_style, style.padding.left, TaffyStyle_SetPaddingLeft);

    // Apply border (convert to length)
    TaffyStyle_SetBorderTop(taffy_style, style.border.top.width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderRight(taffy_style, style.border.right.width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderBottom(taffy_style, style.border.bottom.width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderLeft(taffy_style, style.border.left.width, TAFFY_UNIT_LENGTH);

    // Apply Flexbox properties
    if (style.display == RenderObjectType::FLEX) {
        // Flex direction
        TaffyFlexDirection flex_dir = TAFFY_FLEX_DIRECTION_ROW;
        if (style.flex_direction == "row") flex_dir = TAFFY_FLEX_DIRECTION_ROW;
        else if (style.flex_direction == "row-reverse") flex_dir = TAFFY_FLEX_DIRECTION_ROW_REVERSE;
        else if (style.flex_direction == "column") flex_dir = TAFFY_FLEX_DIRECTION_COLUMN;
        else if (style.flex_direction == "column-reverse") flex_dir = TAFFY_FLEX_DIRECTION_COLUMN_REVERSE;
        TaffyStyle_SetFlexDirection(taffy_style, flex_dir);

        // Flex wrap
        TaffyFlexWrap flex_wrap = TAFFY_FLEX_WRAP_NO_WRAP;
        if (style.flex_wrap == "nowrap") flex_wrap = TAFFY_FLEX_WRAP_NO_WRAP;
        else if (style.flex_wrap == "wrap") flex_wrap = TAFFY_FLEX_WRAP_WRAP;
        else if (style.flex_wrap == "wrap-reverse") flex_wrap = TAFFY_FLEX_WRAP_WRAP_REVERSE;
        TaffyStyle_SetFlexWrap(taffy_style, flex_wrap);

        // Justify content
        TaffyAlignContent justify = TAFFY_ALIGN_CONTENT_FLEX_START;
        if (style.justify_content == "flex-start") justify = TAFFY_ALIGN_CONTENT_FLEX_START;
        else if (style.justify_content == "flex-end") justify = TAFFY_ALIGN_CONTENT_FLEX_END;
        else if (style.justify_content == "center") justify = TAFFY_ALIGN_CONTENT_CENTER;
        else if (style.justify_content == "space-between") justify = TAFFY_ALIGN_CONTENT_SPACE_BETWEEN;
        else if (style.justify_content == "space-around") justify = TAFFY_ALIGN_CONTENT_SPACE_AROUND;
        else if (style.justify_content == "space-evenly") justify = TAFFY_ALIGN_CONTENT_SPACE_EVENLY;
        TaffyStyle_SetJustifyContent(taffy_style, justify);

        // Align items
        TaffyAlignItems align = TAFFY_ALIGN_ITEMS_STRETCH;
        if (style.align_items == "flex-start") align = TAFFY_ALIGN_ITEMS_FLEX_START;
        else if (style.align_items == "flex-end") align = TAFFY_ALIGN_ITEMS_FLEX_END;
        else if (style.align_items == "center") align = TAFFY_ALIGN_ITEMS_CENTER;
        else if (style.align_items == "baseline") align = TAFFY_ALIGN_ITEMS_BASELINE;
        else if (style.align_items == "stretch") align = TAFFY_ALIGN_ITEMS_STRETCH;
        TaffyStyle_SetAlignItems(taffy_style, align);

        // Align content
        TaffyAlignContent align_content = TAFFY_ALIGN_CONTENT_STRETCH;
        if (style.align_content == "flex-start") align_content = TAFFY_ALIGN_CONTENT_FLEX_START;
        else if (style.align_content == "flex-end") align_content = TAFFY_ALIGN_CONTENT_FLEX_END;
        else if (style.align_content == "center") align_content = TAFFY_ALIGN_CONTENT_CENTER;
        else if (style.align_content == "space-between") align_content = TAFFY_ALIGN_CONTENT_SPACE_BETWEEN;
        else if (style.align_content == "space-around") align_content = TAFFY_ALIGN_CONTENT_SPACE_AROUND;
        else if (style.align_content == "stretch") align_content = TAFFY_ALIGN_CONTENT_STRETCH;
        TaffyStyle_SetAlignContent(taffy_style, align_content);

        // Flex grow/shrink
        TaffyStyle_SetFlexGrow(taffy_style, style.flex_grow);
        TaffyStyle_SetFlexShrink(taffy_style, style.flex_shrink);

        // Flex basis
        apply_dimension(taffy_style, style.flex_basis, TaffyStyle_SetFlexBasis);

        // Gap
        apply_dimension(taffy_style, style.column_gap, TaffyStyle_SetColumnGap);
        apply_dimension(taffy_style, style.row_gap, TaffyStyle_SetRowGap);
    }
}

void LayoutEngine::SyncChildren(Element* element, TaffyNodeId node) {
    if (!element || !taffy_tree_) {
        return;
    }

    // Get children from element
    auto children = element->GetChildren();
    for (auto& child_node : children) {
        if (child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            Element* child_element = static_cast<Element*>(child_node.get());

            // Create or get Taffy node for child
            TaffyNodeId child_taffy_node;
            auto it = element_to_node_.find(child_element);
            if (it == element_to_node_.end()) {
                child_taffy_node = CreateNode(child_element);
                element_to_node_[child_element] = child_taffy_node;
                node_to_element_[child_taffy_node._0] = child_element;
            } else {
                child_taffy_node = it->second;
            }

            // Add as child to parent node
            TaffyTree_AppendChild(taffy_tree_, node, child_taffy_node);

            // Recursively sync
            SyncChildren(child_element, child_taffy_node);
        }
    }
}

void LayoutEngine::BuildSubtree(Element* element, TaffyNodeId parent_node) {
    if (!element || !taffy_tree_) {
        return;
    }

    // Create node for this element
    TaffyNodeId node = CreateNode(element);
    element_to_node_[element] = node;
    node_to_element_[node._0] = element;

    // Set as root if no parent
    if (parent_node._0 == 0 && !has_root_) {
        root_node_ = node;
        has_root_ = true;
    }

    // Add to parent in Taffy tree
    if (parent_node._0 != 0) {
        TaffyTree_AppendChild(taffy_tree_, parent_node, node);
    }

    // Recursively build children
    auto children = element->GetChildren();
    for (auto& child_node : children) {
        if (child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            Element* child_element = static_cast<Element*>(child_node.get());
            BuildSubtree(child_element, node);
        }
    }
}

void LayoutEngine::ReadLayoutResults(Element* element) {
    if (!element || !taffy_tree_) {
        return;
    }

    auto it = element_to_node_.find(element);
    if (it == element_to_node_.end()) {
        return;
    }

    // Read layout from Taffy
    TaffyResult_TaffyLayout result = TaffyTree_GetLayout(taffy_tree_, it->second);
    if (result.return_code != TAFFY_RETURN_CODE_OK) {
        return;
    }

    // Update element's render object with layout info
    auto render_obj = element->GetRenderObject();
    if (render_obj) {
        LayoutInfo info;
        info.x = result.value.x;
        info.y = result.value.y;
        info.width = result.value.width;
        info.height = result.value.height;
        info.is_laid_out = true;

        render_obj->SetLayoutInfo(info);
    }
}

} // namespace lightui
