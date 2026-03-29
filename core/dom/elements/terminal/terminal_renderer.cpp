/**
 * @file terminal_renderer.cpp
 * @brief 终端渲染器实现
 */

#include "terminal_renderer.h"

#include "core/render/text/font_manager.h"

#include "include/core/SkFont.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"

#include <algorithm>
#include <iostream>

namespace mbink {

TerminalRenderer::TerminalRenderer() {
    palette_ = ColorPalette::Default();
    
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
    
    // 获取 CJK 字体用于中文显示
    cjk_typeface_ = font_manager.GetCJKTypeface(SkFontStyle::Normal());
    
    font_size_ = 14.0f;
    UpdateCellMetrics();
}

void TerminalRenderer::SetSelection(int start_row, int start_col, 
                                     int end_row, int end_col) {
    has_selection_ = true;
    
    // 规范化：确保 start <= end
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        sel_start_row_ = end_row;
        sel_start_col_ = end_col;
        sel_end_row_ = start_row;
        sel_end_col_ = start_col;
    } else {
        sel_start_row_ = start_row;
        sel_start_col_ = start_col;
        sel_end_row_ = end_row;
        sel_end_col_ = end_col;
    }
}

void TerminalRenderer::ClearSelection() {
    has_selection_ = false;
}

void TerminalRenderer::Render(SkCanvas* canvas, const SkRect& bounds) {
    if (!buffer_) {
        // 没有 buffer 时显示提示
        SkPaint debug_paint;
        debug_paint.setColor(SK_ColorRED);
        SkFont font(typeface_, 14);
        canvas->drawString("No buffer", bounds.left() + 10, bounds.top() + 20, font, debug_paint);
        return;
    }

    // 更新总行数（使用缓冲区总行数用于渲染）
    SetTotalLines(buffer_->total_lines());
    UpdateMetrics(bounds.height());

    // 绘制背景 - 使用 ceil 确保完全覆盖，避免浮点误差导致的白线
    SkPaint bg_paint;
    bg_paint.setColor(default_bg_);
    SkRect bg_bounds = SkRect::MakeXYWH(
        std::floor(bounds.left()),
        std::floor(bounds.top()),
        std::ceil(bounds.width()) + 1,
        std::ceil(bounds.height()) + 1
    );
    canvas->drawRect(bg_bounds, bg_paint);

    // 渲染可见行（+1 处理部分可见的行）
    int start_line = scroll_offset_;
    int end_line = scroll_offset_ + visible_lines_ + 1;
    if (end_line > buffer_->total_lines()) {
        end_line = buffer_->total_lines();
    }

    for (int row = start_line; row < end_line; ++row) {
        int visual_row = row - start_line;
        SkRect line_rect = GetLineRect(visual_row, bounds);
        RenderLine(canvas, row, line_rect);
    }

    // 渲染光标
    if (show_cursor_) {
        RenderCursor(canvas, bounds);
    }
    
    // 渲染滚动条（使用实际内容行数判断是否需要显示）
    RenderScrollbar(canvas, bounds);
}

void TerminalRenderer::RenderLine(SkCanvas* canvas, int row, 
                                   const SkRect& line_rect) {
    if (!buffer_ || row < 0 || row >= buffer_->total_lines()) {
        return;
    }

    SkFont font(typeface_, font_size_);
    // 从 line_rect 的左边开始，加上 padding
    float x = line_rect.left() + padding_;
    float baseline = line_rect.top() + line_height_ * 0.8f;  // 近似基线

    int cols = buffer_->cols();
    for (int col = 0; col < cols; ++col) {
        const Cell& cell = buffer_->GetCell(row, col);
        
        // 跳过宽字符的占位符（width == 0）
        if (cell.width == 0) {
            x += cell_width_;
            continue;
        }
        
        bool selected = IsSelected(row, col);
        
        // 宽字符需要两倍宽度
        float render_width = (cell.width == 2) ? cell_width_ * 2 : cell_width_;
        RenderCell(canvas, cell, x, baseline, selected, render_width);
        
        x += cell_width_;
    }
}

void TerminalRenderer::RenderCell(SkCanvas* canvas, const Cell& cell,
                                   float x, float y, bool selected,
                                   float render_width) {
    SkPaint paint;
    
    // 根据字符类型选择字体
    sk_sp<SkTypeface> use_typeface = typeface_;
    if (FontManager::IsCJK(cell.codepoint) && cjk_typeface_) {
        use_typeface = cjk_typeface_;
    }
    SkFont font(use_typeface, font_size_);

    // 获取颜色
    SkColor fg = ResolveColor(cell.style, true);
    SkColor bg = ResolveColor(cell.style, false);

    // 反色处理
    if (cell.style.IsInverse()) {
        std::swap(fg, bg);
    }

    // 选中时反色
    if (selected) {
        std::swap(fg, bg);
    }

    // 绘制背景（如果不是默认背景）
    if (bg != default_bg_ || selected) {
        paint.setColor(bg);
        canvas->drawRect(SkRect::MakeXYWH(x, y - line_height_ * 0.8f, 
                                          render_width, line_height_), paint);
    }

    // 绘制字符
    if (cell.codepoint != ' ' && cell.codepoint != 0 && !cell.style.IsHidden()) {
        // 使用前景色
        paint.setColor(fg);
        paint.setAntiAlias(true);
        
        // 暗淡处理
        if (cell.style.IsDim()) {
            paint.setAlpha(128);
        }

        // 粗体
        if (cell.style.IsBold()) {
            font.setEmbolden(true);
        }

        // 斜体
        if (cell.style.IsItalic()) {
            font.setSkewX(-0.25f);
        }

        // 转换为 UTF-8
        char utf8[5] = {0};
        if (cell.codepoint < 0x80) {
            utf8[0] = static_cast<char>(cell.codepoint);
        } else if (cell.codepoint < 0x800) {
            utf8[0] = static_cast<char>(0xC0 | (cell.codepoint >> 6));
            utf8[1] = static_cast<char>(0x80 | (cell.codepoint & 0x3F));
        } else if (cell.codepoint < 0x10000) {
            utf8[0] = static_cast<char>(0xE0 | (cell.codepoint >> 12));
            utf8[1] = static_cast<char>(0x80 | ((cell.codepoint >> 6) & 0x3F));
            utf8[2] = static_cast<char>(0x80 | (cell.codepoint & 0x3F));
        } else {
            utf8[0] = static_cast<char>(0xF0 | (cell.codepoint >> 18));
            utf8[1] = static_cast<char>(0x80 | ((cell.codepoint >> 12) & 0x3F));
            utf8[2] = static_cast<char>(0x80 | ((cell.codepoint >> 6) & 0x3F));
            utf8[3] = static_cast<char>(0x80 | (cell.codepoint & 0x3F));
        }

        canvas->drawString(utf8, x, y, font, paint);

        // 下划线
        if (cell.style.IsUnderline()) {
            paint.setStrokeWidth(1);
            canvas->drawLine(x, y + 2, x + render_width, y + 2, paint);
        }

        // 删除线
        if (cell.style.IsStrikethrough()) {
            paint.setStrokeWidth(1);
            float strike_y = y - line_height_ * 0.3f;
            canvas->drawLine(x, strike_y, x + render_width, strike_y, paint);
        }
    }
}

void TerminalRenderer::RenderCursor(SkCanvas* canvas, const SkRect& bounds) {
    if (!buffer_) {
        return;
    }

    // 获取光标的缓冲区行号
    int cursor_buffer_row = buffer_->cursor_buffer_row();
    int cursor_col = buffer_->cursor_col();

    // 检查光标是否在可见区域
    if (cursor_buffer_row < scroll_offset_ || 
        cursor_buffer_row >= scroll_offset_ + visible_lines_) {
        return;
    }

    int visual_row = cursor_buffer_row - scroll_offset_;
    float x = bounds.left() + padding_ + cursor_col * cell_width_;
    float y = bounds.top() + padding_ + visual_row * line_height_;

    SkPaint paint;
    paint.setColor(default_fg_);

    if (cursor_block_) {
        if (is_focused_) {
            // 有焦点：实心块状光标（半透明）
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAlpha(180);
            canvas->drawRect(SkRect::MakeXYWH(x, y, cell_width_, line_height_), paint);
        } else {
            // 无焦点：空心框光标
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(1.5f);
            canvas->drawRect(SkRect::MakeXYWH(x, y, cell_width_, line_height_), paint);
        }
    } else {
        // 下划线光标
        paint.setStrokeWidth(is_focused_ ? 2.0f : 1.0f);
        canvas->drawLine(x, y + line_height_ - 2, 
                        x + cell_width_, y + line_height_ - 2, paint);
    }
}

SkColor TerminalRenderer::ResolveColor(const TextStyle& style, 
                                        bool is_foreground) const {
    uint8_t index = is_foreground ? style.fg_color : style.bg_color;
    
    // 使用调色板
    return palette_.Resolve(index);
}

bool TerminalRenderer::IsSelected(int row, int col) const {
    if (!has_selection_) {
        return false;
    }

    // 检查是否在选择范围内
    if (row < sel_start_row_ || row > sel_end_row_) {
        return false;
    }

    if (row == sel_start_row_ && col < sel_start_col_) {
        return false;
    }

    if (row == sel_end_row_ && col >= sel_end_col_) {
        return false;
    }

    return true;
}

void TerminalRenderer::RenderScrollbar(SkCanvas* canvas, const SkRect& bounds) {
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
    
    // 计算滑块位置 - 使用统一的 max_scroll_offset()
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
