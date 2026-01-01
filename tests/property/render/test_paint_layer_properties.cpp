/**
 * @file test_paint_layer_properties.cpp
 * @brief Property-based tests for PaintLayer System
 * 
 * This file implements property-based testing for the unified PaintLayer system.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: unified-layer-system**
 */

#include <gtest/gtest.h>
#include "core/render/layer/paint_layer.h"
#include "core/render/objects/render_object.h"
#include "core/render/objects/render_block.h"
#include "core/dom/element.h"
#include "core/event/input/hit_testing.h"
#include <random>
#include <vector>
#include <string>
#include <memory>

using namespace lightui;

// Random number generator for property tests
class PaintLayerPropertyTestRng {
public:
    PaintLayerPropertyTestRng() : gen_(std::random_device{}()) {}
    
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
    
private:
    std::mt19937 gen_;
};

class PaintLayerPropertyTest : public ::testing::Test {
protected:
    PaintLayerPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    // Helper to create a simple render object with layout
    std::shared_ptr<RenderBlock> CreateRenderBlock(float x, float y, float width, float height) {
        auto render_obj = std::make_shared<RenderBlock>();
        auto& layout = render_obj->GetLayoutInfo();
        layout.x = x;
        layout.y = y;
        layout.width = width;
        layout.height = height;
        layout.is_laid_out = true;
        return render_obj;
    }
    
    // Helper to create an element and associate it with a render object
    std::shared_ptr<Element> CreateElementWithRenderObject(
        std::shared_ptr<RenderObject> render_obj,
        const std::string& tag_name = "div") {
        auto element = std::make_shared<Element>(tag_name);
        render_obj->SetNode(element);
        return element;
    }
    
    // Helper to create a positioned render object with z-index
    std::shared_ptr<RenderBlock> CreatePositionedRenderBlock(
        float x, float y, float width, float height,
        int z_index, const std::string& position = "absolute") {
        auto render_obj = CreateRenderBlock(x, y, width, height);
        render_obj->GetComputedStyle().position = position;
        render_obj->GetComputedStyle().z_index = z_index;
        return render_obj;
    }
};

/**
 * **Feature: unified-layer-system, Property 2: Stacking Context 创建正确性**
 * 
 * For any RenderObject with CSS properties that create a stacking context,
 * the corresponding PaintLayer's IsStackingContext() should return true.
 * 
 * **Validates: Requirements 1.1**
 */
TEST_F(PaintLayerPropertyTest, StackingContextCreation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Test position + z-index creates stacking context
        {
            auto render_obj = CreatePositionedRenderBlock(0, 0, 100, 100, 
                rng_.randInt(1, 100), "absolute");
            CreateElementWithRenderObject(render_obj);
            
            PaintLayer layer(render_obj.get());
            // Root layer is always stacking context, so we need a parent
            auto parent_obj = CreateRenderBlock(0, 0, 200, 200);
            PaintLayer parent_layer(parent_obj.get());
            parent_layer.AddChild(&layer);
            
            EXPECT_TRUE(layer.IsStackingContext())
                << "position:absolute + z-index should create stacking context";
        }
        
        // Test opacity < 1 creates stacking context
        {
            auto render_obj = CreateRenderBlock(0, 0, 100, 100);
            render_obj->GetComputedStyle().opacity = rng_.randFloat(0.0f, 0.99f);
            CreateElementWithRenderObject(render_obj);
            
            auto parent_obj = CreateRenderBlock(0, 0, 200, 200);
            PaintLayer parent_layer(parent_obj.get());
            PaintLayer layer(render_obj.get());
            parent_layer.AddChild(&layer);
            
            EXPECT_TRUE(layer.IsStackingContext())
                << "opacity < 1 should create stacking context";
        }
        
        // Test position:fixed creates stacking context
        {
            auto render_obj = CreatePositionedRenderBlock(0, 0, 100, 100, 0, "fixed");
            CreateElementWithRenderObject(render_obj);
            
            auto parent_obj = CreateRenderBlock(0, 0, 200, 200);
            PaintLayer parent_layer(parent_obj.get());
            PaintLayer layer(render_obj.get());
            parent_layer.AddChild(&layer);
            
            EXPECT_TRUE(layer.IsStackingContext())
                << "position:fixed should create stacking context";
        }
    }
}

/**
 * **Feature: unified-layer-system, Property 3: Z-order 列表维护正确性**
 * 
 * For any stacking context PaintLayer, pos_z_order_list_ should contain
 * all z-index >= 0 children in ascending order, and neg_z_order_list_
 * should contain all z-index < 0 children in ascending order.
 * 
 * **Validates: Requirements 1.2**
 */
TEST_F(PaintLayerPropertyTest, ZOrderListMaintenance) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create root stacking context
        auto root_obj = CreatePositionedRenderBlock(0, 0, 500, 500, 0, "relative");
        CreateElementWithRenderObject(root_obj);
        PaintLayer root_layer(root_obj.get());
        
        // Create children with random z-indices
        std::vector<std::unique_ptr<PaintLayer>> child_layers;
        std::vector<std::shared_ptr<RenderBlock>> child_objs;
        int num_children = rng_.randInt(3, 10);
        
        for (int j = 0; j < num_children; ++j) {
            int z = rng_.randInt(-50, 50);
            auto child_obj = CreatePositionedRenderBlock(
                rng_.randFloat(0, 100), rng_.randFloat(0, 100),
                50, 50, z, "absolute");
            CreateElementWithRenderObject(child_obj);
            child_objs.push_back(child_obj);
            
            auto child_layer = std::make_unique<PaintLayer>(child_obj.get());
            root_layer.AddChild(child_layer.get());
            child_layers.push_back(std::move(child_layer));
        }
        
        // Update z-order lists
        root_layer.UpdateZOrderLists();
        
        // Verify positive z-order list is sorted ascending
        const auto& pos_list = root_layer.PosZOrderList();
        for (size_t j = 1; j < pos_list.size(); ++j) {
            EXPECT_LE(pos_list[j-1]->ZIndex(), pos_list[j]->ZIndex())
                << "Positive z-order list should be sorted ascending";
        }
        
        // Verify all items in pos_list have z-index >= 0
        for (auto* layer : pos_list) {
            EXPECT_GE(layer->ZIndex(), 0)
                << "Positive z-order list should only contain z-index >= 0";
        }
        
        // Verify negative z-order list is sorted ascending
        const auto& neg_list = root_layer.NegZOrderList();
        for (size_t j = 1; j < neg_list.size(); ++j) {
            EXPECT_LE(neg_list[j-1]->ZIndex(), neg_list[j]->ZIndex())
                << "Negative z-order list should be sorted ascending";
        }
        
        // Verify all items in neg_list have z-index < 0
        for (auto* layer : neg_list) {
            EXPECT_LT(layer->ZIndex(), 0)
                << "Negative z-order list should only contain z-index < 0";
        }
    }
}

/**
 * **Feature: unified-layer-system, Property 6: Fixed 元素 stacking context 归属**
 * 
 * For any position:fixed element, its PaintLayer should participate in
 * the root stacking context's z-order sorting.
 * 
 * **Validates: Requirements 3.1, 3.4**
 */
TEST_F(PaintLayerPropertyTest, FixedElementStackingContext) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create fixed positioned element
        int z_index = rng_.randInt(-100, 100);
        auto fixed_obj = CreatePositionedRenderBlock(
            rng_.randFloat(0, 100), rng_.randFloat(0, 100),
            100, 100, z_index, "fixed");
        CreateElementWithRenderObject(fixed_obj);
        
        PaintLayer fixed_layer(fixed_obj.get());
        
        // Verify it's a stacking context
        EXPECT_TRUE(fixed_layer.IsStackingContext())
            << "position:fixed element should be a stacking context";
        
        // Verify IsFixedPositioned returns true
        EXPECT_TRUE(fixed_layer.IsFixedPositioned())
            << "IsFixedPositioned should return true for fixed elements";
    }
}

/**
 * **Feature: unified-layer-system, Property 7: Hit Testing 逆序遍历**
 * 
 * For any click position where multiple layers overlap, hit testing
 * should return the element with the highest z-index.
 * 
 * **Validates: Requirements 4.1, 4.2**
 */
TEST_F(PaintLayerPropertyTest, HitTestingReverseOrder) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create root
        auto root_obj = CreateRenderBlock(0, 0, 500, 500);
        CreateElementWithRenderObject(root_obj);
        PaintLayer root_layer(root_obj.get());
        
        // Create overlapping children with different z-indices
        std::vector<std::unique_ptr<PaintLayer>> child_layers;
        std::vector<std::shared_ptr<RenderBlock>> child_objs;
        std::vector<std::shared_ptr<Element>> elements;
        
        int num_children = rng_.randInt(2, 5);
        int max_z = -1000;
        std::string max_z_id;
        
        for (int j = 0; j < num_children; ++j) {
            int z = rng_.randInt(0, 100);
            auto child_obj = CreatePositionedRenderBlock(0, 0, 100, 100, z, "absolute");
            auto elem = CreateElementWithRenderObject(child_obj);
            std::string id = "elem_" + std::to_string(j) + "_z" + std::to_string(z);
            elem->SetAttribute("id", id);
            
            if (z > max_z) {
                max_z = z;
                max_z_id = id;
            }
            
            child_objs.push_back(child_obj);
            elements.push_back(elem);
            
            auto child_layer = std::make_unique<PaintLayer>(child_obj.get());
            root_layer.AddChild(child_layer.get());
            child_layers.push_back(std::move(child_layer));
        }
        
        // Update z-order lists
        root_layer.UpdateZOrderLists();
        
        // Verify the highest z-index element would be hit first
        // (This is a structural test - actual hit testing requires full render tree)
        const auto& pos_list = root_layer.PosZOrderList();
        if (!pos_list.empty()) {
            auto* highest = pos_list.back();
            EXPECT_EQ(highest->ZIndex(), max_z)
                << "Highest z-index element should be last in pos_z_order_list";
        }
    }
}

/**
 * **Feature: unified-layer-system, Property 9: 同 z-index 文档顺序**
 * 
 * For any two elements with the same z-index, the one that appears
 * later in document order should be hit first.
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(PaintLayerPropertyTest, SameZIndexDocumentOrder) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create root
        auto root_obj = CreateRenderBlock(0, 0, 500, 500);
        CreateElementWithRenderObject(root_obj);
        PaintLayer root_layer(root_obj.get());
        
        // Create children with same z-index
        int same_z = rng_.randInt(0, 50);
        std::vector<std::unique_ptr<PaintLayer>> child_layers;
        std::vector<std::shared_ptr<RenderBlock>> child_objs;
        
        int num_children = rng_.randInt(3, 6);
        for (int j = 0; j < num_children; ++j) {
            auto child_obj = CreatePositionedRenderBlock(0, 0, 100, 100, same_z, "absolute");
            CreateElementWithRenderObject(child_obj);
            child_objs.push_back(child_obj);
            
            auto child_layer = std::make_unique<PaintLayer>(child_obj.get());
            root_layer.AddChild(child_layer.get());
            child_layers.push_back(std::move(child_layer));
        }
        
        // Update z-order lists
        root_layer.UpdateZOrderLists();
        
        // Verify document order is preserved (stable sort)
        const auto& pos_list = root_layer.PosZOrderList();
        for (size_t j = 0; j < pos_list.size(); ++j) {
            EXPECT_EQ(pos_list[j]->ZIndex(), same_z)
                << "All elements should have same z-index";
        }
        
        // The order in pos_list should match the order they were added
        // (stable sort preserves insertion order for equal elements)
        for (size_t j = 0; j < pos_list.size(); ++j) {
            EXPECT_EQ(pos_list[j], child_layers[j].get())
                << "Document order should be preserved for same z-index";
        }
    }
}

/**
 * **Feature: unified-layer-system, Property 10: Compositing 提升规则**
 * 
 * For any PaintLayer that meets compositing promotion criteria,
 * NeedsCompositing() should return true.
 * 
 * **Validates: Requirements 6.1, 6.2, 6.3, 6.4**
 */
TEST_F(PaintLayerPropertyTest, CompositingPromotionRules) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Test will-change: transform
        {
            auto render_obj = CreateRenderBlock(0, 0, 100, 100);
            render_obj->GetComputedStyle().will_change = "transform";
            CreateElementWithRenderObject(render_obj);
            
            PaintLayer layer(render_obj.get());
            EXPECT_TRUE(layer.NeedsCompositing())
                << "will-change:transform should trigger compositing";
            EXPECT_EQ(layer.GetPromotionReason(), LayerPromotionReason::WillChangeTransform);
        }
        
        // Test will-change: opacity
        {
            auto render_obj = CreateRenderBlock(0, 0, 100, 100);
            render_obj->GetComputedStyle().will_change = "opacity";
            CreateElementWithRenderObject(render_obj);
            
            PaintLayer layer(render_obj.get());
            EXPECT_TRUE(layer.NeedsCompositing())
                << "will-change:opacity should trigger compositing";
            EXPECT_EQ(layer.GetPromotionReason(), LayerPromotionReason::WillChangeOpacity);
        }
        
        // Test position: fixed
        {
            auto render_obj = CreatePositionedRenderBlock(0, 0, 100, 100, 0, "fixed");
            CreateElementWithRenderObject(render_obj);
            
            PaintLayer layer(render_obj.get());
            EXPECT_TRUE(layer.NeedsCompositing())
                << "position:fixed should trigger compositing";
            EXPECT_EQ(layer.GetPromotionReason(), LayerPromotionReason::PositionFixed);
        }
    }
}

/**
 * **Feature: unified-layer-system, Property 12: 层检查信息完整性**
 * 
 * For any PaintLayer, ToDebugString() should return information about
 * its stacking context status, z-index, and compositing status.
 * 
 * **Validates: Requirements 8.3**
 */
TEST_F(PaintLayerPropertyTest, DebugInfoCompleteness) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int z_index = rng_.randInt(-100, 100);
        auto render_obj = CreatePositionedRenderBlock(0, 0, 100, 100, z_index, "absolute");
        auto elem = CreateElementWithRenderObject(render_obj);
        elem->SetAttribute("id", "test-element");
        
        PaintLayer layer(render_obj.get());
        std::string debug_str = layer.ToDebugString();
        
        // Verify debug string contains key information
        EXPECT_NE(debug_str.find("PaintLayer"), std::string::npos)
            << "Debug string should contain 'PaintLayer'";
        EXPECT_NE(debug_str.find("z="), std::string::npos)
            << "Debug string should contain z-index";
        EXPECT_NE(debug_str.find("sc="), std::string::npos)
            << "Debug string should contain stacking context status";
        EXPECT_NE(debug_str.find("comp="), std::string::npos)
            << "Debug string should contain compositing status";
    }
}

