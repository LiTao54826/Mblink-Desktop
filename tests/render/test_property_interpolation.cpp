#include <gtest/gtest.h>
#include "core/render/property_interpolation.h"
#include <chrono>

using namespace lightui;

// ============================================================================
// 数值插值测试
// ============================================================================

TEST(PropertyInterpolationTest, InterpolateNumberWithPx) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "200px", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "150px");
}

TEST(PropertyInterpolationTest, InterpolateNumberWithPercent) {
    auto result = PropertyInterpolation::Interpolate("width", "50%", "100%", 0.25f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "62.5%");
}

TEST(PropertyInterpolationTest, InterpolateNumberWithEm) {
    auto result = PropertyInterpolation::Interpolate("font-size", "1em", "2em", 0.75f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "1.75em");
}

TEST(PropertyInterpolationTest, InterpolateNumberAtStart) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "200px", 0.0f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "100px");
}

TEST(PropertyInterpolationTest, InterpolateNumberAtEnd) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "200px", 1.0f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "200px");
}

TEST(PropertyInterpolationTest, InterpolateNumberDifferentUnits) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "50%", 0.5f);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// 颜色插值测试
// ============================================================================

TEST(PropertyInterpolationTest, InterpolateColorHex) {
    auto result = PropertyInterpolation::Interpolate("color", "#000000", "#ffffff", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(127, 127, 127)");
}

TEST(PropertyInterpolationTest, InterpolateColorRgb) {
    auto result = PropertyInterpolation::Interpolate("color", "rgb(0, 0, 0)", "rgb(255, 255, 255)", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(127, 127, 127)");
}

TEST(PropertyInterpolationTest, InterpolateColorRgba) {
    auto result = PropertyInterpolation::Interpolate("color", "rgba(0, 0, 0, 0.0)", "rgba(255, 255, 255, 1.0)", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgba(127, 127, 127, 0.50)");
}

TEST(PropertyInterpolationTest, InterpolateColorNamed) {
    auto result = PropertyInterpolation::Interpolate("color", "black", "white", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(127, 127, 127)");
}

TEST(PropertyInterpolationTest, InterpolateColorShortHex) {
    auto result = PropertyInterpolation::Interpolate("color", "#000", "#fff", 0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(127, 127, 127)");
}

TEST(PropertyInterpolationTest, InterpolateColorAtStart) {
    auto result = PropertyInterpolation::Interpolate("color", "#ff0000", "#00ff00", 0.0f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(255, 0, 0)");
}

TEST(PropertyInterpolationTest, InterpolateColorAtEnd) {
    auto result = PropertyInterpolation::Interpolate("color", "#ff0000", "#00ff00", 1.0f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "rgb(0, 255, 0)");
}

// ============================================================================
// 属性映射插值测试
// ============================================================================

TEST(PropertyInterpolationTest, InterpolatePropertiesBasic) {
    std::map<std::string, std::string> from = {
        {"width", "100px"},
        {"color", "#000000"}
    };
    
    std::map<std::string, std::string> to = {
        {"width", "200px"},
        {"color", "#ffffff"}
    };
    
    auto result = PropertyInterpolation::InterpolateProperties(from, to, 0.5f);
    
    EXPECT_EQ(result["width"], "150px");
    EXPECT_EQ(result["color"], "rgb(127, 127, 127)");
}

TEST(PropertyInterpolationTest, InterpolatePropertiesPartial) {
    std::map<std::string, std::string> from = {
        {"width", "100px"}
    };
    
    std::map<std::string, std::string> to = {
        {"width", "200px"},
        {"height", "300px"}
    };
    
    auto result = PropertyInterpolation::InterpolateProperties(from, to, 0.5f);
    
    EXPECT_EQ(result["width"], "150px");
    EXPECT_EQ(result["height"], "300px");
}

TEST(PropertyInterpolationTest, InterpolatePropertiesOnlyFrom) {
    std::map<std::string, std::string> from = {
        {"width", "100px"},
        {"height", "200px"}
    };
    
    std::map<std::string, std::string> to = {
        {"width", "200px"}
    };
    
    auto result = PropertyInterpolation::InterpolateProperties(from, to, 0.5f);
    
    EXPECT_EQ(result["width"], "150px");
    EXPECT_EQ(result["height"], "200px");
}

// ============================================================================
// 不支持插值的属性测试
// ============================================================================

TEST(PropertyInterpolationTest, InterpolateNonInterpolableProperty) {
    auto result = PropertyInterpolation::Interpolate("display", "block", "none", 0.4f);
    ASSERT_TRUE(result.has_value());
    // 不支持插值的属性使用阶跃函数 (factor < 0.5 使用 from)
    EXPECT_EQ(*result, "block");

    result = PropertyInterpolation::Interpolate("display", "block", "none", 0.6f);
    ASSERT_TRUE(result.has_value());
    // factor >= 0.5 使用 to
    EXPECT_EQ(*result, "none");
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST(PropertyInterpolationTest, InterpolateFactorBelowZero) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "200px", -0.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "100px");  // 应该被限制在 0.0
}

TEST(PropertyInterpolationTest, InterpolateFactorAboveOne) {
    auto result = PropertyInterpolation::Interpolate("width", "100px", "200px", 1.5f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "200px");  // 应该被限制在 1.0
}

TEST(PropertyInterpolationTest, InterpolateEmptyString) {
    auto result = PropertyInterpolation::Interpolate("width", "", "200px", 0.5f);
    EXPECT_FALSE(result.has_value());
}

TEST(PropertyInterpolationTest, InterpolateInvalidColor) {
    auto result = PropertyInterpolation::Interpolate("color", "invalid", "#ffffff", 0.5f);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(PropertyInterpolationTest, PerformanceInterpolateNumber) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        PropertyInterpolation::Interpolate("width", "100px", "200px", 0.5f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Interpolate 10000 numbers: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 5000);  // 应该 < 5000ms (MSVC regex 较慢)
}

TEST(PropertyInterpolationTest, PerformanceInterpolateColor) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        PropertyInterpolation::Interpolate("color", "#000000", "#ffffff", 0.5f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Interpolate 10000 colors: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 1000);  // 应该 < 1000ms
}

TEST(PropertyInterpolationTest, PerformanceInterpolateProperties) {
    std::map<std::string, std::string> from = {
        {"width", "100px"},
        {"height", "200px"},
        {"color", "#000000"},
        {"background-color", "#ffffff"}
    };

    std::map<std::string, std::string> to = {
        {"width", "200px"},
        {"height", "300px"},
        {"color", "#ffffff"},
        {"background-color", "#000000"}
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        PropertyInterpolation::InterpolateProperties(from, to, 0.5f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Interpolate 10000 property maps: " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 10000);  // 应该 < 10000ms (MSVC regex 较慢)
}

