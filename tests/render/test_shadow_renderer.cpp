/**
 * @file test_shadow_renderer.cpp
 * @brief 阴影渲染器测试
 */

#include <gtest/gtest.h>
#include "render/shadow_renderer.h"
#include "render/css_value.h"

namespace lightui {
namespace test {

class ShadowRendererTest : public ::testing::Test {};

// ========== box-shadow 解析测试 ==========

TEST_F(ShadowRendererTest, ParseSimpleBoxShadow) {
    auto shadows = CSSValue::ParseBoxShadow("10px 10px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, 10.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, 10.0f);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 0.0f);
    EXPECT_FLOAT_EQ(shadows[0].spread_radius, 0.0f);
}

TEST_F(ShadowRendererTest, ParseBoxShadowWithBlur) {
    auto shadows = CSSValue::ParseBoxShadow("10px 10px 5px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 5.0f);
}

TEST_F(ShadowRendererTest, ParseBoxShadowWithSpread) {
    auto shadows = CSSValue::ParseBoxShadow("10px 10px 5px 2px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 5.0f);
    EXPECT_FLOAT_EQ(shadows[0].spread_radius, 2.0f);
}

TEST_F(ShadowRendererTest, ParseInsetBoxShadow) {
    auto shadows = CSSValue::ParseBoxShadow("inset 10px 10px 5px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_TRUE(shadows[0].inset);
}

TEST_F(ShadowRendererTest, ParseMultipleBoxShadows) {
    auto shadows = CSSValue::ParseBoxShadow(
        "10px 10px 5px black, -5px -5px 3px red"
    );
    
    EXPECT_EQ(shadows.size(), 2);
    if (shadows.size() >= 2) {
        EXPECT_FLOAT_EQ(shadows[0].offset_x, 10.0f);
        EXPECT_FLOAT_EQ(shadows[1].offset_x, -5.0f);
    }
}

TEST_F(ShadowRendererTest, ParseNegativeOffsets) {
    auto shadows = CSSValue::ParseBoxShadow("-10px -10px 5px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, -10.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, -10.0f);
}

// ========== text-shadow 解析测试 ==========

TEST_F(ShadowRendererTest, ParseSimpleTextShadow) {
    auto shadows = CSSValue::ParseTextShadow("2px 2px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, 2.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, 2.0f);
}

TEST_F(ShadowRendererTest, ParseTextShadowWithBlur) {
    auto shadows = CSSValue::ParseTextShadow("2px 2px 3px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 3.0f);
}

TEST_F(ShadowRendererTest, ParseMultipleTextShadows) {
    auto shadows = CSSValue::ParseTextShadow(
        "1px 1px white, -1px -1px black"
    );
    
    EXPECT_EQ(shadows.size(), 2);
}

// ========== CSSBoxShadow 结构测试 ==========

TEST_F(ShadowRendererTest, BoxShadowDefaultConstruction) {
    CSSBoxShadow shadow;
    
    EXPECT_FLOAT_EQ(shadow.offset_x, 0.0f);
    EXPECT_FLOAT_EQ(shadow.offset_y, 0.0f);
    EXPECT_FLOAT_EQ(shadow.blur_radius, 0.0f);
    EXPECT_FLOAT_EQ(shadow.spread_radius, 0.0f);
    EXPECT_EQ(shadow.color, SK_ColorBLACK);
    EXPECT_FALSE(shadow.inset);
}

TEST_F(ShadowRendererTest, BoxShadowModification) {
    CSSBoxShadow shadow;
    shadow.offset_x = 10.0f;
    shadow.offset_y = 20.0f;
    shadow.blur_radius = 5.0f;
    shadow.spread_radius = 2.0f;
    shadow.inset = true;
    
    EXPECT_FLOAT_EQ(shadow.offset_x, 10.0f);
    EXPECT_FLOAT_EQ(shadow.offset_y, 20.0f);
    EXPECT_FLOAT_EQ(shadow.blur_radius, 5.0f);
    EXPECT_FLOAT_EQ(shadow.spread_radius, 2.0f);
    EXPECT_TRUE(shadow.inset);
}

// ========== CSSTextShadow 结构测试 ==========

TEST_F(ShadowRendererTest, TextShadowDefaultConstruction) {
    CSSTextShadow shadow;
    
    EXPECT_FLOAT_EQ(shadow.offset_x, 0.0f);
    EXPECT_FLOAT_EQ(shadow.offset_y, 0.0f);
    EXPECT_FLOAT_EQ(shadow.blur_radius, 0.0f);
    EXPECT_EQ(shadow.color, SK_ColorBLACK);
}

// ========== 边界情况测试 ==========

TEST_F(ShadowRendererTest, ParseEmptyShadow) {
    auto shadows = CSSValue::ParseBoxShadow("");
    EXPECT_TRUE(shadows.empty());
}

TEST_F(ShadowRendererTest, ParseNoneShadow) {
    auto shadows = CSSValue::ParseBoxShadow("none");
    EXPECT_TRUE(shadows.empty());
}

TEST_F(ShadowRendererTest, ZeroBlurShadow) {
    auto shadows = CSSValue::ParseBoxShadow("10px 10px 0px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 0.0f);
}

TEST_F(ShadowRendererTest, LargeBlurShadow) {
    auto shadows = CSSValue::ParseBoxShadow("0px 0px 100px black");
    
    ASSERT_GE(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 100.0f);
}

} // namespace test
} // namespace lightui
