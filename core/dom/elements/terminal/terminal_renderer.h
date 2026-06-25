/**
 * @file terminal_renderer.h
 * @brief 终端渲染器
 *
 * 使用 Skia 渲染终端内容，支持虚拟滚动。
 */

#pragma once

#include "terminal_buffer.h"
#include "core/dom/elements/virtual_text/text_style.h"
#include "core/dom/elements/virtual_text/virtual_scroll_renderer.h"

#include "include/core/SkRefCnt.h"

class SkTypeface;

namespace mblink {

/**
 * @brief 终端渲染器
 *
 * 继承自 VirtualScrollRenderer，实现终端特定的渲染逻辑：
 * - 单元格渲染（字符 + 样式）
 * - 颜色调色板
 * - 光标渲染
 * - 选择高亮
 */
class TerminalRenderer : public VirtualScrollRenderer {
public:
    TerminalRenderer();

    // === 配置 ===

    /**
     * @brief 设置缓冲区
     * @param buffer 终端缓冲区指针
     */
    void SetBuffer(TerminalBuffer* buffer) { buffer_ = buffer; }

    /**
     * @brief 设置颜色调色板
     * @param palette 调色板
     */
    void SetPalette(const ColorPalette& palette) { palette_ = palette; }

    /**
     * @brief 设置默认颜色
     * @param fg 前景色
     * @param bg 背景色
     */
    void SetDefaultColors(SkColor fg, SkColor bg) {
        default_fg_ = fg;
        default_bg_ = bg;
    }

    /**
     * @brief 设置是否显示光标
     * @param show 是否显示
     */
    void SetShowCursor(bool show) { show_cursor_ = show; }

    /**
     * @brief 设置光标样式
     * @param block 是否为块状光标（否则为下划线）
     */
    void SetCursorBlock(bool block) { cursor_block_ = block; }

    /**
     * @brief 设置焦点状态
     * @param focused 是否有焦点
     */
    void SetFocused(bool focused) { is_focused_ = focused; }

    /**
     * @brief 获取焦点状态
     */
    bool IsFocused() const { return is_focused_; }

    // === 选择 ===

    /**
     * @brief 设置选择范围
     * @param start_row 起始行
     * @param start_col 起始列
     * @param end_row 结束行
     * @param end_col 结束列
     */
    void SetSelection(int start_row, int start_col, int end_row, int end_col);

    /**
     * @brief 清除选择
     */
    void ClearSelection();

    // === 渲染 ===

    /**
     * @brief 渲染终端内容
     * @param canvas Skia 画布
     * @param bounds 渲染区域
     */
    void Render(SkCanvas* canvas, const SkRect& bounds) override;

    bool has_vertical_scrollbar() const { return has_vertical_scrollbar_; }
    bool has_horizontal_scrollbar() const { return has_horizontal_scrollbar_; }

private:
    TerminalBuffer* buffer_ = nullptr;
    ColorPalette palette_;
    SkColor default_fg_ = SK_ColorWHITE;
    SkColor default_bg_ = SK_ColorBLACK;
    bool show_cursor_ = true;
    bool cursor_block_ = true;
    bool is_focused_ = false;
    
    // CJK 字体（用于中文等宽字符）
    sk_sp<SkTypeface> cjk_typeface_;
    bool has_vertical_scrollbar_ = false;
    bool has_horizontal_scrollbar_ = false;

    // 选择范围
    bool has_selection_ = false;
    int sel_start_row_ = 0;
    int sel_start_col_ = 0;
    int sel_end_row_ = 0;
    int sel_end_col_ = 0;

    /**
     * @brief 渲染单行
     * @param canvas 画布
     * @param row 行号（缓冲区中的绝对行号）
     * @param line_rect 行的渲染区域
     */
    void RenderLine(SkCanvas* canvas, int row, const SkRect& line_rect);

    /**
     * @brief 渲染单元格
     * @param canvas 画布
     * @param cell 单元格
     * @param x X 坐标
     * @param y Y 坐标（基线）
     * @param selected 是否被选中
     * @param render_width 渲染宽度（宽字符为 2 倍单元格宽度）
     */
    void RenderCell(SkCanvas* canvas, const Cell& cell, 
                    float x, float y, bool selected, float render_width);

    /**
     * @brief 渲染光标
     * @param canvas 画布
     * @param bounds 渲染区域
     */
    void RenderCursor(SkCanvas* canvas, const SkRect& bounds);

    /**
     * @brief 渲染滚动条
     * @param canvas 画布
     * @param content_bounds 实际内容区域
     * @param has_horizontal_scrollbar 是否存在横向滚动条
     */
    void RenderScrollbar(SkCanvas* canvas, const SkRect& content_bounds,
                         bool has_horizontal_scrollbar);

    /**
     * @brief 渲染横向滚动条
     * @param canvas 画布
     * @param content_bounds 实际内容区域
     * @param has_vertical_scrollbar 是否存在纵向滚动条
     */
    void RenderHorizontalScrollbar(SkCanvas* canvas, const SkRect& content_bounds,
                                   bool has_vertical_scrollbar);

    /**
     * @brief 解析颜色
     * @param style 样式
     * @param is_foreground 是否为前景色
     * @return Skia 颜色
     */
    SkColor ResolveColor(const TextStyle& style, bool is_foreground) const;

    /**
     * @brief 检查位置是否被选中
     * @param row 行号
     * @param col 列号
     * @return 是否被选中
     */
    bool IsSelected(int row, int col) const;
};

}  // namespace mblink
