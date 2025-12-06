/**
 * @file block.cpp
 * @brief Block layout algorithm implementation
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/block.rs
 */

#include "block.h"
#include <algorithm>
#include <iostream>

namespace lightui {

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

namespace {

/// Compute scrollbar gutter from overflow style
Rect<float> ComputeScrollbarGutter(Point<Overflow> overflow, float scrollbar_width) {
    // Scrollbars take space in the opposite axis
    float right = (overflow.y == Overflow::Scroll) ? scrollbar_width : 0.0f;
    float bottom = (overflow.x == Overflow::Scroll) ? scrollbar_width : 0.0f;
    return Rect<float>{0.0f, right, 0.0f, bottom};
}

} // anonymous namespace

//------------------------------------------------------------------------------
// Main Block Layout
//------------------------------------------------------------------------------

LayoutOutput ComputeBlockLayout(
    LayoutBlockContainer& tree,
    NodeId node_id,
    const LayoutInput& inputs
) {
    const auto& style = tree.GetBlockContainerStyle(node_id);
    
    // Resolve padding and border
    auto padding = ResolveOrZero(style.padding, inputs.parent_size.width);
    auto border = ResolveOrZero(style.border, inputs.parent_size.width);
    Size<float> padding_border_size = {
        padding.left + padding.right + border.left + border.right,
        padding.top + padding.bottom + border.top + border.bottom
    };
    
    // Box sizing adjustment
    Size<float> box_sizing_adjustment = 
        (style.box_sizing == BoxSizing::ContentBox) ? padding_border_size : Size<float>::Zero();
    
    // Resolve min/max/size
    auto min_size = MaybeResolve(style.min_size, inputs.parent_size);
    min_size = MaybeApplyAspectRatio(min_size, style.aspect_ratio);
    min_size = MaybeAdd(min_size, box_sizing_adjustment);
    
    auto max_size = MaybeResolve(style.max_size, inputs.parent_size);
    max_size = MaybeApplyAspectRatio(max_size, style.aspect_ratio);
    max_size = MaybeAdd(max_size, box_sizing_adjustment);
    
    Size<std::optional<float>> clamped_style_size;
    if (inputs.sizing_mode == SizingMode::InherentSize) {
        clamped_style_size = MaybeResolve(style.size, inputs.parent_size);
        clamped_style_size = MaybeApplyAspectRatio(clamped_style_size, style.aspect_ratio);
        clamped_style_size = MaybeAdd(clamped_style_size, box_sizing_adjustment);
        clamped_style_size = MaybeClamp(clamped_style_size, min_size, max_size);
    }
    
    // Compute min_max_definite_size
    Size<std::optional<float>> min_max_definite_size = {
        (min_size.width.has_value() && max_size.width.has_value() && 
         *max_size.width <= *min_size.width) ? min_size.width : std::nullopt,
        (min_size.height.has_value() && max_size.height.has_value() && 
         *max_size.height <= *min_size.height) ? min_size.height : std::nullopt
    };
    
    // Compute styled_based_known_dimensions
    auto styled_based_known_dimensions = inputs.known_dimensions;
    if (!styled_based_known_dimensions.width.has_value()) {
        styled_based_known_dimensions.width = min_max_definite_size.width;
    }
    if (!styled_based_known_dimensions.width.has_value()) {
        styled_based_known_dimensions.width = clamped_style_size.width;
    }
    if (!styled_based_known_dimensions.height.has_value()) {
        styled_based_known_dimensions.height = min_max_definite_size.height;
    }
    if (!styled_based_known_dimensions.height.has_value()) {
        styled_based_known_dimensions.height = clamped_style_size.height;
    }
    
    // Ensure minimum padding_border_size
    if (styled_based_known_dimensions.width.has_value()) {
        styled_based_known_dimensions.width = 
            std::optional<float>(f32_max(*styled_based_known_dimensions.width, padding_border_size.width));
    }
    if (styled_based_known_dimensions.height.has_value()) {
        styled_based_known_dimensions.height = 
            std::optional<float>(f32_max(*styled_based_known_dimensions.height, padding_border_size.height));
    }
    
    // Short-circuit if both dimensions known and only computing size
    if (inputs.sizing_mode == SizingMode::ContentSize) {
        if (styled_based_known_dimensions.width.has_value() && 
            styled_based_known_dimensions.height.has_value()) {
            return LayoutOutput::FromOuterSize(Size<float>{
                *styled_based_known_dimensions.width,
                *styled_based_known_dimensions.height
            });
        }
    }
    
    // Continue with inner layout
    LayoutInput inner_inputs = inputs;
    inner_inputs.known_dimensions = styled_based_known_dimensions;
    
    return ComputeBlockLayoutInner(tree, node_id, inner_inputs);
}

//------------------------------------------------------------------------------
// Inner Block Layout
//------------------------------------------------------------------------------

LayoutOutput ComputeBlockLayoutInner(
    LayoutBlockContainer& tree,
    NodeId node_id,
    const LayoutInput& inputs
) {
    const auto& style = tree.GetBlockContainerStyle(node_id);
    
    // Resolve padding, border, margin
    auto padding = ResolveOrZero(style.padding, inputs.parent_size.width);
    auto border = ResolveOrZero(style.border, inputs.parent_size.width);
    auto scrollbar_gutter = ComputeScrollbarGutter(style.overflow, style.scrollbar_width);


    
    Rect<float> padding_border = {
        padding.left + border.left,
        padding.right + border.right,
        padding.top + border.top,
        padding.bottom + border.bottom
    };
    Size<float> padding_border_size = {
        padding_border.left + padding_border.right,
        padding_border.top + padding_border.bottom
    };
    
    Rect<float> content_box_inset = {
        padding_border.left + scrollbar_gutter.left,
        padding_border.right + scrollbar_gutter.right,
        padding_border.top + scrollbar_gutter.top,
        padding_border.bottom + scrollbar_gutter.bottom
    };
    
    Size<float> content_box_inset_sum = {
        content_box_inset.left + content_box_inset.right,
        content_box_inset.top + content_box_inset.bottom
    };

    // For block layout, we need a definite width for percentage resolution.
    // If known_dimensions.width is not set, use available_space.width as the basis.
    auto container_content_box_size = MaybeSub(inputs.known_dimensions, content_box_inset_sum);
    if (!container_content_box_size.width.has_value() && inputs.available_space.width.IsDefinite()) {
        container_content_box_size.width = std::optional<float>(
            inputs.available_space.width.value - content_box_inset_sum.width);
    }



    // Box sizing adjustment
    Size<float> box_sizing_adjustment = 
        (style.box_sizing == BoxSizing::ContentBox) ? padding_border_size : Size<float>::Zero();
    
    // Resolve size constraints
    auto size = MaybeResolve(style.size, inputs.parent_size);
    size = MaybeApplyAspectRatio(size, style.aspect_ratio);
    size = MaybeAdd(size, box_sizing_adjustment);
    
    auto min_size = MaybeResolve(style.min_size, inputs.parent_size);
    min_size = MaybeApplyAspectRatio(min_size, style.aspect_ratio);
    min_size = MaybeAdd(min_size, box_sizing_adjustment);
    
    auto max_size = MaybeResolve(style.max_size, inputs.parent_size);
    max_size = MaybeApplyAspectRatio(max_size, style.aspect_ratio);
    max_size = MaybeAdd(max_size, box_sizing_adjustment);
    
    // Determine margin collapsing behavior
    // Sticky behaves like relative for margin collapsing
    bool is_in_flow_position = (style.position == Position::Relative || style.position == Position::Sticky);
    Line<bool> own_margins_collapse_with_children = {
        inputs.vertical_margins_are_collapsible.start &&
        style.overflow.x != Overflow::Scroll && style.overflow.y != Overflow::Scroll &&
        is_in_flow_position &&
        padding.top == 0.0f && border.top == 0.0f,

        inputs.vertical_margins_are_collapsible.end &&
        style.overflow.x != Overflow::Scroll && style.overflow.y != Overflow::Scroll &&
        is_in_flow_position &&
        padding.bottom == 0.0f && border.bottom == 0.0f &&
        !size.height.has_value()
    };
    
    BlockTextAlign text_align = style.text_align;
    
    // 1. Generate items
    auto items = GenerateItemList(tree, node_id, container_content_box_size);
    
    // 2. Compute container width
    // For block elements with width: auto, use available width (not intrinsic width)
    // This is the CSS block formatting context behavior
    float container_outer_width;

    if (inputs.known_dimensions.width.has_value()) {
        container_outer_width = *inputs.known_dimensions.width;
    } else if (inputs.available_space.width.IsDefinite() &&
               inputs.sizing_mode == SizingMode::InherentSize &&
               !style.size.width.IsPercent() && !style.size.width.IsLength()) {
        // Block element with width: auto - fill available space
        container_outer_width = inputs.available_space.width.value;
        if (min_size.width.has_value()) {
            container_outer_width = f32_max(container_outer_width, *min_size.width);
        }
        if (max_size.width.has_value()) {
            container_outer_width = f32_min(container_outer_width, *max_size.width);
        }
    } else {
        // Shrink-to-fit width (for floats, inline-blocks, etc.)
        AvailableSpace available_width = inputs.available_space.width;
        if (available_width.IsDefinite()) {
            available_width = AvailableSpace::Definite(
                available_width.value - content_box_inset_sum.width);
        }
        float intrinsic_width = DetermineContentBasedContainerWidth(tree, items, available_width)
            + content_box_inset_sum.width;

        container_outer_width = intrinsic_width;
        if (min_size.width.has_value()) {
            container_outer_width = f32_max(container_outer_width, *min_size.width);
        }
        if (max_size.width.has_value()) {
            container_outer_width = f32_min(container_outer_width, *max_size.width);
        }
        container_outer_width = f32_max(container_outer_width, padding_border_size.width);
    }
    
    // 3. Perform final layout
    auto resolved_padding = ResolveOrZero(style.padding, std::optional<float>(container_outer_width));
    auto resolved_border = ResolveOrZero(style.border, std::optional<float>(container_outer_width));
    Rect<float> resolved_content_box_inset = {
        resolved_padding.left + resolved_border.left + scrollbar_gutter.left,
        resolved_padding.right + resolved_border.right + scrollbar_gutter.right,
        resolved_padding.top + resolved_border.top + scrollbar_gutter.top,
        resolved_padding.bottom + resolved_border.bottom + scrollbar_gutter.bottom
    };
    
    auto [inflow_content_size, intrinsic_outer_height, first_margin_set, last_margin_set] =
        PerformFinalLayoutOnInFlowChildren(
            tree, items, container_outer_width,
            content_box_inset, resolved_content_box_inset,
            text_align, own_margins_collapse_with_children
        );
    
    // Compute final height
    float container_outer_height;
    if (inputs.known_dimensions.height.has_value()) {
        container_outer_height = *inputs.known_dimensions.height;
    } else {
        container_outer_height = intrinsic_outer_height;
        if (min_size.height.has_value()) {
            container_outer_height = f32_max(container_outer_height, *min_size.height);
        }
        if (max_size.height.has_value()) {
            container_outer_height = f32_min(container_outer_height, *max_size.height);
        }
    }
    container_outer_height = f32_max(container_outer_height, padding_border_size.height);
    
    Size<float> final_outer_size = {container_outer_width, container_outer_height};
    
    // 4. Layout absolutely positioned children
    Rect<float> absolute_position_inset = {
        resolved_border.left + scrollbar_gutter.left,
        resolved_border.right + scrollbar_gutter.right,
        resolved_border.top + scrollbar_gutter.top,
        resolved_border.bottom + scrollbar_gutter.bottom
    };
    Size<float> absolute_position_area = {
        final_outer_size.width - absolute_position_inset.left - absolute_position_inset.right,
        final_outer_size.height - absolute_position_inset.top - absolute_position_inset.bottom
    };
    Point<float> absolute_position_offset = {
        absolute_position_inset.left,
        absolute_position_inset.top
    };
    
    auto absolute_content_size = PerformAbsoluteLayoutOnAbsoluteChildren(
        tree, items, absolute_position_area, absolute_position_offset
    );
    
    // Build output
    LayoutOutput output;
    output.size = final_outer_size;
    output.content_size = Size<float>{
        f32_max(inflow_content_size.width, absolute_content_size.width),
        f32_max(inflow_content_size.height, absolute_content_size.height)
    };
    
    if (own_margins_collapse_with_children.start) {
        output.top_margin = first_margin_set.Resolve();
    } else {
        output.top_margin = ResolveOrZero(style.margin.top, inputs.parent_size.width);
    }
    
    if (own_margins_collapse_with_children.end) {
        output.bottom_margin = last_margin_set.Resolve();
    } else {
        output.bottom_margin = ResolveOrZero(style.margin.bottom, inputs.parent_size.width);
    }
    
    return output;
}

//------------------------------------------------------------------------------
// Generate Item List
//------------------------------------------------------------------------------

std::vector<BlockItem> GenerateItemList(
    LayoutBlockContainer& tree,
    NodeId node,
    Size<std::optional<float>> node_inner_size
) {
    std::vector<BlockItem> items;
    size_t child_count = tree.ChildCount(node);

    uint32_t order = 0;
    for (size_t i = 0; i < child_count; ++i) {
        NodeId child_id = tree.GetChildId(node, i);
        const auto& child_style = tree.GetBlockChildStyle(child_id);

        // Skip display:none children
        if (child_style.box_generation_mode == BoxGenerationMode::None) {
            continue;
        }

        // Resolve padding and border
        auto padding = ResolveOrZero(child_style.padding, node_inner_size.width);
        auto border = ResolveOrZero(child_style.border, node_inner_size.width);
        Size<float> pb_sum = {
            padding.left + padding.right + border.left + border.right,
            padding.top + padding.bottom + border.top + border.bottom
        };

        Size<float> box_sizing_adjustment =
            (child_style.box_sizing == BoxSizing::ContentBox) ? pb_sum : Size<float>::Zero();

        // Resolve size constraints
        auto size = MaybeResolve(child_style.size, node_inner_size);
        size = MaybeApplyAspectRatio(size, child_style.aspect_ratio);
        size = MaybeAdd(size, box_sizing_adjustment);

        auto min_size = MaybeResolve(child_style.min_size, node_inner_size);
        min_size = MaybeApplyAspectRatio(min_size, child_style.aspect_ratio);
        min_size = MaybeAdd(min_size, box_sizing_adjustment);

        auto max_size = MaybeResolve(child_style.max_size, node_inner_size);
        max_size = MaybeApplyAspectRatio(max_size, child_style.aspect_ratio);
        max_size = MaybeAdd(max_size, box_sizing_adjustment);

        BlockItem item;
        item.node_id = child_id;
        item.order = order++;
        item.is_table = child_style.IsTable();
        item.size = size;
        item.min_size = min_size;
        item.max_size = max_size;
        item.overflow = child_style.overflow;
        item.scrollbar_width = child_style.scrollbar_width;
        item.position = child_style.position;
        item.inset = child_style.inset;
        item.margin = child_style.margin;
        item.padding = padding;
        item.border = border;
        item.padding_border_sum = pb_sum;
        item.computed_size = Size<float>::Zero();
        item.static_position = Point<float>::Zero();
        item.can_be_collapsed_through = false;

        items.push_back(item);
    }

    return items;
}

//------------------------------------------------------------------------------
// Determine Content-Based Container Width
//------------------------------------------------------------------------------

float DetermineContentBasedContainerWidth(
    LayoutBlockContainer& tree,
    const std::vector<BlockItem>& items,
    AvailableSpace available_width
) {
    Size<AvailableSpace> available_space = {
        available_width,
        AvailableSpace::MinContent()
    };

    float max_child_width = 0.0f;

    for (const auto& item : items) {
        // Skip out-of-flow elements (absolute and fixed)
        if (item.position == Position::Absolute || item.position == Position::Fixed) {
            continue;
        }

        auto known_dimensions = MaybeClamp(item.size, item.min_size, item.max_size);

        float width;
        if (known_dimensions.width.has_value()) {
            width = *known_dimensions.width;
        } else {
            // Resolve margin
            float item_x_margin_sum =
                ResolveOrZero(item.margin.left, available_width.IntoOption()) +
                ResolveOrZero(item.margin.right, available_width.IntoOption());

            // Measure child
            Size<AvailableSpace> child_available = available_space;
            if (child_available.width.IsDefinite()) {
                child_available.width = AvailableSpace::Definite(
                    child_available.width.value - item_x_margin_sum);
            }

            auto child_size = tree.MeasureChildSize(
                item.node_id,
                known_dimensions,
                Size<std::optional<float>>{std::nullopt, std::nullopt},
                child_available,
                SizingMode::InherentSize
            );

            width = child_size.width + item_x_margin_sum;
        }

        width = f32_max(width, item.padding_border_sum.width);
        max_child_width = f32_max(max_child_width, width);
    }

    return max_child_width;
}

//------------------------------------------------------------------------------
// Perform Final Layout on In-Flow Children
//------------------------------------------------------------------------------

std::tuple<Size<float>, float, CollapsibleMarginSet, CollapsibleMarginSet>
PerformFinalLayoutOnInFlowChildren(
    LayoutBlockContainer& tree,
    std::vector<BlockItem>& items,
    float container_outer_width,
    Rect<float> content_box_inset,
    Rect<float> resolved_content_box_inset,
    BlockTextAlign text_align,
    Line<bool> own_margins_collapse_with_children
) {
    float container_inner_width = container_outer_width -
        content_box_inset.left - content_box_inset.right;
    Size<std::optional<float>> parent_size = {
        std::optional<float>(container_outer_width),
        std::nullopt
    };
    Size<AvailableSpace> available_space = {
        AvailableSpace::Definite(container_inner_width),
        AvailableSpace::MinContent()
    };

    Size<float> inflow_content_size = Size<float>::Zero();
    float committed_y_offset = resolved_content_box_inset.top;
    float y_offset_for_absolute = resolved_content_box_inset.top;

    CollapsibleMarginSet first_child_top_margin_set = CollapsibleMarginSet::Zero();
    CollapsibleMarginSet active_collapsible_margin_set = CollapsibleMarginSet::Zero();
    bool is_collapsing_with_first_margin_set = true;

    for (auto& item : items) {
        // Handle out-of-flow elements (absolute and fixed)
        if (item.position == Position::Absolute || item.position == Position::Fixed) {
            item.static_position = Point<float>{
                resolved_content_box_inset.left,
                y_offset_for_absolute
            };
            continue;
        }

        // Resolve margin
        auto item_margin = MaybeResolve(item.margin, std::optional<float>(container_outer_width));
        float item_non_auto_x_margin_sum =
            item_margin.left.value_or(0.0f) + item_margin.right.value_or(0.0f);

        // Compute known dimensions
        Size<std::optional<float>> known_dimensions;
        if (item.is_table) {
            known_dimensions = {std::nullopt, std::nullopt};
        } else {
            float width = item.size.width.value_or(
                container_inner_width - item_non_auto_x_margin_sum);
            if (item.min_size.width.has_value()) {
                width = f32_max(width, *item.min_size.width);
            }
            if (item.max_size.width.has_value()) {
                width = f32_min(width, *item.max_size.width);
            }
            known_dimensions = {std::optional<float>(width), item.size.height};
            known_dimensions = MaybeClamp(known_dimensions, item.min_size, item.max_size);
        }

        // Perform child layout
        Size<AvailableSpace> child_available = available_space;
        if (child_available.width.IsDefinite()) {
            child_available.width = AvailableSpace::Definite(
                child_available.width.value - item_non_auto_x_margin_sum);
        }

        auto item_layout = tree.PerformChildLayout(
            item.node_id,
            known_dimensions,
            parent_size,
            child_available,
            SizingMode::InherentSize,
            LineBoolTrue()
        );

        Size<float> final_size = item_layout.size;

        // Compute margin sets
        CollapsibleMarginSet top_margin_set =
            CollapsibleMarginSet::FromMargin(item_margin.top.value_or(0.0f));
        top_margin_set = top_margin_set.CollapseWithMargin(item_layout.top_margin);

        CollapsibleMarginSet bottom_margin_set =
            CollapsibleMarginSet::FromMargin(item_margin.bottom.value_or(0.0f));
        bottom_margin_set = bottom_margin_set.CollapseWithMargin(item_layout.bottom_margin);

        // Expand auto margins
        float free_x_space = f32_max(0.0f,
            container_inner_width - final_size.width - item_non_auto_x_margin_sum);
        float x_axis_auto_margin_size = 0.0f;
        int auto_margin_count = (!item_margin.left.has_value() ? 1 : 0) +
                                (!item_margin.right.has_value() ? 1 : 0);
        if (auto_margin_count > 0) {
            x_axis_auto_margin_size = free_x_space / auto_margin_count;
        }

        Rect<float> resolved_margin = {
            item_margin.left.value_or(x_axis_auto_margin_size),
            item_margin.right.value_or(x_axis_auto_margin_size),
            top_margin_set.Resolve(),
            bottom_margin_set.Resolve()
        };

        // Compute y margin offset
        float y_margin_offset;
        if (is_collapsing_with_first_margin_set && own_margins_collapse_with_children.start) {
            y_margin_offset = 0.0f;
        } else {
            y_margin_offset = active_collapsible_margin_set
                .CollapseWithMargin(resolved_margin.top).Resolve();
        }

        // Set item properties
        item.computed_size = item_layout.size;
        item.can_be_collapsed_through = item_layout.margins_can_collapse_through;
        item.static_position = Point<float>{
            resolved_content_box_inset.left,
            committed_y_offset + active_collapsible_margin_set.Resolve()
        };

        Point<float> location = {
            resolved_content_box_inset.left + resolved_margin.left,
            committed_y_offset + y_margin_offset
        };

        // Apply text alignment
        float item_outer_width = item_layout.size.width +
            resolved_margin.left + resolved_margin.right;
        if (item_outer_width < container_inner_width) {
            switch (text_align) {
                case BlockTextAlign::LegacyRight:
                    location.x += container_inner_width - item_outer_width;
                    break;
                case BlockTextAlign::LegacyCenter:
                    location.x += (container_inner_width - item_outer_width) / 2.0f;
                    break;
                default:
                    break;
            }
        }

        // Apply relative/sticky positioning (inset offsets)
        // Sticky behaves like relative for initial layout
        if (item.position == Position::Relative || item.position == Position::Sticky) {
            auto inset_left = MaybeResolve(item.inset.left, std::optional<float>(container_outer_width));
            auto inset_right = MaybeResolve(item.inset.right, std::optional<float>(container_outer_width));
            auto inset_top = MaybeResolve(item.inset.top, std::optional<float>(container_outer_width));
            auto inset_bottom = MaybeResolve(item.inset.bottom, std::optional<float>(container_outer_width));

            if (inset_left.has_value()) {
                location.x += *inset_left;
            } else if (inset_right.has_value()) {
                location.x -= *inset_right;
            }
            if (inset_top.has_value()) {
                location.y += *inset_top;
            } else if (inset_bottom.has_value()) {
                location.y -= *inset_bottom;
            }
        }

        // Set layout
        Layout layout;
        layout.order = item.order;
        layout.size = item_layout.size;
        layout.content_size = item_layout.content_size;
        layout.location = location;
        layout.padding = item.padding;
        layout.border = item.border;
        tree.SetUnroundedLayout(item.node_id, layout);

        // Update margin tracking
        if (is_collapsing_with_first_margin_set) {
            if (item.can_be_collapsed_through) {
                first_child_top_margin_set = first_child_top_margin_set
                    .CollapseWithSet(top_margin_set)
                    .CollapseWithSet(bottom_margin_set);
            } else {
                first_child_top_margin_set = first_child_top_margin_set
                    .CollapseWithSet(top_margin_set);
                is_collapsing_with_first_margin_set = false;
            }
        }

        // Update y offset
        if (item.can_be_collapsed_through) {
            active_collapsible_margin_set = active_collapsible_margin_set
                .CollapseWithSet(top_margin_set)
                .CollapseWithSet(bottom_margin_set);
            y_offset_for_absolute = committed_y_offset + item_layout.size.height + y_margin_offset;
        } else {
            committed_y_offset += item_layout.size.height + y_margin_offset;
            active_collapsible_margin_set = bottom_margin_set;
            y_offset_for_absolute = committed_y_offset + active_collapsible_margin_set.Resolve();
        }
    }

    // Compute final height
    CollapsibleMarginSet last_child_bottom_margin_set = active_collapsible_margin_set;
    float bottom_y_margin_offset = own_margins_collapse_with_children.end
        ? 0.0f
        : last_child_bottom_margin_set.Resolve();

    committed_y_offset += resolved_content_box_inset.bottom + bottom_y_margin_offset;
    float content_height = f32_max(0.0f, committed_y_offset);

    return {inflow_content_size, content_height, first_child_top_margin_set, last_child_bottom_margin_set};
}

//------------------------------------------------------------------------------
// Perform Absolute Layout
//------------------------------------------------------------------------------

Size<float> PerformAbsoluteLayoutOnAbsoluteChildren(
    LayoutBlockContainer& tree,
    const std::vector<BlockItem>& items,
    Size<float> area_size,
    Point<float> area_offset
) {
    Size<float> absolute_content_size = Size<float>::Zero();

    for (const auto& item : items) {
        // Process absolute and fixed positioned elements
        if (item.position != Position::Absolute && item.position != Position::Fixed) {
            continue;
        }

        const auto& child_style = tree.GetBlockChildStyle(item.node_id);

        // Skip display:none
        if (child_style.box_generation_mode == BoxGenerationMode::None) {
            continue;
        }

        // Resolve inset
        auto left = MaybeResolve(child_style.inset.left, std::optional<float>(area_size.width));
        auto right = MaybeResolve(child_style.inset.right, std::optional<float>(area_size.width));
        auto top = MaybeResolve(child_style.inset.top, std::optional<float>(area_size.height));
        auto bottom = MaybeResolve(child_style.inset.bottom, std::optional<float>(area_size.height));

        // Resolve margin
        auto margin = MaybeResolve(child_style.margin, std::optional<float>(area_size.width));

        // Resolve padding and border
        auto padding = ResolveOrZero(child_style.padding, std::optional<float>(area_size.width));
        auto border = ResolveOrZero(child_style.border, std::optional<float>(area_size.width));
        Size<float> padding_border_sum = {
            padding.left + padding.right + border.left + border.right,
            padding.top + padding.bottom + border.top + border.bottom
        };

        Size<float> box_sizing_adjustment =
            (child_style.box_sizing == BoxSizing::ContentBox) ? padding_border_sum : Size<float>::Zero();

        // Resolve size constraints
        auto style_size = MaybeResolve(child_style.size, Size<std::optional<float>>{
            std::optional<float>(area_size.width),
            std::optional<float>(area_size.height)
        });
        style_size = MaybeApplyAspectRatio(style_size, child_style.aspect_ratio);
        style_size = MaybeAdd(style_size, box_sizing_adjustment);

        auto min_size = MaybeResolve(child_style.min_size, Size<std::optional<float>>{
            std::optional<float>(area_size.width),
            std::optional<float>(area_size.height)
        });
        min_size = MaybeApplyAspectRatio(min_size, child_style.aspect_ratio);
        min_size = MaybeAdd(min_size, box_sizing_adjustment);

        auto max_size = MaybeResolve(child_style.max_size, Size<std::optional<float>>{
            std::optional<float>(area_size.width),
            std::optional<float>(area_size.height)
        });
        max_size = MaybeApplyAspectRatio(max_size, child_style.aspect_ratio);
        max_size = MaybeAdd(max_size, box_sizing_adjustment);

        auto known_dimensions = MaybeClamp(style_size, min_size, max_size);

        // Fill in width from left/right
        if (!known_dimensions.width.has_value() && left.has_value() && right.has_value()) {
            float new_width = area_size.width - *left - *right;
            if (margin.left.has_value()) new_width -= *margin.left;
            if (margin.right.has_value()) new_width -= *margin.right;
            known_dimensions.width = std::optional<float>(f32_max(new_width, 0.0f));
            known_dimensions = MaybeApplyAspectRatio(known_dimensions, child_style.aspect_ratio);
            known_dimensions = MaybeClamp(known_dimensions, min_size, max_size);
        }

        // Fill in height from top/bottom
        if (!known_dimensions.height.has_value() && top.has_value() && bottom.has_value()) {
            float new_height = area_size.height - *top - *bottom;
            if (margin.top.has_value()) new_height -= *margin.top;
            if (margin.bottom.has_value()) new_height -= *margin.bottom;
            known_dimensions.height = std::optional<float>(f32_max(new_height, 0.0f));
            known_dimensions = MaybeApplyAspectRatio(known_dimensions, child_style.aspect_ratio);
            known_dimensions = MaybeClamp(known_dimensions, min_size, max_size);
        }

        // Measure child
        auto measured_size = tree.MeasureChildSize(
            item.node_id,
            known_dimensions,
            Size<std::optional<float>>{
                std::optional<float>(area_size.width),
                std::optional<float>(area_size.height)
            },
            Size<AvailableSpace>{
                AvailableSpace::Definite(area_size.width),
                AvailableSpace::Definite(area_size.height)
            },
            SizingMode::ContentSize
        );

        Size<float> final_size = {
            known_dimensions.width.value_or(measured_size.width),
            known_dimensions.height.value_or(measured_size.height)
        };
        final_size = Clamp(final_size, min_size, max_size);

        // Perform layout
        auto layout_output = tree.PerformChildLayout(
            item.node_id,
            Size<std::optional<float>>{
                std::optional<float>(final_size.width),
                std::optional<float>(final_size.height)
            },
            Size<std::optional<float>>{
                std::optional<float>(area_size.width),
                std::optional<float>(area_size.height)
            },
            Size<AvailableSpace>{
                AvailableSpace::Definite(area_size.width),
                AvailableSpace::Definite(area_size.height)
            },
            SizingMode::ContentSize,
            LineBoolFalse()
        );

        // Resolve margins
        Rect<float> resolved_margin = {
            margin.left.value_or(0.0f),
            margin.right.value_or(0.0f),
            margin.top.value_or(0.0f),
            margin.bottom.value_or(0.0f)
        };

        // Compute location
        Point<float> location;

        // X position
        if (left.has_value()) {
            location.x = area_offset.x + *left + resolved_margin.left;
        } else if (right.has_value()) {
            location.x = area_offset.x + area_size.width - final_size.width - *right - resolved_margin.right;
        } else {
            location.x = item.static_position.x + resolved_margin.left;
        }

        // Y position
        if (top.has_value()) {
            location.y = area_offset.y + *top + resolved_margin.top;
        } else if (bottom.has_value()) {
            location.y = area_offset.y + area_size.height - final_size.height - *bottom - resolved_margin.bottom;
        } else {
            location.y = item.static_position.y + resolved_margin.top;
        }

        // Set layout
        Layout layout;
        layout.order = item.order;
        layout.size = final_size;
        layout.content_size = layout_output.content_size;
        layout.location = location;
        layout.padding = padding;
        layout.border = border;
        tree.SetUnroundedLayout(item.node_id, layout);
    }

    return absolute_content_size;
}

} // namespace lightui

