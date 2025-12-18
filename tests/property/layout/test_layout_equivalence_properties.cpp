/**
 * @file test_layout_equivalence_properties.cpp
 * @brief Property-based tests for layout result equivalence
 * 
 * This file implements property-based testing for verifying that full layout
 * and incremental layout produce identical results when all nodes are dirty.
 * 
 * **Feature: incremental-layout-optimization**
 * **Property 7: 布局结果等价性**
 * **Validates: Requirements 6.1**
 */

#include <gtest/gtest.h>
#include "layout/native_layout_engine.h"
#include "layout/content_version.h"
#include "render/render_object.h"
#include <random>
#include <memory>
#include <vector>
#include <cmath>

using namespace lightui;

// Random number generator for property tests
class LayoutEquivalencePropertyTestRng {
public:
    LayoutEquivalencePropertyTestRng() : gen_(std::random_device{}()) {}
    
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
    
    // Generate a random fixed pixel value
    float randFixedSize() {
        return randFloat(50.0f, 400.0f);
    }
    
    // Generate random padding/margin value
    float randSpacing() {
        return randFloat(0.0f, 20.0f);
    }
    
private:
    std::mt19937 gen_;
};

/**
 * @brief Test fixture for layout equivalence property tests
 */
class LayoutEquivalencePropertyTest : public ::testing::Test {
protected:
    LayoutEquivalencePropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    static constexpr float EPSILON = 0.01f;  // Tolerance for floating point comparison
    
    void SetUp() override {
        ContentVersionManager::GetInstance().Reset();
    }
    
    // Helper to create a render block
    std::shared_ptr<RenderBlock> createRenderBlock() {
        return std::make_shared<RenderBlock>();
    }
    
    // Helper to create a render text
    std::shared_ptr<RenderText> createRenderText(const std::string& text) {
        return std::make_shared<RenderText>(text);
    }
    
    // Helper to apply fixed width and height to a render object
    void applyFixedSize(RenderObject* obj, float width, float height) {
        ComputedStyle style = obj->GetComputedStyle();
        style.width = CSSLength(width, CSSUnit::PX);
        style.height = CSSLength(height, CSSUnit::PX);
        obj->SetComputedStyle(style);
    }
    
    // Helper to apply auto dimensions
    void applyAutoSize(RenderObject* obj) {
        ComputedStyle style = obj->GetComputedStyle();
        style.width = CSSLength(0, CSSUnit::AUTO);
        style.height = CSSLength(0, CSSUnit::AUTO);
        obj->SetComputedStyle(style);
    }
    
    // Helper to apply padding
    void applyPadding(RenderObject* obj, float top, float right, float bottom, float left) {
        ComputedStyle style = obj->GetComputedStyle();
        style.padding_top = CSSLength(top, CSSUnit::PX);
        style.padding_right = CSSLength(right, CSSUnit::PX);
        style.padding_bottom = CSSLength(bottom, CSSUnit::PX);
        style.padding_left = CSSLength(left, CSSUnit::PX);
        obj->SetComputedStyle(style);
    }
    
    // Helper to apply margin
    void applyMargin(RenderObject* obj, float top, float right, float bottom, float left) {
        ComputedStyle style = obj->GetComputedStyle();
        style.margin_top = CSSLength(top, CSSUnit::PX);
        style.margin_right = CSSLength(right, CSSUnit::PX);
        style.margin_bottom = CSSLength(bottom, CSSUnit::PX);
        style.margin_left = CSSLength(left, CSSUnit::PX);
        obj->SetComputedStyle(style);
    }
    
    // Helper to make a flex container
    void applyFlexContainer(RenderObject* obj) {
        ComputedStyle style = obj->GetComputedStyle();
        style.display = RenderObjectType::FLEX;
        obj->SetComputedStyle(style);
    }
    
    // Helper to compare two LayoutInfo structures
    bool layoutInfoEquals(const LayoutInfo& a, const LayoutInfo& b) {
        return std::abs(a.x - b.x) < EPSILON &&
               std::abs(a.y - b.y) < EPSILON &&
               std::abs(a.width - b.width) < EPSILON &&
               std::abs(a.height - b.height) < EPSILON;
    }
    
    // Helper to collect all layout info from a tree
    void collectLayoutInfo(RenderObject* obj, std::vector<LayoutInfo>& layouts) {
        if (!obj) return;
        layouts.push_back(obj->GetLayoutInfo());
        for (const auto& child : obj->GetChildren()) {
            collectLayoutInfo(child.get(), layouts);
        }
    }
    
    // Helper to mark all nodes as needing layout
    void markAllDirty(NativeLayoutEngine& engine, RenderObject* obj) {
        if (!obj) return;
        engine.MarkNeedsLayout(obj);
        for (const auto& child : obj->GetChildren()) {
            markAllDirty(engine, child.get());
        }
    }
};

/**
 * **Feature: incremental-layout-optimization, Property 7: 布局结果等价性**
 * 
 * For any layout tree, the result of ComputeLayout (full layout) SHALL be
 * identical to the result of ComputeIncrementalLayout when all nodes are
 * marked dirty.
 * 
 * This test creates random trees and verifies that both layout methods
 * produce the same results.
 * 
 * **Validates: Requirements 6.1**
 */
TEST_F(LayoutEquivalencePropertyTest, FullLayoutEqualsIncrementalWithAllDirty) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a random tree structure
        auto root = createRenderBlock();
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        // Create random children
        int num_children = rng_.randInt(1, 4);
        std::vector<std::shared_ptr<RenderBlock>> children;
        
        for (int j = 0; j < num_children; ++j) {
            auto child = createRenderBlock();
            
            // Randomly apply fixed or auto size
            if (rng_.randBool()) {
                applyFixedSize(child.get(), rng_.randFixedSize(), rng_.randFixedSize());
            } else {
                applyAutoSize(child.get());
            }
            
            // Randomly apply padding
            if (rng_.randBool()) {
                applyPadding(child.get(), 
                    rng_.randSpacing(), rng_.randSpacing(),
                    rng_.randSpacing(), rng_.randSpacing());
            }
            
            // Randomly apply margin
            if (rng_.randBool()) {
                applyMargin(child.get(),
                    rng_.randSpacing(), rng_.randSpacing(),
                    rng_.randSpacing(), rng_.randSpacing());
            }
            
            root->AppendChild(child);
            children.push_back(child);
        }
        
        // Engine 1: Full layout
        auto engine1 = std::make_unique<NativeLayoutEngine>();
        engine1->BuildLayoutTree(root);
        engine1->ComputeLayout(800.0f, 600.0f);
        engine1->GetLayoutInfo(root);
        
        // Collect full layout results
        std::vector<LayoutInfo> fullLayoutResults;
        collectLayoutInfo(root.get(), fullLayoutResults);
        
        // Engine 2: Incremental layout with all nodes dirty
        auto engine2 = std::make_unique<NativeLayoutEngine>();
        engine2->BuildLayoutTree(root);
        
        // Mark all nodes as dirty
        markAllDirty(*engine2, root.get());
        
        // Perform incremental layout
        engine2->ComputeIncrementalLayout(800.0f, 600.0f);
        engine2->GetLayoutInfo(root);
        
        // Collect incremental layout results
        std::vector<LayoutInfo> incrementalLayoutResults;
        collectLayoutInfo(root.get(), incrementalLayoutResults);
        
        // Verify results are identical
        ASSERT_EQ(fullLayoutResults.size(), incrementalLayoutResults.size())
            << "Layout result count mismatch. Iteration: " << i;
        
        for (size_t j = 0; j < fullLayoutResults.size(); ++j) {
            EXPECT_TRUE(layoutInfoEquals(fullLayoutResults[j], incrementalLayoutResults[j]))
                << "Layout mismatch at node " << j << ". "
                << "Full: (" << fullLayoutResults[j].x << ", " << fullLayoutResults[j].y 
                << ", " << fullLayoutResults[j].width << ", " << fullLayoutResults[j].height << ") "
                << "Incremental: (" << incrementalLayoutResults[j].x << ", " << incrementalLayoutResults[j].y
                << ", " << incrementalLayoutResults[j].width << ", " << incrementalLayoutResults[j].height << ") "
                << "Iteration: " << i;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 7: 布局结果等价性**
 * 
 * For any flex container tree, the result of ComputeLayout (full layout)
 * SHALL be identical to the result of ComputeIncrementalLayout when all
 * nodes are marked dirty.
 * 
 * **Validates: Requirements 6.1**
 */
TEST_F(LayoutEquivalencePropertyTest, FlexLayoutEquivalence) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a flex container with children
        auto root = createRenderBlock();
        applyFlexContainer(root.get());
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        // Create random flex children
        int num_children = rng_.randInt(2, 5);
        std::vector<std::shared_ptr<RenderBlock>> children;
        
        for (int j = 0; j < num_children; ++j) {
            auto child = createRenderBlock();
            
            // Flex children with various sizes
            if (rng_.randBool()) {
                applyFixedSize(child.get(), rng_.randFixedSize(), rng_.randFixedSize());
            } else {
                // Auto size - will be sized by flex algorithm
                applyAutoSize(child.get());
            }
            
            root->AppendChild(child);
            children.push_back(child);
        }
        
        // Engine 1: Full layout
        auto engine1 = std::make_unique<NativeLayoutEngine>();
        engine1->BuildLayoutTree(root);
        engine1->ComputeLayout(800.0f, 600.0f);
        engine1->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> fullLayoutResults;
        collectLayoutInfo(root.get(), fullLayoutResults);
        
        // Engine 2: Incremental layout with all nodes dirty
        auto engine2 = std::make_unique<NativeLayoutEngine>();
        engine2->BuildLayoutTree(root);
        markAllDirty(*engine2, root.get());
        engine2->ComputeIncrementalLayout(800.0f, 600.0f);
        engine2->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> incrementalLayoutResults;
        collectLayoutInfo(root.get(), incrementalLayoutResults);
        
        // Verify results are identical
        ASSERT_EQ(fullLayoutResults.size(), incrementalLayoutResults.size())
            << "Flex layout result count mismatch. Iteration: " << i;
        
        for (size_t j = 0; j < fullLayoutResults.size(); ++j) {
            EXPECT_TRUE(layoutInfoEquals(fullLayoutResults[j], incrementalLayoutResults[j]))
                << "Flex layout mismatch at node " << j << ". "
                << "Full: (" << fullLayoutResults[j].x << ", " << fullLayoutResults[j].y 
                << ", " << fullLayoutResults[j].width << ", " << fullLayoutResults[j].height << ") "
                << "Incremental: (" << incrementalLayoutResults[j].x << ", " << incrementalLayoutResults[j].y
                << ", " << incrementalLayoutResults[j].width << ", " << incrementalLayoutResults[j].height << ") "
                << "Iteration: " << i;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 7: 布局结果等价性**
 * 
 * For any nested tree structure, the result of ComputeLayout (full layout)
 * SHALL be identical to the result of ComputeIncrementalLayout when all
 * nodes are marked dirty.
 * 
 * **Validates: Requirements 6.1**
 */
TEST_F(LayoutEquivalencePropertyTest, NestedTreeLayoutEquivalence) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a nested tree: root -> level1 -> level2 -> level3
        auto root = createRenderBlock();
        auto level1 = createRenderBlock();
        auto level2 = createRenderBlock();
        auto level3 = createRenderBlock();
        
        root->AppendChild(level1);
        level1->AppendChild(level2);
        level2->AppendChild(level3);
        
        // Apply random sizes
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        if (rng_.randBool()) {
            applyFixedSize(level1.get(), rng_.randFixedSize(), rng_.randFixedSize());
        } else {
            applyAutoSize(level1.get());
        }
        
        if (rng_.randBool()) {
            applyFixedSize(level2.get(), rng_.randFixedSize(), rng_.randFixedSize());
        } else {
            applyAutoSize(level2.get());
        }
        
        applyFixedSize(level3.get(), rng_.randFixedSize(), rng_.randFixedSize());
        
        // Apply random padding/margin
        applyPadding(level1.get(), 
            rng_.randSpacing(), rng_.randSpacing(),
            rng_.randSpacing(), rng_.randSpacing());
        applyMargin(level2.get(),
            rng_.randSpacing(), rng_.randSpacing(),
            rng_.randSpacing(), rng_.randSpacing());
        
        // Engine 1: Full layout
        auto engine1 = std::make_unique<NativeLayoutEngine>();
        engine1->BuildLayoutTree(root);
        engine1->ComputeLayout(800.0f, 600.0f);
        engine1->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> fullLayoutResults;
        collectLayoutInfo(root.get(), fullLayoutResults);
        
        // Engine 2: Incremental layout with all nodes dirty
        auto engine2 = std::make_unique<NativeLayoutEngine>();
        engine2->BuildLayoutTree(root);
        markAllDirty(*engine2, root.get());
        engine2->ComputeIncrementalLayout(800.0f, 600.0f);
        engine2->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> incrementalLayoutResults;
        collectLayoutInfo(root.get(), incrementalLayoutResults);
        
        // Verify results are identical
        ASSERT_EQ(fullLayoutResults.size(), incrementalLayoutResults.size())
            << "Nested tree layout result count mismatch. Iteration: " << i;
        
        for (size_t j = 0; j < fullLayoutResults.size(); ++j) {
            EXPECT_TRUE(layoutInfoEquals(fullLayoutResults[j], incrementalLayoutResults[j]))
                << "Nested tree layout mismatch at node " << j << ". "
                << "Full: (" << fullLayoutResults[j].x << ", " << fullLayoutResults[j].y 
                << ", " << fullLayoutResults[j].width << ", " << fullLayoutResults[j].height << ") "
                << "Incremental: (" << incrementalLayoutResults[j].x << ", " << incrementalLayoutResults[j].y
                << ", " << incrementalLayoutResults[j].width << ", " << incrementalLayoutResults[j].height << ") "
                << "Iteration: " << i;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 7: 布局结果等价性**
 * 
 * For any tree with mixed display types (block and flex), the result of
 * ComputeLayout (full layout) SHALL be identical to the result of
 * ComputeIncrementalLayout when all nodes are marked dirty.
 * 
 * **Validates: Requirements 6.1**
 */
TEST_F(LayoutEquivalencePropertyTest, MixedDisplayTypeLayoutEquivalence) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a tree with mixed display types
        auto root = createRenderBlock();
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        // Block child
        auto blockChild = createRenderBlock();
        applyFixedSize(blockChild.get(), rng_.randFixedSize(), rng_.randFixedSize());
        root->AppendChild(blockChild);
        
        // Flex child
        auto flexChild = createRenderBlock();
        applyFlexContainer(flexChild.get());
        applyFixedSize(flexChild.get(), rng_.randFixedSize(), rng_.randFixedSize());
        root->AppendChild(flexChild);
        
        // Add children to flex container
        for (int j = 0; j < rng_.randInt(1, 3); ++j) {
            auto flexGrandchild = createRenderBlock();
            applyFixedSize(flexGrandchild.get(), rng_.randFixedSize(), rng_.randFixedSize());
            flexChild->AppendChild(flexGrandchild);
        }
        
        // Engine 1: Full layout
        auto engine1 = std::make_unique<NativeLayoutEngine>();
        engine1->BuildLayoutTree(root);
        engine1->ComputeLayout(800.0f, 600.0f);
        engine1->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> fullLayoutResults;
        collectLayoutInfo(root.get(), fullLayoutResults);
        
        // Engine 2: Incremental layout with all nodes dirty
        auto engine2 = std::make_unique<NativeLayoutEngine>();
        engine2->BuildLayoutTree(root);
        markAllDirty(*engine2, root.get());
        engine2->ComputeIncrementalLayout(800.0f, 600.0f);
        engine2->GetLayoutInfo(root);
        
        std::vector<LayoutInfo> incrementalLayoutResults;
        collectLayoutInfo(root.get(), incrementalLayoutResults);
        
        // Verify results are identical
        ASSERT_EQ(fullLayoutResults.size(), incrementalLayoutResults.size())
            << "Mixed display type layout result count mismatch. Iteration: " << i;
        
        for (size_t j = 0; j < fullLayoutResults.size(); ++j) {
            EXPECT_TRUE(layoutInfoEquals(fullLayoutResults[j], incrementalLayoutResults[j]))
                << "Mixed display type layout mismatch at node " << j << ". "
                << "Full: (" << fullLayoutResults[j].x << ", " << fullLayoutResults[j].y 
                << ", " << fullLayoutResults[j].width << ", " << fullLayoutResults[j].height << ") "
                << "Incremental: (" << incrementalLayoutResults[j].x << ", " << incrementalLayoutResults[j].y
                << ", " << incrementalLayoutResults[j].width << ", " << incrementalLayoutResults[j].height << ") "
                << "Iteration: " << i;
        }
    }
}
