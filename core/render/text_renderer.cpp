/**
 * @file text_renderer.cpp
 * @brief 文本渲染器实现
 */

#include "text_renderer.h"
#include <sstream>
#include <cstdint>

namespace lightui {

// UTF-8解码辅助函数
// 返回码点和消耗的字节数
static std::pair<uint32_t, int> DecodeUTF8Char(const char* str, size_t len) {
    if (len == 0 || !str) {
        return {0, 0};
    }

    uint8_t first = static_cast<uint8_t>(str[0]);

    // ASCII (0xxxxxxx)
    if ((first & 0x80) == 0) {
        return {first, 1};
    }

    // 2字节序列 (110xxxxx 10xxxxxx)
    if ((first & 0xE0) == 0xC0 && len >= 2) {
        uint32_t cp = (first & 0x1F) << 6;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F);
        return {cp, 2};
    }

    // 3字节序列 (1110xxxx 10xxxxxx 10xxxxxx)
    if ((first & 0xF0) == 0xE0 && len >= 3) {
        uint32_t cp = (first & 0x0F) << 12;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F) << 6;
        cp |= (static_cast<uint8_t>(str[2]) & 0x3F);
        return {cp, 3};
    }

    // 4字节序列 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
    if ((first & 0xF8) == 0xF0 && len >= 4) {
        uint32_t cp = (first & 0x07) << 18;
        cp |= (static_cast<uint8_t>(str[1]) & 0x3F) << 12;
        cp |= (static_cast<uint8_t>(str[2]) & 0x3F) << 6;
        cp |= (static_cast<uint8_t>(str[3]) & 0x3F);
        return {cp, 4};
    }

    // 无效UTF-8序列，跳过一个字节
    return {0xFFFD, 1};  // 替换字符
}

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

void TextRenderer::DrawTextWithEmoji(const std::string& text, float x, float y,
                                     const SkFont& font, const Paint& paint) {
    if (!canvas_ || text.empty()) return;

    auto& font_manager = FontManager::GetInstance();
    sk_sp<SkTypeface> emoji_typeface = font_manager.GetEmojiTypeface();

    // 获取原始字体的样式（粗细、斜体等）
    SkFontStyle original_style;
    if (font.getTypeface()) {
        original_style = font.getTypeface()->fontStyle();
    }

    // 使用原始字体的样式获取 CJK 字体
    sk_sp<SkTypeface> cjk_typeface = font_manager.GetCJKTypeface(original_style);

    // 创建emoji字体（保持相同大小）
    SkFont emoji_font(emoji_typeface, font.getSize());
    emoji_font.setEdging(SkFont::Edging::kAntiAlias);
    emoji_font.setSubpixel(true);

    // 创建CJK字体（保持相同大小和样式）
    SkFont cjk_font(cjk_typeface, font.getSize());
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, CJK };

    float current_x = x;
    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    std::string run_text;
    CharType run_type = CharType::NORMAL;

    auto getCharType = [](uint32_t codepoint) -> CharType {
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsCJK(codepoint)) return CharType::CJK;
        return CharType::NORMAL;
    };

    auto drawRun = [&](const std::string& text, CharType type, float& x_pos) {
        if (text.empty()) return;
        const SkFont* use_font = &font;
        switch (type) {
            case CharType::EMOJI:
                use_font = emoji_typeface ? &emoji_font : &font;
                break;
            case CharType::CJK:
                use_font = cjk_typeface ? &cjk_font : &font;
                break;
            default:
                use_font = &font;
                break;
        }
        canvas_->drawSimpleText(text.c_str(), text.size(), SkTextEncoding::kUTF8,
                               x_pos, y, *use_font, paint.GetSkPaint());
        x_pos += use_font->measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);
    };

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        CharType char_type = getCharType(codepoint);

        // 如果字符类型变化，绘制之前积累的文本
        if (!run_text.empty() && char_type != run_type) {
            drawRun(run_text, run_type, current_x);
            run_text.clear();
        }

        // 添加当前字符到run
        run_text.append(str + pos, bytes);
        run_type = char_type;
        pos += bytes;
    }

    // 绘制最后的run
    if (!run_text.empty()) {
        drawRun(run_text, run_type, current_x);
    }
}

void TextRenderer::DrawMultilineText(const std::string& text, float x, float y, float max_width,
                                     float line_height, const SkFont& font, const Paint& paint) {
    if (!canvas_ || text.empty()) return;

    // 分割文本为多行
    std::vector<std::string> lines = WrapText(text, max_width, font);

    // 绘制每一行
    float current_y = y;
    for (const auto& line : lines) {
        DrawTextWithEmoji(line, x, current_y, font, paint);  // 使用支持emoji的绘制
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

float TextRenderer::MeasureTextWidthWithEmoji(const std::string& text, const SkFont& font) {
    if (text.empty()) {
        return 0.0f;
    }

    auto& font_manager = FontManager::GetInstance();
    sk_sp<SkTypeface> emoji_typeface = font_manager.GetEmojiTypeface();

    // 获取原始字体的样式（粗细、斜体等）
    SkFontStyle original_style;
    if (font.getTypeface()) {
        original_style = font.getTypeface()->fontStyle();
    }

    // 使用原始字体的样式获取 CJK 字体
    sk_sp<SkTypeface> cjk_typeface = font_manager.GetCJKTypeface(original_style);

    // 创建emoji字体
    SkFont emoji_font(emoji_typeface, font.getSize());
    emoji_font.setEdging(SkFont::Edging::kAntiAlias);
    emoji_font.setSubpixel(true);

    // 创建CJK字体（保持相同样式）
    SkFont cjk_font(cjk_typeface, font.getSize());
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, CJK };

    auto getCharType = [](uint32_t codepoint) -> CharType {
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsCJK(codepoint)) return CharType::CJK;
        return CharType::NORMAL;
    };

    auto measureRun = [&](const std::string& text, CharType type) -> float {
        if (text.empty()) return 0.0f;
        const SkFont* use_font = &font;
        switch (type) {
            case CharType::EMOJI:
                use_font = emoji_typeface ? &emoji_font : &font;
                break;
            case CharType::CJK:
                use_font = cjk_typeface ? &cjk_font : &font;
                break;
            default:
                use_font = &font;
                break;
        }
        return use_font->measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8);
    };

    float total_width = 0.0f;
    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    std::string run_text;
    CharType run_type = CharType::NORMAL;

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        CharType char_type = getCharType(codepoint);

        // 如果字符类型变化，测量之前积累的文本
        if (!run_text.empty() && char_type != run_type) {
            total_width += measureRun(run_text, run_type);
            run_text.clear();
        }

        run_text.append(str + pos, bytes);
        run_type = char_type;
        pos += bytes;
    }

    // 测量最后的run
    if (!run_text.empty()) {
        total_width += measureRun(run_text, run_type);
    }

    return total_width;
}

float TextRenderer::MeasureMixedTextWidth(const std::string& text, const SkFont& font) {
    if (text.empty()) {
        return 0.0f;
    }

    auto& font_manager = FontManager::GetInstance();
    sk_sp<SkTypeface> emoji_typeface = font_manager.GetEmojiTypeface();

    // 获取原始字体的样式（粗细、斜体等）
    SkFontStyle original_style;
    if (font.getTypeface()) {
        original_style = font.getTypeface()->fontStyle();
    }

    // 使用原始字体的样式获取 CJK 字体
    sk_sp<SkTypeface> cjk_typeface = font_manager.GetCJKTypeface(original_style);

    // 创建emoji字体
    SkFont emoji_font(emoji_typeface, font.getSize());
    emoji_font.setEdging(SkFont::Edging::kAntiAlias);
    emoji_font.setSubpixel(true);

    // 创建CJK字体（保持相同样式）
    SkFont cjk_font(cjk_typeface, font.getSize());
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, CJK };

    auto getCharType = [](uint32_t codepoint) -> CharType {
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsCJK(codepoint)) return CharType::CJK;
        return CharType::NORMAL;
    };

    auto measureRun = [&](const std::string& run_text, CharType type) -> float {
        if (run_text.empty()) return 0.0f;
        const SkFont* use_font = &font;
        switch (type) {
            case CharType::EMOJI:
                use_font = emoji_typeface ? &emoji_font : &font;
                break;
            case CharType::CJK:
                use_font = cjk_typeface ? &cjk_font : &font;
                break;
            default:
                use_font = &font;
                break;
        }
        return use_font->measureText(run_text.c_str(), run_text.size(), SkTextEncoding::kUTF8);
    };

    float total_width = 0.0f;
    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    std::string run_text;
    CharType run_type = CharType::NORMAL;

    while (pos < len) {
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        CharType char_type = getCharType(codepoint);

        // 如果字符类型变化，测量之前积累的文本
        if (!run_text.empty() && char_type != run_type) {
            total_width += measureRun(run_text, run_type);
            run_text.clear();
        }

        run_text.append(str + pos, bytes);
        run_type = char_type;
        pos += bytes;
    }

    // 测量最后的run
    if (!run_text.empty()) {
        total_width += measureRun(run_text, run_type);
    }

    return total_width;
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

        // 测量整段文本 - 使用支持混合字体的测量方法
        float paragraph_width = MeasureTextWidthWithEmoji(paragraph, font);

        if (paragraph_width <= max_width) {
            // 整段文本可以放在一行
            lines.push_back(paragraph);
            continue;
        }

        // 需要换行 - 逐字符处理以支持中文
        std::string current_line;
        float current_width = 0.0f;
        const char* str = paragraph.c_str();
        size_t len = paragraph.size();
        size_t pos = 0;

        while (pos < len) {
            // 解码UTF-8字符
            auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
            if (bytes == 0) break;

            std::string char_str(str + pos, bytes);
            // 使用支持混合字体的测量方法
            float char_width = MeasureTextWidthWithEmoji(char_str, font);

            // 检查是否是空格（用于单词边界）
            bool is_space = (codepoint == ' ' || codepoint == '\t');

            // 检查添加这个字符后是否会超出宽度
            if (current_width + char_width > max_width && !current_line.empty()) {
                // 当前行已满，保存并开始新行
                // 如果当前字符是空格，跳过它（不要在新行开头放空格）
                if (!is_space) {
                    lines.push_back(current_line);
                    current_line = char_str;
                    current_width = char_width;
                } else {
                    lines.push_back(current_line);
                    current_line.clear();
                    current_width = 0.0f;
                }
            } else {
                current_line += char_str;
                current_width += char_width;
            }

            pos += bytes;
        }

        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }

    return lines;
}

} // namespace lightui
