/**
 * @file test_skia_text_simple.cpp
 * @brief 简单的 Skia 文本渲染测试 - 直接保存到 PNG
 */

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include "core/render/text/font_manager.h"
#include <iostream>

using namespace lightui;

int main() {
    std::cout << "=== Skia Text Rendering Test ===" << std::endl;
    
    // 1. 初始化字体管理器
    std::cout << "Initializing FontManager..." << std::endl;
    FontManager::GetInstance().Initialize();
    
    // 2. 创建 Raster Surface (CPU 渲染)
    int width = 800;
    int height = 600;
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    
    if (!surface) {
        std::cerr << "Failed to create surface!" << std::endl;
        return 1;
    }
    std::cout << "Surface created: " << width << "x" << height << std::endl;
    
    // 3. 获取 Canvas
    SkCanvas* canvas = surface->getCanvas();
    if (!canvas) {
        std::cerr << "Failed to get canvas!" << std::endl;
        return 1;
    }
    
    // 4. 清空画布为白色
    canvas->clear(SK_ColorWHITE);
    std::cout << "Canvas cleared to white" << std::endl;
    
    // 5. 创建字体
    FontDescriptor desc;
    desc.family = "Arial";
    desc.size = 48.0f;
    desc.weight = FontWeight::BOLD;
    SkFont font = FontManager::GetInstance().LoadFont(desc);
    std::cout << "Font loaded: Arial, size=" << desc.size << std::endl;
    
    // 6. 创建画笔
    SkPaint paint;
    paint.setColor(SK_ColorBLACK);
    paint.setAntiAlias(true);
    std::cout << "Paint created: black, antialiased" << std::endl;
    
    // 7. 绘制文本
    const char* text = "Hello, Skia!";
    float x = 50.0f;
    float y = 100.0f;
    
    std::cout << "Drawing text: '" << text << "' at (" << x << ", " << y << ")" << std::endl;
    canvas->drawSimpleText(text, strlen(text), SkTextEncoding::kUTF8, x, y, font, paint);
    
    // 8. 绘制更多文本测试
    paint.setColor(SK_ColorRED);
    canvas->drawSimpleText("Red Text", 8, SkTextEncoding::kUTF8, 50, 200, font, paint);
    
    paint.setColor(SK_ColorBLUE);
    desc.size = 32.0f;
    SkFont font2 = FontManager::GetInstance().LoadFont(desc);
    canvas->drawSimpleText("Blue Text (32px)", 16, SkTextEncoding::kUTF8, 50, 300, font2, paint);
    
    // 9. 绘制一个矩形作为参考
    paint.setColor(SK_ColorGREEN);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    canvas->drawRect(SkRect::MakeXYWH(50, 350, 200, 100), paint);
    std::cout << "Drew reference rectangle" << std::endl;
    
    // 10. 保存到 PNG
    std::cout << "Creating image snapshot..." << std::endl;
    sk_sp<SkImage> image = surface->makeImageSnapshot();
    if (!image) {
        std::cerr << "Failed to create image snapshot!" << std::endl;
        return 1;
    }
    
    std::cout << "Encoding to PNG..." << std::endl;
    sk_sp<SkData> data = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!data) {
        std::cerr << "Failed to encode PNG!" << std::endl;
        return 1;
    }
    
    std::cout << "Writing to file..." << std::endl;
    SkFILEWStream stream("test_skia_text_output.png");
    if (!stream.write(data->data(), data->size())) {
        std::cerr << "Failed to write file!" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Success! Output saved to: test_skia_text_output.png" << std::endl;
    std::cout << "Please open the PNG file to verify text rendering." << std::endl;
    
    return 0;
}

