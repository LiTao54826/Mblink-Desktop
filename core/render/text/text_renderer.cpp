/**
 * @file text_renderer.cpp
 * @brief 文本渲染器实现
 */

#include "text_renderer.h"
#include <sstream>
#include <cstdint>
#include <iostream>

namespace mbink {

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
    sk_sp<SkTypeface> symbol_typeface = font_manager.GetSymbolTypeface();

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

    // 创建符号字体（保持相同大小）
    SkFont symbol_font(symbol_typeface, font.getSize());
    symbol_font.setEdging(SkFont::Edging::kAntiAlias);
    symbol_font.setSubpixel(true);

    // 创建CJK字体（保持相同大小和样式）
    SkFont cjk_font(cjk_typeface, font.getSize());
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, SYMBOL, CJK };

    float current_x = x;
    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    std::string run_text;
    CharType run_type = CharType::NORMAL;

    // 检查是否是零宽度修饰符（变体选择符、零宽连接符等）
    auto isZeroWidthModifier = [](uint32_t codepoint) -> bool {
        // Variation Selectors (U+FE00-U+FE0F)
        if (codepoint >= 0xFE00 && codepoint <= 0xFE0F) return true;
        // Zero Width Joiner (U+200D)
        if (codepoint == 0x200D) return true;
        // Zero Width Non-Joiner (U+200C)
        if (codepoint == 0x200C) return true;
        // Combining marks that modify emoji
        if (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF) return true;  // Skin tone modifiers
        return false;
    };

    auto getCharType = [&isZeroWidthModifier](uint32_t codepoint) -> CharType {
        // 零宽度修饰符跟随前一个字符的类型，返回 EMOJI
        if (isZeroWidthModifier(codepoint)) return CharType::EMOJI;
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsSymbol(codepoint)) return CharType::SYMBOL;
        if (FontManager::IsCJK(codepoint)) return CharType::CJK;
        return CharType::NORMAL;
    };

    auto drawRun = [&](const std::string& run_text_arg, CharType type, float& x_pos) {
        if (run_text_arg.empty()) return;
        const SkFont* use_font = &font;
        switch (type) {
            case CharType::EMOJI:
                use_font = emoji_typeface ? &emoji_font : &font;
                break;
            case CharType::SYMBOL:
                use_font = symbol_typeface ? &symbol_font : &font;
                break;
            case CharType::CJK:
                use_font = cjk_typeface ? &cjk_font : &font;
                break;
            default:
                use_font = &font;
                break;
        }

        // 绘制文本（包括零宽度修饰符，让字体正确渲染组合字符）
        canvas_->drawSimpleText(run_text_arg.c_str(), run_text_arg.size(), SkTextEncoding::kUTF8,
                               x_pos, y, *use_font, paint.GetSkPaint());

        // 对于 emoji，过滤掉零宽度修饰符后再计算宽度
        float width = 0.0f;
        if (type == CharType::EMOJI) {
            std::string filtered_text;
            const char* run_str = run_text_arg.c_str();
            size_t run_len = run_text_arg.size();
            size_t run_pos = 0;

            while (run_pos < run_len) {
                auto [cp, bytes] = DecodeUTF8Char(run_str + run_pos, run_len - run_pos);
                if (bytes == 0) break;

                // 跳过零宽度修饰符
                if (!isZeroWidthModifier(cp)) {
                    filtered_text.append(run_str + run_pos, bytes);
                }
                run_pos += bytes;
            }

            if (!filtered_text.empty()) {
                width = use_font->measureText(filtered_text.c_str(), filtered_text.size(), SkTextEncoding::kUTF8);
            }
        } else {
            width = use_font->measureText(run_text_arg.c_str(), run_text_arg.size(), SkTextEncoding::kUTF8);
        }

        x_pos += width;
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
    sk_sp<SkTypeface> symbol_typeface = font_manager.GetSymbolTypeface();

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

    // 创建符号字体
    SkFont symbol_font(symbol_typeface, font.getSize());
    symbol_font.setEdging(SkFont::Edging::kAntiAlias);
    symbol_font.setSubpixel(true);

    // 创建CJK字体（保持相同样式）
    SkFont cjk_font(cjk_typeface, font.getSize());
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, SYMBOL, CJK };

    auto getCharType = [](uint32_t codepoint) -> CharType {
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsSymbol(codepoint)) return CharType::SYMBOL;
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
            case CharType::SYMBOL:
                use_font = symbol_typeface ? &symbol_font : &font;
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

    // Ensure font manager is initialized
    auto& font_manager = FontManager::GetInstance();
    font_manager.Initialize();
    
    // Get the font size from the input font
    float font_size = font.getSize();
    if (font_size <= 0.0f) {
        font_size = 16.0f;  // Default size
    }
    
    // Create a working font with a valid typeface
    SkFont working_font;
    if (font.getTypeface()) {
        // Use the provided font if it has a typeface
        working_font = font;
    } else {
        // Get a default font with a valid typeface
        working_font = font_manager.GetDefaultFont(font_size);
    }
    
    sk_sp<SkTypeface> emoji_typeface = font_manager.GetEmojiTypeface();
    sk_sp<SkTypeface> symbol_typeface = font_manager.GetSymbolTypeface();

    // 获取原始字体的样式（粗细、斜体等）
    SkFontStyle original_style;
    if (working_font.getTypeface()) {
        original_style = working_font.getTypeface()->fontStyle();
    } else {
        // If no typeface, use default style
        original_style = SkFontStyle();
    }

    // 使用原始字体的样式获取 CJK 字体
    sk_sp<SkTypeface> cjk_typeface = font_manager.GetCJKTypeface(original_style);
    
    // Ensure we have a valid CJK typeface
    if (!cjk_typeface) {
        // Try to get a default typeface from the system font manager
        auto sys_font_mgr = font_manager.GetSystemFontManager();
        if (sys_font_mgr) {
            cjk_typeface = sys_font_mgr->matchFamilyStyle(nullptr, SkFontStyle());
        }
    }

    // 创建emoji字体
    SkFont emoji_font(emoji_typeface, font_size);
    emoji_font.setEdging(SkFont::Edging::kAntiAlias);
    emoji_font.setSubpixel(true);

    // 创建符号字体
    SkFont symbol_font(symbol_typeface, font_size);
    symbol_font.setEdging(SkFont::Edging::kAntiAlias);
    symbol_font.setSubpixel(true);

    // 创建CJK字体（保持相同样式）
    SkFont cjk_font(cjk_typeface, font_size);
    cjk_font.setEdging(SkFont::Edging::kAntiAlias);
    cjk_font.setSubpixel(true);

    // 字符类型枚举
    enum class CharType { NORMAL, EMOJI, SYMBOL, CJK };

    // 检查是否是零宽度修饰符（变体选择符、零宽连接符等）
    auto isZeroWidthModifier = [](uint32_t codepoint) -> bool {
        // Variation Selectors (U+FE00-U+FE0F)
        if (codepoint >= 0xFE00 && codepoint <= 0xFE0F) return true;
        // Zero Width Joiner (U+200D)
        if (codepoint == 0x200D) return true;
        // Zero Width Non-Joiner (U+200C)
        if (codepoint == 0x200C) return true;
        // Combining marks that modify emoji
        if (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF) return true;  // Skin tone modifiers
        return false;
    };

    auto getCharType = [&isZeroWidthModifier](uint32_t codepoint) -> CharType {
        // 零宽度修饰符跟随前一个字符的类型，但在这里我们返回 EMOJI
        // 因为这些修饰符主要用于 emoji
        if (isZeroWidthModifier(codepoint)) return CharType::EMOJI;
        if (FontManager::IsEmoji(codepoint)) return CharType::EMOJI;
        if (FontManager::IsSymbol(codepoint)) return CharType::SYMBOL;
        if (FontManager::IsCJK(codepoint)) return CharType::CJK;
        return CharType::NORMAL;
    };

    auto measureRun = [&](const std::string& run_text, CharType type) -> float {
        if (run_text.empty()) return 0.0f;
        const SkFont* use_font = nullptr;
        
        switch (type) {
            case CharType::EMOJI:
                use_font = emoji_typeface ? &emoji_font : &working_font;
                break;
            case CharType::SYMBOL:
                use_font = symbol_typeface ? &symbol_font : &working_font;
                break;
            case CharType::CJK:
                use_font = cjk_typeface ? &cjk_font : &working_font;
                break;
            default:
                use_font = &working_font;
                break;
        }
        
        // Safety check: if use_font is still null, return 0
        if (!use_font) return 0.0f;

        // 对于 emoji，过滤掉零宽度修饰符后再测量
        float width = 0.0f;
        if (type == CharType::EMOJI) {
            // 创建过滤后的文本（移除零宽度修饰符）
            std::string filtered_text;
            const char* run_str = run_text.c_str();
            size_t run_len = run_text.size();
            size_t run_pos = 0;

            while (run_pos < run_len) {
                auto [cp, bytes] = DecodeUTF8Char(run_str + run_pos, run_len - run_pos);
                if (bytes == 0) break;

                // 跳过零宽度修饰符
                if (!isZeroWidthModifier(cp)) {
                    filtered_text.append(run_str + run_pos, bytes);
                }
                run_pos += bytes;
            }

            if (!filtered_text.empty()) {
                width = use_font->measureText(filtered_text.c_str(), filtered_text.size(), SkTextEncoding::kUTF8);
            }


        } else {
            width = use_font->measureText(run_text.c_str(), run_text.size(), SkTextEncoding::kUTF8);
        }

        return width;
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

float TextRenderer::MeasureMinContentWidth(const std::string& text, const SkFont& font) {
    if (text.empty()) {
        return 0.0f;
    }

    // For min-content, we need to find the width of the longest "word"
    // A word is defined as:
    // - For CJK characters: each character is a word (can break anywhere)
    // - For non-CJK: a sequence of non-whitespace characters

    float max_word_width = 0.0f;
    std::string current_word;
    const char* str = text.c_str();
    size_t len = text.size();
    size_t pos = 0;

    auto isCJK = [](uint32_t codepoint) -> bool {
        // CJK Unified Ideographs
        if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) return true;
        // CJK Unified Ideographs Extension A
        if (codepoint >= 0x3400 && codepoint <= 0x4DBF) return true;
        // CJK Unified Ideographs Extension B-F
        if (codepoint >= 0x20000 && codepoint <= 0x2EBEF) return true;
        // CJK Compatibility Ideographs
        if (codepoint >= 0xF900 && codepoint <= 0xFAFF) return true;
        // Hiragana
        if (codepoint >= 0x3040 && codepoint <= 0x309F) return true;
        // Katakana
        if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return true;
        // Hangul Syllables
        if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return true;
        return false;
    };

    while (pos < len) {
        // Decode UTF-8 character
        auto [codepoint, bytes] = DecodeUTF8Char(str + pos, len - pos);
        if (bytes == 0) break;

        std::string char_str(str + pos, bytes);
        bool is_space = (codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || codepoint == '\r');
        bool is_cjk = isCJK(codepoint);

        if (is_cjk) {
            // CJK character: measure it as a single word
            // First, finish any pending non-CJK word
            if (!current_word.empty()) {
                float word_width = MeasureTextWidthWithEmoji(current_word, font);
                max_word_width = std::max(max_word_width, word_width);
                current_word.clear();
            }
            // Measure the CJK character
            float char_width = MeasureTextWidthWithEmoji(char_str, font);
            max_word_width = std::max(max_word_width, char_width);
        } else if (is_space) {
            // Space: finish current word
            if (!current_word.empty()) {
                float word_width = MeasureTextWidthWithEmoji(current_word, font);
                max_word_width = std::max(max_word_width, word_width);
                current_word.clear();
            }
        } else {
            // Non-CJK, non-space: add to current word
            current_word += char_str;
        }

        pos += bytes;
    }

    // Don't forget the last word
    if (!current_word.empty()) {
        float word_width = MeasureTextWidthWithEmoji(current_word, font);
        max_word_width = std::max(max_word_width, word_width);
    }

    return max_word_width;
}

} // namespace mbink
