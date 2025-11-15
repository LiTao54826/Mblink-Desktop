/**
 * @file test_shadow_renderer.cpp
 * @brief ShadowRenderer 单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/render/shadow_renderer.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"

using namespace lightui;

class ShadowRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的 Surface 和 Canvas
        SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
        surface_ = SkSurfaces::Raster(info);
        canvas_ = surface_->getCanvas();
    }

    void TearDown() override {
        canvas_ = nullptr;
        surface_ = nullptr;
    }

    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
};

// ============================================================================
// Box Shadow 测试
// ============================================================================

TEST_F(ShadowRendererTest, RenderSingleOutsetShadow) {
    // 测试单个外阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 5.0f;
    shadow.offset_y = 5.0f;
    shadow.blur_radius = 10.0f;
    shadow.spread_radius = 0.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0); // 半透明黑色
    shadow.inset = false;

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    // 应该不会崩溃
    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

TEST_F(ShadowRendererTest, RenderMultipleOutsetShadows) {
    // 测试多个外阴影
    std::vector<CSSBoxShadow> shadows;

    // 第一个阴影：黑色
    CSSBoxShadow shadow1;
    shadow1.offset_x = 5.0f;
    shadow1.offset_y = 5.0f;
    shadow1.blur_radius = 10.0f;
    shadow1.spread_radius = 0.0f;
    shadow1.color = SkColorSetARGB(128, 0, 0, 0);
    shadow1.inset = false;
    shadows.push_back(shadow1);

    // 第二个阴影：红色
    CSSBoxShadow shadow2;
    shadow2.offset_x = -5.0f;
    shadow2.offset_y = -5.0f;
    shadow2.blur_radius = 5.0f;
    shadow2.spread_radius = 2.0f;
    shadow2.color = SkColorSetARGB(128, 255, 0, 0);
    shadow2.inset = false;
    shadows.push_back(shadow2);

    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

TEST_F(ShadowRendererTest, RenderInsetShadow) {
    // 测试内阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 5.0f;
    shadow.offset_y = 5.0f;
    shadow.blur_radius = 10.0f;
    shadow.spread_radius = 0.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);
    shadow.inset = true; // 内阴影

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

TEST_F(ShadowRendererTest, RenderShadowWithBorderRadius) {
    // 测试带圆角的阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 5.0f;
    shadow.offset_y = 5.0f;
    shadow.blur_radius = 10.0f;
    shadow.spread_radius = 0.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);
    shadow.inset = false;

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);
    float border_radius = 10.0f;

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, border_radius);
    });
}

TEST_F(ShadowRendererTest, RenderShadowWithSpread) {
    // 测试带扩展半径的阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 0.0f;
    shadow.offset_y = 0.0f;
    shadow.blur_radius = 10.0f;
    shadow.spread_radius = 5.0f; // 扩展半径
    shadow.color = SkColorSetARGB(128, 0, 0, 255);
    shadow.inset = false;

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

TEST_F(ShadowRendererTest, RenderShadowWithNoBlur) {
    // 测试无模糊的阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 5.0f;
    shadow.offset_y = 5.0f;
    shadow.blur_radius = 0.0f; // 无模糊
    shadow.spread_radius = 0.0f;
    shadow.color = SkColorSetARGB(255, 0, 0, 0);
    shadow.inset = false;

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

TEST_F(ShadowRendererTest, RenderEmptyShadowList) {
    // 测试空阴影列表（应该什么都不做）
    std::vector<CSSBoxShadow> shadows;
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    });
}

// ============================================================================
// Text Shadow 测试
// ============================================================================

TEST_F(ShadowRendererTest, RenderTextWithSingleShadow) {
    // 测试单个文本阴影
    CSSTextShadow shadow;
    shadow.offset_x = 2.0f;
    shadow.offset_y = 2.0f;
    shadow.blur_radius = 4.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);

    std::vector<CSSTextShadow> shadows = {shadow};
    
    SkFont font;
    font.setSize(24.0f);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Hello Shadow", font, 100.0f, 100.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(ShadowRendererTest, RenderTextWithMultipleShadows) {
    // 测试多个文本阴影
    std::vector<CSSTextShadow> shadows;

    CSSTextShadow shadow1;
    shadow1.offset_x = 2.0f;
    shadow1.offset_y = 2.0f;
    shadow1.blur_radius = 4.0f;
    shadow1.color = SkColorSetARGB(128, 0, 0, 0);
    shadows.push_back(shadow1);

    CSSTextShadow shadow2;
    shadow2.offset_x = -2.0f;
    shadow2.offset_y = -2.0f;
    shadow2.blur_radius = 2.0f;
    shadow2.color = SkColorSetARGB(128, 255, 255, 255);
    shadows.push_back(shadow2);

    SkFont font;
    font.setSize(24.0f);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "Multiple Shadows", font, 100.0f, 100.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(ShadowRendererTest, RenderTextWithNoShadow) {
    // 测试无阴影的文本（应该只渲染文本本身）
    std::vector<CSSTextShadow> shadows;
    
    SkFont font;
    font.setSize(24.0f);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "No Shadow", font, 100.0f, 100.0f,
            SK_ColorBLACK, shadows
        );
    });
}

TEST_F(ShadowRendererTest, RenderEmptyText) {
    // 测试空文本（应该什么都不做）
    CSSTextShadow shadow;
    shadow.offset_x = 2.0f;
    shadow.offset_y = 2.0f;
    shadow.blur_radius = 4.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);

    std::vector<CSSTextShadow> shadows = {shadow};
    
    SkFont font;
    font.setSize(24.0f);

    EXPECT_NO_THROW({
        ShadowRenderer::RenderTextWithShadow(
            canvas_, "", font, 100.0f, 100.0f,
            SK_ColorBLACK, shadows
        );
    });
}

// ============================================================================
// 性能测试
// ============================================================================

TEST_F(ShadowRendererTest, PerformanceTest) {
    // 性能测试：渲染100个阴影
    CSSBoxShadow shadow;
    shadow.offset_x = 5.0f;
    shadow.offset_y = 5.0f;
    shadow.blur_radius = 10.0f;
    shadow.spread_radius = 0.0f;
    shadow.color = SkColorSetARGB(128, 0, 0, 0);
    shadow.inset = false;

    std::vector<CSSBoxShadow> shadows = {shadow};
    SkRect rect = SkRect::MakeXYWH(100, 100, 200, 100);

    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        ShadowRenderer::RenderBoxShadow(canvas_, rect, shadows, 0.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 应该在合理时间内完成（< 1秒）
    EXPECT_LT(duration.count(), 1000);
}

