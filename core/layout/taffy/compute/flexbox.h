/**
 * @file flexbox.h
 * @brief Flexbox layout algorithm
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/flexbox.rs
 * 
 * Computes the flexbox layout algorithm according to the CSS Flexbox spec:
 * https://www.w3.org/TR/css-flexbox-1/
 */

#pragma once

#include "../geometry.h"
#include "../style.h"
#include "../layout.h"
#include "../tree/traits.h"
#include <vector>
#include <optional>

namespace lightui {

//------------------------------------------------------------------------------
// Flexbox Style Traits
//------------------------------------------------------------------------------

/// Style properties for flexbox containers
struct FlexboxContainerStyle : public CoreStyle {
    FlexDirection flex_direction = FlexDirection::Row;
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    AlignItems align_items = AlignItems::Stretch;
    AlignContent align_content = AlignContent::Stretch;
    std::optional<JustifyContent> justify_content = std::nullopt;
    Size<LengthPercentage> gap = {LengthPercentage::Zero(), LengthPercentage::Zero()};
};

/// Style properties for flexbox items (children)
struct FlexboxItemStyle : public CoreStyle {
    BoxGenerationMode box_generation_mode = BoxGenerationMode::Normal;
    std::optional<AlignSelf> align_self = std::nullopt;  // None means inherit from parent
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    Dimension flex_basis = Dimension::Auto();
    int order = 0;  // CSS order property for flex item ordering
};

//------------------------------------------------------------------------------
// Flexbox Layout Tree Interface
//------------------------------------------------------------------------------

/// Interface for trees that support flexbox layout
class LayoutFlexboxContainer : public LayoutTree {
public:
    virtual ~LayoutFlexboxContainer() = default;
    
    /// Get the flexbox container style for a node
    virtual const FlexboxContainerStyle& GetFlexboxContainerStyle(NodeId node) const = 0;
    
    /// Get the flexbox item style for a child node
    virtual const FlexboxItemStyle& GetFlexboxChildStyle(NodeId node) const = 0;
};

//------------------------------------------------------------------------------
// Internal Data Structures
//------------------------------------------------------------------------------

/// The intermediate results of a flexbox calculation for a single item
struct FlexItem {
    /// The identifier for the associated node
    NodeId node;
    
    /// The order of the node relative to its siblings
    uint32_t order;
    
    /// The base size of this item
    Size<std::optional<float>> size;
    /// The minimum allowable size of this item
    Size<std::optional<float>> min_size;
    /// The maximum allowable size of this item
    Size<std::optional<float>> max_size;
    /// The cross-alignment of this item
    AlignSelf align_self;
    
    /// The overflow style of the item
    Point<Overflow> overflow;
    /// The width of the scrollbars (if it has any)
    float scrollbar_width;
    /// The flex shrink style of the item
    float flex_shrink;
    /// The flex grow style of the item
    float flex_grow;
    
    /// The minimum size of the item (including content-based automatic minimum sizes)
    float resolved_minimum_main_size;
    
    /// The final offset of this item
    Rect<std::optional<float>> inset;
    /// The margin of this item
    Rect<float> margin;
    /// Whether each margin is an auto margin or not
    Rect<bool> margin_is_auto;
    /// The padding of this item
    Rect<float> padding;
    /// The border of this item
    Rect<float> border;
    
    /// The default size of this item
    float flex_basis;
    /// The default size of this item, minus padding and border
    float inner_flex_basis;
    /// The amount by which this item has deviated from its target size
    float violation;
    /// Is the size of this item locked
    bool frozen;
    
    /// Either the max- or min- content flex fraction
    float content_flex_fraction;
    
    /// The proposed inner size of this item
    Size<float> hypothetical_inner_size;
    /// The proposed outer size of this item
    Size<float> hypothetical_outer_size;
    /// The size that this item wants to be
    Size<float> target_size;
    /// The size that this item wants to be, plus any padding and border
    Size<float> outer_target_size;
    
    /// The position of the bottom edge of this item
    float baseline;
    
    /// A temporary value for the main offset
    float offset_main;
    /// A temporary value for the cross offset
    float offset_cross;
    
    /// Returns true if the item is a scroll container
    bool IsScrollContainer() const {
        return lightui::IsScrollContainer(overflow.x) || lightui::IsScrollContainer(overflow.y);
    }
};

/// A line of FlexItems used for intermediate computation
struct FlexLine {
    /// The slice of items to iterate over during computation of this line
    size_t start_index;
    size_t end_index;
    /// The dimensions of the cross-axis
    float cross_size;
    /// The relative offset of the cross-axis
    float offset_cross;
};

/// Values that can be cached during the flexbox algorithm
struct FlexAlgoConstants {
    /// The direction of the current segment being laid out
    FlexDirection dir;
    /// Is this segment a row
    bool is_row;
    /// Is this segment a column
    bool is_column;
    /// Is wrapping enabled (in either direction)
    bool is_wrap;
    /// Is the wrap direction inverted
    bool is_wrap_reverse;
    
    /// The item's min_size style
    Size<std::optional<float>> min_size;
    /// The item's max_size style
    Size<std::optional<float>> max_size;
    /// The margin of this section
    Rect<float> margin;
    /// The border of this section
    Rect<float> border;
    /// The space between the content box and the border box
    Rect<float> content_box_inset;
    /// The size reserved for scrollbar gutters in each axis
    Point<float> scrollbar_gutter;
    /// The gap of this section
    Size<float> gap;
    /// The align_items property of this node
    AlignItems align_items;
    /// The align_content property of this node
    AlignContent align_content;
    /// The justify_content property of this node
    std::optional<JustifyContent> justify_content;
    
    /// The border-box size of the node being laid out (if known)
    Size<std::optional<float>> node_outer_size;
    /// The content-box size of the node being laid out (if known)
    Size<std::optional<float>> node_inner_size;
    
    /// The size of the virtual container containing the flex items
    Size<float> container_size;
    /// The size of the internal container
    Size<float> inner_container_size;
};

//------------------------------------------------------------------------------
// Main Entry Point
//------------------------------------------------------------------------------

/// Computes the layout of a box according to the flexbox algorithm
LayoutOutput ComputeFlexboxLayout(
    LayoutFlexboxContainer& tree,
    NodeId node,
    const LayoutInput& inputs
);

} // namespace lightui

