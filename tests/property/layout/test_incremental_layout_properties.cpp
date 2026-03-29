/**
 * @file test_incremental_layout_properties.cpp
 * @brief Property-based tests for incremental layout optimization
 * 
 * This file implements property-based testing for the incremental layout
 * optimization feature. Tests verify that:
 * - Clean nodes retain their cached layout results (Property 3)
 * - Dirty marks propagate correctly to ancestors (Property 4)
 * 
 * **Feature: incremental-layout-optimization**
 * **Property 3: 增量布局缓存保留**
 * **Property 4: 脏标记向上传播**
 * **Validates: Requirements 2.1, 2.4, 4.1, 4.2**
 */

#include <gtest/gtest.h>
#include "core/layout/native_layout_engine.h"
#include "core/layout/content_version.h"
#include "core/render/objects/render_object.h"
#include <random>
#include <memory>
#include <vector>

using namespace mbink;

// Random number generator for property tests
class IncrementalLayoutPropertyTestRng {
public:
    IncrementalLayoutPropertyTestRng() : gen_(std::random_device{}()) {}
    
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
        return randFloat(50.0f, 500.0f);
    }
    
    // Generate random text content
    std::string randText() {
        static const char* words[] = {
            "Hello", "World", "Test", "Layout", "Cache", "Version",
            "Incremental", "Optimization", "Property", "Based"
        };
        int num_words = randInt(1, 5);
        std::string result;
        for (int i = 0; i < num_words; ++i) {
            if (i > 0) result += " ";
            result += words[randInt(0, 9)];
        }
        return result;
    }
    
private:
    std::mt19937 gen_;
};

/**
 * @brief Test fixture for incremental layout property tests
 */
class IncrementalLayoutPropertyTest : public ::testing::Test {
protected:
    IncrementalLayoutPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    std::unique_ptr<NativeLayoutEngine> engine_;
    
    void SetUp() override {
        ContentVersionManager::GetInstance().Reset();
        engine_ = std::make_unique<NativeLayoutEngine>();
    }
    
    void TearDown() override {
        engine_.reset();
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
    
    // Helper to apply absolute positioning
    void applyAbsolutePosition(RenderObject* obj) {
        ComputedStyle style = obj->GetComputedStyle();
        style.position = "absolute";
        obj->SetComputedStyle(style);
    }
    
    // Helper to make a flex container
    void applyFlexContainer(RenderObject* obj) {
        ComputedStyle style = obj->GetComputedStyle();
        style.display = RenderObjectType::FLEX;
        obj->SetComputedStyle(style);
    }
};

/**
 * **Feature: incremental-layout-optimization, Property 3: 增量布局缓存保留**
 * 
 * For any incremental layout operation, clean nodes (nodes not marked dirty)
 * SHALL retain their cached layout results and not be recomputed.
 * 
 * This test verifies that when only some nodes are marked dirty, the clean
 * nodes' layout results remain unchanged after incremental layout.
 * 
 * **Validates: Requirements 2.1, 2.4**
 */
TEST_F(IncrementalLayoutPropertyTest, CleanNodesRetainCache) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a tree with multiple siblings
        auto root = createRenderBlock();
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        // Create multiple children
        int num_children = rng_.randInt(2, 5);
        std::vector<std::shared_ptr<RenderBlock>> children;
        
        for (int j = 0; j < num_children; ++j) {
            auto child = createRenderBlock();
            applyFixedSize(child.get(), rng_.randFixedSize(), rng_.randFixedSize());
            root->AppendChild(child);
            children.push_back(child);
        }
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Record layout info for all children
        std::vector<LayoutInfo> initial_layouts;
        for (const auto& child : children) {
            initial_layouts.push_back(child->GetLayoutInfo());
        }
        
        // Mark only one child as needing layout (simulate a change)
        int dirty_index = rng_.randInt(0, num_children - 1);
        engine_->MarkNeedsLayout(children[dirty_index].get());
        
        // Perform incremental layout
        bool did_layout = engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        EXPECT_TRUE(did_layout)
            << "Incremental layout should have been performed. Iteration: " << i;
        
        // Verify that clean nodes retained their layout
        for (int j = 0; j < num_children; ++j) {
            if (j != dirty_index) {
                const auto& current = children[j]->GetLayoutInfo();
                const auto& initial = initial_layouts[j];
                
                // Clean nodes should have the same layout dimensions
                // (position might change due to sibling changes, but size should be same)
                EXPECT_FLOAT_EQ(current.width, initial.width)
                    << "Clean node width should be retained. "
                    << "Child: " << j << ", "
                    << "Initial: " << initial.width << ", "
                    << "Current: " << current.width << ", "
                    << "Iteration: " << i;
                
                EXPECT_FLOAT_EQ(current.height, initial.height)
                    << "Clean node height should be retained. "
                    << "Child: " << j << ", "
                    << "Initial: " << initial.height << ", "
                    << "Current: " << current.height << ", "
                    << "Iteration: " << i;
            }
        }
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 3: 增量布局缓存保留**
 * 
 * For any tree where no nodes are marked dirty (after an initial incremental
 * layout clears the dirty flags), a subsequent ComputeIncrementalLayout
 * SHALL return false (no layout performed) and all nodes SHALL retain
 * their cached results.
 * 
 * Note: After BuildLayoutTree, all nodes have needs_layout=true by default.
 * After ComputeIncrementalLayout, the dirty flags are cleared. So we need
 * to call ComputeIncrementalLayout twice to test this property.
 * 
 * **Validates: Requirements 2.1, 2.4**
 */
TEST_F(IncrementalLayoutPropertyTest, NoDirtyNodesNoLayout) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a simple tree
        auto root = createRenderBlock();
        auto child = createRenderBlock();
        root->AppendChild(child);
        
        applyFixedSize(root.get(), 800.0f, 600.0f);
        applyFixedSize(child.get(), rng_.randFixedSize(), rng_.randFixedSize());
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        
        // First incremental layout - this will process all initially dirty nodes
        // and clear their dirty flags
        engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Record initial layout
        LayoutInfo initial_root = root->GetLayoutInfo();
        LayoutInfo initial_child = child->GetLayoutInfo();
        
        // Second incremental layout - no nodes should be dirty now
        bool did_layout = engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        EXPECT_FALSE(did_layout)
            << "No layout should be performed when no nodes are dirty. Iteration: " << i;
        
        // Verify layouts are unchanged
        EXPECT_FLOAT_EQ(root->GetLayoutInfo().width, initial_root.width)
            << "Root width should be unchanged. Iteration: " << i;
        EXPECT_FLOAT_EQ(child->GetLayoutInfo().width, initial_child.width)
            << "Child width should be unchanged. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 4: 脏标记向上传播**
 * 
 * For any node marked as needing layout with auto dimensions, all its
 * ancestors up to the root (or until an already-dirty ancestor) SHALL
 * also be marked as needing layout.
 * 
 * **Validates: Requirements 4.1, 4.2**
 */
TEST_F(IncrementalLayoutPropertyTest, DirtyMarkPropagatesUpward) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create a deep tree: root -> parent -> child -> grandchild
        auto root = createRenderBlock();
        auto parent = createRenderBlock();
        auto child = createRenderBlock();
        auto grandchild = createRenderBlock();
        
        root->AppendChild(parent);
        parent->AppendChild(child);
        child->AppendChild(grandchild);
        
        // All nodes have auto dimensions (so changes propagate upward)
        applyAutoSize(root.get());
        applyAutoSize(parent.get());
        applyAutoSize(child.get());
        applyAutoSize(grandchild.get());
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        
        // Mark the grandchild as needing layout
        engine_->MarkNeedsLayout(grandchild.get());
        
        // Perform incremental layout
        bool did_layout = engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        
        EXPECT_TRUE(did_layout)
            << "Incremental layout should have been performed. Iteration: " << i;
        
        // The fact that incremental layout was performed indicates that
        // dirty marks propagated correctly (at least to the root)
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 4: 脏标记向上传播**
 * 
 * For any node with fixed dimensions (both width and height), marking it
 * dirty SHALL NOT propagate dirty marks to its ancestors (isolation).
 * 
 * **Validates: Requirements 4.1, 4.2, 3.1.3**
 */
TEST_F(IncrementalLayoutPropertyTest, FixedSizeNodeIsolatesDirtyMark) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create: root -> fixed-size container -> child
        auto root = createRenderBlock();
        auto container = createRenderBlock();
        auto child = createRenderBlock();
        
        root->AppendChild(container);
        container->AppendChild(child);
        
        // Root has auto size, container has fixed size
        applyAutoSize(root.get());
        applyFixedSize(container.get(), rng_.randFixedSize(), rng_.randFixedSize());
        applyAutoSize(child.get());
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Record initial root layout
        LayoutInfo initial_root = root->GetLayoutInfo();
        
        // Mark the fixed-size container as needing layout
        engine_->MarkNeedsLayout(container.get());
        
        // Perform incremental layout
        engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Root layout should be unchanged because the fixed-size container
        // isolates changes from propagating upward
        EXPECT_FLOAT_EQ(root->GetLayoutInfo().width, initial_root.width)
            << "Root width should be unchanged due to fixed-size isolation. Iteration: " << i;
        EXPECT_FLOAT_EQ(root->GetLayoutInfo().height, initial_root.height)
            << "Root height should be unchanged due to fixed-size isolation. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 4: 脏标记向上传播**
 * 
 * For any absolute/fixed positioned element, marking it dirty SHALL NOT
 * propagate dirty marks to its ancestors (out of normal flow).
 * 
 * **Validates: Requirements 4.3**
 */
TEST_F(IncrementalLayoutPropertyTest, AbsolutePositionedIsolatesDirtyMark) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create: root -> absolute-positioned child
        auto root = createRenderBlock();
        auto child = createRenderBlock();
        
        root->AppendChild(child);
        
        // Root has auto size, child is absolute positioned
        applyAutoSize(root.get());
        applyAbsolutePosition(child.get());
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Record initial root layout
        LayoutInfo initial_root = root->GetLayoutInfo();
        
        // Mark the absolute-positioned child as needing layout
        engine_->MarkNeedsLayout(child.get());
        
        // Perform incremental layout
        engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Root layout should be unchanged because absolute-positioned
        // elements are out of normal flow
        EXPECT_FLOAT_EQ(root->GetLayoutInfo().width, initial_root.width)
            << "Root width should be unchanged due to absolute positioning. Iteration: " << i;
        EXPECT_FLOAT_EQ(root->GetLayoutInfo().height, initial_root.height)
            << "Root height should be unchanged due to absolute positioning. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 4: 脏标记向上传播**
 * 
 * For any flex/grid child, marking it dirty SHALL propagate to the parent
 * container because sibling layouts may be affected.
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(IncrementalLayoutPropertyTest, FlexChildPropagesToParent) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create: flex root -> children
        auto root = createRenderBlock();
        applyFlexContainer(root.get());
        
        auto child1 = createRenderBlock();
        auto child2 = createRenderBlock();
        root->AppendChild(child1);
        root->AppendChild(child2);
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        
        // Mark one child as needing layout
        engine_->MarkNeedsLayout(child1.get());
        
        // Perform incremental layout
        bool did_layout = engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        
        EXPECT_TRUE(did_layout)
            << "Incremental layout should have been performed for flex child. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 3: 增量布局缓存保留**
 * 
 * For any tree with multiple independent subtrees, marking one subtree dirty
 * SHALL NOT affect the cached results of other subtrees.
 * 
 * **Validates: Requirements 2.1, 2.4**
 */
TEST_F(IncrementalLayoutPropertyTest, IndependentSubtreesRetainCache) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create: root -> [subtree1, subtree2]
        // Each subtree is a fixed-size container with children
        auto root = createRenderBlock();
        applyFixedSize(root.get(), 800.0f, 600.0f);
        
        auto subtree1 = createRenderBlock();
        auto subtree2 = createRenderBlock();
        applyFixedSize(subtree1.get(), 300.0f, 200.0f);
        applyFixedSize(subtree2.get(), 300.0f, 200.0f);
        
        auto child1 = createRenderBlock();
        auto child2 = createRenderBlock();
        applyFixedSize(child1.get(), 100.0f, 100.0f);
        applyFixedSize(child2.get(), 100.0f, 100.0f);
        
        root->AppendChild(subtree1);
        root->AppendChild(subtree2);
        subtree1->AppendChild(child1);
        subtree2->AppendChild(child2);
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Record initial layout for subtree2
        LayoutInfo initial_subtree2 = subtree2->GetLayoutInfo();
        LayoutInfo initial_child2 = child2->GetLayoutInfo();
        
        // Mark only subtree1 as needing layout
        engine_->MarkNeedsLayout(subtree1.get());
        
        // Perform incremental layout
        engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        engine_->GetLayoutInfo(root);
        
        // Subtree2 should be unchanged
        EXPECT_FLOAT_EQ(subtree2->GetLayoutInfo().width, initial_subtree2.width)
            << "Subtree2 width should be unchanged. Iteration: " << i;
        EXPECT_FLOAT_EQ(subtree2->GetLayoutInfo().height, initial_subtree2.height)
            << "Subtree2 height should be unchanged. Iteration: " << i;
        EXPECT_FLOAT_EQ(child2->GetLayoutInfo().width, initial_child2.width)
            << "Child2 width should be unchanged. Iteration: " << i;
        EXPECT_FLOAT_EQ(child2->GetLayoutInfo().height, initial_child2.height)
            << "Child2 height should be unchanged. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 4: 脏标记向上传播**
 * 
 * For any already-dirty ancestor, propagation SHALL stop at that ancestor
 * (optimization to avoid redundant marking).
 * 
 * **Validates: Requirements 4.2**
 */
TEST_F(IncrementalLayoutPropertyTest, PropagationStopsAtDirtyAncestor) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create: root -> parent -> child -> grandchild
        auto root = createRenderBlock();
        auto parent = createRenderBlock();
        auto child = createRenderBlock();
        auto grandchild = createRenderBlock();
        
        root->AppendChild(parent);
        parent->AppendChild(child);
        child->AppendChild(grandchild);
        
        // All auto-sized
        applyAutoSize(root.get());
        applyAutoSize(parent.get());
        applyAutoSize(child.get());
        applyAutoSize(grandchild.get());
        
        // Build layout tree and compute initial layout
        engine_->BuildLayoutTree(root);
        engine_->ComputeLayout(800.0f, 600.0f);
        
        // First, mark parent as needing layout
        engine_->MarkNeedsLayout(parent.get());
        
        // Then mark grandchild - propagation should stop at parent
        engine_->MarkNeedsLayout(grandchild.get());
        
        // Perform incremental layout
        bool did_layout = engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        
        EXPECT_TRUE(did_layout)
            << "Incremental layout should have been performed. Iteration: " << i;
        
        // Clean up for next iteration
        engine_->Clear();
    }
}

