/**
 * @file log_renderer.cpp
 * @brief 日志渲染器实现
 */

#include "log_renderer.h"

#include "core/render/text/font_manager.h"
#include "core/render/text/text_renderer.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"

#include <cstdio>
#include <iostream>
#include <string>

namespace mbink {

namespace {

bool IsAsciiText(std::string_view text) {
    for (unsigned char ch : text) {
        if (ch >= 0x80) {
            return false;
        }
    }
    return true;
}

}  // namespace

LogRenderer::LogRenderer() {
    // 使用 FontManager 加载等宽字体
    auto& font_manager = FontManager::GetInstance();
    font_manager.Initialize();

    // 使用 Consolas 作为基础等宽字体
    FontDescriptor desc;
    desc.family = "Consolas";
    desc.size = 14.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;

    SkFont font = font_manager.LoadFont(desc);
    typeface_ = font.refTypeface();
    font_size_ = 14.0f;

    UpdateCellMetrics();
}

void LogRenderer::SetConfig(const LogViewConfig& config) {
    config_ = config;
    InvalidateWidthCache();
}

void LogRenderer::SetFont(sk_sp<SkTypeface> typeface, float size) {
    VirtualScrollRenderer::SetFont(std::move(typeface), size);
    InvalidateWidthCache();
}

void LogRenderer::SetBuffer(LogBuffer* buffer) {
    buffer_ = buffer;
    InvalidateWidthCache();
}

void LogRenderer::SetFilteredIndices(const std::vector<size_t>* indices) {
    if (filtered_indices_ == indices) {
        return;
    }
    filtered_indices_ = indices;
    InvalidateWidthCache();
}

void LogRenderer::SetSearch(const LogSearch* search) {
    search_ = search;
}

void LogRenderer::SetSelection(const virtual_text::SelectionManager* selection) {
    selection_ = selection;
}

void LogRenderer::InvalidateWidthCache() {
    max_content_width_dirty_ = true;
}

void LogRenderer::UpdateCachedWidthForEntry(size_t log_index) {
    if (!buffer_ || log_index >= buffer_->size()) {
        return;
    }
    if (max_content_width_dirty_) {
        return;
    }
    cached_max_content_width_ =
        std::max(cached_max_content_width_, ComputeEntryWidth(log_index));
}

void LogRenderer::UpdateScrollMetricsForBounds(const SkRect& bounds) {
    if (!buffer_) {
        return;
    }
    UpdateLayoutMetrics(bounds);
}

void LogRenderer::UpdateLineMetricsForBounds(const SkRect& bounds) {
    total_lines_ = GetDisplayLineCount();
    UpdateMetrics(bounds.height());
}

void LogRenderer::UpdateLineMetricsForBounds(const SkRect& bounds,
                                             float known_content_width) {
    LayoutResult result;
    total_lines_ = GetDisplayLineCount();

    const float scrollbar_thickness = 8.0f;
    const float scrollbar_gap = 2.0f;
    const float inner_width = std::max(0.0f, bounds.width() - 2 * padding_);
    const float inner_height = std::max(0.0f, bounds.height() - 2 * padding_);

    while (true) {
        const float viewport_width = std::max(
            0.0f, inner_width - (result.need_vertical_scrollbar
                                     ? (scrollbar_thickness + scrollbar_gap)
                                     : 0.0f));
        const float viewport_height = std::max(
            0.0f, inner_height - (result.need_horizontal_scrollbar
                                      ? (scrollbar_thickness + scrollbar_gap)
                                      : 0.0f));

        int visible_lines = 0;
        if (line_height_ > 0) {
            visible_lines = static_cast<int>(viewport_height / line_height_);
            if (visible_lines < 1) visible_lines = 1;
        }

        const bool new_need_vertical = total_lines_ > visible_lines;
        const int horizontal_max = static_cast<int>(std::ceil(
            std::max(0.0f, known_content_width - viewport_width) /
            std::max(cell_width_, 1.0f)));
        const bool new_need_horizontal = horizontal_max > 0;

        if (new_need_vertical == result.need_vertical_scrollbar &&
            new_need_horizontal == result.need_horizontal_scrollbar) {
            break;
        }

        result.need_vertical_scrollbar = new_need_vertical;
        result.need_horizontal_scrollbar = new_need_horizontal;
    }

    result.content_bounds = bounds;
    if (result.need_vertical_scrollbar) {
        result.content_bounds.fRight -= scrollbar_thickness + scrollbar_gap;
    }
    if (result.need_horizontal_scrollbar) {
        result.content_bounds.fBottom -= scrollbar_thickness + scrollbar_gap;
    }

    UpdateMetrics(result.content_bounds.height());
    const float viewport_width =
        std::max(0.0f, result.content_bounds.width() - 2 * padding_);
    SetMaxHorizontalScrollOffset(static_cast<int>(std::ceil(
        std::max(0.0f, known_content_width - viewport_width) /
        std::max(cell_width_, 1.0f))));
}

void LogRenderer::UpdateLineMetricsForEntryBounds(const SkRect& bounds,
                                                  size_t log_index) {
    if (!max_content_width_dirty_) {
        UpdateCachedWidthForEntry(log_index);
        UpdateLineMetricsForBounds(bounds, cached_max_content_width_);
        return;
    }

    UpdateLineMetricsForBounds(bounds, ComputeEntryWidth(log_index));
}

void LogRenderer::Render(SkCanvas* canvas, const SkRect& bounds) {
    if (!buffer_ || !canvas) {
        return;
    }

    LayoutResult layout = UpdateLayoutMetrics(bounds);

    SkPaint bg_paint;
    bg_paint.setColor(config_.background_color);
    canvas->drawRect(bounds, bg_paint);

    if (total_lines_ == 0) {
        return;
    }

    canvas->save();
    canvas->clipRect(layout.content_bounds);

    int start_line = scroll_offset_;
    int end_line = std::min(start_line + visible_lines_ + 1, total_lines_);

    for (int i = start_line; i < end_line; ++i) {
        size_t log_index = DisplayIndexToLogIndex(i);
        SkRect line_rect = GetLineRect(i - start_line, layout.content_bounds);

        bool is_current = false;
        if (search_ && search_->current_match() >= 0) {
            int match_idx = search_->current_match();
            if (match_idx < static_cast<int>(search_->matches().size())) {
                is_current = search_->matches()[match_idx].log_index == log_index;
            }
        }

        RenderLine(canvas, log_index, line_rect, is_current);
    }

    canvas->restore();

    RenderScrollbar(canvas, layout.content_bounds, layout.need_horizontal_scrollbar);
    RenderHorizontalScrollbar(canvas, layout.content_bounds,
                              layout.need_vertical_scrollbar);
}




int LogRenderer::GetDisplayLineCount() const {
    if (!buffer_) {
        return 0;
    }

    if (filtered_indices_) {
        return static_cast<int>(filtered_indices_->size());
    }
    return static_cast<int>(buffer_->size());
}

size_t LogRenderer::DisplayIndexToLogIndex(int display_index) const {
    if (filtered_indices_ && display_index < static_cast<int>(filtered_indices_->size())) {
        return (*filtered_indices_)[display_index];
    }
    return static_cast<size_t>(display_index);
}

void LogRenderer::RenderLine(SkCanvas* canvas, size_t log_index,
                             const SkRect& line_rect, bool is_current_match) {
    auto entry = buffer_->GetEntry(log_index);
    LogLevel level = entry.level();

    // 计算显示行号（用于选择判断）
    int display_line = -1;
    if (filtered_indices_) {
        for (size_t i = 0; i < filtered_indices_->size(); ++i) {
            if ((*filtered_indices_)[i] == log_index) {
                display_line = static_cast<int>(i);
                break;
            }
        }
    } else {
        display_line = static_cast<int>(log_index);
    }

    // 绘制选中背景
    if (selection_ && selection_->HasSelection() && display_line >= 0) {
        auto sel = selection_->GetSelection();
        if (display_line >= sel.start_line && display_line <= sel.end_line) {
            SkPaint sel_bg;
            sel_bg.setColor(config_.selection_color);
            canvas->drawRect(line_rect, sel_bg);
        }
    }
    // 绘制行背景（如果有搜索匹配）
    else if (search_ && search_->HasMatchInEntry(log_index)) {
        SkPaint match_bg;
        match_bg.setColor(is_current_match ? config_.current_match_color
                                           : config_.match_color);
        canvas->drawRect(line_rect, match_bg);
    }

    float x = line_rect.left() + padding_ -
              horizontal_scroll_offset_ * cell_width_;
    float y = line_rect.top() + line_height_ - padding_;

    // 渲染时间戳
    if (config_.show_timestamp) {
        x = RenderTimestamp(canvas, entry.timestamp(), x, y);
        x += cell_width_;  // 间隔
    }

    // 渲染级别
    if (config_.show_level) {
        x = RenderLevel(canvas, level, x, y);
        x += cell_width_;
    }

    // 渲染源名
    if (config_.show_source) {
        const std::string& source = buffer_->GetSourceName(entry.source_id());
        if (!source.empty()) {
            x = RenderSource(canvas, source, x, y);
            x += cell_width_;
        }
    }

    // 渲染消息
    float max_width = line_rect.right() - x - padding_;
    RenderMessage(canvas, log_index, entry.message(), level, x, y, max_width);
}

float LogRenderer::RenderTimestamp(SkCanvas* canvas, uint32_t timestamp,
                                   float x, float y) {
    std::string ts = FormatTimestamp(timestamp);

    SkFont font(typeface_, font_size_);
    Paint paint;
    paint.SetColor(config_.timestamp_color);
    paint.SetAntiAlias(true);

    return DrawTextRun(canvas, ts, x, y, font, paint);
}

float LogRenderer::RenderLevel(SkCanvas* canvas, LogLevel level,
                               float x, float y) {
    const char* level_str = LogLevelToString(level);
    int level_idx = static_cast<int>(level);
    if (level_idx < 0 || level_idx > 4) level_idx = 1;

    SkFont font(typeface_, font_size_);
    Paint paint;
    paint.SetColor(config_.level_colors[level_idx]);
    paint.SetAntiAlias(true);

    std::string text = std::string("[") + level_str + "]";
    return DrawTextRun(canvas, text, x, y, font, paint);
}

float LogRenderer::RenderSource(SkCanvas* canvas, const std::string& source,
                                float x, float y) {
    SkFont font(typeface_, font_size_);
    Paint paint;
    paint.SetColor(config_.source_color);
    paint.SetAntiAlias(true);

    std::string text = "[" + source + "]";
    return DrawTextRun(canvas, text, x, y, font, paint);
}

void LogRenderer::RenderMessage(SkCanvas* canvas, size_t log_index,
                                std::string_view message, LogLevel level,
                                float x, float y, float max_width) {
    SkFont font(typeface_, font_size_);

    int level_idx = static_cast<int>(level);
    if (level_idx < 0 || level_idx > 4) level_idx = 1;
    SkColor text_color = config_.level_colors[level_idx];

    std::vector<SearchMatch> matches;
    if (search_ && search_->HasSearch()) {
        matches = search_->GetMatchesForEntry(log_index);
    }

    if (matches.empty()) {
        Paint paint;
        paint.SetColor(text_color);
        paint.SetAntiAlias(true);
        DrawTextRun(canvas, message, x, y, font, paint);
    } else {
        size_t pos = 0;
        float current_x = x;

        for (const auto& match : matches) {
            if (match.start_pos > pos) {
                std::string before(message.substr(pos, match.start_pos - pos));
                Paint paint;
                paint.SetColor(text_color);
                paint.SetAntiAlias(true);
                current_x = DrawTextRun(canvas, before, current_x, y, font, paint);
            }

            std::string match_text(message.substr(match.start_pos, match.length));
            float match_width = MeasureTextRun(match_text, font);

            bool is_current = search_->IsCurrentMatch(log_index, match.start_pos);
            SkPaint bg_paint;
            bg_paint.setColor(is_current ? config_.current_match_color
                                         : config_.match_color);

            SkRect match_rect = SkRect::MakeXYWH(
                current_x, y - line_height_ + padding_,
                match_width, line_height_);
            canvas->drawRect(match_rect, bg_paint);

            Paint paint;
            paint.SetColor(text_color);
            paint.SetAntiAlias(true);
            DrawTextRun(canvas, match_text, current_x, y, font, paint);
            current_x += match_width;

            pos = match.start_pos + match.length;
        }

        if (pos < message.length()) {
            std::string after(message.substr(pos));
            Paint paint;
            paint.SetColor(text_color);
            paint.SetAntiAlias(true);
            DrawTextRun(canvas, after, current_x, y, font, paint);
        }
    }
}

float LogRenderer::DrawTextRun(SkCanvas* canvas, std::string_view text,
                               float x, float y, const SkFont& font,
                               const Paint& paint) const {
    if (!canvas || text.empty()) {
        return x;
    }

    if (IsAsciiText(text)) {
        canvas->drawSimpleText(text.data(), text.size(), SkTextEncoding::kUTF8,
                               x, y, font, paint.GetSkPaint());
        return x + static_cast<float>(text.size()) * cell_width_;
    }

    TextRenderer text_renderer(canvas);
    std::string owned_text(text);
    text_renderer.DrawTextWithEmoji(owned_text, x, y, font, paint);
    return x + TextRenderer::MeasureMixedTextWidth(owned_text, font);
}

float LogRenderer::MeasureTextRun(std::string_view text, const SkFont& font) const {
    if (text.empty()) {
        return 0.0f;
    }

    if (IsAsciiText(text)) {
        return static_cast<float>(text.size()) * cell_width_;
    }

    return TextRenderer::MeasureMixedTextWidth(std::string(text), font);
}

std::string LogRenderer::FormatTimestamp(uint32_t timestamp) const {
    uint32_t ms = timestamp % 1000;
    uint32_t total_sec = timestamp / 1000;
    uint32_t sec = total_sec % 60;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t hour = total_sec / 3600;

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u.%03u",
                  hour, min, sec, ms);
    return std::string(buffer);
}

float LogRenderer::ComputeEntryWidth(size_t log_index) const {
    if (!buffer_ || log_index >= buffer_->size()) {
        return 0.0f;
    }

    auto entry = buffer_->GetEntry(log_index);
    SkFont font(typeface_, font_size_);
    float width = 0.0f;

    if (config_.show_timestamp) {
        width += MeasureTextRun(FormatTimestamp(entry.timestamp()), font) + cell_width_;
    }

    if (config_.show_level) {
        std::string text = std::string("[") + LogLevelToString(entry.level()) + "]";
        width += MeasureTextRun(text, font) + cell_width_;
    }

    if (config_.show_source) {
        const std::string& source = buffer_->GetSourceName(entry.source_id());
        if (!source.empty()) {
            width += MeasureTextRun("[" + source + "]", font) + cell_width_;
        }
    }

    width += MeasureTextRun(entry.message(), font);
    return width;
}

float LogRenderer::ComputeMaxContentWidth() const {
    if (!buffer_) {
        return 0.0f;
    }

    if (!max_content_width_dirty_) {
        return cached_max_content_width_;
    }

    float max_width = 0.0f;
    int display_count = GetDisplayLineCount();
    for (int i = 0; i < display_count; ++i) {
        max_width = std::max(max_width, ComputeEntryWidth(DisplayIndexToLogIndex(i)));
    }

    cached_max_content_width_ = max_width;
    max_content_width_dirty_ = false;
    return cached_max_content_width_;
}

LogRenderer::LayoutResult LogRenderer::UpdateLayoutMetrics(const SkRect& bounds) {
    LayoutResult result;
    total_lines_ = GetDisplayLineCount();
    float content_width = ComputeMaxContentWidth();

    const float scrollbar_thickness = 8.0f;
    const float scrollbar_gap = 2.0f;

    float inner_width = std::max(0.0f, bounds.width() - 2 * padding_);
    float inner_height = std::max(0.0f, bounds.height() - 2 * padding_);

    while (true) {
        float viewport_width = std::max(
            0.0f, inner_width - (result.need_vertical_scrollbar
                                     ? (scrollbar_thickness + scrollbar_gap)
                                     : 0.0f));
        float viewport_height = std::max(
            0.0f, inner_height - (result.need_horizontal_scrollbar
                                      ? (scrollbar_thickness + scrollbar_gap)
                                      : 0.0f));

        int visible_lines = 0;
        if (line_height_ > 0) {
            visible_lines = static_cast<int>(viewport_height / line_height_);
            if (visible_lines < 1) visible_lines = 1;
        }

        bool new_need_vertical = total_lines_ > visible_lines;
        int horizontal_max = static_cast<int>(std::ceil(
            std::max(0.0f, content_width - viewport_width) /
            std::max(cell_width_, 1.0f)));
        bool new_need_horizontal = horizontal_max > 0;

        if (new_need_vertical == result.need_vertical_scrollbar &&
            new_need_horizontal == result.need_horizontal_scrollbar) {
            break;
        }

        result.need_vertical_scrollbar = new_need_vertical;
        result.need_horizontal_scrollbar = new_need_horizontal;
    }

    result.content_bounds = bounds;
    if (result.need_vertical_scrollbar) {
        result.content_bounds.fRight -= scrollbar_thickness + scrollbar_gap;
    }
    if (result.need_horizontal_scrollbar) {
        result.content_bounds.fBottom -= scrollbar_thickness + scrollbar_gap;
    }

    UpdateMetrics(result.content_bounds.height());
    total_lines_ = GetDisplayLineCount();
    float viewport_width =
        std::max(0.0f, result.content_bounds.width() - 2 * padding_);
    SetMaxHorizontalScrollOffset(static_cast<int>(std::ceil(
        std::max(0.0f, content_width - viewport_width) /
        std::max(cell_width_, 1.0f))));

    return result;
}

void LogRenderer::RenderScrollbar(SkCanvas* canvas, const SkRect& content_bounds,
                                  bool has_horizontal_scrollbar) {
    if (total_lines_ <= visible_lines_) {
        return;
    }

    const float scrollbar_width = 8.0f;
    const float min_thumb_height = 20.0f;
    const float scrollbar_margin = 2.0f;

    float track_x = content_bounds.right() + scrollbar_margin;
    float track_y = content_bounds.top() + padding_;
    float track_height = content_bounds.height() - 2 * padding_;
    if (has_horizontal_scrollbar) {
        track_height = std::max(0.0f, track_height);
    }
    if (track_height <= 0) {
        return;
    }

    SkPaint track_paint;
    track_paint.setColor(SkColorSetARGB(60, 255, 255, 255));
    track_paint.setAntiAlias(true);
    SkRect track_rect =
        SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height);
    canvas->drawRoundRect(track_rect, 4.0f, 4.0f, track_paint);

    float content_ratio = static_cast<float>(visible_lines_) / total_lines_;
    float thumb_height = track_height * content_ratio;
    if (thumb_height < min_thumb_height) thumb_height = min_thumb_height;
    if (thumb_height > track_height) thumb_height = track_height;

    int max_scroll = max_scroll_offset();
    if (max_scroll < 1) max_scroll = 1;
    float scroll_ratio = static_cast<float>(scroll_offset_) / max_scroll;
    if (scroll_ratio > 1.0f) scroll_ratio = 1.0f;

    float available_track = track_height - thumb_height;
    float thumb_y = track_y + scroll_ratio * available_track;

    SkPaint thumb_paint;
    thumb_paint.setColor(SkColorSetARGB(150, 200, 200, 200));
    thumb_paint.setAntiAlias(true);
    SkRect thumb_rect =
        SkRect::MakeXYWH(track_x, thumb_y, scrollbar_width, thumb_height);
    canvas->drawRoundRect(thumb_rect, 4.0f, 4.0f, thumb_paint);
}

void LogRenderer::RenderHorizontalScrollbar(SkCanvas* canvas,
                                            const SkRect& content_bounds,
                                            bool has_vertical_scrollbar) {
    if (max_horizontal_scroll_offset() <= 0) {
        return;
    }

    const float scrollbar_height = 8.0f;
    const float min_thumb_width = 20.0f;
    const float scrollbar_margin = 2.0f;
    float track_x = content_bounds.left() + padding_;
    float track_y = content_bounds.bottom() + scrollbar_margin;
    float track_width = content_bounds.width() - 2 * padding_;
    if (track_width <= 0) {
        return;
    }

    SkPaint track_paint;
    track_paint.setColor(SkColorSetARGB(60, 255, 255, 255));
    track_paint.setAntiAlias(true);
    SkRect track_rect =
        SkRect::MakeXYWH(track_x, track_y, track_width, scrollbar_height);
    canvas->drawRoundRect(track_rect, 4.0f, 4.0f, track_paint);

    float total_columns = static_cast<float>(max_horizontal_scroll_offset()) +
                          std::max(1.0f, track_width / std::max(cell_width_, 1.0f));
    float visible_columns =
        std::max(1.0f, track_width / std::max(cell_width_, 1.0f));
    float thumb_width = track_width * (visible_columns / total_columns);
    thumb_width = std::max(thumb_width, min_thumb_width);
    if (thumb_width > track_width) thumb_width = track_width;

    float available_track = track_width - thumb_width;
    float scroll_ratio = max_horizontal_scroll_offset() > 0
                             ? static_cast<float>(horizontal_scroll_offset()) /
                                   max_horizontal_scroll_offset()
                             : 0.0f;
    scroll_ratio = std::clamp(scroll_ratio, 0.0f, 1.0f);
    float thumb_x = track_x + scroll_ratio * available_track;

    SkPaint thumb_paint;
    thumb_paint.setColor(SkColorSetARGB(150, 200, 200, 200));
    thumb_paint.setAntiAlias(true);
    SkRect thumb_rect =
        SkRect::MakeXYWH(thumb_x, track_y, thumb_width, scrollbar_height);
    canvas->drawRoundRect(thumb_rect, 4.0f, 4.0f, thumb_paint);
}

}  // namespace mbink
