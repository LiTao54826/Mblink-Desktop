/**
 * @file test_css_value.cpp
 * @brief CSS 值解析单元测试
 */

#include <gtest/gtest.h>
#include "render/css/css_value.h"

namespace mblink {
namespace test {

class CSSValueTest : public ::testing::Test {};

// ========== 长度值测试 ==========

TEST_F(CSSValueTest, ParsePixels) {
    auto value = CSSValue::ParseLength("100px");
    EXPECT_EQ(value.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(value.value, 100.0f);
}

TEST_F(CSSValueTest, ParseEm) {
    auto value = CSSValue::ParseLength("2em");
    EXPECT_EQ(value.unit, CSSUnit::EM);
    EXPECT_FLOAT_EQ(value.value, 2.0f);
}

TEST_F(CSSValueTest, ParseRem) {
    auto value = CSSValue::ParseLength("1.5rem");
    EXPECT_EQ(value.unit, CSSUnit::REM);
    EXPECT_FLOAT_EQ(value.value, 1.5f);
}

TEST_F(CSSValueTest, ParsePercent) {
    auto value = CSSValue::ParseLength("50%");
    EXPECT_EQ(value.unit, CSSUnit::PERCENT);
    EXPECT_FLOAT_EQ(value.value, 50.0f);
}

TEST_F(CSSValueTest, ParseAuto) {
    auto value = CSSValue::ParseLength("auto");
    EXPECT_TRUE(value.IsAuto());
}

TEST_F(CSSValueTest, ParseNone) {
    auto value = CSSValue::ParseLength("none");
    EXPECT_EQ(value.unit, CSSUnit::NONE);
}

TEST_F(CSSValueTest, ParseZero) {
    auto value = CSSValue::ParseLength("0");
    EXPECT_TRUE(value.IsZero());
    EXPECT_FLOAT_EQ(value.value, 0.0f);
}

TEST_F(CSSValueTest, ParseNegativeValue) {
    auto value = CSSValue::ParseLength("-10px");
    EXPECT_EQ(value.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(value.value, -10.0f);
}

TEST_F(CSSValueTest, ParseFloatValue) {
    auto value = CSSValue::ParseLength("10.5px");
    EXPECT_EQ(value.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(value.value, 10.5f);
}

// ========== 颜色值测试 ==========

TEST_F(CSSValueTest, ParseHexColor6) {
    auto color = CSSValue::ParseColor("#ff0000");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 0);
    EXPECT_EQ(SkColorGetB(color), 0);
}

TEST_F(CSSValueTest, ParseHexColor3) {
    auto color = CSSValue::ParseColor("#f00");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 0);
    EXPECT_EQ(SkColorGetB(color), 0);
}

TEST_F(CSSValueTest, ParseHexColor8) {
    auto color = CSSValue::ParseColor("#ff000080");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetA(color), 128);
}

TEST_F(CSSValueTest, ParseRgbColor) {
    auto color = CSSValue::ParseColor("rgb(255, 128, 64)");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 128);
    EXPECT_EQ(SkColorGetB(color), 64);
}

TEST_F(CSSValueTest, ParseRgbaColor) {
    auto color = CSSValue::ParseColor("rgba(255, 128, 64, 0.5)");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 128);
    EXPECT_EQ(SkColorGetB(color), 64);
    EXPECT_NEAR(SkColorGetA(color), 128, 1);  // 0.5 * 255 ≈ 128
}

TEST_F(CSSValueTest, ParseNamedColor) {
    auto color = CSSValue::ParseColor("red");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 0);
    EXPECT_EQ(SkColorGetB(color), 0);
}

TEST_F(CSSValueTest, ParseTransparent) {
    auto color = CSSValue::ParseColor("transparent");
    EXPECT_EQ(SkColorGetA(color), 0);
}

TEST_F(CSSValueTest, ParseHslColor) {
    auto color = CSSValue::ParseColor("hsl(0, 100%, 50%)");
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 0);
    EXPECT_EQ(SkColorGetB(color), 0);
}

// ========== 数字测试 ==========

TEST_F(CSSValueTest, ParseInteger) {
    auto value = CSSValue::ParseInt("42");
    EXPECT_EQ(value, 42);
}

TEST_F(CSSValueTest, ParseFloat) {
    auto value = CSSValue::ParseFloat("3.14");
    EXPECT_FLOAT_EQ(value, 3.14f);
}

// ========== Calc 表达式测试 ==========

TEST_F(CSSValueTest, ParseCalcExpression) {
    auto value = CSSValue::ParseCalc("calc(100% - 40px)");
    EXPECT_TRUE(value.is_calc);
    EXPECT_FLOAT_EQ(value.calc_percent, 100.0f);
    EXPECT_FLOAT_EQ(value.calc_px, -40.0f);
    EXPECT_FLOAT_EQ(value.ToPx(800.0f), 760.0f);
}

TEST_F(CSSValueTest, ParseCalcAddition) {
    auto value = CSSValue::ParseCalc("calc(50% + 20px)");
    EXPECT_TRUE(value.is_calc);
    EXPECT_FLOAT_EQ(value.calc_percent, 50.0f);
    EXPECT_FLOAT_EQ(value.calc_px, 20.0f);
    EXPECT_FLOAT_EQ(value.ToPx(400.0f), 220.0f);
}

TEST_F(CSSValueTest, ParseMinFunction) {
    auto value = CSSValue::ParseLength("min(100%, 720px)");
    EXPECT_EQ(value.function_type, CSSLength::FunctionType::MIN);
    EXPECT_FLOAT_EQ(value.ToPx(600.0f), 600.0f);
    EXPECT_FLOAT_EQ(value.ToPx(1000.0f), 720.0f);
}

TEST_F(CSSValueTest, ParseMaxFunction) {
    auto value = CSSValue::ParseLength("max(50%, 320px)");
    EXPECT_EQ(value.function_type, CSSLength::FunctionType::MAX);
    EXPECT_FLOAT_EQ(value.ToPx(400.0f), 320.0f);
    EXPECT_FLOAT_EQ(value.ToPx(1000.0f), 500.0f);
}

TEST_F(CSSValueTest, ParseClampFunction) {
    auto value = CSSValue::ParseLength("clamp(200px, 50%, 720px)");
    EXPECT_EQ(value.function_type, CSSLength::FunctionType::CLAMP);
    EXPECT_FLOAT_EQ(value.ToPx(300.0f), 200.0f);
    EXPECT_FLOAT_EQ(value.ToPx(800.0f), 400.0f);
    EXPECT_FLOAT_EQ(value.ToPx(2000.0f), 720.0f);
}

TEST_F(CSSValueTest, ParseNestedMinWithCalc) {
    auto value = CSSValue::ParseLength("min(calc(100% - 40px), 720px)");
    EXPECT_EQ(value.function_type, CSSLength::FunctionType::MIN);
    EXPECT_FLOAT_EQ(value.ToPx(600.0f), 560.0f);
    EXPECT_FLOAT_EQ(value.ToPx(1000.0f), 720.0f);
}

// ========== 边界情况测试 ==========

TEST_F(CSSValueTest, ParseEmptyString) {
    auto value = CSSValue::ParseLength("");
    // Empty string should return a default/zero value
    EXPECT_TRUE(value.IsZero() || value.unit == CSSUnit::PX);
}

TEST_F(CSSValueTest, ParseWhitespace) {
    auto value = CSSValue::ParseLength("   ");
    // Whitespace should be trimmed and treated as empty
    EXPECT_TRUE(value.IsZero() || value.unit == CSSUnit::PX);
}

TEST_F(CSSValueTest, ParseWithWhitespace) {
    auto value = CSSValue::ParseLength("  100px  ");
    EXPECT_EQ(value.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(value.value, 100.0f);
}

} // namespace test
} // namespace mblink
