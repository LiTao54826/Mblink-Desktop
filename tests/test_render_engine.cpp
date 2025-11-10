/**
 * @file test_render_engine.cpp
 * @brief 渲染引擎测试
 * 
 * 测试内容：
 * - Renderer 基类
 * - RenderContext 状态管理
 * - Color 颜色管理
 * - Paint 画笔管理
 * - Shapes 图形绘制
 * - TextRenderer 文本渲染
 * - ImageLoader 图片加载
 * - ImageCache 图片缓存
 */

#include <iostream>
#include <cassert>
#include "core/render/renderer.h"
#include "core/render/render_context.h"
#include "core/render/color.h"
#include "core/render/paint.h"
#include "core/render/shapes.h"
#include "core/render/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/image/image_loader.h"
#include "core/render/image/image_cache.h"
#include "core/render/image/image_renderer.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"

using namespace lightui;

// 测试辅助函数
void SaveSurfaceToPNG(sk_sp<SkSurface> surface, const std::string& filename) {
    if (!surface) return;
    
    sk_sp<SkImage> image = surface->makeImageSnapshot();
    if (!image) return;
    
    SkFILEWStream stream(filename.c_str());
    SkPngEncoder::Encode(&stream, image.get(), SkPngEncoder::Options());
    
    std::cout << "Saved: " << filename << std::endl;
}

// 测试 1: Renderer 基类
void TestRenderer() {
    std::cout << "\n=== Test 1: Renderer ===" << std::endl;
    
    // 创建表面
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(800, 600);
    assert(surface != nullptr);
    
    // 创建渲染器
    Renderer renderer(surface);
    assert(renderer.GetCanvas() != nullptr);
    assert(renderer.GetSurface() != nullptr);
    
    // 清空画布
    renderer.Clear(SK_ColorWHITE);
    
    // 状态管理
    int save_count = renderer.Save();
    assert(save_count > 0);
    renderer.Restore();
    
    // 变换操作
    renderer.Translate(100, 100);
    renderer.Scale(1.5f, 1.5f);
    renderer.Rotate(45.0f);
    
    std::cout << "✓ Renderer tests passed" << std::endl;
}

// 测试 2: Color 颜色管理
void TestColor() {
    std::cout << "\n=== Test 2: Color ===" << std::endl;
    
    // RGB 颜色
    SkColor red = Color::FromRGB(255, 0, 0);
    assert(Color::GetRed(red) == 255);
    assert(Color::GetGreen(red) == 0);
    assert(Color::GetBlue(red) == 0);
    
    // RGBA 颜色
    SkColor semi_transparent = Color::FromRGBA(100, 150, 200, 128);
    assert(Color::GetAlpha(semi_transparent) == 128);
    
    // HEX 颜色
    SkColor hex_color = Color::FromHex("#FF5733");
    assert(Color::GetRed(hex_color) == 0xFF);
    
    // 命名颜色
    SkColor blue = Color::FromName("blue");
    assert(blue == SK_ColorBLUE);
    
    // CSS 解析
    SkColor parsed = Color::Parse("rgb(255, 128, 64)");
    assert(Color::GetRed(parsed) == 255);
    
    // 转换为 HEX
    std::string hex = Color::ToHex(SK_ColorRED);
    assert(hex == "#FF0000");
    
    std::cout << "✓ Color tests passed" << std::endl;
}

// 测试 3: Paint 画笔管理
void TestPaint() {
    std::cout << "\n=== Test 3: Paint ===" << std::endl;
    
    Paint paint;
    
    // 颜色设置
    paint.SetColor(SK_ColorRED);
    assert(paint.GetColor() == SK_ColorRED);
    
    // 透明度
    paint.SetAlpha(128);
    assert(paint.GetAlpha() == 128);
    
    // 样式
    paint.SetStyle(PaintStyle::STROKE);
    assert(paint.GetStyle() == PaintStyle::STROKE);
    
    // 描边设置
    paint.SetStrokeWidth(5.0f);
    assert(paint.GetStrokeWidth() == 5.0f);
    
    paint.SetStrokeCap(StrokeCap::ROUND);
    assert(paint.GetStrokeCap() == StrokeCap::ROUND);
    
    // 抗锯齿
    paint.SetAntiAlias(true);
    assert(paint.IsAntiAlias());
    
    std::cout << "✓ Paint tests passed" << std::endl;
}

// 测试 4: Shapes 图形绘制
void TestShapes() {
    std::cout << "\n=== Test 4: Shapes ===" << std::endl;
    
    // 创建表面
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(800, 600);
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    
    Shapes shapes(canvas);
    Paint paint;
    paint.SetAntiAlias(true);
    
    // 绘制矩形
    paint.SetColor(SK_ColorRED);
    shapes.FillRect(50, 50, 100, 80, paint);
    
    paint.SetColor(SK_ColorBLUE);
    paint.SetStrokeWidth(3.0f);
    shapes.StrokeRect(200, 50, 100, 80, paint);
    
    // 绘制圆形
    paint.SetColor(SK_ColorGREEN);
    shapes.FillCircle(100, 200, 40, paint);
    
    paint.SetColor(SK_ColorMAGENTA);
    shapes.StrokeCircle(250, 200, 40, paint);
    
    // 绘制线条
    paint.SetColor(SK_ColorBLACK);
    shapes.DrawLine(50, 300, 300, 300, paint);
    
    // 绘制圆角矩形
    paint.SetColor(Color::FromRGB(255, 165, 0));  // Orange
    shapes.FillRoundRect(50, 350, 100, 80, 10, paint);
    
    // 绘制路径
    PathBuilder path_builder;
    path_builder.MoveTo(400, 50)
                .LineTo(450, 100)
                .LineTo(400, 150)
                .LineTo(350, 100)
                .Close();
    
    paint.SetColor(SK_ColorCYAN);
    shapes.FillPath(path_builder.Build(), paint);
    
    SaveSurfaceToPNG(surface, "test_shapes.png");
    std::cout << "✓ Shapes tests passed" << std::endl;
}

// 测试 5: TextRenderer 文本渲染
void TestTextRenderer() {
    std::cout << "\n=== Test 5: TextRenderer ===" << std::endl;
    
    // 初始化字体管理器
    FontManager::GetInstance().Initialize();
    
    // 创建表面
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(800, 600);
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    
    TextRenderer text_renderer(canvas);
    Paint paint;
    paint.SetColor(SK_ColorBLACK);
    paint.SetAntiAlias(true);
    
    // 获取字体
    FontDescriptor descriptor;
    descriptor.family = "Arial";
    descriptor.size = 24.0f;
    SkFont font = FontManager::GetInstance().LoadFont(descriptor);
    
    // 绘制文本
    text_renderer.DrawText("Hello, LightUI!", 50, 100, font, paint);
    
    // 测量文本
    TextMetrics metrics = text_renderer.MeasureText("Test", font);
    assert(metrics.width > 0);
    assert(metrics.height > 0);
    
    // 绘制多行文本
    std::string long_text = "This is a long text that should wrap to multiple lines when the maximum width is reached.";
    text_renderer.DrawMultilineText(long_text, 50, 200, 400, 30, font, paint);
    
    // 绘制下划线
    float text_width = text_renderer.MeasureTextWidth("Underlined", font);
    text_renderer.DrawText("Underlined", 50, 350, font, paint);
    text_renderer.DrawUnderline(50, 350, text_width, paint);
    
    // 绘制删除线
    text_width = text_renderer.MeasureTextWidth("Strikethrough", font);
    text_renderer.DrawText("Strikethrough", 50, 400, font, paint);
    text_renderer.DrawLineThrough(50, 400, text_width, font, paint);
    
    SaveSurfaceToPNG(surface, "test_text.png");
    std::cout << "✓ TextRenderer tests passed" << std::endl;
}

// 测试 6: ImageCache 图片缓存
void TestImageCache() {
    std::cout << "\n=== Test 6: ImageCache ===" << std::endl;
    
    ImageCache& cache = ImageCache::GetInstance();
    
    // 设置缓存大小
    cache.SetMaxCacheSize(10 * 1024 * 1024);  // 10MB
    assert(cache.GetMaxCacheSize() == 10 * 1024 * 1024);
    
    // 清空缓存
    cache.Clear();
    assert(cache.GetCacheCount() == 0);
    
    // 创建测试图片
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(100, 100);
    sk_sp<SkImage> test_image = surface->makeImageSnapshot();
    
    // 添加到缓存
    cache.Put("test_key", test_image);
    assert(cache.Contains("test_key"));
    assert(cache.GetCacheCount() == 1);
    
    // 从缓存获取
    sk_sp<SkImage> cached = cache.Get("test_key");
    assert(cached != nullptr);
    
    // 移除
    cache.Remove("test_key");
    assert(!cache.Contains("test_key"));
    
    std::cout << "✓ ImageCache tests passed" << std::endl;
}

// 主测试函数
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  LightUI Render Engine Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        TestRenderer();
        TestColor();
        TestPaint();
        TestShapes();
        TestTextRenderer();
        TestImageCache();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  All tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}

