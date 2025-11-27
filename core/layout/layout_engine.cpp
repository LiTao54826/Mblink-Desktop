/**
 * @file layout_engine.cpp
 * @brief Layout engine implementation using Taffy
 */

#include "layout_engine.h"
#include "dom/element.h"
#include "render/render_object.h"
#include "render/text_renderer.h"
#include "render/text/font_manager.h"
#include <cmath>
#include <iostream>

// Text measurement callback for Taffy
// This function is called by Taffy during layout to measure text nodes
static TaffySize TextMeasureFunction(
    TaffyMeasureMode width_measure_mode,
    float width,
    TaffyMeasureMode height_measure_mode,
    float height,
    void* context)
{
    TaffySize size = {0.0f, 0.0f};

    if (!context) {
        return size;
    }

    auto* text_obj = static_cast<lightui::RenderText*>(context);
    const std::string& text = text_obj->GetText();

    if (text.empty()) {
        return size;
    }

    const auto& style = text_obj->GetComputedStyle();

    // Create font
    lightui::FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? lightui::FontWeight::BOLD : lightui::FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? lightui::FontStyle::ITALIC : lightui::FontStyle::NORMAL;

    SkFont font = lightui::FontManager::GetInstance().LoadFont(desc);
    lightui::TextRenderer text_renderer(nullptr);

    // Determine available width for text wrapping
    float available_width = 0.0f;
    bool should_wrap = false;

    switch (width_measure_mode) {
        case TAFFY_MEASURE_MODE_EXACT:
            // Exact width constraint - wrap text to this width
            available_width = width;
            should_wrap = true;
            break;
        case TAFFY_MEASURE_MODE_FIT_CONTENT:
            // Fit content with max width constraint
            available_width = width;
            should_wrap = (width > 0);
            break;
        case TAFFY_MEASURE_MODE_MIN_CONTENT:
            // Minimum content width - wrap at every opportunity
            available_width = 0;
            should_wrap = true;
            break;
        case TAFFY_MEASURE_MODE_MAX_CONTENT:
            // Maximum content width - no wrapping
            available_width = 0;
            should_wrap = false;
            break;
    }

    if (should_wrap && available_width > 0) {
        // Wrap text and calculate size
        std::vector<std::string> lines = text_renderer.WrapText(text, available_width, font);

        // Store wrapped lines in the RenderText object for later rendering
        text_obj->SetWrappedLines(lines);

        float max_line_width = 0.0f;
        for (size_t i = 0; i < lines.size(); ++i) {
            float line_width = text_renderer.MeasureTextWidth(lines[i], font);
            max_line_width = std::max(max_line_width, line_width);
        }

        float line_height = style.line_height * style.font_size;
        size.width = max_line_width;
        size.height = lines.size() * line_height;

        // Store actual text width for text-align calculation
        text_obj->SetActualTextWidth(max_line_width);
    } else {
        // Single line measurement - clear any previous wrapped lines
        text_obj->SetWrappedLines({});

        auto metrics = text_renderer.MeasureText(text, font);
        size.width = metrics.width;
        size.height = metrics.height;

        // Store actual text width for text-align calculation
        text_obj->SetActualTextWidth(metrics.width);
    }

    return size;
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

            // For text nodes, set up measure function for dynamic text measurement
            if (render_obj->GetType() == RenderObjectType::TEXT) {
                auto* text_obj = dynamic_cast<RenderText*>(render_obj);
                if (text_obj) {
                    // Set the measure function with the RenderText object as context
                    // Taffy will call this function during layout to measure the text
                    TaffyTree_SetNodeContext(taffy_tree_, node, TextMeasureFunction, text_obj);

                    // Set text node style
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

                        // Don't set explicit width/height - let Taffy call measure function
                        // Set width to auto so it can be determined by parent container
                        TaffyStyle_SetWidth(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                        TaffyStyle_SetHeight(taffy_style, 0.0f, TAFFY_UNIT_AUTO);
                    }
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
    } else if (style.display == RenderObjectType::GRID) {
        display = TAFFY_DISPLAY_GRID;
    } else if (style.display == RenderObjectType::NONE) {
        display = TAFFY_DISPLAY_NONE;
    } else if (style.display == RenderObjectType::BLOCK) {
        display = TAFFY_DISPLAY_BLOCK;
    }
    TaffyStyle_SetDisplay(taffy_style, display);

    // Apply sizing
    auto apply_dimension = [](TaffyStyleMutRef style_ref, const CSSLength& css_len,
                              auto setter_func, const char* name = nullptr) {
        (void)name;  // suppress unused warning
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

    // Apply CSS Grid properties
    if (style.display == RenderObjectType::GRID) {
        // Grid auto flow
        TaffyGridAutoFlow grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW;
        if (style.grid_auto_flow == "row") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW;
        else if (style.grid_auto_flow == "column") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_COLUMN;
        else if (style.grid_auto_flow == "row dense") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_ROW_DENSE;
        else if (style.grid_auto_flow == "column dense") grid_auto_flow = TAFFY_GRID_AUTO_FLOW_COLUMN_DENSE;
        TaffyStyle_SetGridAutoFlow(taffy_style, grid_auto_flow);

        // Gap (Grid uses the same gap properties as Flexbox)
        apply_dimension(taffy_style, style.column_gap, TaffyStyle_SetColumnGap);
        apply_dimension(taffy_style, style.row_gap, TaffyStyle_SetRowGap);

        // Grid template columns
        if (!style.grid_template_columns.empty()) {
            std::vector<TaffyGridTrack> columns = ParseGridTemplate(style.grid_template_columns);
            if (!columns.empty()) {
                TaffyStyle_SetGridTemplateColumns(taffy_style, columns.data(), columns.size());
            }
        }

        // Grid template rows
        if (!style.grid_template_rows.empty()) {
            std::vector<TaffyGridTrack> rows = ParseGridTemplate(style.grid_template_rows);
            if (!rows.empty()) {
                TaffyStyle_SetGridTemplateRows(taffy_style, rows.data(), rows.size());
            }
        }
    }

    // Apply Grid item placement (for children of grid containers)
    if (!style.grid_column.empty()) {
        // Parse grid-column: "span 2", "1 / 3", etc.
        TaffyGridPlacement column_placement = ParseGridPlacement(style.grid_column);
        TaffyStyle_SetGridColumn(taffy_style, column_placement);
    }

    if (!style.grid_row.empty()) {
        // Parse grid-row: "span 2", "1 / 2", etc.
        TaffyGridPlacement row_placement = ParseGridPlacement(style.grid_row);
        TaffyStyle_SetGridRow(taffy_style, row_placement);
    }

    // Apply Position property
    TaffyPosition position = TAFFY_POSITION_RELATIVE;
    if (style.position == "relative") {
        position = TAFFY_POSITION_RELATIVE;
    } else if (style.position == "absolute") {
        position = TAFFY_POSITION_ABSOLUTE;
    }
    // Note: CSS "fixed" and "sticky" are not supported by Taffy, they will be treated as absolute
    else if (style.position == "fixed" || style.position == "sticky") {
        position = TAFFY_POSITION_ABSOLUTE;
    }
    TaffyStyle_SetPosition(taffy_style, position);

    // Apply Inset properties (top, right, bottom, left)
    apply_dimension(taffy_style, style.top, TaffyStyle_SetInsetTop);
    apply_dimension(taffy_style, style.right, TaffyStyle_SetInsetRight);
    apply_dimension(taffy_style, style.bottom, TaffyStyle_SetInsetBottom);
    apply_dimension(taffy_style, style.left, TaffyStyle_SetInsetLeft);

    // Apply Overflow properties
    TaffyOverflow overflow_x = TAFFY_OVERFLOW_VISIBLE;
    TaffyOverflow overflow_y = TAFFY_OVERFLOW_VISIBLE;

    if (style.overflow == "visible") {
        overflow_x = overflow_y = TAFFY_OVERFLOW_VISIBLE;
    } else if (style.overflow == "hidden") {
        overflow_x = overflow_y = TAFFY_OVERFLOW_HIDDEN;
    } else if (style.overflow == "scroll") {
        overflow_x = overflow_y = TAFFY_OVERFLOW_SCROLL;
    } else if (style.overflow == "auto") {
        overflow_x = overflow_y = TAFFY_OVERFLOW_SCROLL;  // Taffy treats auto as scroll
    }
    // Note: "clip" is not supported by Taffy, treat as hidden
    else if (style.overflow == "clip") {
        overflow_x = overflow_y = TAFFY_OVERFLOW_HIDDEN;
    }

    TaffyStyle_SetOverflowX(taffy_style, overflow_x);
    TaffyStyle_SetOverflowY(taffy_style, overflow_y);

    // Note: z-index is not handled by Taffy layout engine
    // It should be handled by the rendering layer during paint
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

    const auto& style = render_obj->GetComputedStyle();

    // Recursively read layout for children
    auto& children = render_obj->GetChildren();
    for (auto& child : children) {
        ReadLayoutResults(child.get());
    }

    // Fix height for elements with auto height
    // Taffy sometimes doesn't correctly calculate content-based height
    if (style.height.unit == CSSUnit::AUTO && !children.empty()) {
        // Calculate the required height based on children
        float padding_top = style.padding.top.ToPx(info.width, style.font_size);
        float padding_bottom = style.padding.bottom.ToPx(info.width, style.font_size);

        // First, fix any children with negative y coordinates
        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();
            if (child_info.y < 0) {
                // Move child to padding_top position
                child_info.y = padding_top;
            }
        }

        // Now calculate max_child_bottom
        float max_child_bottom = 0;
        for (auto& child : children) {
            const LayoutInfo& child_info = child->GetLayoutInfo();
            float child_bottom = child_info.y + child_info.height;
            max_child_bottom = std::max(max_child_bottom, child_bottom);
        }

        // Calculate required height: content + padding_bottom
        float required_height = max_child_bottom + padding_bottom;

        // Only adjust if the calculated height is larger than Taffy's result
        if (required_height > info.height) {
            info.height = required_height;
        }
    }

    // Fix position for absolute children with bottom property
    // After children heights have been corrected, we need to recalculate y position
    // for absolute elements that use bottom positioning
    if (style.position == "relative" && !children.empty()) {
        for (auto& child : children) {
            const auto& child_style = child->GetComputedStyle();
            if (child_style.position == "absolute" &&
                child_style.bottom.unit == CSSUnit::PX &&
                child_style.top.unit == CSSUnit::AUTO) {

                LayoutInfo& child_info = child->GetLayoutInfo();
                float bottom_offset = child_style.bottom.value;

                // Calculate new y position: parent_height - bottom - child_height
                float new_y = info.height - bottom_offset - child_info.height;
                child_info.y = new_y;
            }
        }
    }

    // Apply text-align to children (Taffy doesn't support text-align CSS property)
    if (!children.empty() && (style.text_align == "center" || style.text_align == "right")) {
        // Calculate content width (excluding padding)
        float padding_left = style.padding.left.ToPx(info.width, style.font_size);
        float padding_right = style.padding.right.ToPx(info.width, style.font_size);
        float content_width = info.width - padding_left - padding_right;

        for (auto& child : children) {
            LayoutInfo& child_info = child->GetLayoutInfo();

            // For text nodes, use actual measured text width instead of layout width
            // (Taffy may stretch text nodes to fill container)
            float child_width = child_info.width;
            if (child->GetType() == RenderObjectType::TEXT) {
                auto* text_obj = dynamic_cast<RenderText*>(child.get());
                if (text_obj && text_obj->GetActualTextWidth() > 0) {
                    child_width = text_obj->GetActualTextWidth();
                }
            }

            float old_x = child_info.x;

            if (style.text_align == "center") {
                // Center align: move child to center
                float offset = (content_width - child_width) / 2.0f;
                child_info.x = padding_left + offset;
            } else if (style.text_align == "right") {
                // Right align: move child to right
                child_info.x = padding_left + content_width - child_width;
            }
            (void)old_x;  // suppress unused warning
        }
    }
}

TaffyGridPlacement LayoutEngine::ParseGridPlacement(const std::string& value) {
    TaffyGridPlacement placement;
    placement.start = 0;
    placement.end = 0;
    placement.span = 0;

    if (value.empty()) {
        return placement;
    }

    // Parse "span N" format
    if (value.find("span") != std::string::npos) {
        size_t span_pos = value.find("span");
        std::string span_str = value.substr(span_pos + 4);

        // Trim whitespace
        size_t first = span_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            span_str = span_str.substr(first);
            size_t last = span_str.find_last_not_of(" \t");
            span_str = span_str.substr(0, last + 1);

            try {
                placement.span = static_cast<uint16_t>(std::stoi(span_str));
            } catch (...) {
                placement.span = 1;
            }
        }
        return placement;
    }

    // Parse "start / end" format
    size_t slash_pos = value.find('/');
    if (slash_pos != std::string::npos) {
        std::string start_str = value.substr(0, slash_pos);
        std::string end_str = value.substr(slash_pos + 1);

        // Trim whitespace
        size_t first = start_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            start_str = start_str.substr(first);
            size_t last = start_str.find_last_not_of(" \t");
            start_str = start_str.substr(0, last + 1);
        }

        first = end_str.find_first_not_of(" \t");
        if (first != std::string::npos) {
            end_str = end_str.substr(first);
            size_t last = end_str.find_last_not_of(" \t");
            end_str = end_str.substr(0, last + 1);
        }

        try {
            placement.start = static_cast<int16_t>(std::stoi(start_str));
            placement.end = static_cast<int16_t>(std::stoi(end_str));
        } catch (...) {
            // Invalid format, use defaults
        }
        return placement;
    }

    // Parse single number (start line)
    try {
        placement.start = static_cast<int16_t>(std::stoi(value));
    } catch (...) {
        // Invalid format, use defaults
    }

    return placement;
}

std::vector<TaffyGridTrack> LayoutEngine::ParseGridTemplate(const std::string& value) {
    std::vector<TaffyGridTrack> tracks;

    if (value.empty()) {
        return tracks;
    }

    // Split by whitespace to get individual track definitions
    std::istringstream iss(value);
    std::string token;

    while (iss >> token) {
        TaffyGridTrack track;
        track.unit = TAFFY_UNIT_AUTO;
        track.value = 0.0f;

        // Check for "auto"
        if (token == "auto") {
            track.unit = TAFFY_UNIT_AUTO;
            track.value = 0.0f;
        }
        // Check for "min-content"
        else if (token == "min-content") {
            track.unit = TAFFY_UNIT_MIN_CONTENT;
            track.value = 0.0f;
        }
        // Check for "max-content"
        else if (token == "max-content") {
            track.unit = TAFFY_UNIT_MAX_CONTENT;
            track.value = 0.0f;
        }
        // Check for fr units (e.g., "1fr", "2.5fr")
        else if (token.find("fr") != std::string::npos) {
            try {
                float fr_value = std::stof(token.substr(0, token.find("fr")));
                track.unit = TAFFY_UNIT_FR;
                track.value = fr_value;
            } catch (...) {
                track.unit = TAFFY_UNIT_FR;
                track.value = 1.0f;
            }
        }
        // Check for percentage (e.g., "50%", "33.33%")
        else if (token.find('%') != std::string::npos) {
            try {
                float percent_value = std::stof(token.substr(0, token.find('%')));
                track.unit = TAFFY_UNIT_PERCENT;
                track.value = percent_value / 100.0f;  // Convert to 0.0-1.0 range
            } catch (...) {
                track.unit = TAFFY_UNIT_PERCENT;
                track.value = 0.0f;
            }
        }
        // Check for pixel values (e.g., "100px", "200px")
        else if (token.find("px") != std::string::npos) {
            try {
                float px_value = std::stof(token.substr(0, token.find("px")));
                track.unit = TAFFY_UNIT_LENGTH;
                track.value = px_value;
            } catch (...) {
                track.unit = TAFFY_UNIT_LENGTH;
                track.value = 0.0f;
            }
        }
        // Check for fit-content() function
        else if (token.find("fit-content(") != std::string::npos) {
            size_t start = token.find('(') + 1;
            size_t end = token.find(')');
            if (end != std::string::npos) {
                std::string arg = token.substr(start, end - start);

                if (arg.find('%') != std::string::npos) {
                    try {
                        float percent_value = std::stof(arg.substr(0, arg.find('%')));
                        track.unit = TAFFY_UNIT_FIT_CONTENT_PERCENT;
                        track.value = percent_value / 100.0f;
                    } catch (...) {
                        track.unit = TAFFY_UNIT_AUTO;
                        track.value = 0.0f;
                    }
                } else if (arg.find("px") != std::string::npos) {
                    try {
                        float px_value = std::stof(arg.substr(0, arg.find("px")));
                        track.unit = TAFFY_UNIT_FIT_CONTENT_PX;
                        track.value = px_value;
                    } catch (...) {
                        track.unit = TAFFY_UNIT_AUTO;
                        track.value = 0.0f;
                    }
                }
            }
        }
        // If we couldn't parse it, default to auto
        else {
            track.unit = TAFFY_UNIT_AUTO;
            track.value = 0.0f;
        }

        tracks.push_back(track);
    }

    return tracks;
}

} // namespace lightui
