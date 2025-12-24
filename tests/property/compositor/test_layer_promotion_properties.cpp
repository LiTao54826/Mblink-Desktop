/**
 * @file test_layer_promotion_properties.cpp
 * @brief Property-based tests for layer promotion logic
 *
 * This file implements property-based testing for the LayerTreeBuilder's
 * layer promotion functionality.
 *
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 * **Validates: Requirements 2.1, 2.2, 2.3**
 */

#include <gtest/gtest.h>
#include "core/compositor/layer_tree_builder.h"
#include "core/compositor/compositor_layer.h"
#include "core/render/render_object.h"
#include <random>
#include <vector>
#include <string>

using namespace lightui;

// Random number generator for property tests
class LayerPromotionPropertyTestRng {
public:
    LayerPromotionPropertyTestRng() : gen_(std::random_device{}()) {}

    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }

    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }

    bool randBool() {
        return randInt(0, 1) == 1;
    }

    std::string randPosition() {
        const char* positions[] = {"static", "relative", "absolute", "fixed"};
        return positions[randInt(0, 3)];
    }

    std::string randOverflow() {
        const char* overflows[] = {"visible", "hidden", "scroll", "auto"};
        return overflows[randInt(0, 3)];
    }

private:
    std::mt19937 gen_;
};

class LayerPromotionPropertyTest : public ::testing::Test {
protected:
    LayerPromotionPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;

    void SetUp() override {
    }

    void TearDown() override {
    }

    // Helper to create a render object with layout
    std::shared_ptr<RenderBlock> CreateRenderBlock(float x, float y, float w, float h) {
        auto obj = std::make_shared<RenderBlock>();
        auto& layout = obj->GetLayoutInfo();
        layout.x = x;
        layout.y = y;
        layout.width = w;
        layout.height = h;
        layout.is_laid_out = true;
        return obj;
    }
};

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * For any element with `position: fixed`, the system SHALL create a dedicated
 * compositing layer for that element.
 *
 * **Validates: Requirements 2.2**
 */
TEST_F(LayerPromotionPropertyTest, PositionFixedCreatesLayer) {
    LayerTreeBuilder builder;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto obj = CreateRenderBlock(
            rng_.randFloat(0, 100),
            rng_.randFloat(0, 100),
            rng_.randFloat(50, 200),
            rng_.randFloat(50, 200)
        );

        // Set position: fixed
        obj->GetComputedStyle().position = "fixed";

        LayerPromotionReason reason = builder.ShouldPromote(obj.get());

        EXPECT_EQ(reason, LayerPromotionReason::PositionFixed)
            << "Element with position: fixed should be promoted (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * For any element with `position: static`, the system SHALL NOT create a
 * dedicated compositing layer (unless other promotion reasons exist).
 *
 * **Validates: Requirements 2.1, 2.2, 2.3**
 */
TEST_F(LayerPromotionPropertyTest, PositionStaticNoLayer) {
    LayerTreeBuilder builder;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto obj = CreateRenderBlock(
            rng_.randFloat(0, 100),
            rng_.randFloat(0, 100),
            rng_.randFloat(50, 200),
            rng_.randFloat(50, 200)
        );

        // Set position: static (default)
        obj->GetComputedStyle().position = "static";
        // Ensure no other promotion reasons
        obj->GetComputedStyle().overflow = "visible";
        obj->GetComputedStyle().overflow_x = "visible";
        obj->GetComputedStyle().overflow_y = "visible";

        LayerPromotionReason reason = builder.ShouldPromote(obj.get());

        EXPECT_EQ(reason, LayerPromotionReason::None)
            << "Element with position: static should not be promoted (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * For any scrollable container with overflow content, the system SHALL create
 * a dedicated compositing layer.
 *
 * **Validates: Requirements 2.4**
 */
TEST_F(LayerPromotionPropertyTest, ScrollableContainerCreatesLayer) {
    LayerTreeBuilder builder;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto obj = CreateRenderBlock(0, 0, 100, 100);

        // Set overflow: scroll
        obj->GetComputedStyle().overflow_y = "scroll";

        // Set content size larger than container
        obj->SetContentSize(100, 200);  // Content height > container height

        LayerPromotionReason reason = builder.ShouldPromote(obj.get());

        EXPECT_EQ(reason, LayerPromotionReason::ScrollableContent)
            << "Scrollable container with overflow content should be promoted (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * For any container with overflow: scroll but no overflow content, the system
 * SHALL NOT create a dedicated compositing layer.
 *
 * **Validates: Requirements 2.4**
 */
TEST_F(LayerPromotionPropertyTest, ScrollableWithoutOverflowNoLayer) {
    LayerTreeBuilder builder;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto obj = CreateRenderBlock(0, 0, 200, 200);

        // Set overflow: scroll
        obj->GetComputedStyle().overflow_y = "scroll";

        // Set content size smaller than container
        obj->SetContentSize(100, 100);  // Content fits in container

        LayerPromotionReason reason = builder.ShouldPromote(obj.get());

        EXPECT_EQ(reason, LayerPromotionReason::None)
            << "Scrollable container without overflow content should not be promoted (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * Layer promotion should be disabled when SetLayerPromotionEnabled(false) is called.
 *
 * **Validates: Requirements 8.3**
 */
TEST_F(LayerPromotionPropertyTest, DisabledPromotionReturnsNone) {
    LayerTreeBuilder builder;
    builder.SetLayerPromotionEnabled(false);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto obj = CreateRenderBlock(0, 0, 100, 100);

        // Set various promotion triggers
        if (rng_.randBool()) {
            obj->GetComputedStyle().position = "fixed";
        }
        if (rng_.randBool()) {
            obj->GetComputedStyle().overflow_y = "scroll";
            obj->SetContentSize(100, 200);
        }

        LayerPromotionReason reason = builder.ShouldPromote(obj.get());

        EXPECT_EQ(reason, LayerPromotionReason::None)
            << "Disabled promotion should always return None (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * Building a layer tree should create exactly one layer for each promoted element
 * plus the root layer.
 *
 * **Validates: Requirements 2.1, 2.2, 2.3, 2.4**
 */
TEST_F(LayerPromotionPropertyTest, LayerCountMatchesPromotedElements) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        LayerTreeBuilder builder;

        // Create root
        auto root = CreateRenderBlock(0, 0, 800, 600);

        // Create random number of children
        int num_children = rng_.randInt(1, 5);
        int expected_promoted = 0;

        for (int j = 0; j < num_children; ++j) {
            auto child = CreateRenderBlock(
                rng_.randFloat(0, 100),
                rng_.randFloat(0, 100),
                rng_.randFloat(50, 100),
                rng_.randFloat(50, 100)
            );

            // Randomly make some children promotable
            if (rng_.randBool()) {
                child->GetComputedStyle().position = "fixed";
                expected_promoted++;
            }

            root->AppendChild(child);
        }

        // Build layer tree
        auto root_layer = builder.Build(root.get());

        // Layer count should be: 1 (root) + promoted children
        EXPECT_EQ(builder.GetLayerCount(), static_cast<size_t>(1 + expected_promoted))
            << "Layer count should match promoted elements + root (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * Each promoted element should have a corresponding layer with correct bounds.
 *
 * **Validates: Requirements 2.1, 2.2, 2.3**
 */
TEST_F(LayerPromotionPropertyTest, PromotedElementHasLayerWithCorrectBounds) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        LayerTreeBuilder builder;

        // Create root
        auto root = CreateRenderBlock(0, 0, 800, 600);

        // Create a fixed-position child
        float x = rng_.randFloat(10, 100);
        float y = rng_.randFloat(10, 100);
        float w = rng_.randFloat(50, 200);
        float h = rng_.randFloat(50, 200);

        auto child = CreateRenderBlock(x, y, w, h);
        child->GetComputedStyle().position = "fixed";
        root->AppendChild(child);

        // Build layer tree
        builder.Build(root.get());

        // Get layer for child
        auto layer = builder.GetLayerForRenderObject(child.get());

        ASSERT_NE(layer, nullptr)
            << "Promoted element should have a layer (iteration " << i << ")";

        const auto& bounds = layer->GetBounds();
        EXPECT_FLOAT_EQ(bounds.x(), x)
            << "Layer x should match element x (iteration " << i << ")";
        EXPECT_FLOAT_EQ(bounds.y(), y)
            << "Layer y should match element y (iteration " << i << ")";
        EXPECT_FLOAT_EQ(bounds.width(), w)
            << "Layer width should match element width (iteration " << i << ")";
        EXPECT_FLOAT_EQ(bounds.height(), h)
            << "Layer height should match element height (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 2: Layer promotion based on CSS properties**
 *
 * Clearing the builder should reset all state.
 *
 * **Validates: Requirements 2.5**
 */
TEST_F(LayerPromotionPropertyTest, ClearResetsState) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        LayerTreeBuilder builder;

        // Create and build a tree
        auto root = CreateRenderBlock(0, 0, 800, 600);
        auto child = CreateRenderBlock(10, 10, 100, 100);
        child->GetComputedStyle().position = "fixed";
        root->AppendChild(child);

        builder.Build(root.get());

        EXPECT_GT(builder.GetLayerCount(), 0u)
            << "Should have layers after build (iteration " << i << ")";

        // Clear
        builder.Clear();

        EXPECT_EQ(builder.GetLayerCount(), 0u)
            << "Layer count should be 0 after clear (iteration " << i << ")";

        EXPECT_EQ(builder.GetLayerForRenderObject(root.get()), nullptr)
            << "Root layer should be null after clear (iteration " << i << ")";

        EXPECT_EQ(builder.GetLayerForRenderObject(child.get()), nullptr)
            << "Child layer should be null after clear (iteration " << i << ")";
    }
}



/**
 * **Feature: layer-compositing-architecture, Property 10: Layer cleanup on condition change**
 *
 * When a layer's promotion reason is removed (e.g., animation completes),
 * the layer SHALL be demoted and merged back to its parent.
 *
 * **Validates: Requirements 2.5, 7.4**
 */
TEST_F(LayerPromotionPropertyTest, LayerDemotedWhenReasonRemoved) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        LayerTreeBuilder builder;

        // Create root
        auto root = CreateRenderBlock(0, 0, 800, 600);

        // Create a fixed-position child
        auto child = CreateRenderBlock(10, 10, 100, 100);
        child->GetComputedStyle().position = "fixed";
        root->AppendChild(child);

        // Build layer tree
        builder.Build(root.get());

        // Verify child has a layer
        ASSERT_NE(builder.GetLayerForRenderObject(child.get()), nullptr)
            << "Child should have a layer initially (iteration " << i << ")";

        size_t initial_count = builder.GetLayerCount();

        // Remove the promotion reason
        child->GetComputedStyle().position = "static";

        // Update the layer tree
        builder.Update(child.get());

        // Verify layer count decreased
        EXPECT_LT(builder.GetLayerCount(), initial_count)
            << "Layer count should decrease after removing promotion reason (iteration " << i << ")";

        // Verify child no longer has a layer
        EXPECT_EQ(builder.GetLayerForRenderObject(child.get()), nullptr)
            << "Child should not have a layer after demotion (iteration " << i << ")";
    }
}

/**
 * **Feature: layer-compositing-architecture, Property 10: Layer cleanup on condition change**
 *
 * When rebuilding the layer tree, old layers should be properly cleaned up.
 *
 * **Validates: Requirements 2.5**
 */
TEST_F(LayerPromotionPropertyTest, RebuildCleansUpOldLayers) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        LayerTreeBuilder builder;

        // Create first tree
        auto root1 = CreateRenderBlock(0, 0, 800, 600);
        auto child1 = CreateRenderBlock(10, 10, 100, 100);
        child1->GetComputedStyle().position = "fixed";
        root1->AppendChild(child1);

        builder.Build(root1.get());
        size_t count1 = builder.GetLayerCount();

        // Create second tree (different structure)
        auto root2 = CreateRenderBlock(0, 0, 800, 600);
        // No fixed-position children

        builder.Build(root2.get());
        size_t count2 = builder.GetLayerCount();

        // Second tree should have fewer layers
        EXPECT_LT(count2, count1)
            << "Rebuilding with fewer promoted elements should reduce layer count (iteration " << i << ")";

        // Old layers should be cleaned up
        EXPECT_EQ(builder.GetLayerForRenderObject(root1.get()), nullptr)
            << "Old root should not have a layer after rebuild (iteration " << i << ")";
        EXPECT_EQ(builder.GetLayerForRenderObject(child1.get()), nullptr)
            << "Old child should not have a layer after rebuild (iteration " << i << ")";
    }
}

