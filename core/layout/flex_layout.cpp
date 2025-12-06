/**
 * @file flex_layout.cpp
 * @brief Flexbox layout algorithm implementation
 *
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/flexbox.rs (~2349 lines)
 */

#include "flex_layout.h"
#include "util/math.h"
#include "util/resolve.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace lightui {

//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------

static LayoutOutput ComputePreliminary(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const LayoutInput& inputs
);

static FlexAlgoConstants ComputeConstants(
    LayoutFlexboxContainer& tree,
    const FlexboxContainerStyle& style,
    Size<std::optional<float>> known_dimensions,
    Size<std::optional<float>> parent_size
);

static std::vector<FlexItem> GenerateAnonymousFlexItems(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const FlexAlgoConstants& constants
);

static Size<AvailableSpace> DetermineAvailableSpace(
    Size<std::optional<float>> known_dimensions,
    Size<AvailableSpace> available_space,
    const FlexAlgoConstants& constants
);

static void DetermineFlexBaseSize(
    LayoutFlexboxContainer& tree,
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space,
    std::vector<FlexItem>& flex_items
);

static std::vector<FlexLine> CollectFlexLines(
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space,
    std::vector<FlexItem>& flex_items
);

static void DetermineContainerMainSize(
    LayoutFlexboxContainer& tree,
    Size<AvailableSpace> available_space,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    FlexAlgoConstants& constants
);

static void ResolveFlexibleLengths(
    FlexLine& line,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static void DetermineHypotheticalCrossSize(
    LayoutFlexboxContainer& tree,
    FlexLine& line,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space
);

static void CalculateChildrenBaseLines(
    LayoutFlexboxContainer& tree,
    Size<std::optional<float>> known_dimensions,
    Size<AvailableSpace> available_space,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static void CalculateCrossSize(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    Size<std::optional<float>> known_dimensions,
    const FlexAlgoConstants& constants
);

static void HandleAlignContentStretch(
    std::vector<FlexLine>& flex_lines,
    Size<std::optional<float>> known_dimensions,
    const FlexAlgoConstants& constants
);

static void DetermineUsedCrossSize(
    LayoutFlexboxContainer& tree,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static void DistributeRemainingFreeSpace(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static void ResolveCrossAxisAutoMargins(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static float DetermineContainerCrossSize(
    const std::vector<FlexLine>& flex_lines,
    Size<std::optional<float>> known_dimensions,
    FlexAlgoConstants& constants
);

static void AlignFlexLinesPerAlignContent(
    std::vector<FlexLine>& flex_lines,
    const FlexAlgoConstants& constants,
    float total_line_cross_size
);

static Size<float> FinalLayoutPass(
    LayoutFlexboxContainer& tree,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
);

static Size<float> PerformAbsoluteLayoutOnAbsoluteChildren(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const FlexAlgoConstants& constants
);

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

/// Apply alignment fallback for negative free space or single item
/// Based on https://www.w3.org/TR/css-align-3/ and https://github.com/w3c/csswg-drafts/issues/10154
static AlignContent ApplyAlignmentFallback(
    float free_space,
    size_t num_items,
    AlignContent alignment_mode,
    bool is_safe = false
) {
    // Fallback occurs in two cases:

    // 1. If there is only a single item being aligned and alignment is a distributed alignment keyword
    //    https://www.w3.org/TR/css-align-3/#distribution-values
    if (num_items <= 1 || free_space <= 0.0f) {
        switch (alignment_mode) {
            case AlignContent::Stretch:
                alignment_mode = AlignContent::FlexStart;
                is_safe = true;
                break;
            case AlignContent::SpaceBetween:
                alignment_mode = AlignContent::FlexStart;
                is_safe = true;
                break;
            case AlignContent::SpaceAround:
                alignment_mode = AlignContent::Center;
                is_safe = true;
                break;
            case AlignContent::SpaceEvenly:
                alignment_mode = AlignContent::Center;
                is_safe = true;
                break;
            default:
                break;
        }
    }

    // 2. If free space is negative the "safe" alignment variants all fallback to Start alignment
    if (free_space <= 0.0f && is_safe) {
        alignment_mode = AlignContent::Start;
    }

    return alignment_mode;
}

/// Compute alignment offset for a given alignment mode (supports flex-reverse)
static float ComputeAlignmentOffset(
    float free_space,
    size_t num_items,
    float gap,
    AlignContent alignment,
    bool layout_is_flex_reversed,
    bool is_first
) {
    if (is_first) {
        switch (alignment) {
            case AlignContent::Start:
                return 0.0f;
            case AlignContent::FlexStart:
                return layout_is_flex_reversed ? free_space : 0.0f;
            case AlignContent::End:
                return free_space;
            case AlignContent::FlexEnd:
                return layout_is_flex_reversed ? 0.0f : free_space;
            case AlignContent::Center:
                return free_space / 2.0f;
            case AlignContent::Stretch:
                return 0.0f;
            case AlignContent::SpaceBetween:
                return 0.0f;
            case AlignContent::SpaceAround:
                if (free_space >= 0.0f && num_items > 0) {
                    return (free_space / static_cast<float>(num_items)) / 2.0f;
                } else {
                    return free_space / 2.0f;
                }
            case AlignContent::SpaceEvenly:
                if (free_space >= 0.0f && num_items > 0) {
                    return free_space / static_cast<float>(num_items + 1);
                } else {
                    return free_space / 2.0f;
                }
            default:
                return 0.0f;
        }
    } else {
        float positive_free_space = std::max(free_space, 0.0f);
        float extra = 0.0f;
        switch (alignment) {
            case AlignContent::SpaceBetween:
                if (num_items > 1) {
                    extra = positive_free_space / static_cast<float>(num_items - 1);
                }
                break;
            case AlignContent::SpaceAround:
                if (num_items > 0) {
                    extra = positive_free_space / static_cast<float>(num_items);
                }
                break;
            case AlignContent::SpaceEvenly:
                if (num_items > 0) {
                    extra = positive_free_space / static_cast<float>(num_items + 1);
                }
                break;
            default:
                break;
        }
        return gap + extra;
    }
}

/// Get the gap between items for a given alignment mode
static float GetGapForAlignment(
    float free_space,
    size_t num_items,
    AlignContent alignment
) {
    if (num_items <= 1) return 0.0f;
    
    switch (alignment) {
        case AlignContent::SpaceBetween:
            return free_space / static_cast<float>(num_items - 1);
        case AlignContent::SpaceAround:
            return free_space / static_cast<float>(num_items);
        case AlignContent::SpaceEvenly:
            return free_space / static_cast<float>(num_items + 1);
        default:
            return 0.0f;
    }
}

//------------------------------------------------------------------------------
// Main Entry Point
//------------------------------------------------------------------------------

LayoutOutput ComputeFlexboxLayout(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    const auto& style = tree.GetFlexboxContainerStyle(node);
    
    // Pull these out earlier to avoid borrowing issues
    auto aspect_ratio = style.aspect_ratio;
    auto padding = ResolveOrZero(style.padding, inputs.parent_size.width);
    auto border = ResolveOrZero(style.border, inputs.parent_size.width);
    Size<float> padding_border_sum = {
        padding.left + padding.right + border.left + border.right,
        padding.top + padding.bottom + border.top + border.bottom
    };
    Size<float> box_sizing_adjustment =
        (style.box_sizing == BoxSizing::ContentBox) ? padding_border_sum : Size<float>::Zero();
    
    auto min_size = MaybeResolve(style.min_size, inputs.parent_size);
    min_size = MaybeApplyAspectRatio(min_size, aspect_ratio);
    min_size = MaybeAdd(min_size, box_sizing_adjustment);
    
    auto max_size = MaybeResolve(style.max_size, inputs.parent_size);
    max_size = MaybeApplyAspectRatio(max_size, aspect_ratio);
    max_size = MaybeAdd(max_size, box_sizing_adjustment);
    
    Size<std::optional<float>> clamped_style_size;
    if (inputs.sizing_mode == SizingMode::InherentSize) {
        clamped_style_size = MaybeResolve(style.size, inputs.parent_size);
        clamped_style_size = MaybeApplyAspectRatio(clamped_style_size, aspect_ratio);
        clamped_style_size = MaybeAdd(clamped_style_size, box_sizing_adjustment);
        clamped_style_size = MaybeClamp(clamped_style_size, min_size, max_size);
    } else {
        clamped_style_size = {std::nullopt, std::nullopt};
    }
    
    // If both min and max in a given axis are set and max <= min then this determines the size
    Size<std::optional<float>> min_max_definite_size = {
        (min_size.width.has_value() && max_size.width.has_value() && 
         *max_size.width <= *min_size.width) ? min_size.width : std::nullopt,
        (min_size.height.has_value() && max_size.height.has_value() && 
         *max_size.height <= *min_size.height) ? min_size.height : std::nullopt
    };
    
    // The size of the container should be floored by the padding and border
    auto styled_based_known_dimensions = inputs.known_dimensions;
    if (!styled_based_known_dimensions.width.has_value()) {
        if (min_max_definite_size.width.has_value()) {
            styled_based_known_dimensions.width = min_max_definite_size.width;
        } else if (clamped_style_size.width.has_value()) {
            styled_based_known_dimensions.width = 
                std::optional<float>(f32_max(*clamped_style_size.width, padding_border_sum.width));
        }
    }
    if (!styled_based_known_dimensions.height.has_value()) {
        if (min_max_definite_size.height.has_value()) {
            styled_based_known_dimensions.height = min_max_definite_size.height;
        } else if (clamped_style_size.height.has_value()) {
            styled_based_known_dimensions.height = 
                std::optional<float>(f32_max(*clamped_style_size.height, padding_border_sum.height));
        }
    }
    
    // Short-circuit layout if the container's size is fully determined
    if (inputs.run_mode == RunMode::ComputeSize) {
        if (styled_based_known_dimensions.width.has_value() && 
            styled_based_known_dimensions.height.has_value()) {
            return LayoutOutput::FromOuterSize(Size<float>{
                *styled_based_known_dimensions.width,
                *styled_based_known_dimensions.height
            });
        }
    }
    
    // Compute preliminary layout
    LayoutInput modified_inputs = inputs;
    modified_inputs.known_dimensions = styled_based_known_dimensions;
    
    return ComputePreliminary(tree, node, modified_inputs);
}

//------------------------------------------------------------------------------
// Preliminary Computation
//------------------------------------------------------------------------------

static LayoutOutput ComputePreliminary(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    const auto& style = tree.GetFlexboxContainerStyle(node);
    
    // Define some general constants we will need for the remainder of the algorithm
    auto constants = ComputeConstants(tree, style, inputs.known_dimensions, inputs.parent_size);
    
    // 9.1. Initial Setup
    // 1. Generate anonymous flex items
    auto flex_items = GenerateAnonymousFlexItems(tree, node, constants);
    
    // 9.2. Line Length Determination
    // 2. Determine the available main and cross space for the flex items
    auto available_space = DetermineAvailableSpace(inputs.known_dimensions, inputs.available_space, constants);
    
    // 3. Determine the flex base size and hypothetical main size of each item
    DetermineFlexBaseSize(tree, constants, available_space, flex_items);
    
    // 4. Determine the main size of the flex container (already done in compute_constants)
    
    // 9.3. Main Size Determination
    // 5. Collect flex items into flex lines
    auto flex_lines = CollectFlexLines(constants, available_space, flex_items);
    
    // If container size is undefined, determine the container's main size
    auto main_inner = constants.node_inner_size.Main(constants.dir);
    if (main_inner.has_value()) {
        float outer_main_size = *main_inner + RectMainAxisSum(constants.content_box_inset, constants.dir);
        constants.inner_container_size.SetMain(constants.dir, *main_inner);
        constants.container_size.SetMain(constants.dir, outer_main_size);
    } else {
        DetermineContainerMainSize(tree, available_space, flex_lines, flex_items, constants);
        constants.node_inner_size.SetMain(constants.dir, 
            std::optional<float>(constants.inner_container_size.Main(constants.dir)));
        constants.node_outer_size.SetMain(constants.dir, 
            std::optional<float>(constants.container_size.Main(constants.dir)));
        
        // Re-resolve percentage gaps
        float inner_container_size = constants.inner_container_size.Main(constants.dir);
        auto new_gap = MaybeResolve(style.gap.Main(constants.dir), std::optional<float>(inner_container_size));
        constants.gap.SetMain(constants.dir, new_gap.value_or(0.0f));
    }
    
    // 6. Resolve the flexible lengths of all the flex items
    for (auto& line : flex_lines) {
        ResolveFlexibleLengths(line, flex_items, constants);
    }
    
    // 9.4. Cross Size Determination
    // 7. Determine the hypothetical cross size of each item
    for (auto& line : flex_lines) {
        DetermineHypotheticalCrossSize(tree, line, flex_items, constants, available_space);
    }
    
    // Calculate child baselines
    CalculateChildrenBaseLines(tree, inputs.known_dimensions, available_space, flex_lines, flex_items, constants);
    
    // 8. Calculate the cross size of each flex line
    CalculateCrossSize(flex_lines, flex_items, inputs.known_dimensions, constants);
    
    // 9. Handle 'align-content: stretch'
    HandleAlignContentStretch(flex_lines, inputs.known_dimensions, constants);
    
    // 11. Determine the used cross size of each flex item
    DetermineUsedCrossSize(tree, flex_lines, flex_items, constants);
    
    // 9.5. Main-Axis Alignment
    // 12. Distribute any remaining free space
    DistributeRemainingFreeSpace(flex_lines, flex_items, constants);
    
    // 9.6. Cross-Axis Alignment
    // 13. Resolve cross-axis auto margins
    ResolveCrossAxisAutoMargins(flex_lines, flex_items, constants);
    
    // 15. Determine the flex container's used cross size
    float total_line_cross_size = DetermineContainerCrossSize(flex_lines, inputs.known_dimensions, constants);
    
    // If our caller does not care about performing layout we are done now
    if (inputs.run_mode == RunMode::ComputeSize) {
        return LayoutOutput::FromOuterSize(constants.container_size);
    }
    
    // 16. Align all flex lines per align-content
    AlignFlexLinesPerAlignContent(flex_lines, constants, total_line_cross_size);
    
    // Do a final layout pass and gather the resulting layouts
    auto inflow_content_size = FinalLayoutPass(tree, flex_lines, flex_items, constants);
    
    // Perform absolute layout on all absolutely positioned children
    auto absolute_content_size = PerformAbsoluteLayoutOnAbsoluteChildren(tree, node, constants);
    
    // Handle display:none children
    size_t len = tree.ChildCount(node);
    for (size_t order = 0; order < len; ++order) {
        NodeId child = tree.GetChildId(node, order);
        if (tree.GetFlexboxChildStyle(child).box_generation_mode == BoxGenerationMode::None) {
            Layout layout;
            layout.order = static_cast<uint32_t>(order);
            tree.SetUnroundedLayout(child, layout);
            tree.PerformChildLayout(
                child,
                Size<std::optional<float>>{std::nullopt, std::nullopt},
                Size<std::optional<float>>{std::nullopt, std::nullopt},
                Size<AvailableSpace>{AvailableSpace::MaxContent(), AvailableSpace::MaxContent()},
                SizingMode::InherentSize,
                LineBoolFalse()
            );
        }
    }
    
    // Calculate first baseline
    std::optional<float> first_vertical_baseline = std::nullopt;
    if (!flex_lines.empty()) {
        for (size_t i = flex_lines[0].start_index; i < flex_lines[0].end_index; ++i) {
            const auto& item = flex_items[i];
            if (constants.is_column || item.align_self == AlignSelf::Baseline) {
                first_vertical_baseline = item.baseline;
                break;
            }
        }
    }
    
    // Compute content size
    Size<float> content_size = {
        f32_max(inflow_content_size.width, absolute_content_size.width),
        f32_max(inflow_content_size.height, absolute_content_size.height)
    };
    
    LayoutOutput output;
    output.size = constants.container_size;
    output.content_size = content_size;
    output.first_baselines = first_vertical_baseline;
    return output;
}

//------------------------------------------------------------------------------
// Compute Constants
//------------------------------------------------------------------------------

static FlexAlgoConstants ComputeConstants(
    LayoutFlexboxContainer& tree,
    const FlexboxContainerStyle& style,
    Size<std::optional<float>> known_dimensions,
    Size<std::optional<float>> parent_size
) {
    FlexAlgoConstants constants;

    constants.dir = style.flex_direction;
    constants.is_row = IsRow(style.flex_direction);
    constants.is_column = IsColumn(style.flex_direction);
    constants.is_wrap = style.flex_wrap != FlexWrap::NoWrap;
    constants.is_wrap_reverse = style.flex_wrap == FlexWrap::WrapReverse;

    auto aspect_ratio = style.aspect_ratio;
    auto margin = ResolveOrZero(style.margin, parent_size.width);
    auto padding = ResolveOrZero(style.padding, parent_size.width);
    auto border = ResolveOrZero(style.border, parent_size.width);

    Size<float> padding_border_sum = {
        padding.left + padding.right + border.left + border.right,
        padding.top + padding.bottom + border.top + border.bottom
    };
    Size<float> box_sizing_adjustment =
        (style.box_sizing == BoxSizing::ContentBox) ? padding_border_sum : Size<float>::Zero();

    constants.min_size = MaybeResolve(style.min_size, parent_size);
    constants.min_size = MaybeApplyAspectRatio(constants.min_size, aspect_ratio);
    constants.min_size = MaybeAdd(constants.min_size, box_sizing_adjustment);

    constants.max_size = MaybeResolve(style.max_size, parent_size);
    constants.max_size = MaybeApplyAspectRatio(constants.max_size, aspect_ratio);
    constants.max_size = MaybeAdd(constants.max_size, box_sizing_adjustment);

    constants.margin = margin;
    constants.border = border;

    // Scrollbar gutter (simplified - no scrollbar support for now)
    constants.scrollbar_gutter = {0.0f, 0.0f};

    // Content box inset = padding + border + scrollbar_gutter
    constants.content_box_inset = {
        padding.left + border.left + constants.scrollbar_gutter.x,
        padding.right + border.right,
        padding.top + border.top + constants.scrollbar_gutter.y,
        padding.bottom + border.bottom
    };

    // Resolve gap
    constants.gap = {
        MaybeResolve(style.gap.width, parent_size.width).value_or(0.0f),
        MaybeResolve(style.gap.height, parent_size.height).value_or(0.0f)
    };

    constants.align_items = style.align_items;
    constants.align_content = style.align_content;
    constants.justify_content = style.justify_content;

    // Compute node outer size
    auto node_outer_size = known_dimensions;
    node_outer_size = MaybeClamp(node_outer_size, constants.min_size, constants.max_size);

    // Compute node inner size
    Size<std::optional<float>> node_inner_size = {
        node_outer_size.width.has_value()
            ? std::optional<float>(*node_outer_size.width - RectHorizontalAxisSum(constants.content_box_inset))
            : std::nullopt,
        node_outer_size.height.has_value()
            ? std::optional<float>(*node_outer_size.height - RectVerticalAxisSum(constants.content_box_inset))
            : std::nullopt
    };

    constants.node_outer_size = node_outer_size;
    constants.node_inner_size = node_inner_size;
    constants.container_size = Size<float>::Zero();
    constants.inner_container_size = Size<float>::Zero();

    return constants;
}

//------------------------------------------------------------------------------
// Generate Anonymous Flex Items
//------------------------------------------------------------------------------

static std::vector<FlexItem> GenerateAnonymousFlexItems(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const FlexAlgoConstants& constants
) {
    std::vector<FlexItem> items;
    size_t child_count = tree.ChildCount(node);
    items.reserve(child_count);

    for (size_t i = 0; i < child_count; ++i) {
        NodeId child = tree.GetChildId(node, i);
        const auto& child_style = tree.GetFlexboxChildStyle(child);

        // Skip display:none items
        if (child_style.box_generation_mode == BoxGenerationMode::None) {
            continue;
        }

        FlexItem item;
        item.node = child;
        item.order = static_cast<uint32_t>(child_style.order);

        auto aspect_ratio = child_style.aspect_ratio;
        auto padding = ResolveOrZero(child_style.padding, constants.node_inner_size.width);
        auto border = ResolveOrZero(child_style.border, constants.node_inner_size.width);

        Size<float> padding_border_sum = {
            padding.left + padding.right + border.left + border.right,
            padding.top + padding.bottom + border.top + border.bottom
        };
        Size<float> box_sizing_adjustment =
            (child_style.box_sizing == BoxSizing::ContentBox) ? padding_border_sum : Size<float>::Zero();

        item.size = MaybeResolve(child_style.size, constants.node_inner_size);
        item.size = MaybeApplyAspectRatio(item.size, aspect_ratio);
        item.size = MaybeAdd(item.size, box_sizing_adjustment);

        item.min_size = MaybeResolve(child_style.min_size, constants.node_inner_size);
        item.min_size = MaybeApplyAspectRatio(item.min_size, aspect_ratio);
        item.min_size = MaybeAdd(item.min_size, box_sizing_adjustment);

        item.max_size = MaybeResolve(child_style.max_size, constants.node_inner_size);
        item.max_size = MaybeApplyAspectRatio(item.max_size, aspect_ratio);
        item.max_size = MaybeAdd(item.max_size, box_sizing_adjustment);

        // Resolve align_self (use parent's align_items if not specified)
        item.align_self = child_style.align_self.value_or(static_cast<AlignSelf>(constants.align_items));

        item.overflow = child_style.overflow;
        item.scrollbar_width = child_style.scrollbar_width;
        item.flex_shrink = child_style.flex_shrink;
        item.flex_grow = child_style.flex_grow;

        // Resolve inset
        item.inset = MaybeResolve(child_style.inset, constants.node_inner_size.width);

        // Resolve margin
        auto margin = ResolveOrZero(child_style.margin, constants.node_inner_size.width);
        item.margin = margin;
        item.margin_is_auto = {
            child_style.margin.left.IsAuto(),
            child_style.margin.right.IsAuto(),
            child_style.margin.top.IsAuto(),
            child_style.margin.bottom.IsAuto()
        };

        item.padding = padding;
        item.border = border;

        // Initialize other fields
        item.flex_basis = 0.0f;
        item.inner_flex_basis = 0.0f;
        item.violation = 0.0f;
        item.frozen = false;
        item.content_flex_fraction = 0.0f;
        item.resolved_minimum_main_size = 0.0f;
        item.hypothetical_inner_size = Size<float>::Zero();
        item.hypothetical_outer_size = Size<float>::Zero();
        item.target_size = Size<float>::Zero();
        item.outer_target_size = Size<float>::Zero();
        item.baseline = 0.0f;
        item.offset_main = 0.0f;
        item.offset_cross = 0.0f;

        items.push_back(item);
    }

    // Sort items by CSS order property (stable sort to preserve DOM order for equal order values)
    std::stable_sort(items.begin(), items.end(), [](const FlexItem& a, const FlexItem& b) {
        return static_cast<int32_t>(a.order) < static_cast<int32_t>(b.order);
    });

    return items;
}

//------------------------------------------------------------------------------
// Determine Available Space
//------------------------------------------------------------------------------

static Size<AvailableSpace> DetermineAvailableSpace(
    Size<std::optional<float>> known_dimensions,
    Size<AvailableSpace> available_space,
    const FlexAlgoConstants& constants
) {
    // Main axis
    AvailableSpace main_available;
    auto main_known = known_dimensions.Main(constants.dir);
    if (main_known.has_value()) {
        float inner = *main_known - RectMainAxisSum(constants.content_box_inset, constants.dir);
        main_available = AvailableSpace::Definite(f32_max(inner, 0.0f));
    } else {
        main_available = available_space.Main(constants.dir);
    }

    // Cross axis
    AvailableSpace cross_available;
    auto cross_known = known_dimensions.Cross(constants.dir);
    if (cross_known.has_value()) {
        float inner = *cross_known - RectCrossAxisSum(constants.content_box_inset, constants.dir);
        cross_available = AvailableSpace::Definite(f32_max(inner, 0.0f));
    } else {
        cross_available = available_space.Cross(constants.dir);
    }

    Size<AvailableSpace> result;
    result.SetMain(constants.dir, main_available);
    result.SetCross(constants.dir, cross_available);
    return result;
}

//------------------------------------------------------------------------------
// Determine Flex Base Size
//------------------------------------------------------------------------------

static void DetermineFlexBaseSize(
    LayoutFlexboxContainer& tree,
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space,
    std::vector<FlexItem>& flex_items
) {
    for (auto& item : flex_items) {
        const auto& child_style = tree.GetFlexboxChildStyle(item.node);

        // Resolve flex_basis
        auto flex_basis = child_style.flex_basis;
        std::optional<float> resolved_flex_basis = std::nullopt;

        if (!flex_basis.IsAuto()) {
            resolved_flex_basis = MaybeResolve(flex_basis, constants.node_inner_size.Main(constants.dir));
        }

        // A. If the item has a definite used flex basis, that's the flex base size
        if (resolved_flex_basis.has_value()) {
            item.flex_basis = *resolved_flex_basis;
        }
        // B. If the flex item has an intrinsic aspect ratio, a used flex basis of content,
        //    and a definite cross size, then the flex base size is calculated from the cross size
        else if (child_style.aspect_ratio.has_value() && item.size.Cross(constants.dir).has_value()) {
            float cross = *item.size.Cross(constants.dir);
            item.flex_basis = constants.is_row
                ? cross * *child_style.aspect_ratio
                : cross / *child_style.aspect_ratio;
        }
        // C. If the used flex basis is content or depends on its available space
        else {
            // Measure the item
            auto child_available_space = available_space;

            // If we have a definite cross size, use it
            if (item.size.Cross(constants.dir).has_value()) {
                child_available_space.SetCross(constants.dir,
                    AvailableSpace::Definite(*item.size.Cross(constants.dir)));
            }

            auto measured_size = tree.MeasureChildSize(
                item.node,
                item.size,
                constants.node_inner_size,
                child_available_space,
                SizingMode::ContentSize
            );

            item.flex_basis = measured_size.Main(constants.dir);
        }

        // Compute padding + border sum for main axis
        float main_padding_border = RectMainAxisSum(item.padding, constants.dir) +
                                    RectMainAxisSum(item.border, constants.dir);

        // Floor flex-basis by the padding_border_sum (floors inner_flex_basis at zero)
        // This matches Chrome and Firefox's behaviour.
        // See: https://www.w3.org/TR/css-flexbox-1/#intrinsic-item-contributions
        item.flex_basis = f32_max(item.flex_basis, main_padding_border);

        // Note: flex_basis should NOT be clamped by min size here.
        // min constraints are applied during the flex algorithm's
        // "freeze" and "clamp" steps, not to the initial flex-basis.
        // max constraints can be applied here.
        // See: https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-lengths
        auto main_min = item.min_size.Main(constants.dir);
        auto main_max = item.max_size.Main(constants.dir);
        if (main_max.has_value()) {
            item.flex_basis = f32_min(item.flex_basis, *main_max);
        }

        // Compute inner_flex_basis (flex_basis minus padding and border)
        item.inner_flex_basis = f32_max(item.flex_basis - main_padding_border, 0.0f);

        // Compute hypothetical main size
        float hypothetical_inner_main = item.flex_basis;
        float hypothetical_outer_main = hypothetical_inner_main +
            item.margin.MainStart(constants.dir) + item.margin.MainEnd(constants.dir);

        item.hypothetical_inner_size.SetMain(constants.dir, hypothetical_inner_main);
        item.hypothetical_outer_size.SetMain(constants.dir, hypothetical_outer_main);

        // Compute resolved minimum main size
        if (main_min.has_value()) {
            item.resolved_minimum_main_size = *main_min;
        } else if (!item.IsScrollContainer()) {
            // Content-based minimum size (CSS min-width: auto)
            auto content_size = tree.MeasureChildSize(
                item.node,
                Size<std::optional<float>>{std::nullopt, std::nullopt},
                constants.node_inner_size,
                Size<AvailableSpace>{AvailableSpace::MinContent(), AvailableSpace::MinContent()},
                SizingMode::ContentSize
            );
            item.resolved_minimum_main_size = f32_min(content_size.Main(constants.dir), item.flex_basis);
        } else {
            item.resolved_minimum_main_size = 0.0f;
        }

        // Clamp by max size
        if (main_max.has_value()) {
            item.resolved_minimum_main_size = f32_min(item.resolved_minimum_main_size, *main_max);
        }
    }
}

//------------------------------------------------------------------------------
// Collect Flex Lines
//------------------------------------------------------------------------------

static std::vector<FlexLine> CollectFlexLines(
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space,
    std::vector<FlexItem>& flex_items
) {
    std::vector<FlexLine> lines;

    if (flex_items.empty()) {
        return lines;
    }

    // If not wrapping, all items go in one line
    if (!constants.is_wrap) {
        FlexLine line;
        line.start_index = 0;
        line.end_index = flex_items.size();
        line.cross_size = 0.0f;
        line.offset_cross = 0.0f;
        lines.push_back(line);
        return lines;
    }

    // Get available main space
    float available_main = available_space.Main(constants.dir).IntoOption().value_or(INFINITY);

    size_t line_start = 0;
    float line_main_size = 0.0f;

    for (size_t i = 0; i < flex_items.size(); ++i) {
        const auto& item = flex_items[i];
        float item_main_size = item.hypothetical_outer_size.Main(constants.dir);

        // Check if we need to start a new line
        bool should_break = false;
        if (i > line_start) {
            float gap = constants.gap.Main(constants.dir);
            if (line_main_size + gap + item_main_size > available_main) {
                should_break = true;
            }
        }

        if (should_break) {
            // End current line
            FlexLine line;
            line.start_index = line_start;
            line.end_index = i;
            line.cross_size = 0.0f;
            line.offset_cross = 0.0f;
            lines.push_back(line);

            // Start new line
            line_start = i;
            line_main_size = item_main_size;
        } else {
            if (i > line_start) {
                line_main_size += constants.gap.Main(constants.dir);
            }
            line_main_size += item_main_size;
        }
    }

    // Add final line
    FlexLine line;
    line.start_index = line_start;
    line.end_index = flex_items.size();
    line.cross_size = 0.0f;
    line.offset_cross = 0.0f;
    lines.push_back(line);

    return lines;
}

//------------------------------------------------------------------------------
// Determine Container Main Size
//------------------------------------------------------------------------------

static void DetermineContainerMainSize(
    LayoutFlexboxContainer& tree,
    Size<AvailableSpace> available_space,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    FlexAlgoConstants& constants
) {
    float longest_line_length = 0.0f;

    for (const auto& line : flex_lines) {
        float line_length = 0.0f;
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            if (i > line.start_index) {
                line_length += constants.gap.Main(constants.dir);
            }
            line_length += flex_items[i].hypothetical_outer_size.Main(constants.dir);
        }
        longest_line_length = f32_max(longest_line_length, line_length);
    }

    // Clamp by min/max size
    auto main_min = constants.min_size.Main(constants.dir);
    auto main_max = constants.max_size.Main(constants.dir);

    float content_box_inset = RectMainAxisSum(constants.content_box_inset, constants.dir);
    float outer_main_size = longest_line_length + content_box_inset;

    outer_main_size = f32_clamp(
        outer_main_size,
        main_min.value_or(0.0f),
        main_max.value_or(INFINITY)
    );

    // Also clamp by available space
    auto available_main = available_space.Main(constants.dir).IntoOption();
    if (available_main.has_value()) {
        outer_main_size = f32_min(outer_main_size, *available_main);
    }

    float inner_main_size = outer_main_size - content_box_inset;

    constants.container_size.SetMain(constants.dir, outer_main_size);
    constants.inner_container_size.SetMain(constants.dir, inner_main_size);
}

//------------------------------------------------------------------------------
// Resolve Flexible Lengths
//------------------------------------------------------------------------------

static void ResolveFlexibleLengths(
    FlexLine& line,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    // Calculate total flex grow and flex shrink
    float total_flex_grow = 0.0f;
    float total_flex_shrink = 0.0f;
    float total_hypothetical_main_size = 0.0f;

    for (size_t i = line.start_index; i < line.end_index; ++i) {
        auto& item = flex_items[i];
        total_flex_grow += item.flex_grow;
        total_flex_shrink += item.flex_shrink * item.inner_flex_basis;
        total_hypothetical_main_size += item.hypothetical_outer_size.Main(constants.dir);
        if (i > line.start_index) {
            total_hypothetical_main_size += constants.gap.Main(constants.dir);
        }
    }

    float inner_main_size = constants.inner_container_size.Main(constants.dir);
    float free_space = inner_main_size - total_hypothetical_main_size;

    // Determine if we're growing or shrinking
    bool growing = free_space > 0.0f;

    // Initialize items
    for (size_t i = line.start_index; i < line.end_index; ++i) {
        auto& item = flex_items[i];
        item.target_size.SetMain(constants.dir, item.hypothetical_inner_size.Main(constants.dir));
        item.frozen = (growing && item.flex_grow == 0.0f) || (!growing && item.flex_shrink == 0.0f);
    }

    // Iterate until all items are frozen
    for (int iteration = 0; iteration < 10; ++iteration) {
        // Calculate remaining free space
        float used_space = 0.0f;
        float unfrozen_flex_factor = 0.0f;

        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];
            if (i > line.start_index) {
                used_space += constants.gap.Main(constants.dir);
            }

            if (item.frozen) {
                used_space += item.target_size.Main(constants.dir) +
                    item.margin.MainStart(constants.dir) + item.margin.MainEnd(constants.dir);
            } else {
                used_space += item.flex_basis +
                    item.margin.MainStart(constants.dir) + item.margin.MainEnd(constants.dir);
                if (growing) {
                    unfrozen_flex_factor += item.flex_grow;
                } else {
                    unfrozen_flex_factor += item.flex_shrink * item.inner_flex_basis;
                }
            }
        }

        float remaining_free_space = inner_main_size - used_space;

        // Check if all items are frozen
        bool all_frozen = true;
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            if (!flex_items[i].frozen) {
                all_frozen = false;
                break;
            }
        }
        if (all_frozen) break;

        // Distribute free space
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];
            if (item.frozen) continue;

            float ratio;
            if (growing) {
                ratio = (unfrozen_flex_factor > 0.0f) ? item.flex_grow / unfrozen_flex_factor : 0.0f;
            } else {
                ratio = (unfrozen_flex_factor > 0.0f)
                    ? (item.flex_shrink * item.inner_flex_basis) / unfrozen_flex_factor
                    : 0.0f;
            }

            float adjustment = remaining_free_space * ratio;
            float new_main_size = item.flex_basis + adjustment;

            // Clamp by min/max
            auto main_min = item.min_size.Main(constants.dir);
            auto main_max = item.max_size.Main(constants.dir);
            float clamped = f32_clamp(
                new_main_size,
                f32_max(main_min.value_or(0.0f), item.resolved_minimum_main_size),
                main_max.value_or(INFINITY)
            );

            item.violation = clamped - new_main_size;
            item.target_size.SetMain(constants.dir, clamped);
        }

        // Freeze items with violations
        float total_violation = 0.0f;
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            total_violation += flex_items[i].violation;
        }

        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];
            if (item.frozen) continue;

            if (total_violation > 0.0f && item.violation > 0.0f) {
                item.frozen = true;
            } else if (total_violation < 0.0f && item.violation < 0.0f) {
                item.frozen = true;
            } else if (total_violation == 0.0f) {
                item.frozen = true;
            }
        }
    }

    // Set outer target size
    for (size_t i = line.start_index; i < line.end_index; ++i) {
        auto& item = flex_items[i];
        float main_margin = item.margin.MainStart(constants.dir) + item.margin.MainEnd(constants.dir);
        item.outer_target_size.SetMain(constants.dir, item.target_size.Main(constants.dir) + main_margin);
    }
}

//------------------------------------------------------------------------------
// Determine Hypothetical Cross Size
//------------------------------------------------------------------------------

static void DetermineHypotheticalCrossSize(
    LayoutFlexboxContainer& tree,
    FlexLine& line,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants,
    Size<AvailableSpace> available_space
) {
    for (size_t i = line.start_index; i < line.end_index; ++i) {
        auto& item = flex_items[i];

        // If cross size is already known, use it
        auto cross_size = item.size.Cross(constants.dir);
        if (cross_size.has_value()) {
            item.hypothetical_inner_size.SetCross(constants.dir, *cross_size);
            float cross_margin = item.margin.CrossStart(constants.dir) + item.margin.CrossEnd(constants.dir);
            item.hypothetical_outer_size.SetCross(constants.dir, *cross_size + cross_margin);
            continue;
        }

        // Measure the item with known main size
        Size<std::optional<float>> known_dimensions = item.size;
        known_dimensions.SetMain(constants.dir, std::optional<float>(item.target_size.Main(constants.dir)));

        auto measured_size = tree.MeasureChildSize(
            item.node,
            known_dimensions,
            constants.node_inner_size,
            available_space,
            SizingMode::ContentSize
        );

        // Clamp by min/max
        auto cross_min = item.min_size.Cross(constants.dir);
        auto cross_max = item.max_size.Cross(constants.dir);
        float clamped_cross = f32_clamp(
            measured_size.Cross(constants.dir),
            cross_min.value_or(0.0f),
            cross_max.value_or(INFINITY)
        );

        item.hypothetical_inner_size.SetCross(constants.dir, clamped_cross);
        float cross_margin = item.margin.CrossStart(constants.dir) + item.margin.CrossEnd(constants.dir);
        item.hypothetical_outer_size.SetCross(constants.dir, clamped_cross + cross_margin);
    }
}

//------------------------------------------------------------------------------
// Calculate Children Base Lines
//------------------------------------------------------------------------------

static void CalculateChildrenBaseLines(
    LayoutFlexboxContainer& tree,
    Size<std::optional<float>> known_dimensions,
    Size<AvailableSpace> available_space,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    // Check if we need baselines
    bool needs_baseline = false;
    for (const auto& line : flex_lines) {
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            if (flex_items[i].align_self == AlignSelf::Baseline) {
                needs_baseline = true;
                break;
            }
        }
        if (needs_baseline) break;
    }

    if (!needs_baseline && !constants.is_column) {
        return;
    }

    // Calculate baselines for items that need them
    for (auto& line : flex_lines) {
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];

            if (item.align_self != AlignSelf::Baseline && !constants.is_column) {
                continue;
            }

            // Perform layout to get baseline
            Size<std::optional<float>> child_known = item.size;
            child_known.SetMain(constants.dir, std::optional<float>(item.target_size.Main(constants.dir)));
            child_known.SetCross(constants.dir, std::optional<float>(item.hypothetical_inner_size.Cross(constants.dir)));

            auto layout_output = tree.PerformChildLayout(
                item.node,
                child_known,
                constants.node_inner_size,
                available_space,
                SizingMode::ContentSize,
                LineBoolFalse()
            );

            // Get baseline from layout output
            if (layout_output.first_baselines.has_value()) {
                item.baseline = *layout_output.first_baselines;
            } else {
                // Use bottom of content box as baseline
                item.baseline = item.hypothetical_inner_size.Cross(constants.dir);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Calculate Cross Size
//------------------------------------------------------------------------------

static void CalculateCrossSize(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    Size<std::optional<float>> known_dimensions,
    const FlexAlgoConstants& constants
) {
    for (auto& line : flex_lines) {
        float max_cross_size = 0.0f;

        for (size_t i = line.start_index; i < line.end_index; ++i) {
            const auto& item = flex_items[i];
            max_cross_size = f32_max(max_cross_size, item.hypothetical_outer_size.Cross(constants.dir));
        }

        line.cross_size = max_cross_size;
    }

    // If single line and cross size is known, use it
    if (flex_lines.size() == 1 && known_dimensions.Cross(constants.dir).has_value()) {
        float known_cross = *known_dimensions.Cross(constants.dir);
        float cross_inset = RectCrossAxisSum(constants.content_box_inset, constants.dir);
        flex_lines[0].cross_size = f32_max(known_cross - cross_inset, 0.0f);
    }
}

//------------------------------------------------------------------------------
// Handle Align Content Stretch
//------------------------------------------------------------------------------

static void HandleAlignContentStretch(
    std::vector<FlexLine>& flex_lines,
    Size<std::optional<float>> known_dimensions,
    const FlexAlgoConstants& constants
) {
    if (constants.align_content != AlignContent::Stretch) {
        return;
    }

    auto cross_known = known_dimensions.Cross(constants.dir);
    if (!cross_known.has_value()) {
        return;
    }

    float cross_inset = RectCrossAxisSum(constants.content_box_inset, constants.dir);
    float available_cross = *cross_known - cross_inset;

    // Calculate total line cross size
    float total_cross = 0.0f;
    for (const auto& line : flex_lines) {
        total_cross += line.cross_size;
    }
    total_cross += constants.gap.Cross(constants.dir) * static_cast<float>(flex_lines.size() - 1);

    if (total_cross >= available_cross) {
        return;
    }

    // Distribute extra space
    float extra_space = available_cross - total_cross;
    float extra_per_line = extra_space / static_cast<float>(flex_lines.size());

    for (auto& line : flex_lines) {
        line.cross_size += extra_per_line;
    }
}

//------------------------------------------------------------------------------
// Determine Used Cross Size
//------------------------------------------------------------------------------

static void DetermineUsedCrossSize(
    LayoutFlexboxContainer& tree,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    for (auto& line : flex_lines) {
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];

            // If align-self is stretch and cross size is auto
            if (item.align_self == AlignSelf::Stretch && !item.size.Cross(constants.dir).has_value()) {
                float cross_margin = item.margin.CrossStart(constants.dir) + item.margin.CrossEnd(constants.dir);
                float stretched_cross = f32_max(line.cross_size - cross_margin, 0.0f);

                // Clamp by min/max
                auto cross_min = item.min_size.Cross(constants.dir);
                auto cross_max = item.max_size.Cross(constants.dir);
                stretched_cross = f32_clamp(
                    stretched_cross,
                    cross_min.value_or(0.0f),
                    cross_max.value_or(INFINITY)
                );

                item.target_size.SetCross(constants.dir, stretched_cross);
            } else {
                item.target_size.SetCross(constants.dir, item.hypothetical_inner_size.Cross(constants.dir));
            }

            float cross_margin = item.margin.CrossStart(constants.dir) + item.margin.CrossEnd(constants.dir);
            item.outer_target_size.SetCross(constants.dir, item.target_size.Cross(constants.dir) + cross_margin);
        }
    }
}

//------------------------------------------------------------------------------
// Distribute Remaining Free Space
//------------------------------------------------------------------------------

static void DistributeRemainingFreeSpace(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    bool layout_reverse = IsReverse(constants.dir);

    for (auto& line : flex_lines) {
        // Calculate used main space
        float used_space = 0.0f;
        size_t num_auto_margins = 0;

        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];
            if (i > line.start_index) {
                used_space += constants.gap.Main(constants.dir);
            }
            used_space += item.outer_target_size.Main(constants.dir);

            if (item.margin_is_auto.MainStart(constants.dir)) num_auto_margins++;
            if (item.margin_is_auto.MainEnd(constants.dir)) num_auto_margins++;
        }

        float free_space = constants.inner_container_size.Main(constants.dir) - used_space;

        // Distribute to auto margins first
        if (num_auto_margins > 0 && free_space > 0.0f) {
            float margin_per_auto = free_space / static_cast<float>(num_auto_margins);
            for (size_t i = line.start_index; i < line.end_index; ++i) {
                auto& item = flex_items[i];
                if (item.margin_is_auto.MainStart(constants.dir)) {
                    item.margin.SetMainStart(constants.dir, margin_per_auto);
                }
                if (item.margin_is_auto.MainEnd(constants.dir)) {
                    item.margin.SetMainEnd(constants.dir, margin_per_auto);
                }
            }
            free_space = 0.0f;
        }

        // Apply justify-content with reverse support
        size_t num_items = line.end_index - line.start_index;
        JustifyContent raw_justify = constants.justify_content.value_or(JustifyContent::FlexStart);
        float gap = constants.gap.Main(constants.dir);

        // Apply alignment fallback for negative free space
        bool is_safe = false; // TODO: Implement safe alignment
        JustifyContent justify = ApplyAlignmentFallback(free_space, num_items, raw_justify, is_safe);

        // Set item offsets - iterate in reverse order if layout_reverse
        if (layout_reverse) {
            // Reverse iteration: enumerate from end to start
            for (size_t idx = 0; idx < num_items; ++idx) {
                size_t i = line.end_index - 1 - idx;
                auto& item = flex_items[i];
                item.offset_main = ComputeAlignmentOffset(free_space, num_items, gap, justify, layout_reverse, idx == 0);
            }
        } else {
            // Normal iteration
            for (size_t idx = 0; idx < num_items; ++idx) {
                size_t i = line.start_index + idx;
                auto& item = flex_items[i];
                item.offset_main = ComputeAlignmentOffset(free_space, num_items, gap, justify, layout_reverse, idx == 0);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Resolve Cross Axis Auto Margins
//------------------------------------------------------------------------------

static void ResolveCrossAxisAutoMargins(
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    for (auto& line : flex_lines) {
        // First pass: find the maximum baseline for baseline-aligned items in this line
        float max_baseline = 0.0f;
        bool has_baseline_items = false;
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            const auto& item = flex_items[i];
            if (item.align_self == AlignSelf::Baseline) {
                has_baseline_items = true;
                // The baseline is measured from the top of the item's content box
                // We need to add the cross-start margin to get the baseline from the line start
                float item_baseline = item.margin.CrossStart(constants.dir) + item.baseline;
                max_baseline = f32_max(max_baseline, item_baseline);
            }
        }

        // Second pass: apply alignment
        for (size_t i = line.start_index; i < line.end_index; ++i) {
            auto& item = flex_items[i];

            float free_space = line.cross_size - item.outer_target_size.Cross(constants.dir);

            bool start_auto = item.margin_is_auto.CrossStart(constants.dir);
            bool end_auto = item.margin_is_auto.CrossEnd(constants.dir);

            if (start_auto && end_auto) {
                item.margin.SetCrossStart(constants.dir, free_space / 2.0f);
                item.margin.SetCrossEnd(constants.dir, free_space / 2.0f);
            } else if (start_auto) {
                item.margin.SetCrossStart(constants.dir, free_space);
            } else if (end_auto) {
                item.margin.SetCrossEnd(constants.dir, free_space);
            } else {
                // Apply align-self
                switch (item.align_self) {
                    case AlignSelf::FlexStart:
                    case AlignSelf::Start:
                        item.offset_cross = 0.0f;
                        break;
                    case AlignSelf::FlexEnd:
                    case AlignSelf::End:
                        item.offset_cross = free_space;
                        break;
                    case AlignSelf::Center:
                        item.offset_cross = free_space / 2.0f;
                        break;
                    case AlignSelf::Baseline:
                        if (has_baseline_items) {
                            // Align this item's baseline with the max baseline
                            float item_baseline = item.margin.CrossStart(constants.dir) + item.baseline;
                            item.offset_cross = max_baseline - item_baseline;
                        } else {
                            item.offset_cross = 0.0f;
                        }
                        break;
                    case AlignSelf::Stretch:
                    default:
                        item.offset_cross = 0.0f;
                        break;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Determine Container Cross Size
//------------------------------------------------------------------------------

static float DetermineContainerCrossSize(
    const std::vector<FlexLine>& flex_lines,
    Size<std::optional<float>> known_dimensions,
    FlexAlgoConstants& constants
) {
    float total_cross = 0.0f;
    for (size_t i = 0; i < flex_lines.size(); ++i) {
        if (i > 0) {
            total_cross += constants.gap.Cross(constants.dir);
        }
        total_cross += flex_lines[i].cross_size;
    }

    float cross_inset = RectCrossAxisSum(constants.content_box_inset, constants.dir);
    float outer_cross = total_cross + cross_inset;

    // Clamp by min/max
    auto cross_min = constants.min_size.Cross(constants.dir);
    auto cross_max = constants.max_size.Cross(constants.dir);
    outer_cross = f32_clamp(
        outer_cross,
        cross_min.value_or(0.0f),
        cross_max.value_or(INFINITY)
    );

    // Use known dimension if available
    auto cross_known = known_dimensions.Cross(constants.dir);
    if (cross_known.has_value()) {
        outer_cross = *cross_known;
    }

    constants.container_size.SetCross(constants.dir, outer_cross);
    constants.inner_container_size.SetCross(constants.dir, outer_cross - cross_inset);

    return total_cross;
}

//------------------------------------------------------------------------------
// Align Flex Lines Per Align Content
//------------------------------------------------------------------------------

static void AlignFlexLinesPerAlignContent(
    std::vector<FlexLine>& flex_lines,
    const FlexAlgoConstants& constants,
    float total_line_cross_size
) {
    float inner_cross = constants.inner_container_size.Cross(constants.dir);
    float free_space = inner_cross - total_line_cross_size;

    size_t num_lines = flex_lines.size();

    float initial_offset = 0.0f;
    float gap_between = constants.gap.Cross(constants.dir);

    switch (constants.align_content) {
        case AlignContent::FlexStart:
        case AlignContent::Start:
            initial_offset = 0.0f;
            break;
        case AlignContent::FlexEnd:
        case AlignContent::End:
            initial_offset = free_space;
            break;
        case AlignContent::Center:
            initial_offset = free_space / 2.0f;
            break;
        case AlignContent::Stretch:
            initial_offset = 0.0f;
            break;
        case AlignContent::SpaceBetween:
            if (num_lines > 1) {
                gap_between += free_space / static_cast<float>(num_lines - 1);
            }
            break;
        case AlignContent::SpaceAround:
            if (num_lines > 0) {
                float space = free_space / static_cast<float>(num_lines);
                initial_offset = space / 2.0f;
                gap_between += space;
            }
            break;
        case AlignContent::SpaceEvenly:
            if (num_lines > 0) {
                float space = free_space / static_cast<float>(num_lines + 1);
                initial_offset = space;
                gap_between += space;
            }
            break;
    }

    // Handle wrap-reverse
    if (constants.is_wrap_reverse) {
        initial_offset = inner_cross - initial_offset;
    }

    float offset = initial_offset;
    for (size_t i = 0; i < flex_lines.size(); ++i) {
        auto& line = flex_lines[i];
        if (constants.is_wrap_reverse) {
            line.offset_cross = offset - line.cross_size;
            offset -= line.cross_size + gap_between;
        } else {
            line.offset_cross = offset;
            offset += line.cross_size + gap_between;
        }
    }
}

//------------------------------------------------------------------------------
// Final Layout Pass
//------------------------------------------------------------------------------

// Helper function to calculate layout for a single flex item
static void CalculateFlexItem(
    LayoutFlexboxContainer& tree,
    FlexItem& item,
    float& total_offset_main,
    float total_offset_cross,
    float line_offset_cross,
    Size<float>& content_size,
    const FlexAlgoConstants& constants
) {
    // Perform final layout
    Size<std::optional<float>> known_dimensions = {
        std::optional<float>(item.target_size.width),
        std::optional<float>(item.target_size.height)
    };

    auto layout_output = tree.PerformChildLayout(
        item.node,
        known_dimensions,
        constants.node_inner_size,
        Size<AvailableSpace>{
            AvailableSpace::Definite(item.target_size.width),
            AvailableSpace::Definite(item.target_size.height)
        },
        SizingMode::InherentSize,
        LineBoolFalse()
    );

    // Compute position
    float offset_main = total_offset_main + item.offset_main + item.margin.MainStart(constants.dir);
    float offset_cross = total_offset_cross + item.offset_cross + line_offset_cross + item.margin.CrossStart(constants.dir);

    // Handle relative positioning (inset)
    if (item.inset.MainStart(constants.dir).has_value()) {
        offset_main += *item.inset.MainStart(constants.dir);
    } else if (item.inset.MainEnd(constants.dir).has_value()) {
        offset_main -= *item.inset.MainEnd(constants.dir);
    }
    if (item.inset.CrossStart(constants.dir).has_value()) {
        offset_cross += *item.inset.CrossStart(constants.dir);
    } else if (item.inset.CrossEnd(constants.dir).has_value()) {
        offset_cross -= *item.inset.CrossEnd(constants.dir);
    }

    Point<float> location;
    if (constants.is_row) {
        location.x = offset_main;
        location.y = offset_cross;
    } else {
        location.x = offset_cross;
        location.y = offset_main;
    }

    // Set layout
    Layout layout;
    layout.order = item.order;
    layout.size = layout_output.size;
    layout.content_size = layout_output.content_size;
    layout.location = location;
    layout.padding = item.padding;
    layout.border = item.border;
    tree.SetUnroundedLayout(item.node, layout);

    // Update total_offset_main for next item
    total_offset_main += item.offset_main + RectMainAxisSum(item.margin, constants.dir) + layout_output.size.Main(constants.dir);

    // Update content size
    float right = location.x + layout_output.size.width;
    float bottom = location.y + layout_output.size.height;
    content_size.width = f32_max(content_size.width, right);
    content_size.height = f32_max(content_size.height, bottom);
}

static Size<float> FinalLayoutPass(
    LayoutFlexboxContainer& tree,
    std::vector<FlexLine>& flex_lines,
    std::vector<FlexItem>& flex_items,
    const FlexAlgoConstants& constants
) {
    Size<float> content_size = Size<float>::Zero();
    bool layout_reverse = IsReverse(constants.dir);
    // content_box_inset.CrossStart is the padding/border offset from container edge to content area
    float container_cross_start = constants.content_box_inset.CrossStart(constants.dir);

    for (auto& line : flex_lines) {
        float total_offset_main = constants.content_box_inset.MainStart(constants.dir);
        // line.offset_cross is already an absolute offset from content area start (calculated by AlignFlexLinesPerAlignContent)
        // So total_offset_cross = container_cross_start + line.offset_cross
        float total_offset_cross = container_cross_start + line.offset_cross;

        if (layout_reverse) {
            // Reverse iteration for row-reverse/column-reverse
            for (size_t idx = 0; idx < (line.end_index - line.start_index); ++idx) {
                size_t i = line.end_index - 1 - idx;
                auto& item = flex_items[i];
                // Pass 0.0f for line_offset_cross since it's already included in total_offset_cross
                CalculateFlexItem(tree, item, total_offset_main, total_offset_cross, 0.0f, content_size, constants);
            }
        } else {
            // Normal iteration
            for (size_t i = line.start_index; i < line.end_index; ++i) {
                auto& item = flex_items[i];
                // Pass 0.0f for line_offset_cross since it's already included in total_offset_cross
                CalculateFlexItem(tree, item, total_offset_main, total_offset_cross, 0.0f, content_size, constants);
            }
        }
    }

    return content_size;
}

//------------------------------------------------------------------------------
// Perform Absolute Layout On Absolute Children
//------------------------------------------------------------------------------

static Size<float> PerformAbsoluteLayoutOnAbsoluteChildren(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const FlexAlgoConstants& constants
) {
    Size<float> content_size = Size<float>::Zero();

    size_t child_count = tree.ChildCount(node);
    for (size_t i = 0; i < child_count; ++i) {
        NodeId child = tree.GetChildId(node, i);
        const auto& child_style = tree.GetFlexboxChildStyle(child);

        // Skip non-absolute/fixed children
        if (child_style.position != Position::Absolute && child_style.position != Position::Fixed) {
            continue;
        }

        // Skip display:none
        if (child_style.box_generation_mode == BoxGenerationMode::None) {
            continue;
        }

        auto aspect_ratio = child_style.aspect_ratio;
        auto padding = ResolveOrZero(child_style.padding, constants.node_inner_size.width);
        auto border = ResolveOrZero(child_style.border, constants.node_inner_size.width);

        Size<float> padding_border_sum = {
            padding.left + padding.right + border.left + border.right,
            padding.top + padding.bottom + border.top + border.bottom
        };
        Size<float> box_sizing_adjustment =
            (child_style.box_sizing == BoxSizing::ContentBox) ? padding_border_sum : Size<float>::Zero();

        // Resolve inset
        auto inset = MaybeResolve(child_style.inset, constants.node_inner_size.width);

        // Resolve margin
        auto margin = ResolveOrZero(child_style.margin, constants.node_inner_size.width);

        // Resolve size
        auto style_size = MaybeResolve(child_style.size, constants.node_inner_size);
        style_size = MaybeApplyAspectRatio(style_size, aspect_ratio);
        style_size = MaybeAdd(style_size, box_sizing_adjustment);

        auto min_size = MaybeResolve(child_style.min_size, constants.node_inner_size);
        min_size = MaybeApplyAspectRatio(min_size, aspect_ratio);
        min_size = MaybeAdd(min_size, box_sizing_adjustment);

        auto max_size = MaybeResolve(child_style.max_size, constants.node_inner_size);
        max_size = MaybeApplyAspectRatio(max_size, aspect_ratio);
        max_size = MaybeAdd(max_size, box_sizing_adjustment);

        auto known_dimensions = MaybeClamp(style_size, min_size, max_size);

        // Fill in width from left/right
        if (!known_dimensions.width.has_value() && inset.left.has_value() && inset.right.has_value()) {
            float new_width = constants.inner_container_size.width - *inset.left - *inset.right -
                             margin.left - margin.right;
            known_dimensions.width = std::optional<float>(f32_max(new_width, 0.0f));
            known_dimensions = MaybeApplyAspectRatio(known_dimensions, aspect_ratio);
            known_dimensions = MaybeClamp(known_dimensions, min_size, max_size);
        }

        // Fill in height from top/bottom
        if (!known_dimensions.height.has_value() && inset.top.has_value() && inset.bottom.has_value()) {
            float new_height = constants.inner_container_size.height - *inset.top - *inset.bottom -
                              margin.top - margin.bottom;
            known_dimensions.height = std::optional<float>(f32_max(new_height, 0.0f));
            known_dimensions = MaybeApplyAspectRatio(known_dimensions, aspect_ratio);
            known_dimensions = MaybeClamp(known_dimensions, min_size, max_size);
        }

        // Measure child
        auto layout_output = tree.PerformChildLayout(
            child,
            known_dimensions,
            constants.node_inner_size,
            Size<AvailableSpace>{
                AvailableSpace::Definite(constants.inner_container_size.width),
                AvailableSpace::Definite(constants.inner_container_size.height)
            },
            SizingMode::ContentSize,
            LineBoolFalse()
        );

        auto final_size = Clamp(layout_output.size, min_size, max_size);

        // Compute location
        Point<float> location;

        // X position
        if (inset.left.has_value()) {
            location.x = constants.content_box_inset.left + *inset.left + margin.left;
        } else if (inset.right.has_value()) {
            location.x = constants.container_size.width - constants.content_box_inset.right -
                        *inset.right - margin.right - final_size.width;
        } else {
            location.x = constants.content_box_inset.left + margin.left;
        }

        // Y position
        if (inset.top.has_value()) {
            location.y = constants.content_box_inset.top + *inset.top + margin.top;
        } else if (inset.bottom.has_value()) {
            location.y = constants.container_size.height - constants.content_box_inset.bottom -
                        *inset.bottom - margin.bottom - final_size.height;
        } else {
            location.y = constants.content_box_inset.top + margin.top;
        }

        // Set layout
        Layout layout;
        layout.order = static_cast<uint32_t>(i);
        layout.size = final_size;
        layout.content_size = layout_output.content_size;
        layout.location = location;
        layout.padding = padding;
        layout.border = border;
        tree.SetUnroundedLayout(child, layout);

        // Update content size
        float right = location.x + final_size.width;
        float bottom = location.y + final_size.height;
        content_size.width = f32_max(content_size.width, right);
        content_size.height = f32_max(content_size.height, bottom);
    }

    return content_size;
}

} // namespace lightui

