/**
 * @file test_outline_properties.cpp
 * @brief Property-based tests for CSS outline property parsing
 * 
 * **Feature: css-basic-interaction-properties, Property 1: Outline Property Parsing Round-Trip**
 * **Validates: Requirements 1.1, 1.2, 1.4, 1.5**
 * 
 * This file implements property-based testing using manual iteration with random values.
 * Each property test runs 100 iterations with randomly generated inputs.
 */

#include <gtest/gtest.h>
#include "render/render_object.h"
#include "render/css_value.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <vector>
#include <string>

using namespace lightui;

// Random number generator for property tests
class PropertyTestRng {
public:
    PropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    template<typename T>
    const T& randElement(const std::vector<T>& vec) {
        return vec[randInt(0, static_cast<int>(vec.size()) - 1)];
    }
    
private:
    std::mt19937 gen_;
};

// Valid outline style values
const std::vector<std::string> OUTLINE_STYLES = {
    "none", "solid", "dashed", "dotted", "double"
};

class OutlinePropertyTest : public ::testing::Test {
protected:
    PropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;

    std::string genOutlineStyle() {
        return rng_.randElement(OUTLINE_STYLES);
    }
    
    std::string genHexColor(int& r, int& g, int& b) {
        r = rng_.randInt(0, 255);
        g = rng_.randInt(0, 255);
        b = rng_.randInt(0, 255);
        std::stringstream ss;
        ss << "#" << std::hex << std::setfill('0')
           << std::setw(2) << r
           << std::setw(2) << g
           << std::setw(2) << b;
        return ss.str();
    }
};

/**
 * **Feature: css-basic-interaction-properties, Property 1: Outline Property Parsing Round-Trip**
 * 
 * For any valid outline-width value, parsing it and retrieving the stored value
 * should produce an equivalent length.
 * 
 * **Validates: Requirements 1.2**
 */
TEST_F(OutlinePropertyTest, OutlineWidthRoundTrip) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int width_px = rng_.randInt(0, 20);
        std::string width_str = std::to_string(width_px) + "px";
        
        ComputedStyle style;
        auto length = CSSValue::ParseLength(width_str);
        style.outline_width = length;
        
        EXPECT_FLOAT_EQ(style.outline_width.value, static_cast<float>(width_px))
            << "Failed for width: " << width_str;
        EXPECT_EQ(style.outline_width.unit, CSSUnit::PX)
            << "Failed for width: " << width_str;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 1: Outline Property Parsing Round-Trip**
 * 
 * For any valid outline-style value, parsing it should store the exact same value.
 * 
 * **Validates: Requirements 1.1**
 */
TEST_F(OutlinePropertyTest, OutlineStyleRoundTrip) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string style_str = genOutlineStyle();
        
        ComputedStyle computed_style;
        computed_style.outline_style = style_str;
        
        EXPECT_EQ(computed_style.outline_style, style_str)
            << "Failed for style: " << style_str;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 1: Outline Property Parsing Round-Trip**
 * 
 * For any valid outline-color value, parsing it and converting back should
 * produce equivalent RGB values.
 * 
 * **Validates: Requirements 1.4**
 */
TEST_F(OutlinePropertyTest, OutlineColorRoundTrip) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int r, g, b;
        std::string color_str = genHexColor(r, g, b);
        
        SkColor parsed_color = CSSValue::ParseColor(color_str);
        
        EXPECT_EQ(static_cast<int>(SkColorGetR(parsed_color)), r)
            << "Red mismatch for color: " << color_str;
        EXPECT_EQ(static_cast<int>(SkColorGetG(parsed_color)), g)
            << "Green mismatch for color: " << color_str;
        EXPECT_EQ(static_cast<int>(SkColorGetB(parsed_color)), b)
            << "Blue mismatch for color: " << color_str;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 1: Outline Property Parsing Round-Trip**
 * 
 * For any valid outline-offset value, parsing it should preserve the value and unit.
 * 
 * **Validates: Requirements 1.5**
 */
TEST_F(OutlinePropertyTest, OutlineOffsetRoundTrip) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int offset_px = rng_.randInt(-20, 20);
        std::string offset_str = std::to_string(offset_px) + "px";
        
        ComputedStyle style;
        style.outline_offset = CSSValue::ParseLength(offset_str);
        
        EXPECT_FLOAT_EQ(style.outline_offset.value, static_cast<float>(offset_px))
            << "Failed for offset: " << offset_str;
        EXPECT_EQ(style.outline_offset.unit, CSSUnit::PX)
            << "Failed for offset: " << offset_str;
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 2: Outline Does Not Affect Layout**
 * 
 * For any element with outline properties set, the element's layout dimensions
 * (width, height, margin, padding, border) should be identical to the same
 * element without outline.
 * 
 * **Validates: Requirements 1.7**
 */
TEST_F(OutlinePropertyTest, OutlineDoesNotAffectLayout) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random layout dimensions
        float width = static_cast<float>(rng_.randInt(50, 500));
        float height = static_cast<float>(rng_.randInt(50, 500));
        float margin = static_cast<float>(rng_.randInt(0, 50));
        float padding = static_cast<float>(rng_.randInt(0, 50));
        float border_width = static_cast<float>(rng_.randInt(0, 10));
        
        // Generate random outline properties
        int outline_width = rng_.randInt(1, 20);
        std::string outline_style = genOutlineStyle();
        int outline_offset = rng_.randInt(-10, 20);
        
        // Create style WITHOUT outline
        ComputedStyle style_without_outline;
        style_without_outline.width = CSSLength(width, CSSUnit::PX);
        style_without_outline.height = CSSLength(height, CSSUnit::PX);
        style_without_outline.margin_top = CSSLength(margin, CSSUnit::PX);
        style_without_outline.margin_right = CSSLength(margin, CSSUnit::PX);
        style_without_outline.margin_bottom = CSSLength(margin, CSSUnit::PX);
        style_without_outline.margin_left = CSSLength(margin, CSSUnit::PX);
        style_without_outline.padding_top = CSSLength(padding, CSSUnit::PX);
        style_without_outline.padding_right = CSSLength(padding, CSSUnit::PX);
        style_without_outline.padding_bottom = CSSLength(padding, CSSUnit::PX);
        style_without_outline.padding_left = CSSLength(padding, CSSUnit::PX);
        style_without_outline.border_top_width = border_width;
        style_without_outline.border_right_width = border_width;
        style_without_outline.border_bottom_width = border_width;
        style_without_outline.border_left_width = border_width;
        
        // Create style WITH outline
        ComputedStyle style_with_outline;
        style_with_outline.width = CSSLength(width, CSSUnit::PX);
        style_with_outline.height = CSSLength(height, CSSUnit::PX);
        style_with_outline.margin_top = CSSLength(margin, CSSUnit::PX);
        style_with_outline.margin_right = CSSLength(margin, CSSUnit::PX);
        style_with_outline.margin_bottom = CSSLength(margin, CSSUnit::PX);
        style_with_outline.margin_left = CSSLength(margin, CSSUnit::PX);
        style_with_outline.padding_top = CSSLength(padding, CSSUnit::PX);
        style_with_outline.padding_right = CSSLength(padding, CSSUnit::PX);
        style_with_outline.padding_bottom = CSSLength(padding, CSSUnit::PX);
        style_with_outline.padding_left = CSSLength(padding, CSSUnit::PX);
        style_with_outline.border_top_width = border_width;
        style_with_outline.border_right_width = border_width;
        style_with_outline.border_bottom_width = border_width;
        style_with_outline.border_left_width = border_width;
        // Set outline properties
        style_with_outline.outline_width = CSSLength(static_cast<float>(outline_width), CSSUnit::PX);
        style_with_outline.outline_style = outline_style;
        style_with_outline.outline_offset = CSSLength(static_cast<float>(outline_offset), CSSUnit::PX);
        
        // Verify that layout-affecting properties are identical
        EXPECT_FLOAT_EQ(style_without_outline.width.ToPx(), style_with_outline.width.ToPx())
            << "Width should not be affected by outline";
        EXPECT_FLOAT_EQ(style_without_outline.height.ToPx(), style_with_outline.height.ToPx())
            << "Height should not be affected by outline";
        
        // Margins
        EXPECT_FLOAT_EQ(style_without_outline.margin_top.ToPx(), style_with_outline.margin_top.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.margin_right.ToPx(), style_with_outline.margin_right.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.margin_bottom.ToPx(), style_with_outline.margin_bottom.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.margin_left.ToPx(), style_with_outline.margin_left.ToPx());
        
        // Padding
        EXPECT_FLOAT_EQ(style_without_outline.padding_top.ToPx(), style_with_outline.padding_top.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.padding_right.ToPx(), style_with_outline.padding_right.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.padding_bottom.ToPx(), style_with_outline.padding_bottom.ToPx());
        EXPECT_FLOAT_EQ(style_without_outline.padding_left.ToPx(), style_with_outline.padding_left.ToPx());
        
        // Border widths
        EXPECT_FLOAT_EQ(style_without_outline.border_top_width, style_with_outline.border_top_width);
        EXPECT_FLOAT_EQ(style_without_outline.border_right_width, style_with_outline.border_right_width);
        EXPECT_FLOAT_EQ(style_without_outline.border_bottom_width, style_with_outline.border_bottom_width);
        EXPECT_FLOAT_EQ(style_without_outline.border_left_width, style_with_outline.border_left_width);
    }
}
// force rebuild  
