/**
 * @file test_list_style_properties.cpp
 * @brief Property-based tests for CSS list-style properties
 * 
 * This file implements property-based testing for list-style-type,
 * list-style-position, and list-style-image functionality.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-media-layout-properties**
 * **Validates: Requirements 4.1-4.15**
 */

#include <gtest/gtest.h>
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_object.h"
#include "core/render/objects/list_marker.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <random>
#include <vector>
#include <string>

using namespace mbink;

// Random number generator for property tests
class ListStylePropertyTestRng {
public:
    ListStylePropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    std::string randListStyleType() {
        static const std::vector<std::string> values = {
            "disc", "circle", "square", "decimal", "decimal-leading-zero",
            "lower-roman", "upper-roman", "lower-alpha", "upper-alpha", "none"
        };
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randListStylePosition() {
        static const std::vector<std::string> values = {"inside", "outside"};
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randInvalidListStyleType() {
        static const std::vector<std::string> values = {
            "bullet", "number", "letter", "roman", "DISC", "CIRCLE",
            "123", "disc-circle", "none-disc"
        };
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
private:
    std::mt19937 gen_;
};

class ListStylePropertyTest : public ::testing::Test {
protected:
    ListStylePropertyTestRng rng_;
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
                                        const std::string& style_str,
                                        const ComputedStyle* parent_style = nullptr) {
        element->SetAttribute("style", style_str);
        return style_resolver_.ResolveStyle(element, parent_style);
    }
};

/**
 * **Feature: css-media-layout-properties, Property 6: List Style Type Inheritance**
 * 
 * For any <li> element without explicit list-style-type, it should inherit
 * the value from its parent <ul> or <ol>.
 * 
 * **Validates: Requirements 4.15**
 */
TEST_F(ListStylePropertyTest, ListStyleTypeInheritance) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string parent_list_style = rng_.randListStyleType();
        
        auto doc = CreateMockDocument();
        auto ul = CreateMockElement(doc, "ul");
        auto li = CreateMockElement(doc, "li");
        
        // Apply list-style-type to parent ul
        ComputedStyle parent_style = ApplyStyleAndResolve(ul, 
            "list-style-type: " + parent_list_style);
        
        // Resolve li style with parent style (should inherit)
        ComputedStyle child_style = ApplyStyleAndResolve(li, "", &parent_style);
        
        EXPECT_EQ(child_style.list_style_type, parent_list_style)
            << "list-style-type should be inherited from parent. "
            << "Parent value: \"" << parent_list_style << "\", "
            << "Child got: \"" << child_style.list_style_type << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 7: List Style None Hides Marker**
 * 
 * For any list item with list-style-type: none, no marker should be rendered.
 * This is verified by checking that ParseListStyleType returns NONE.
 * 
 * **Validates: Requirements 4.10**
 */
TEST_F(ListStylePropertyTest, ListStyleNoneHidesMarker) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto doc = CreateMockDocument();
        auto li = CreateMockElement(doc, "li");
        
        // Apply list-style-type: none
        ComputedStyle style = ApplyStyleAndResolve(li, "list-style-type: none");
        
        EXPECT_EQ(style.list_style_type, "none")
            << "list-style-type: none should be parsed correctly.";
        
        // Verify that ParseListStyleType returns NONE
        ListMarkerType marker_type = ParseListStyleType(style.list_style_type);
        EXPECT_EQ(marker_type, ListMarkerType::NONE)
            << "ParseListStyleType should return NONE for 'none' value.";
        
        // Verify that GenerateMarkerText returns empty string for NONE
        std::string marker_text = GenerateMarkerText(marker_type, rng_.randInt(1, 100));
        EXPECT_TRUE(marker_text.empty())
            << "GenerateMarkerText should return empty string for NONE type.";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 8: Decimal List Markers Sequential**
 * 
 * For any ordered list with list-style-type: decimal, markers should be
 * sequential integers starting from 1.
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(ListStylePropertyTest, DecimalListMarkersSequential) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate a random starting index and count
        int start_index = rng_.randInt(1, 100);
        int count = rng_.randInt(1, 20);
        
        for (int j = 0; j < count; ++j) {
            int index = start_index + j;
            std::string marker_text = GenerateMarkerText(ListMarkerType::DECIMAL, index);
            
            std::string expected = std::to_string(index) + ".";
            EXPECT_EQ(marker_text, expected)
                << "Decimal marker for index " << index << " should be \"" << expected << "\", "
                << "got \"" << marker_text << "\"";
        }
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Valid List Style Types Are Parsed**
 * 
 * For any valid list-style-type value, the StyleResolver should correctly
 * parse and store the value in ComputedStyle.
 * 
 * **Validates: Requirements 4.1-4.10**
 */
TEST_F(ListStylePropertyTest, ValidListStyleTypesAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string list_style_type = rng_.randListStyleType();
        
        auto doc = CreateMockDocument();
        auto li = CreateMockElement(doc, "li");
        ComputedStyle style = ApplyStyleAndResolve(li, 
            "list-style-type: " + list_style_type);
        
        EXPECT_EQ(style.list_style_type, list_style_type)
            << "list-style-type should be parsed correctly. "
            << "Input: \"" << list_style_type << "\", "
            << "Got: \"" << style.list_style_type << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Invalid List Style Types Are Ignored**
 * 
 * For any invalid list-style-type value, the StyleResolver should ignore it
 * and keep the default value "disc".
 * 
 * **Validates: Requirements 4.1-4.10**
 */
TEST_F(ListStylePropertyTest, InvalidListStyleTypesAreIgnored) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string invalid_value = rng_.randInvalidListStyleType();
        
        auto doc = CreateMockDocument();
        auto li = CreateMockElement(doc, "li");
        ComputedStyle style = ApplyStyleAndResolve(li, 
            "list-style-type: " + invalid_value);
        
        EXPECT_EQ(style.list_style_type, "disc")
            << "Invalid list-style-type should be ignored, keeping default 'disc'. "
            << "Input: \"" << invalid_value << "\", "
            << "Got: \"" << style.list_style_type << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: List Style Position Parsing**
 * 
 * For any valid list-style-position value, the StyleResolver should correctly
 * parse and store the value in ComputedStyle.
 * 
 * **Validates: Requirements 4.11-4.12**
 */
TEST_F(ListStylePropertyTest, ListStylePositionParsing) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string position = rng_.randListStylePosition();
        
        auto doc = CreateMockDocument();
        auto li = CreateMockElement(doc, "li");
        ComputedStyle style = ApplyStyleAndResolve(li, 
            "list-style-position: " + position);
        
        EXPECT_EQ(style.list_style_position, position)
            << "list-style-position should be parsed correctly. "
            << "Input: \"" << position << "\", "
            << "Got: \"" << style.list_style_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: List Style Position Inheritance**
 * 
 * For any <li> element without explicit list-style-position, it should inherit
 * the value from its parent.
 * 
 * **Validates: Requirements 4.11-4.12**
 */
TEST_F(ListStylePropertyTest, ListStylePositionInheritance) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string parent_position = rng_.randListStylePosition();
        
        auto doc = CreateMockDocument();
        auto ul = CreateMockElement(doc, "ul");
        auto li = CreateMockElement(doc, "li");
        
        // Apply list-style-position to parent ul
        ComputedStyle parent_style = ApplyStyleAndResolve(ul, 
            "list-style-position: " + parent_position);
        
        // Resolve li style with parent style (should inherit)
        ComputedStyle child_style = ApplyStyleAndResolve(li, "", &parent_style);
        
        EXPECT_EQ(child_style.list_style_position, parent_position)
            << "list-style-position should be inherited from parent. "
            << "Parent value: \"" << parent_position << "\", "
            << "Child got: \"" << child_style.list_style_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Roman Numeral Conversion**
 * 
 * For any number in the valid range (1-3999), ToLowerRoman and ToUpperRoman
 * should produce valid Roman numeral strings.
 * 
 * **Validates: Requirements 4.6, 4.7**
 */
TEST_F(ListStylePropertyTest, RomanNumeralConversion) {
    // Test specific known values
    EXPECT_EQ(ToLowerRoman(1), "i");
    EXPECT_EQ(ToLowerRoman(4), "iv");
    EXPECT_EQ(ToLowerRoman(5), "v");
    EXPECT_EQ(ToLowerRoman(9), "ix");
    EXPECT_EQ(ToLowerRoman(10), "x");
    EXPECT_EQ(ToLowerRoman(40), "xl");
    EXPECT_EQ(ToLowerRoman(50), "l");
    EXPECT_EQ(ToLowerRoman(90), "xc");
    EXPECT_EQ(ToLowerRoman(100), "c");
    EXPECT_EQ(ToLowerRoman(400), "cd");
    EXPECT_EQ(ToLowerRoman(500), "d");
    EXPECT_EQ(ToLowerRoman(900), "cm");
    EXPECT_EQ(ToLowerRoman(1000), "m");
    EXPECT_EQ(ToLowerRoman(1999), "mcmxcix");
    EXPECT_EQ(ToLowerRoman(3999), "mmmcmxcix");
    
    // Test uppercase versions
    EXPECT_EQ(ToUpperRoman(1), "I");
    EXPECT_EQ(ToUpperRoman(4), "IV");
    EXPECT_EQ(ToUpperRoman(1999), "MCMXCIX");
    
    // Property test: random values should produce non-empty strings
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int num = rng_.randInt(1, 3999);
        std::string lower = ToLowerRoman(num);
        std::string upper = ToUpperRoman(num);
        
        EXPECT_FALSE(lower.empty())
            << "ToLowerRoman(" << num << ") should not be empty";
        EXPECT_FALSE(upper.empty())
            << "ToUpperRoman(" << num << ") should not be empty";
        
        // Upper should be uppercase version of lower
        std::string lower_to_upper;
        for (char c : lower) {
            lower_to_upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        EXPECT_EQ(upper, lower_to_upper)
            << "ToUpperRoman should be uppercase of ToLowerRoman for " << num;
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Alpha Conversion**
 * 
 * For any positive number, ToLowerAlpha and ToUpperAlpha should produce
 * valid alphabetic strings (a, b, ..., z, aa, ab, ...).
 * 
 * **Validates: Requirements 4.8, 4.9**
 */
TEST_F(ListStylePropertyTest, AlphaConversion) {
    // Test specific known values
    EXPECT_EQ(ToLowerAlpha(1), "a");
    EXPECT_EQ(ToLowerAlpha(2), "b");
    EXPECT_EQ(ToLowerAlpha(26), "z");
    EXPECT_EQ(ToLowerAlpha(27), "aa");
    EXPECT_EQ(ToLowerAlpha(28), "ab");
    EXPECT_EQ(ToLowerAlpha(52), "az");
    EXPECT_EQ(ToLowerAlpha(53), "ba");
    
    // Test uppercase versions
    EXPECT_EQ(ToUpperAlpha(1), "A");
    EXPECT_EQ(ToUpperAlpha(26), "Z");
    EXPECT_EQ(ToUpperAlpha(27), "AA");
    
    // Property test: random values should produce non-empty strings
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int num = rng_.randInt(1, 1000);
        std::string lower = ToLowerAlpha(num);
        std::string upper = ToUpperAlpha(num);
        
        EXPECT_FALSE(lower.empty())
            << "ToLowerAlpha(" << num << ") should not be empty";
        EXPECT_FALSE(upper.empty())
            << "ToUpperAlpha(" << num << ") should not be empty";
        
        // Upper should be uppercase version of lower
        std::string lower_to_upper;
        for (char c : lower) {
            lower_to_upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        EXPECT_EQ(upper, lower_to_upper)
            << "ToUpperAlpha should be uppercase of ToLowerAlpha for " << num;
    }
}

/**
 * **Feature: css-media-layout-properties, Property: Decimal Leading Zero**
 * 
 * For decimal-leading-zero, numbers 1-9 should have a leading zero,
 * while numbers >= 10 should not.
 * 
 * **Validates: Requirements 4.5**
 */
TEST_F(ListStylePropertyTest, DecimalLeadingZero) {
    // Test numbers 1-9 should have leading zero
    for (int i = 1; i <= 9; ++i) {
        std::string marker = GenerateMarkerText(ListMarkerType::DECIMAL_LEADING_ZERO, i);
        std::string expected = "0" + std::to_string(i) + ".";
        EXPECT_EQ(marker, expected)
            << "Decimal-leading-zero for " << i << " should be \"" << expected << "\"";
    }
    
    // Test numbers >= 10 should not have leading zero
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        int num = rng_.randInt(10, 1000);
        std::string marker = GenerateMarkerText(ListMarkerType::DECIMAL_LEADING_ZERO, num);
        std::string expected = std::to_string(num) + ".";
        EXPECT_EQ(marker, expected)
            << "Decimal-leading-zero for " << num << " should be \"" << expected << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property: List Style Shorthand Parsing**
 * 
 * For any valid list-style shorthand value, the StyleResolver should correctly
 * parse and store all component values.
 * 
 * **Validates: Requirements 4.14**
 */
TEST_F(ListStylePropertyTest, ListStyleShorthandParsing) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string type = rng_.randListStyleType();
        std::string position = rng_.randListStylePosition();
        
        auto doc = CreateMockDocument();
        auto li = CreateMockElement(doc, "li");
        
        // Test shorthand with type and position
        ComputedStyle style = ApplyStyleAndResolve(li, 
            "list-style: " + type + " " + position);
        
        EXPECT_EQ(style.list_style_type, type)
            << "list-style shorthand should parse type correctly. "
            << "Input: \"" << type << " " << position << "\", "
            << "Got type: \"" << style.list_style_type << "\"";
        
        EXPECT_EQ(style.list_style_position, position)
            << "list-style shorthand should parse position correctly. "
            << "Input: \"" << type << " " << position << "\", "
            << "Got position: \"" << style.list_style_position << "\"";
    }
}

