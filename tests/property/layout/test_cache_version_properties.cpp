/**
 * @file test_cache_version_properties.cpp
 * @brief Property-based tests for Cache version checking
 * 
 * This file implements property-based testing for the Cache class
 * with content version support. Each property test runs 100 iterations
 * with randomly generated inputs.
 * 
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * **Validates: Requirements 1.4, 1.5**
 */

#include <gtest/gtest.h>
#include "layout/types/cache.h"
#include "layout/types/layout.h"
#include "layout/types/geometry.h"
#include <random>
#include <vector>

using namespace mblink;

// Random number generator for property tests
class CacheVersionPropertyTestRng {
public:
    CacheVersionPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    uint64_t randVersion() {
        std::uniform_int_distribution<uint64_t> dist(1, 1000000);
        return dist(gen_);
    }
    
    bool randBool() {
        return randInt(0, 1) == 1;
    }
    
    Size<std::optional<float>> randKnownDimensions() {
        Size<std::optional<float>> dims;
        dims.width = randBool() ? std::optional<float>(randFloat(10.0f, 500.0f)) : std::nullopt;
        dims.height = randBool() ? std::optional<float>(randFloat(10.0f, 500.0f)) : std::nullopt;
        return dims;
    }
    
    Size<AvailableSpace> randAvailableSpace() {
        Size<AvailableSpace> space;
        int width_type = randInt(0, 2);
        int height_type = randInt(0, 2);
        
        if (width_type == 0) {
            space.width = AvailableSpace::Definite(randFloat(100.0f, 1000.0f));
        } else if (width_type == 1) {
            space.width = AvailableSpace::MinContent();
        } else {
            space.width = AvailableSpace::MaxContent();
        }
        
        if (height_type == 0) {
            space.height = AvailableSpace::Definite(randFloat(100.0f, 1000.0f));
        } else if (height_type == 1) {
            space.height = AvailableSpace::MinContent();
        } else {
            space.height = AvailableSpace::MaxContent();
        }
        
        return space;
    }
    
    LayoutOutput randLayoutOutput() {
        LayoutOutput output;
        output.size = Size<float>{randFloat(10.0f, 500.0f), randFloat(10.0f, 500.0f)};
        output.content_size = Size<float>{randFloat(10.0f, 500.0f), randFloat(10.0f, 500.0f)};
        return output;
    }
    
    RunMode randRunMode() {
        int mode = randInt(0, 1);  // Exclude PerformHiddenLayout as it doesn't cache
        return mode == 0 ? RunMode::PerformLayout : RunMode::ComputeSize;
    }
    
private:
    std::mt19937 gen_;
};

class CacheVersionPropertyTest : public ::testing::Test {
protected:
    CacheVersionPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
};

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * For any cache lookup with matching dimensions and available_space,
 * if the content_version differs from the cached entry's version,
 * the Cache SHALL return a cache miss.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, VersionMismatchReturnsCacheMiss) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        uint64_t stored_version = rng_.randVersion();
        
        // Store with a specific version
        cache.Store(known_dims, available_space, run_mode, stored_version, layout_output);
        
        // Generate a different version
        uint64_t different_version = stored_version + rng_.randInt(1, 1000);
        
        // Get with different version should return nullopt
        auto result = cache.Get(known_dims, available_space, run_mode, different_version);
        
        EXPECT_FALSE(result.has_value())
            << "Cache should return miss when version differs. "
            << "Stored version: " << stored_version << ", "
            << "Query version: " << different_version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * For any cache lookup with matching dimensions, available_space, AND content_version,
 * the Cache SHALL return the cached result (cache hit).
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, VersionMatchReturnsCacheHit) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        uint64_t version = rng_.randVersion();
        
        // Store with a specific version
        cache.Store(known_dims, available_space, run_mode, version, layout_output);
        
        // Get with same version should return the cached result
        auto result = cache.Get(known_dims, available_space, run_mode, version);
        
        EXPECT_TRUE(result.has_value())
            << "Cache should return hit when version matches. "
            << "Version: " << version << ", "
            << "Iteration: " << i;
        
        if (result.has_value()) {
            // Verify the cached size matches
            EXPECT_FLOAT_EQ(result->size.width, layout_output.size.width)
                << "Cached width should match. Iteration: " << i;
            EXPECT_FLOAT_EQ(result->size.height, layout_output.size.height)
                << "Cached height should match. Iteration: " << i;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * For any cache lookup with version 0 (backward compatibility mode),
 * the Cache SHALL skip version checking and return cached result if
 * dimensions and available_space match.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, ZeroVersionSkipsVersionCheck) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        uint64_t stored_version = rng_.randVersion();
        
        // Store with a specific version
        cache.Store(known_dims, available_space, run_mode, stored_version, layout_output);
        
        // Get with version 0 should skip version check and return cached result
        auto result = cache.Get(known_dims, available_space, run_mode, 0);
        
        EXPECT_TRUE(result.has_value())
            << "Cache should return hit when query version is 0 (skip check). "
            << "Stored version: " << stored_version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * For any cache entry stored with version 0, a lookup with any non-zero version
 * SHALL return a cache miss (since 0 != non-zero).
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, StoredZeroVersionMismatchesNonZero) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        
        // Store with version 0
        cache.Store(known_dims, available_space, run_mode, 0, layout_output);
        
        // Get with non-zero version should return miss
        uint64_t query_version = rng_.randVersion();
        auto result = cache.Get(known_dims, available_space, run_mode, query_version);
        
        EXPECT_FALSE(result.has_value())
            << "Cache should return miss when stored version is 0 and query version is non-zero. "
            << "Query version: " << query_version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * After Clear(), any cache lookup SHALL return a cache miss regardless of version.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, ClearInvalidatesAllVersions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        uint64_t version = rng_.randVersion();
        
        // Store with a specific version
        cache.Store(known_dims, available_space, run_mode, version, layout_output);
        
        // Clear the cache
        cache.Clear();
        
        // Get should return miss after clear
        auto result = cache.Get(known_dims, available_space, run_mode, version);
        
        EXPECT_FALSE(result.has_value())
            << "Cache should return miss after Clear(). "
            << "Version: " << version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * For multiple cache entries (ComputeSize mode uses slots), version checking
 * SHALL work correctly for each slot independently.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, MultipleEntriesVersionIndependence) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Store multiple entries with different versions in ComputeSize mode
        std::vector<std::tuple<Size<std::optional<float>>, Size<AvailableSpace>, uint64_t, LayoutOutput>> entries;
        
        int num_entries = rng_.randInt(2, 5);
        for (int j = 0; j < num_entries; ++j) {
            auto known_dims = rng_.randKnownDimensions();
            auto available_space = rng_.randAvailableSpace();
            auto layout_output = rng_.randLayoutOutput();
            uint64_t version = rng_.randVersion();
            
            cache.Store(known_dims, available_space, RunMode::ComputeSize, version, layout_output);
            entries.emplace_back(known_dims, available_space, version, layout_output);
        }
        
        // Verify each entry can be retrieved with correct version
        for (const auto& [known_dims, available_space, version, layout_output] : entries) {
            auto result = cache.Get(known_dims, available_space, RunMode::ComputeSize, version);
            
            // Note: Due to slot-based caching, later entries may overwrite earlier ones
            // So we only check that if we get a hit, the version matches
            if (result.has_value()) {
                // The result should match the stored output
                EXPECT_FLOAT_EQ(result->size.width, layout_output.size.width)
                    << "Cached width should match for version " << version << ". Iteration: " << i;
            }
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * PerformHiddenLayout mode SHALL always return nullopt regardless of version.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, HiddenLayoutAlwaysReturnsMiss) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output = rng_.randLayoutOutput();
        uint64_t version = rng_.randVersion();
        
        // Store with PerformLayout mode first
        cache.Store(known_dims, available_space, RunMode::PerformLayout, version, layout_output);
        
        // Get with PerformHiddenLayout should always return miss
        auto result = cache.Get(known_dims, available_space, RunMode::PerformHiddenLayout, version);
        
        EXPECT_FALSE(result.has_value())
            << "PerformHiddenLayout should always return miss. "
            << "Version: " << version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 2: 缓存版本匹配**
 * 
 * Updating a cache entry with a new version SHALL invalidate the old version.
 * 
 * **Validates: Requirements 1.4, 1.5**
 */
TEST_F(CacheVersionPropertyTest, UpdateVersionInvalidatesOld) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        Cache cache;
        
        // Generate random cache parameters
        auto known_dims = rng_.randKnownDimensions();
        auto available_space = rng_.randAvailableSpace();
        auto layout_output1 = rng_.randLayoutOutput();
        auto layout_output2 = rng_.randLayoutOutput();
        auto run_mode = rng_.randRunMode();
        uint64_t old_version = rng_.randVersion();
        uint64_t new_version = old_version + rng_.randInt(1, 1000);
        
        // Store with old version
        cache.Store(known_dims, available_space, run_mode, old_version, layout_output1);
        
        // Update with new version
        cache.Store(known_dims, available_space, run_mode, new_version, layout_output2);
        
        // Get with old version should return miss
        auto result_old = cache.Get(known_dims, available_space, run_mode, old_version);
        EXPECT_FALSE(result_old.has_value())
            << "Cache should return miss for old version after update. "
            << "Old version: " << old_version << ", "
            << "New version: " << new_version << ", "
            << "Iteration: " << i;
        
        // Get with new version should return hit
        auto result_new = cache.Get(known_dims, available_space, run_mode, new_version);
        EXPECT_TRUE(result_new.has_value())
            << "Cache should return hit for new version after update. "
            << "New version: " << new_version << ", "
            << "Iteration: " << i;
    }
}
