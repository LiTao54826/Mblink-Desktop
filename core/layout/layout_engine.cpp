/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation using Taffy
 */

#include "layout_engine.h"
#include "dom/element.h"
#include "render/render_object.h"
#include <cmath>
#include <iostream>

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

void LayoutEngine::BuildLayoutTree(std::shared_ptr<RenderObject> root) {
    if (!root || !taffy_tree_) {
        return;
    }

    Clear();

    // Build tree from root
    TaffyNodeId invalid_parent;
    invalid_parent._0 = 0;
    BuildSubtree(root.get(), invalid_parent);
}

void LayoutEngine::ComputeLayout(float available_width, float available_height) {
    if (!has_root_ || !taffy_tree_) {
        return;
    }

    // Call Taffy layout computation
    TaffyTree_ComputeLayout(taffy_tree_, root_node_, available_width, available_height);
}

void LayoutEngine::GetLayoutInfo(std::shared_ptr<RenderObject> root) {
    if (!root || !taffy_tree_) {
        return;
    }

    // Read layout results recursively
    ReadLayoutResults(root.get());
}

void LayoutEngine::UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
    auto it = element_to_node_.find(render_obj);
    if (it != element_to_node_.end()) {
        ApplyStyle(it->second, style);
    }
}

void LayoutEngine::AddElement(RenderObject* render_obj, RenderObject* parent) {
    if (!render_obj || HasElement(render_obj) || !taffy_tree_) {
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

    TaffyNodeId node = CreateNode(render_obj);
    element_to_node_[render_obj] = node;
    node_to_element_[node._0] = render_obj;

    // Add to parent in Taffy tree
    if (parent_node._0 != 0) {
        TaffyTree_AppendChild(taffy_tree_, parent_node, node);
    }

    if (!has_root_ && !parent) {
        root_node_ = node;
        has_root_ = true;
    }
}

void LayoutEngine::RemoveElement(RenderObject* render_obj) {
    auto it = element_to_node_.find(render_obj);
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

bool LayoutEngine::HasElement(RenderObject* render_obj) const {
    return element_to_node_.find(render_obj) != element_to_node_.end();
}

TaffyNodeId LayoutEngine::CreateNode(RenderObject* render_obj) {
    TaffyNodeId node;
    node._0 = 0;

    if (!taffy_tree_) {
        return node;
    }

    // Create Taffy node
    TaffyNodeIdResult result = TaffyTree_NewNode(taffy_tree_);
    if (result.return_code == TAFFY_RETURN_CODE_OK) {
        node = result.value;

        // Apply render object's computed style
        if (render_obj) {
            ApplyStyle(node, render_obj->GetComputedStyle());

            // For text nodes, measure text and set explicit size
            if (render_obj->GetType() == RenderObjectType::TEXT) {
                // Force layout to measure text
                render_obj->Layout(0, 0);
                const auto& layout_info = render_obj->GetLayoutInfo();

                // Debug: Print text measurement
                auto* text_obj = dynamic_cast<RenderText*>(render_obj);
                if (text_obj) {
                    printf("[TextMeasure] Text: \"%s\", measured size: %.1fx%.1f\n",
                           text_obj->GetText().c_str(), layout_info.width, layout_info.height);
                }

                // Set explicit width and height for text node
                // Note: Text nodes should use border-box since they don't have padding/border
                TaffyStyleMutRefResult style_result = TaffyTree_GetStyleMut(taffy_tree_, node);
                if (style_result.return_code == TAFFY_RETURN_CODE_OK) {
                    TaffyStyleMutRef taffy_style = style_result.value;

                    // Text nodes should use border-box (Taffy default)
                    TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_BORDER_BOX);

                    // Clear any padding/margin/border for text nodes
                    TaffyStyle_SetPaddingLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetPaddingRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetPaddingTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetPaddingBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                    TaffyStyle_SetMarginLeft(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetMarginRight(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetMarginTop(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetMarginBottom(taffy_style, 0.0f, TAFFY_UNIT_LENGTH);

                    TaffyStyle_SetWidth(taffy_style, layout_info.width, TAFFY_UNIT_LENGTH);
                    TaffyStyle_SetHeight(taffy_style, layout_info.height, TAFFY_UNIT_LENGTH);

                    printf("[TextStyle] Set text node size: %.1fx%.1f\n", layout_info.width, layout_info.height);
                }
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

    // Set box-sizing to content-box (CSS default)
    // Taffy defaults to border-box, but CSS defaults to content-box
    TaffyStyle_SetBoxSizing(taffy_style, TAFFY_BOX_SIZING_CONTENT_BOX);

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
    TaffyStyle_SetBorderTop(taffy_style, style.border_top_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderRight(taffy_style, style.border_right_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderBottom(taffy_style, style.border_bottom_width, TAFFY_UNIT_LENGTH);
    TaffyStyle_SetBorderLeft(taffy_style, style.border_left_width, TAFFY_UNIT_LENGTH);

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

void LayoutEngine::SyncChildren(RenderObject* render_obj, TaffyNodeId node) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    // Get children from render object
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        // Create or get Taffy node for child
        TaffyNodeId child_taffy_node;
        auto it = element_to_node_.find(child.get());
        if (it == element_to_node_.end()) {
            child_taffy_node = CreateNode(child.get());
            element_to_node_[child.get()] = child_taffy_node;
            node_to_element_[child_taffy_node._0] = child.get();
        } else {
            child_taffy_node = it->second;
        }

        // Add as child to parent node
        TaffyTree_AppendChild(taffy_tree_, node, child_taffy_node);

        // Recursively sync
        SyncChildren(child.get(), child_taffy_node);
    }
}

void LayoutEngine::BuildSubtree(RenderObject* render_obj, TaffyNodeId parent_node) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    // Create node for this render object
    TaffyNodeId node = CreateNode(render_obj);
    element_to_node_[render_obj] = node;
    node_to_element_[node._0] = render_obj;

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
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        BuildSubtree(child.get(), node);
    }
}

void LayoutEngine::ReadLayoutResults(RenderObject* render_obj) {
    if (!render_obj || !taffy_tree_) {
        return;
    }

    auto it = element_to_node_.find(render_obj);
    if (it == element_to_node_.end()) {
        return;
    }

    // Read layout from Taffy
    TaffyResult_TaffyLayout result = TaffyTree_GetLayout(taffy_tree_, it->second);
    if (result.return_code != TAFFY_RETURN_CODE_OK) {
        return;
    }

    // Update render object with layout info
    LayoutInfo& info = render_obj->GetLayoutInfo();
    info.x = result.value.x;
    info.y = result.value.y;
    info.width = result.value.width;
    info.height = result.value.height;
    info.is_laid_out = true;

    // Debug: Print layout info for first few elements
    static int debug_count = 0;
    if (debug_count < 10) {
        std::cout << "[ReadLayoutResults] Element " << debug_count
                  << ": x=" << info.x << ", y=" << info.y
                  << ", w=" << info.width << ", h=" << info.height << std::endl;
        debug_count++;
    }

    // Recursively read layout for children
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        ReadLayoutResults(child.get());
    }

    // Apply text-align to children (Taffy doesn't support text-align CSS property)
    const auto& style = render_obj->GetComputedStyle();
    if (!children.empty() && (style.text_align == "center" || style.text_align == "right")) {
        // Calculate content width (excluding padding)
        float padding_left = style.padding.left.ToPx(info.width, style.font_size);
        float padding_right = style.padding.right.ToPx(info.width, style.font_size);
        float content_width = info.width - padding_left - padding_right;

        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            float child_width = child_info.width;

            if (style.text_align == "center") {
                // Center align: move child to center
                float offset = (content_width - child_width) / 2.0f;
                child_info.x = padding_left + offset;
            } else if (style.text_align == "right") {
                // Right align: move child to right
                child_info.x = padding_left + content_width - child_width;
            }
        }
    }
}

} // namespace lightui
