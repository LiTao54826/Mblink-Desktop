/**
 * @file render_example.cpp
 * @brief LightUI 渲染引擎使用示例
 * 
 * 本示例展示如何使用 LightUI 渲染引擎绘制图形、文本和图片
 */

#include "core/render/renderer.h"
#include "core/render/shapes.h"
#include "core/render/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/image/image_renderer.h"
#include "core/render/color.h"
#include "core/render/paint.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"

using namespace lightui;

int main() {
    // 1. 创建渲染表面（800x600 像素）
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(800, 600);
    if (!surface) {
        return 1;
    }
    
    // 2. 创建渲染器
    Renderer renderer(surface);
    
    // 3. 清空画布为白色
    renderer.Clear(SK_ColorWHITE);
    
    // ========== 绘制图形 ==========
    
    Shapes shapes(renderer.GetCanvas());
    Paint paint;
    paint.SetAntiAlias(true);
    
    // 绘制红色填充矩形
    paint.SetColor(Color::FromRGB(255, 0, 0));
    shapes.FillRect(50, 50, 150, 100, paint);
    
    // 绘制蓝色描边矩形
    paint.SetColor(Color::FromRGB(0, 0, 255));
    paint.SetStrokeWidth(3.0f);
    shapes.StrokeRect(250, 50, 150, 100, paint);
    
    // 绘制绿色圆形
    paint.SetColor(Color::FromRGB(0, 255, 0));
    shapes.FillCircle(125, 250, 50, paint);
    
    // 绘制橙色圆角矩形
    paint.SetColor(Color::FromHex("#FFA500"));
    shapes.FillRoundRect(250, 200, 150, 100, 15, paint);
    
    // 绘制自定义路径（星形）
    PathBuilder star;
    star.MoveTo(500, 100)
        .LineTo(520, 150)
        .LineTo(570, 150)
        .LineTo(530, 180)
        .LineTo(550, 230)
        .LineTo(500, 200)
        .LineTo(450, 230)
        .LineTo(470, 180)
        .LineTo(430, 150)
        .LineTo(480, 150)
        .Close();
    
    paint.SetColor(Color::FromName("gold"));
    shapes.FillPath(star.Build(), paint);
    
    // ========== 绘制文本 ==========
    
    // 初始化字体管理器
    FontManager::GetInstance().Initialize();
    
    TextRenderer text_renderer(renderer.GetCanvas());
    
    // 创建字体
    FontDescriptor font_desc;
    font_desc.family = "Arial";
    font_desc.size = 32.0f;
    font_desc.weight = FontWeight::BOLD;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);
    
    // 绘制标题
    paint.SetColor(SK_ColorBLACK);
    text_renderer.DrawText("LightUI Rendering Engine", 50, 400, font, paint);
    
    // 绘制带下划线的文本
    font_desc.size = 24.0f;
    font_desc.weight = FontWeight::NORMAL;
    SkFont normal_font = FontManager::GetInstance().LoadFont(font_desc);
    
    std::string underlined_text = "Underlined Text";
    float text_width = text_renderer.MeasureTextWidth(underlined_text, normal_font);
    text_renderer.DrawText(underlined_text, 50, 450, normal_font, paint);
    text_renderer.DrawUnderline(50, 450, text_width, paint);
    
    // 绘制多行文本
    std::string long_text = "This is a demonstration of the LightUI rendering engine. "
                           "It supports shapes, text, images, and more!";
    text_renderer.DrawMultilineText(long_text, 50, 500, 700, 30, normal_font, paint);
    
    // ========== 保存结果 ==========
    
    sk_sp<SkImage> image = surface->makeImageSnapshot();
    if (image) {
        SkFILEWStream stream("render_example_output.png");
        SkPngEncoder::Encode(&stream, image.get(), SkPngEncoder::Options());
        
        std::cout << "渲染完成！输出文件：render_example_output.png" << std::endl;
    }
    
    return 0;
}

