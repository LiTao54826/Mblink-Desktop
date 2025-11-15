/**
 * @file test_css_filters.cpp
 * @brief CSS 滤镜测试
 */

#include "core/render/css_filters.h"
#include "core/render/style_resolver.h"
#include "core/dom/document.h"
#include <iostream>
#include <cmath>

using namespace lightui;

// 测试计数器
static int total_tests = 0;
static int passed_tests = 0;

// 测试宏
#define TEST(name) \
    std::cout << "\n[TEST] " << name << std::endl

#define ASSERT(condition, message) \
    total_tests++; \
    if (condition) { \
        passed_tests++; \
        std::cout << "  ✓ " << message << std::endl; \
    } else { \
        std::cout << "  ✗ " << message << " (FAILED)" << std::endl; \
    }

// 浮点数比较
bool FloatEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// ========== 滤镜创建测试 ==========

void TestFilterCreation() {
    TEST("CSS Filters - Filter Creation");
    
    // Blur
    auto blur = CSSFilter::Blur(5.0f);
    ASSERT(blur.type == CSSFilterType::Blur, "Blur filter type");
    ASSERT(FloatEqual(blur.value, 5.0f), "Blur radius value");
    
    // Brightness
    auto brightness = CSSFilter::Brightness(1.5f);
    ASSERT(brightness.type == CSSFilterType::Brightness, "Brightness filter type");
    ASSERT(FloatEqual(brightness.amount, 1.5f), "Brightness amount");
    
    // Contrast
    auto contrast = CSSFilter::Contrast(0.8f);
    ASSERT(contrast.type == CSSFilterType::Contrast, "Contrast filter type");
    ASSERT(FloatEqual(contrast.amount, 0.8f), "Contrast amount");
    
    // Grayscale
    auto grayscale = CSSFilter::Grayscale(0.5f);
    ASSERT(grayscale.type == CSSFilterType::Grayscale, "Grayscale filter type");
    ASSERT(FloatEqual(grayscale.amount, 0.5f), "Grayscale amount");
    
    // Sepia
    auto sepia = CSSFilter::Sepia(0.7f);
    ASSERT(sepia.type == CSSFilterType::Sepia, "Sepia filter type");
    ASSERT(FloatEqual(sepia.amount, 0.7f), "Sepia amount");
    
    // Saturate
    auto saturate = CSSFilter::Saturate(1.2f);
    ASSERT(saturate.type == CSSFilterType::Saturate, "Saturate filter type");
    ASSERT(FloatEqual(saturate.amount, 1.2f), "Saturate amount");
    
    // HueRotate
    auto hue_rotate = CSSFilter::HueRotate(90.0f);
    ASSERT(hue_rotate.type == CSSFilterType::HueRotate, "HueRotate filter type");
    ASSERT(FloatEqual(hue_rotate.angle, 90.0f), "HueRotate angle");
    
    // Invert
    auto invert = CSSFilter::Invert(0.6f);
    ASSERT(invert.type == CSSFilterType::Invert, "Invert filter type");
    ASSERT(FloatEqual(invert.amount, 0.6f), "Invert amount");
    
    // Opacity
    auto opacity = CSSFilter::Opacity(0.8f);
    ASSERT(opacity.type == CSSFilterType::Opacity, "Opacity filter type");
    ASSERT(FloatEqual(opacity.amount, 0.8f), "Opacity amount");
    
    // DropShadow
    auto drop_shadow = CSSFilter::DropShadow(2.0f, 3.0f, 4.0f, 0xFF000000);
    ASSERT(drop_shadow.type == CSSFilterType::DropShadow, "DropShadow filter type");
    ASSERT(FloatEqual(drop_shadow.offset_x, 2.0f), "DropShadow offset_x");
    ASSERT(FloatEqual(drop_shadow.offset_y, 3.0f), "DropShadow offset_y");
    ASSERT(FloatEqual(drop_shadow.blur_radius, 4.0f), "DropShadow blur_radius");
}

// ========== 滤镜列表测试 ==========

void TestFilterList() {
    TEST("CSS Filters - Filter List");
    
    CSSFilterList list;
    
    ASSERT(list.IsEmpty(), "Empty list initially");
    ASSERT(list.GetCount() == 0, "Count is 0 initially");
    
    list.AddFilter(CSSFilter::Blur(5.0f));
    ASSERT(!list.IsEmpty(), "Not empty after adding filter");
    ASSERT(list.GetCount() == 1, "Count is 1 after adding one filter");
    
    list.AddFilter(CSSFilter::Brightness(1.2f));
    ASSERT(list.GetCount() == 2, "Count is 2 after adding two filters");
    
    const auto& filters = list.GetFilters();
    ASSERT(filters.size() == 2, "Filters vector size is 2");
    ASSERT(filters[0].type == CSSFilterType::Blur, "First filter is Blur");
    ASSERT(filters[1].type == CSSFilterType::Brightness, "Second filter is Brightness");
    
    list.Clear();
    ASSERT(list.IsEmpty(), "Empty after clear");
    ASSERT(list.GetCount() == 0, "Count is 0 after clear");
}

// ========== 滤镜解析测试 ==========

void TestFilterParsing() {
    TEST("CSS Filters - Filter Parsing");
    
    // 单个滤镜
    auto result1 = CSSFilterParser::Parse("blur(5px)");
    ASSERT(result1.has_value(), "Parse blur(5px)");
    ASSERT(result1->GetCount() == 1, "One filter parsed");
    ASSERT(result1->GetFilters()[0].type == CSSFilterType::Blur, "Blur filter type");
    ASSERT(FloatEqual(result1->GetFilters()[0].value, 5.0f), "Blur radius 5px");
    
    // 多个滤镜
    auto result2 = CSSFilterParser::Parse("blur(5px) brightness(1.2)");
    ASSERT(result2.has_value(), "Parse multiple filters");
    ASSERT(result2->GetCount() == 2, "Two filters parsed");
    
    // brightness 百分比
    auto result3 = CSSFilterParser::Parse("brightness(120%)");
    ASSERT(result3.has_value(), "Parse brightness(120%)");
    ASSERT(FloatEqual(result3->GetFilters()[0].amount, 1.2f), "Brightness 120% = 1.2");
    
    // contrast
    auto result4 = CSSFilterParser::Parse("contrast(0.8)");
    ASSERT(result4.has_value(), "Parse contrast(0.8)");
    ASSERT(result4->GetFilters()[0].type == CSSFilterType::Contrast, "Contrast filter type");
    
    // grayscale
    auto result5 = CSSFilterParser::Parse("grayscale(50%)");
    ASSERT(result5.has_value(), "Parse grayscale(50%)");
    ASSERT(FloatEqual(result5->GetFilters()[0].amount, 0.5f), "Grayscale 50% = 0.5");
    
    // sepia
    auto result6 = CSSFilterParser::Parse("sepia(0.7)");
    ASSERT(result6.has_value(), "Parse sepia(0.7)");
    ASSERT(result6->GetFilters()[0].type == CSSFilterType::Sepia, "Sepia filter type");
    
    // saturate
    auto result7 = CSSFilterParser::Parse("saturate(150%)");
    ASSERT(result7.has_value(), "Parse saturate(150%)");
    ASSERT(FloatEqual(result7->GetFilters()[0].amount, 1.5f), "Saturate 150% = 1.5");
    
    // hue-rotate
    auto result8 = CSSFilterParser::Parse("hue-rotate(90deg)");
    ASSERT(result8.has_value(), "Parse hue-rotate(90deg)");
    ASSERT(FloatEqual(result8->GetFilters()[0].angle, 90.0f), "HueRotate 90deg");
    
    // invert
    auto result9 = CSSFilterParser::Parse("invert(60%)");
    ASSERT(result9.has_value(), "Parse invert(60%)");
    ASSERT(FloatEqual(result9->GetFilters()[0].amount, 0.6f), "Invert 60% = 0.6");
    
    // opacity
    auto result10 = CSSFilterParser::Parse("opacity(0.8)");
    ASSERT(result10.has_value(), "Parse opacity(0.8)");
    ASSERT(result10->GetFilters()[0].type == CSSFilterType::Opacity, "Opacity filter type");
    
    // none
    auto result11 = CSSFilterParser::Parse("none");
    ASSERT(!result11.has_value(), "Parse 'none' returns nullopt");
    
    // empty
    auto result12 = CSSFilterParser::Parse("");
    ASSERT(!result12.has_value(), "Parse empty string returns nullopt");
}

// ========== 角度解析测试 ==========

void TestAngleParsing() {
    TEST("CSS Filters - Angle Parsing");
    
    // deg
    auto result1 = CSSFilterParser::Parse("hue-rotate(90deg)");
    ASSERT(result1.has_value(), "Parse hue-rotate(90deg)");
    ASSERT(FloatEqual(result1->GetFilters()[0].angle, 90.0f), "90deg = 90");
    
    // rad
    auto result2 = CSSFilterParser::Parse("hue-rotate(1.5708rad)");
    ASSERT(result2.has_value(), "Parse hue-rotate(1.5708rad)");
    ASSERT(FloatEqual(result2->GetFilters()[0].angle, 90.0f, 0.1f), "1.5708rad ≈ 90deg");
    
    // turn
    auto result3 = CSSFilterParser::Parse("hue-rotate(0.25turn)");
    ASSERT(result3.has_value(), "Parse hue-rotate(0.25turn)");
    ASSERT(FloatEqual(result3->GetFilters()[0].angle, 90.0f), "0.25turn = 90deg");
}

// ========== 复杂滤镜链测试 ==========

void TestComplexFilterChain() {
    TEST("CSS Filters - Complex Filter Chain");
    
    auto result = CSSFilterParser::Parse(
        "blur(5px) brightness(1.2) contrast(0.9) grayscale(0.3) sepia(0.2)"
    );
    
    ASSERT(result.has_value(), "Parse complex filter chain");
    ASSERT(result->GetCount() == 5, "Five filters parsed");
    
    const auto& filters = result->GetFilters();
    ASSERT(filters[0].type == CSSFilterType::Blur, "Filter 1: Blur");
    ASSERT(filters[1].type == CSSFilterType::Brightness, "Filter 2: Brightness");
    ASSERT(filters[2].type == CSSFilterType::Contrast, "Filter 3: Contrast");
    ASSERT(filters[3].type == CSSFilterType::Grayscale, "Filter 4: Grayscale");
    ASSERT(filters[4].type == CSSFilterType::Sepia, "Filter 5: Sepia");
}

// ========== Skia 滤镜创建测试 ==========

void TestSkiaFilterCreation() {
    TEST("CSS Filters - Skia Filter Creation");
    
    // Blur
    auto blur_filter = CSSFilterRenderer::CreateSkiaFilter(CSSFilter::Blur(5.0f));
    ASSERT(blur_filter != nullptr, "Blur Skia filter created");
    
    // Brightness
    auto brightness_filter = CSSFilterRenderer::CreateSkiaFilter(CSSFilter::Brightness(1.2f));
    ASSERT(brightness_filter != nullptr, "Brightness Skia filter created");
    
    // Contrast
    auto contrast_filter = CSSFilterRenderer::CreateSkiaFilter(CSSFilter::Contrast(0.8f));
    ASSERT(contrast_filter != nullptr, "Contrast Skia filter created");
    
    // Grayscale
    auto grayscale_filter = CSSFilterRenderer::CreateSkiaFilter(CSSFilter::Grayscale(0.5f));
    ASSERT(grayscale_filter != nullptr, "Grayscale Skia filter created");
    
    // Filter chain
    CSSFilterList list;
    list.AddFilter(CSSFilter::Blur(5.0f));
    list.AddFilter(CSSFilter::Brightness(1.2f));
    
    auto chain_filter = list.CreateSkiaFilter();
    ASSERT(chain_filter != nullptr, "Filter chain Skia filter created");
}

// ========== 主函数 ==========

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CSS Filters Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 滤镜创建测试
    TestFilterCreation();
    
    // 滤镜列表测试
    TestFilterList();
    
    // 滤镜解析测试
    TestFilterParsing();
    
    // 角度解析测试
    TestAngleParsing();
    
    // 复杂滤镜链测试
    TestComplexFilterChain();
    
    // Skia 滤镜创建测试
    TestSkiaFilterCreation();
    
    // 输出测试结果
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results: " << passed_tests << "/" << total_tests << " passed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (passed_tests == total_tests) ? 0 : 1;
}

