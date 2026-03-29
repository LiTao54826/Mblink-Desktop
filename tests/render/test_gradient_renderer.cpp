/**
 * @file test_gradient_renderer.cpp
 * @brief 渐变渲染器测试
 */

#include <gtest/gtest.h>
#include "render/utils/gradient_renderer.h"
#include "render/css/css_value.h"

namespace mbink {
namespace test {

class GradientRendererTest : public ::testing::Test {};

// ========== 线性渐变解析测试 ==========

TEST_F(GradientRendererTest, ParseLinearGradientSimple) {
    auto gradient = CSSValue::ParseLinearGradient("linear-gradient(red, blue)");
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_EQ(gradient->stops.size(), 2);
    }
}

TEST_F(GradientRendererTest, ParseLinearGradientWithAngle) {
    auto gradient = CSSValue::ParseLinearGradient("linear-gradient(45deg, red, blue)");
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_FLOAT_EQ(gradient->angle, 45.0f);
    }
}

TEST_F(GradientRendererTest, ParseLinearGradientWithStops) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(red 0%, yellow 50%, blue 100%)"
    );
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_EQ(gradient->stops.size(), 3);
        EXPECT_FLOAT_EQ(gradient->stops[0].position, 0.0f);
        EXPECT_FLOAT_EQ(gradient->stops[1].position, 0.5f);
        EXPECT_FLOAT_EQ(gradient->stops[2].position, 1.0f);
    }
}

TEST_F(GradientRendererTest, ParseLinearGradientMultipleColors) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(red, orange, yellow, green, blue)"
    );
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_GE(gradient->stops.size(), 5);
    }
}

// ========== 径向渐变解析测试 ==========

TEST_F(GradientRendererTest, ParseRadialGradientSimple) {
    auto gradient = CSSValue::ParseRadialGradient("radial-gradient(red, blue)");
    
    EXPECT_TRUE(gradient.has_value());
}

TEST_F(GradientRendererTest, ParseRadialGradientWithCircle) {
    auto gradient = CSSValue::ParseRadialGradient("radial-gradient(circle, red, blue)");
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_TRUE(gradient->is_circle);
    }
}

TEST_F(GradientRendererTest, ParseRadialGradientWithEllipse) {
    auto gradient = CSSValue::ParseRadialGradient("radial-gradient(ellipse, red, blue)");
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_FALSE(gradient->is_circle);
    }
}

TEST_F(GradientRendererTest, ParseRadialGradientWithPosition) {
    auto gradient = CSSValue::ParseRadialGradient(
        "radial-gradient(circle at 50% 50%, red, blue)"
    );
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient) {
        EXPECT_FLOAT_EQ(gradient->center_x, 0.5f);
        EXPECT_FLOAT_EQ(gradient->center_y, 0.5f);
    }
}

// ========== 颜色停止点测试 ==========

TEST_F(GradientRendererTest, ColorStopAutoDistribution) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(red, yellow, blue)"
    );
    
    EXPECT_TRUE(gradient.has_value());
    if (gradient && gradient->stops.size() >= 3) {
        EXPECT_FLOAT_EQ(gradient->stops[0].position, 0.0f);
        EXPECT_FLOAT_EQ(gradient->stops[1].position, 0.5f);
        EXPECT_FLOAT_EQ(gradient->stops[2].position, 1.0f);
    }
}

// ========== 边界情况测试 ==========

TEST_F(GradientRendererTest, ParseInvalidGradient) {
    auto gradient = CSSValue::ParseLinearGradient("invalid-gradient");
    EXPECT_FALSE(gradient.has_value());
}

TEST_F(GradientRendererTest, ParseEmptyGradient) {
    auto gradient = CSSValue::ParseLinearGradient("");
    EXPECT_FALSE(gradient.has_value());
}

// ========== CSSGradientStop 测试 ==========

TEST_F(GradientRendererTest, GradientStopConstruction) {
    CSSGradientStop stop;
    EXPECT_EQ(stop.color, SK_ColorBLACK);
    EXPECT_FLOAT_EQ(stop.position, 0.0f);
    
    CSSGradientStop stop2(SK_ColorRED, 0.5f);
    EXPECT_EQ(stop2.color, SK_ColorRED);
    EXPECT_FLOAT_EQ(stop2.position, 0.5f);
}

// ========== CSSLinearGradient 测试 ==========

TEST_F(GradientRendererTest, LinearGradientConstruction) {
    CSSLinearGradient gradient;
    EXPECT_FLOAT_EQ(gradient.angle, 0.0f);
    EXPECT_TRUE(gradient.stops.empty());
}

// ========== CSSRadialGradient 测试 ==========

TEST_F(GradientRendererTest, RadialGradientConstruction) {
    CSSRadialGradient gradient;
    EXPECT_FLOAT_EQ(gradient.center_x, 0.5f);
    EXPECT_FLOAT_EQ(gradient.center_y, 0.5f);
    EXPECT_TRUE(gradient.is_circle);
    EXPECT_TRUE(gradient.stops.empty());
}

} // namespace test
} // namespace mbink
