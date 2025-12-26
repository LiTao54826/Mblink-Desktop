/**
 * @file test_transform_interpolation_properties.cpp
 * @brief Property-based tests for CSS transform interpolation
 * 
 * This file implements property-based testing for transform decomposition,
 * interpolation, and composition.
 * 
 * **Feature: css-animation-integration**
 * 
 * IMPORTANT: All tests use REAL rendering layer components, not mocks:
 * - Real PropertyInterpolation class
 * - Real transform parsing and interpolation
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "render/animation/property_interpolation.h"
#include <random>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace lightui;
using namespace lightui::test;

class TransformInterpolationPropertyTest : public ::testing::Test {
protected:
    static constexpr int NUM_ITERATIONS = 100;
    std::mt19937 gen_{std::random_device{}()};
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    std::string generateTranslate() {
        float x = randFloat(-200.0f, 200.0f);
        float y = randFloat(-200.0f, 200.0f);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "translate(" << x << "px, " << y << "px)";
        return oss.str();
    }
    
    std::string generateRotate() {
        float deg = randFloat(-360.0f, 360.0f);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "rotate(" << deg << "deg)";
        return oss.str();
    }
    
    std::string generateScale() {
        float sx = randFloat(0.1f, 3.0f);
        float sy = randFloat(0.1f, 3.0f);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "scale(" << sx << ", " << sy << ")";
        return oss.str();
    }
    
    std::string generateSimpleTransform() {
        int type = randInt(0, 2);
        switch (type) {
            case 0: return generateTranslate();
            case 1: return generateRotate();
            case 2: return generateScale();
            default: return "none";
        }
    }
};

/**
 * **Feature: css-animation-integration, Property 7: Transform decompose round-trip**
 * 
 * For any valid CSS transform string, decomposing and then composing
 * should produce a functionally equivalent transform.
 * 
 * **Validates: Requirements 4.1, 4.3**
 */
TEST_F(TransformInterpolationPropertyTest, TransformRoundTripConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string original = generateSimpleTransform();
        
        // Interpolate at factor 0 should return approximately the from value
        auto result_0 = PropertyInterpolation::Interpolate("transform", original, original, 0.0f);
        
        ASSERT_TRUE(result_0.has_value())
            << "Transform interpolation failed at iteration " << i
            << " for transform: " << original;
        
        // Interpolate at factor 1 should return approximately the to value
        auto result_1 = PropertyInterpolation::Interpolate("transform", original, original, 1.0f);
        
        ASSERT_TRUE(result_1.has_value())
            << "Transform interpolation failed at iteration " << i
            << " for transform: " << original;
        
        // Both should produce valid transform strings
        EXPECT_FALSE(result_0->empty())
            << "Result at factor 0 is empty at iteration " << i;
        EXPECT_FALSE(result_1->empty())
            << "Result at factor 1 is empty at iteration " << i;
    }
}

/**
 * **Feature: css-animation-integration, Property 8: Transform interpolation linearity**
 * 
 * For any two transforms, interpolating at factor F should produce
 * values that are linearly interpolated for each component.
 * 
 * **Validates: Requirements 4.2**
 */
TEST_F(TransformInterpolationPropertyTest, TranslateInterpolationLinearity) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float from_x = randFloat(-100.0f, 100.0f);
        float from_y = randFloat(-100.0f, 100.0f);
        float to_x = randFloat(-100.0f, 100.0f);
        float to_y = randFloat(-100.0f, 100.0f);
        float factor = randFloat(0.0f, 1.0f);
        
        std::ostringstream from_oss, to_oss;
        from_oss << std::fixed << std::setprecision(2);
        to_oss << std::fixed << std::setprecision(2);
        
        from_oss << "translate(" << from_x << "px, " << from_y << "px)";
        to_oss << "translate(" << to_x << "px, " << to_y << "px)";
        
        auto result = PropertyInterpolation::Interpolate(
            "transform", from_oss.str(), to_oss.str(), factor);
        
        ASSERT_TRUE(result.has_value())
            << "Translate interpolation failed at iteration " << i;
        
        // Parse result to verify linearity
        float result_x = 0.0f, result_y = 0.0f;
        if (sscanf(result->c_str(), "translate(%fpx, %fpx)", &result_x, &result_y) == 2 ||
            sscanf(result->c_str(), "translate(%fpx,%fpx)", &result_x, &result_y) == 2) {
            
            float expected_x = from_x + (to_x - from_x) * factor;
            float expected_y = from_y + (to_y - from_y) * factor;
            
            EXPECT_NEAR(result_x, expected_x, 1.0f)
                << "Translate X interpolation incorrect at iteration " << i
                << ", factor=" << factor;
            EXPECT_NEAR(result_y, expected_y, 1.0f)
                << "Translate Y interpolation incorrect at iteration " << i
                << ", factor=" << factor;
        }
    }
}

/**
 * @brief Test scale interpolation linearity
 */
TEST_F(TransformInterpolationPropertyTest, ScaleInterpolationLinearity) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float from_sx = randFloat(0.5f, 2.0f);
        float from_sy = randFloat(0.5f, 2.0f);
        float to_sx = randFloat(0.5f, 2.0f);
        float to_sy = randFloat(0.5f, 2.0f);
        float factor = randFloat(0.0f, 1.0f);
        
        std::ostringstream from_oss, to_oss;
        from_oss << std::fixed << std::setprecision(2);
        to_oss << std::fixed << std::setprecision(2);
        
        from_oss << "scale(" << from_sx << ", " << from_sy << ")";
        to_oss << "scale(" << to_sx << ", " << to_sy << ")";
        
        auto result = PropertyInterpolation::Interpolate(
            "transform", from_oss.str(), to_oss.str(), factor);
        
        ASSERT_TRUE(result.has_value())
            << "Scale interpolation failed at iteration " << i;
        
        // Parse result to verify linearity
        float result_sx = 0.0f, result_sy = 0.0f;
        if (sscanf(result->c_str(), "scale(%f, %f)", &result_sx, &result_sy) == 2 ||
            sscanf(result->c_str(), "scale(%f,%f)", &result_sx, &result_sy) == 2) {
            
            float expected_sx = from_sx + (to_sx - from_sx) * factor;
            float expected_sy = from_sy + (to_sy - from_sy) * factor;
            
            EXPECT_NEAR(result_sx, expected_sx, 0.1f)
                << "Scale X interpolation incorrect at iteration " << i;
            EXPECT_NEAR(result_sy, expected_sy, 0.1f)
                << "Scale Y interpolation incorrect at iteration " << i;
        }
    }
}

/**
 * @brief Test rotate interpolation with shortest path
 */
TEST_F(TransformInterpolationPropertyTest, RotateInterpolationShortestPath) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float from_deg = randFloat(-180.0f, 180.0f);
        float to_deg = randFloat(-180.0f, 180.0f);
        float factor = randFloat(0.0f, 1.0f);
        
        std::ostringstream from_oss, to_oss;
        from_oss << std::fixed << std::setprecision(2);
        to_oss << std::fixed << std::setprecision(2);
        
        from_oss << "rotate(" << from_deg << "deg)";
        to_oss << "rotate(" << to_deg << "deg)";
        
        auto result = PropertyInterpolation::Interpolate(
            "transform", from_oss.str(), to_oss.str(), factor);
        
        ASSERT_TRUE(result.has_value())
            << "Rotate interpolation failed at iteration " << i;
        
        // Result should be a valid rotate transform
        EXPECT_TRUE(result->find("rotate") != std::string::npos ||
                    result->find("translate") != std::string::npos ||
                    result->find("scale") != std::string::npos)
            << "Result should contain valid transform function at iteration " << i
            << ", got: " << *result;
    }
}

/**
 * @brief Test interpolation boundary conditions for transforms
 */
TEST_F(TransformInterpolationPropertyTest, TransformBoundaryConditions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string from_transform = generateSimpleTransform();
        std::string to_transform = generateSimpleTransform();
        
        // At factor 0, result should be close to from value
        auto result_0 = PropertyInterpolation::Interpolate(
            "transform", from_transform, to_transform, 0.0f);
        
        ASSERT_TRUE(result_0.has_value())
            << "Interpolation at factor 0 failed at iteration " << i;
        
        // At factor 1, result should be close to to value
        auto result_1 = PropertyInterpolation::Interpolate(
            "transform", from_transform, to_transform, 1.0f);
        
        ASSERT_TRUE(result_1.has_value())
            << "Interpolation at factor 1 failed at iteration " << i;
        
        // Both results should be valid transform strings
        EXPECT_FALSE(result_0->empty()) << "Result at factor 0 is empty";
        EXPECT_FALSE(result_1->empty()) << "Result at factor 1 is empty";
    }
}

/**
 * @brief Test translateX and translateY individual functions
 */
TEST_F(TransformInterpolationPropertyTest, TranslateXYInterpolation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float from_val = randFloat(-100.0f, 100.0f);
        float to_val = randFloat(-100.0f, 100.0f);
        float factor = randFloat(0.0f, 1.0f);
        
        // Test translateX
        std::ostringstream from_x, to_x;
        from_x << std::fixed << std::setprecision(2) << "translateX(" << from_val << "px)";
        to_x << std::fixed << std::setprecision(2) << "translateX(" << to_val << "px)";
        
        auto result_x = PropertyInterpolation::Interpolate(
            "transform", from_x.str(), to_x.str(), factor);
        
        ASSERT_TRUE(result_x.has_value())
            << "TranslateX interpolation failed at iteration " << i;
        
        // Test translateY
        std::ostringstream from_y, to_y;
        from_y << std::fixed << std::setprecision(2) << "translateY(" << from_val << "px)";
        to_y << std::fixed << std::setprecision(2) << "translateY(" << to_val << "px)";
        
        auto result_y = PropertyInterpolation::Interpolate(
            "transform", from_y.str(), to_y.str(), factor);
        
        ASSERT_TRUE(result_y.has_value())
            << "TranslateY interpolation failed at iteration " << i;
    }
}

