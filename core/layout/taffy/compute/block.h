/**
 * @file block.h
 * @brief Block layout algorithm
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/block.rs
 */

#pragma once

#include "../geometry.h"
#include "../style.h"
#include "../layout.h"
#include "../tree/traits.h"
#include "../util/math.h"
#include "../util/resolve.h"
#include <vector>

namespace lightui {

//------------------------------------------------------------------------------
// Block Item
//------------------------------------------------------------------------------

/// Per-child data accumulated during block layout
struct BlockItem {
    /// Node identifier
    NodeId node_id;
    
    /// Source order
    uint32_t order;
    
    /// Whether this is a table element
    bool is_table;
    
    /// Base size
    Size<std::optional<float>> size;
    /// Minimum size
    Size<std::optional<float>> min_size;
    /// Maximum size
    Size<std::optional<float>> max_size;
    
    /// Overflow style
    Point<Overflow> overflow;
    /// Scrollbar width
    float scrollbar_width;
    
    /// Position style
    Position position;
    /// Inset values
    Rect<LengthPercentageAuto> inset;
    /// Margin values
    Rect<LengthPercentageAuto> margin;
    /// Resolved padding
    Rect<float> padding;
    /// Resolved border
    Rect<float> border;
    /// Sum of padding and border
    Size<float> padding_border_sum;
    
    /// Computed size (filled during layout)
    Size<float> computed_size;
    /// Static position (filled during layout)
    Point<float> static_position;
    /// Whether margins can collapse through
    bool can_be_collapsed_through;
};

//------------------------------------------------------------------------------
// Block Layout Functions
//------------------------------------------------------------------------------

/// Compute block layout for a node
LayoutOutput ComputeBlockLayout(
    LayoutBlockContainer& tree,
    NodeId node_id,
    const LayoutInput& inputs
);

/// Inner block layout computation
LayoutOutput ComputeBlockLayoutInner(
    LayoutBlockContainer& tree,
    NodeId node_id,
    const LayoutInput& inputs
);

/// Generate list of block items from children
std::vector<BlockItem> GenerateItemList(
    LayoutBlockContainer& tree,
    NodeId node,
    Size<std::optional<float>> node_inner_size
);

/// Determine content-based container width
float DetermineContentBasedContainerWidth(
    LayoutBlockContainer& tree,
    const std::vector<BlockItem>& items,
    AvailableSpace available_width
);

/// Perform final layout on in-flow children
/// Returns: (inflow_content_size, intrinsic_height, first_margin_set, last_margin_set)
std::tuple<Size<float>, float, CollapsibleMarginSet, CollapsibleMarginSet>
PerformFinalLayoutOnInFlowChildren(
    LayoutBlockContainer& tree,
    std::vector<BlockItem>& items,
    float container_outer_width,
    Rect<float> content_box_inset,
    Rect<float> resolved_content_box_inset,
    BlockTextAlign text_align,
    Line<bool> own_margins_collapse_with_children
);

/// Perform absolute layout on absolutely positioned children
Size<float> PerformAbsoluteLayoutOnAbsoluteChildren(
    LayoutBlockContainer& tree,
    const std::vector<BlockItem>& items,
    Size<float> area_size,
    Point<float> area_offset
);

} // namespace lightui

