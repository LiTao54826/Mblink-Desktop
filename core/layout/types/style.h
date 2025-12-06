/**
 * @file style.h
 * @brief CSS style types for layout computation
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/style/mod.rs, dimension.rs, alignment.rs, flex.rs
 */

#pragma once

#include "geometry.h"
#include <optional>
#include <cstdint>
#include <cmath>

namespace lightui {

//------------------------------------------------------------------------------
// Dimension Types
//------------------------------------------------------------------------------

/// Tag values for compact length representation
enum class LengthTag : uint8_t {
    Length = 0,
    Percent = 1,
    Auto = 2,
    Calc = 3,  // calc() expression: percent + px
};

/// A unit of linear measurement - Length, Percent, Auto, or Calc
struct LengthPercentageAuto {
    LengthTag tag;
    float value;
    float calc_px = 0.0f;  // For calc: the px offset (value stores percent)

    static LengthPercentageAuto Length(float val) {
        return LengthPercentageAuto{LengthTag::Length, val, 0.0f};
    }

    static LengthPercentageAuto Percent(float val) {
        return LengthPercentageAuto{LengthTag::Percent, val, 0.0f};
    }

    static LengthPercentageAuto Auto() {
        return LengthPercentageAuto{LengthTag::Auto, 0.0f, 0.0f};
    }

    static LengthPercentageAuto Zero() {
        return Length(0.0f);
    }

    /// Create a calc expression: percent% + px
    static LengthPercentageAuto Calc(float percent, float px) {
        return LengthPercentageAuto{LengthTag::Calc, percent, px};
    }

    bool IsAuto() const { return tag == LengthTag::Auto; }
    bool IsLength() const { return tag == LengthTag::Length; }
    bool IsPercent() const { return tag == LengthTag::Percent; }
    bool IsCalc() const { return tag == LengthTag::Calc; }

    /// Resolve to option: Length returns value, Percent resolves, Auto returns nullopt
    std::optional<float> ResolveToOption(float context) const {
        switch (tag) {
            case LengthTag::Length: return value;
            case LengthTag::Percent: return context * value;
            case LengthTag::Calc: return context * value + calc_px;
            case LengthTag::Auto: return std::nullopt;
        }
        return std::nullopt;
    }

    bool operator==(const LengthPercentageAuto& other) const {
        if (tag != other.tag) return false;
        if (tag == LengthTag::Auto) return true;
        if (tag == LengthTag::Calc) return value == other.value && calc_px == other.calc_px;
        return value == other.value;
    }
};

/// A unit of linear measurement - Length, Percent, or Calc (no Auto)
struct LengthPercentage {
    LengthTag tag;
    float value;
    float calc_px = 0.0f;  // For calc: the px offset

    static LengthPercentage Length(float val) {
        return LengthPercentage{LengthTag::Length, val, 0.0f};
    }

    static LengthPercentage Percent(float val) {
        return LengthPercentage{LengthTag::Percent, val, 0.0f};
    }

    static LengthPercentage Zero() {
        return Length(0.0f);
    }

    static LengthPercentage Calc(float percent, float px) {
        return LengthPercentage{LengthTag::Calc, percent, px};
    }

    bool IsLength() const { return tag == LengthTag::Length; }
    bool IsPercent() const { return tag == LengthTag::Percent; }
    bool IsCalc() const { return tag == LengthTag::Calc; }

    /// Resolve value against context
    float Resolve(float context) const {
        switch (tag) {
            case LengthTag::Length: return value;
            case LengthTag::Percent: return context * value;
            case LengthTag::Calc: return context * value + calc_px;
            default: return 0.0f;
        }
    }

    operator LengthPercentageAuto() const {
        return LengthPercentageAuto{tag, value, calc_px};
    }

    bool operator==(const LengthPercentage& other) const {
        if (tag != other.tag) return false;
        if (tag == LengthTag::Calc) return value == other.value && calc_px == other.calc_px;
        return value == other.value;
    }
};

/// A dimension value: Length, Percent, or Auto
using Dimension = LengthPercentageAuto;

//------------------------------------------------------------------------------
// Alignment Types
//------------------------------------------------------------------------------

/// Used to control how child nodes are aligned
/// For Flexbox: controls alignment in the cross axis
/// For Grid: controls alignment in the block axis
enum class AlignItems {
    Start,
    End,
    FlexStart,
    FlexEnd,
    Center,
    Baseline,
    Stretch
};

using JustifyItems = AlignItems;
using AlignSelf = AlignItems;
using JustifySelf = AlignItems;

/// Sets the distribution of space between and around content items
/// For Flexbox: controls alignment in the cross axis
/// For Grid: controls alignment in the block axis
enum class AlignContent {
    Start,
    End,
    FlexStart,
    FlexEnd,
    Center,
    Stretch,
    SpaceBetween,
    SpaceEvenly,
    SpaceAround
};

using JustifyContent = AlignContent;

//------------------------------------------------------------------------------
// Display and Position
//------------------------------------------------------------------------------

/// Layout display mode
enum class Display {
    Block,
    Flex,
    Grid,
    None
};

/// Box generation mode
enum class BoxGenerationMode {
    Normal,
    None
};

/// Positioning strategy
enum class Position {
    Relative,
    Absolute,
    Fixed,    // 相对于视口定位
    Sticky    // 粘性定位（滚动时切换 relative/fixed）
};

/// Box sizing mode
enum class BoxSizing {
    BorderBox,
    ContentBox
};

/// Overflow behavior
enum class Overflow {
    Visible,
    Clip,
    Hidden,
    Scroll
};

/// Check if overflow creates a scroll container
inline bool IsScrollContainer(Overflow overflow) {
    return overflow == Overflow::Scroll || overflow == Overflow::Hidden;
}

//------------------------------------------------------------------------------
// Flexbox Types
//------------------------------------------------------------------------------

/// Direction of flex layout
/// (FlexDirection is already defined in geometry.h)

/// Whether flex items wrap
enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

//------------------------------------------------------------------------------
// Grid Types (forward declarations - full implementation in grid_style.h)
//------------------------------------------------------------------------------

enum class GridAutoFlow {
    Row,
    Column,
    RowDense,
    ColumnDense
};

//------------------------------------------------------------------------------
// Style Structure
//------------------------------------------------------------------------------

/// Core style properties shared by all layout types
struct CoreStyle {
    // Display mode
    Display display = Display::Block;
    bool item_is_table = false;
    bool item_is_replaced = false;
    BoxSizing box_sizing = BoxSizing::BorderBox;

    // Overflow
    Point<Overflow> overflow = {Overflow::Visible, Overflow::Visible};
    float scrollbar_width = 0.0f;

    // Position
    Position position = Position::Relative;
    Rect<LengthPercentageAuto> inset = {
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto()
    };

    // Size
    Size<Dimension> size = {Dimension::Auto(), Dimension::Auto()};
    Size<Dimension> min_size = {Dimension::Auto(), Dimension::Auto()};
    Size<Dimension> max_size = {Dimension::Auto(), Dimension::Auto()};
    std::optional<float> aspect_ratio = std::nullopt;

    // Spacing
    Rect<LengthPercentageAuto> margin = Rect<LengthPercentageAuto>::Zero();
    Rect<LengthPercentage> padding = Rect<LengthPercentage>::Zero();
    Rect<LengthPercentage> border = Rect<LengthPercentage>::Zero();
};

/// Complete style for layout computation
struct Style : public CoreStyle {

    // Alignment (Flexbox & Grid)
    std::optional<AlignItems> align_items = std::nullopt;
    std::optional<AlignSelf> align_self = std::nullopt;
    std::optional<AlignItems> justify_items = std::nullopt;
    std::optional<AlignSelf> justify_self = std::nullopt;
    std::optional<AlignContent> align_content = std::nullopt;
    std::optional<JustifyContent> justify_content = std::nullopt;
    Size<LengthPercentage> gap = {LengthPercentage::Zero(), LengthPercentage::Zero()};

    // Flexbox properties
    FlexDirection flex_direction = FlexDirection::Row;
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    Dimension flex_basis = Dimension::Auto();
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    int order = 0;  // CSS order property for flex item ordering

    // Grid properties (simplified - full grid support in grid_style.h)
    GridAutoFlow grid_auto_flow = GridAutoFlow::Row;

    /// Get box generation mode
    BoxGenerationMode GetBoxGenerationMode() const {
        return display == Display::None ? BoxGenerationMode::None : BoxGenerationMode::Normal;
    }

    /// Check if block layout
    bool IsBlock() const {
        return display == Display::Block;
    }

    /// Check if flex layout
    bool IsFlex() const {
        return display == Display::Flex;
    }

    /// Check if grid layout
    bool IsGrid() const {
        return display == Display::Grid;
    }

    /// Check if hidden
    bool IsHidden() const {
        return display == Display::None;
    }

    /// Default style
    static Style Default() {
        return Style{};
    }
};

//------------------------------------------------------------------------------
// Helper functions for Size and Rect with Dimension types
//------------------------------------------------------------------------------

// Size<Dimension> Auto
inline Size<Dimension> SizeDimensionAuto() {
    return Size<Dimension>{Dimension::Auto(), Dimension::Auto()};
}

// Size<Dimension> Zero
inline Size<Dimension> SizeDimensionZero() {
    return Size<Dimension>{Dimension::Zero(), Dimension::Zero()};
}

// Rect<LengthPercentageAuto> Auto
inline Rect<LengthPercentageAuto> RectLPAAuto() {
    return Rect<LengthPercentageAuto>{
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto(),
        LengthPercentageAuto::Auto()
    };
}

// Rect<LengthPercentageAuto> Zero
inline Rect<LengthPercentageAuto> RectLPAZero() {
    return Rect<LengthPercentageAuto>{
        LengthPercentageAuto::Zero(),
        LengthPercentageAuto::Zero(),
        LengthPercentageAuto::Zero(),
        LengthPercentageAuto::Zero()
    };
}

// Rect<LengthPercentage> Zero
inline Rect<LengthPercentage> RectLPZero() {
    return Rect<LengthPercentage>{
        LengthPercentage::Zero(),
        LengthPercentage::Zero(),
        LengthPercentage::Zero(),
        LengthPercentage::Zero()
    };
}

} // namespace lightui

