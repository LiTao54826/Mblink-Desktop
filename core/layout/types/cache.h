/**
 * @file cache.h
 * @brief Layout cache for storing results of layout computation
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/tree/cache.rs
 */

#pragma once

#include "geometry.h"
#include "layout.h"
#include <optional>
#include <array>
#include <cmath>

namespace mblink {

/// Number of cache entries per node
constexpr size_t CACHE_SIZE = 9;

// Note: RunMode is defined in layout.h

//------------------------------------------------------------------------------
// Cache Entry
//------------------------------------------------------------------------------

/// Cached intermediate layout result
template<typename T>
struct CacheEntry {
    /// Known dimensions when cached
    Size<std::optional<float>> known_dimensions;
    /// Available space when cached
    Size<AvailableSpace> available_space;
    /// Content version when cached (for incremental layout invalidation)
    uint64_t content_version;
    /// Cached content
    T content;
};

//------------------------------------------------------------------------------
// Cache
//------------------------------------------------------------------------------

/// A cache for storing layout results
class Cache {
public:
    Cache() : is_empty_(true) {}

    /// Compute cache slot based on inputs
    static size_t ComputeCacheSlot(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space
    ) {
        const bool has_known_width = known_dimensions.width.has_value();
        const bool has_known_height = known_dimensions.height.has_value();

        // Slot 0: Both known_dimensions were set
        if (has_known_width && has_known_height) {
            return 0;
        }

        // Slot 1-2: width known, height unknown
        if (has_known_width && !has_known_height) {
            return 1 + (available_space.height.IsMinContent() ? 1 : 0);
        }

        // Slot 3-4: height known, width unknown
        if (has_known_height && !has_known_width) {
            return 3 + (available_space.width.IsMinContent() ? 1 : 0);
        }

        // Slots 5-8: Neither known
        const bool width_is_min_content = available_space.width.IsMinContent();
        const bool height_is_min_content = available_space.height.IsMinContent();
        
        if (!width_is_min_content && !height_is_min_content) return 5;
        if (!width_is_min_content && height_is_min_content) return 6;
        if (width_is_min_content && !height_is_min_content) return 7;
        return 8;
    }

    /// Check if two AvailableSpace values are roughly equal
    static bool IsRoughlyEqual(AvailableSpace a, AvailableSpace b) {
        if (a.type != b.type) return false;
        if (!a.IsDefinite()) return true;
        // Allow small floating point differences
        return std::abs(a.value - b.value) < 0.001f;
    }

    /// Try to retrieve a cached result
    /// @param known_dimensions Known dimensions for the layout
    /// @param available_space Available space for the layout
    /// @param run_mode The run mode for layout computation
    /// @param content_version The current content version (0 means skip version check for backward compatibility)
    std::optional<LayoutOutput> Get(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space,
        RunMode run_mode,
        uint64_t content_version = 0
    ) const {
        if (run_mode == RunMode::PerformHiddenLayout) {
            return std::nullopt;
        }

        if (run_mode == RunMode::PerformLayout) {
            if (final_layout_entry_.has_value()) {
                const auto& entry = *final_layout_entry_;
                
                // Check content version first (if provided)
                // Version 0 means skip version check for backward compatibility
                if (content_version != 0 && entry.content_version != content_version) {
                    return std::nullopt;
                }
                
                const auto cached_size = entry.content.size;

                bool width_matches = 
                    known_dimensions.width == entry.known_dimensions.width ||
                    known_dimensions.width == std::optional<float>(cached_size.width);
                bool height_matches = 
                    known_dimensions.height == entry.known_dimensions.height ||
                    known_dimensions.height == std::optional<float>(cached_size.height);
                bool width_space_matches = known_dimensions.width.has_value() ||
                    IsRoughlyEqual(entry.available_space.width, available_space.width);
                bool height_space_matches = known_dimensions.height.has_value() ||
                    IsRoughlyEqual(entry.available_space.height, available_space.height);

                if (width_matches && height_matches && width_space_matches && height_space_matches) {
                    return entry.content;
                }
            }
            return std::nullopt;
        }

        // RunMode::ComputeSize
        for (const auto& entry : measure_entries_) {
            if (!entry.has_value()) continue;

            // Check content version first (if provided)
            // Version 0 means skip version check for backward compatibility
            if (content_version != 0 && entry->content_version != content_version) {
                continue;
            }

            const auto cached_size = entry->content;

            bool width_matches =
                known_dimensions.width == entry->known_dimensions.width ||
                known_dimensions.width == std::optional<float>(cached_size.width);
            bool height_matches =
                known_dimensions.height == entry->known_dimensions.height ||
                known_dimensions.height == std::optional<float>(cached_size.height);
            bool width_space_matches = known_dimensions.width.has_value() ||
                IsRoughlyEqual(entry->available_space.width, available_space.width);
            bool height_space_matches = known_dimensions.height.has_value() ||
                IsRoughlyEqual(entry->available_space.height, available_space.height);

            if (width_matches && height_matches && width_space_matches && height_space_matches) {
                return LayoutOutput::FromOuterSize(cached_size);
            }
        }

        return std::nullopt;
    }

    /// Store a computed result in the cache
    /// @param known_dimensions Known dimensions for the layout
    /// @param available_space Available space for the layout
    /// @param run_mode The run mode for layout computation
    /// @param content_version The current content version to store with the cache entry
    /// @param layout_output The layout output to cache
    void Store(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space,
        RunMode run_mode,
        uint64_t content_version,
        const LayoutOutput& layout_output
    ) {
        if (run_mode == RunMode::PerformHiddenLayout) {
            return;
        }

        is_empty_ = false;

        if (run_mode == RunMode::PerformLayout) {
            final_layout_entry_ = CacheEntry<LayoutOutput>{
                known_dimensions,
                available_space,
                content_version,
                layout_output
            };
        } else {
            size_t slot = ComputeCacheSlot(known_dimensions, available_space);
            measure_entries_[slot] = CacheEntry<Size<float>>{
                known_dimensions,
                available_space,
                content_version,
                layout_output.size
            };
        }
    }
    
    /// Store a computed result in the cache (backward compatible overload)
    /// @deprecated Use the version with content_version parameter
    void Store(
        Size<std::optional<float>> known_dimensions,
        Size<AvailableSpace> available_space,
        RunMode run_mode,
        const LayoutOutput& layout_output
    ) {
        Store(known_dimensions, available_space, run_mode, 0, layout_output);
    }

    /// Clear all cache entries
    void Clear() {
        if (is_empty_) return;
        
        is_empty_ = true;
        final_layout_entry_.reset();
        for (auto& entry : measure_entries_) {
            entry.reset();
        }
    }

    /// Check if cache is empty
    bool IsEmpty() const {
        return is_empty_;
    }

private:
    /// Cache entry for final layout
    std::optional<CacheEntry<LayoutOutput>> final_layout_entry_;
    
    /// Cache entries for size measurements
    std::array<std::optional<CacheEntry<Size<float>>>, CACHE_SIZE> measure_entries_;
    
    /// Track if all entries are empty
    bool is_empty_;
};

} // namespace mblink

