#include "textarea_metrics.h"

#include "core/dom/elements/html_textarea_element.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/font_manager.h"
#include "include/core/SkFontMetrics.h"

#include <algorithm>
#include <cmath>

namespace mblink::textarea_metrics {

namespace {

float ResolveBorderLeft(const ComputedStyle& style) {
    return style.border_left_width > 0.0f ? style.border_left_width : style.border.width.ToPx();
}

float ResolveBorderRight(const ComputedStyle& style) {
    return style.border_right_width > 0.0f ? style.border_right_width : style.border.width.ToPx();
}

float ResolveBorderTop(const ComputedStyle& style) {
    return style.border_top_width > 0.0f ? style.border_top_width : style.border.width.ToPx();
}

float ResolveBorderBottom(const ComputedStyle& style) {
    return style.border_bottom_width > 0.0f ? style.border_bottom_width : style.border.width.ToPx();
}

SkFont LoadFont(const ComputedStyle& style) {
    FontDescriptor desc;
    desc.family = style.font_family.empty() ? "sans-serif" : style.font_family;
    desc.size = style.font_size > 0.0f ? style.font_size : 14.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    return FontManager::GetInstance().LoadFont(desc);
}

float ResolveLineHeight(const ComputedStyle& style, const SkFont& font) {
    SkFontMetrics metrics;
    font.getMetrics(&metrics);

    const float font_size = style.font_size > 0.0f ? style.font_size : font.getSize();
    float metric_line_height = std::max(0.0f, -metrics.fAscent + metrics.fDescent);
    if (metrics.fLeading > 0.0f) {
        metric_line_height += metrics.fLeading;
    } else {
        metric_line_height += font_size * 0.2f;
    }

    const float css_line_height = style.line_height > 0.0f
        ? style.line_height * font_size
        : font_size * 1.2f;
    return std::max(1.0f, std::max(metric_line_height, css_line_height));
}

size_t Utf8CharByteLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

}  // namespace

bool ResolveBox(const HTMLTextAreaElement* textarea,
                const RenderObject* render_object,
                BoxMetrics& metrics) {
    if (!textarea || !render_object) {
        return false;
    }

    const auto& style = render_object->GetComputedStyle();
    const auto& layout = render_object->GetLayoutInfo();
    metrics.border_left = ResolveBorderLeft(style);
    metrics.border_right = ResolveBorderRight(style);
    metrics.border_top = ResolveBorderTop(style);
    metrics.border_bottom = ResolveBorderBottom(style);
    metrics.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    metrics.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    metrics.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    metrics.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    metrics.content_left = metrics.border_left + metrics.padding_left;
    metrics.content_top = metrics.border_top + metrics.padding_top;

    const float client_box_width = std::max(0.0f,
        layout.width - metrics.border_left - metrics.border_right);
    const float client_box_height = std::max(0.0f,
        layout.height - metrics.border_top - metrics.border_bottom);
    metrics.content_width = std::max(0.0f,
        client_box_width - metrics.padding_left - metrics.padding_right);
    metrics.content_height = std::max(0.0f,
        client_box_height - metrics.padding_top - metrics.padding_bottom);

    metrics.font = LoadFont(style);
    metrics.line_height = ResolveLineHeight(style, metrics.font);
    metrics.text_content_height = textarea->GetContentHeight(metrics.line_height);
    metrics.max_line_width = textarea->GetMaxLineWidth(metrics.font);

    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
    metrics.need_v_scrollbar = metrics.text_content_height > metrics.content_height;
    metrics.need_h_scrollbar = metrics.max_line_width > metrics.content_width;
    metrics.visible_width = metrics.content_width -
        (metrics.need_v_scrollbar ? scrollbar_width : 0.0f);
    metrics.visible_height = metrics.content_height -
        (metrics.need_h_scrollbar ? scrollbar_width : 0.0f);

    if (!metrics.need_v_scrollbar && metrics.text_content_height > metrics.visible_height) {
        metrics.need_v_scrollbar = true;
        metrics.visible_width = metrics.content_width - scrollbar_width;
    }
    if (!metrics.need_h_scrollbar && metrics.max_line_width > metrics.visible_width) {
        metrics.need_h_scrollbar = true;
        metrics.visible_height = metrics.content_height - scrollbar_width;
    }

    metrics.visible_width = std::max(0.0f, metrics.visible_width);
    metrics.visible_height = std::max(0.0f, metrics.visible_height);
    metrics.client_width = metrics.visible_width + metrics.padding_left + metrics.padding_right;
    metrics.client_height = metrics.visible_height + metrics.padding_top + metrics.padding_bottom;
    metrics.scroll_width = std::max(metrics.client_width,
        metrics.max_line_width + metrics.padding_left + metrics.padding_right);
    metrics.scroll_height = std::max(metrics.client_height,
        metrics.text_content_height + metrics.padding_top + metrics.padding_bottom);
    metrics.max_scroll_x = std::max(0.0f, metrics.max_line_width - metrics.visible_width);
    metrics.max_scroll_y = std::max(0.0f, metrics.text_content_height - metrics.visible_height);
    return true;
}

float ClampScrollTop(float scroll_top, const BoxMetrics& metrics) {
    return std::clamp(std::max(0.0f, scroll_top), 0.0f, metrics.max_scroll_y);
}

float ClampScrollLeft(float scroll_left, const BoxMetrics& metrics) {
    return std::clamp(std::max(0.0f, scroll_left), 0.0f, metrics.max_scroll_x);
}

std::vector<LineSpan> BuildLineSpans(const std::string& value) {
    std::vector<LineSpan> lines;
    size_t line_start_byte = 0;
    int line_start_char = 0;
    int char_pos = 0;

    for (size_t i = 0; i < value.size(); ) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        size_t char_bytes = Utf8CharByteLength(c);
        if (i + char_bytes > value.size()) {
            char_bytes = 1;
        }

        if (c == '\n') {
            lines.push_back({line_start_byte, i, line_start_char, char_pos});
            i += char_bytes;
            char_pos++;
            line_start_byte = i;
            line_start_char = char_pos;
            continue;
        }

        i += char_bytes;
        char_pos++;
    }

    lines.push_back({line_start_byte, value.size(), line_start_char, char_pos});
    return lines;
}

std::string LineText(const std::string& value, const LineSpan& line) {
    if (line.byte_start >= value.size()) {
        return "";
    }
    const size_t byte_end = std::min(line.byte_end, value.size());
    return value.substr(line.byte_start, byte_end - line.byte_start);
}

std::pair<std::string, int> LineForHitTest(const std::string& value, int target_line) {
    const auto lines = BuildLineSpans(value);
    if (lines.empty()) {
        return {"", 0};
    }

    const int line_index = std::clamp(target_line, 0, static_cast<int>(lines.size()) - 1);
    const auto& line = lines[line_index];
    return {LineText(value, line), line.char_start};
}

std::pair<int, int> FindLineColumn(const std::vector<LineSpan>& lines, int char_pos) {
    if (lines.empty()) {
        return {0, 0};
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        if (char_pos <= line.char_end || i + 1 == lines.size()) {
            const int col = std::clamp(char_pos - line.char_start,
                                       0,
                                       line.char_end - line.char_start);
            return {static_cast<int>(i), col};
        }
    }

    const auto& last = lines.back();
    return {static_cast<int>(lines.size() - 1),
            std::max(0, last.char_end - last.char_start)};
}

std::pair<int, int> VisibleLineRange(float scroll_top,
                                     float visible_height,
                                     float line_height,
                                     int line_count) {
    if (line_count <= 0 || line_height <= 0.0f || visible_height <= 0.0f) {
        return {0, -1};
    }

    int first = static_cast<int>(std::floor(std::max(0.0f, scroll_top) / line_height)) - 1;
    int last = static_cast<int>(std::ceil((std::max(0.0f, scroll_top) + visible_height) / line_height)) + 1;
    first = std::clamp(first, 0, line_count - 1);
    last = std::clamp(last, first, line_count - 1);
    return {first, last};
}

}  // namespace mblink::textarea_metrics
