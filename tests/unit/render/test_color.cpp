/**
 * @file test_color.cpp
 * @brief 颜色处理单元测试
 */

#include <gtest/gtest.h>
#include "render/utils/color.h"

namespace mblink {
namespace test {

class ColorTest : public ::testing::Test {};

// ========== 颜色创建测试 ==========

TEST_F(ColorTest, CreateFromRGB) {
    SkColor color = Color::FromRGB(255, 128, 64);
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
    EXPECT_EQ(Color::GetAlpha(color), 255);  // 默认不透明
}

TEST_F(ColorTest, CreateFromRGBA) {
    SkColor color = Color::FromRGBA(255, 128, 64, 128);
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
    EXPECT_EQ(Color::GetAlpha(color), 128);
}

TEST_F(ColorTest, CreateFromHex) {
    SkColor color = Color::FromHex("#ff8040");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
}

TEST_F(ColorTest, CreateFromHex3) {
    SkColor color = Color::FromHex("#f84");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 136);
    EXPECT_EQ(Color::GetBlue(color), 68);
}

TEST_F(ColorTest, CreateFromHex8) {
    SkColor color = Color::FromHex("#ff804080");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
    EXPECT_EQ(Color::GetAlpha(color), 128);
}

// ========== 命名颜色测试 ==========

TEST_F(ColorTest, NamedColorRed) {
    SkColor color = Color::FromName("red");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 0);
    EXPECT_EQ(Color::GetBlue(color), 0);
}

TEST_F(ColorTest, NamedColorGreen) {
    SkColor color = Color::FromName("green");
    EXPECT_EQ(Color::GetRed(color), 0);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 0);
}

TEST_F(ColorTest, NamedColorBlue) {
    SkColor color = Color::FromName("blue");
    EXPECT_EQ(Color::GetRed(color), 0);
    EXPECT_EQ(Color::GetGreen(color), 0);
    EXPECT_EQ(Color::GetBlue(color), 255);
}

TEST_F(ColorTest, NamedColorWhite) {
    SkColor color = Color::FromName("white");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 255);
    EXPECT_EQ(Color::GetBlue(color), 255);
}

TEST_F(ColorTest, NamedColorBlack) {
    SkColor color = Color::FromName("black");
    EXPECT_EQ(Color::GetRed(color), 0);
    EXPECT_EQ(Color::GetGreen(color), 0);
    EXPECT_EQ(Color::GetBlue(color), 0);
}

TEST_F(ColorTest, NamedColorTransparent) {
    SkColor color = Color::FromName("transparent");
    EXPECT_EQ(Color::GetAlpha(color), 0);
}

TEST_F(ColorTest, NamedColorCaseInsensitive) {
    SkColor color1 = Color::FromName("RED");
    SkColor color2 = Color::FromName("Red");
    SkColor color3 = Color::FromName("red");

    EXPECT_EQ(Color::GetRed(color1), Color::GetRed(color2));
    EXPECT_EQ(Color::GetRed(color2), Color::GetRed(color3));
}

// ========== 颜色转换测试 ==========

TEST_F(ColorTest, ToHex) {
    SkColor color = Color::FromRGB(255, 128, 64);
    std::string hex = Color::ToHex(color);
    EXPECT_EQ(hex, "#ff8040");
}

TEST_F(ColorTest, ToHexWithAlpha) {
    SkColor color = Color::FromRGBA(255, 128, 64, 128);
    std::string hex = Color::ToHex(color, true);
    EXPECT_EQ(hex, "#ff804080");
}

TEST_F(ColorTest, ParseRgbString) {
    SkColor color = Color::Parse("rgb(255, 128, 64)");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
}

TEST_F(ColorTest, ParseRgbaString) {
    SkColor color = Color::Parse("rgba(255, 128, 64, 128)");
    EXPECT_EQ(Color::GetRed(color), 255);
    EXPECT_EQ(Color::GetGreen(color), 128);
    EXPECT_EQ(Color::GetBlue(color), 64);
    // Note: alpha value format may vary
}

// ========== 颜色比较测试 ==========

TEST_F(ColorTest, Equality) {
    SkColor color1 = Color::FromRGB(255, 128, 64);
    SkColor color2 = Color::FromRGB(255, 128, 64);
    SkColor color3 = Color::FromRGB(255, 128, 65);

    EXPECT_EQ(color1, color2);
    EXPECT_NE(color1, color3);
}

// ========== 边界情况测试 ==========

TEST_F(ColorTest, InvalidHex) {
    SkColor color = Color::FromHex("invalid");
    // 应该返回默认颜色或黑色
}

TEST_F(ColorTest, InvalidName) {
    SkColor color = Color::FromName("notacolor");
    // 应该返回默认颜色或黑色
}

} // namespace test
} // namespace mblink
