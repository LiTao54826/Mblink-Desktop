/**
 * @file log_renderer.cpp
 * @brief 日志渲染器实现
 */

#include "log_renderer.h"

#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace mbink {

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
}

void LogRenderer::SetBuffer(LogBuffer* buffer) {
    buffer_ = buffer;
}

void LogRenderer::SetFilteredIndices(const std::vector<size_t>* indices) {
    filtered_indices_ = indices;
}

void LogRenderer::SetSearch(const LogSearch* search) {
    search_ = search;
}

void LogRenderer::SetSelection(const virtual_text::SelectionManager* selection) {
    selection_ = selection;
}

void LogRenderer::Render(SkCanvas* canvas, const SkRect& bounds) {
    if (!buffer_ || !canvas) {
        return;
    }

    // 更新度量
    UpdateMetrics(bounds.height());
    total_lines_ = GetDisplayLineCount();

    // 绘制背景
    SkPaint bg_paint;
    bg_paint.setColor(config_.background_color);
    canvas->drawRect(bounds, bg_paint);

    if (total_lines_ == 0) {
        return;
    }

    // 计算可见范围
    int start_line = scroll_offset_;
    int end_line = std::min(start_line + visible_lines_ + 1, total_lines_);

    // 渲染可见行
    for (int i = start_line; i < end_line; ++i) {
        size_t log_index = DisplayIndexToLogIndex(i);
        SkRect line_rect = GetLineRect(i - start_line, bounds);

        // 检查是否是当前搜索匹配
        bool is_current = false;
        if (search_ && search_->current_match() >= 0) {
            int match_idx = search_->current_match();
            if (match_idx < static_cast<int>(search_->matches().size())) {
                is_current = search_->matches()[match_idx].log_index == log_index;
            }
        }

        RenderLine(canvas, log_index, line_rect, is_current);
    }

    // 渲染滚动条
    RenderScrollbar(canvas, bounds);
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

    float x = line_rect.left() + padding_;
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
    SkPaint paint;
    paint.setColor(config_.timestamp_color);
    paint.setAntiAlias(true);

    canvas->drawString(ts.c_str(), x, y, font, paint);

    return x + ts.length() * cell_width_;
}

float LogRenderer::RenderLevel(SkCanvas* canvas, LogLevel level,
                               float x, float y) {
    const char* level_str = LogLevelToString(level);
    int level_idx = static_cast<int>(level);
    if (level_idx < 0 || level_idx > 4) level_idx = 1;

    SkFont font(typeface_, font_size_);
    SkPaint paint;
    paint.setColor(config_.level_colors[level_idx]);
    paint.setAntiAlias(true);

    // 绘制带括号的级别
    std::string text = std::string("[") + level_str + "]";
    canvas->drawString(text.c_str(), x, y, font, paint);

    return x + text.length() * cell_width_;
}

float LogRenderer::RenderSource(SkCanvas* canvas, const std::string& source,
                                float x, float y) {
    SkFont font(typeface_, font_size_);
    SkPaint paint;
    paint.setColor(config_.source_color);
    paint.setAntiAlias(true);

    std::string text = "[" + source + "]";
    canvas->drawString(text.c_str(), x, y, font, paint);

    return x + text.length() * cell_width_;
}

void LogRenderer::RenderMessage(SkCanvas* canvas, size_t log_index,
                                std::string_view message, LogLevel level,
                                float x, float y, float max_width) {
    SkFont font(typeface_, font_size_);

    int level_idx = static_cast<int>(level);
    if (level_idx < 0 || level_idx > 4) level_idx = 1;
    SkColor text_color = config_.level_colors[level_idx];

    // 获取该条目的搜索匹配
    std::vector<SearchMatch> matches;
    if (search_ && search_->HasSearch()) {
        matches = search_->GetMatchesForEntry(log_index);
    }

    if (matches.empty()) {
        // 无匹配，直接绘制
        SkPaint paint;
        paint.setColor(text_color);
        paint.setAntiAlias(true);

        std::string msg(message);
        canvas->drawString(msg.c_str(), x, y, font, paint);
    } else {
        // 有匹配，分段绘制
        size_t pos = 0;
        float current_x = x;

        for (const auto& match : matches) {
            // 绘制匹配前的文本
            if (match.start_pos > pos) {
                std::string before(message.substr(pos, match.start_pos - pos));
                SkPaint paint;
                paint.setColor(text_color);
                paint.setAntiAlias(true);
                canvas->drawString(before.c_str(), current_x, y, font, paint);
                current_x += before.length() * cell_width_;
            }

            // 绘制匹配高亮背景
            std::string match_text(message.substr(match.start_pos, match.length));
            float match_width = match_text.length() * cell_width_;

            bool is_current = search_->IsCurrentMatch(log_index, match.start_pos);
            SkPaint bg_paint;
            bg_paint.setColor(is_current ? config_.current_match_color
                                         : config_.match_color);

            SkRect match_rect = SkRect::MakeXYWH(
                current_x, y - line_height_ + padding_,
                match_width, line_height_);
            canvas->drawRect(match_rect, bg_paint);

            // 绘制匹配文本
            SkPaint paint;
            paint.setColor(text_color);
            paint.setAntiAlias(true);
            canvas->drawString(match_text.c_str(), current_x, y, font, paint);
            current_x += match_width;

            pos = match.start_pos + match.length;
        }

        // 绘制剩余文本
        if (pos < message.length()) {
            std::string after(message.substr(pos));
            SkPaint paint;
            paint.setColor(text_color);
            paint.setAntiAlias(true);
            canvas->drawString(after.c_str(), current_x, y, font, paint);
        }
    }
}

std::string LogRenderer::FormatTimestamp(uint32_t timestamp) const {
    uint32_t ms = timestamp % 1000;
    uint32_t total_sec = timestamp / 1000;
    uint32_t sec = total_sec % 60;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t hour = total_sec / 3600;

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour << ":"
        << std::setw(2) << min << ":"
        << std::setw(2) << sec << "."
        << std::setw(3) << ms;
    return oss.str();
}

void LogRenderer::RenderScrollbar(SkCanvas* canvas, const SkRect& bounds) {
    // 只有当实际内容超出可见区域时才显示滚动条
    if (total_lines_ <= visible_lines_) {
        return;
    }
    
    const float scrollbar_width = 8.0f;
    const float min_thumb_height = 20.0f;
    
    // 滚动条轨道区域
    float track_x = bounds.right() - scrollbar_width - 2.0f;
    float track_y = bounds.top() + padding_;
    float track_height = bounds.height() - 2 * padding_;
    
    // 绘制滚动条轨道背景
    SkPaint track_paint;
    track_paint.setColor(SkColorSetARGB(60, 255, 255, 255));
    track_paint.setAntiAlias(true);
    SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height);
    canvas->drawRoundRect(track_rect, 4.0f, 4.0f, track_paint);
    
    // 计算滚动条滑块大小和位置
    float content_ratio = static_cast<float>(visible_lines_) / total_lines_;
    float thumb_height = track_height * content_ratio;
    if (thumb_height < min_thumb_height) thumb_height = min_thumb_height;
    
    // 计算滑块位置
    int max_scroll = max_scroll_offset();
    if (max_scroll < 1) max_scroll = 1;
    float scroll_ratio = static_cast<float>(scroll_offset_) / max_scroll;
    if (scroll_ratio > 1.0f) scroll_ratio = 1.0f;
    
    // 计算滑块位置，确保不超出轨道
    float available_track = track_height - thumb_height;
    float thumb_y = track_y + scroll_ratio * available_track;
    
    // 绘制滚动条滑块
    SkPaint thumb_paint;
    thumb_paint.setColor(SkColorSetARGB(150, 200, 200, 200));
    thumb_paint.setAntiAlias(true);
    SkRect thumb_rect = SkRect::MakeXYWH(track_x, thumb_y, scrollbar_width, thumb_height);
    canvas->drawRoundRect(thumb_rect, 4.0f, 4.0f, thumb_paint);
}

}  // namespace mbink
