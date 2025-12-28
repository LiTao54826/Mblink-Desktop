/**
 * @file test_text_renderer.cpp
 * @brief 文本渲染器测试
 */

#include <gtest/gtest.h>
#include "render/text/text_renderer.h"

namespace lightui {
namespace test {

class TextRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // TextRenderer 需要 SkCanvas，这里测试不需要实际渲染
        // 只测试静态方法和结构体
    }
};

// ========== TextMetrics 测试 ==========

TEST_F(TextRendererTest, TextMetricsDefaultConstruction) {
    TextMetrics metrics;
    
    EXPECT_FLOAT_EQ(metrics.width, 0.0f);
    EXPECT_FLOAT_EQ(metrics.height, 0.0f);
    EXPECT_FLOAT_EQ(metrics.ascent, 0.0f);
    EXPECT_FLOAT_EQ(metrics.descent, 0.0f);
    EXPECT_FLOAT_EQ(metrics.leading, 0.0f);
}

TEST_F(TextRendererTest, TextMetricsModification) {
    TextMetrics metrics;
    metrics.width = 100.0f;
    metrics.height = 20.0f;
    metrics.ascent = 15.0f;
    metrics.descent = 5.0f;
    metrics.leading = 2.0f;
    
    EXPECT_FLOAT_EQ(metrics.width, 100.0f);
    EXPECT_FLOAT_EQ(metrics.height, 20.0f);
    EXPECT_FLOAT_EQ(metrics.ascent, 15.0f);
    EXPECT_FLOAT_EQ(metrics.descent, 5.0f);
    EXPECT_FLOAT_EQ(metrics.leading, 2.0f);
}

// ========== TextAlign 枚举测试 ==========

TEST_F(TextRendererTest, TextAlignValues) {
    EXPECT_EQ(static_cast<int>(TextAlign::LEFT), 0);
    EXPECT_EQ(static_cast<int>(TextAlign::CENTER), 1);
    EXPECT_EQ(static_cast<int>(TextAlign::RIGHT), 2);
    EXPECT_EQ(static_cast<int>(TextAlign::JUSTIFY), 3);
}

// ========== TextDecoration 枚举测试 ==========

TEST_F(TextRendererTest, TextDecorationValues) {
    EXPECT_EQ(static_cast<int>(TextDecoration::NONE), 0);
    EXPECT_EQ(static_cast<int>(TextDecoration::UNDERLINE), 1);
    EXPECT_EQ(static_cast<int>(TextDecoration::LINE_THROUGH), 2);
    EXPECT_EQ(static_cast<int>(TextDecoration::OVERLINE), 4);
}

TEST_F(TextRendererTest, TextDecorationCombination) {
    // 测试位标志组合
    int combined = static_cast<int>(TextDecoration::UNDERLINE) | 
                   static_cast<int>(TextDecoration::LINE_THROUGH);
    EXPECT_EQ(combined, 3);
}

// ========== 静态方法测试 ==========

TEST_F(TextRendererTest, MeasureMixedTextWidthEmpty) {
    SkFont font;
    font.setSize(16);
    
    float width = TextRenderer::MeasureMixedTextWidth("", font);
    EXPECT_FLOAT_EQ(width, 0.0f);
}

TEST_F(TextRendererTest, MeasureMixedTextWidthSimple) {
    SkFont font;
    font.setSize(16);
    
    float width = TextRenderer::MeasureMixedTextWidth("Hello", font);
    EXPECT_GT(width, 0.0f);
}

TEST_F(TextRendererTest, MeasureMixedTextWidthLonger) {
    SkFont font;
    font.setSize(16);
    
    float width1 = TextRenderer::MeasureMixedTextWidth("Hi", font);
    float width2 = TextRenderer::MeasureMixedTextWidth("Hello World", font);
    
    EXPECT_GT(width2, width1);
}

TEST_F(TextRendererTest, MeasureMixedTextWidthDifferentSizes) {
    SkFont font12;
    font12.setSize(12);
    
    SkFont font24;
    font24.setSize(24);
    
    float width12 = TextRenderer::MeasureMixedTextWidth("Test", font12);
    float width24 = TextRenderer::MeasureMixedTextWidth("Test", font24);
    
    EXPECT_GT(width24, width12);
}

// ========== Unicode 文本测试 ==========

TEST_F(TextRendererTest, MeasureMixedTextWidthChinese) {
    SkFont font;
    font.setSize(16);
    
    float width = TextRenderer::MeasureMixedTextWidth("你好", font);
    EXPECT_GT(width, 0.0f);
}

TEST_F(TextRendererTest, MeasureMixedTextWidthMixed) {
    SkFont font;
    font.setSize(16);
    
    float width = TextRenderer::MeasureMixedTextWidth("Hello你好", font);
    EXPECT_GT(width, 0.0f);
}

} // namespace test
} // namespace lightui
