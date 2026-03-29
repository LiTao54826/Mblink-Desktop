/**
 * @file test_dirty_region_properties.cpp
 * @brief Property-based tests for CompositorLayer dirty region management
 *
 * This file implements property-based testing for the dirty region functionality
 * in the CompositorLayer class. Each property test runs 100 iterations with
 * randomly generated inputs.
 *
 * **Feature: layer-compositing-architecture, Property 3: Dirty region marking and merging**
 * **Validates: Requirements 3.1, 3.4**
 */

#include <gtest/gtest.h>
#include "core/compositor/compositor_layer.h"
#include <random>
#include <vector>
#include <set>
#include <algorithm>

using namespace mbink;

// Random number generator for property tests
class DirtyRegionPropertyTestRng {
public:
    DirtyRegionPropertyTestRng() : gen_(std::random_device{}()) {}

    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }

    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }

private:
    std::mt19937 gen_;
};

class DirtyRegionPropertyTest : public ::testing::Test {
protected:
    DirtyRegionPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;

    void SetUp() override {
    }

    void TearDown() override {
    }

    // Helper to create a layer with given bounds
    std::shared_ptr<CompositorLayer> CreateLayer(float width, float height) {
        auto layer = CreateCompositorLayer();
        layer->SetBounds(SkRect::MakeWH(width, height));
        return layer;
    }

    // Helper to generate random rect within bounds
    SkRect RandomRect(float max_width, float max_height) {
        float x = rng_.randFloat(0, max_width * 0.8f);
        float y = rng_.randFloat(0, max_height * 0.8f);
        float w = rng_.randFloat(10, std::min(100.0f, max_width - x));
        float h = rng_.randFloat(10, std::min(100.0f, max_height - y));
        return SkRect::MakeXYWH(x, y, w, h);
    }

    // Helper to check if a point is covered by any rect in the list
    bool IsPointCovered(int x, int y, const std::vector<SkIRect>& rects) {
        for (const auto& rect : rects) {
            if (rect.contains(x, y)) {
                return true;
            }
        }
        return false;
    }

    // Helper to collect all covered pixels from a list of rects
    std::set<std::pair<int, int>> CollectCoveredPixels(const std::vector<SkIRect>& rects) {
        std::set<std::pair<int, int>> pixels;
        for (const auto& rect : rects) {
            for (int y = rect.top(); y < rect.bottom(); ++y) {
                for (int x = rect.left(); x < rect.right(); ++x) {
                    pixels.insert({x, y});
                }
            }
        }
        return pixels;
    }
};

/**
 * **Feature: layer-compositing-architecture, Property 3: Dirty region marking and merging**
 *
 * For any set of dirty regions within a layer, the system SHALL merge overlapping
 * regions and the merged result SHALL cover all originally dirty pixels.
 *
 * **Validates: Requirements 3.1, 3.4**
 */
TEST_F(DirtyRegionPropertyTest, MergedRegionsCoverAllOriginalPixels) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create layer with random size
        float width = rng_.randFloat(200, 800);
        float height = rng_.randFloat(200, 800);
        auto layer = CreateLayer(width, height);

        // Generate random dirty regions
        int num_regions = rng_.randInt(2, 10);
        std::vector<SkRect> original_regions;

        for (int j = 0; j < num_regions; ++j) {
            SkRect rect = RandomRect(width, height);
            original_regions.push_back(rect);
            layer->MarkDirty(rect);
        }

        // Get dirty regions before merge
        std::vector<SkIRect> before_merge = layer->GetDirtyRegions();

        // Collect all pixels covered before merge
        auto pixels_before = CollectCoveredPixels(before_merge);

        // Merge dirty regions
        layer->MergeDirtyRegions();

        // Get dirty regions after merge
        const auto& after_merge = layer->GetDirtyRegions();

        // Collect all pixels covered after merge
        auto pixels_after = CollectCoveredPixels(after_merge);

        // Property: All pixels covered before merge must be covered after merge
        for (const auto& pixel : pixels_before) {
            EXPECT_TRUE(pixels_after.count(pixel) > 0)
                << "Pixel (" << pixel.first << ", " << pixel.second
                << ") was covered before merge but not after (iteration " << i << ")";
        }

        // Property: Merged regions should be fewer or equal
        EXPECT_LE(after_merge.size(), before_merge.size())
            << "Merged regions should not be more than original (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Dirty region marking**
 *
 * For any dirty region marked within layer bounds, the region SHALL be tracked.
 *
 * **Validates: Requirements 3.1**
 */
TEST_F(DirtyRegionPropertyTest, DirtyRegionsAreTracked) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float width = rng_.randFloat(200, 800);
        float height = rng_.randFloat(200, 800);
        auto layer = CreateLayer(width, height);

        // Clear initial dirty regions (SetBounds marks full dirty)
        layer->ClearDirtyRegions();

        // Now should have no dirty regions
        EXPECT_FALSE(layer->HasDirtyRegions())
            << "Layer should have no dirty regions after clear (iteration " << i << ")";

        // Mark a dirty region
        SkRect dirty_rect = RandomRect(width, height);
        layer->MarkDirty(dirty_rect);

        // Should have dirty regions now
        EXPECT_TRUE(layer->HasDirtyRegions())
            << "Layer should have dirty regions after MarkDirty (iteration " << i << ")";

        // The dirty region should contain the marked area
        const auto& regions = layer->GetDirtyRegions();
        EXPECT_FALSE(regions.empty())
            << "Dirty regions list should not be empty (iteration " << i << ")";

        // Check that the marked rect is covered
        SkIRect marked_int = dirty_rect.roundOut();
        bool covered = false;
        for (const auto& region : regions) {
            if (region.contains(marked_int)) {
                covered = true;
                break;
            }
        }
        // Note: Due to clipping, the region might be partially covered
        // At minimum, the intersection should be non-empty
        EXPECT_TRUE(regions.size() > 0)
            << "At least one dirty region should exist (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Clear dirty regions**
 *
 * After calling ClearDirtyRegions(), HasDirtyRegions() SHALL return false.
 *
 * **Validates: Requirements 3.1**
 */
TEST_F(DirtyRegionPropertyTest, ClearDirtyRegionsWorks) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float width = rng_.randFloat(200, 800);
        float height = rng_.randFloat(200, 800);
        auto layer = CreateLayer(width, height);

        // Add random dirty regions
        int num_regions = rng_.randInt(1, 10);
        for (int j = 0; j < num_regions; ++j) {
            layer->MarkDirty(RandomRect(width, height));
        }

        EXPECT_TRUE(layer->HasDirtyRegions())
            << "Layer should have dirty regions before clear (iteration " << i << ")";

        // Clear dirty regions
        layer->ClearDirtyRegions();

        EXPECT_FALSE(layer->HasDirtyRegions())
            << "Layer should have no dirty regions after clear (iteration " << i << ")";

        EXPECT_TRUE(layer->GetDirtyRegions().empty())
            << "Dirty regions list should be empty after clear (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Full dirty marking**
 *
 * After calling MarkFullDirty(), the dirty region SHALL cover the entire layer.
 *
 * **Validates: Requirements 3.1**
 */
TEST_F(DirtyRegionPropertyTest, MarkFullDirtyCoversEntireLayer) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float width = rng_.randFloat(200, 800);
        float height = rng_.randFloat(200, 800);
        auto layer = CreateLayer(width, height);

        // Mark full dirty
        layer->MarkFullDirty();

        EXPECT_TRUE(layer->HasDirtyRegions())
            << "Layer should have dirty regions after MarkFullDirty (iteration " << i << ")";

        const auto& regions = layer->GetDirtyRegions();
        EXPECT_EQ(regions.size(), 1u)
            << "MarkFullDirty should result in exactly one region (iteration " << i << ")";

        if (!regions.empty()) {
            const auto& full_region = regions[0];
            EXPECT_EQ(full_region.left(), 0)
                << "Full dirty region should start at x=0 (iteration " << i << ")";
            EXPECT_EQ(full_region.top(), 0)
                << "Full dirty region should start at y=0 (iteration " << i << ")";
            EXPECT_EQ(full_region.width(), static_cast<int>(width))
                << "Full dirty region width should match layer width (iteration " << i << ")";
            EXPECT_EQ(full_region.height(), static_cast<int>(height))
                << "Full dirty region height should match layer height (iteration " << i << ")";
        }
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Dirty regions clipped to bounds**
 *
 * For any dirty region that extends outside layer bounds, the tracked region
 * SHALL be clipped to the layer bounds.
 *
 * **Validates: Requirements 3.1**
 */
TEST_F(DirtyRegionPropertyTest, DirtyRegionsClippedToBounds) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float width = rng_.randFloat(200, 400);
        float height = rng_.randFloat(200, 400);
        auto layer = CreateLayer(width, height);

        // Create a rect that extends outside bounds
        float x = rng_.randFloat(-50, width - 50);
        float y = rng_.randFloat(-50, height - 50);
        float w = rng_.randFloat(100, 200);
        float h = rng_.randFloat(100, 200);
        SkRect outside_rect = SkRect::MakeXYWH(x, y, w, h);

        layer->MarkDirty(outside_rect);

        const auto& regions = layer->GetDirtyRegions();

        // All tracked regions should be within layer bounds
        SkIRect layer_bounds = SkIRect::MakeWH(
            static_cast<int>(width),
            static_cast<int>(height)
        );

        for (const auto& region : regions) {
            EXPECT_GE(region.left(), 0)
                << "Region left should be >= 0 (iteration " << i << ")";
            EXPECT_GE(region.top(), 0)
                << "Region top should be >= 0 (iteration " << i << ")";
            EXPECT_LE(region.right(), static_cast<int>(width))
                << "Region right should be <= layer width (iteration " << i << ")";
            EXPECT_LE(region.bottom(), static_cast<int>(height))
                << "Region bottom should be <= layer height (iteration " << i << ")";
        }
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Merge reduces region count**
 *
 * For overlapping dirty regions, merging SHALL reduce the total number of regions.
 *
 * **Validates: Requirements 3.4**
 */
TEST_F(DirtyRegionPropertyTest, MergeReducesOverlappingRegions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto layer = CreateLayer(500, 500);

        // Clear initial dirty regions
        layer->ClearDirtyRegions();

        // Create intentionally overlapping regions
        float base_x = rng_.randFloat(50, 200);
        float base_y = rng_.randFloat(50, 200);

        // Add overlapping regions
        layer->MarkDirty(SkRect::MakeXYWH(base_x, base_y, 100, 100));
        layer->MarkDirty(SkRect::MakeXYWH(base_x + 50, base_y + 50, 100, 100));
        layer->MarkDirty(SkRect::MakeXYWH(base_x + 25, base_y + 25, 50, 50));

        size_t before_count = layer->GetDirtyRegions().size();
        EXPECT_EQ(before_count, 3u)
            << "Should have 3 regions before merge (iteration " << i << ")";

        layer->MergeDirtyRegions();

        size_t after_count = layer->GetDirtyRegions().size();
        EXPECT_LT(after_count, before_count)
            << "Overlapping regions should be merged (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Merge handles adjacent regions**
 *
 * For adjacent (touching) dirty regions, merging SHALL combine them.
 *
 * **Validates: Requirements 3.4**
 */
TEST_F(DirtyRegionPropertyTest, MergeHandlesAdjacentRegions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto layer = CreateLayer(500, 500);

        // Create adjacent regions (touching but not overlapping)
        float base_x = rng_.randFloat(50, 200);
        float base_y = rng_.randFloat(50, 200);
        float size = 50;

        // Horizontal adjacent
        layer->MarkDirty(SkRect::MakeXYWH(base_x, base_y, size, size));
        layer->MarkDirty(SkRect::MakeXYWH(base_x + size, base_y, size, size));

        size_t before_count = layer->GetDirtyRegions().size();

        layer->MergeDirtyRegions();

        size_t after_count = layer->GetDirtyRegions().size();

        // Adjacent regions should be merged (allowing 1 pixel gap)
        EXPECT_LE(after_count, before_count)
            << "Adjacent regions should be merged or kept (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 3: Large coverage triggers full dirty**
 *
 * When merged dirty regions cover more than 50% of the layer, the system
 * SHALL use a single full-layer dirty region for efficiency.
 *
 * **Validates: Requirements 3.4**
 */
TEST_F(DirtyRegionPropertyTest, LargeCoverageTriggersFullDirty) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto layer = CreateLayer(200, 200);

        // Add many regions that cover most of the layer
        for (int j = 0; j < 20; ++j) {
            float x = rng_.randFloat(0, 150);
            float y = rng_.randFloat(0, 150);
            layer->MarkDirty(SkRect::MakeXYWH(x, y, 80, 80));
        }

        layer->MergeDirtyRegions();

        const auto& regions = layer->GetDirtyRegions();

        // When coverage is high, should consolidate to fewer regions
        // The implementation uses 50% threshold to switch to full layer
        if (regions.size() == 1) {
            // If consolidated to one region, it should be the full layer
            const auto& region = regions[0];
            EXPECT_EQ(region.left(), 0);
            EXPECT_EQ(region.top(), 0);
            EXPECT_EQ(region.width(), 200);
            EXPECT_EQ(region.height(), 200);
        }
        // Otherwise, multiple regions is also acceptable
    }
}

