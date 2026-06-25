/**
 * @file style.h
 * @brief CSS style types for layout computation
 *
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/style/mod.rs, dimension.rs, alignment.rs, flex.rs
 */

#pragma once

#include "geometry.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <memory>
#include <optional>

namespace mblink {

//------------------------------------------------------------------------------
// Dimension Types
//------------------------------------------------------------------------------

enum class LengthTag : uint8_t {
    Length = 0,
    Percent = 1,
    Auto = 2,
    Calc = 3,
    Min = 4,
    Max = 5,
    Clamp = 6,
};

struct LengthPercentageAuto {
    LengthTag tag = LengthTag::Auto;
    float value = 0.0f;
    float calc_px = 0.0f;
    std::shared_ptr<LengthPercentageAuto> func_a;
    std::shared_ptr<LengthPercentageAuto> func_b;
    std::shared_ptr<LengthPercentageAuto> func_c;

    static LengthPercentageAuto Length(float val) { return {LengthTag::Length, val, 0.0f}; }
    static LengthPercentageAuto Percent(float val) { return {LengthTag::Percent, val, 0.0f}; }
    static LengthPercentageAuto Auto() { return {LengthTag::Auto, 0.0f, 0.0f}; }
    static LengthPercentageAuto Zero() { return Length(0.0f); }
    static LengthPercentageAuto Calc(float percent, float px) { return {LengthTag::Calc, percent, px}; }
    static LengthPercentageAuto Min(const LengthPercentageAuto& a, const LengthPercentageAuto& b) {
        LengthPercentageAuto out{LengthTag::Min, 0.0f, 0.0f};
        out.func_a = std::make_shared<LengthPercentageAuto>(a);
        out.func_b = std::make_shared<LengthPercentageAuto>(b);
        return out;
    }
    static LengthPercentageAuto Max(const LengthPercentageAuto& a, const LengthPercentageAuto& b) {
        LengthPercentageAuto out{LengthTag::Max, 0.0f, 0.0f};
        out.func_a = std::make_shared<LengthPercentageAuto>(a);
        out.func_b = std::make_shared<LengthPercentageAuto>(b);
        return out;
    }
    static LengthPercentageAuto Clamp(const LengthPercentageAuto& a, const LengthPercentageAuto& b, const LengthPercentageAuto& c) {
        LengthPercentageAuto out{LengthTag::Clamp, 0.0f, 0.0f};
        out.func_a = std::make_shared<LengthPercentageAuto>(a);
        out.func_b = std::make_shared<LengthPercentageAuto>(b);
        out.func_c = std::make_shared<LengthPercentageAuto>(c);
        return out;
    }

    bool IsAuto() const { return tag == LengthTag::Auto; }
    bool IsLength() const { return tag == LengthTag::Length; }
    bool IsPercent() const { return tag == LengthTag::Percent; }
    bool IsCalc() const { return tag == LengthTag::Calc; }

    std::optional<float> ResolveToOption(float context) const {
        auto eval = [&](const std::shared_ptr<LengthPercentageAuto>& expr) -> std::optional<float> {
            return expr ? expr->ResolveToOption(context) : std::nullopt;
        };
        switch (tag) {
            case LengthTag::Length: return value;
            case LengthTag::Percent: return context * value;
            case LengthTag::Calc: return context * value + calc_px;
            case LengthTag::Min: { auto a = eval(func_a); auto b = eval(func_b); return (a && b) ? std::optional<float>(std::min(*a, *b)) : std::nullopt; }
            case LengthTag::Max: { auto a = eval(func_a); auto b = eval(func_b); return (a && b) ? std::optional<float>(std::max(*a, *b)) : std::nullopt; }
            case LengthTag::Clamp: { auto a = eval(func_a); auto b = eval(func_b); auto c = eval(func_c); return (a && b && c) ? std::optional<float>(std::max(*a, std::min(*b, *c))) : std::nullopt; }
            case LengthTag::Auto: return std::nullopt;
        }
        return std::nullopt;
    }

    bool operator==(const LengthPercentageAuto& other) const {
        if (tag != other.tag || value != other.value || calc_px != other.calc_px) return false;
        auto same = [](const std::shared_ptr<LengthPercentageAuto>& a, const std::shared_ptr<LengthPercentageAuto>& b) {
            return (!a && !b) || (a && b && *a == *b);
        };
        return same(func_a, other.func_a) && same(func_b, other.func_b) && same(func_c, other.func_c);
    }
};

struct LengthPercentage {
    LengthTag tag = LengthTag::Length;
    float value = 0.0f;
    float calc_px = 0.0f;
    std::shared_ptr<LengthPercentage> func_a;
    std::shared_ptr<LengthPercentage> func_b;
    std::shared_ptr<LengthPercentage> func_c;

    static LengthPercentage Length(float val) { return {LengthTag::Length, val, 0.0f}; }
    static LengthPercentage Percent(float val) { return {LengthTag::Percent, val, 0.0f}; }
    static LengthPercentage Zero() { return Length(0.0f); }
    static LengthPercentage Calc(float percent, float px) { return {LengthTag::Calc, percent, px}; }
    static LengthPercentage Min(const LengthPercentage& a, const LengthPercentage& b) { LengthPercentage out{LengthTag::Min, 0.0f, 0.0f}; out.func_a = std::make_shared<LengthPercentage>(a); out.func_b = std::make_shared<LengthPercentage>(b); return out; }
    static LengthPercentage Max(const LengthPercentage& a, const LengthPercentage& b) { LengthPercentage out{LengthTag::Max, 0.0f, 0.0f}; out.func_a = std::make_shared<LengthPercentage>(a); out.func_b = std::make_shared<LengthPercentage>(b); return out; }
    static LengthPercentage Clamp(const LengthPercentage& a, const LengthPercentage& b, const LengthPercentage& c) { LengthPercentage out{LengthTag::Clamp, 0.0f, 0.0f}; out.func_a = std::make_shared<LengthPercentage>(a); out.func_b = std::make_shared<LengthPercentage>(b); out.func_c = std::make_shared<LengthPercentage>(c); return out; }
    bool IsLength() const { return tag == LengthTag::Length; }
    bool IsPercent() const { return tag == LengthTag::Percent; }
    bool IsCalc() const { return tag == LengthTag::Calc; }
    float Resolve(float context) const { return ResolveToOption(context).value_or(0.0f); }
    std::optional<float> ResolveToOption(float context) const {
        auto eval = [&](const std::shared_ptr<LengthPercentage>& expr) -> std::optional<float> { return expr ? expr->ResolveToOption(context) : std::nullopt; };
        switch (tag) {
            case LengthTag::Length: return value;
            case LengthTag::Percent: return context * value;
            case LengthTag::Calc: return context * value + calc_px;
            case LengthTag::Min: { auto a = eval(func_a); auto b = eval(func_b); return (a && b) ? std::optional<float>(std::min(*a, *b)) : std::nullopt; }
            case LengthTag::Max: { auto a = eval(func_a); auto b = eval(func_b); return (a && b) ? std::optional<float>(std::max(*a, *b)) : std::nullopt; }
            case LengthTag::Clamp: { auto a = eval(func_a); auto b = eval(func_b); auto c = eval(func_c); return (a && b && c) ? std::optional<float>(std::max(*a, std::min(*b, *c))) : std::nullopt; }
            default: return std::nullopt;
        }
    }
    operator LengthPercentageAuto() const {
        LengthPercentageAuto out{tag, value, calc_px};
        if (func_a) out.func_a = std::make_shared<LengthPercentageAuto>(static_cast<LengthPercentageAuto>(*func_a));
        if (func_b) out.func_b = std::make_shared<LengthPercentageAuto>(static_cast<LengthPercentageAuto>(*func_b));
        if (func_c) out.func_c = std::make_shared<LengthPercentageAuto>(static_cast<LengthPercentageAuto>(*func_c));
        return out;
    }
    bool operator==(const LengthPercentage& other) const {
        if (tag != other.tag || value != other.value || calc_px != other.calc_px) return false;
        auto same = [](const std::shared_ptr<LengthPercentage>& a, const std::shared_ptr<LengthPercentage>& b) { return (!a && !b) || (a && b && *a == *b); };
        return same(func_a, other.func_a) && same(func_b, other.func_b) && same(func_c, other.func_c);
    }
};

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
    Scroll,
    Auto
};

/// Check if overflow creates a scroll container
inline bool IsScrollContainer(Overflow overflow) {
    return overflow == Overflow::Scroll ||
           overflow == Overflow::Hidden ||
           overflow == Overflow::Auto;
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
    float scrollbar_height = 0.0f;

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

    // Margin collapsing behavior
    // CSS spec: margins do NOT collapse for inline-block, floats, absolutely positioned,
    // flex/grid items, and elements that establish new block formatting contexts
    bool margins_collapse = true;
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

    // Block layout properties
    // Note: BlockTextAlign is defined in traits.h for now
    // text_align is used for block-level text alignment (legacy values)
    int block_text_align = 0;  // 0=Auto, 1=LegacyLeft, 2=LegacyRight, 3=LegacyCenter

    /// Get box generation mode
    BoxGenerationMode GetBoxGenerationMode() const {
        return display == Display::None ? BoxGenerationMode::None : BoxGenerationMode::Normal;
    }

    /// Check if this is a table element
    bool IsTable() const {
        return item_is_table;
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

} // namespace mblink

