#include "textarea_painter.h"

#include "textarea_metrics.h"
#include "text_edit_metrics.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/editing/textarea_edit_state.h"
#include "core/render/objects/render_object.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/text/text_renderer.h"
#include "core/render/utils/color.h"
#include "core/utils/utf8_utils.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"
#include "include/effects/SkDashPathEffect.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace mblink::textarea_painter {

namespace {

SkColor ResolveTextColor(const ComputedStyle& style, bool is_placeholder) {
    if (is_placeholder) {
        return SkColorSetRGB(150, 150, 150);
    }
    if (!style.color.empty()) {
        return mblink::Color::Parse(style.color);
    }
    return SK_ColorBLACK;
}

void PaintCompositionUnderline(SkCanvas* canvas,
                               const ComputedStyle& style,
                               const textarea_metrics::BoxMetrics& metrics,
                               const Box& box,
                               const std::string& visual_value,
                               const std::vector<textarea_metrics::LineSpan>& paint_lines,
                               const std::shared_ptr<TextAreaEditState>& edit_state,
                               float text_x,
                               float text_y,
                               float scroll_top,
                               int first_visible_line,
                               int last_visible_line,
                               const SkFontMetrics& font_metrics) {
    if (!canvas || !edit_state || !edit_state->HasActiveComposition() || paint_lines.empty()) {
        return;
    }

    static const bool ime_render_debug =
        std::getenv("MBLINK_DEBUG_IME_RENDER") != nullptr ||
        std::getenv("MBLINK_DEBUG_IME_AREA") != nullptr;
    if (ime_render_debug) {
        std::cout << "[IME_RENDER] element=textarea"
                  << " font_size=" << style.font_size
                  << " line_height=" << metrics.line_height
                  << " content_height=" << box.content_height
                  << " text_y=" << text_y
                  << " scroll_top=" << scroll_top
                  << " visual_value_length=" << utf8::CharCount(visual_value)
                  << " composition_text=" << edit_state->composition_state.text
                  << " comp_range=[" << edit_state->composition_state.start << ","
                  << edit_state->composition_state.end << "]"
                  << std::endl;
    }

    const int comp_start = std::max(0, edit_state->composition_state.start);
    const int comp_end = comp_start +
        static_cast<int>(utf8::CharCount(edit_state->composition_state.text));
    auto [comp_line, comp_col] = textarea_metrics::FindLineColumn(paint_lines, comp_start);
    auto [comp_end_line, comp_end_col] = textarea_metrics::FindLineColumn(paint_lines, comp_end);

    SkPaint comp_underline_paint;
    comp_underline_paint.setColor(SkColorSetRGB(66, 133, 244));
    comp_underline_paint.setStyle(SkPaint::kStroke_Style);
    comp_underline_paint.setStrokeWidth(std::max(1.0f, font_metrics.fUnderlineThickness));
    comp_underline_paint.setAntiAlias(true);
    const SkScalar dash_intervals[] = {3.0f, 2.0f};
    comp_underline_paint.setPathEffect(SkDashPathEffect::Make(dash_intervals, 2, 0));

    const int first_comp_line = std::max(comp_line, first_visible_line);
    const int last_comp_line = std::min(comp_end_line, last_visible_line);
    for (int line_index = first_comp_line; line_index <= last_comp_line; ++line_index) {
        const auto& current_paint_line = paint_lines[line_index];
        const std::string current_line = textarea_metrics::LineText(visual_value, current_paint_line);
        const int line_char_count = current_paint_line.char_end - current_paint_line.char_start;
        const int start_col = (line_index == comp_line) ? comp_col : 0;
        const int end_col = (line_index == comp_end_line) ? comp_end_col : line_char_count;
        const float rect_x = text_x +
            text_edit_metrics::MeasurePrefixWidth(current_line, start_col, metrics.font, false);
        const float rect_end_x = text_x +
            text_edit_metrics::MeasurePrefixWidth(current_line, end_col, metrics.font, false);
        const float rect_y = text_y + line_index * metrics.line_height + font_metrics.fAscent;
        const float rect_h = -font_metrics.fAscent + font_metrics.fDescent;
        const float underline_y =
            rect_y + rect_h + std::max(1.0f,
                                       font_metrics.fUnderlinePosition +
                                       font_metrics.fUnderlineThickness);
        canvas->drawLine(rect_x, underline_y, rect_end_x, underline_y, comp_underline_paint);
    }
}

void PaintSelectionAndCaret(SkCanvas* canvas,
                            HTMLTextAreaElement* textarea,
                            const textarea_metrics::BoxMetrics& metrics,
                            const std::string& visual_value,
                            const std::vector<textarea_metrics::LineSpan>& paint_lines,
                            const std::shared_ptr<TextAreaEditState>& edit_state,
                            float text_x,
                            float text_y,
                            int first_visible_line,
                            int last_visible_line,
                            const SkFontMetrics& font_metrics,
                            bool cursor_visible) {
    if (!canvas || !textarea || paint_lines.empty() || first_visible_line > last_visible_line) {
        return;
    }

    int sel_start = textarea->GetSelectionStart();
    int sel_end = textarea->GetSelectionEnd();
    if (edit_state && edit_state->HasActiveComposition()) {
        sel_start = edit_state->composition_state.start;
        sel_end = edit_state->composition_state.start +
            static_cast<int>(utf8::CharCount(edit_state->composition_state.text));
    }

    if (sel_start != sel_end) {
        const int start = std::min(sel_start, sel_end);
        const int end = std::max(sel_start, sel_end);

        SkPaint selection_paint;
        selection_paint.setColor(SkColorSetARGB(128, 66, 133, 244));
        selection_paint.setStyle(SkPaint::kFill_Style);

        for (int line_index = first_visible_line; line_index <= last_visible_line; ++line_index) {
            const auto& paint_line = paint_lines[line_index];
            const int line_start = paint_line.char_start;
            const int line_end = paint_line.char_end;
            if (end <= line_start || start >= line_end + 1) {
                continue;
            }

            const std::string current_line = textarea_metrics::LineText(visual_value, paint_line);
            const int line_char_count = line_end - line_start;
            const int sel_start_in_line = std::max(0, start - line_start);
            const int sel_end_in_line = std::min(line_char_count, end - line_start);
            const float sel_x_start = text_x +
                text_edit_metrics::MeasurePrefixWidth(current_line,
                                                       sel_start_in_line,
                                                       metrics.font,
                                                       false);
            const float sel_x_end = text_x +
                text_edit_metrics::MeasurePrefixWidth(current_line,
                                                       sel_end_in_line,
                                                       metrics.font,
                                                       false);

            canvas->drawRect(
                SkRect::MakeXYWH(
                    sel_x_start,
                    text_y + line_index * metrics.line_height + font_metrics.fAscent,
                    std::max(1.0f, sel_x_end - sel_x_start),
                    -font_metrics.fAscent + font_metrics.fDescent
                ),
                selection_paint
            );
        }
    }

    if (!cursor_visible) {
        return;
    }

    const int cursor_pos = sel_end;
    auto [cursor_line, cursor_col] = textarea_metrics::FindLineColumn(paint_lines, cursor_pos);
    const auto& cursor_paint_line = paint_lines[cursor_line];
    const std::string cursor_line_text = textarea_metrics::LineText(visual_value, cursor_paint_line);
    const float cursor_x = text_x +
        text_edit_metrics::MeasurePrefixWidth(cursor_line_text, cursor_col, metrics.font, false);
    const float cursor_y = text_y + cursor_line * metrics.line_height;

    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(1);
    cursor_paint.setAntiAlias(true);
    canvas->drawLine(cursor_x, cursor_y + font_metrics.fAscent,
                     cursor_x, cursor_y + font_metrics.fDescent, cursor_paint);
}

void PaintScrollbars(SkCanvas* canvas,
                     const ComputedStyle& style,
                     const Box& box,
                     const textarea_metrics::BoxMetrics& metrics,
                     float scroll_top,
                     float scroll_left) {
    if (!canvas) {
        return;
    }

    const float scrollbar_width = HTMLTextAreaElement::SCROLLBAR_WIDTH;
    const float scrollbar_min_size = 20.0f;
    SkColor scrollbar_track_color = SkColorSetARGB(30, 0, 0, 0);
    SkColor scrollbar_thumb_color = SkColorSetARGB(128, 100, 100, 100);
    if (!style.scrollbar_color_auto) {
        scrollbar_thumb_color = style.scrollbar_thumb_color;
        scrollbar_track_color = style.scrollbar_track_color;
    }

    const float padding_box_x = box.content_x - box.padding_left;
    const float padding_box_y = box.content_y - box.padding_top;
    const float padding_box_width = box.content_width + box.padding_left + box.padding_right;
    const float padding_box_height = box.content_height + box.padding_top + box.padding_bottom;

    if (metrics.need_v_scrollbar && metrics.text_content_height > 0.0f) {
        const float track_x = padding_box_x + padding_box_width - scrollbar_width;
        const float track_y = padding_box_y;
        const float track_height = metrics.need_h_scrollbar
            ? padding_box_height - scrollbar_width
            : padding_box_height;

        SkPaint track_paint;
        track_paint.setColor(scrollbar_track_color);
        track_paint.setAntiAlias(true);
        canvas->drawRRect(
            SkRRect::MakeRectXY(SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height),
                                scrollbar_width / 2.0f,
                                scrollbar_width / 2.0f),
            track_paint
        );

        const float thumb_ratio = metrics.visible_height / metrics.text_content_height;
        const float thumb_height = std::max(scrollbar_min_size, track_height * thumb_ratio);
        const float scroll_ratio = metrics.max_scroll_y > 0.0f
            ? scroll_top / metrics.max_scroll_y
            : 0.0f;
        const float thumb_y = track_y + scroll_ratio * (track_height - thumb_height);

        SkPaint thumb_paint;
        thumb_paint.setColor(scrollbar_thumb_color);
        thumb_paint.setAntiAlias(true);
        canvas->drawRRect(
            SkRRect::MakeRectXY(SkRect::MakeXYWH(track_x, thumb_y, scrollbar_width, thumb_height),
                                scrollbar_width / 2.0f,
                                scrollbar_width / 2.0f),
            thumb_paint
        );
    }

    if (metrics.need_h_scrollbar && metrics.max_line_width > 0.0f) {
        const float track_x = padding_box_x;
        const float track_y = padding_box_y + padding_box_height - scrollbar_width;
        const float track_width = metrics.need_v_scrollbar
            ? padding_box_width - scrollbar_width
            : padding_box_width;

        SkPaint track_paint;
        track_paint.setColor(scrollbar_track_color);
        track_paint.setAntiAlias(true);
        canvas->drawRRect(
            SkRRect::MakeRectXY(SkRect::MakeXYWH(track_x, track_y, track_width, scrollbar_width),
                                scrollbar_width / 2.0f,
                                scrollbar_width / 2.0f),
            track_paint
        );

        const float thumb_ratio = metrics.visible_width / metrics.max_line_width;
        const float thumb_width = std::max(scrollbar_min_size, track_width * thumb_ratio);
        const float scroll_ratio = metrics.max_scroll_x > 0.0f
            ? scroll_left / metrics.max_scroll_x
            : 0.0f;
        const float thumb_x = track_x + scroll_ratio * (track_width - thumb_width);

        SkPaint thumb_paint;
        thumb_paint.setColor(scrollbar_thumb_color);
        thumb_paint.setAntiAlias(true);
        canvas->drawRRect(
            SkRRect::MakeRectXY(SkRect::MakeXYWH(thumb_x, track_y, thumb_width, scrollbar_width),
                                scrollbar_width / 2.0f,
                                scrollbar_width / 2.0f),
            thumb_paint
        );
    }
}

}  // namespace

void Paint(SkCanvas* canvas,
           HTMLTextAreaElement* textarea,
           RenderObject* render_object,
           const Box& box,
           bool cursor_visible) {
    if (!canvas || !textarea || !render_object) {
        return;
    }

    const ComputedStyle& style = render_object->GetComputedStyle();
    std::string value = textarea->GetValue();
    auto edit_state = textarea->GetEditState();
    std::string visual_value = value;
    bool is_placeholder = false;

    if (edit_state && edit_state->HasActiveComposition()) {
        const auto& composition = edit_state->composition_state;
        const int start = std::max(0, composition.start);
        const int end = std::max(start, composition.end);
        const size_t start_byte = utf8::CharPosToBytePos(value, start);
        const size_t end_byte = utf8::CharPosToBytePos(value, end);
        visual_value = value.substr(0, start_byte) + composition.text + value.substr(end_byte);
    }

    if (value.empty() && (!edit_state || !edit_state->HasActiveComposition())) {
        visual_value = textarea->GetPlaceholder();
        is_placeholder = true;
    }

    textarea_metrics::BoxMetrics metrics;
    if (!textarea_metrics::ResolveBox(textarea, render_object, metrics)) {
        return;
    }

    SkFontMetrics font_metrics;
    metrics.font.getMetrics(&font_metrics);

    const float scroll_top = textarea_metrics::ClampScrollTop(textarea->GetScrollTop(), metrics);
    const float scroll_left = textarea_metrics::ClampScrollLeft(textarea->GetScrollLeft(), metrics);

    const auto node = render_object->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    const bool has_focus = element && element->HasPseudoClass("focus");

    const auto paint_lines = textarea_metrics::BuildLineSpans(visual_value);
    auto [first_visible_line, last_visible_line] = textarea_metrics::VisibleLineRange(
        scroll_top,
        metrics.visible_height,
        metrics.line_height,
        static_cast<int>(paint_lines.size())
    );

    const float text_x = box.content_x - scroll_left;
    const float text_y = box.content_y - font_metrics.fAscent - scroll_top;

    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(box.content_x,
                                      box.content_y,
                                      metrics.visible_width,
                                      metrics.visible_height));

    if (!visual_value.empty() && first_visible_line <= last_visible_line) {
        TextRenderer text_renderer(canvas);
        mblink::Paint text_paint;
        text_paint.SetColor(ResolveTextColor(style, is_placeholder));

        for (int line_index = first_visible_line; line_index <= last_visible_line; ++line_index) {
            const auto& paint_line = paint_lines[line_index];
            const std::string current_line = textarea_metrics::LineText(visual_value, paint_line);
            text_renderer.DrawTextWithEmoji(current_line,
                                            text_x,
                                            text_y + line_index * metrics.line_height,
                                            metrics.font,
                                            text_paint);
        }

        if (!is_placeholder) {
            PaintCompositionUnderline(canvas,
                                      style,
                                      metrics,
                                      box,
                                      visual_value,
                                      paint_lines,
                                      edit_state,
                                      text_x,
                                      text_y,
                                      scroll_top,
                                      first_visible_line,
                                      last_visible_line,
                                      font_metrics);
        }
    }

    if (has_focus) {
        PaintSelectionAndCaret(canvas,
                               textarea,
                               metrics,
                               visual_value,
                               paint_lines,
                               edit_state,
                               text_x,
                               text_y,
                               first_visible_line,
                               last_visible_line,
                               font_metrics,
                               cursor_visible);
    }

    canvas->restore();

    PaintScrollbars(canvas, style, box, metrics, scroll_top, scroll_left);
}

}  // namespace mblink::textarea_painter
