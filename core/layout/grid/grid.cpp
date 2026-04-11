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
#include "../util/math.h"
#include "../util/resolve.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mbink {

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

/// Get the minimum size from a MinTrackSizingFunction for auto-repeat calculation
static float GetMinTrackSize(const MinTrackSizingFunction& min_func, float available_space) {
    if (min_func.type == MinTrackSizingFunctionType::Fixed) {
        if (min_func.is_percent) {
            return min_func.value * available_space;
        }
        return min_func.value;
    }
    // For auto, min-content, max-content, use a reasonable default
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

    // Get gap for this axis
    float gap = (axis == AbsoluteAxis::Horizontal)
        ? (style.column_gap.IsLength() ? style.column_gap.value : 0.0f)
        : (style.row_gap.IsLength() ? style.row_gap.value : 0.0f);

    uint16_t auto_repetition_count = 1;
    uint16_t track_count = 0;

    for (const auto& track : template_tracks) {
        if (track.type == TrackSizingFunction::Type::Single) {
            track_count++;
        } else {
            // Repeat
            if (track.repeat_count == 0 || track.repeat_count == UINT16_MAX) {
                // auto-fill (0) or auto-fit (UINT16_MAX): compute based on available space
                if (available_space.has_value() && !track.repeat_tracks.empty()) {
                    float space = available_space.value();

                    // Calculate the minimum size of one repetition (sum of all tracks in repeat)
                    float min_repetition_size = 0.0f;
                    for (const auto& repeat_track : track.repeat_tracks) {
                        float track_min = GetMinTrackSize(repeat_track.min, space);
                        min_repetition_size += track_min;
                    }

                    // Add gaps between tracks within one repetition
                    if (track.repeat_tracks.size() > 1) {
                        min_repetition_size += gap * (track.repeat_tracks.size() - 1);
                    }

                    if (min_repetition_size > 0.0f) {
                        // Calculate how many repetitions fit
                        // Formula: floor((available_space + gap) / (min_repetition_size + gap))
                        // This accounts for gaps between repetitions
                        float effective_space = space + gap;
                        float effective_track_size = min_repetition_size + gap;
                        auto_repetition_count = static_cast<uint16_t>(
                            std::max(1.0f, std::floor(effective_space / effective_track_size))
                        );
                    } else {
                        // If min size is 0 (e.g., auto), use a reasonable default
                        auto_repetition_count = 1;
                    }
                } else {
                    // No available space, use 1 as default
                    auto_repetition_count = 1;
                }
                track_count += auto_repetition_count * static_cast<uint16_t>(track.repeat_tracks.size());
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
    float gap,
    uint16_t auto_repetition_count
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
            uint16_t count;
            if (track_func.repeat_count == 0 || track_func.repeat_count == UINT16_MAX) {
                // auto-fill or auto-fit: use the calculated auto_repetition_count
                count = auto_repetition_count;
            } else {
                // Fixed repeat count
                count = track_func.repeat_count;
            }
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
static float ResolveMinTrackBaseSize(
    const GridTrack& track,
    std::optional<float> available_space
) {
    if (track.kind == GridTrackKind::Gutter) {
        return track.min_track_sizing_function.value;
    }

    switch (track.min_track_sizing_function.type) {
        case MinTrackSizingFunctionType::Fixed:
            if (track.min_track_sizing_function.is_percent && available_space.has_value()) {
                return track.min_track_sizing_function.value * *available_space;
            }
            if (!track.min_track_sizing_function.is_percent) {
                return track.min_track_sizing_function.value;
            }
            return 0.0f;
        case MinTrackSizingFunctionType::MinContent:
        case MinTrackSizingFunctionType::MaxContent:
        case MinTrackSizingFunctionType::Auto:
            return 0.0f;
    }

    return 0.0f;
}

static float ResolveMaxTrackGrowthLimit(
    const GridTrack& track,
    std::optional<float> available_space
) {
    if (track.kind == GridTrackKind::Gutter) {
        return track.min_track_sizing_function.value;
    }

    switch (track.max_track_sizing_function.type) {
        case MaxTrackSizingFunctionType::Fixed:
            if (track.max_track_sizing_function.is_percent && available_space.has_value()) {
                return track.max_track_sizing_function.value * *available_space;
            }
            if (!track.max_track_sizing_function.is_percent) {
                return track.max_track_sizing_function.value;
            }
            return INFINITY;
        case MaxTrackSizingFunctionType::FitContent:
            if (track.max_track_sizing_function.is_percent && available_space.has_value()) {
                return track.max_track_sizing_function.value * *available_space;
            }
            if (!track.max_track_sizing_function.is_percent) {
                return track.max_track_sizing_function.value;
            }
            return INFINITY;
        case MaxTrackSizingFunctionType::MinContent:
        case MaxTrackSizingFunctionType::MaxContent:
        case MaxTrackSizingFunctionType::Auto:
        case MaxTrackSizingFunctionType::Fraction:
            return INFINITY;
    }

    return INFINITY;
}

static bool TrackUsesIntrinsicMinSizing(const GridTrack& track) {
    return track.kind == GridTrackKind::Track && (
        track.min_track_sizing_function.type == MinTrackSizingFunctionType::Auto ||
        track.min_track_sizing_function.type == MinTrackSizingFunctionType::MinContent ||
        track.min_track_sizing_function.type == MinTrackSizingFunctionType::MaxContent
    );
}

static bool TrackUsesIntrinsicMaxSizing(const GridTrack& track) {
    return track.kind == GridTrackKind::Track && (
        track.max_track_sizing_function.type == MaxTrackSizingFunctionType::Auto ||
        track.max_track_sizing_function.type == MaxTrackSizingFunctionType::MinContent ||
        track.max_track_sizing_function.type == MaxTrackSizingFunctionType::MaxContent ||
        track.max_track_sizing_function.type == MaxTrackSizingFunctionType::FitContent
    );
}

/// Resolve track base sizes
static void ResolveTrackBaseSizes(
    std::vector<GridTrack>& tracks,
    std::optional<float> available_space
) {
    for (auto& track : tracks) {
        float min_size = ResolveMinTrackBaseSize(track, available_space);
        float max_size = ResolveMaxTrackGrowthLimit(track, available_space);
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

    float total_flex = 0.0f;
    for (const auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            total_flex += track.FlexFactor();
        }
    }

    if (total_flex <= 0.0f) return;

    for (auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            float share = (track.FlexFactor() / total_flex) * free_space;
            track.base_size += share;
            track.growth_limit = std::max(track.growth_limit, track.base_size);
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


/// Stretch tracks to fill a definite available size.
/// Non-flex tracks keep their intrinsic/fixed size; flex tracks are reset to their minimum
/// contribution first, then receive the remaining free space.
static void StretchTracksToAvailableSpace(
    std::vector<GridTrack>& tracks,
    float available_space
) {
    if (available_space <= 0.0f) return;

    for (auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            float min_size = ResolveMinTrackBaseSize(track, available_space);
            float max_size = ResolveMaxTrackGrowthLimit(track, available_space);
            track.base_size = min_size;
            track.growth_limit = std::max(min_size, max_size);
        }
    }

    float used_space = SumTrackBaseSizes(tracks);
    if (used_space >= available_space) return;

    float extra_space = available_space - used_space;
    float total_flex = 0.0f;
    for (const auto& track : tracks) {
        if (track.kind == GridTrackKind::Track && track.IsFlexible()) {
            total_flex += track.FlexFactor();
        }
    }

    if (total_flex > 0.0f) {
        DistributeFreeSpaceToFlexTracks(tracks, extra_space);
        return;
    }

    bool grew = true;
    while (extra_space > 0.0f && grew) {
        grew = false;
        size_t growable_tracks = 0;
        for (const auto& track : tracks) {
            if (track.kind != GridTrackKind::Track || track.IsFlexible()) continue;
            if (!std::isfinite(track.growth_limit) || track.base_size < track.growth_limit - 0.001f) {
                growable_tracks++;
            }
        }

        if (growable_tracks == 0) return;

        float extra_per_track = extra_space / static_cast<float>(growable_tracks);
        float consumed = 0.0f;
        for (auto& track : tracks) {
            if (track.kind != GridTrackKind::Track || track.IsFlexible()) continue;

            float target = track.base_size + extra_per_track;
            float capped = std::isfinite(track.growth_limit)
                ? f32_min(target, track.growth_limit)
                : target;
            if (capped > track.base_size) {
                consumed += capped - track.base_size;
                track.base_size = capped;
                grew = true;
            }
        }

        if (!grew || consumed <= 0.0f) return;
        extra_space -= consumed;
    }
}



//------------------------------------------------------------------------------
// Main Grid Layout Function
//------------------------------------------------------------------------------

LayoutOutput ComputeGridLayout(
    LayoutGridContainer& tree,
    NodeId node,
    const LayoutInput& inputs
) {
    // Get unified Style for common properties
    const auto& style = tree.GetContainerStyle(node);
    // Get Grid-specific data for grid-template-columns/rows, grid-auto-rows/columns, etc.
    const auto& grid_style = tree.GetGridContainerStyle(node);

    // Extract inputs
    auto known_dimensions = inputs.known_dimensions;
    auto parent_size = inputs.parent_size;
    auto available_space = inputs.available_space;
    auto run_mode = inputs.run_mode;

    // 1. Compute "available grid space"
    // Read common properties from unified Style
    auto aspect_ratio = style.aspect_ratio;
    auto padding = ResolveOrZero(style.padding, parent_size.width);
    auto border = ResolveOrZero(style.border, parent_size.width);
    auto padding_border = RectAdd(padding, border);
    auto padding_border_size = Size<float>{
        padding_border.left + padding_border.right,
        padding_border.top + padding_border.bottom
    };

    // Resolve sizes from unified Style
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

    // For grid containers with auto width, use definite available inline space.
    // Height is intentionally not inferred from available_space here: in normal
    // flow an auto-height grid must stay content-sized, otherwise it can wrongly
    // expand to fill an ancestor's available block size.
    if (!outer_node_size.width.has_value() && available_space.width.IsDefinite()) {
        outer_node_size.width = std::optional<float>(available_space.width.value);
    }

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
    // Use grid_style for Grid-specific properties (grid-template-columns/rows)
    auto [col_auto_rep, col_count] = ComputeExplicitGridSizeInAxis(grid_style, inner_node_size.width, AbsoluteAxis::Horizontal);
    auto [row_auto_rep, row_count] = ComputeExplicitGridSizeInAxis(grid_style, inner_node_size.height, AbsoluteAxis::Vertical);

    // 3. Calculate required rows for auto-placement
    // Count only element children (skip text nodes)
    size_t total_child_count = tree.ChildCount(node);
    size_t grid_item_count = 0;
    for (size_t i = 0; i < total_child_count; i++) {
        NodeId child_id = tree.GetChildId(node, i);
        if (!tree.IsTextNode(child_id)) {
            grid_item_count++;
        }
    }

    size_t num_cols = col_count > 0 ? col_count : 1;
    size_t num_rows = row_count > 0 ? row_count : 1;

    // Expand rows if needed for auto-placement
    while (num_rows * num_cols < grid_item_count) {
        num_rows++;
    }

    // Update implicit track counts needed for auto-placement.
    // When no explicit grid-template-columns is provided, we still need at least
    // one implicit column track so auto-placed children have a real cell width.
    uint16_t implicit_cols_needed = 0;
    if (col_count == 0 && grid_item_count > 0) {
        implicit_cols_needed = static_cast<uint16_t>(num_cols);
    } else if (num_cols > col_count) {
        implicit_cols_needed = static_cast<uint16_t>(num_cols - col_count);
    }

    uint16_t implicit_rows_needed = 0;
    if (row_count == 0 && grid_item_count > 0) {
        // No explicit rows, all rows are implicit
        implicit_rows_needed = static_cast<uint16_t>(num_rows);
    } else if (num_rows > row_count) {
        // Some implicit rows needed beyond explicit rows
        implicit_rows_needed = static_cast<uint16_t>(num_rows - row_count);
    }

    // 4. Estimate track counts
    TrackCounts col_counts{0, col_count, implicit_cols_needed};
    // For rows: if no explicit rows, put all needed rows as positive_implicit
    TrackCounts row_counts{0, row_count, implicit_rows_needed};

    // 4. Initialize tracks
    // Read gap from unified Style (style.gap contains row and column gaps)
    float col_gap = ResolveLengthPercentageValue(style.gap.width, inner_node_size.width.value_or(0.0f));
    float row_gap = ResolveLengthPercentageValue(style.gap.height, inner_node_size.height.value_or(0.0f));

    std::vector<GridTrack> columns;
    std::vector<GridTrack> rows;
    // Use grid_style for Grid-specific properties (grid-template-columns/rows, grid-auto-columns/rows)
    // Pass auto_repetition_count for auto-fit/auto-fill support
    InitializeGridTracks(columns, col_counts, grid_style, AbsoluteAxis::Horizontal, col_gap, col_auto_rep);
    InitializeGridTracks(rows, row_counts, grid_style, AbsoluteAxis::Vertical, row_gap, row_auto_rep);

    // 5. Resolve track base sizes
    ResolveTrackBaseSizes(columns, inner_node_size.width);
    ResolveTrackBaseSizes(rows, inner_node_size.height);

    // 6. Distribute free space to flexible tracks
    float col_sum = SumTrackBaseSizes(columns);
    float row_sum = SumTrackBaseSizes(rows);

    if (inner_node_size.width.has_value()) {
        StretchTracksToAvailableSpace(columns, *inner_node_size.width);
        col_sum = SumTrackBaseSizes(columns);
    }

    if (inner_node_size.height.has_value()) {
        StretchTracksToAvailableSpace(rows, *inner_node_size.height);
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

    float inner_width = container_width - padding_border_size.width;
    if (inner_width > 0.0f) {
        StretchTracksToAvailableSpace(columns, inner_width);
        col_sum = SumTrackBaseSizes(columns);
    }

    Size<float> container_size{container_width, container_height};

    // 8. Calculate track offsets
    CalculateTrackOffsets(columns, padding_border.left);
    CalculateTrackOffsets(rows, padding_border.top);

    // 9. Layout children - place items in grid cells
    // (child_count, num_cols, num_rows already calculated above)

    // Calculate required number of track slots (tracks + gutters between them)
    // For n tracks, we need: track, gutter, track, gutter, ..., track = 2*n - 1 slots
    size_t required_slots = num_rows > 0 ? num_rows * 2 - 1 : 0;

    // Track index for cycling through grid_auto_rows
    size_t auto_row_idx = 0;

    // Ensure we have enough row tracks (including gutters)
    while (rows.size() < required_slots) {
        if (!rows.empty()) {
            GridTrack gutter = GridTrack::Gutter(row_gap);
            // Set base_size for gutter immediately
            gutter.base_size = row_gap;
            gutter.growth_limit = row_gap;
            rows.push_back(gutter);
        }
        // Use grid_auto_rows if available, otherwise use Auto
        // Read from grid_style for Grid-specific properties
        if (!grid_style.grid_auto_rows.empty()) {
            const auto& auto_track = grid_style.grid_auto_rows[auto_row_idx % grid_style.grid_auto_rows.size()];
            auto_row_idx++;
            GridTrack track = GridTrack::New(auto_track.min, auto_track.max);
            // Resolve the track size immediately for fixed values
            if (auto_track.min.type == MinTrackSizingFunctionType::Fixed) {
                if (auto_track.min.is_percent && inner_node_size.height.has_value()) {
                    track.base_size = auto_track.min.value * *inner_node_size.height;
                } else if (!auto_track.min.is_percent) {
                    track.base_size = auto_track.min.value;
                }
            }
            if (auto_track.max.type == MaxTrackSizingFunctionType::Fixed) {
                if (auto_track.max.is_percent && inner_node_size.height.has_value()) {
                    track.growth_limit = auto_track.max.value * *inner_node_size.height;
                } else if (!auto_track.max.is_percent) {
                    track.growth_limit = auto_track.max.value;
                }
            }
            if (track.growth_limit < track.base_size) {
                track.growth_limit = track.base_size;
            }
            rows.push_back(track);
        } else {
            rows.push_back(GridTrack::New(MinTrackSizingFunction::Auto(), MaxTrackSizingFunction::Auto()));
        }
    }

    // Structure to hold child placement info
    struct ChildPlacement {
        NodeId child_id;
        size_t col_idx;
        size_t row_idx;
        size_t col_span;
        size_t row_span;
        float measured_width;
        float measured_height;
        float min_content_width;
        float max_content_width;
        float min_content_height;
        float max_content_height;
    };
    std::vector<ChildPlacement> placements;
    placements.reserve(grid_item_count);

    // Cell occupancy matrix to track which cells are occupied
    // True means the cell is occupied
    std::vector<std::vector<bool>> occupied(num_rows, std::vector<bool>(num_cols, false));

    // Helper lambda to find next available cell
    auto findNextAvailableCell = [&](size_t& col, size_t& row, size_t col_span, size_t row_span) {
        while (true) {
            // Check if current position can fit the item
            bool can_fit = true;
            if (col + col_span > num_cols) {
                can_fit = false;
            } else if (row + row_span > num_rows) {
                // Need to expand rows
                size_t new_rows = row + row_span;
                occupied.resize(new_rows, std::vector<bool>(num_cols, false));
                num_rows = new_rows;
            }

            if (can_fit) {
                // Check if all cells in the span area are free
                for (size_t r = row; r < row + row_span && can_fit; r++) {
                    for (size_t c = col; c < col + col_span && can_fit; c++) {
                        if (occupied[r][c]) {
                            can_fit = false;
                        }
                    }
                }
            }

            if (can_fit) {
                return; // Found a valid position
            }

            // Move to next cell
            col++;
            if (col >= num_cols) {
                col = 0;
                row++;
                // Expand rows if needed
                if (row >= occupied.size()) {
                    occupied.push_back(std::vector<bool>(num_cols, false));
                    num_rows = occupied.size();
                }
            }
        }
    };

    // Helper lambda to mark cells as occupied
    auto markCellsOccupied = [&](size_t col, size_t row, size_t col_span, size_t row_span) {
        // Ensure we have enough rows
        while (row + row_span > occupied.size()) {
            occupied.push_back(std::vector<bool>(num_cols, false));
        }
        for (size_t r = row; r < row + row_span; r++) {
            for (size_t c = col; c < col + col_span; c++) {
                if (c < num_cols) {
                    occupied[r][c] = true;
                }
            }
        }
    };

    // First pass: determine placement and measure children
    size_t current_col = 0;
    size_t current_row = 0;

    for (size_t i = 0; i < total_child_count; i++) {
        NodeId child_id = tree.GetChildId(node, i);

        // Skip text nodes - they don't participate in grid layout
        if (tree.IsTextNode(child_id)) {
            continue;
        }

        // Get Grid-specific item style for placement properties (grid-row-start/end, grid-column-start/end)
        const auto& grid_item_style = tree.GetGridItemStyle(child_id);

        // Determine grid cell for this child
        size_t col_idx = current_col;
        size_t row_idx = current_row;
        size_t col_span = 1;
        size_t row_span = 1;
        bool has_explicit_col = false;
        bool has_explicit_row = false;

        // Check for explicit placement (Grid-specific properties)
        if (grid_item_style.grid_column_start.IsLine()) {
            int16_t line = grid_item_style.grid_column_start.value;
            if (line > 0) col_idx = static_cast<size_t>(line - 1);
            else if (line < 0) col_idx = num_cols + line;
            has_explicit_col = true;
        }
        if (grid_item_style.grid_row_start.IsLine()) {
            int16_t line = grid_item_style.grid_row_start.value;
            if (line > 0) row_idx = static_cast<size_t>(line - 1);
            else if (line < 0) row_idx = num_rows + line;
            has_explicit_row = true;
        }

        // Check for span (Grid-specific properties)
        if (grid_item_style.grid_column_end.IsSpan()) {
            col_span = static_cast<size_t>(grid_item_style.grid_column_end.value);
        } else if (grid_item_style.grid_column_end.IsLine()) {
            int16_t end_line = grid_item_style.grid_column_end.value;
            // CSS Grid lines are 1-based. For N columns, there are N+1 lines (1 to N+1).
            // Negative indices count from the end: -1 is the last line (N+1), -2 is N, etc.
            // So for end_line = -1 with 4 columns: end_idx = 4 + 1 + (-1) = 4 (meaning span to column 4)
            size_t end_idx = end_line > 0 ? static_cast<size_t>(end_line - 1) : num_cols + 1 + end_line;
            if (end_idx > col_idx) col_span = end_idx - col_idx;
        }
        if (grid_item_style.grid_row_end.IsSpan()) {
            row_span = static_cast<size_t>(grid_item_style.grid_row_end.value);
        } else if (grid_item_style.grid_row_end.IsLine()) {
            int16_t end_line = grid_item_style.grid_row_end.value;
            // Same logic for rows
            size_t end_idx = end_line > 0 ? static_cast<size_t>(end_line - 1) : num_rows + 1 + end_line;
            if (end_idx > row_idx) row_span = end_idx - row_idx;
        }

        // For auto-placed items, find the next available cell that can fit the span
        if (!has_explicit_col && !has_explicit_row) {
            findNextAvailableCell(current_col, current_row, col_span, row_span);
            col_idx = current_col;
            row_idx = current_row;
        } else if (has_explicit_col && !has_explicit_row) {
            // Has explicit column but not row - find next available row at this column
            row_idx = current_row;
            // Check if the cells are occupied and find next available row
            while (row_idx < occupied.size()) {
                bool can_place = true;
                for (size_t c = col_idx; c < col_idx + col_span && c < num_cols; c++) {
                    if (row_idx < occupied.size() && c < occupied[row_idx].size() && occupied[row_idx][c]) {
                        can_place = false;
                        break;
                    }
                }
                if (can_place) break;
                row_idx++;
            }
            // Expand occupied grid if needed
            while (row_idx >= occupied.size()) {
                occupied.push_back(std::vector<bool>(num_cols, false));
            }
        }

        // Clamp indices
        if (col_idx >= num_cols) col_idx = num_cols - 1;
        if (row_idx + row_span > num_rows) {
            // Expand rows to fit
            num_rows = row_idx + row_span;
        }
        if (col_idx + col_span > num_cols) col_span = num_cols - col_idx;

        // Calculate available size for this cell.
        // We try to pass a definite block size during the measurement pass when
        // the row tracks are already known, so nested grid/flex containers can
        // resolve percentage heights and flexible tracks instead of collapsing
        // to intrinsic height 0.
        size_t col_track_start = col_idx * 2;
        size_t col_track_end = (col_idx + col_span - 1) * 2;
        float cell_width = 0.0f;
        for (size_t t = col_track_start; t <= col_track_end && t < columns.size(); t++) {
            if (columns[t].kind != GridTrackKind::Gutter) {
                cell_width += columns[t].base_size;
            } else if (t > col_track_start && t < col_track_end) {
                cell_width += columns[t].base_size;
            }
        }

        size_t row_track_start = row_idx * 2;
        size_t row_track_end = (row_idx + row_span - 1) * 2;
        float cell_height = 0.0f;
        for (size_t t = row_track_start; t <= row_track_end && t < rows.size(); t++) {
            if (rows[t].kind != GridTrackKind::Gutter) {
                cell_height += rows[t].base_size;
            } else if (t > row_track_start && t < row_track_end) {
                cell_height += rows[t].base_size;
            }
        }

        // Measure child to get its intrinsic size.
        // When an auto column has not been sized yet, cell_width may still be 0.
        // In that case, preserve the container's current inline sizing mode instead
        // of forcing MaxContent: a MinContent sizing pass must stay MinContent,
        // otherwise intrinsic measurement can explode to ~infinite width.
        AvailableSpace child_width_space;
        std::optional<float> child_parent_width = std::nullopt;
        if (cell_width > 0.0f) {
            child_width_space = AvailableSpace::Definite(cell_width);
            child_parent_width = std::optional<float>(cell_width);
        } else if (inner_node_size.width.has_value()) {
            float fallback_width = *inner_node_size.width / static_cast<float>(std::max<size_t>(1, col_span));
            child_width_space = AvailableSpace::Definite(fallback_width);
            child_parent_width = std::optional<float>(fallback_width);
        } else if (available_space.width.IsDefinite()) {
            float fallback_width = f32_max(available_space.width.value - padding_border_size.width, 0.0f)
                / static_cast<float>(std::max<size_t>(1, col_span));
            child_width_space = AvailableSpace::Definite(fallback_width);
            child_parent_width = std::optional<float>(fallback_width);
        } else if (available_space.width.IsMinContent()) {
            child_width_space = AvailableSpace::MinContent();
        } else {
            child_width_space = AvailableSpace::MaxContent();
        }

        AvailableSpace child_height_space;
        std::optional<float> child_parent_height = std::nullopt;
        if (cell_height > 0.0f) {
            child_height_space = AvailableSpace::Definite(cell_height);
            child_parent_height = std::optional<float>(cell_height);
        } else if (inner_node_size.height.has_value()) {
            float fallback_height = *inner_node_size.height / static_cast<float>(std::max<size_t>(1, row_span));
            child_height_space = AvailableSpace::Definite(fallback_height);
            child_parent_height = std::optional<float>(fallback_height);
        } else if (available_space.height.IsMinContent()) {
            child_height_space = AvailableSpace::MinContent();
        } else {
            child_height_space = AvailableSpace::MaxContent();
        }

        Size<AvailableSpace> measure_space{
            child_width_space,
            child_height_space
        };

        auto child_output = tree.PerformChildLayout(
            child_id,
            Size<std::optional<float>>{std::nullopt, std::nullopt},
            Size<std::optional<float>>{child_parent_width, child_parent_height},
            measure_space,
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        auto min_content_width_output = tree.PerformChildLayout(
            child_id,
            Size<std::optional<float>>{std::nullopt, std::nullopt},
            Size<std::optional<float>>{child_parent_width, child_parent_height},
            Size<AvailableSpace>{AvailableSpace::MinContent(), child_height_space},
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        auto max_content_width_output = tree.PerformChildLayout(
            child_id,
            Size<std::optional<float>>{std::nullopt, std::nullopt},
            Size<std::optional<float>>{child_parent_width, child_parent_height},
            Size<AvailableSpace>{AvailableSpace::MaxContent(), child_height_space},
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        auto min_content_height_output = tree.PerformChildLayout(
            child_id,
            Size<std::optional<float>>{std::nullopt, std::nullopt},
            Size<std::optional<float>>{child_parent_width, child_parent_height},
            Size<AvailableSpace>{child_width_space, AvailableSpace::MinContent()},
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        auto max_content_height_output = tree.PerformChildLayout(
            child_id,
            Size<std::optional<float>>{std::nullopt, std::nullopt},
            Size<std::optional<float>>{child_parent_width, child_parent_height},
            Size<AvailableSpace>{child_width_space, AvailableSpace::MaxContent()},
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        placements.push_back({
            child_id,
            col_idx,
            row_idx,
            col_span,
            row_span,
            child_output.size.width,
            child_output.size.height,
            min_content_width_output.size.width,
            max_content_width_output.size.width,
            min_content_height_output.size.height,
            max_content_height_output.size.height
        });

        // Mark cells as occupied
        markCellsOccupied(col_idx, row_idx, col_span, row_span);

        // Move to next cell for auto-placement
        // The next item will use findNextAvailableCell to skip occupied cells
        current_col = col_idx + col_span;
        if (current_col >= num_cols) {
            current_col = 0;
            current_row = row_idx + 1;
        }
    }

    // Ensure we have enough row tracks after auto-placement may have expanded num_rows
    size_t required_row_slots = num_rows > 0 ? num_rows * 2 - 1 : 0;
    while (rows.size() < required_row_slots) {
        if (!rows.empty()) {
            GridTrack gutter = GridTrack::Gutter(row_gap);
            gutter.base_size = row_gap;
            gutter.growth_limit = row_gap;
            rows.push_back(gutter);
        }
        // Use grid_auto_rows if available, otherwise use Auto
        // Read from grid_style for Grid-specific properties
        if (!grid_style.grid_auto_rows.empty()) {
            const auto& auto_track = grid_style.grid_auto_rows[auto_row_idx % grid_style.grid_auto_rows.size()];
            auto_row_idx++;
            GridTrack track = GridTrack::New(auto_track.min, auto_track.max);
            // Resolve the track size immediately for fixed values
            if (auto_track.min.type == MinTrackSizingFunctionType::Fixed) {
                if (auto_track.min.is_percent && inner_node_size.height.has_value()) {
                    track.base_size = auto_track.min.value * *inner_node_size.height;
                } else if (!auto_track.min.is_percent) {
                    track.base_size = auto_track.min.value;
                }
            }
            if (auto_track.max.type == MaxTrackSizingFunctionType::Fixed) {
                if (auto_track.max.is_percent && inner_node_size.height.has_value()) {
                    track.growth_limit = auto_track.max.value * *inner_node_size.height;
                } else if (!auto_track.max.is_percent) {
                    track.growth_limit = auto_track.max.value;
                }
            }
            if (track.growth_limit < track.base_size) {
                track.growth_limit = track.base_size;
            }
            rows.push_back(track);
        } else {
            rows.push_back(GridTrack::New(MinTrackSizingFunction::Auto(), MaxTrackSizingFunction::Auto()));
        }
    }

    // Update column widths based on measured children and intrinsic track sizing
    for (const auto& placement : placements) {
        for (size_t c = 0; c < placement.col_span; c++) {
            size_t col_track_idx = (placement.col_idx + c) * 2;
            if (col_track_idx >= columns.size() || columns[col_track_idx].kind != GridTrackKind::Track) {
                continue;
            }

            auto& track = columns[col_track_idx];
            if (!TrackUsesIntrinsicMinSizing(track) && !TrackUsesIntrinsicMaxSizing(track)) {
                continue;
            }

            float width_per_col = placement.measured_width / static_cast<float>(placement.col_span);
            float min_content_per_col = placement.min_content_width / static_cast<float>(placement.col_span);
            float max_content_per_col = placement.max_content_width / static_cast<float>(placement.col_span);
            float candidate = track.base_size;

            switch (track.min_track_sizing_function.type) {
                case MinTrackSizingFunctionType::Auto:
                    candidate = std::max(candidate, width_per_col);
                    break;
                case MinTrackSizingFunctionType::MinContent:
                    candidate = std::max(candidate, min_content_per_col);
                    break;
                case MinTrackSizingFunctionType::MaxContent:
                    candidate = std::max(candidate, max_content_per_col);
                    break;
                case MinTrackSizingFunctionType::Fixed:
                    break;
            }

            switch (track.max_track_sizing_function.type) {
                case MaxTrackSizingFunctionType::Auto:
                    candidate = std::max(candidate, width_per_col);
                    break;
                case MaxTrackSizingFunctionType::MinContent:
                    candidate = std::max(candidate, min_content_per_col);
                    break;
                case MaxTrackSizingFunctionType::MaxContent:
                    candidate = std::max(candidate, max_content_per_col);
                    break;
                case MaxTrackSizingFunctionType::FitContent:
                    candidate = std::max(candidate, f32_min(max_content_per_col, track.growth_limit));
                    break;
                case MaxTrackSizingFunctionType::Fixed:
                case MaxTrackSizingFunctionType::Fraction:
                    break;
            }

            if (std::isfinite(track.growth_limit)) {
                candidate = f32_min(candidate, track.growth_limit);
            }
            track.base_size = std::max(track.base_size, candidate);
        }
    }

    // Update row heights based on measured children and intrinsic track sizing
    for (const auto& placement : placements) {
        for (size_t r = 0; r < placement.row_span; r++) {
            size_t row_track_idx = (placement.row_idx + r) * 2;
            if (row_track_idx >= rows.size() || rows[row_track_idx].kind != GridTrackKind::Track) {
                continue;
            }

            auto& track = rows[row_track_idx];
            if (!TrackUsesIntrinsicMinSizing(track) && !TrackUsesIntrinsicMaxSizing(track)) {
                continue;
            }

            float height_per_row = placement.measured_height / static_cast<float>(placement.row_span);
            float min_content_per_row = placement.min_content_height / static_cast<float>(placement.row_span);
            float max_content_per_row = placement.max_content_height / static_cast<float>(placement.row_span);
            float candidate = track.base_size;

            switch (track.min_track_sizing_function.type) {
                case MinTrackSizingFunctionType::Auto:
                    candidate = std::max(candidate, height_per_row);
                    break;
                case MinTrackSizingFunctionType::MinContent:
                    candidate = std::max(candidate, min_content_per_row);
                    break;
                case MinTrackSizingFunctionType::MaxContent:
                    candidate = std::max(candidate, max_content_per_row);
                    break;
                case MinTrackSizingFunctionType::Fixed:
                    break;
            }

            switch (track.max_track_sizing_function.type) {
                case MaxTrackSizingFunctionType::Auto:
                    candidate = std::max(candidate, height_per_row);
                    break;
                case MaxTrackSizingFunctionType::MinContent:
                    candidate = std::max(candidate, min_content_per_row);
                    break;
                case MaxTrackSizingFunctionType::MaxContent:
                    candidate = std::max(candidate, max_content_per_row);
                    break;
                case MaxTrackSizingFunctionType::FitContent:
                    candidate = std::max(candidate, f32_min(max_content_per_row, track.growth_limit));
                    break;
                case MaxTrackSizingFunctionType::Fixed:
                case MaxTrackSizingFunctionType::Fraction:
                    break;
            }

            if (std::isfinite(track.growth_limit)) {
                candidate = f32_min(candidate, track.growth_limit);
            }
            track.base_size = std::max(track.base_size, candidate);
        }
    }

    // Recalculate sums and offsets after intrinsic track growth
    col_sum = SumTrackBaseSizes(columns);
    row_sum = SumTrackBaseSizes(rows);

    // Update container width after intrinsic column growth.
    // This is critical for implicit/auto columns: their measured intrinsic width
    // must be reflected in the final container width before second-pass layout.
    container_width = outer_node_size.width.value_or(col_sum + padding_border_size.width);
    if (min_size.width.has_value()) container_width = f32_max(container_width, *min_size.width);
    if (max_size.width.has_value()) container_width = f32_min(container_width, *max_size.width);
    container_size.width = container_width;

    inner_width = container_width - padding_border_size.width;
    if (inner_width > 0.0f) {
        StretchTracksToAvailableSpace(columns, inner_width);
        col_sum = SumTrackBaseSizes(columns);
    }

    // Update container height
    container_height = outer_node_size.height.value_or(row_sum + padding_border_size.height);
    if (min_size.height.has_value()) container_height = f32_max(container_height, *min_size.height);
    if (max_size.height.has_value()) container_height = f32_min(container_height, *max_size.height);
    container_size.height = container_height;

    float inner_height = container_height - padding_border_size.height;
    if (inner_height > 0.0f) {
        StretchTracksToAvailableSpace(rows, inner_height);
        row_sum = SumTrackBaseSizes(rows);
    }

    CalculateTrackOffsets(columns, padding_border.left);
    CalculateTrackOffsets(rows, padding_border.top);

    // If only size requested, return early after measuring children
    if (run_mode == RunMode::ComputeSize) {
        LayoutOutput output;
        output.size = container_size;
        return output;
    }

    // Second pass: position children in their cells
    for (const auto& placement : placements) {
        size_t col_track_start = placement.col_idx * 2;
        size_t row_track_start = placement.row_idx * 2;
        size_t col_track_end = (placement.col_idx + placement.col_span - 1) * 2;
        size_t row_track_end = (placement.row_idx + placement.row_span - 1) * 2;

        float cell_x = 0.0f;
        float cell_y = 0.0f;
        float cell_width = 0.0f;
        float cell_height = 0.0f;

        // Get position from first track
        if (col_track_start < columns.size()) {
            cell_x = columns[col_track_start].offset;
        }
        if (row_track_start < rows.size()) {
            cell_y = rows[row_track_start].offset;
        }

        // Calculate width spanning multiple tracks
        for (size_t t = col_track_start; t <= col_track_end && t < columns.size(); t++) {
            if (columns[t].kind != GridTrackKind::Gutter) {
                cell_width += columns[t].base_size;
            } else if (t > col_track_start && t < col_track_end) {
                cell_width += columns[t].base_size;
            }
        }

        // Calculate height spanning multiple tracks
        for (size_t t = row_track_start; t <= row_track_end && t < rows.size(); t++) {
            if (rows[t].kind != GridTrackKind::Gutter) {
                cell_height += rows[t].base_size;
            } else if (t > row_track_start && t < row_track_end) {
                cell_height += rows[t].base_size;
            }
        }

        // Get child's unified Style for common properties (align-self, justify-self, size)
        const auto& child_style = tree.GetChildStyle(placement.child_id);

        // Determine final size and position based on alignment
        // Default is stretch (fill the cell)
        float final_width = placement.measured_width;
        float final_height = placement.measured_height;
        float offset_x = 0.0f;
        float offset_y = 0.0f;

        // Resolve justify-items (horizontal alignment within cell)
        // Child's justify-self overrides container's justify-items
        // Read from unified Style for alignment properties
        auto justify = child_style.justify_self.value_or(
            style.justify_items.value_or(AlignItems::Stretch));

        // Check if child has explicit width - if so, don't stretch even if justify is Stretch
        // Read from unified Style for size property
        bool has_explicit_width = !child_style.size.width.IsAuto();

        if (justify == AlignItems::Stretch && !has_explicit_width) {
            final_width = cell_width;
        } else {
            // For non-stretch, or when child has explicit width, we need to measure with intrinsic sizing
            // to get the child's natural width (respecting its own width property)
            // Pass cell_width as parent size so percentage widths can resolve
            auto intrinsic_output = tree.PerformChildLayout(
                placement.child_id,
                Size<std::optional<float>>{std::nullopt, std::nullopt},
                Size<std::optional<float>>{cell_width, cell_height},
                Size<AvailableSpace>{AvailableSpace::Definite(cell_width), AvailableSpace::MaxContent()},
                SizingMode::InherentSize,
                Line<bool>{false, false}
            );
            final_width = intrinsic_output.size.width;

            float free_space = cell_width - final_width;
            if (free_space > 0) {
                switch (justify) {
                    case AlignItems::Center:
                        offset_x = free_space / 2.0f;
                        break;
                    case AlignItems::End:
                    case AlignItems::FlexEnd:
                        offset_x = free_space;
                        break;
                    case AlignItems::Start:
                    case AlignItems::FlexStart:
                    case AlignItems::Stretch:  // Stretch with explicit width acts like Start
                    default:
                        offset_x = 0.0f;
                        break;
                }
            }
        }

        // Resolve align-items (vertical alignment within cell)
        // Child's align-self overrides container's align-items
        // Read from unified Style for alignment properties
        auto align = child_style.align_self.value_or(
            style.align_items.value_or(AlignItems::Stretch));

        // Check if child has explicit height - if so, don't stretch even if align is Stretch
        // Read from unified Style for size property
        bool has_explicit_height = !child_style.size.height.IsAuto();

        if (align == AlignItems::Stretch && !has_explicit_height) {
            final_height = cell_height;
        } else {
            // For non-stretch, or when child has explicit height, use measured height
            float free_space = cell_height - final_height;
            if (free_space > 0) {
                switch (align) {
                    case AlignItems::Center:
                        offset_y = free_space / 2.0f;
                        break;
                    case AlignItems::End:
                    case AlignItems::FlexEnd:
                        offset_y = free_space;
                        break;
                    case AlignItems::Start:
                    case AlignItems::FlexStart:
                    case AlignItems::Stretch:  // Stretch with explicit height acts like Start
                    default:
                        offset_y = 0.0f;
                        break;
                }
            }
        }

        // Perform final layout with known dimensions so child containers
        // (like flex containers) can properly align their children
        tree.PerformChildLayout(
            placement.child_id,
            Size<std::optional<float>>{final_width, final_height},
            Size<std::optional<float>>{cell_width, cell_height},
            Size<AvailableSpace>{AvailableSpace::Definite(final_width), AvailableSpace::Definite(final_height)},
            SizingMode::InherentSize,
            Line<bool>{false, false}
        );

        tree.SetUnroundedLayout(placement.child_id, Layout{
            0,  // order
            cell_x + offset_x,
            cell_y + offset_y,
            final_width,
            final_height
        });
    }

    Size<float> content_size{col_sum, row_sum};

    LayoutOutput output;
    output.size = container_size;
    output.content_size = content_size;
    return output;
}

} // namespace mbink

