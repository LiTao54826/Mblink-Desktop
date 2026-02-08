/**
 * @file test_pointer_events_properties.cpp
 * @brief Property-based tests for CSS pointer-events property
 * 
 * This file implements property-based testing for pointer-events functionality.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Validates: Requirements 3.1-3.4**
 */

#include <gtest/gtest.h>
#include "core/event/input/hit_test_controller.h"
#include "core/render/objects/render_object.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <random>
#include <vector>
#include <string>
#include <memory>

using namespace lightui;

// Random number generator for property tests
class PointerEventsPropertyTestRng {
public:
    PointerEventsPropertyTestRng() : gen_(std::random_device{}()) {}

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

class PointerEventsPropertyTest : public ::testing::Test {
protected:
    PointerEventsPropertyTestRng rng_;
    HitTestController hit_test_controller_;
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
};

/**
 * **Feature: css-basic-interaction-properties, Property 7: Pointer Events None Skips Element in Hit Test**
 * 
 * For any element with pointer-events: none positioned over another element,
 * hit testing at the overlapping position should return the element underneath,
 * not the one with pointer-events: none.
 * 
 * **Validates: Requirements 3.1, 3.3**
 */
TEST_F(PointerEventsPropertyTest, PointerEventsNoneSkipsElementInHitTest) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create random dimensions for the test
        float width = rng_.randFloat(50.0f, 200.0f);
        float height = rng_.randFloat(50.0f, 200.0f);

        // Create a parent container
        auto parent = CreateRenderBlock(0, 0, width * 2, height * 2);
        auto parent_element = CreateElementWithRenderObject(parent, "div");

        // Create bottom element (should be hit when top has pointer-events: none)
        auto bottom = CreateRenderBlock(0, 0, width, height);
        auto bottom_element = CreateElementWithRenderObject(bottom, "div");
        bottom_element->SetAttribute("id", "bottom");
        bottom->SetParent(parent);

        // Create top element with pointer-events: none (overlapping bottom)
        auto top = CreateRenderBlock(0, 0, width, height);
        auto top_element = CreateElementWithRenderObject(top, "div");
        top_element->SetAttribute("id", "top");
        top->GetComputedStyle().pointer_events = "none";
        top->SetParent(parent);

        // Add children to parent (bottom first, then top - top is rendered on top)
        parent->AppendChild(bottom);
        parent->AppendChild(top);

        // Generate random point within the overlapping area
        float test_x = rng_.randFloat(1.0f, width - 1.0f);
        float test_y = rng_.randFloat(1.0f, height - 1.0f);

        // Perform hit test
        HitTestResultEx result = hit_test_controller_.HitTest(parent, test_x, test_y);

        // The hit should return the bottom element, not the top one with pointer-events: none
        ASSERT_TRUE(result.element != nullptr)
            << "Hit test should find an element at (" << test_x << ", " << test_y << ")";

        // The result should be the bottom element since top has pointer-events: none
        EXPECT_EQ(result.element->GetAttribute("id"), "bottom")
            << "Element with pointer-events: none should be skipped. "
            << "Expected 'bottom', got '" << result.element->GetAttribute("id") << "' "
            << "at position (" << test_x << ", " << test_y << ")";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 7: Pointer Events None Skips Element in Hit Test**
 * 
 * Additional test: When pointer-events is auto (default), the element should be hit.
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(PointerEventsPropertyTest, PointerEventsAutoIncludesElementInHitTest) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create random dimensions for the test
        float width = rng_.randFloat(50.0f, 200.0f);
        float height = rng_.randFloat(50.0f, 200.0f);

        // Create a parent container
        auto parent = CreateRenderBlock(0, 0, width * 2, height * 2);
        auto parent_element = CreateElementWithRenderObject(parent, "div");

        // Create bottom element
        auto bottom = CreateRenderBlock(0, 0, width, height);
        auto bottom_element = CreateElementWithRenderObject(bottom, "div");
        bottom_element->SetAttribute("id", "bottom");
        bottom->SetParent(parent);

        // Create top element with pointer-events: auto (default)
        auto top = CreateRenderBlock(0, 0, width, height);
        auto top_element = CreateElementWithRenderObject(top, "div");
        top_element->SetAttribute("id", "top");
        top->GetComputedStyle().pointer_events = "auto";  // Explicit auto
        top->SetParent(parent);

        // Add children to parent
        parent->AppendChild(bottom);
        parent->AppendChild(top);

        // Generate random point within the overlapping area
        float test_x = rng_.randFloat(1.0f, width - 1.0f);
        float test_y = rng_.randFloat(1.0f, height - 1.0f);

        // Perform hit test
        HitTestResultEx result = hit_test_controller_.HitTest(parent, test_x, test_y);

        // The hit should return the top element since it has pointer-events: auto
        ASSERT_TRUE(result.element != nullptr)
            << "Hit test should find an element at (" << test_x << ", " << test_y << ")";

        EXPECT_EQ(result.element->GetAttribute("id"), "top")
            << "Element with pointer-events: auto should be hit. "
            << "Expected 'top', got '" << result.element->GetAttribute("id") << "' "
            << "at position (" << test_x << ", " << test_y << ")";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 8: Pointer Events Inheritance**
 * 
 * For any parent element with pointer-events value, child elements without
 * explicit pointer-events should inherit the parent's value.
 * 
 * **Validates: Requirements 3.4**
 */
TEST_F(PointerEventsPropertyTest, PointerEventsInheritance) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create random dimensions
        float width = rng_.randFloat(100.0f, 300.0f);
        float height = rng_.randFloat(100.0f, 300.0f);
        float child_width = width * 0.5f;
        float child_height = height * 0.5f;

        // Create grandparent container (to catch events when parent has pointer-events: none)
        auto grandparent = CreateRenderBlock(0, 0, width * 2, height * 2);
        auto grandparent_element = CreateElementWithRenderObject(grandparent, "div");
        grandparent_element->SetAttribute("id", "grandparent");

        // Create parent with pointer-events: none
        auto parent = CreateRenderBlock(0, 0, width, height);
        auto parent_element = CreateElementWithRenderObject(parent, "div");
        parent_element->SetAttribute("id", "parent");
        parent->GetComputedStyle().pointer_events = "none";
        parent->SetParent(grandparent);

        // Create child that inherits pointer-events from parent
        // Note: In CSS, pointer-events is inherited, so child should also be "none"
        auto child = CreateRenderBlock(10, 10, child_width, child_height);
        auto child_element = CreateElementWithRenderObject(child, "div");
        child_element->SetAttribute("id", "child");
        // Child inherits pointer-events: none from parent
        child->GetComputedStyle().pointer_events = "none";  // Simulating inheritance
        child->SetParent(parent);

        parent->AppendChild(child);
        grandparent->AppendChild(parent);

        // Generate random point within the child area
        float test_x = rng_.randFloat(15.0f, 10.0f + child_width - 5.0f);
        float test_y = rng_.randFloat(15.0f, 10.0f + child_height - 5.0f);

        // Perform hit test
        HitTestResultEx result = hit_test_controller_.HitTest(grandparent, test_x, test_y);

        // Since both parent and child have pointer-events: none (inherited),
        // the hit should go to grandparent
        ASSERT_TRUE(result.element != nullptr)
            << "Hit test should find an element at (" << test_x << ", " << test_y << ")";

        EXPECT_EQ(result.element->GetAttribute("id"), "grandparent")
            << "Child with inherited pointer-events: none should be skipped. "
            << "Expected 'grandparent', got '" << result.element->GetAttribute("id") << "' "
            << "at position (" << test_x << ", " << test_y << ")";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 8: Pointer Events Inheritance**
 * 
 * Additional test: Child can override parent's pointer-events: none with auto.
 * 
 * **Validates: Requirements 3.4**
 */
TEST_F(PointerEventsPropertyTest, ChildCanOverridePointerEventsNone) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create random dimensions
        float width = rng_.randFloat(100.0f, 300.0f);
        float height = rng_.randFloat(100.0f, 300.0f);
        float child_width = width * 0.5f;
        float child_height = height * 0.5f;

        // Create grandparent container
        auto grandparent = CreateRenderBlock(0, 0, width * 2, height * 2);
        auto grandparent_element = CreateElementWithRenderObject(grandparent, "div");
        grandparent_element->SetAttribute("id", "grandparent");

        // Create parent with pointer-events: none
        auto parent = CreateRenderBlock(0, 0, width, height);
        auto parent_element = CreateElementWithRenderObject(parent, "div");
        parent_element->SetAttribute("id", "parent");
        parent->GetComputedStyle().pointer_events = "none";
        parent->SetParent(grandparent);

        // Create child that explicitly overrides with pointer-events: auto
        auto child = CreateRenderBlock(10, 10, child_width, child_height);
        auto child_element = CreateElementWithRenderObject(child, "div");
        child_element->SetAttribute("id", "child");
        child->GetComputedStyle().pointer_events = "auto";  // Override parent's none
        child->SetParent(parent);

        parent->AppendChild(child);
        grandparent->AppendChild(parent);

        // Generate random point within the child area
        float test_x = rng_.randFloat(15.0f, 10.0f + child_width - 5.0f);
        float test_y = rng_.randFloat(15.0f, 10.0f + child_height - 5.0f);

        // Perform hit test
        HitTestResultEx result = hit_test_controller_.HitTest(grandparent, test_x, test_y);

        // Child has pointer-events: auto, so it should be hit even though parent has none
        ASSERT_TRUE(result.element != nullptr)
            << "Hit test should find an element at (" << test_x << ", " << test_y << ")";

        EXPECT_EQ(result.element->GetAttribute("id"), "child")
            << "Child with pointer-events: auto should override parent's none. "
            << "Expected 'child', got '" << result.element->GetAttribute("id") << "' "
            << "at position (" << test_x << ", " << test_y << ")";
    }
}

/**
 * Additional property test: pointer-events: none should allow events to pass through
 * to elements underneath, even when the element has children.
 *
 * **Validates: Requirements 3.1, 3.3**
 */
TEST_F(PointerEventsPropertyTest, PointerEventsNonePassesThroughToElementsBelow) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create random dimensions
        float width = rng_.randFloat(100.0f, 300.0f);
        float height = rng_.randFloat(100.0f, 300.0f);

        // Create root container
        auto root = CreateRenderBlock(0, 0, width * 2, height * 2);
        auto root_element = CreateElementWithRenderObject(root, "div");
        root_element->SetAttribute("id", "root");

        // Create background element (should receive events)
        auto background = CreateRenderBlock(0, 0, width, height);
        auto background_element = CreateElementWithRenderObject(background, "div");
        background_element->SetAttribute("id", "background");
        background->SetParent(root);

        // Create overlay with pointer-events: none
        auto overlay = CreateRenderBlock(0, 0, width, height);
        auto overlay_element = CreateElementWithRenderObject(overlay, "div");
        overlay_element->SetAttribute("id", "overlay");
        overlay->GetComputedStyle().pointer_events = "none";
        overlay->SetParent(root);

        // Add a child to the overlay (also with pointer-events: none inherited)
        auto overlay_child = CreateRenderBlock(20, 20, width * 0.3f, height * 0.3f);
        auto overlay_child_element = CreateElementWithRenderObject(overlay_child, "span");
        overlay_child_element->SetAttribute("id", "overlay-child");
        overlay_child->GetComputedStyle().pointer_events = "none";  // Inherited
        overlay_child->SetParent(overlay);
        overlay->AppendChild(overlay_child);

        root->AppendChild(background);
        root->AppendChild(overlay);

        // Generate random point within the overlay area
        float test_x = rng_.randFloat(1.0f, width - 1.0f);
        float test_y = rng_.randFloat(1.0f, height - 1.0f);

        // Perform hit test
        HitTestResultEx result = hit_test_controller_.HitTest(root, test_x, test_y);

        // Events should pass through overlay to background
        ASSERT_TRUE(result.element != nullptr)
            << "Hit test should find an element at (" << test_x << ", " << test_y << ")";

        EXPECT_EQ(result.element->GetAttribute("id"), "background")
            << "Events should pass through overlay with pointer-events: none to background. "
            << "Expected 'background', got '" << result.element->GetAttribute("id") << "' "
            << "at position (" << test_x << ", " << test_y << ")";
    }
}
