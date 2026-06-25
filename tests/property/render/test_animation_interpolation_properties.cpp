/**
 * @file test_animation_interpolation_properties.cpp
 * @brief Property-based tests for animation property interpolation
 * 
 * This file implements property-based testing for PropertyInterpolation.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real PropertyInterpolation class
 * - Real color parsing and interpolation
 * - Real numeric value interpolation
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/render/animation/property_interpolation.h"
#include "core/render/utils/color.h"
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

using namespace mblink;
using namespace mblink::test;

// Random number generator for property tests
class InterpolationTestRng {
public:
    InterpolationTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    std::string randNumericValue() {
        float value = randFloat(0.0f, 500.0f);
        static const std::vector<std::string> units = {"px", "em", "%"};
        std::string unit = units[randInt(0, units.size() - 1)];
        
        std::ostringstream oss;
        oss << value << unit;
        return oss.str();
    }
    
    std::string randColorHex() {
        std::ostringstream oss;
        oss << "#" << std::hex << std::setfill('0')
            << std::setw(2) << randInt(0, 255)
            << std::setw(2) << randInt(0, 255)
            << std::setw(2) << randInt(0, 255);
        return oss.str();
    }
    
    std::string randColorRgb() {
        std::ostringstream oss;
        oss << "rgb(" << randInt(0, 255) << ", " 
            << randInt(0, 255) << ", " 
            << randInt(0, 255) << ")";
        return oss.str();
    }
    
private:
    std::mt19937 gen_;
};


/**
 * @brief Test fixture for interpolation property tests
 */
class AnimationInterpolationPropertyTest : public ::testing::Test {
protected:
    InterpolationTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
};


/**
 * **Feature: css-animation-integration, Property 6: Property interpolation correctness**
 * 
 * For any two numeric property values V1 and V2 with the same unit,
 * interpolating at factor F should produce V1 + (V2 - V1) * F.
 * 
 * **Validates: Requirements 3.2, 3.3**
 */
TEST_F(AnimationInterpolationPropertyTest, NumericInterpolationCorrectness) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random numeric values with same unit
        float from_value = rng_.randFloat(0.0f, 500.0f);
        float to_value = rng_.randFloat(0.0f, 500.0f);
        float factor = rng_.randFloat(0.0f, 1.0f);
        
        std::string unit = "px";
        std::ostringstream from_oss, to_oss;
        from_oss << from_value << unit;
        to_oss << to_value << unit;
        
        std::string from_str = from_oss.str();
        std::string to_str = to_oss.str();
        
        // Interpolate using REAL PropertyInterpolation
        auto result = PropertyInterpolation::Interpolate("width", from_str, to_str, factor);
        
        ASSERT_TRUE(result.has_value())
            << "Interpolation failed at iteration " << i
            << ", from: " << from_str << ", to: " << to_str;
        
        // Parse result
        std::string result_str = *result;
        float result_value = 0.0f;
        std::sscanf(result_str.c_str(), "%f", &result_value);
        
        // Calculate expected value
        float expected = from_value + (to_value - from_value) * factor;
        
        // Verify with tolerance
        EXPECT_NEAR(result_value, expected, 0.01f)
            << "Numeric interpolation incorrect at iteration " << i
            << ", from: " << from_value << ", to: " << to_value
            << ", factor: " << factor
            << ", expected: " << expected << ", got: " << result_value;
    }
}


/**
 * @brief Test color interpolation correctness
 * 
 * For any two color values, interpolating should produce a color
 * where each RGB component is linearly interpolated.
 */
TEST_F(AnimationInterpolationPropertyTest, ColorInterpolationCorrectness) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random RGB values
        int from_r = rng_.randInt(0, 255);
        int from_g = rng_.randInt(0, 255);
        int from_b = rng_.randInt(0, 255);
        int to_r = rng_.randInt(0, 255);
        int to_g = rng_.randInt(0, 255);
        int to_b = rng_.randInt(0, 255);
        float factor = rng_.randFloat(0.0f, 1.0f);
        
        std::ostringstream from_oss, to_oss;
        from_oss << "rgb(" << from_r << ", " << from_g << ", " << from_b << ")";
        to_oss << "rgb(" << to_r << ", " << to_g << ", " << to_b << ")";
        
        std::string from_str = from_oss.str();
        std::string to_str = to_oss.str();
        
        // Interpolate using REAL PropertyInterpolation
        auto result = PropertyInterpolation::Interpolate("color", from_str, to_str, factor);
        
        ASSERT_TRUE(result.has_value())
            << "Color interpolation failed at iteration " << i
            << ", from: " << from_str << ", to: " << to_str;
        
        // Parse result (expecting rgb(r, g, b) format)
        std::string result_str = *result;
        int result_r = 0, result_g = 0, result_b = 0;
        std::sscanf(result_str.c_str(), "rgb(%d, %d, %d)", &result_r, &result_g, &result_b);
        
        // Calculate expected values
        int expected_r = static_cast<int>(from_r + (to_r - from_r) * factor);
        int expected_g = static_cast<int>(from_g + (to_g - from_g) * factor);
        int expected_b = static_cast<int>(from_b + (to_b - from_b) * factor);
        
        // Verify with tolerance (allow ±1 for rounding)
        EXPECT_NEAR(result_r, expected_r, 1)
            << "Red component incorrect at iteration " << i;
        EXPECT_NEAR(result_g, expected_g, 1)
            << "Green component incorrect at iteration " << i;
        EXPECT_NEAR(result_b, expected_b, 1)
            << "Blue component incorrect at iteration " << i;
    }
}


/**
 * @brief Test interpolation boundary conditions
 * 
 * At factor 0, result should equal from value.
 * At factor 1, result should equal to value.
 */
TEST_F(AnimationInterpolationPropertyTest, InterpolationBoundaryConditions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float from_value = rng_.randFloat(0.0f, 500.0f);
        float to_value = rng_.randFloat(0.0f, 500.0f);
        
        std::ostringstream from_oss, to_oss;
        from_oss << from_value << "px";
        to_oss << to_value << "px";
        
        std::string from_str = from_oss.str();
        std::string to_str = to_oss.str();
        
        // Test factor = 0
        auto result_0 = PropertyInterpolation::Interpolate("width", from_str, to_str, 0.0f);
        ASSERT_TRUE(result_0.has_value());
        
        float result_value_0 = 0.0f;
        std::sscanf(result_0->c_str(), "%f", &result_value_0);
        
        EXPECT_NEAR(result_value_0, from_value, 0.01f)
            << "At factor 0, result should equal from value at iteration " << i;
        
        // Test factor = 1
        auto result_1 = PropertyInterpolation::Interpolate("width", from_str, to_str, 1.0f);
        ASSERT_TRUE(result_1.has_value());
        
        float result_value_1 = 0.0f;
        std::sscanf(result_1->c_str(), "%f", &result_value_1);
        
        EXPECT_NEAR(result_value_1, to_value, 0.01f)
            << "At factor 1, result should equal to value at iteration " << i;
    }
}


/**
 * @brief Test interpolation with different numeric properties
 * 
 * Verifies that various numeric properties (width, height, margin, etc.)
 * all interpolate correctly.
 */
TEST_F(AnimationInterpolationPropertyTest, DifferentNumericProperties) {
    static const std::vector<std::string> numeric_properties = {
        "width", "height", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding-top", "padding-right", "padding-bottom", "padding-left",
        "top", "right", "bottom", "left", "font-size", "opacity"
    };
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Pick a random property
        std::string property = numeric_properties[rng_.randInt(0, numeric_properties.size() - 1)];
        
        float from_value = rng_.randFloat(0.0f, 100.0f);
        float to_value = rng_.randFloat(0.0f, 100.0f);
        float factor = rng_.randFloat(0.0f, 1.0f);
        
        std::string unit = (property == "opacity") ? "" : "px";
        
        std::ostringstream from_oss, to_oss;
        from_oss << from_value << unit;
        to_oss << to_value << unit;
        
        std::string from_str = from_oss.str();
        std::string to_str = to_oss.str();
        
        // Interpolate
        auto result = PropertyInterpolation::Interpolate(property, from_str, to_str, factor);
        
        ASSERT_TRUE(result.has_value())
            << "Interpolation failed for property '" << property << "' at iteration " << i;
        
        // Parse and verify
        float result_value = 0.0f;
        std::sscanf(result->c_str(), "%f", &result_value);
        
        float expected = from_value + (to_value - from_value) * factor;
        
        EXPECT_NEAR(result_value, expected, 0.01f)
            << "Property '" << property << "' interpolation incorrect at iteration " << i;
    }
}


/**
 * @brief Test interpolation with mismatched units returns nullopt
 * 
 * When units don't match, interpolation should fail gracefully.
 */
TEST_F(AnimationInterpolationPropertyTest, MismatchedUnitsHandling) {
    // px vs %
    auto result1 = PropertyInterpolation::Interpolate("width", "100px", "50%", 0.5f);
    EXPECT_FALSE(result1.has_value()) << "Mismatched units (px vs %) should fail";
    
    // px vs em
    auto result2 = PropertyInterpolation::Interpolate("width", "100px", "5em", 0.5f);
    EXPECT_FALSE(result2.has_value()) << "Mismatched units (px vs em) should fail";
}


/**
 * @brief Test property map interpolation
 * 
 * Verifies that InterpolateProperties correctly handles multiple properties.
 */
TEST_F(AnimationInterpolationPropertyTest, PropertyMapInterpolation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::map<std::string, std::string> from_props;
        std::map<std::string, std::string> to_props;
        
        // Generate random properties
        float opacity_from = rng_.randFloat(0.0f, 1.0f);
        float opacity_to = rng_.randFloat(0.0f, 1.0f);
        float width_from = rng_.randFloat(0.0f, 500.0f);
        float width_to = rng_.randFloat(0.0f, 500.0f);
        
        std::ostringstream oss;
        oss << opacity_from;
        from_props["opacity"] = oss.str();
        oss.str(""); oss << opacity_to;
        to_props["opacity"] = oss.str();
        
        oss.str(""); oss << width_from << "px";
        from_props["width"] = oss.str();
        oss.str(""); oss << width_to << "px";
        to_props["width"] = oss.str();
        
        float factor = rng_.randFloat(0.0f, 1.0f);
        
        // Interpolate using REAL PropertyInterpolation
        auto result = PropertyInterpolation::InterpolateProperties(from_props, to_props, factor);
        
        // Verify opacity
        ASSERT_TRUE(result.find("opacity") != result.end())
            << "opacity missing from result at iteration " << i;
        
        float result_opacity = 0.0f;
        std::sscanf(result["opacity"].c_str(), "%f", &result_opacity);
        float expected_opacity = opacity_from + (opacity_to - opacity_from) * factor;
        
        EXPECT_NEAR(result_opacity, expected_opacity, 0.01f)
            << "opacity interpolation incorrect at iteration " << i;
        
        // Verify width
        ASSERT_TRUE(result.find("width") != result.end())
            << "width missing from result at iteration " << i;
        
        float result_width = 0.0f;
        std::sscanf(result["width"].c_str(), "%f", &result_width);
        float expected_width = width_from + (width_to - width_from) * factor;
        
        EXPECT_NEAR(result_width, expected_width, 0.01f)
            << "width interpolation incorrect at iteration " << i;
    }
}
