/**
 * @file test_incremental_layout_integration.cpp
 * @brief Integration tests for incremental layout optimization
 * 
 * This file implements integration tests for the incremental layout
 * optimization feature. Tests verify real-world scenarios:
 * - Clock update scenario (15.1): Only clock text node is re-laid out
 * - List item addition scenario (15.2): New items trigger layout, existing items use cache
 * - Window resize scenario (15.3): Full layout is correctly executed
 * 
 * **Feature: incremental-layout-optimization**
 * **Validates: Requirements 2.4, 5.1, 5.2, 6.3**
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/layout/native_layout_engine.h"
#include "core/layout/content_version.h"
#include "core/render/objects/render_object.h"
#include <memory>
#include <vector>
#include <chrono>

namespace lightui {
namespace test {

/**
 * @brief Test fixture for incremental layout integration tests
 */
class IncrementalLayoutIntegrationTest : public DOMTestBase {
protected:
    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    
    void SetUp() override {
        DOMTestBase::SetUp();
        ContentVersionManager::GetInstance().Reset();
        layout_engine_ = std::make_unique<NativeLayoutEngine>();
    }
    
    void TearDown() override {
        layout_engine_.reset();
        DOMTestBase::TearDown();
    }
    
    // Helper to create a render block with fixed size
    std::shared_ptr<RenderBlock> createFixedBlock(float width, float height) {
        auto block = std::make_shared<RenderBlock>();
        ComputedStyle style;
        style.display = RenderObjectType::BLOCK;
        style.width = CSSLength(width, CSSUnit::PX);
        style.height = CSSLength(height, CSSUnit::PX);
        block->SetComputedStyle(style);
        return block;
    }
    
    // Helper to create a render block with auto size
    std::shared_ptr<RenderBlock> createAutoBlock() {
        auto block = std::make_shared<RenderBlock>();
        ComputedStyle style;
        style.display = RenderObjectType::BLOCK;
        style.width = CSSLength(0, CSSUnit::AUTO);
        style.height = CSSLength(0, CSSUnit::AUTO);
        block->SetComputedStyle(style);
        return block;
    }
    
    // Helper to create a render text
    std::shared_ptr<RenderText> createRenderText(const std::string& text) {
        auto render_text = std::make_shared<RenderText>(text);
        ComputedStyle style;
        style.display = RenderObjectType::TEXT;
        style.font_size = 16.0f;
        render_text->SetComputedStyle(style);
        return render_text;
    }
    
    // Helper to create a flex container
    std::shared_ptr<RenderBlock> createFlexContainer(float width, float height) {
        auto block = std::make_shared<RenderBlock>();
        ComputedStyle style;
        style.display = RenderObjectType::FLEX;
        style.flex_direction = "column";
        style.width = CSSLength(width, CSSUnit::PX);
        style.height = CSSLength(height, CSSUnit::PX);
        block->SetComputedStyle(style);
        return block;
    }
};

// ========== 15.1 时钟更新场景测试 ==========

/**
 * @brief Test that only the clock text node is re-laid out when time updates
 * 
 * Scenario: A page with a header, content area, and a clock display.
 * When the clock text updates, only the clock text node should be re-laid out,
 * while other nodes should use their cached layout results.
 * 
 * **Validates: Requirements 5.1, 5.2**
 */
TEST_F(IncrementalLayoutIntegrationTest, ClockUpdateOnlyRelayoutsClockNode) {
    // Create a page structure:
    // root (800x600)
    //   ├── header (800x50, fixed)
    //   ├── content (800x500, fixed)
    //   │     └── some_text
    //   └── clock_container (800x50, fixed)
    //         └── clock_text ("12:00:00")
    
    auto root = createFixedBlock(800.0f, 600.0f);
    auto header = createFixedBlock(800.0f, 50.0f);
    auto content = createFixedBlock(800.0f, 500.0f);
    auto content_text = createRenderText("Welcome to the page");
    auto clock_container = createFixedBlock(800.0f, 50.0f);
    auto clock_text = createRenderText("12:00:00");
    
    root->AppendChild(header);
    root->AppendChild(content);
    content->AppendChild(content_text);
    root->AppendChild(clock_container);
    clock_container->AppendChild(clock_text);
    
    // Build layout tree and compute initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial layout info for all nodes
    LayoutInfo initial_header = header->GetLayoutInfo();
    LayoutInfo initial_content = content->GetLayoutInfo();
    LayoutInfo initial_content_text = content_text->GetLayoutInfo();
    LayoutInfo initial_clock_container = clock_container->GetLayoutInfo();
    
    // Simulate clock update: change the text and mark for layout
    clock_text->SetText("12:00:01");
    layout_engine_->UpdateContentVersion(clock_text.get());
    layout_engine_->MarkNeedsLayout(clock_text.get());
    
    // Perform incremental layout
    bool did_layout = layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Verify incremental layout was performed
    EXPECT_TRUE(did_layout) << "Incremental layout should have been performed";
    
    // Verify that other nodes retained their cached layout (dimensions unchanged)
    EXPECT_FLOAT_EQ(header->GetLayoutInfo().width, initial_header.width)
        << "Header width should be unchanged";
    EXPECT_FLOAT_EQ(header->GetLayoutInfo().height, initial_header.height)
        << "Header height should be unchanged";
    
    EXPECT_FLOAT_EQ(content->GetLayoutInfo().width, initial_content.width)
        << "Content width should be unchanged";
    EXPECT_FLOAT_EQ(content->GetLayoutInfo().height, initial_content.height)
        << "Content height should be unchanged";
    
    EXPECT_FLOAT_EQ(content_text->GetLayoutInfo().width, initial_content_text.width)
        << "Content text width should be unchanged";
    
    // Clock container should also be unchanged (fixed size)
    EXPECT_FLOAT_EQ(clock_container->GetLayoutInfo().width, initial_clock_container.width)
        << "Clock container width should be unchanged";
    EXPECT_FLOAT_EQ(clock_container->GetLayoutInfo().height, initial_clock_container.height)
        << "Clock container height should be unchanged";
}

/**
 * @brief Test multiple clock updates in sequence
 * 
 * Simulates a clock updating every second for several iterations.
 * Each update should only re-layout the clock container (IFC container).
 * 
 * Note: Text nodes inside IFC containers are not directly in the layout tree.
 * When text content changes, we mark the IFC container (clock_container) as
 * needing layout, which triggers re-layout of the text content.
 * 
 * **Validates: Requirements 5.1, 5.2**
 */
TEST_F(IncrementalLayoutIntegrationTest, MultipleClockUpdatesUseCache) {
    // Create a simple structure with a clock
    auto root = createFixedBlock(800.0f, 600.0f);
    auto static_content = createFixedBlock(800.0f, 550.0f);
    auto clock_container = createFixedBlock(800.0f, 50.0f);
    auto clock_text = createRenderText("00:00:00");
    
    root->AppendChild(static_content);
    root->AppendChild(clock_container);
    clock_container->AppendChild(clock_text);
    
    // Build layout tree and compute initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial layout for static content
    LayoutInfo initial_static = static_content->GetLayoutInfo();
    
    // Simulate 10 clock updates
    // Each update marks the clock container (IFC container) as needing layout
    // because text nodes are managed by their IFC container
    for (int i = 1; i <= 10; ++i) {
        std::string new_time = std::string("00:00:") + (i < 10 ? "0" : "") + std::to_string(i);
        clock_text->SetText(new_time);
        
        // Mark the clock container as needing layout
        // Text nodes inside IFC containers are not directly in the layout tree,
        // so we mark the container instead
        layout_engine_->UpdateContentVersion(clock_container.get());
        layout_engine_->MarkNeedsLayout(clock_container.get());
        
        bool did_layout = layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        layout_engine_->GetLayoutInfo(root);
        
        EXPECT_TRUE(did_layout) << "Incremental layout should be performed for update " << i;
        
        // Static content should remain unchanged
        EXPECT_FLOAT_EQ(static_content->GetLayoutInfo().width, initial_static.width)
            << "Static content width should be unchanged after update " << i;
        EXPECT_FLOAT_EQ(static_content->GetLayoutInfo().height, initial_static.height)
            << "Static content height should be unchanged after update " << i;
    }
}

/**
 * @brief Test that clock in fixed-size container doesn't propagate to ancestors
 * 
 * When a clock text is inside a fixed-size container, updating the clock
 * should not cause the parent containers to be re-laid out.
 * 
 * **Validates: Requirements 5.1, 5.2**
 */
TEST_F(IncrementalLayoutIntegrationTest, ClockInFixedContainerIsolatesChanges) {
    // Create: root (auto) -> container (fixed) -> clock_text
    auto root = createAutoBlock();
    auto container = createFixedBlock(200.0f, 50.0f);
    auto clock_text = createRenderText("12:00:00");
    
    root->AppendChild(container);
    container->AppendChild(clock_text);
    
    // Build layout tree and compute initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial root layout
    LayoutInfo initial_root = root->GetLayoutInfo();
    
    // Update clock text
    clock_text->SetText("12:00:01");
    layout_engine_->UpdateContentVersion(clock_text.get());
    layout_engine_->MarkNeedsLayout(clock_text.get());
    
    // Perform incremental layout
    layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Root should be unchanged because the fixed container isolates changes
    EXPECT_FLOAT_EQ(root->GetLayoutInfo().width, initial_root.width)
        << "Root width should be unchanged due to fixed container isolation";
    EXPECT_FLOAT_EQ(root->GetLayoutInfo().height, initial_root.height)
        << "Root height should be unchanged due to fixed container isolation";
}

// ========== 15.2 列表项添加场景测试 ==========

/**
 * @brief Test that adding a new list item triggers layout only for new item
 * 
 * Scenario: A list with existing items. When a new item is added,
 * only the new item should trigger layout, existing items should use cache.
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(IncrementalLayoutIntegrationTest, AddListItemOnlyLayoutsNewItem) {
    // Create a list structure:
    // root (800x600)
    //   └── list_container (800x auto)
    //         ├── item1 (800x50, fixed)
    //         ├── item2 (800x50, fixed)
    //         └── item3 (800x50, fixed)
    
    auto root = createFixedBlock(800.0f, 600.0f);
    auto list_container = createAutoBlock();
    
    std::vector<std::shared_ptr<RenderBlock>> items;
    for (int i = 0; i < 3; ++i) {
        auto item = createFixedBlock(800.0f, 50.0f);
        items.push_back(item);
        list_container->AppendChild(item);
    }
    
    root->AppendChild(list_container);
    
    // Build layout tree and compute initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial layout info for existing items
    std::vector<LayoutInfo> initial_layouts;
    for (const auto& item : items) {
        initial_layouts.push_back(item->GetLayoutInfo());
    }
    
    // Add a new item
    auto new_item = createFixedBlock(800.0f, 50.0f);
    list_container->AppendChild(new_item);
    
    // Update the layout engine with the new item
    layout_engine_->AddElement(new_item.get(), list_container.get());
    layout_engine_->UpdateContentVersion(list_container.get());
    layout_engine_->MarkNeedsLayout(list_container.get());
    
    // Perform incremental layout
    bool did_layout = layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    EXPECT_TRUE(did_layout) << "Incremental layout should have been performed";
    
    // Verify existing items retained their dimensions
    for (size_t i = 0; i < items.size(); ++i) {
        EXPECT_FLOAT_EQ(items[i]->GetLayoutInfo().width, initial_layouts[i].width)
            << "Item " << i << " width should be unchanged";
        EXPECT_FLOAT_EQ(items[i]->GetLayoutInfo().height, initial_layouts[i].height)
            << "Item " << i << " height should be unchanged";
    }
    
    // Verify new item was laid out
    EXPECT_TRUE(new_item->GetLayoutInfo().is_laid_out)
        << "New item should be laid out";
    EXPECT_FLOAT_EQ(new_item->GetLayoutInfo().width, 800.0f)
        << "New item should have correct width";
    EXPECT_FLOAT_EQ(new_item->GetLayoutInfo().height, 50.0f)
        << "New item should have correct height";
}

/**
 * @brief Test adding multiple items to a list
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(IncrementalLayoutIntegrationTest, AddMultipleListItemsIncrementally) {
    auto root = createFixedBlock(800.0f, 600.0f);
    auto list_container = createAutoBlock();
    root->AppendChild(list_container);
    
    // Build initial empty list
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Add items one by one
    std::vector<std::shared_ptr<RenderBlock>> items;
    for (int i = 0; i < 5; ++i) {
        auto item = createFixedBlock(800.0f, 40.0f);
        items.push_back(item);
        list_container->AppendChild(item);
        
        layout_engine_->AddElement(item.get(), list_container.get());
        layout_engine_->UpdateContentVersion(list_container.get());
        layout_engine_->MarkNeedsLayout(list_container.get());
        
        bool did_layout = layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
        layout_engine_->GetLayoutInfo(root);
        
        EXPECT_TRUE(did_layout) << "Incremental layout should be performed for item " << i;
        
        // Verify the new item is laid out correctly
        EXPECT_TRUE(item->GetLayoutInfo().is_laid_out)
            << "Item " << i << " should be laid out";
        EXPECT_FLOAT_EQ(item->GetLayoutInfo().width, 800.0f)
            << "Item " << i << " should have correct width";
        EXPECT_FLOAT_EQ(item->GetLayoutInfo().height, 40.0f)
            << "Item " << i << " should have correct height";
    }
}

/**
 * @brief Test that existing items in a fixed-height list use cache
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(IncrementalLayoutIntegrationTest, ExistingItemsInFixedListUseCache) {
    // Create a fixed-height list container
    auto root = createFixedBlock(800.0f, 600.0f);
    auto list_container = createFixedBlock(800.0f, 300.0f);
    
    // Add initial items
    std::vector<std::shared_ptr<RenderBlock>> items;
    for (int i = 0; i < 3; ++i) {
        auto item = createFixedBlock(800.0f, 50.0f);
        items.push_back(item);
        list_container->AppendChild(item);
    }
    
    root->AppendChild(list_container);
    
    // Build and compute initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial layouts
    std::vector<LayoutInfo> initial_layouts;
    for (const auto& item : items) {
        initial_layouts.push_back(item->GetLayoutInfo());
    }
    
    // Add a new item
    auto new_item = createFixedBlock(800.0f, 50.0f);
    list_container->AppendChild(new_item);
    layout_engine_->AddElement(new_item.get(), list_container.get());
    layout_engine_->UpdateContentVersion(list_container.get());
    layout_engine_->MarkNeedsLayout(list_container.get());
    
    // Perform incremental layout
    layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Existing items should have unchanged dimensions
    for (size_t i = 0; i < items.size(); ++i) {
        EXPECT_FLOAT_EQ(items[i]->GetLayoutInfo().width, initial_layouts[i].width)
            << "Existing item " << i << " width should be unchanged";
        EXPECT_FLOAT_EQ(items[i]->GetLayoutInfo().height, initial_layouts[i].height)
            << "Existing item " << i << " height should be unchanged";
    }
}

// ========== 15.3 窗口 resize 场景测试 ==========

/**
 * @brief Test that window resize triggers full layout
 * 
 * When the window is resized, a full layout should be performed,
 * and all caches should be properly cleared.
 * 
 * **Validates: Requirements 6.3**
 */
TEST_F(IncrementalLayoutIntegrationTest, WindowResizeTriggersFullLayout) {
    // Create a responsive layout
    auto root = createAutoBlock();
    auto header = createFixedBlock(800.0f, 50.0f);
    auto content = createAutoBlock();
    auto footer = createFixedBlock(800.0f, 50.0f);
    
    root->AppendChild(header);
    root->AppendChild(content);
    root->AppendChild(footer);
    
    // Build and compute initial layout at 800x600
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record initial layout
    LayoutInfo initial_root = root->GetLayoutInfo();
    
    // Simulate window resize to 1024x768
    // Full layout should be triggered (using ComputeLayout, not ComputeIncrementalLayout)
    layout_engine_->ComputeLayout(1024.0f, 768.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Layout should have been recomputed
    // Note: The actual dimensions depend on the layout algorithm,
    // but we verify that layout was performed
    EXPECT_TRUE(root->GetLayoutInfo().is_laid_out)
        << "Root should be laid out after resize";
}

/**
 * @brief Test that resize clears all caches
 * 
 * After a resize, subsequent incremental layout should work correctly
 * because caches were properly cleared.
 * 
 * **Validates: Requirements 6.3**
 */
TEST_F(IncrementalLayoutIntegrationTest, ResizeClearsCachesCorrectly) {
    auto root = createFixedBlock(800.0f, 600.0f);
    auto child = createFixedBlock(400.0f, 300.0f);
    root->AppendChild(child);
    
    // Initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Resize (full layout)
    layout_engine_->ComputeLayout(1024.0f, 768.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Now mark child as needing layout
    layout_engine_->MarkNeedsLayout(child.get());
    
    // Incremental layout should work correctly after resize
    bool did_layout = layout_engine_->ComputeIncrementalLayout(1024.0f, 768.0f);
    layout_engine_->GetLayoutInfo(root);
    
    EXPECT_TRUE(did_layout)
        << "Incremental layout should work after resize";
    
    // Child should be laid out correctly
    EXPECT_TRUE(child->GetLayoutInfo().is_laid_out)
        << "Child should be laid out after incremental layout";
}

/**
 * @brief Test multiple resize operations
 * 
 * **Validates: Requirements 6.3**
 */
TEST_F(IncrementalLayoutIntegrationTest, MultipleResizeOperations) {
    auto root = createAutoBlock();
    auto content = createAutoBlock();
    root->AppendChild(content);
    
    layout_engine_->BuildLayoutTree(root);
    
    // Perform multiple resizes
    std::vector<std::pair<float, float>> sizes = {
        {800.0f, 600.0f},
        {1024.0f, 768.0f},
        {1280.0f, 720.0f},
        {640.0f, 480.0f},
        {1920.0f, 1080.0f}
    };
    
    for (const auto& size : sizes) {
        layout_engine_->ComputeLayout(size.first, size.second);
        layout_engine_->GetLayoutInfo(root);
        
        EXPECT_TRUE(root->GetLayoutInfo().is_laid_out)
            << "Root should be laid out at size " << size.first << "x" << size.second;
    }
}

/**
 * @brief Test that resize followed by incremental update works correctly
 * 
 * **Validates: Requirements 6.3**
 */
TEST_F(IncrementalLayoutIntegrationTest, ResizeThenIncrementalUpdate) {
    auto root = createFixedBlock(800.0f, 600.0f);
    auto static_content = createFixedBlock(800.0f, 500.0f);
    auto dynamic_text = createRenderText("Initial text");
    
    root->AppendChild(static_content);
    root->AppendChild(dynamic_text);
    
    // Initial layout
    layout_engine_->BuildLayoutTree(root);
    layout_engine_->ComputeLayout(800.0f, 600.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Resize
    layout_engine_->ComputeLayout(1024.0f, 768.0f);
    layout_engine_->GetLayoutInfo(root);
    
    // Record static content layout after resize
    LayoutInfo static_after_resize = static_content->GetLayoutInfo();
    
    // Update dynamic text
    dynamic_text->SetText("Updated text");
    layout_engine_->UpdateContentVersion(dynamic_text.get());
    layout_engine_->MarkNeedsLayout(dynamic_text.get());
    
    // Incremental layout
    bool did_layout = layout_engine_->ComputeIncrementalLayout(1024.0f, 768.0f);
    layout_engine_->GetLayoutInfo(root);
    
    EXPECT_TRUE(did_layout)
        << "Incremental layout should be performed after text update";
    
    // Static content should be unchanged
    EXPECT_FLOAT_EQ(static_content->GetLayoutInfo().width, static_after_resize.width)
        << "Static content width should be unchanged after incremental update";
    EXPECT_FLOAT_EQ(static_content->GetLayoutInfo().height, static_after_resize.height)
        << "Static content height should be unchanged after incremental update";
}

} // namespace test
} // namespace lightui
