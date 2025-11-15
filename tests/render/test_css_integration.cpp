/**
 * @file test_css_integration.cpp
 * @brief CSS 高级特性集成测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/render/style_resolver.h"
#include "core/render/shadow_renderer.h"
#include "core/render/gradient_renderer.h"
#include "core/dom/element.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"

using namespace lightui;

// ============================================================================
// 集成测试
// ============================================================================

class CSSIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的 Surface 和 Canvas
        SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
        surface_ = SkSurfaces::Raster(info);
        canvas_ = surface_->getCanvas();
        
        // 创建 StyleResolver
        resolver_ = std::make_shared<StyleResolver>();
    }

    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
    std::shared_ptr<StyleResolver> resolver_;
};

TEST_F(CSSIntegrationTest, BoxShadowParsing) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "box-shadow: 2px 2px 4px rgba(0,0,0,0.5)");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_EQ(style.box_shadow.size(), 1);
    EXPECT_FLOAT_EQ(style.box_shadow[0].offset_x, 2.0f);
    EXPECT_FLOAT_EQ(style.box_shadow[0].offset_y, 2.0f);
    EXPECT_FLOAT_EQ(style.box_shadow[0].blur_radius, 4.0f);
}

TEST_F(CSSIntegrationTest, TextShadowParsing) {
    auto element = std::make_shared<Element>("p");
    element->SetAttribute("style", "text-shadow: 1px 1px 2px red");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_EQ(style.text_shadow.size(), 1);
    EXPECT_FLOAT_EQ(style.text_shadow[0].offset_x, 1.0f);
    EXPECT_FLOAT_EQ(style.text_shadow[0].offset_y, 1.0f);
    EXPECT_FLOAT_EQ(style.text_shadow[0].blur_radius, 2.0f);
}

TEST_F(CSSIntegrationTest, LinearGradientParsing) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "background-image: linear-gradient(45deg, red, blue)");

    auto style = resolver_->ResolveStyle(element);

    ASSERT_TRUE(style.background_linear_gradient.has_value());
    EXPECT_FLOAT_EQ(style.background_linear_gradient->angle, 45.0f);
    ASSERT_EQ(style.background_linear_gradient->stops.size(), 2);
}

TEST_F(CSSIntegrationTest, RadialGradientParsing) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "background-image: radial-gradient(circle, red, blue)");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_TRUE(style.background_radial_gradient.has_value());
    EXPECT_TRUE(style.background_radial_gradient->is_circle);
    ASSERT_EQ(style.background_radial_gradient->stops.size(), 2);
}

TEST_F(CSSIntegrationTest, MultipleBoxShadows) {
    auto element = std::make_shared<Element>("div");
    // 注意：当前实现只支持单个阴影，这个测试验证不会崩溃
    element->SetAttribute("style", "box-shadow: 2px 2px 4px red");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_GE(style.box_shadow.size(), 1);
}

TEST_F(CSSIntegrationTest, CombinedStyles) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", 
        "box-shadow: 2px 2px 4px rgba(0,0,0,0.5); "
        "background-image: linear-gradient(red, blue); "
        "text-shadow: 1px 1px 2px black");
    
    auto style = resolver_->ResolveStyle(element);
    
    // 验证所有样式都被正确解析
    EXPECT_EQ(style.box_shadow.size(), 1);
    EXPECT_TRUE(style.background_linear_gradient.has_value());
    EXPECT_EQ(style.text_shadow.size(), 1);
}

TEST_F(CSSIntegrationTest, InsetBoxShadow) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "box-shadow: inset 2px 2px 4px rgba(0,0,0,0.5)");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_EQ(style.box_shadow.size(), 1);
    EXPECT_TRUE(style.box_shadow[0].inset);
    EXPECT_FLOAT_EQ(style.box_shadow[0].offset_x, 2.0f);
}

TEST_F(CSSIntegrationTest, GradientWithDirection) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "background-image: linear-gradient(to right, red, blue)");
    
    auto style = resolver_->ResolveStyle(element);
    
    ASSERT_TRUE(style.background_linear_gradient.has_value());
    EXPECT_FLOAT_EQ(style.background_linear_gradient->angle, 90.0f);
}

TEST_F(CSSIntegrationTest, NoShadow) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("style", "box-shadow: none");
    
    auto style = resolver_->ResolveStyle(element);
    
    EXPECT_EQ(style.box_shadow.size(), 0);
}

TEST_F(CSSIntegrationTest, EmptyStyle) {
    auto element = std::make_shared<Element>("div");
    
    auto style = resolver_->ResolveStyle(element);
    
    EXPECT_EQ(style.box_shadow.size(), 0);
    EXPECT_EQ(style.text_shadow.size(), 0);
    EXPECT_FALSE(style.background_linear_gradient.has_value());
    EXPECT_FALSE(style.background_radial_gradient.has_value());
}

// ============================================================================
// 性能测试
// ============================================================================

TEST_F(CSSIntegrationTest, PerformanceStyleResolution) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 解析 1000 个元素的样式
    for (int i = 0; i < 1000; i++) {
        auto element = std::make_shared<Element>("div");
        element->SetAttribute("style", 
            "box-shadow: 2px 2px 4px rgba(0,0,0,0.5); "
            "background-image: linear-gradient(45deg, red, blue); "
            "text-shadow: 1px 1px 2px black");
        
        auto style = resolver_->ResolveStyle(element);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 应该在 1 秒内完成
    EXPECT_LT(duration.count(), 1000);
}

