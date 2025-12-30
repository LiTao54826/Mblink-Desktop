/**
 * @file render_text.cpp
 * @brief RenderText 类实现
 * 
 * 从 render_object.cpp 提取的文本渲染对象实现。
 * 包含 RenderText::Layout 和 RenderText::Paint 方法。
 */

#include "render_object.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/text_transform.h"
#include "core/render/utils/shadow_renderer.h"
#include "core/render/utils/color.h"
#include <algorithm>
#include <sstream>
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

namespace lightui {

// 辅助函数：计算浏览器风格的 line-height: normal
// 与 IFCLayout::MeasureTextStatic 中的查找表保持一致
static float GetBrowserNormalLineHeight(float font_size) {
    int font_size_int = static_cast<int>(font_size + 0.5f);
    switch (font_size_int) {
        case 10: return 11.5f;
        case 11: return 13.0f;
        case 12: return 14.0f;
        case 13: return 15.0f;
        case 14: return 16.0f;
        case 15: return 17.5f;
        case 16: return 18.5f;
        case 17: return 19.5f;
        case 18: return 21.0f;
        case 19: return 22.0f;
        case 20: return 23.0f;
        case 22: return 25.5f;
        case 24: return 28.0f;
        case 32: return 37.0f;
        default:
            float line_height = font_size * 1.156f;
            return std::round(line_height * 2.0f) / 2.0f;
    }
}

// ========== RenderText 实现 ==========

void RenderText::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 创建字体
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? FontWeight::BOLD : FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 创建文本渲染器来测量文本
    TextRenderer text_renderer(nullptr);

    // 计算 line-height
    float line_height;
    if (std::abs(style.line_height - 1.2f) < 0.001f) {
        line_height = GetBrowserNormalLineHeight(style.font_size);
    } else {
        line_height = style.line_height * style.font_size;
    }

    // 检查是否包含换行符
    if (text_.find('\n') != std::string::npos) {
        std::istringstream iss(text_);
        std::string line;
        float max_width = 0;
        int line_count = 0;

        while (std::getline(iss, line)) {
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            max_width = std::max(max_width, line_width);
            line_count++;
        }

        layout_info_.width = max_width;
        layout_info_.height = line_count * line_height;
    } else if (parent_width > 0) {
        float text_width = text_renderer.MeasureTextWidthWithEmoji(text_, font);
        const float epsilon = 0.01f;
        bool needs_wrap = text_width > parent_width + epsilon;

        if (needs_wrap) {
            std::vector<std::string> lines = text_renderer.WrapText(text_, parent_width, font);
            float max_width = 0;
            for (const auto& line : lines) {
                float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                max_width = std::max(max_width, line_width);
            }
            layout_info_.width = max_width;
            layout_info_.height = lines.size() * line_height;
        } else {
            layout_info_.width = text_width;
            layout_info_.height = line_height;
        }
    } else {
        float width = text_renderer.MeasureTextWidthWithEmoji(text_, font);
        layout_info_.width = width;
        layout_info_.height = line_height;
    }

    layout_info_.content_rect = SkRect::MakeWH(layout_info_.width, layout_info_.height);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderText::Paint(SkCanvas* canvas) {
    if (!canvas || text_.empty()) {
        needs_paint_ = false;
        return;
    }

    // 跳过零高度元素（如 CodeMirror 的测量占位元素）
    // 这些元素有宽度但高度为0，用于测量文本宽度
    if (layout_info_.height <= 0) {
        needs_paint_ = false;
        return;
    }

    // Viewport Culling
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, 
                                          layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(10, 10))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    float text_x = layout.x;
    float text_y = layout.y;

    canvas->save();
    canvas->translate(text_x, text_y);

    // 创建字体
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = (style.font_weight == "bold") ? FontWeight::BOLD : FontWeight::NORMAL;
    desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 获取字体度量信息
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    float skia_text_height = -font_metrics.fAscent + font_metrics.fDescent;
    float css_line_height = style.line_height * style.font_size;

    float half_leading = 0.0f;
    if (css_line_height > skia_text_height) {
        half_leading = (css_line_height - skia_text_height) / 2.0f;
    }

    float baseline_y = half_leading + (-font_metrics.fAscent);

    // 处理 vertical-align
    if (style.vertical_align == "super") {
        baseline_y -= style.font_size * 0.4f;
    } else if (style.vertical_align == "sub") {
        baseline_y += style.font_size * 0.2f;
    }

    TextRenderer text_renderer(canvas);

    SkColor text_color;
    if (!style.color.empty()) {
        text_color = lightui::Color::Parse(style.color);
    } else {
        text_color = SK_ColorBLACK;
    }

    // Determine lines to render
    std::vector<std::string> lines_to_render;
    if (!wrapped_lines_.empty()) {
        lines_to_render = wrapped_lines_;
    } else if (text_.find('\n') != std::string::npos) {
        std::istringstream iss(text_);
        std::string line;
        while (std::getline(iss, line)) {
            lines_to_render.push_back(line);
        }
    } else {
        lines_to_render.push_back(text_);
    }

    // Check for text-overflow: ellipsis
    bool use_ellipsis = false;
    float available_width = 0.0f;
    auto parent = GetParent();
    if (parent) {
        const auto& parent_style = parent->GetComputedStyle();
        if (parent_style.text_overflow == "ellipsis") {
            use_ellipsis = true;
            const auto& parent_layout = parent->GetLayoutInfo();
            float padding_left = parent_style.padding.left.ToPx(parent_layout.width, parent_style.font_size);
            float padding_right = parent_style.padding.right.ToPx(parent_layout.width, parent_style.font_size);
            available_width = parent_layout.width - padding_left - padding_right;
        }
    }

    float current_y = baseline_y;
    float line_height = style.line_height * style.font_size;

    for (const auto& line : lines_to_render) {
        if (!line.empty()) {
            std::string text_to_render = line;

            // Apply text-transform
            if (!style.text_transform.empty() && style.text_transform != "none") {
                text_to_render = TransformText(text_to_render, style.text_transform);
            }

            // Apply text-overflow: ellipsis
            if (use_ellipsis && available_width > 0) {
                float text_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
                if (text_width > available_width) {
                    const std::string ellipsis = "...";
                    float ellipsis_width = font.measureText(ellipsis.c_str(), ellipsis.length(), SkTextEncoding::kUTF8);
                    float target_width = available_width - ellipsis_width;

                    if (target_width > 0) {
                        std::string truncated;
                        size_t len = line.length();
                        size_t low = 0, high = len;

                        while (low < high) {
                            size_t mid = (low + high + 1) / 2;
                            size_t char_end = mid;
                            while (char_end > 0 && char_end < len && (line[char_end] & 0xC0) == 0x80) {
                                char_end--;
                            }
                            std::string test = line.substr(0, char_end);
                            float test_width = text_renderer.MeasureTextWidthWithEmoji(test, font);
                            if (test_width <= target_width) {
                                low = mid;
                                truncated = test;
                            } else {
                                high = mid - 1;
                            }
                        }
                        text_to_render = truncated + ellipsis;
                    } else {
                        text_to_render = ellipsis;
                    }
                }
            }

            if (!style.text_shadow.empty()) {
                ShadowRenderer::RenderTextWithShadow(canvas, text_to_render, font, 
                                                     0, current_y, text_color, style.text_shadow);
            } else {
                lightui::Paint text_paint;
                text_paint.SetColor(text_color);
                text_renderer.DrawTextWithEmoji(text_to_render, 0, current_y, font, text_paint);
            }
        }
        current_y += line_height;
    }

    // 绘制文本装饰
    bool has_underline = style.text_decoration.find("underline") != std::string::npos;
    bool has_line_through = style.text_decoration.find("line-through") != std::string::npos;
    bool is_dotted = style.text_decoration.find("dotted") != std::string::npos;
    bool is_dashed = style.text_decoration.find("dashed") != std::string::npos;

    if (has_underline || has_line_through) {
        SkPaint line_paint;
        line_paint.setColor(text_color);
        line_paint.setAntiAlias(true);

        if (is_dotted) {
            const SkScalar intervals[] = {2.0f, 2.0f};
            line_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        } else if (is_dashed) {
            const SkScalar intervals[] = {4.0f, 2.0f};
            line_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
        }

        float decoration_current_y = baseline_y;

        for (const auto& line : lines_to_render) {
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            if (line_width <= 0) {
                decoration_current_y += line_height;
                continue;
            }

            if (has_underline) {
                float underline_y = decoration_current_y + font_metrics.fUnderlinePosition;
                float underline_thickness = font_metrics.fUnderlineThickness;
                if (underline_thickness < 1.0f) underline_thickness = 1.0f;
                line_paint.setStrokeWidth(underline_thickness);
                canvas->drawLine(0, underline_y, line_width, underline_y, line_paint);
            }

            if (has_line_through) {
                float strikethrough_y = decoration_current_y + font_metrics.fStrikeoutPosition;
                float strikethrough_thickness = font_metrics.fStrikeoutThickness;
                if (strikethrough_thickness < 1.0f) strikethrough_thickness = 1.0f;
                line_paint.setStrokeWidth(strikethrough_thickness);
                canvas->drawLine(0, strikethrough_y, line_width, strikethrough_y, line_paint);
            }

            decoration_current_y += line_height;
        }
    }

    canvas->restore();
    needs_paint_ = false;
}

} // namespace lightui
