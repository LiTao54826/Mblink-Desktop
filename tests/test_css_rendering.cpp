/**
 * @file test_css_rendering.cpp
 * @brief CSS 样式渲染测试
 */

#include "core/render/css_value.h"
#include "core/render/box_renderer.h"
#include "core/render/renderer.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include <iostream>
#include <cassert>
#include <unordered_map>

using namespace lightui;

// ========== CSS 值解析测试 ==========

void TestCSSValueParsing() {
    std::cout << "Testing CSS Value Parsing..." << std::endl;
    
    // 测试长度解析
    {
        CSSLength px = CSSValue::ParseLength("10px");
        assert(px.value == 10.0f && px.unit == CSSUnit::PX);
        
        CSSLength percent = CSSValue::ParseLength("50%");
        assert(percent.value == 50.0f && percent.unit == CSSUnit::PERCENT);
        
        CSSLength em = CSSValue::ParseLength("2em");
        assert(em.value == 2.0f && em.unit == CSSUnit::EM);
        
        CSSLength rem = CSSValue::ParseLength("1.5rem");
        assert(rem.value == 1.5f && rem.unit == CSSUnit::REM);
        
        CSSLength auto_val = CSSValue::ParseLength("auto");
        assert(auto_val.IsAuto());
    }
    
    // 测试边距解析
    {
        // 一个值
        CSSEdges edges1 = CSSValue::ParseEdges("10px");
        assert(edges1.top.value == 10.0f);
        assert(edges1.right.value == 10.0f);
        assert(edges1.bottom.value == 10.0f);
        assert(edges1.left.value == 10.0f);
        
        // 两个值
        CSSEdges edges2 = CSSValue::ParseEdges("10px 20px");
        assert(edges2.top.value == 10.0f);
        assert(edges2.right.value == 20.0f);
        assert(edges2.bottom.value == 10.0f);
        assert(edges2.left.value == 20.0f);
        
        // 四个值
        CSSEdges edges4 = CSSValue::ParseEdges("10px 20px 30px 40px");
        assert(edges4.top.value == 10.0f);
        assert(edges4.right.value == 20.0f);
        assert(edges4.bottom.value == 30.0f);
        assert(edges4.left.value == 40.0f);
    }
    
    // 测试边框样式解析
    {
        assert(CSSValue::ParseBorderStyle("solid") == CSSBorderStyle::SOLID);
        assert(CSSValue::ParseBorderStyle("dashed") == CSSBorderStyle::DASHED);
        assert(CSSValue::ParseBorderStyle("dotted") == CSSBorderStyle::DOTTED);
        assert(CSSValue::ParseBorderStyle("none") == CSSBorderStyle::NONE);
    }
    
    // 测试颜色解析
    {
        SkColor red = CSSValue::ParseColor("red");
        assert(red == SK_ColorRED);
        
        SkColor hex = CSSValue::ParseColor("#FF0000");
        assert(hex == SK_ColorRED);
        
        SkColor rgb = CSSValue::ParseColor("rgb(255, 0, 0)");
        assert(SkColorGetR(rgb) == 255);
        assert(SkColorGetG(rgb) == 0);
        assert(SkColorGetB(rgb) == 0);
    }
    
    std::cout << "✓ CSS Value Parsing tests passed!" << std::endl;
}

// ========== 盒模型计算测试 ==========

void TestBoxComputation() {
    std::cout << "Testing Box Computation..." << std::endl;
    
    std::unordered_map<std::string, std::string> styles;
    styles["padding"] = "10px";
    styles["border-width"] = "2px";
    styles["margin"] = "5px";
    
    Box box = BoxRenderer::ComputeBox(styles, 100, 100, 200, 150);
    
    assert(box.content_x == 100);
    assert(box.content_y == 100);
    assert(box.content_width == 200);
    assert(box.content_height == 150);
    assert(box.padding_top == 10.0f);
    assert(box.padding_right == 10.0f);
    assert(box.padding_bottom == 10.0f);
    assert(box.padding_left == 10.0f);
    assert(box.border_top_width == 2.0f);
    assert(box.margin_top == 5.0f);
    
    std::cout << "✓ Box Computation tests passed!" << std::endl;
}

// ========== 盒模型渲染测试 ==========

void TestBoxRendering() {
    std::cout << "Testing Box Rendering..." << std::endl;

    // 创建渲染表面
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    if (!surface) {
        std::cerr << "Failed to create surface" << std::endl;
        return;
    }
    
    Renderer renderer(surface);
    renderer.Clear(SK_ColorWHITE);
    
    BoxRenderer box_renderer(renderer.GetCanvas());
    
    // 测试 1: 背景颜色
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#FF6B6B";
        styles["padding"] = "20px";
        
        Box box = BoxRenderer::ComputeBox(styles, 50, 50, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 2: 边框
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#4ECDC4";
        styles["border-width"] = "3px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#1A535C";
        styles["padding"] = "15px";
        
        Box box = BoxRenderer::ComputeBox(styles, 250, 50, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 3: 虚线边框
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#FFE66D";
        styles["border-width"] = "2px";
        styles["border-style"] = "dashed";
        styles["border-color"] = "#FF6B6B";
        styles["padding"] = "10px";
        
        Box box = BoxRenderer::ComputeBox(styles, 450, 50, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 4: 点线边框
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#95E1D3";
        styles["border-width"] = "3px";
        styles["border-style"] = "dotted";
        styles["border-color"] = "#38A3A5";
        styles["padding"] = "12px";
        
        Box box = BoxRenderer::ComputeBox(styles, 50, 200, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 5: 双线边框
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#F38181";
        styles["border-width"] = "6px";
        styles["border-style"] = "double";
        styles["border-color"] = "#AA4465";
        styles["padding"] = "15px";
        
        Box box = BoxRenderer::ComputeBox(styles, 250, 200, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 6: 不同边距
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#A8E6CF";
        styles["padding-top"] = "5px";
        styles["padding-right"] = "10px";
        styles["padding-bottom"] = "15px";
        styles["padding-left"] = "20px";
        styles["border-width"] = "2px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#56AB91";
        
        Box box = BoxRenderer::ComputeBox(styles, 450, 200, 150, 100);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 7: 百分比 padding
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "#FFD3B6";
        styles["padding"] = "5%";
        styles["border-width"] = "1px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#FFAAA5";
        
        Box box = BoxRenderer::ComputeBox(styles, 50, 350, 200, 150, 800.0f);
        box_renderer.RenderBox(box, styles);
    }
    
    // 测试 8: 复杂盒模型
    {
        std::unordered_map<std::string, std::string> styles;
        styles["background-color"] = "rgba(100, 150, 200, 0.8)";
        styles["padding"] = "20px 30px";
        styles["border-width"] = "4px";
        styles["border-style"] = "solid";
        styles["border-color"] = "#2C3E50";
        styles["margin"] = "10px";
        
        Box box = BoxRenderer::ComputeBox(styles, 300, 350, 250, 150);
        box_renderer.RenderBox(box, styles);
    }
    
    // 保存结果
    sk_sp<SkImage> image = surface->makeImageSnapshot();
    if (image) {
        SkPixmap pixmap;
        if (image->peekPixels(&pixmap)) {
            SkFILEWStream stream("test_css_rendering.png");
            SkPngEncoder::Encode(&stream, pixmap, SkPngEncoder::Options());
            std::cout << "✓ Box Rendering test completed! Output: test_css_rendering.png" << std::endl;
        }
    }
}

// ========== 主函数 ==========

int main() {
    std::cout << "=== CSS Rendering Tests ===" << std::endl << std::endl;
    
    TestCSSValueParsing();
    std::cout << std::endl;
    
    TestBoxComputation();
    std::cout << std::endl;
    
    TestBoxRendering();
    std::cout << std::endl;
    
    std::cout << "=== All CSS Rendering Tests Passed! ===" << std::endl;
    
    return 0;
}

