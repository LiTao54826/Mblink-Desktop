/**
 * @file scroll_invalidation_stats.h
 * @brief Shared scroll invalidation fallback counters.
 */

#pragma once

#include <cstdint>

namespace mblink {

enum class ScrollInvalidationReason {
    None,
    ClipLayerFullDirty,
    AncestorLayerFullDirty,
    MissingLayerTarget
};

struct ScrollInvalidationStats {
    std::uint64_t scrolls_handled = 0;
    std::uint64_t full_dirty_scrolls = 0;
    std::uint64_t clip_layer_full_dirty_scrolls = 0;
    std::uint64_t ancestor_layer_full_dirty_scrolls = 0;
    std::uint64_t missing_layer_target_scrolls = 0;
    ScrollInvalidationReason last_reason = ScrollInvalidationReason::None;

    void Reset() {
        scrolls_handled = 0;
        full_dirty_scrolls = 0;
        clip_layer_full_dirty_scrolls = 0;
        ancestor_layer_full_dirty_scrolls = 0;
        missing_layer_target_scrolls = 0;
        last_reason = ScrollInvalidationReason::None;
    }
};

inline std::uint64_t IncrementalEligibleScrollFallbacks(const ScrollInvalidationStats& stats) {
    return stats.clip_layer_full_dirty_scrolls;
}

inline std::uint64_t ConservativeScrollFallbacks(const ScrollInvalidationStats& stats) {
    return stats.ancestor_layer_full_dirty_scrolls +
           stats.missing_layer_target_scrolls;
}

} // namespace mblink
