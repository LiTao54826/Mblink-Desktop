/**
 * @file layout.h
 * @brief Layout output types
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/tree/layout.rs
 */

#pragma once

#include "geometry.h"
#include <cstdint>

namespace mblink {

//------------------------------------------------------------------------------
// Run Mode
//------------------------------------------------------------------------------

/// Whether we are performing a full layout, or we merely need to size the node
enum class RunMode {
    /// A full layout for this node and all children should be computed
    PerformLayout,
    /// The layout algorithm should be executed such that an accurate container size
    /// for the node can be determined. Layout steps that aren't necessary for
    /// determining the container size of the current node can be skipped.
    ComputeSize,
    /// This node should have a null layout set as it has been hidden (i.e. using Display::None)
    PerformHiddenLayout
};

//------------------------------------------------------------------------------
// Sizing Mode
//------------------------------------------------------------------------------

/// The type of size constraint to apply when measuring a node
enum class SizingMode {
    /// Node should be sized to fill content while respecting constraints
    ContentSize,
    /// Node should be sized to fill available space while respecting constraints
    InherentSize
};

//------------------------------------------------------------------------------
// Available Space
//------------------------------------------------------------------------------

/// Represents available space for layout
struct AvailableSpace {
    enum class Type : uint8_t {
        /// Definite available space (exact pixel value)
        Definite,
        /// Indefinite available space (min-content/max-content)
        MinContent,
        MaxContent
    };

    Type type;
    float value;

    /// Create definite available space
    static AvailableSpace Definite(float val) {
        return AvailableSpace{Type::Definite, val};
    }

    /// Create min-content available space
    static AvailableSpace MinContent() {
        return AvailableSpace{Type::MinContent, 0.0f};
    }

    /// Create max-content available space
    static AvailableSpace MaxContent() {
        return AvailableSpace{Type::MaxContent, 0.0f};
    }

    /// Create from optional - Some becomes Definite, None becomes MaxContent
    static AvailableSpace FromOption(std::optional<float> opt) {
        return opt.has_value() ? Definite(*opt) : MaxContent();
    }

    /// Create zero available space
    static AvailableSpace Zero() {
        return Definite(0.0f);
    }

    /// Check if definite
    bool IsDefinite() const { return type == Type::Definite; }
    bool IsMinContent() const { return type == Type::MinContent; }
    bool IsMaxContent() const { return type == Type::MaxContent; }

    /// Get value if definite
    std::optional<float> IntoOption() const {
        return IsDefinite() ? std::optional<float>(value) : std::nullopt;
    }

    /// Get value or default
    float UnwrapOr(float def) const {
        return IsDefinite() ? value : def;
    }

    /// Map definite value
    AvailableSpace MapDefinite(float (*f)(float)) const {
        if (IsDefinite()) {
            return Definite(f(value));
        }
        return *this;
    }

    /// Compute free space (available - used)
    float ComputeFreeSpace(float used) const {
        return IsDefinite() ? (value - used) : 0.0f;
    }

    /// Return 0 for definite/min-content, infinity for max-content
    float MaybeMax() const {
        return IsMaxContent() ? INFINITY : 0.0f;
    }

    /// Return 0 for definite/max-content, infinity for min-content
    float MaybeMin() const {
        return IsMinContent() ? INFINITY : 0.0f;
    }
};

//------------------------------------------------------------------------------
// Layout Input
//------------------------------------------------------------------------------

/// Input parameters for layout computation
struct LayoutInput {
    /// Whether we only need to know the Node's size, or whether we need to perform full layout
    RunMode run_mode = RunMode::PerformLayout;

    /// Whether a Node's style sizes should be taken into account or ignored
    SizingMode sizing_mode = SizingMode::InherentSize;

    /// Axis to run layout for (for caching)
    /// None = compute both axes
    std::optional<AbstractAxis> axis = std::nullopt;

    /// Known dimensions of the node
    Size<std::optional<float>> known_dimensions = {std::nullopt, std::nullopt};

    /// Parent size for percentage resolution
    Size<std::optional<float>> parent_size = {std::nullopt, std::nullopt};

    /// Available space for layout
    Size<AvailableSpace> available_space = {
        AvailableSpace::MaxContent(),
        AvailableSpace::MaxContent()
    };

    /// Whether vertical margins are collapsible (start, end)
    Line<bool> vertical_margins_are_collapsible = {false, false};

    /// A LayoutInput that can be used to request hidden layout
    static LayoutInput Hidden() {
        return LayoutInput{
            RunMode::PerformHiddenLayout,
            SizingMode::InherentSize,
            std::nullopt,
            {std::nullopt, std::nullopt},
            {std::nullopt, std::nullopt},
            {AvailableSpace::MaxContent(), AvailableSpace::MaxContent()},
            {false, false}
        };
    }
};

//------------------------------------------------------------------------------
// Layout Output
//------------------------------------------------------------------------------

/// Output from layout computation
struct LayoutOutput {
    /// Computed size of the node
    Size<float> size = Size<float>::Zero();
    
    /// Size of content (for scrolling)
    Size<float> content_size = Size<float>::Zero();
    
    /// First baseline offset from top
    std::optional<float> first_baselines = std::nullopt;
    
    /// Top margin that may collapse
    float top_margin = 0.0f;
    
    /// Bottom margin that may collapse
    float bottom_margin = 0.0f;
    
    /// Whether margins collapse through this element
    bool margins_can_collapse_through = false;

    /// Create output with just size
    static LayoutOutput FromOuterSize(Size<float> size) {
        LayoutOutput output;
        output.size = size;
        return output;
    }

    /// Create output for hidden node
    static LayoutOutput Hidden() {
        return LayoutOutput{};
    }
};

//------------------------------------------------------------------------------
// Layout - Final computed position and size
//------------------------------------------------------------------------------

/// Final computed layout for a node
struct Layout {
    /// Order in which node was laid out (for debugging)
    uint32_t order = 0;
    
    /// Position relative to parent
    Point<float> location = Point<float>::Zero();
    
    /// Outer size (including border + padding)
    Size<float> size = Size<float>::Zero();
    
    /// Size of content area (for scrolling)
    Size<float> content_size = Size<float>::Zero();
    
    /// Border widths
    Rect<float> border = Rect<float>::Zero();
    
    /// Padding widths
    Rect<float> padding = Rect<float>::Zero();
    
    /// Scrollbar gutters
    Size<float> scrollbar_size = Size<float>::Zero();

    /// Create new layout with position and size
    static Layout New() {
        return Layout{};
    }

    /// Create layout with size
    static Layout WithSize(Size<float> size) {
        Layout layout;
        layout.size = size;
        return layout;
    }
};

//------------------------------------------------------------------------------
// Collected Flex Item
//------------------------------------------------------------------------------

/// A flex item that has been collected and is ready for layout
struct CollectedFlexItem {
    /// Index of the item in the flex container
    size_t index;
    
    /// Source index (original order before sorting)
    size_t source_index;
    
    /// Computed size
    Size<float> size;
    
    /// Computed min size
    Size<float> min_size;
    
    /// Computed max size
    Size<float> max_size;
    
    /// Hypothetical main size before flex
    float hypothetical_main_size;
    
    /// Hypothetical cross size
    float hypothetical_cross_size;
    
    /// Target main size after flex
    float target_main_size;
    
    /// Outer target main size (including margin)
    float outer_target_main_size;
    
    /// Flex basis
    float flex_basis;
    
    /// Flex grow
    float flex_grow;
    
    /// Flex shrink
    float flex_shrink;
    
    /// Is frozen (no more flexing)
    bool frozen;
    
    /// Resolved margin
    Rect<float> margin;
    
    /// Resolved padding
    Rect<float> padding;
    
    /// Resolved border
    Rect<float> border;
    
    /// Baseline offset from top
    std::optional<float> baseline;
};

} // namespace mblink

