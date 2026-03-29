/**
 * @file virtual_scroll_renderer.cpp
 * @brief 虚拟滚动渲染基类实现
 */

#include "virtual_scroll_renderer.h"

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkTypeface.h"

#include <algorithm>

namespace mbink {

VirtualScrollRenderer::VirtualScrollRenderer() {
    // 使用空字体作为默认值，实际使用时应通过 SetFont 设置
    typeface_ = SkTypeface::MakeEmpty();
    UpdateCellMetrics();
}

void VirtualScrollRenderer::SetFont(sk_sp<SkTypeface> typeface, float size) {
    typeface_ = std::move(typeface);
    font_size_ = size;
    UpdateCellMetrics();
}

void VirtualScrollRenderer::SetLineHeight(float height) {
    line_height_ = height;
}

void VirtualScrollRenderer::SetPadding(float padding) {
    padding_ = padding;
}

void VirtualScrollRenderer::SetScrollOffset(int line_offset) {
    scroll_offset_ = line_offset;
    ClampScrollOffset();
}

void VirtualScrollRenderer::ScrollTo(int line) {
    scroll_offset_ = line;
    ClampScrollOffset();
}

void VirtualScrollRenderer::ScrollBy(int delta) {
    scroll_offset_ += delta;
    ClampScrollOffset();
}

void VirtualScrollRenderer::SetTotalLines(int total) {
    total_lines_ = total;
    ClampScrollOffset();
}

void VirtualScrollRenderer::UpdateMetrics(float view_height) {
    if (line_height_ > 0) {
        // 计算能完全显示的行数（不含部分可见的行）
        visible_lines_ = static_cast<int>((view_height - 2 * padding_) / line_height_);
        if (visible_lines_ < 1) visible_lines_ = 1;
    } else {
        visible_lines_ = 0;
    }
    ClampScrollOffset();
}

int VirtualScrollRenderer::HitTestLine(float y) const {
    if (line_height_ <= 0) {
        return 0;
    }
    int relative_line = static_cast<int>((y - padding_) / line_height_);
    return scroll_offset_ + relative_line;
}

int VirtualScrollRenderer::HitTestColumn(float x) const {
    if (cell_width_ <= 0) {
        return 0;
    }
    return static_cast<int>((x - padding_) / cell_width_);
}

void VirtualScrollRenderer::UpdateCellMetrics() {
    if (!typeface_) {
        cell_width_ = font_size_ * 0.6f;  // 估算值
        line_height_ = font_size_ * 1.2f;
        return;
    }

    SkFont font(typeface_, font_size_);
    
    // 计算字符宽度（使用 'M' 作为参考）
    SkGlyphID glyphs[1];
    const char* text = "M";
    int glyph_count = font.textToGlyphs(text, 1, SkTextEncoding::kUTF8, glyphs, 1);
    
    if (glyph_count > 0) {
        SkScalar widths[1];
        font.getWidths(glyphs, 1, widths);
        cell_width_ = widths[0] > 0 ? widths[0] : font_size_ * 0.6f;
    } else {
        cell_width_ = font_size_ * 0.6f;
    }

    // 计算行高
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    
    // 如果没有手动设置行高，使用字体度量
    if (line_height_ <= 0) {
        line_height_ = metrics.fDescent - metrics.fAscent + metrics.fLeading;
        if (line_height_ <= 0) {
            line_height_ = font_size_ * 1.2f;
        }
    }
}

SkRect VirtualScrollRenderer::GetLineRect(int line_index, const SkRect& bounds) const {
    float y = bounds.top() + padding_ + line_index * line_height_;
    return SkRect::MakeXYWH(
        bounds.left() + padding_,
        y,
        bounds.width() - 2 * padding_,
        line_height_
    );
}

void VirtualScrollRenderer::ClampScrollOffset() {
    // 确保滚动偏移不小于 0
    if (scroll_offset_ < 0) scroll_offset_ = 0;
    
    // 确保滚动偏移不超过最大值
    int max_offset = max_scroll_offset();
    if (scroll_offset_ > max_offset) scroll_offset_ = max_offset;
}

}  // namespace mbink
