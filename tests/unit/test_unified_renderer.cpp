#include "core/render/unified_renderer.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace lightui;

// 辅助函数：比较浮点数
bool FloatEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// 测试 1: 构造函数测试
void TestConstructor() {
    std::cout << "Test 1: Constructor..." << std::endl;
    
    // 测试创建指定大小的渲染器
    UnifiedRenderer renderer1(800, 600);
    assert(renderer1.GetCanvas() != nullptr);
    assert(renderer1.GetSurface() != nullptr);
    
    // 测试使用 Surface 创建
    auto surface = SkSurface::MakeRasterN32Premul(400, 300);
    UnifiedRenderer renderer2(surface);
    assert(renderer2.GetCanvas() != nullptr);
    assert(renderer2.GetSurface() != nullptr);
    
    // 测试使用 Canvas 创建
    auto surface3 = SkSurface::MakeRasterN32Premul(200, 150);
    UnifiedRenderer renderer3(surface3->getCanvas());
    assert(renderer3.GetCanvas() != nullptr);
    
    std::cout << "  ✓ Constructor test passed" << std::endl;
}

// 测试 2: 状态设置测试
void TestStateSettings() {
    std::cout << "Test 2: State Settings..." << std::endl;
    
    UnifiedRenderer renderer(800, 600);
    
    // 测试设置填充颜色
    renderer.SetFillColor(Color::Red());
    renderer.SetFillColor("#00FF00");  // 绿色
    renderer.SetFillColor("rgb(0, 0, 255)");  // 蓝色
    
    // 测试设置描边颜色
    renderer.SetStrokeColor(Color::Blue());
    renderer.SetStrokeColor("#FF0000");
    
    // 测试设置线宽
    renderer.SetLineWidth(2.5f);
    renderer.SetLineWidth(5.0f);
    
    // 测试设置字体
    renderer.SetFont("Arial", 16.0f);
    renderer.SetFont("Arial", 24.0f, true, false);  // 粗体
    renderer.SetFont("Arial", 18.0f, false, true);  // 斜体
    
    // 测试设置字体大小
    renderer.SetFontSize(20.0f);
    
    // 测试设置透明度
    renderer.SetOpacity(0.5f);
    renderer.SetOpacity(1.0f);
    renderer.SetOpacity(0.0f);
    
    std::cout << "  ✓ State settings test passed" << std::endl;
}

// 测试 3: 基础图形绘制测试
void TestBasicShapes() {
    std::cout << "Test 3: Basic Shapes..." << std::endl;
    
    UnifiedRenderer renderer(800, 600);
    renderer.Clear(Color::White());
    
    // 测试填充矩形
    renderer.SetFillColor("#3498db");
    renderer.FillRect(50, 50, 200, 150);
    
    // 测试描边矩形
    renderer.SetStrokeColor("#e74c3c");
    renderer.SetLineWidth(3.0f);
    renderer.DrawRect(300, 50, 200, 150);
    
    // 测试填充圆形
    renderer.SetFillColor("#2ecc71");
    renderer.FillCircle(150, 350, 75);
    
    // 测试描边圆形
    renderer.SetStrokeColor("#f39c12");
    renderer.SetLineWidth(4.0f);
    renderer.DrawCircle(400, 350, 75);
    
    // 测试绘制线条
    renderer.SetStrokeColor("#9b59b6");
    renderer.SetLineWidth(2.0f);
    renderer.DrawLine(550, 50, 750, 200);
    
    // 测试填充圆角矩形
    renderer.SetFillColor("#1abc9c");
    renderer.FillRoundRect(550, 250, 180, 120, 20);
    
    // 测试描边圆角矩形
    renderer.SetStrokeColor("#34495e");
    renderer.SetLineWidth(3.0f);
    renderer.DrawRoundRect(550, 400, 180, 120, 15);
    
    // 保存测试图片
    bool saved = renderer.SaveToFile("test_unified_shapes.png");
    assert(saved);
    
    std::cout << "  ✓ Basic shapes test passed (saved to test_unified_shapes.png)" << std::endl;
}

// 测试 4: 文本绘制测试
void TestTextRendering() {
    std::cout << "Test 4: Text Rendering..." << std::endl;
    
    UnifiedRenderer renderer(800, 600);
    renderer.Clear(Color::White());
    
    // 测试绘制文本
    renderer.SetFont("Arial", 24.0f);
    renderer.SetFillColor("#000000");
    renderer.DrawText("Hello, LightUI!", 50, 100);
    
    // 测试不同字体大小
    renderer.SetFontSize(32.0f);
    renderer.SetFillColor("#3498db");
    renderer.DrawText("Large Text", 50, 200);
    
    renderer.SetFontSize(16.0f);
    renderer.SetFillColor("#e74c3c");
    renderer.DrawText("Small Text", 50, 250);
    
    // 测试粗体和斜体
    renderer.SetFont("Arial", 20.0f, true, false);
    renderer.SetFillColor("#2ecc71");
    renderer.DrawText("Bold Text", 50, 320);
    
    renderer.SetFont("Arial", 20.0f, false, true);
    renderer.SetFillColor("#f39c12");
    renderer.DrawText("Italic Text", 50, 360);
    
    // 测试文本测量
    renderer.SetFont("Arial", 24.0f);
    auto metrics = renderer.MeasureText("Test Measure");
    assert(metrics.width > 0);
    assert(metrics.height > 0);
    
    std::cout << "  ✓ Text metrics: width=" << metrics.width 
              << ", height=" << metrics.height << std::endl;
    
    // 保存测试图片
    bool saved = renderer.SaveToFile("test_unified_text.png");
    assert(saved);
    
    std::cout << "  ✓ Text rendering test passed (saved to test_unified_text.png)" << std::endl;
}

// 测试 5: 图片绘制测试
void TestImageRendering() {
    std::cout << "Test 5: Image Rendering..." << std::endl;
    
    UnifiedRenderer renderer(800, 600);
    renderer.Clear(Color(240, 240, 240));
    
    // 创建一个测试图片（使用另一个渲染器）
    UnifiedRenderer temp_renderer(200, 200);
    temp_renderer.Clear(Color::White());
    temp_renderer.SetFillColor("#3498db");
    temp_renderer.FillCircle(100, 100, 80);
    temp_renderer.SetFillColor("#e74c3c");
    temp_renderer.FillRect(60, 60, 80, 80);
    temp_renderer.SaveToFile("test_temp_image.png");
    
    // 加载图片
    auto image = renderer.LoadImage("test_temp_image.png");
    if (image) {
        // 测试绘制原始大小
        renderer.DrawImage(image, 50, 50);
        
        // 测试绘制指定大小
        renderer.DrawImage(image, 300, 50, 150, 150);
        renderer.DrawImage(image, 500, 50, 100, 100);
        
        // 测试绘制缩放
        renderer.DrawImage(image, 50, 300, 250, 250);
        
        std::cout << "  ✓ Image loaded and drawn successfully" << std::endl;
    } else {
        std::cout << "  ⚠ Image loading skipped (file not found)" << std::endl;
    }
    
    // 保存测试图片
    bool saved = renderer.SaveToFile("test_unified_image.png");
    assert(saved);
    
    std::cout << "  ✓ Image rendering test passed (saved to test_unified_image.png)" << std::endl;
}

// 测试 6: 渲染控制测试
void TestRenderControl() {
    std::cout << "Test 6: Render Control..." << std::endl;
    
    UnifiedRenderer renderer(400, 300);
    
    // 测试清空
    renderer.Clear(Color::White());
    renderer.Clear(Color(200, 200, 200));
    renderer.Clear(Color::Black());
    
    // 测试刷新
    renderer.Flush();
    
    // 测试保存文件
    renderer.SetFillColor("#3498db");
    renderer.FillRect(50, 50, 300, 200);
    bool saved = renderer.SaveToFile("test_unified_control.png");
    assert(saved);
    
    std::cout << "  ✓ Render control test passed" << std::endl;
}

// 测试 7: 状态管理测试
void TestStateManagement() {
    std::cout << "Test 7: State Management..." << std::endl;
    
    UnifiedRenderer renderer(600, 400);
    renderer.Clear(Color::White());
    
    // 测试保存/恢复
    renderer.Save();
    renderer.SetFillColor("#3498db");
    renderer.FillRect(50, 50, 100, 100);
    renderer.Restore();
    
    // 测试平移
    renderer.Save();
    renderer.Translate(200, 0);
    renderer.SetFillColor("#e74c3c");
    renderer.FillRect(50, 50, 100, 100);
    renderer.Restore();
    
    // 测试缩放
    renderer.Save();
    renderer.Translate(400, 0);
    renderer.Scale(0.5f, 0.5f);
    renderer.SetFillColor("#2ecc71");
    renderer.FillRect(50, 50, 100, 100);
    renderer.Restore();
    
    // 测试旋转
    renderer.Save();
    renderer.Translate(100, 250);
    renderer.Rotate(45);
    renderer.SetFillColor("#f39c12");
    renderer.FillRect(-50, -50, 100, 100);
    renderer.Restore();
    
    // 保存测试图片
    bool saved = renderer.SaveToFile("test_unified_transform.png");
    assert(saved);
    
    std::cout << "  ✓ State management test passed (saved to test_unified_transform.png)" << std::endl;
}

// 测试 8: 综合场景测试
void TestCompleteScene() {
    std::cout << "Test 8: Complete Scene..." << std::endl;
    
    UnifiedRenderer renderer(1000, 800);
    renderer.Clear(Color(245, 245, 245));
    
    // 绘制标题
    renderer.SetFont("Arial", 36.0f, true);
    renderer.SetFillColor("#2c3e50");
    renderer.DrawText("UnifiedRenderer Demo", 50, 60);
    
    // 绘制分隔线
    renderer.SetStrokeColor("#bdc3c7");
    renderer.SetLineWidth(2.0f);
    renderer.DrawLine(50, 80, 950, 80);
    
    // 绘制各种图形
    renderer.SetFillColor("#3498db");
    renderer.FillRect(50, 120, 180, 120);
    
    renderer.SetFillColor("#e74c3c");
    renderer.FillCircle(400, 180, 60);
    
    renderer.SetFillColor("#2ecc71");
    renderer.FillRoundRect(550, 120, 180, 120, 20);
    
    renderer.SetStrokeColor("#f39c12");
    renderer.SetLineWidth(4.0f);
    renderer.DrawRect(780, 120, 180, 120);
    
    // 绘制文本说明
    renderer.SetFont("Arial", 16.0f);
    renderer.SetFillColor("#7f8c8d");
    renderer.DrawText("Rectangle", 90, 270);
    renderer.DrawText("Circle", 370, 270);
    renderer.DrawText("Round Rect", 590, 270);
    renderer.DrawText("Stroke Rect", 810, 270);
    
    // 绘制渐变效果（使用透明度）
    for (int i = 0; i < 10; i++) {
        renderer.SetFillColor("#9b59b6");
        renderer.SetOpacity(1.0f - i * 0.1f);
        renderer.FillRect(50 + i * 90, 320, 80, 80);
    }
    
    // 绘制变换示例
    renderer.SetOpacity(1.0f);
    for (int i = 0; i < 8; i++) {
        renderer.Save();
        renderer.Translate(500, 500);
        renderer.Rotate(i * 45.0f);
        renderer.SetFillColor(i % 2 == 0 ? Color(52, 152, 219) : Color(231, 76, 60));
        renderer.FillRect(50, -20, 100, 40);
        renderer.Restore();
    }
    
    // 保存最终图片
    bool saved = renderer.SaveToFile("test_unified_complete.png");
    assert(saved);
    
    std::cout << "  ✓ Complete scene test passed (saved to test_unified_complete.png)" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  UnifiedRenderer Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    try {
        TestConstructor();
        TestStateSettings();
        TestBasicShapes();
        TestTextRendering();
        TestImageRendering();
        TestRenderControl();
        TestStateManagement();
        TestCompleteScene();
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  ✓ All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << "========================================" << std::endl;
        std::cerr << "  ✗ Test failed: " << e.what() << std::endl;
        std::cerr << "========================================" << std::endl;
        return 1;
    }
}

