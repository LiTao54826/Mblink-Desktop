/**
 * @file render_text.cpp
 * @brief RenderText 类实现
 *
 * 从 render_object.cpp 提取的文本渲染对象实现。
 * 包含 RenderText::Layout 和 RenderText::Paint 方法。
 */

#include "render_object.h"
#include "core/render/text/font_manager.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/text_transform.h"
#include "core/render/utils/shadow_renderer.h"
#include "core/render/utils/color.h"
#include "core/dom/element.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <unordered_map>
#include <string>
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif


namespace mbink {

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
        default: {
            float line_height = font_size * 1.156f;
            return std::round(line_height * 2.0f) / 2.0f;
        }
    }
}


void RenderText::SetWrappedLines(const std::vector<std::string>& lines) {
    wrapped_lines_ = lines;
    wrapped_line_x_offsets_.clear();
    wrapped_line_y_offsets_.clear();
}

void RenderText::SetWrappedLinesWithAlignedOffsets(const std::vector<std::string>& lines,
                                                   float line_box_width) {
    wrapped_lines_ = lines;
    wrapped_line_y_offsets_.clear();
    wrapped_line_x_offsets_.assign(lines.size(), 0.0f);

    if (lines.empty() || line_box_width <= 0.0f) {
        return;
    }

    std::string text_align = computed_style_.text_align;
    if (auto parent = GetParent()) {
        const auto& parent_style = parent->GetComputedStyle();
        if (!parent_style.text_align.empty()) {
            text_align = parent_style.text_align;
        }
    }

    if (text_align != "center" && text_align != "right") {
        return;
    }

    FontDescriptor desc;
    desc.family = computed_style_.font_family;
    desc.size = computed_style_.font_size;
    desc.weight = ParseCSSFontWeight(computed_style_.font_weight);
    desc.style = (computed_style_.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);
    TextRenderer text_renderer(nullptr);

    for (size_t i = 0; i < lines.size(); ++i) {
        const float line_width = text_renderer.MeasureTextWidthWithEmoji(lines[i], font);
        const float free_width = std::max(0.0f, line_box_width - line_width);
        wrapped_line_x_offsets_[i] = (text_align == "center") ? free_width / 2.0f : free_width;
    }
}

void RenderText::SetWrappedLinesWithOffsets(const std::vector<std::string>& lines,
                                            const std::vector<float>& x_offsets) {
    wrapped_lines_ = lines;
    wrapped_line_x_offsets_ = x_offsets;
    wrapped_line_y_offsets_.clear();
}

void RenderText::SetWrappedLinesWithOffsets(const std::vector<std::string>& lines,
                                            const std::vector<float>& x_offsets,
                                            const std::vector<float>& y_offsets) {
    wrapped_lines_ = lines;
    wrapped_line_x_offsets_ = x_offsets;
    wrapped_line_y_offsets_ = y_offsets;
}

void RenderText::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 创建字体
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = ParseCSSFontWeight(style.font_weight);
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
        std::vector<std::string> lines;
        float max_width = 0;

        while (std::getline(iss, line)) {
            float line_width = text_renderer.MeasureTextWidthWithEmoji(line, font);
            max_width = std::max(max_width, line_width);
            lines.push_back(line);
        }

        SetWrappedLinesWithAlignedOffsets(lines, max_width);
        layout_info_.width = max_width;
        layout_info_.height = lines.size() * line_height;
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
            SetWrappedLinesWithAlignedOffsets(lines, parent_width);
            layout_info_.width = max_width;
            layout_info_.height = lines.size() * line_height;
        } else {
            SetWrappedLines({});
            layout_info_.width = text_width;
            layout_info_.height = line_height;
        }
    } else {
        SetWrappedLines({});
        float width = text_renderer.MeasureTextWidthWithEmoji(text_, font);
        layout_info_.width = width;
        layout_info_.height = line_height;
    }

    layout_info_.content_rect = SkRect::MakeWH(layout_info_.width, layout_info_.height);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderText::Paint(SkCanvas* canvas) {
    // 🔍 DEBUG: 增量更新问题调试
    static bool debug_paint = std::getenv("DEBUG_INCREMENTAL_PAINT") != nullptr;

    if (!canvas || text_.empty()) {
        if (debug_paint && !text_.empty()) {
        }
        needs_paint_ = false;
        return;
    }

    // 跳过零高度元素（如 CodeMirror 的测量占位元素）
    if (layout_info_.height <= 0) {
        if (debug_paint) {
        }
        needs_paint_ = false;
        return;
    }

    // Viewport Culling
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y,
                                          layout_info_.width, layout_info_.height);
    bool culled = canvas->quickReject(paint_rect.makeOutset(10, 10));

    if (debug_paint) {
    }

    if (culled) {
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
    desc.weight = ParseCSSFontWeight(style.font_weight);
    desc.style = (style.font_style == "italic") ? FontStyle::ITALIC : FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // 获取字体度量信息
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    float skia_text_height = -font_metrics.fAscent + font_metrics.fDescent;

    // 计算 css_line_height - 必须与 Layout 中的计算保持一致！
    // 如果 style.line_height 是默认值 1.2，使用浏览器风格的 line-height: normal
    float css_line_height;
    if (std::abs(style.line_height - 1.2f) < 0.001f) {
        css_line_height = GetBrowserNormalLineHeight(style.font_size);
    } else {
        css_line_height = style.line_height * style.font_size;
    }

    // 计算 baseline_y - 用于垂直居中文本
    // 必须与 vertical_aligner.cpp 中的 GetBoxMetrics 保持一致的逻辑
    //
    // GetBoxMetrics 的逻辑：
    // - 当 box.height > content_height 时，添加 half-leading
    // - 当 box.height < content_height 时，按比例缩放 ascent/descent
    // - baseline = ascent (缩放后的)
    //
    // 这里 box.height 对应 css_line_height，content_height 对应 skia_text_height
    float baseline_y;
    float raw_ascent = -font_metrics.fAscent;
    float raw_descent = font_metrics.fDescent;

    if (css_line_height > skia_text_height) {
        // 有额外空间，添加 half-leading
        float half_leading = (css_line_height - skia_text_height) / 2.0f;
        baseline_y = half_leading + raw_ascent;
    } else if (css_line_height < skia_text_height) {
        // line-height 小于 Skia 测量的高度，按比例缩放
        // 这与 GetBoxMetrics 中的处理保持一致
        float scale = css_line_height / skia_text_height;
        float scaled_ascent = raw_ascent * scale;
        // baseline 就是缩放后的 ascent
        baseline_y = scaled_ascent;
    } else {
        // 完全相等
        baseline_y = raw_ascent;
    }

    // DEBUG: 输出文字渲染位置信息
    static bool debug_text_paint = std::getenv("DEBUG_TEXT_PAINT") != nullptr;
    if (debug_text_paint) {
    }

    // 处理 vertical-align
    if (style.vertical_align == "super") {
        baseline_y -= style.font_size * 0.4f;
    } else if (style.vertical_align == "sub") {
        baseline_y += style.font_size * 0.2f;
    }

    TextRenderer text_renderer(canvas);

    SkColor text_color;
    if (!style.color.empty()) {
        text_color = mbink::Color::Parse(style.color);
    } else {
        text_color = SK_ColorBLACK;
    }

    // Determine lines to render
    // 优先使用与当前布局高度一致的 wrapped_lines_，避免旧测量阶段遗留的 wrapped_lines_
    // 导致“布局单行但绘制多行”的不一致。
    std::vector<std::string> lines_to_render;
    if (!wrapped_lines_.empty()) {
        const float expected_height = css_line_height * static_cast<float>(wrapped_lines_.size());
        const float tolerance = 0.5f;
        const bool wrapped_matches_layout = (layout.height + tolerance >= expected_height);

        if (wrapped_matches_layout) {
            lines_to_render = wrapped_lines_;
        } else {
            // wrapped_lines_ 与当前 layout 高度不匹配，视为过期数据，回退到单行/显式换行路径
            if (debug_text_paint) {
                std::cout << "[TEXT_PAINT] ignore stale wrapped_lines: lines=" << wrapped_lines_.size()
                          << " layout_h=" << layout.height
                          << " expected_h=" << expected_height << std::endl;
            }
        }
    }

    if (lines_to_render.empty()) {
        if (text_.find('\n') != std::string::npos) {
            std::istringstream iss(text_);
            std::string line;
            while (std::getline(iss, line)) {
                lines_to_render.push_back(line);
            }
        } else {
            lines_to_render.push_back(text_);
        }
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
    // 多行文本的行间距也需要与 Layout 保持一致
    float line_height = css_line_height;

    for (size_t i = 0; i < lines_to_render.size(); ++i) {
        const auto& line = lines_to_render[i];
        float line_x = 0.0f;
        if (i < wrapped_line_x_offsets_.size()) {
            line_x = wrapped_line_x_offsets_[i];
        }

        float line_y = current_y;
        if (i < wrapped_line_y_offsets_.size()) {
            line_y = wrapped_line_y_offsets_[i] + baseline_y;
        }

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
                                                     line_x, line_y, text_color, style.text_shadow, text_renderer);
            } else {
                mbink::Paint text_paint;
                text_paint.SetColor(text_color);
                text_renderer.DrawTextWithEmoji(text_to_render, line_x, line_y, font, text_paint);
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

        for (size_t i = 0; i < lines_to_render.size(); ++i) {
            const auto& line = lines_to_render[i];
            float line_x = 0.0f;
            if (i < wrapped_line_x_offsets_.size()) {
                line_x = wrapped_line_x_offsets_[i];
            }

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
                canvas->drawLine(line_x, underline_y, line_x + line_width, underline_y, line_paint);
            }

            if (has_line_through) {
                float strikethrough_y = decoration_current_y + font_metrics.fStrikeoutPosition;
                float strikethrough_thickness = font_metrics.fStrikeoutThickness;
                if (strikethrough_thickness < 1.0f) strikethrough_thickness = 1.0f;
                line_paint.setStrokeWidth(strikethrough_thickness);
                canvas->drawLine(line_x, strikethrough_y, line_x + line_width, strikethrough_y, line_paint);
            }

            decoration_current_y += line_height;
        }
    }

    canvas->restore();
    needs_paint_ = false;
}

} // namespace mbink
