/**
 * @file test_user_select_properties.cpp
 * @brief Property-based tests for CSS user-select property inheritance
 * 
 * This file implements property-based testing for user-select inheritance.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-basic-interaction-properties, Property 9: User Select Inheritance**
 * **Validates: Requirements 4.5**
 */

#include <gtest/gtest.h>
#include "core/render/objects/render_object.h"
#include "core/render/css/style_resolver.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <random>
#include <vector>
#include <string>
#include <memory>

using namespace mbink;

// Random number generator for property tests
class UserSelectPropertyTestRng {
public:
    UserSelectPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    std::string randomUserSelectValue() {
        static const std::vector<std::string> values = {"auto", "none", "text", "all"};
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randomTagName() {
        static const std::vector<std::string> tags = {"div", "span", "p", "section", "article"};
        return tags[randInt(0, static_cast<int>(tags.size()) - 1)];
    }
    
private:
    std::mt19937 gen_;
};

class UserSelectPropertyTest : public ::testing::Test {
protected:
    UserSelectPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    StyleResolver style_resolver_;
};

/**
 * **Feature: css-basic-interaction-properties, Property 9: User Select Inheritance**
 * 
 * For any parent element with user-select value, child elements without
 * explicit user-select should inherit the parent's value.
 * 
 * **Validates: Requirements 4.5**
 */
TEST_F(UserSelectPropertyTest, UserSelectInheritance) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random user-select value for parent
        std::string parent_user_select = rng_.randomUserSelectValue();
        
        // Create parent element with user-select set
        auto parent_element = std::make_shared<Element>(rng_.randomTagName());
        parent_element->SetAttribute("style", "user-select: " + parent_user_select);
        
        // Resolve parent style
        ComputedStyle parent_style = style_resolver_.ResolveStyle(parent_element, nullptr);
        
        // Verify parent has the correct user-select value
        ASSERT_EQ(parent_style.user_select, parent_user_select)
            << "Parent should have user-select: " << parent_user_select;
        
        // Create child element without explicit user-select
        auto child_element = std::make_shared<Element>(rng_.randomTagName());
        // No user-select style set on child
        
        // Resolve child style with parent style for inheritance
        ComputedStyle child_style = style_resolver_.ResolveStyle(child_element, &parent_style);
        
        // Child should inherit user-select from parent
        EXPECT_EQ(child_style.user_select, parent_user_select)
            << "Child should inherit user-select: " << parent_user_select << " from parent, "
            << "but got: " << child_style.user_select;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 9: User Select Inheritance**
 * 
 * Additional test: Child can override parent's user-select value.
 * 
 * **Validates: Requirements 4.5**
 */
TEST_F(UserSelectPropertyTest, ChildCanOverrideUserSelect) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random user-select values
        std::string parent_user_select = rng_.randomUserSelectValue();
        std::string child_user_select = rng_.randomUserSelectValue();
        
        // Create parent element with user-select set
        auto parent_element = std::make_shared<Element>(rng_.randomTagName());
        parent_element->SetAttribute("style", "user-select: " + parent_user_select);
        
        // Resolve parent style
        ComputedStyle parent_style = style_resolver_.ResolveStyle(parent_element, nullptr);
        
        // Create child element with explicit user-select override
        auto child_element = std::make_shared<Element>(rng_.randomTagName());
        child_element->SetAttribute("style", "user-select: " + child_user_select);
        
        // Resolve child style with parent style for inheritance
        ComputedStyle child_style = style_resolver_.ResolveStyle(child_element, &parent_style);
        
        // Child should have its own user-select value, not inherited
        EXPECT_EQ(child_style.user_select, child_user_select)
            << "Child should have its own user-select: " << child_user_select
            << ", not inherited value: " << parent_user_select;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 9: User Select Inheritance**
 * 
 * Additional test: Deep inheritance chain - grandchild inherits from grandparent
 * through parent.
 * 
 * **Validates: Requirements 4.5**
 */
TEST_F(UserSelectPropertyTest, DeepInheritanceChain) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random user-select value for grandparent
        std::string grandparent_user_select = rng_.randomUserSelectValue();
        
        // Create grandparent element with user-select set
        auto grandparent_element = std::make_shared<Element>(rng_.randomTagName());
        grandparent_element->SetAttribute("style", "user-select: " + grandparent_user_select);
        
        // Resolve grandparent style
        ComputedStyle grandparent_style = style_resolver_.ResolveStyle(grandparent_element, nullptr);
        
        // Create parent element without explicit user-select (inherits from grandparent)
        auto parent_element = std::make_shared<Element>(rng_.randomTagName());
        
        // Resolve parent style with grandparent style for inheritance
        ComputedStyle parent_style = style_resolver_.ResolveStyle(parent_element, &grandparent_style);
        
        // Parent should inherit from grandparent
        ASSERT_EQ(parent_style.user_select, grandparent_user_select)
            << "Parent should inherit user-select from grandparent";
        
        // Create grandchild element without explicit user-select
        auto grandchild_element = std::make_shared<Element>(rng_.randomTagName());
        
        // Resolve grandchild style with parent style for inheritance
        ComputedStyle grandchild_style = style_resolver_.ResolveStyle(grandchild_element, &parent_style);
        
        // Grandchild should inherit user-select through the chain
        EXPECT_EQ(grandchild_style.user_select, grandparent_user_select)
            << "Grandchild should inherit user-select: " << grandparent_user_select
            << " through inheritance chain, but got: " << grandchild_style.user_select;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 9: User Select Inheritance**
 * 
 * Additional test: Default value is "auto" when no parent style exists.
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(UserSelectPropertyTest, DefaultValueIsAuto) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Create element without any user-select style
        auto element = std::make_shared<Element>(rng_.randomTagName());
        
        // Resolve style without parent (root element)
        ComputedStyle style = style_resolver_.ResolveStyle(element, nullptr);
        
        // Default user-select should be "auto"
        EXPECT_EQ(style.user_select, "auto")
            << "Default user-select should be 'auto', but got: " << style.user_select;
    }
}

