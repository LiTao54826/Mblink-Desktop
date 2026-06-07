/**
 * @file audio_element_painter.cpp
 * @brief Native audio control painter shared by block and inline-block renderers.
 */

#include "audio_element_painter.h"
#include "core/dom/elements/html_audio_element.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/text/text_renderer.h"
#include "core/render/utils/paint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace mbink {

namespace {

std::string FormatAudioControlTime(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0) {
        seconds = 0.0;
    }

    const int total_seconds = static_cast<int>(std::floor(seconds));
    const int hours = total_seconds / 3600;
    const int minutes = (total_seconds / 60) % 60;
    const int remaining_seconds = total_seconds % 60;

    auto two_digits = [](int value) {
        return (value < 10 ? std::string("0") : std::string()) + std::to_string(value);
    };

    if (hours > 0) {
        return std::to_string(hours) + ":" + two_digits(minutes) + ":" + two_digits(remaining_seconds);
    }

    return std::to_string(minutes) + ":" + two_digits(remaining_seconds);
}

} // namespace

void PaintAudioElementControl(
    SkCanvas* canvas,
    HTMLAudioElement* audio,
    const Box& box,
    const ComputedStyle& style) {
    if (!canvas || !audio) {
        return;
    }

    const SkRect bounds = SkRect::MakeXYWH(box.content_x, box.content_y, box.content_width, box.content_height);
    if (bounds.isEmpty()) {
        return;
    }

    SkPaint background_paint;
    background_paint.setColor(SkColorSetRGB(245, 247, 250));
    background_paint.setStyle(SkPaint::kFill_Style);
    background_paint.setAntiAlias(true);

    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(198, 205, 214));
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(1.0f);
    border_paint.setAntiAlias(true);

    const float radius = std::min(6.0f, bounds.height() * 0.25f);
    const SkRRect control_rrect = SkRRect::MakeRectXY(bounds, radius, radius);
    canvas->drawRRect(control_rrect, background_paint);
    canvas->drawRRect(control_rrect, border_paint);

    if (!audio->GetControls()) {
        return;
    }

    const auto geometry = audio->ComputeControlGeometry(
        box.content_x,
        box.content_y,
        box.content_width,
        box.content_height);
    const float center_y = box.content_y + box.content_height * 0.5f;

    SkPaint icon_paint;
    icon_paint.setColor(SkColorSetRGB(34, 42, 53));
    icon_paint.setStyle(SkPaint::kFill_Style);
    icon_paint.setAntiAlias(true);

    if (audio->GetPaused()) {
        SkPath play_path;
        const float left = geometry.play_button.left() + geometry.play_button.width() * 0.34f;
        const float top = geometry.play_button.top() + geometry.play_button.height() * 0.25f;
        const float bottom = geometry.play_button.bottom() - geometry.play_button.height() * 0.25f;
        const float right = geometry.play_button.right() - geometry.play_button.width() * 0.25f;
        play_path.moveTo(left, top);
        play_path.lineTo(right, center_y);
        play_path.lineTo(left, bottom);
        play_path.close();
        canvas->drawPath(play_path, icon_paint);
    } else {
        const float bar_width = std::max(2.0f, geometry.play_button.width() * 0.18f);
        const float bar_height = geometry.play_button.height() * 0.48f;
        const float gap = bar_width;
        const float left = geometry.play_button.centerX() - gap * 0.5f - bar_width;
        const float top = geometry.play_button.centerY() - bar_height * 0.5f;
        canvas->drawRect(SkRect::MakeXYWH(left, top, bar_width, bar_height), icon_paint);
        canvas->drawRect(SkRect::MakeXYWH(left + bar_width + gap, top, bar_width, bar_height), icon_paint);
    }

    auto draw_track = [&](const SkRect& track, float ratio, SkColor fill_color) {
        if (track.width() <= 0.0f || track.height() <= 0.0f) {
            return;
        }

        SkPaint track_paint;
        track_paint.setColor(SkColorSetRGB(214, 220, 228));
        track_paint.setStyle(SkPaint::kFill_Style);
        track_paint.setAntiAlias(true);
        const float track_radius = track.height() * 0.5f;
        canvas->drawRRect(SkRRect::MakeRectXY(track, track_radius, track_radius), track_paint);

        const float clamped_ratio = std::clamp(ratio, 0.0f, 1.0f);
        if (clamped_ratio > 0.0f) {
            SkRect fill = track;
            fill.fRight = fill.fLeft + fill.width() * clamped_ratio;
            SkPaint fill_paint;
            fill_paint.setColor(fill_color);
            fill_paint.setStyle(SkPaint::kFill_Style);
            fill_paint.setAntiAlias(true);
            canvas->drawRRect(SkRRect::MakeRectXY(fill, track_radius, track_radius), fill_paint);
        }

        SkPaint thumb_paint;
        thumb_paint.setColor(SkColorSetRGB(42, 105, 214));
        thumb_paint.setStyle(SkPaint::kFill_Style);
        thumb_paint.setAntiAlias(true);
        const float thumb_radius = std::max(4.0f, track.height());
        const float thumb_x = track.left() + track.width() * clamped_ratio;
        canvas->drawCircle(thumb_x, track.centerY(), thumb_radius, thumb_paint);
    };

    const double current_time = audio->CurrentTime();
    const double duration = audio->GetDuration();
    const float progress_ratio = duration > 0.0
        ? static_cast<float>(current_time / duration)
        : 0.0f;
    draw_track(geometry.progress_track, progress_ratio, SkColorSetRGB(42, 105, 214));

    if (geometry.time_label.width() > 0.0f && geometry.time_label.height() > 0.0f) {
        FontDescriptor desc;
        desc.family = style.font_family;
        desc.size = std::clamp(style.font_size * 0.78f, 10.0f, 12.0f);
        desc.weight = FontWeight::NORMAL;
        desc.style = FontStyle::NORMAL;
        SkFont font = FontManager::GetInstance().LoadFont(desc);

        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        const float text_height = -font_metrics.fAscent + font_metrics.fDescent;
        const float text_x = geometry.time_label.left();
        const float text_y = geometry.time_label.top() +
            (geometry.time_label.height() - text_height) * 0.5f - font_metrics.fAscent;

        const std::string time_text =
            FormatAudioControlTime(current_time) + " / " + FormatAudioControlTime(duration);

        canvas->save();
        canvas->clipRect(geometry.time_label);
        TextRenderer text_renderer(canvas);
        mbink::Paint text_paint;
        text_paint.SetColor(SkColorSetRGB(82, 91, 105));
        text_renderer.DrawTextWithEmoji(time_text, text_x, text_y, font, text_paint);
        canvas->restore();
    }

    const float volume_ratio = audio->GetMuted() ? 0.0f : static_cast<float>(audio->GetVolume());
    draw_track(geometry.volume_track, volume_ratio, SkColorSetRGB(77, 94, 117));
    audio->ScheduleControlsRepaint();
}

} // namespace mbink
