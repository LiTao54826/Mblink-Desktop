/**
 * @file text_renderer.cpp
 * @brief 文本渲染器实现
 */

#include "text_renderer.h"
#include <sstream>

namespace lightui {

// ========== 构造函数 ==========

TextRenderer::TextRenderer(SkCanvas* canvas)
    : canvas_(canvas) {
}

// ========== 文本绘制 ==========

void TextRenderer::DrawText(const std::string& text, float x, float y, const SkFont& font, const Paint& paint) {
    if (!canvas_ || text.empty()) return;

    canvas_->drawSimpleText(text.c_str(), text.size(), SkTextEncoding::kUTF8,
                           x, y, font, paint.GetSkPaint());
}

void TextRenderer::DrawText(const std::string& text, float x, float y,
                           const FontDescriptor& descriptor, const Paint& paint) {
    if (!canvas_ || text.empty()) return;

    SkFont font = FontManager::GetInstance().LoadFont(descriptor);
    DrawText(text, x, y, font, paint);
}

void TextRenderer::DrawMultilineText(const std::string& text, float x, float y, float max_width,
                                     float line_height, const SkFont& font, const Paint& paint) {
    if (!canvas_ || text.empty()) return;

    // 分割文本为多行
    std::vector<std::string> lines = WrapText(text, max_width, font);

    // 绘制每一行
    float current_y = y;
    for (const auto& line : lines) {
        DrawText(line, x, current_y, font, paint);
        current_y += line_height;
    }
}

// ========== 文本测量 ==========

TextMetrics TextRenderer::MeasureText(const std::string& text, const SkFont& font) {
    TextMetrics metrics;

    if (text.empty()) {
        return metrics;
    }

    // 测量文本宽度
    SkRect bounds;
    metrics.width = font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8, &bounds);

    // 获取字体度量
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    metrics.ascent = -font_metrics.fAscent;
    metrics.descent = font_metrics.fDescent;
    metrics.leading = font_metrics.fLeading;
    metrics.height = metrics.ascent + metrics.descent;

    return metrics;
}

float TextRenderer::MeasureTextWidth(const std::string& text, const SkFont& font) {
    if (text.empty()) {
        return 0.0f;
    }

    return font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);
}

float TextRenderer::MeasureTextHeight(const SkFont& font) {
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    return -metrics.fAscent + metrics.fDescent;
}

// ========== 文本装饰 ==========

void TextRenderer::DrawUnderline(float x, float y, float width, const Paint& paint) {
    if (!canvas_) return;

    // 下划线位置在基线下方约 1/10 字体大小
    float underline_y = y + 2.0f;
    float underline_thickness = 1.0f;

    Paint line_paint = paint;
    line_paint.SetStyle(PaintStyle::STROKE);
    line_paint.SetStrokeWidth(underline_thickness);

    canvas_->drawLine(x, underline_y, x + width, underline_y, line_paint.GetSkPaint());
}

void TextRenderer::DrawLineThrough(float x, float y, float width, const SkFont& font, const Paint& paint) {
    if (!canvas_) return;

    // 删除线位置在基线上方约 1/3 字体大小
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    float line_y = y + metrics.fAscent / 2.0f;
    float line_thickness = 1.0f;

    Paint line_paint = paint;
    line_paint.SetStyle(PaintStyle::STROKE);
    line_paint.SetStrokeWidth(line_thickness);

    canvas_->drawLine(x, line_y, x + width, line_y, line_paint.GetSkPaint());
}

// ========== 文本换行 ==========

std::vector<std::string> TextRenderer::WrapText(const std::string& text, float max_width, const SkFont& font) {
    std::vector<std::string> lines;

    if (text.empty()) {
        return lines;
    }

    // 按换行符分割
    std::istringstream iss(text);
    std::string paragraph;

    while (std::getline(iss, paragraph)) {
        if (paragraph.empty()) {
            lines.push_back("");
            continue;
        }

        // 测量整段文本
        float paragraph_width = MeasureTextWidth(paragraph, font);

        if (paragraph_width <= max_width) {
            // 整段文本可以放在一行
            lines.push_back(paragraph);
            continue;
        }

        // 需要换行
        std::string current_line;
        std::istringstream word_stream(paragraph);
        std::string word;

        while (word_stream >> word) {
            std::string test_line = current_line.empty() ? word : current_line + " " + word;
            float test_width = MeasureTextWidth(test_line, font);

            if (test_width <= max_width) {
                current_line = test_line;
            } else {
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                }
                current_line = word;
            }
        }

        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }

    return lines;
}

} // namespace lightui
