/**
 * @file test_ifc_cache_properties.cpp
 * @brief Property-based tests for IFC cache independence
 * 
 * This file implements property-based testing for the IFCLayout cache
 * with content version support. Each property test runs 100 iterations
 * with randomly generated inputs.
 * 
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * **Validates: Requirements 3.4, 3.5**
 */

#include <gtest/gtest.h>
#include "core/layout/ifc/ifc_layout.h"
#include "core/render/objects/render_object.h"
#include <random>
#include <memory>
#include <vector>

using namespace lightui;

// Random number generator for property tests
class IFCCachePropertyTestRng {
public:
    IFCCachePropertyTestRng() : gen_(std::random_device{}()) {}
    
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
    
    float randAvailableWidth() {
        return randFloat(100.0f, 800.0f);
    }
    
    std::string randText() {
        static const std::vector<std::string> texts = {
            "Hello World",
            "Test content",
            "Lorem ipsum dolor sit amet",
            "Short",
            "A longer piece of text for testing purposes"
        };
        return texts[randInt(0, texts.size() - 1)];
    }
    
private:
    std::mt19937 gen_;
};

class IFCCachePropertyTest : public ::testing::Test {
protected:
    IFCCachePropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    // Helper to create a simple IFC container with text
    // Returns a unique RenderBlock with a RenderText child
    std::shared_ptr<RenderObject> CreateIFCContainer(const std::string& text) {
        // Create render objects directly without DOM nodes
        auto render_div = std::make_shared<RenderBlock>();
        auto render_text = std::make_shared<RenderText>();
        render_text->SetText(text);
        
        // Add child using GetChildrenMutable()
        render_div->GetChildrenMutable().push_back(render_text);
        
        return render_div;
    }
    
    // Keep containers alive during tests
    std::vector<std::shared_ptr<RenderObject>> containers_;
};

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * For any two distinct IFC containers, invalidating one container's cache
 * SHALL NOT affect the other container's cache validity.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, IndependentContainerCaches) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        // Create two distinct containers
        auto container1 = CreateIFCContainer(rng_.randText());
        auto container2 = CreateIFCContainer(rng_.randText());
        
        float width1 = rng_.randAvailableWidth();
        float width2 = rng_.randAvailableWidth();
        uint64_t version1 = rng_.randVersion();
        uint64_t version2 = rng_.randVersion();
        
        // Layout both containers (this populates their caches)
        ifc_layout.Layout(container1.get(), width1, false, version1);
        ifc_layout.Layout(container2.get(), width2, false, version2);
        
        // Verify both caches are valid
        EXPECT_TRUE(ifc_layout.IsCacheValid(container1.get(), width1, version1))
            << "Container1 cache should be valid after layout. Iteration: " << i;
        EXPECT_TRUE(ifc_layout.IsCacheValid(container2.get(), width2, version2))
            << "Container2 cache should be valid after layout. Iteration: " << i;
        
        // Invalidate container1's cache
        ifc_layout.InvalidateCache(container1.get());
        
        // Container1's cache should be invalid
        EXPECT_FALSE(ifc_layout.IsCacheValid(container1.get(), width1, version1))
            << "Container1 cache should be invalid after InvalidateCache. Iteration: " << i;
        
        // Container2's cache should still be valid (independence)
        EXPECT_TRUE(ifc_layout.IsCacheValid(container2.get(), width2, version2))
            << "Container2 cache should remain valid after invalidating Container1. Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * For any IFC container, changing the content_version SHALL invalidate
 * the cache for that container only.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, VersionChangeInvalidatesOnlyTargetContainer) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        // Create two distinct containers
        auto container1 = CreateIFCContainer(rng_.randText());
        auto container2 = CreateIFCContainer(rng_.randText());
        
        float width1 = rng_.randAvailableWidth();
        float width2 = rng_.randAvailableWidth();
        uint64_t version1 = rng_.randVersion();
        uint64_t version2 = rng_.randVersion();
        
        // Layout both containers
        ifc_layout.Layout(container1.get(), width1, false, version1);
        ifc_layout.Layout(container2.get(), width2, false, version2);
        
        // Change version for container1
        uint64_t new_version1 = version1 + rng_.randInt(1, 1000);
        
        // Container1 with new version should be cache miss
        EXPECT_FALSE(ifc_layout.IsCacheValid(container1.get(), width1, new_version1))
            << "Container1 cache should be invalid with new version. Iteration: " << i;
        
        // Container2 should still be valid (independence)
        EXPECT_TRUE(ifc_layout.IsCacheValid(container2.get(), width2, version2))
            << "Container2 cache should remain valid when Container1 version changes. Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * For any IFC container, the cache SHALL be valid when queried with
 * the same available_width and content_version used during layout.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, CacheValidWithMatchingParameters) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        auto container = CreateIFCContainer(rng_.randText());
        float width = rng_.randAvailableWidth();
        uint64_t version = rng_.randVersion();
        
        // Layout the container
        auto result = ifc_layout.Layout(container.get(), width, false, version);
        EXPECT_TRUE(result.success) << "Layout should succeed. Iteration: " << i;
        
        // Cache should be valid with same parameters
        EXPECT_TRUE(ifc_layout.IsCacheValid(container.get(), width, version))
            << "Cache should be valid with matching parameters. Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * For any IFC container, the cache SHALL be invalid when queried with
 * a different available_width (even with same content_version).
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, CacheInvalidWithDifferentWidth) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        auto container = CreateIFCContainer(rng_.randText());
        float width1 = rng_.randAvailableWidth();
        float width2 = width1 + rng_.randFloat(10.0f, 100.0f);  // Different width
        uint64_t version = rng_.randVersion();
        
        // Layout the container
        ifc_layout.Layout(container.get(), width1, false, version);
        
        // Cache should be invalid with different width
        EXPECT_FALSE(ifc_layout.IsCacheValid(container.get(), width2, version))
            << "Cache should be invalid with different width. Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * ClearCache() SHALL invalidate all container caches.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, ClearCacheInvalidatesAll) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        // Create multiple containers
        int num_containers = rng_.randInt(2, 5);
        std::vector<std::shared_ptr<RenderObject>> containers;
        std::vector<float> widths;
        std::vector<uint64_t> versions;
        
        for (int j = 0; j < num_containers; ++j) {
            auto container = CreateIFCContainer(rng_.randText());
            float width = rng_.randAvailableWidth();
            uint64_t version = rng_.randVersion();
            
            ifc_layout.Layout(container.get(), width, false, version);
            
            containers.push_back(container);
            widths.push_back(width);
            versions.push_back(version);
        }
        
        // Clear all caches
        ifc_layout.ClearCache();
        
        // All caches should be invalid
        for (int j = 0; j < num_containers; ++j) {
            EXPECT_FALSE(ifc_layout.IsCacheValid(containers[j].get(), widths[j], versions[j]))
                << "Container " << j << " cache should be invalid after ClearCache. Iteration: " << i;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * For any IFC container, using version 0 (backward compatibility mode)
 * SHALL use the old hash-based cache validation.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, ZeroVersionUsesHashBasedValidation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        auto container = CreateIFCContainer(rng_.randText());
        float width = rng_.randAvailableWidth();
        
        // Layout with version 0 (backward compatibility)
        ifc_layout.Layout(container.get(), width, false, 0);
        
        // Cache should be valid with version 0 (uses hash-based validation)
        EXPECT_TRUE(ifc_layout.IsCacheValid(container.get(), width, 0))
            << "Cache should be valid with version 0 (hash-based). Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 8: IFC 缓存独立性**
 * 
 * Re-layout with the same parameters SHALL return cached results.
 * 
 * **Validates: Requirements 3.4, 3.5**
 */
TEST_F(IFCCachePropertyTest, RelayoutReturnsCachedResults) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        IFCLayout ifc_layout;
        
        auto container = CreateIFCContainer(rng_.randText());
        float width = rng_.randAvailableWidth();
        uint64_t version = rng_.randVersion();
        
        // First layout
        auto result1 = ifc_layout.Layout(container.get(), width, false, version);
        EXPECT_TRUE(result1.success) << "First layout should succeed. Iteration: " << i;
        
        // Second layout with same parameters (should use cache)
        auto result2 = ifc_layout.Layout(container.get(), width, false, version);
        EXPECT_TRUE(result2.success) << "Second layout should succeed. Iteration: " << i;
        
        // Results should be identical
        EXPECT_FLOAT_EQ(result1.total_height, result2.total_height)
            << "Cached height should match. Iteration: " << i;
        EXPECT_FLOAT_EQ(result1.max_width, result2.max_width)
            << "Cached width should match. Iteration: " << i;
        EXPECT_EQ(result1.line_count, result2.line_count)
            << "Cached line count should match. Iteration: " << i;
    }
}

