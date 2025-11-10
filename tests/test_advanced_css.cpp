/**
 * @file test_advanced_css.cpp
 * @brief 高级 CSS 样式渲染测试
 */

#include "core/render/box_renderer.h"
#include "core/render/css_value.h"
#include "core/render/color.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include <iostream>
#include <unordered_map>

using namespace lightui;

void TestBorderRadius() {
    std::cout << "Testing Border Radius..." << std::endl;
    
    // 测试圆角解析
    auto radius1 = CSSValue::ParseBorderRadius("10px");
    if (radius1.top_left.value == 10.0f && radius1.top_right.value == 10.0f &&
        radius1.bottom_right.value == 10.0f && radius1.bottom_left.value == 10.0f) {
        std::cout << "✓ Single value border-radius parsed correctly!" << std::endl;
    }
    
    auto radius2 = CSSValue::ParseBorderRadius("10px 20px");
    if (radius2.top_left.value == 10.0f && radius2.top_right.value == 20.0f) {
        std::cout << "✓ Two value border-radius parsed correctly!" << std::endl;
    }
    
    auto radius4 = CSSValue::ParseBorderRadius("5px 10px 15px 20px");
    if (radius4.top_left.value == 5.0f && radius4.top_right.value == 10.0f &&
        radius4.bottom_right.value == 15.0f && radius4.bottom_left.value == 20.0f) {
        std::cout << "✓ Four value border-radius parsed correctly!" << std::endl;
    }
}

void TestBoxShadow() {
    std::cout << "\nTesting Box Shadow..." << std::endl;
    
    // 测试阴影解析
    auto shadows = CSSValue::ParseBoxShadow("2px 2px 4px rgba(0,0,0,0.5)");
    if (!shadows.empty()) {
        const auto& shadow = shadows[0];
        if (shadow.offset_x == 2.0f && shadow.offset_y == 2.0f && shadow.blur_radius == 4.0f) {
            std::cout << "✓ Box shadow parsed correctly!" << std::endl;
        }
    }
    
    auto shadows2 = CSSValue::ParseBoxShadow("inset 0px 0px 10px black");
    if (!shadows2.empty() && shadows2[0].inset) {
        std::cout << "✓ Inset shadow parsed correctly!" << std::endl;
    }
}

void TestGradients() {
    std::cout << "\nTesting Gradients..." << std::endl;
    
    // 测试线性渐变
    auto linear = CSSValue::ParseLinearGradient("linear-gradient(90deg, red, blue)");
    if (linear.has_value() && linear->angle == 90.0f && linear->stops.size() == 2) {
        std::cout << "✓ Linear gradient parsed correctly!" << std::endl;
    }
    
    auto linear2 = CSSValue::ParseLinearGradient("linear-gradient(to right, red, yellow, green)");
    if (linear2.has_value() && linear2->stops.size() == 3) {
        std::cout << "✓ Linear gradient with direction parsed correctly!" << std::endl;
    }
    
    // 测试径向渐变
    auto radial = CSSValue::ParseRadialGradient("radial-gradient(circle, red, blue)");
    if (radial.has_value() && radial->is_circle && radial->stops.size() == 2) {
        std::cout << "✓ Radial gradient parsed correctly!" << std::endl;
    }
}

void TestBackgroundProperties() {
    std::cout << "\nTesting Background Properties..." << std::endl;
    
    // 测试 background-repeat
    auto repeat1 = CSSValue::ParseBackgroundRepeat("no-repeat");
    if (repeat1 == CSSBackgroundRepeat::NO_REPEAT) {
        std::cout << "✓ Background-repeat no-repeat parsed correctly!" << std::endl;
    }
    
    auto repeat2 = CSSValue::ParseBackgroundRepeat("repeat-x");
    if (repeat2 == CSSBackgroundRepeat::REPEAT_X) {
        std::cout << "✓ Background-repeat repeat-x parsed correctly!" << std::endl;
    }
    
    // 测试 background-size
    auto size1 = CSSValue::ParseBackgroundSize("cover");
    if (size1.type == CSSBackgroundSize::Type::COVER) {
        std::cout << "✓ Background-size cover parsed correctly!" << std::endl;
    }
    
    auto size2 = CSSValue::ParseBackgroundSize("contain");
    if (size2.type == CSSBackgroundSize::Type::CONTAIN) {
        std::cout << "✓ Background-size contain parsed correctly!" << std::endl;
    }
    
    auto size3 = CSSValue::ParseBackgroundSize("100px 200px");
    if (size3.type == CSSBackgroundSize::Type::LENGTH && 
        size3.width.value == 100.0f && size3.height.value == 200.0f) {
        std::cout << "✓ Background-size with dimensions parsed correctly!" << std::endl;
    }
}

void TestAdvancedRendering() {
    std::cout << "\nTesting Advanced Rendering..." << std::endl;
    
    // 创建画布
    SkImageInfo info = SkImageInfo::MakeN32Premul(1200, 800);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    SkCanvas* canvas = surface->getCanvas();
    
    // 清空画布
    canvas->clear(SK_ColorWHITE);
    
    BoxRenderer renderer(canvas);
    
    // 测试 1: 圆角边框
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#E3F2FD";
        styles["border-width"] = "3px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#2196F3";
        styles["border-radius"] = "15px";
        
        Box box = BoxRenderer::ComputeBox(styles, 50, 50, 150, 100);
        
        // 解析圆角
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        
        // 渲染背景
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
        
        // 渲染圆角边框
        renderer.RenderRoundedBorder(box, styles["border-width"], 
                                    styles["border-style"], 
                                    styles["border-color"], radius);
    }
    
    // 测试 2: 阴影效果
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#FFF3E0";
        styles["border-width"] = "2px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#FF9800";
        styles["border-radius"] = "10px";
        styles["box-shadow"] = "4px 4px 8px rgba(0,0,0,0.3)";
        
        Box box = BoxRenderer::ComputeBox(styles, 250, 50, 150, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        auto shadows = CSSValue::ParseBoxShadow(styles["box-shadow"]);
        
        // 先渲染阴影
        renderer.RenderBoxShadow(box, shadows, &radius);
        
        // 再渲染背景和边框
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
        renderer.RenderRoundedBorder(box, styles["border-width"], 
                                    styles["border-style"], 
                                    styles["border-color"], radius);
    }
    
    // 测试 3: 线性渐变
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background"] = "linear-gradient(90deg, #FF6B6B, #4ECDC4)";
        styles["border-radius"] = "20px";
        
        Box box = BoxRenderer::ComputeBox(styles, 450, 50, 150, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
    }
    
    // 测试 4: 径向渐变
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background"] = "radial-gradient(circle, #FFE66D, #FF6B6B)";
        styles["border-radius"] = "75px";
        
        Box box = BoxRenderer::ComputeBox(styles, 650, 50, 150, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
    }
    
    // 测试 5: 多色渐变
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background"] = "linear-gradient(to bottom, #667eea, #764ba2, #f093fb)";
        styles["border-radius"] = "15px";
        
        Box box = BoxRenderer::ComputeBox(styles, 850, 50, 150, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
    }
    
    // 测试 6: 大圆角
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#C5E1A5";
        styles["border-width"] = "4px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#7CB342";
        styles["border-radius"] = "50px";
        
        Box box = BoxRenderer::ComputeBox(styles, 1050, 50, 100, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
        renderer.RenderRoundedBorder(box, styles["border-width"], 
                                    styles["border-style"], 
                                    styles["border-color"], radius);
    }
    
    // 测试 7: 不同圆角
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#F8BBD0";
        styles["border-width"] = "3px";
        styles["border-style"] = "dashed";
        styles["border-color"] = "#E91E63";
        styles["border-radius"] = "5px 20px 35px 50px";
        
        Box box = BoxRenderer::ComputeBox(styles, 50, 200, 150, 100);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
        renderer.RenderRoundedBorder(box, styles["border-width"], 
                                    styles["border-style"], 
                                    styles["border-color"], radius);
    }
    
    // 测试 8: 组合效果（渐变 + 阴影 + 圆角）
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background"] = "linear-gradient(135deg, #667eea, #764ba2)";
        styles["border-radius"] = "25px";
        styles["box-shadow"] = "8px 8px 16px rgba(0,0,0,0.4)";
        
        Box box = BoxRenderer::ComputeBox(styles, 250, 200, 200, 150);
        
        CSSBorderRadius radius = CSSValue::ParseBorderRadius(styles["border-radius"]);
        auto shadows = CSSValue::ParseBoxShadow(styles["box-shadow"]);
        
        renderer.RenderBoxShadow(box, shadows, &radius);
        renderer.RenderBackgroundAdvanced(box, styles, &radius);
    }
    
    // 保存图片
    sk_sp<SkImage> image = surface->makeImageSnapshot();
    SkPixmap pixmap;
    if (image->peekPixels(&pixmap)) {
        SkFILEWStream stream("test_advanced_css.png");
        SkPngEncoder::Encode(&stream, pixmap, SkPngEncoder::Options());
        std::cout << "✓ Advanced CSS rendering test completed! Output: test_advanced_css.png" << std::endl;
    }
}

int main() {
    std::cout << "=== Advanced CSS Rendering Tests ===" << std::endl;
    std::cout << std::endl;
    
    TestBorderRadius();
    TestBoxShadow();
    TestGradients();
    TestBackgroundProperties();
    TestAdvancedRendering();
    
    std::cout << std::endl;
    std::cout << "=== All Advanced CSS Tests Passed! ===" << std::endl;
    
    return 0;
}

