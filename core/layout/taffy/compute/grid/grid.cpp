/**
 * @file grid.cpp
 * @brief CSS Grid layout algorithm implementation
 *
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/compute/grid/mod.rs
 *
 * This is a partial implementation of the CSS Grid Level 1 specification
 * https://www.w3.org/TR/css-grid-1
 */

#include "grid.h"
#include "../../util/math.h"
#include "../../util/resolve.h"
#include <algorithm>
#include <cmath>

namespace lightui {

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

/// Resolve a LengthPercentage to a float value
static float ResolveLengthPercentageValue(const LengthPercentage& lp, float parent_size) {
    if (lp.IsLength()) {
        return lp.value;
    } else if (lp.IsPercent()) {
        return lp.value * parent_size;
    }
    return 0.0f;
}

/// Compute the explicit grid size in one axis
static std::pair<uint16_t, uint16_t> ComputeExplicitGridSizeInAxis(
    const GridContainerStyle& style,
    std::optional<float> available_space,
    AbsoluteAxis axis
) {
    const auto& template_tracks = (axis == AbsoluteAxis::Horizontal) 
        ? style.grid_template_columns 
        : style.grid_template_rows;
    
    uint16_t auto_repetition_count = 1;
    uint16_t track_count = 0;
    
    for (const auto& track : template_tracks) {
        if (track.type == TrackSizingFunction::Type::Single) {
            track_count++;
        } else {
            // Repeat
            if (track.repeat_count == 0) {
                // auto-fill: compute based on available space
                // For now, use 1 as default
                auto_repetition_count = 1;
                track_count += static_cast<uint16_t>(track.repeat_tracks.size());
            } else if (track.repeat_count == UINT16_MAX) {
                // auto-fit: similar to auto-fill
                auto_repetition_count = 1;
                track_count += static_cast<uint16_t>(track.repeat_tracks.size());
            } else {
                // Fixed repeat count
                track_count += track.repeat_count * static_cast<uint16_t>(track.repeat_tracks.size());
            }
        }
    }
    
    return {auto_repetition_count, track_count};
}

/// Initialize grid tracks from template
static void InitializeGridTracks(
    std::vector<GridTrack>& tracks,
    TrackCounts track_counts,
    const GridContainerStyle& style,
    AbsoluteAxis axis,
    float gap
) {
    const auto& template_tracks = (axis == AbsoluteAxis::Horizontal) 
        ? style.grid_template_columns 
        : style.grid_template_rows;
    const auto& auto_tracks = (axis == AbsoluteAxis::Horizontal) 
        ? style.grid_auto_columns 
        : style.grid_auto_rows;
    
    tracks.clear();
    
    // Add negative implicit tracks
    for (uint16_t i = 0; i < track_counts.negative_implicit; i++) {
        // Add gutter before track (except for first)
        if (!tracks.empty()) {
            tracks.push_back(GridTrack::Gutter(gap));
        }
        
        // Add implicit track
        if (!auto_tracks.empty()) {
            size_t idx = i % auto_tracks.size();
            tracks.push_back(GridTrack::New(auto_tracks[idx].min, auto_tracks[idx].max));
        } else {
            tracks.push_back(GridTrack::New(MinTrackSizingFunction::Auto(), MaxTrackSizingFunction::Auto()));
        }
    }
    
    // Add explicit tracks
    for (const auto& track_func : template_tracks) {
        if (track_func.type == TrackSizingFunction::Type::Single) {
            // Add gutter before track (except for first)
            if (!tracks.empty()) {
                tracks.push_back(GridTrack::Gutter(gap));
            }
            tracks.push_back(GridTrack::New(track_func.single.min, track_func.single.max));
        } else {
            // Repeat
            uint16_t count = (track_func.repeat_count == 0 || track_func.repeat_count == UINT16_MAX) 
                ? 1 
                : track_func.repeat_count;
            for (uint16_t r = 0; r < count; r++) {
                for (const auto& repeat_track : track_func.repeat_tracks) {
                    if (!tracks.empty()) {
                        tracks.push_back(GridTrack::Gutter(gap));
                    }
                    tracks.push_back(GridTrack::New(repeat_track.min, repeat_track.max));
                }
            }
        }
    }
    
    // Add positive implicit tracks
    for (uint16_t i = 0; i < track_counts.positive_implicit; i++) {
        if (!tracks.empty()) {
            tracks.push_back(GridTrack::Gutter(gap));
        }
        
        if (!auto_tracks.empty()) {
            size_t idx = i % auto_tracks.size();
            tracks.push_back(GridTrack::New(auto_tracks[idx].min, auto_tracks[idx].max));
        } else {
            tracks.push_back(GridTrack::New(MinTrackSizingFunction::Auto(), MaxTrackSizingFunction::Auto()));
        }
    }
}

/// Resolve track base sizes
static void ResolveTrackBaseSizes(
    std::vector<GridTrack>& tracks,
    std::optional<float> available_space
) {
    for (auto& track : tracks) {
        if (track.kind == GridTrackKind::Gutter) {
            // Gutters have fixed size
            track.base_size = track.min_track_sizing_function.value;
            track.growth_limit = track.base_size;
            continue;
        }
        
        // Resolve min track sizing function
        float min_size = 0.0f;
        switch (track.min_track_sizing_function.type) {
            case MinTrackSizingFunctionType::Fixed:
                if (track.min_track_sizing_function.is_percent && available_space.has_value()) {
                    min_size = track.min_track_sizing_function.value * *available_space;
                } else if (!track.min_track_sizing_function.is_percent) {
                    min_size = track.min_track_sizing_function.value;
                }
                break;
            case MinTrackSizingFunctionType::MinContent:
            case MinTrackSizingFunctionType::MaxContent:
            case MinTrackSizingFunctionType::Auto:
                min_size = 0.0f;  // Will be resolved during track sizing
                break;
        }
        
        // Resolve max track sizing function
        float max_size = INFINITY;
        switch (track.max_track_sizing_function.type) {
            case MaxTrackSizingFunctionType::Fixed:
                if (track.max_track_sizing_function.is_percent && available_space.has_value()) {
                    max_size = track.max_track_sizing_function.value * *available_space;
                } else if (!track.max_track_sizing_function.is_percent) {
                    max_size = track.max_track_sizing_function.value;
                }
                break;
            case MaxTrackSizingFunctionType::MinContent:
            case MaxTrackSizingFunctionType::MaxContent:
            case MaxTrackSizingFunctionType::FitContent:
            case MaxTrackSizingFunctionType::Auto:
                max_size = INFINITY;
                break;
            case MaxTrackSizingFunctionType::Fraction:
                max_size = INFINITY;  // Fr tracks are sized later
                break;
        }
        
        track.base_size = min_size;
        track.growth_limit = std::max(min_size, max_size);
    }
}

/// Distribute free space to flexible tracks
static void DistributeFreeSpaceToFlexTracks(
    std::vector<GridTrack>& tracks,
    float free_space
) {
    if (free_space <= 0.0f) return;
    
    // Calculate total flex factor
    float total_flex = 0.0f;
    for (const auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            total_flex += track.FlexFactor();
        }
    }
    
    if (total_flex <= 0.0f) return;
    
    // Distribute space proportionally
    for (auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            float share = (track.FlexFactor() / total_flex) * free_space;
            track.base_size = std::max(track.base_size, share);
            track.growth_limit = track.base_size;
        }
    }
}

/// Calculate track offsets
static void CalculateTrackOffsets(std::vector<GridTrack>& tracks, float start_offset) {
    float offset = start_offset;
    for (auto& track : tracks) {
        track.offset = offset;
        offset += track.base_size;
    }
}

/// Sum of all track base sizes
static float SumTrackBaseSizes(const std::vector<GridTrack>& tracks) {
    float sum = 0.0f;
    for (const auto& track : tracks) {
        sum += track.base_size;
    }
    return sum;
}

//------------------------------------------------------------------------------
// Main Grid Layout Function
//------------------------------------------------------------------------------

LayoutOutput ComputeGridLayout(
    LayoutGridContainer& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    const auto& style = tree.GetGridContainerStyle(node);

    // Extract inputs
    auto known_dimensions = inputs.known_dimensions;
    auto parent_size = inputs.parent_size;
    auto available_space = inputs.available_space;
    auto run_mode = inputs.run_mode;

    // 1. Compute "available grid space"
    auto aspect_ratio = style.aspect_ratio;
    auto padding = ResolveOrZero(style.padding, parent_size.width);
    auto border = ResolveOrZero(style.border, parent_size.width);
    auto padding_border = RectAdd(padding, border);
    auto padding_border_size = Size<float>{
        padding_border.left + padding_border.right,
        padding_border.top + padding_border.bottom
    };

    // Resolve sizes
    auto min_size = MaybeResolve(style.min_size, parent_size);
    auto max_size = MaybeResolve(style.max_size, parent_size);
    auto preferred_size = (inputs.sizing_mode == SizingMode::InherentSize)
        ? MaybeResolve(style.size, parent_size)
        : Size<std::optional<float>>{std::nullopt, std::nullopt};

    // Compute outer node size
    auto outer_node_size = MaybeClamp(
        MaybeOr(known_dimensions, preferred_size),
        min_size,
        max_size
    );
    outer_node_size = MaybeMaxOpt(outer_node_size, Size<std::optional<float>>{
        std::optional<float>(padding_border_size.width),
        std::optional<float>(padding_border_size.height)
    });

    // Early return if we can compute size directly
    if (run_mode == RunMode::ComputeSize &&
        outer_node_size.width.has_value() &&
        outer_node_size.height.has_value()) {
        LayoutOutput output;
        output.size = Size<float>{*outer_node_size.width, *outer_node_size.height};
        return output;
    }

    // Compute inner node size (content box)
    auto inner_node_size = Size<std::optional<float>>{
        outer_node_size.width.has_value()
            ? std::optional<float>(*outer_node_size.width - padding_border.left - padding_border.right)
            : std::nullopt,
        outer_node_size.height.has_value()
            ? std::optional<float>(*outer_node_size.height - padding_border.top - padding_border.bottom)
            : std::nullopt
    };

    // 2. Resolve the explicit grid
    auto [col_auto_rep, col_count] = ComputeExplicitGridSizeInAxis(style, inner_node_size.width, AbsoluteAxis::Horizontal);
    auto [row_auto_rep, row_count] = ComputeExplicitGridSizeInAxis(style, inner_node_size.height, AbsoluteAxis::Vertical);

    // 3. Estimate track counts (simplified - just use explicit counts for now)
    TrackCounts col_counts{0, col_count, 0};
    TrackCounts row_counts{0, row_count, 0};

    // 4. Initialize tracks
    float col_gap = ResolveLengthPercentageValue(style.column_gap, inner_node_size.width.value_or(0.0f));
    float row_gap = ResolveLengthPercentageValue(style.row_gap, inner_node_size.height.value_or(0.0f));

    std::vector<GridTrack> columns;
    std::vector<GridTrack> rows;
    InitializeGridTracks(columns, col_counts, style, AbsoluteAxis::Horizontal, col_gap);
    InitializeGridTracks(rows, row_counts, style, AbsoluteAxis::Vertical, row_gap);

    // 5. Resolve track base sizes
    ResolveTrackBaseSizes(columns, inner_node_size.width);
    ResolveTrackBaseSizes(rows, inner_node_size.height);

    // 6. Distribute free space to flexible tracks
    float col_sum = SumTrackBaseSizes(columns);
    float row_sum = SumTrackBaseSizes(rows);

    if (inner_node_size.width.has_value()) {
        float free_space = *inner_node_size.width - col_sum;
        DistributeFreeSpaceToFlexTracks(columns, free_space);
        col_sum = SumTrackBaseSizes(columns);
    }

    if (inner_node_size.height.has_value()) {
        float free_space = *inner_node_size.height - row_sum;
        DistributeFreeSpaceToFlexTracks(rows, free_space);
        row_sum = SumTrackBaseSizes(rows);
    }

    // 7. Compute container size
    float container_width = outer_node_size.width.value_or(col_sum + padding_border_size.width);
    float container_height = outer_node_size.height.value_or(row_sum + padding_border_size.height);

    // Clamp to min/max
    if (min_size.width.has_value()) container_width = f32_max(container_width, *min_size.width);
    if (max_size.width.has_value()) container_width = f32_min(container_width, *max_size.width);
    if (min_size.height.has_value()) container_height = f32_max(container_height, *min_size.height);
    if (max_size.height.has_value()) container_height = f32_min(container_height, *max_size.height);

    Size<float> container_size{container_width, container_height};

    // If only size requested, return early
    if (run_mode == RunMode::ComputeSize) {
        LayoutOutput output;
        output.size = container_size;
        return output;
    }

    // 8. Calculate track offsets
    CalculateTrackOffsets(columns, padding_border.left);
    CalculateTrackOffsets(rows, padding_border.top);

    // 9. Layout children
    // TODO: Implement full grid item placement and layout
    // For now, just layout children in their grid areas

    Size<float> content_size{col_sum, row_sum};

    LayoutOutput output;
    output.size = container_size;
    output.content_size = content_size;
    return output;
}

} // namespace lightui

