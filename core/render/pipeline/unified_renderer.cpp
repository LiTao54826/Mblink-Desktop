#include "core/render/unified_renderer.h"
#include "include/core/SkStream.h"
#include "include/encode/SkPngEncoder.h"
#include <sstream>

namespace mblink {

// ==================== 构造函数 ====================

UnifiedRenderer::UnifiedRenderer(int width, int height)
    : owns_surface_(true) {
    // 创建 Surface
    surface_ = SkSurface::MakeRasterN32Premul(width, height);
    if (!surface_) {
        throw std::runtime_error("Failed to create SkSurface");
    }
    canvas_ = surface_->getCanvas();
    
    // 初始化状态
    fill_paint_.SetColor(SK_ColorBLACK);
    fill_paint_.SetStyle(Paint::Style::Fill);

    stroke_paint_.SetColor(SK_ColorBLACK);
    stroke_paint_.SetStyle(Paint::Style::Stroke);
    stroke_paint_.SetStrokeWidth(1.0f);
    
    // 初始化字体
    font_manager_ = FontManager::GetInstance();
    current_font_.setSize(14.0f);
    
    // 辅助对象延迟创建
}

UnifiedRenderer::UnifiedRenderer(sk_sp<SkSurface> surface)
    : surface_(surface), owns_surface_(false) {
    if (!surface_) {
        throw std::runtime_error("Surface cannot be null");
    }
    canvas_ = surface_->getCanvas();
    
    // 初始化状态
    fill_paint_.SetColor(SK_ColorBLACK);
    fill_paint_.SetStyle(Paint::Style::Fill);

    stroke_paint_.SetColor(SK_ColorBLACK);
    stroke_paint_.SetStyle(Paint::Style::Stroke);
    stroke_paint_.SetStrokeWidth(1.0f);
    
    // 初始化字体
    font_manager_ = FontManager::GetInstance();
    current_font_.setSize(14.0f);
}

UnifiedRenderer::UnifiedRenderer(SkCanvas* canvas)
    : canvas_(canvas), owns_surface_(false) {
    if (!canvas_) {
        throw std::runtime_error("Canvas cannot be null");
    }
    
    // 初始化状态
    fill_paint_.SetColor(SK_ColorBLACK);
    fill_paint_.SetStyle(Paint::Style::Fill);

    stroke_paint_.SetColor(SK_ColorBLACK);
    stroke_paint_.SetStyle(Paint::Style::Stroke);
    stroke_paint_.SetStrokeWidth(1.0f);
    
    // 初始化字体
    font_manager_ = FontManager::GetInstance();
    current_font_.setSize(14.0f);
}

// ==================== 状态设置方法 ====================

void UnifiedRenderer::SetFillColor(const Color& color) {
    fill_paint_.SetColor(color);
}

void UnifiedRenderer::SetFillColor(const std::string& color_str) {
    auto color = Color::Parse(color_str);
    fill_paint_.SetColor(color);
}

void UnifiedRenderer::SetStrokeColor(const Color& color) {
    stroke_paint_.SetColor(color);
}

void UnifiedRenderer::SetStrokeColor(const std::string& color_str) {
    auto color = Color::Parse(color_str);
    stroke_paint_.SetColor(color);
}

void UnifiedRenderer::SetLineWidth(float width) {
    stroke_paint_.SetStrokeWidth(width);
}

void UnifiedRenderer::SetFont(const std::string& family, float size, bool bold, bool italic) {
    FontDescriptor desc;
    desc.family = family;
    desc.size = size;
    desc.weight = bold ? FontWeight::Bold : FontWeight::Normal;
    desc.style = italic ? FontStyle::Italic : FontStyle::Normal;
    
    auto typeface = font_manager_->GetFont(desc);
    if (typeface) {
        current_font_.setTypeface(typeface);
        current_font_.setSize(size);
    }
}

void UnifiedRenderer::SetFontSize(float size) {
    current_font_.setSize(size);
}

void UnifiedRenderer::SetOpacity(float alpha) {
    // 限制在 0.0 - 1.0 范围内
    alpha = std::max(0.0f, std::min(1.0f, alpha));
    
    // 设置填充和描边的透明度
    auto fill_color = fill_paint_.GetColor();
    fill_color.a = static_cast<uint8_t>(alpha * 255);
    fill_paint_.SetColor(fill_color);
    
    auto stroke_color = stroke_paint_.GetColor();
    stroke_color.a = static_cast<uint8_t>(alpha * 255);
    stroke_paint_.SetColor(stroke_color);
}

// ==================== 基础图形绘制 ====================

void UnifiedRenderer::FillRect(float x, float y, float width, float height) {
    EnsureShapes();
    shapes_->FillRect(x, y, width, height, fill_paint_);
}

void UnifiedRenderer::DrawRect(float x, float y, float width, float height) {
    EnsureShapes();
    shapes_->StrokeRect(x, y, width, height, stroke_paint_);
}

void UnifiedRenderer::FillCircle(float cx, float cy, float radius) {
    EnsureShapes();
    shapes_->FillCircle(cx, cy, radius, fill_paint_);
}

void UnifiedRenderer::DrawCircle(float cx, float cy, float radius) {
    EnsureShapes();
    shapes_->StrokeCircle(cx, cy, radius, stroke_paint_);
}

void UnifiedRenderer::DrawLine(float x1, float y1, float x2, float y2) {
    EnsureShapes();
    shapes_->DrawLine(x1, y1, x2, y2, stroke_paint_);
}

void UnifiedRenderer::FillRoundRect(float x, float y, float width, float height, float radius) {
    EnsureShapes();
    shapes_->FillRoundRect(x, y, width, height, radius, fill_paint_);
}

void UnifiedRenderer::DrawRoundRect(float x, float y, float width, float height, float radius) {
    EnsureShapes();
    shapes_->StrokeRoundRect(x, y, width, height, radius, stroke_paint_);
}

// ==================== 文本绘制 ====================

void UnifiedRenderer::DrawText(const std::string& text, float x, float y) {
    EnsureTextRenderer();
    text_renderer_->DrawText(text, x, y, current_font_, fill_paint_);
}

TextMetrics UnifiedRenderer::MeasureText(const std::string& text) {
    EnsureTextRenderer();
    return text_renderer_->MeasureText(text, current_font_);
}

// ==================== 图片绘制 ====================

sk_sp<SkImage> UnifiedRenderer::LoadImage(const std::string& path) {
    // 支持本地文件和网络URL
    return ImageLoader::LoadFromUrl(path);
}

void UnifiedRenderer::DrawImage(sk_sp<SkImage> image, float x, float y) {
    if (!image) return;
    EnsureImageRenderer();
    image_renderer_->DrawImage(image, x, y);
}

void UnifiedRenderer::DrawImage(sk_sp<SkImage> image, float x, float y, float width, float height) {
    if (!image) return;
    EnsureImageRenderer();
    image_renderer_->DrawImage(image, x, y, width, height);
}

// ==================== 渲染控制 ====================

void UnifiedRenderer::Clear(SkColor color) {
    canvas_->clear(color);
}

void UnifiedRenderer::Flush() {
    if (surface_) {
        surface_->flush();
    }
}

bool UnifiedRenderer::SaveToFile(const std::string& path) {
    if (!surface_) {
        return false;
    }
    
    // 创建图片快照
    auto image = surface_->makeImageSnapshot();
    if (!image) {
        return false;
    }
    
    // 编码为 PNG
    auto data = image->encodeToData(SkEncodedImageFormat::kPNG, 100);
    if (!data) {
        return false;
    }
    
    // 写入文件
    SkFILEWStream stream(path.c_str());
    return stream.write(data->data(), data->size());
}

// ==================== 状态管理 ====================

void UnifiedRenderer::Save() {
    canvas_->save();
}

void UnifiedRenderer::Restore() {
    canvas_->restore();
}

void UnifiedRenderer::Translate(float dx, float dy) {
    canvas_->translate(dx, dy);
}

void UnifiedRenderer::Scale(float sx, float sy) {
    canvas_->scale(sx, sy);
}

void UnifiedRenderer::Rotate(float degrees) {
    canvas_->rotate(degrees);
}

// ==================== 辅助方法 ====================

void UnifiedRenderer::EnsureShapes() {
    if (!shapes_) {
        shapes_ = std::make_unique<Shapes>(canvas_);
    }
}

void UnifiedRenderer::EnsureTextRenderer() {
    if (!text_renderer_) {
        text_renderer_ = std::make_unique<TextRenderer>(canvas_);
    }
}

void UnifiedRenderer::EnsureImageRenderer() {
    if (!image_renderer_) {
        image_renderer_ = std::make_unique<ImageRenderer>(canvas_);
    }
}

} // namespace mblink

