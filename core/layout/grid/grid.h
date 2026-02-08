/**
 * @file grid.h
 * @brief CSS Grid layout algorithm
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/grid/mod.rs
 * 
 * This is a partial implementation of the CSS Grid Level 1 specification
 * https://www.w3.org/TR/css-grid-1
 */

#pragma once

#include "../types/geometry.h"
#include "../types/style.h"
#include "../types/layout.h"
#include "../types/traits.h"
#include "types.h"
#include <vector>
#include <optional>

namespace lightui {

//------------------------------------------------------------------------------
// Grid Container Style
//------------------------------------------------------------------------------

/// Non-repeated track sizing function
struct NonRepeatedTrackSizingFunction {
    MinTrackSizingFunction min;
    MaxTrackSizingFunction max;
    
    static NonRepeatedTrackSizingFunction Auto() {
        return NonRepeatedTrackSizingFunction{
            MinTrackSizingFunction::Auto(),
            MaxTrackSizingFunction::Auto()
        };
    }
    
    static NonRepeatedTrackSizingFunction Fixed(float px) {
        return NonRepeatedTrackSizingFunction{
            MinTrackSizingFunction::Fixed(px),
            MaxTrackSizingFunction::Fixed(px)
        };
    }
    
    static NonRepeatedTrackSizingFunction Flex(float fr) {
        return NonRepeatedTrackSizingFunction{
            MinTrackSizingFunction::Auto(),
            MaxTrackSizingFunction::Fraction(fr)
        };
    }
    
    static NonRepeatedTrackSizingFunction MinMax(MinTrackSizingFunction min_func, MaxTrackSizingFunction max_func) {
        return NonRepeatedTrackSizingFunction{min_func, max_func};
    }
};

/// Track sizing function (can be repeated)
struct TrackSizingFunction {
    enum class Type {
        Single,
        Repeat
    };
    
    Type type = Type::Single;
    NonRepeatedTrackSizingFunction single;
    
    // For repeat
    uint16_t repeat_count = 0;  // 0 = auto-fill, UINT16_MAX = auto-fit
    std::vector<NonRepeatedTrackSizingFunction> repeat_tracks;
    
    static TrackSizingFunction Single(NonRepeatedTrackSizingFunction func) {
        TrackSizingFunction tsf;
        tsf.type = Type::Single;
        tsf.single = func;
        return tsf;
    }
    
    static TrackSizingFunction Repeat(uint16_t count, std::vector<NonRepeatedTrackSizingFunction> tracks) {
        TrackSizingFunction tsf;
        tsf.type = Type::Repeat;
        tsf.repeat_count = count;
        tsf.repeat_tracks = std::move(tracks);
        return tsf;
    }
};

/// Grid placement for a single axis
struct GridPlacement {
    enum class Type {
        Auto,
        Line,
        Span
    };
    
    Type type = Type::Auto;
    int16_t value = 0;  // Line number or span count
    
    static GridPlacement Auto() {
        return GridPlacement{Type::Auto, 0};
    }
    
    static GridPlacement Line(int16_t line) {
        return GridPlacement{Type::Line, line};
    }
    
    static GridPlacement Span(uint16_t span) {
        return GridPlacement{Type::Span, static_cast<int16_t>(span)};
    }
    
    bool IsAuto() const { return type == Type::Auto; }
    bool IsLine() const { return type == Type::Line; }
    bool IsSpan() const { return type == Type::Span; }
};

/// Style properties for grid containers
/// @deprecated 使用 Style 替代通用属性，此结构保留用于 Grid 特有数据（grid-template-columns/rows 等）
struct GridContainerStyle : public CoreStyle {
    /// Defines the track sizing functions of the grid rows
    std::vector<TrackSizingFunction> grid_template_rows;
    
    /// Defines the track sizing functions of the grid columns
    std::vector<TrackSizingFunction> grid_template_columns;
    
    /// Defines the size of implicitly created rows
    std::vector<NonRepeatedTrackSizingFunction> grid_auto_rows;
    
    /// Defines the size of implicitly created columns
    std::vector<NonRepeatedTrackSizingFunction> grid_auto_columns;
    
    /// Controls how auto-placed items are inserted in the grid
    enum class GridAutoFlow {
        Row,
        Column,
        RowDense,
        ColumnDense
    };
    GridAutoFlow grid_auto_flow = GridAutoFlow::Row;
    
    /// Gap between rows
    LengthPercentage row_gap = LengthPercentage::Zero();
    
    /// Gap between columns
    LengthPercentage column_gap = LengthPercentage::Zero();
    
    /// Alignment of items in the inline axis
    std::optional<AlignItems> align_items = std::nullopt;
    
    /// Alignment of items in the block axis
    std::optional<AlignItems> justify_items = std::nullopt;
    
    /// Alignment of content in the inline axis
    std::optional<AlignContent> align_content = std::nullopt;
    
    /// Alignment of content in the block axis
    std::optional<AlignContent> justify_content = std::nullopt;
};

/// Style properties for grid items
/// @deprecated 使用 Style 替代通用属性，此结构保留用于 Grid 特有数据（grid-row-start/end 等）
struct GridItemStyle : public CoreStyle {
    /// Grid row start placement
    GridPlacement grid_row_start = GridPlacement::Auto();
    
    /// Grid row end placement
    GridPlacement grid_row_end = GridPlacement::Auto();
    
    /// Grid column start placement
    GridPlacement grid_column_start = GridPlacement::Auto();
    
    /// Grid column end placement
    GridPlacement grid_column_end = GridPlacement::Auto();
    
    /// Alignment of this item in the inline axis (overrides container's align_items)
    std::optional<AlignSelf> align_self = std::nullopt;
    
    /// Alignment of this item in the block axis (overrides container's justify_items)
    std::optional<AlignSelf> justify_self = std::nullopt;
};

//------------------------------------------------------------------------------
// Grid Layout Tree Interface
//------------------------------------------------------------------------------

/// Interface for trees that support grid layout
///
/// 重构说明：接口现在使用统一的 Style 结构，而非专门的 GridContainerStyle/GridItemStyle。
/// 这消除了样式同步问题，布局算法直接从 Style 读取所需属性。
/// Grid 特有数据（如 grid-template-columns/rows）通过单独的方法获取。
class LayoutGridContainer : public LayoutTree {
public:
    virtual ~LayoutGridContainer() = default;

    /// Get the style for a container node
    /// @param node The node ID
    /// @return Reference to the node's Style
    virtual const Style& GetContainerStyle(NodeId node) const = 0;

    /// Get the style for a child node
    /// @param node The child node ID
    /// @return Reference to the child node's Style
    virtual const Style& GetChildStyle(NodeId node) const = 0;

    /// Get the grid container style for a node (Grid-specific data)
    /// This provides access to Grid-specific properties like grid-template-columns/rows
    /// that are not part of the unified Style structure.
    /// @param node The node ID
    /// @return Reference to the node's GridContainerStyle
    virtual const GridContainerStyle& GetGridContainerStyle(NodeId node) const = 0;

    /// Get the grid item style for a child node (Grid-specific data)
    /// This provides access to Grid-specific properties like grid-row-start/end
    /// that are not part of the unified Style structure.
    /// @param node The child node ID
    /// @return Reference to the child node's GridItemStyle
    virtual const GridItemStyle& GetGridItemStyle(NodeId node) const = 0;

    /// Check if node is a text node (should be skipped in grid layout)
    /// @param node The node ID
    /// @return true if the node is a text node
    virtual bool IsTextNode(NodeId node) const = 0;
};

//------------------------------------------------------------------------------
// Grid Item (Internal)
//------------------------------------------------------------------------------

/// Represents a single grid item during layout
struct GridItem {
    /// The node id of the item
    NodeId node;
    
    /// The item's row placement
    Line<OriginZeroLine> row;
    
    /// The item's column placement
    Line<OriginZeroLine> column;
    
    /// The item's overflow style
    Point<Overflow> overflow;
    
    /// The item's resolved size
    Size<std::optional<float>> size;
    
    /// The item's resolved min size
    Size<std::optional<float>> min_size;
    
    /// The item's resolved max size
    Size<std::optional<float>> max_size;
    
    /// The item's alignment
    AlignSelf align_self;
    AlignSelf justify_self;
    
    /// The item's baseline
    float baseline = 0.0f;
    
    /// The item's baseline shim
    float baseline_shim = 0.0f;
    
    /// Whether the item crosses flexible tracks
    bool crosses_flexible_row = false;
    bool crosses_flexible_column = false;
    
    /// Whether the item crosses intrinsic tracks
    bool crosses_intrinsic_row = false;
    bool crosses_intrinsic_column = false;
    
    /// Placement order for auto-placement
    uint32_t source_order = 0;
    
    /// Get the placement for an axis
    Line<OriginZeroLine> Placement(AbsoluteAxis axis) const {
        return axis == AbsoluteAxis::Horizontal ? column : row;
    }
    
    /// Get the span for an axis
    uint16_t Span(AbsoluteAxis axis) const {
        return LineSpan(Placement(axis));
    }
    
    /// Check if item crosses flexible tracks in an axis
    bool CrossesFlexibleTrack(AbsoluteAxis axis) const {
        return axis == AbsoluteAxis::Horizontal ? crosses_flexible_column : crosses_flexible_row;
    }
    
    /// Check if item crosses intrinsic tracks in an axis
    bool CrossesIntrinsicTrack(AbsoluteAxis axis) const {
        return axis == AbsoluteAxis::Horizontal ? crosses_intrinsic_column : crosses_intrinsic_row;
    }
};

//------------------------------------------------------------------------------
// Grid Layout Function
//------------------------------------------------------------------------------

/// Compute grid layout for a node
LayoutOutput ComputeGridLayout(
    LayoutGridContainer& tree,
    NodeId node,
    const LayoutInput& inputs
);

} // namespace lightui

