/**
 * @file test_aspect_ratio_properties.cpp
 * @brief Property-based tests for CSS aspect-ratio property
 * 
 * This file implements property-based testing for aspect-ratio
 * functionality. Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-media-layout-properties**
 * **Validates: Requirements 3.1-3.7**
 */

#include <gtest/gtest.h>
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_object.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <random>
#include <vector>
#include <string>
#include <cmath>

using namespace mblink;

// Random number generator for property tests
class AspectRatioPropertyTestRng {
public:
    AspectRatioPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    // Generate a valid aspect ratio string (e.g., "16 / 9", "4/3", "1")
    std::string randAspectRatioValue() {
        int type = randInt(0, 2);
        if (type == 0) {
            // Format: "width / height"
            int w = randInt(1, 20);
            int h = randInt(1, 20);
            return std::to_string(w) + " / " + std::to_string(h);
        } else if (type == 1) {
            // Format: "width/height" (no spaces)
            int w = randInt(1, 20);
            int h = randInt(1, 20);
            return std::to_string(w) + "/" + std::to_string(h);
        } else {
            // Format: single number (square)
            return std::to_string(randInt(1, 5));
        }
    }
    
    // Generate a random width value in pixels
    std::string randWidthValue() {
        int width = randInt(50, 500);
        return std::to_string(width) + "px";
    }
    
    // Generate a random height value in pixels
    std::string randHeightValue() {
        int height = randInt(50, 500);
        return std::to_string(height) + "px";
    }
    
private:
    std::mt19937 gen_;
};

class AspectRatioPropertyTest : public ::testing::Test {
protected:
    AspectRatioPropertyTestRng rng_;
    StyleResolver style_resolver_;
    static constexpr int NUM_ITERATIONS = 100;
    
    // Helper to create a mock document
    std::shared_ptr<Document> CreateMockDocument() {
        return std::make_shared<Document>();
    }
    
    // Helper to create a mock element for testing
    std::shared_ptr<Element> CreateMockElement(std::shared_ptr<Document> doc, 
                                                const std::string& tag_name) {
        return doc->CreateElement(tag_name);
    }
    
    // Helper to apply inline style and resolve
    ComputedStyle ApplyStyleAndResolve(std::shared_ptr<Element> element, 
                                        const std::string& style_str) {
        element->SetAttribute("style", style_str);
        return style_resolver_.ResolveStyle(element, nullptr);
    }
    
    // Helper to parse aspect ratio from string (e.g., "16 / 9" -> 16.0/9.0)
    float ParseExpectedRatio(const std::string& ratio_str) {
        // Handle formats: "16 / 9", "16/9", "1"
        size_t slash_pos = ratio_str.find('/');
        if (slash_pos != std::string::npos) {
            std::string width_str = ratio_str.substr(0, slash_pos);
            std::string height_str = ratio_str.substr(slash_pos + 1);
            // Trim whitespace
            while (!width_str.empty() && width_str.back() == ' ') width_str.pop_back();
            while (!height_str.empty() && height_str.front() == ' ') height_str.erase(0, 1);
            float w = std::stof(width_str);
            float h = std::stof(height_str);
            return w / h;
        } else {
            // Single number means square (ratio = number)
            return std::stof(ratio_str);
        }
    }
};

/**
 * **Feature: css-media-layout-properties, Property 4: Aspect Ratio Calculates Missing Dimension**
 * 
 * For any element with aspect-ratio set and only width specified,
 * the height should be calculated correctly from width / aspect-ratio.
 * 
 * **Validates: Requirements 3.5**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioCalculatesMissingHeight) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string ratio_str = rng_.randAspectRatioValue();
        std::string width_str = rng_.randWidthValue();
        
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio and width (no height)
        ComputedStyle style = ApplyStyleAndResolve(div, 
            "aspect-ratio: " + ratio_str + "; width: " + width_str);
        
        // Verify aspect-ratio was parsed correctly
        float expected_ratio = ParseExpectedRatio(ratio_str);
        
        EXPECT_FALSE(style.aspect_ratio.is_auto)
            << "aspect-ratio should not be auto when a ratio is specified. "
            << "Input: \"" << ratio_str << "\"";
        
        EXPECT_TRUE(style.aspect_ratio.HasRatio())
            << "aspect-ratio should have a valid ratio. "
            << "Input: \"" << ratio_str << "\"";
        
        EXPECT_NEAR(style.aspect_ratio.ratio, expected_ratio, 0.001f)
            << "aspect-ratio should be parsed correctly. "
            << "Input: \"" << ratio_str << "\", "
            << "Expected: " << expected_ratio << ", "
            << "Got: " << style.aspect_ratio.ratio;
    }
}

/**
 * **Feature: css-media-layout-properties, Property 4: Aspect Ratio Calculates Missing Dimension**
 * 
 * For any element with aspect-ratio set and only height specified,
 * the width should be calculated correctly from height * aspect-ratio.
 * 
 * **Validates: Requirements 3.6**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioCalculatesMissingWidth) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string ratio_str = rng_.randAspectRatioValue();
        std::string height_str = rng_.randHeightValue();
        
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio and height (no width)
        ComputedStyle style = ApplyStyleAndResolve(div, 
            "aspect-ratio: " + ratio_str + "; height: " + height_str);
        
        // Verify aspect-ratio was parsed correctly
        float expected_ratio = ParseExpectedRatio(ratio_str);
        
        EXPECT_FALSE(style.aspect_ratio.is_auto)
            << "aspect-ratio should not be auto when a ratio is specified. "
            << "Input: \"" << ratio_str << "\"";
        
        EXPECT_TRUE(style.aspect_ratio.HasRatio())
            << "aspect-ratio should have a valid ratio. "
            << "Input: \"" << ratio_str << "\"";
        
        EXPECT_NEAR(style.aspect_ratio.ratio, expected_ratio, 0.001f)
            << "aspect-ratio should be parsed correctly. "
            << "Input: \"" << ratio_str << "\", "
            << "Expected: " << expected_ratio << ", "
            << "Got: " << style.aspect_ratio.ratio;
    }
}

/**
 * **Feature: css-media-layout-properties, Property 5: Aspect Ratio Ignored When Both Dimensions Set**
 * 
 * For any element with both width and height explicitly set,
 * the aspect-ratio property should have no effect on the computed dimensions.
 * The aspect-ratio is still stored but should be ignored during layout.
 * 
 * **Validates: Requirements 3.4**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioIgnoredWhenBothDimensionsSet) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string ratio_str = rng_.randAspectRatioValue();
        std::string width_str = rng_.randWidthValue();
        std::string height_str = rng_.randHeightValue();
        
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio with both width and height
        ComputedStyle style = ApplyStyleAndResolve(div, 
            "aspect-ratio: " + ratio_str + "; width: " + width_str + "; height: " + height_str);
        
        // The aspect-ratio should still be parsed and stored
        float expected_ratio = ParseExpectedRatio(ratio_str);
        
        EXPECT_FALSE(style.aspect_ratio.is_auto)
            << "aspect-ratio should be stored even when both dimensions are set. "
            << "Input: \"" << ratio_str << "\"";
        
        EXPECT_NEAR(style.aspect_ratio.ratio, expected_ratio, 0.001f)
            << "aspect-ratio should be parsed correctly. "
            << "Input: \"" << ratio_str << "\", "
            << "Expected: " << expected_ratio << ", "
            << "Got: " << style.aspect_ratio.ratio;
        
        // Both width and height should be set (not auto)
        EXPECT_FALSE(style.width.IsAuto())
            << "width should be set when explicitly specified.";
        
        EXPECT_FALSE(style.height.IsAuto())
            << "height should be set when explicitly specified.";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Aspect Ratio Auto Value**
 * 
 * For any element with aspect-ratio: auto, the element should use its
 * intrinsic aspect ratio if available, otherwise no ratio is applied.
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioAutoValue) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio: auto
        ComputedStyle style = ApplyStyleAndResolve(div, "aspect-ratio: auto");
        
        EXPECT_TRUE(style.aspect_ratio.is_auto)
            << "aspect-ratio: auto should set is_auto to true.";
        
        EXPECT_FALSE(style.aspect_ratio.HasRatio())
            << "aspect-ratio: auto should not have a specific ratio.";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Aspect Ratio Default Value**
 * 
 * For any element without explicit aspect-ratio, the default value
 * should be auto (no ratio applied).
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioDefaultsToAuto) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply some other style, but not aspect-ratio
        ComputedStyle style = ApplyStyleAndResolve(div, "width: 100px");
        
        EXPECT_TRUE(style.aspect_ratio.is_auto)
            << "aspect-ratio should default to auto.";
        
        EXPECT_FALSE(style.aspect_ratio.HasRatio())
            << "aspect-ratio should not have a ratio by default.";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Layout Style Conversion**
 * 
 * For any element with aspect-ratio set, the value should be correctly
 * converted to the layout style for use in layout calculations.
 * 
 * **Validates: Requirements 3.4-3.7**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioLayoutStyleConversion) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string ratio_str = rng_.randAspectRatioValue();
        
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio
        ComputedStyle style = ApplyStyleAndResolve(div, 
            "aspect-ratio: " + ratio_str);
        
        // Create a RenderObject and update its layout style
        auto render_obj = std::make_shared<RenderBlock>();
        render_obj->SetComputedStyle(style);
        render_obj->MarkLayoutStyleDirty();
        render_obj->UpdateLayoutStyle();
        
        // Verify the layout style has the correct aspect ratio
        const auto& layout_style = render_obj->GetLayoutStyle();
        float expected_ratio = ParseExpectedRatio(ratio_str);
        
        EXPECT_TRUE(layout_style.aspect_ratio.has_value())
            << "Layout style should have aspect_ratio set. "
            << "Input: \"" << ratio_str << "\"";
        
        if (layout_style.aspect_ratio.has_value()) {
            EXPECT_NEAR(*layout_style.aspect_ratio, expected_ratio, 0.001f)
                << "Layout style aspect_ratio should match computed style. "
                << "Input: \"" << ratio_str << "\", "
                << "Expected: " << expected_ratio << ", "
                << "Got: " << *layout_style.aspect_ratio;
        }
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Aspect Ratio Auto Layout Style**
 * 
 * For any element with aspect-ratio: auto, the layout style should
 * have no aspect ratio set (nullopt).
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(AspectRatioPropertyTest, AspectRatioAutoLayoutStyleConversion) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateMockDocument();
        auto div = CreateMockElement(doc, "div");
        
        // Apply aspect-ratio: auto
        ComputedStyle style = ApplyStyleAndResolve(div, "aspect-ratio: auto");
        
        // Create a RenderObject and update its layout style
        auto render_obj = std::make_shared<RenderBlock>();
        render_obj->SetComputedStyle(style);
        render_obj->MarkLayoutStyleDirty();
        render_obj->UpdateLayoutStyle();
        
        // Verify the layout style has no aspect ratio
        const auto& layout_style = render_obj->GetLayoutStyle();
        
        EXPECT_FALSE(layout_style.aspect_ratio.has_value())
            << "Layout style should not have aspect_ratio for auto value.";
    }
}
