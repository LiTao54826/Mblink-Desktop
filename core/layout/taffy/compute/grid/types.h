/**
 * @file types.h
 * @brief Grid layout types
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/grid/types/
 */

#pragma once

#include "../../geometry.h"
#include "../../style.h"
#include <cstdint>
#include <vector>
#include <optional>
#include <algorithm>

namespace lightui {

//------------------------------------------------------------------------------
// Grid Coordinates
//------------------------------------------------------------------------------

/// Forward declaration
struct TrackCounts;

/// Represents a grid line position in "CSS Grid Line" coordinates
/// - The line at left hand (or top) edge of the explicit grid is line 1
/// - The line at the right hand (or bottom) edge of the explicit grid is -1
/// - 0 is not a valid index
struct GridLine {
    int16_t value;
    
    GridLine() : value(0) {}
    explicit GridLine(int16_t v) : value(v) {}
    
    int16_t AsI16() const { return value; }
    
    /// Convert into OriginZero coordinates using the specified explicit track count
    struct OriginZeroLine IntoOriginZeroLine(uint16_t explicit_track_count) const;
};

/// Represents a grid line position in "OriginZero" coordinates
/// - The line at left hand (or top) edge of the explicit grid is line 0
/// - The next line to the right (or down) is 1, and so on
/// - The next line to the left (or up) is -1, and so on
struct OriginZeroLine {
    int16_t value;
    
    OriginZeroLine() : value(0) {}
    explicit OriginZeroLine(int16_t v) : value(v) {}
    
    // Arithmetic operators
    OriginZeroLine operator+(const OriginZeroLine& rhs) const {
        return OriginZeroLine(value + rhs.value);
    }
    OriginZeroLine operator-(const OriginZeroLine& rhs) const {
        return OriginZeroLine(value - rhs.value);
    }
    OriginZeroLine operator+(uint16_t rhs) const {
        return OriginZeroLine(value + static_cast<int16_t>(rhs));
    }
    OriginZeroLine operator-(uint16_t rhs) const {
        return OriginZeroLine(value - static_cast<int16_t>(rhs));
    }
    OriginZeroLine& operator+=(uint16_t rhs) {
        value += static_cast<int16_t>(rhs);
        return *this;
    }
    
    // Comparison operators
    bool operator<(const OriginZeroLine& rhs) const { return value < rhs.value; }
    bool operator<=(const OriginZeroLine& rhs) const { return value <= rhs.value; }
    bool operator>(const OriginZeroLine& rhs) const { return value > rhs.value; }
    bool operator>=(const OriginZeroLine& rhs) const { return value >= rhs.value; }
    bool operator==(const OriginZeroLine& rhs) const { return value == rhs.value; }
    bool operator!=(const OriginZeroLine& rhs) const { return value != rhs.value; }
    
    /// Converts a grid line in OriginZero coordinates into the index of that same grid line in the GridTrackVec.
    size_t IntoTrackVecIndex(TrackCounts track_counts) const;
    
    /// Fallible version for absolutely positioned items
    std::optional<size_t> TryIntoTrackVecIndex(TrackCounts track_counts) const;
    
    /// The minimum number of negative implicit track there must be if a grid item starts at this line.
    uint16_t ImpliedNegativeImplicitTracks() const {
        return value < 0 ? static_cast<uint16_t>(-value) : 0;
    }
    
    /// The minimum number of positive implicit track there must be if a grid item end at this line.
    uint16_t ImpliedPositiveImplicitTracks(uint16_t explicit_track_count) const {
        return value > static_cast<int16_t>(explicit_track_count) 
            ? static_cast<uint16_t>(value) - explicit_track_count 
            : 0;
    }
};

/// Line span helper
inline uint16_t LineSpan(Line<OriginZeroLine> line) {
    int16_t diff = line.end.value - line.start.value;
    return static_cast<uint16_t>(diff > 0 ? diff : 0);
}

//------------------------------------------------------------------------------
// Track Counts
//------------------------------------------------------------------------------

/// Stores the number of tracks in each type of grid track
struct TrackCounts {
    /// The number of track in the implicit grid before the explicit grid
    uint16_t negative_implicit = 0;
    /// The number of tracks in the explicit grid
    uint16_t explicit_count = 0;
    /// The number of tracks in the implicit grid after the explicit grid
    uint16_t positive_implicit = 0;
    
    /// Total number of tracks
    uint16_t Total() const {
        return negative_implicit + explicit_count + positive_implicit;
    }
};

//------------------------------------------------------------------------------
// Grid Track Kind
//------------------------------------------------------------------------------

/// Whether a GridTrack represents an actual track or a gutter.
enum class GridTrackKind {
    /// Track is an actual track
    Track,
    /// Track is a gutter (aka grid line) (aka gap)
    Gutter
};

//------------------------------------------------------------------------------
// Min/Max Track Sizing Functions
//------------------------------------------------------------------------------

/// Minimum track sizing function
enum class MinTrackSizingFunctionType {
    Fixed,      // Fixed size (px or %)
    MinContent,
    MaxContent,
    Auto
};

struct MinTrackSizingFunction {
    MinTrackSizingFunctionType type = MinTrackSizingFunctionType::Auto;
    float value = 0.0f;  // For Fixed type
    bool is_percent = false;  // For Fixed type
    
    static MinTrackSizingFunction Fixed(float px) {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::Fixed, px, false};
    }
    static MinTrackSizingFunction Percent(float pct) {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::Fixed, pct, true};
    }
    static MinTrackSizingFunction MinContent() {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::MinContent, 0.0f, false};
    }
    static MinTrackSizingFunction MaxContent() {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::MaxContent, 0.0f, false};
    }
    static MinTrackSizingFunction Auto() {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::Auto, 0.0f, false};
    }
    static MinTrackSizingFunction Zero() {
        return MinTrackSizingFunction{MinTrackSizingFunctionType::Fixed, 0.0f, false};
    }
    
    bool IsIntrinsic() const {
        return type == MinTrackSizingFunctionType::MinContent ||
               type == MinTrackSizingFunctionType::MaxContent ||
               type == MinTrackSizingFunctionType::Auto;
    }
    
    bool UsesPercentage() const {
        return type == MinTrackSizingFunctionType::Fixed && is_percent;
    }
};

/// Maximum track sizing function
enum class MaxTrackSizingFunctionType {
    Fixed,      // Fixed size (px or %)
    MinContent,
    MaxContent,
    FitContent, // fit-content(limit)
    Auto,
    Fraction    // fr unit
};

struct MaxTrackSizingFunction {
    MaxTrackSizingFunctionType type = MaxTrackSizingFunctionType::Auto;
    float value = 0.0f;  // For Fixed, FitContent, Fraction types
    bool is_percent = false;  // For Fixed and FitContent types
    
    static MaxTrackSizingFunction Fixed(float px) {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::Fixed, px, false};
    }
    static MaxTrackSizingFunction Percent(float pct) {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::Fixed, pct, true};
    }
    static MaxTrackSizingFunction MinContent() {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::MinContent, 0.0f, false};
    }
    static MaxTrackSizingFunction MaxContent() {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::MaxContent, 0.0f, false};
    }
    static MaxTrackSizingFunction FitContentPx(float px) {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::FitContent, px, false};
    }
    static MaxTrackSizingFunction FitContentPercent(float pct) {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::FitContent, pct, true};
    }
    static MaxTrackSizingFunction Auto() {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::Auto, 0.0f, false};
    }
    static MaxTrackSizingFunction Fraction(float fr) {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::Fraction, fr, false};
    }
    static MaxTrackSizingFunction Zero() {
        return MaxTrackSizingFunction{MaxTrackSizingFunctionType::Fixed, 0.0f, false};
    }
    
    bool IsFr() const {
        return type == MaxTrackSizingFunctionType::Fraction;
    }
    
    bool IsIntrinsic() const {
        return type == MaxTrackSizingFunctionType::MinContent ||
               type == MaxTrackSizingFunctionType::MaxContent ||
               type == MaxTrackSizingFunctionType::FitContent ||
               type == MaxTrackSizingFunctionType::Auto;
    }
    
    bool UsesPercentage() const {
        return (type == MaxTrackSizingFunctionType::Fixed && is_percent) ||
               (type == MaxTrackSizingFunctionType::FitContent && is_percent);
    }
    
    float FitContentLimit(std::optional<float> axis_available_grid_space) const {
        if (type == MaxTrackSizingFunctionType::FitContent) {
            if (is_percent) {
                return axis_available_grid_space.has_value() 
                    ? *axis_available_grid_space * value 
                    : INFINITY;
            }
            return value;
        }
        return INFINITY;
    }
};

//------------------------------------------------------------------------------
// Grid Track
//------------------------------------------------------------------------------

/// Internal sizing information for a single grid track (row/column)
struct GridTrack {
    /// Whether the track is a full track, a gutter, or a placeholder
    GridTrackKind kind = GridTrackKind::Track;
    
    /// Whether the track is collapsed
    bool is_collapsed = false;
    
    /// The minimum track sizing function
    MinTrackSizingFunction min_track_sizing_function;
    
    /// The maximum track sizing function
    MaxTrackSizingFunction max_track_sizing_function;
    
    /// The distance of the start of the track from the start of the grid container
    float offset = 0.0f;
    
    /// The size (width/height as applicable) of the track
    float base_size = 0.0f;
    
    /// A temporary scratch value when sizing tracks (can be infinity)
    float growth_limit = 0.0f;
    
    /// Content alignment adjustment
    float content_alignment_adjustment = 0.0f;
    
    /// Temporary scratch values for distributing space
    float item_incurred_increase = 0.0f;
    float base_size_planned_increase = 0.0f;
    float growth_limit_planned_increase = 0.0f;
    bool infinitely_growable = false;
    
    /// Create new GridTrack representing an actual track
    static GridTrack New(MinTrackSizingFunction min_func, MaxTrackSizingFunction max_func) {
        GridTrack track;
        track.kind = GridTrackKind::Track;
        track.min_track_sizing_function = min_func;
        track.max_track_sizing_function = max_func;
        return track;
    }
    
    /// Create a new GridTrack representing a gutter
    static GridTrack Gutter(float size) {
        GridTrack track;
        track.kind = GridTrackKind::Gutter;
        track.min_track_sizing_function = MinTrackSizingFunction::Fixed(size);
        track.max_track_sizing_function = MaxTrackSizingFunction::Fixed(size);
        return track;
    }
    
    /// Mark a GridTrack as collapsed
    void Collapse() {
        is_collapsed = true;
        min_track_sizing_function = MinTrackSizingFunction::Zero();
        max_track_sizing_function = MaxTrackSizingFunction::Zero();
    }
    
    /// Returns true if the track is flexible (has a Flex MaxTrackSizingFunction)
    bool IsFlexible() const {
        return max_track_sizing_function.IsFr();
    }
    
    /// Returns true if the track uses percentage sizing
    bool UsesPercentage() const {
        return min_track_sizing_function.UsesPercentage() || 
               max_track_sizing_function.UsesPercentage();
    }
    
    /// Returns true if the track has an intrinsic sizing function
    bool HasIntrinsicSizingFunction() const {
        return min_track_sizing_function.IsIntrinsic() || 
               max_track_sizing_function.IsIntrinsic();
    }
    
    /// Returns the fit-content limit
    float FitContentLimit(std::optional<float> axis_available_grid_space) const {
        return max_track_sizing_function.FitContentLimit(axis_available_grid_space);
    }
    
    /// Returns the fit-content limited growth limit
    float FitContentLimitedGrowthLimit(std::optional<float> axis_available_grid_space) const {
        return std::min(growth_limit, FitContentLimit(axis_available_grid_space));
    }
    
    /// Returns the track's flex factor if it is a flex track, else 0
    float FlexFactor() const {
        return max_track_sizing_function.IsFr() ? max_track_sizing_function.value : 0.0f;
    }
};

//------------------------------------------------------------------------------
// Implementation of coordinate conversions
//------------------------------------------------------------------------------

inline OriginZeroLine GridLine::IntoOriginZeroLine(uint16_t explicit_track_count) const {
    int16_t explicit_line_count = static_cast<int16_t>(explicit_track_count + 1);
    int16_t oz_line;
    if (value > 0) {
        oz_line = value - 1;
    } else if (value < 0) {
        oz_line = value + explicit_line_count;
    } else {
        // Grid line of zero is invalid - should not happen
        oz_line = 0;
    }
    return OriginZeroLine(oz_line);
}

inline size_t OriginZeroLine::IntoTrackVecIndex(TrackCounts track_counts) const {
    auto result = TryIntoTrackVecIndex(track_counts);
    if (!result.has_value()) {
        // This should not happen in normal usage
        return 0;
    }
    return *result;
}

inline std::optional<size_t> OriginZeroLine::TryIntoTrackVecIndex(TrackCounts track_counts) const {
    // OriginZero grid line cannot be less than the number of negative grid lines
    if (value < -static_cast<int16_t>(track_counts.negative_implicit)) {
        return std::nullopt;
    }
    // OriginZero grid line cannot be more than the number of positive grid lines
    if (value > static_cast<int16_t>(track_counts.explicit_count + track_counts.positive_implicit)) {
        return std::nullopt;
    }
    
    return static_cast<size_t>(2 * (value + static_cast<int16_t>(track_counts.negative_implicit)));
}

} // namespace lightui

