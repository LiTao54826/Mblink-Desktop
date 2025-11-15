/**
 * @file test_gradient_renderer.cpp
 * @brief Gradient Renderer 单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/render/gradient_renderer.h"
#include "core/render/css_value.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"

using namespace lightui;

// ============================================================================
// CSS 解析测试
// ============================================================================

class GradientParseTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GradientParseTest, ParseSimpleLinearGradient) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(red, blue)"
    );
    
    ASSERT_TRUE(gradient.has_value());
    EXPECT_FLOAT_EQ(gradient->angle, 0.0f); // 默认向下
    ASSERT_EQ(gradient->stops.size(), 2);
}

TEST_F(GradientParseTest, ParseLinearGradientWithAngle) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(45deg, red, blue)"
    );
    
    ASSERT_TRUE(gradient.has_value());
    EXPECT_FLOAT_EQ(gradient->angle, 45.0f);
    ASSERT_EQ(gradient->stops.size(), 2);
}

TEST_F(GradientParseTest, ParseLinearGradientWithDirection) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(to right, red, blue)"
    );
    
    ASSERT_TRUE(gradient.has_value());
    EXPECT_FLOAT_EQ(gradient->angle, 90.0f);
}

TEST_F(GradientParseTest, ParseLinearGradientMultipleStops) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(red, yellow, green, blue)"
    );
    
    ASSERT_TRUE(gradient.has_value());
    ASSERT_EQ(gradient->stops.size(), 4);
}

TEST_F(GradientParseTest, ParseInvalidLinearGradient) {
    auto gradient = CSSValue::ParseLinearGradient("not-a-gradient");
    EXPECT_FALSE(gradient.has_value());
}

// ============================================================================
// 渲染测试
// ============================================================================

class GradientRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的 Surface 和 Canvas
        SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
        surface_ = SkSurfaces::Raster(info);
        canvas_ = surface_->getCanvas();
    }

    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
};

TEST_F(GradientRenderTest, RenderSimpleLinearGradient) {
    CSSLinearGradient gradient;
    gradient.angle = 0.0f;
    
    CSSGradientStop stop1;
    stop1.color = SK_ColorRED;
    stop1.position = 0.0f;
    gradient.stops.push_back(stop1);
    
    CSSGradientStop stop2;
    stop2.color = SK_ColorBLUE;
    stop2.position = 1.0f;
    gradient.stops.push_back(stop2);

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    EXPECT_NO_THROW({
        GradientRenderer::RenderLinearGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderLinearGradientWithAngle) {
    CSSLinearGradient gradient;
    gradient.angle = 45.0f;
    
    CSSGradientStop stop1;
    stop1.color = SK_ColorRED;
    stop1.position = 0.0f;
    gradient.stops.push_back(stop1);
    
    CSSGradientStop stop2;
    stop2.color = SK_ColorBLUE;
    stop2.position = 1.0f;
    gradient.stops.push_back(stop2);

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    EXPECT_NO_THROW({
        GradientRenderer::RenderLinearGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderLinearGradientMultipleStops) {
    CSSLinearGradient gradient;
    gradient.angle = 90.0f;
    
    gradient.stops.push_back({SK_ColorRED, 0.0f});
    gradient.stops.push_back({SK_ColorYELLOW, 0.33f});
    gradient.stops.push_back({SK_ColorGREEN, 0.66f});
    gradient.stops.push_back({SK_ColorBLUE, 1.0f});

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    EXPECT_NO_THROW({
        GradientRenderer::RenderLinearGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderEmptyLinearGradient) {
    CSSLinearGradient gradient;
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    // 应该不会崩溃
    EXPECT_NO_THROW({
        GradientRenderer::RenderLinearGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderSimpleRadialGradient) {
    CSSRadialGradient gradient;
    gradient.center_x = 0.5f;
    gradient.center_y = 0.5f;
    gradient.is_circle = true;
    
    CSSGradientStop stop1;
    stop1.color = SK_ColorRED;
    stop1.position = 0.0f;
    gradient.stops.push_back(stop1);
    
    CSSGradientStop stop2;
    stop2.color = SK_ColorBLUE;
    stop2.position = 1.0f;
    gradient.stops.push_back(stop2);

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    EXPECT_NO_THROW({
        GradientRenderer::RenderRadialGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderRadialGradientEllipse) {
    CSSRadialGradient gradient;
    gradient.center_x = 0.5f;
    gradient.center_y = 0.5f;
    gradient.is_circle = false; // 椭圆
    
    gradient.stops.push_back({SK_ColorRED, 0.0f});
    gradient.stops.push_back({SK_ColorBLUE, 1.0f});

    SkRect rect = SkRect::MakeXYWH(100, 100, 300, 150);

    EXPECT_NO_THROW({
        GradientRenderer::RenderRadialGradient(canvas_, rect, gradient);
    });
}

TEST_F(GradientRenderTest, RenderRadialGradientOffCenter) {
    CSSRadialGradient gradient;
    gradient.center_x = 0.25f;
    gradient.center_y = 0.75f;
    gradient.is_circle = true;
    
    gradient.stops.push_back({SK_ColorRED, 0.0f});
    gradient.stops.push_back({SK_ColorYELLOW, 0.5f});
    gradient.stops.push_back({SK_ColorBLUE, 1.0f});

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    EXPECT_NO_THROW({
        GradientRenderer::RenderRadialGradient(canvas_, rect, gradient);
    });
}

// ============================================================================
// 集成测试
// ============================================================================

TEST_F(GradientRenderTest, ParseAndRenderLinearGradient) {
    auto gradient = CSSValue::ParseLinearGradient(
        "linear-gradient(45deg, red, blue)"
    );
    
    ASSERT_TRUE(gradient.has_value());
    
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);
    
    EXPECT_NO_THROW({
        GradientRenderer::RenderLinearGradient(canvas_, rect, *gradient);
    });
}

// ============================================================================
// 性能测试
// ============================================================================

TEST_F(GradientRenderTest, PerformanceTestLinearGradient) {
    CSSLinearGradient gradient;
    gradient.angle = 45.0f;
    gradient.stops.push_back({SK_ColorRED, 0.0f});
    gradient.stops.push_back({SK_ColorYELLOW, 0.5f});
    gradient.stops.push_back({SK_ColorBLUE, 1.0f});

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    auto start = std::chrono::high_resolution_clock::now();

    // 渲染 100 次
    for (int i = 0; i < 100; i++) {
        GradientRenderer::RenderLinearGradient(canvas_, rect, gradient);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 应该在 1 秒内完成
    EXPECT_LT(duration.count(), 1000);
}

TEST_F(GradientRenderTest, PerformanceTestRadialGradient) {
    CSSRadialGradient gradient;
    gradient.center_x = 0.5f;
    gradient.center_y = 0.5f;
    gradient.is_circle = true;
    gradient.stops.push_back({SK_ColorRED, 0.0f});
    gradient.stops.push_back({SK_ColorYELLOW, 0.5f});
    gradient.stops.push_back({SK_ColorBLUE, 1.0f});

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 200);

    auto start = std::chrono::high_resolution_clock::now();

    // 渲染 100 次
    for (int i = 0; i < 100; i++) {
        GradientRenderer::RenderRadialGradient(canvas_, rect, gradient);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 应该在 1 秒内完成
    EXPECT_LT(duration.count(), 1000);
}

