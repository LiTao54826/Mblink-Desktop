/**
 * @file traits.h
 * @brief Layout tree traits and interfaces
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/tree/traits.rs
 */

#pragma once

#include "geometry.h"
#include "style.h"
#include "layout.h"
#include "cache.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <string>

namespace lightui {

//------------------------------------------------------------------------------
// IFC Measurement Structures (for Taffy integration)
//------------------------------------------------------------------------------

/// Information about a single line box in IFC layout
struct LineBoxInfo {
    /// Y position of the line box (relative to IFC container)
    float y = 0.0f;
    /// Height of the line box
    float height = 0.0f;
    /// Baseline position (distance from top of line box)
    float baseline = 0.0f;
    /// Content width of the line
    float content_width = 0.0f;
    /// Number of inline boxes in this line
    size_t box_count = 0;
};

/// Result of IFC measurement/layout (for Taffy to call)
struct IFCMeasureResult {
    /// Width of the content area (max line width)
    float content_width = 0.0f;
    /// Height of the content area (sum of line heights)
    float content_height = 0.0f;
    /// Line box information for each line
    std::vector<LineBoxInfo> line_boxes;
    /// Whether the measurement was successful
    bool success = false;
};

/// Text measurement result (for measuring individual text runs)
struct TextMeasureResult {
    /// Width of the text
    float width = 0.0f;
    /// Height of the text box (based on line-height)
    float height = 0.0f;
    /// Skia-measured ascent (positive value, distance above baseline)
    float skia_ascent = 0.0f;
    /// Skia-measured descent (positive value, distance below baseline)
    float skia_descent = 0.0f;
};

/// Node identifier type
using NodeId = uint64_t;

/// Invalid node ID constant
constexpr NodeId INVALID_NODE_ID = UINT64_MAX;

//------------------------------------------------------------------------------
// Collapsible Margin Set
//------------------------------------------------------------------------------

/// A set of margins that can collapse with each other
struct CollapsibleMarginSet {
    /// Positive margin value
    float positive = 0.0f;
    /// Negative margin value
    float negative = 0.0f;

    /// Create zero margin set
    static CollapsibleMarginSet Zero() {
        return CollapsibleMarginSet{0.0f, 0.0f};
    }

    /// Create from a single margin value
    static CollapsibleMarginSet FromMargin(float margin) {
        if (margin >= 0.0f) {
            return CollapsibleMarginSet{margin, 0.0f};
        } else {
            return CollapsibleMarginSet{0.0f, margin};
        }
    }

    /// Collapse with another margin value
    CollapsibleMarginSet CollapseWithMargin(float margin) const {
        if (margin >= 0.0f) {
            return CollapsibleMarginSet{
                f32_max(positive, margin),
                negative
            };
        } else {
            return CollapsibleMarginSet{
                positive,
                f32_min(negative, margin)
            };
        }
    }

    /// Collapse with another margin set
    CollapsibleMarginSet CollapseWithSet(const CollapsibleMarginSet& other) const {
        return CollapsibleMarginSet{
            f32_max(positive, other.positive),
            f32_min(negative, other.negative)
        };
    }

    /// Resolve the collapsed margin
    float Resolve() const {
        return positive + negative;
    }

private:
    static float f32_max(float a, float b) {
        return a > b ? a : b;
    }
    static float f32_min(float a, float b) {
        return a < b ? a : b;
    }
};

// Note: BoxGenerationMode is defined in style.h

//------------------------------------------------------------------------------
// Block Text Align (for layout purposes)
//------------------------------------------------------------------------------

/// Text alignment for block containers (different from render TextAlign)
enum class BlockTextAlign {
    Auto,
    LegacyLeft,
    LegacyRight,
    LegacyCenter
};

//------------------------------------------------------------------------------
// Style Traits (using CoreStyle from style.h)
//------------------------------------------------------------------------------

/// Block container style
struct BlockContainerStyle : public CoreStyle {
    BlockTextAlign text_align = BlockTextAlign::Auto;

    /// Check if this is a block-level element
    bool IsBlock() const {
        return display == Display::Block || display == Display::Flex || display == Display::Grid;
    }
};

/// Block item (child) style
struct BlockItemStyle : public CoreStyle {
    BoxGenerationMode box_generation_mode = BoxGenerationMode::Normal;

    /// Check if this is a table element
    bool IsTable() const {
        return false; // Tables not yet supported
    }
};

//------------------------------------------------------------------------------
// Layout Tree Interface
//------------------------------------------------------------------------------

/// Interface for accessing layout tree
class LayoutTree {
public:
    virtual ~LayoutTree() = default;

    /// Get number of children
    virtual size_t ChildCount(NodeId node) const = 0;
    
    /// Get child ID by index
    virtual NodeId GetChildId(NodeId node, size_t index) const = 0;
    
    /// Get node's cache
    virtual Cache& GetCache(NodeId node) = 0;
    
    /// Set node's unrounded layout
    virtual void SetUnroundedLayout(NodeId node, const Layout& layout) = 0;
    
    /// Get node's computed layout
    virtual const Layout& GetLayout(NodeId node) const = 0;
    
    /// Perform child layout
    virtual LayoutOutput PerformChildLayout(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode,
        Line<bool> vertical_margins_are_collapsible
    ) = 0;
    
    /// Measure child size
    virtual Size<float> MeasureChildSize(
        NodeId node,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>> parent_size,
        Size<AvailableSpace> available_space,
        SizingMode sizing_mode
    ) = 0;
};

/// Interface for block layout
class LayoutBlockContainer : public LayoutTree {
public:
    /// Get block container style
    virtual const BlockContainerStyle& GetBlockContainerStyle(NodeId node) const = 0;
    
    /// Check if node is a text node (should be skipped in block layout)
    virtual bool IsTextNode(NodeId node) const = 0;
    
    /// Get block child style
    virtual const BlockItemStyle& GetBlockChildStyle(NodeId node) const = 0;
};

/// Interface for flexbox layout
class LayoutFlexContainer : public LayoutTree {
public:
    /// Get flex container style
    virtual const Style& GetFlexContainerStyle(NodeId node) const = 0;
    
    /// Get flex item style
    virtual const Style& GetFlexItemStyle(NodeId node) const = 0;
};

} // namespace lightui

