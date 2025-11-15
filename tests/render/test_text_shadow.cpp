/**
 * @file test_text_shadow.cpp
 * @brief Text Shadow 解析和渲染单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/render/css_value.h"
#include "core/render/shadow_renderer.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkFont.h"

using namespace lightui;

// ============================================================================
// CSS 解析测试
// ============================================================================

class TextShadowParseTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TextShadowParseTest, ParseSimpleTextShadow) {
    auto shadows = CSSValue::ParseTextShadow("2px 2px 4px rgba(0,0,0,0.5)");
    
    ASSERT_EQ(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, 2.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, 2.0f);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 4.0f);
}

TEST_F(TextShadowParseTest, ParseTextShadowWithoutBlur) {
    auto shadows = CSSValue::ParseTextShadow("1px 1px black");
    
    ASSERT_EQ(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, 1.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, 1.0f);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 0.0f);
}

TEST_F(TextShadowParseTest, ParseTextShadowWithNegativeOffset) {
    auto shadows = CSSValue::ParseTextShadow("-2px -2px 3px red");
    
    ASSERT_EQ(shadows.size(), 1);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, -2.0f);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, -2.0f);
    EXPECT_FLOAT_EQ(shadows[0].blur_radius, 3.0f);
}

TEST_F(TextShadowParseTest, ParseEmptyTextShadow) {
    auto shadows = CSSValue::ParseTextShadow("");
    EXPECT_EQ(shadows.size(), 0);
}

TEST_F(TextShadowParseTest, ParseNoneTextShadow) {
    auto shadows = CSSValue::ParseTextShadow("none");
    EXPECT_EQ(shadows.size(), 0);
}

// ============================================================================
// 渲染测试
// ============================================================================

class TextShadowRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的 Surface 和 Canvas
        SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
        surface_ = SkSurfaces::Raster(info);
        canvas_ = surface_->getCanvas();
        
        // 创建字体
        font_.setSize(24.0f);
    }

    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
    SkFont font_;
};

TEST_F(TextShadowRenderTest, RenderTextWithSingleShadow) {
    std::vector<CSSTextShadow> shadows;
    CSSTextShadow shadow;
    shadow.offset_x = 2.0f;
    shadow.offset_y = 2.0f;
    shadow.blur_radius = 4.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);
    shadows.push_back(shadow);

    // 应该不会崩溃
    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Hello Shadow", font_,
            100.0f, 100.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(TextShadowRenderTest, RenderTextWithMultipleShadows) {
    std::vector<CSSTextShadow> shadows;
    
    // 第一个阴影
    CSSTextShadow shadow1;
    shadow1.offset_x = 2.0f;
    shadow1.offset_y = 2.0f;
    shadow1.blur_radius = 4.0f;
    shadow1.color = SkColorSetARGB(128, 0, 0, 0);
    shadows.push_back(shadow1);
    
    // 第二个阴影
    CSSTextShadow shadow2;
    shadow2.offset_x = -1.0f;
    shadow2.offset_y = -1.0f;
    shadow2.blur_radius = 2.0f;
    shadow2.color = SkColorSetARGB(128, 255, 255, 255);
    shadows.push_back(shadow2);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Multiple Shadows", font_,
            100.0f, 150.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(TextShadowRenderTest, RenderTextWithNoShadow) {
    std::vector<CSSTextShadow> shadows;

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "No Shadow", font_,
            100.0f, 200.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(TextShadowRenderTest, RenderTextWithNoBlur) {
    std::vector<CSSTextShadow> shadows;
    CSSTextShadow shadow;
    shadow.offset_x = 1.0f;
    shadow.offset_y = 1.0f;
    shadow.blur_radius = 0.0f;
    shadow.color = SK_ColorGRAY;
    shadows.push_back(shadow);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Sharp Shadow", font_,
            100.0f, 250.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(TextShadowRenderTest, RenderEmptyText) {
    std::vector<CSSTextShadow> shadows;
    CSSTextShadow shadow;
    shadow.offset_x = 2.0f;
    shadow.offset_y = 2.0f;
    shadow.blur_radius = 4.0f;
    shadow.color = SK_ColorBLACK;
    shadows.push_back(shadow);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "", font_,
            100.0f, 300.0f,
            SK_ColorBLACK, shadows
        );
    });
}

// ============================================================================
// 集成测试
// ============================================================================

TEST_F(TextShadowRenderTest, ParseAndRenderIntegration) {
    // 解析 CSS
    auto shadows = CSSValue::ParseTextShadow("2px 2px 4px rgba(0,0,0,0.5)");
    
    ASSERT_EQ(shadows.size(), 1);
    
    // 渲染
    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Integrated Test", font_,
            100.0f, 350.0f,
            SK_ColorBLACK, shadows
        );
    });
}

// ============================================================================
// 性能测试
// ============================================================================

TEST_F(TextShadowRenderTest, PerformanceTest) {
    std::vector<CSSTextShadow> shadows;
    
    // 创建 5 个阴影
    for (int i = 0; i < 5; i++) {
        CSSTextShadow shadow;
        shadow.offset_x = static_cast<float>(i + 1);
        shadow.offset_y = static_cast<float>(i + 1);
        shadow.blur_radius = static_cast<float>(i * 2);
        shadow.color = SkColorSetARGB(128, 0, 0, 0);
        shadows.push_back(shadow);
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    // 渲染 100 次
    for (int i = 0; i < 100; i++) {
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Performance Test Text", font_,
            100.0f, 400.0f,
            SK_ColorBLACK, shadows
        );
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 应该在 1 秒内完成
    EXPECT_LT(duration.count(), 1000);
}

