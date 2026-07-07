/**
 * @file render_block.cpp
 * @brief RenderBlock 类实现
 *
 * 从 render_object.cpp 提取的块级元素渲染对象实现。
 * 包含 RenderBlock::Layout, Paint, PaintContentEditableCaret 方法。
 * 表单元素绘制委托给 FormElementPainter。
 *
 * @note 大文件说明 (约1900行)
 * 本文件包含 RenderBlock 的完整实现，是块级元素渲染的核心。
 * 文件较大的原因：
 * 1. Layout 方法包含复杂的块级布局逻辑
 * 2. Paint 方法包含完整的渲染管线（背景、边框、阴影、滚动条等）
 * 3. PaintContentEditableCaret 包含复杂的光标定位逻辑
 */

#include "render_object.h"
#include "render_inline_block.h"
#include "positioned_layout.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/utils/gradient_renderer.h"
#include "core/render/utils/shadow_renderer.h"
#include "core/render/utils/color.h"
#include "list_marker.h"
#include "core/render/painters/background_painter.h"
#include "core/render/painters/border_painter.h"
#include "core/render/painters/scrollbar_painter.h"
#include "core/render/painters/form_element_painter.h"
#include "core/render/painters/audio_element_painter.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/dom/selection/selection.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_select_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_canvas_element.h"
#include "core/dom/elements/html_image_element.h"
#include "core/dom/elements/html_audio_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include "core/render/image/image_fit.h"
#include "core/render/image/image_loader.h"
#include "core/editing/contenteditable_geometry.h"
#include "core/render/input/textarea_painter.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "include/core/SkSurface.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef DrawText
#undef min
#undef max
#undef ERROR  // 防止与 LogLevel::ERROR 冲突
#endif

namespace mblink {

#ifdef _WIN32
static void PrintCallStack();
#endif


// 🐛 调试辅助函数：打印调用栈
#ifdef _WIN32
static void PrintCallStack() {
    static bool sym_initialized = false;
    if (!sym_initialized) {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        sym_initialized = true;
    }

    void* stack[20];
    unsigned short frames = CaptureStackBackTrace(0, 20, stack, NULL);

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    for (unsigned short i = 0; i < frames; i++) {
        DWORD64 address = (DWORD64)(stack[i]);
        if (SymFromAddr(GetCurrentProcess(), address, 0, symbol)) {
        } else {
        }
    }
}
#else
static void PrintCallStack() {
}
#endif

// 外部全局变量声明（定义在 render_object.cpp）
extern std::atomic<long long> g_paint_bg_time;
extern std::atomic<long long> g_paint_shadow_time;
extern std::atomic<long long> g_paint_children_time;
extern std::atomic<int> g_paint_total_calls;
extern std::atomic<int> g_paint_culled_calls;

// ========== RenderBlock 实现 ==========

void RenderBlock::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // ✅ 检查是否是 flex 容器，如果是则使用 flex 布局
    if (style.display == RenderObjectType::FLEX) {
        LayoutAsFlex(parent_width, parent_height);
        return;
    }

    // 计算宽度
    float width = HasExternalLayoutWidth() ? GetExternalLayoutWidth() : parent_width;
    if (!HasExternalLayoutWidth() && !style.width.IsAuto()) {
        width = style.width.ToPx(parent_width, style.font_size);
    }

    // 应用 min-width 和 max-width
    if (!HasExternalLayoutWidth() && !style.min_width.IsZero()) {
        float min_w = style.min_width.ToPx(parent_width, style.font_size);
        width = std::max(width, min_w);
    }
    if (!HasExternalLayoutWidth() && style.max_width.unit != CSSUnit::NONE) {
        float max_w = style.max_width.ToPx(parent_width, style.font_size);
        width = std::min(width, max_w);
    }

    // 计算 padding
    float padding_left = style.padding.left.ToPx(width, style.font_size);
    float padding_right = style.padding.right.ToPx(width, style.font_size);
    float padding_top = style.padding.top.ToPx(width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(width, style.font_size);

    // 计算 border
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 计算内容区域宽度
    float content_width = width - padding_left - padding_right - border_left - border_right;

    // 布局子元素 - 第一遍：计算尺寸
    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();

        // position:absolute/fixed 子元素不参与正常流，但仍需布局以计算自身尺寸
        if (child_style.position == "absolute" || child_style.position == "fixed") {
            if (child->NeedsLayout()) {
                child->Layout(content_width, 0);
            }
            continue;
        }

        if (child->NeedsLayout()) {
            auto child_node = child->GetNode();
            bool is_legend = false;
            if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto child_elem = std::static_pointer_cast<Element>(child_node);
                is_legend = (child_elem->GetTagName() == "legend");
            }

            if (is_legend) {
                child->Layout(10000, 0);
                float max_right = 0.0f;
                for (const auto& grandchild : child->GetChildren()) {
                    auto& gc_layout = grandchild->GetLayoutInfo();
                    max_right = std::max(max_right, gc_layout.x + gc_layout.width);
                }
                auto& child_style_legend = child->GetComputedStyle();
                float legend_padding_right = child_style_legend.padding.right.ToPx();
                float legend_border_right = child_style_legend.border_right_width > 0 ?
                    child_style_legend.border_right_width : child_style_legend.border.width.ToPx();
                child->GetLayoutInfo().width = max_right + legend_padding_right + legend_border_right;
            } else {
                child->Layout(content_width, 0);
            }
        }
    }

    // 布局子元素 - 第二遍：设置位置
    float current_y = 0;
    float current_x = padding_left + border_left;
    float line_height = 0;

    bool is_fieldset = false;
    auto this_node = GetNode();
    if (this_node && this_node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto this_elem = std::static_pointer_cast<Element>(this_node);
        is_fieldset = (this_elem->GetTagName() == "fieldset");
    }

    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();
        auto& child_style = child->GetComputedStyle();

        // 跳过 position:absolute/fixed 子元素 - 它们不参与正常流定位
        if (child_style.position == "absolute" || child_style.position == "fixed") {
            continue;
        }

        float child_margin_top = child_style.margin.top.ToPx(width, child_style.font_size);
        float child_margin_bottom = child_style.margin.bottom.ToPx(width, child_style.font_size);
        float child_margin_left = child_style.margin.left.ToPx(width, child_style.font_size);
        float child_margin_right = child_style.margin.right.ToPx(width, child_style.font_size);

        bool is_legend = false;
        if (is_fieldset) {
            auto child_node = child->GetNode();
            if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto child_elem = std::static_pointer_cast<Element>(child_node);
                is_legend = (child_elem->GetTagName() == "legend");
            }
        }

        bool is_inline = (dynamic_cast<RenderInline*>(child.get()) != nullptr ||
                         dynamic_cast<RenderText*>(child.get()) != nullptr ||
                         dynamic_cast<RenderInlineBlock*>(child.get()) != nullptr);

        if (is_inline) {
            float child_width = child_layout.width + child_margin_left + child_margin_right;
            if (current_x + child_width > width - padding_right - border_right &&
                current_x > padding_left + border_left) {
                current_y += line_height;
                current_x = padding_left + border_left;
                line_height = 0;
            }
            float new_x = current_x + child_margin_left;
            child_layout.x = new_x;
            child_layout.y = current_y + padding_top + border_top + child_margin_top;
            current_x += child_width;
            line_height = std::max(line_height, child_layout.height);
        } else {
            if (current_x > padding_left + border_left) {
                current_y += line_height;
                current_x = padding_left + border_left;
                line_height = 0;
            }

            float new_x = padding_left + border_left + child_margin_left;
            if (style.text_align == "center") {
                float available_width = content_width - child_margin_left - child_margin_right;
                if (child_layout.width < available_width) {
                    new_x = padding_left + border_left + (available_width - child_layout.width) / 2.0f;
                }
            } else if (style.text_align == "right") {
                float available_width = content_width - child_margin_left - child_margin_right;
                if (child_layout.width < available_width) {
                    new_x = padding_left + border_left + available_width - child_layout.width - child_margin_right;
                }
            }

            float new_y = current_y + padding_top + border_top + child_margin_top;
            if (is_legend) {
                new_y = 0;
            }

            child_layout.x = new_x;
            child_layout.y = new_y;

            if (!is_legend) {
                current_y += child_margin_top + child_layout.height + child_margin_bottom;
            }
        }
    }

    if (current_x > padding_left + border_left) {
        current_y += line_height;
    }

    // 计算高度
    float height = 0;
    if (HasExternalLayoutHeight()) {
        height = GetExternalLayoutHeight();
    } else if (!style.height.IsAuto()) {
        height = style.height.ToPx(parent_height, style.font_size);
    } else {
        height = current_y + padding_top + padding_bottom + border_top + border_bottom;
    }

    if (!HasExternalLayoutHeight() && !style.min_height.IsZero()) {
        float min_h = style.min_height.ToPx(parent_height, style.font_size);
        height = std::max(height, min_h);
    }
    if (!HasExternalLayoutHeight() && style.max_height.unit != CSSUnit::NONE) {
        float max_h = style.max_height.ToPx(parent_height, style.font_size);
        height = std::min(height, max_h);
    }

    layout_info_.width = width;
    layout_info_.height = height;

    // 第三遍：处理 position:absolute/fixed 子元素的定位
    // absolute 子元素相对于最近的 positioned ancestor（即本元素）的 content box 定位
    float container_w = content_width;
    float container_h = height - padding_top - padding_bottom - border_top - border_bottom;

    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();
        if (child_style.position != "absolute" && child_style.position != "fixed") {
            continue;
        }

        LayoutPositionedChild(child, container_w, container_h,
                              padding_left + border_left,
                              padding_top + border_top);
    }

    layout_info_.content_rect = SkRect::MakeXYWH(
        padding_left + border_left, padding_top + border_top,
        content_width, current_y
    );

    layout_info_.padding_rect = SkRect::MakeXYWH(
        border_left, border_top,
        content_width + padding_left + padding_right,
        current_y + padding_top + padding_bottom
    );

    layout_info_.border_rect = SkRect::MakeXYWH(0, 0, width, height);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderBlock::LayoutAsFlex(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 计算宽度
    float width = HasExternalLayoutWidth() ? GetExternalLayoutWidth() : parent_width;
    if (!HasExternalLayoutWidth() && !style.width.IsAuto()) {
        width = style.width.ToPx(parent_width, style.font_size);
    }

    // 应用 min-width 和 max-width
    if (!HasExternalLayoutWidth() && !style.min_width.IsZero()) {
        float min_w = style.min_width.ToPx(parent_width, style.font_size);
        width = std::max(width, min_w);
    }
    if (!HasExternalLayoutWidth() && style.max_width.unit != CSSUnit::NONE) {
        float max_w = style.max_width.ToPx(parent_width, style.font_size);
        width = std::min(width, max_w);
    }

    // 计算 padding
    float padding_left = style.padding.left.ToPx(width, style.font_size);
    float padding_right = style.padding.right.ToPx(width, style.font_size);
    float padding_top = style.padding.top.ToPx(width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(width, style.font_size);

    // 计算 border
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    // 计算内容区域尺寸
    float content_width = width - padding_left - padding_right - border_left - border_right;

    // 计算高度
    float height = 0;
    float content_height = 0;
    if (HasExternalLayoutHeight()) {
        height = GetExternalLayoutHeight();
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    } else if (!style.height.IsAuto()) {
        height = style.height.ToPx(parent_height, style.font_size);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    }

    // 确定 flex 方向
    bool is_row = (style.flex_direction == "row" || style.flex_direction == "row-reverse");
    bool is_reverse = (style.flex_direction == "row-reverse" || style.flex_direction == "column-reverse");

    // 第一遍：布局所有子元素获取尺寸
    float total_main_size = 0;
    float max_cross_size = 0;

    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();

        // 跳过 position:absolute/fixed 子元素 - 它们不参与 flex 尺寸计算
        // 但仍需布局以计算自身尺寸
        if (child_style.position == "absolute" || child_style.position == "fixed") {
            if (child->NeedsLayout()) {
                child->Layout(content_width, content_height > 0 ? content_height : 0);
            }
            continue;
        }

        if (child->NeedsLayout()) {
            child->Layout(content_width, content_height > 0 ? content_height : 0);
        }

        auto& child_layout = child->GetLayoutInfo();

        float child_margin_main_start = is_row ?
            child_style.margin.left.ToPx(width, child_style.font_size) :
            child_style.margin.top.ToPx(width, child_style.font_size);
        float child_margin_main_end = is_row ?
            child_style.margin.right.ToPx(width, child_style.font_size) :
            child_style.margin.bottom.ToPx(width, child_style.font_size);
        float child_margin_cross_start = is_row ?
            child_style.margin.top.ToPx(width, child_style.font_size) :
            child_style.margin.left.ToPx(width, child_style.font_size);
        float child_margin_cross_end = is_row ?
            child_style.margin.bottom.ToPx(width, child_style.font_size) :
            child_style.margin.right.ToPx(width, child_style.font_size);

        float child_main_size = is_row ? child_layout.width : child_layout.height;
        float child_cross_size = is_row ? child_layout.height : child_layout.width;

        total_main_size += child_main_size + child_margin_main_start + child_margin_main_end;
        max_cross_size = std::max(max_cross_size, child_cross_size + child_margin_cross_start + child_margin_cross_end);
    }

    // 如果高度是 auto，根据内容计算
    if (!HasExternalLayoutHeight() && style.height.IsAuto()) {
        if (is_row) {
            content_height = max_cross_size;
        } else {
            content_height = total_main_size;
        }
        height = content_height + padding_top + padding_bottom + border_top + border_bottom;
    }

    // 应用 min-height 和 max-height
    if (!HasExternalLayoutHeight() && !style.min_height.IsZero()) {
        float min_h = style.min_height.ToPx(parent_height, style.font_size);
        height = std::max(height, min_h);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    }
    if (!HasExternalLayoutHeight() && style.max_height.unit != CSSUnit::NONE) {
        float max_h = style.max_height.ToPx(parent_height, style.font_size);
        height = std::min(height, max_h);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    }

    // 计算主轴可用空间
    float main_size = is_row ? content_width : content_height;
    float cross_size = is_row ? content_height : content_width;
    float free_space = main_size - total_main_size;

    // 根据 justify-content 计算主轴起始位置
    float main_start = 0;
    float gap = 0;
    // 只计算非 absolute/fixed 子元素数量
    size_t num_children = 0;
    for (auto& child : children_) {
        auto& cs = child->GetComputedStyle();
        if (cs.position != "absolute" && cs.position != "fixed") {
            num_children++;
        }
    }

    if (style.justify_content == "flex-start" || style.justify_content == "start") {
        main_start = is_reverse ? free_space : 0;
    } else if (style.justify_content == "flex-end" || style.justify_content == "end") {
        main_start = is_reverse ? 0 : free_space;
    } else if (style.justify_content == "center") {
        main_start = free_space / 2.0f;
    } else if (style.justify_content == "space-between" && num_children > 1) {
        main_start = 0;
        gap = free_space / (num_children - 1);
    } else if (style.justify_content == "space-around" && num_children > 0) {
        gap = free_space / num_children;
        main_start = gap / 2.0f;
    } else if (style.justify_content == "space-evenly" && num_children > 0) {
        gap = free_space / (num_children + 1);
        main_start = gap;
    }

    // 第二遍：设置子元素位置（跳过 absolute/fixed）
    float current_main = main_start;

    for (size_t i = 0; i < children_.size(); ++i) {
        size_t idx = is_reverse ? (children_.size() - 1 - i) : i;
        auto& child = children_[idx];
        auto& child_layout = child->GetLayoutInfo();
        auto& child_style = child->GetComputedStyle();

        // 跳过 position:absolute/fixed 子元素
        if (child_style.position == "absolute" || child_style.position == "fixed") {
            continue;
        }

        float child_margin_main_start = is_row ?
            child_style.margin.left.ToPx(width, child_style.font_size) :
            child_style.margin.top.ToPx(width, child_style.font_size);
        float child_margin_main_end = is_row ?
            child_style.margin.right.ToPx(width, child_style.font_size) :
            child_style.margin.bottom.ToPx(width, child_style.font_size);
        float child_margin_cross_start = is_row ?
            child_style.margin.top.ToPx(width, child_style.font_size) :
            child_style.margin.left.ToPx(width, child_style.font_size);
        float child_margin_cross_end = is_row ?
            child_style.margin.bottom.ToPx(width, child_style.font_size) :
            child_style.margin.right.ToPx(width, child_style.font_size);

        float child_main_size = is_row ? child_layout.width : child_layout.height;
        float child_cross_size = is_row ? child_layout.height : child_layout.width;

        // 计算交叉轴位置（根据 align-items）
        float cross_offset = 0;
        float cross_free_space = cross_size - child_cross_size - child_margin_cross_start - child_margin_cross_end;

        // 检查子元素的 align-self
        std::string align = child_style.align_self;
        if (align == "auto" || align.empty()) {
            align = style.align_items;
        }

        if (align == "flex-start" || align == "start") {
            cross_offset = 0;
        } else if (align == "flex-end" || align == "end") {
            cross_offset = cross_free_space;
        } else if (align == "center") {
            cross_offset = cross_free_space / 2.0f;
        } else if (align == "stretch") {
            cross_offset = 0;
        } else {
            cross_offset = 0;
        }

        // 设置子元素位置
        if (is_row) {
            child_layout.x = padding_left + border_left + current_main + child_margin_main_start;
            child_layout.y = padding_top + border_top + cross_offset + child_margin_cross_start;
        } else {
            child_layout.x = padding_left + border_left + cross_offset + child_margin_cross_start;
            child_layout.y = padding_top + border_top + current_main + child_margin_main_start;
        }

        current_main += child_margin_main_start + child_main_size + child_margin_main_end + gap;
    }

    // 设置布局信息
    layout_info_.width = width;
    layout_info_.height = height;

    // 第三遍：处理 position:absolute/fixed 子元素的定位
    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();
        if (child_style.position != "absolute" && child_style.position != "fixed") {
            continue;
        }

        LayoutPositionedChild(child, content_width, content_height,
                              padding_left + border_left,
                              padding_top + border_top);
    }

    layout_info_.content_rect = SkRect::MakeXYWH(
        padding_left + border_left, padding_top + border_top,
        content_width, content_height
    );

    layout_info_.padding_rect = SkRect::MakeXYWH(
        border_left, border_top,
        content_width + padding_left + padding_right,
        content_height + padding_top + padding_bottom
    );

    layout_info_.border_rect = SkRect::MakeXYWH(0, 0, width, height);
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}


void RenderBlock::Paint(SkCanvas* canvas) {
    if (!canvas) {
        return;
    }

    // 🐛 hover bug 调试日志
    static bool debug_hover_bug = std::getenv("MBLINK_DEBUG_HOVER_BUG") != nullptr;
    if (debug_hover_bug) {
        std::string tag = "unknown";
        std::string id = "";
        auto node_locked = node_.lock();
        if (node_locked) {
            if (node_locked->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto elem = std::dynamic_pointer_cast<Element>(node_locked);
                if (elem) {
                    tag = elem->GetTagName();
                    id = elem->GetAttribute("id");
                }
            } else if (node_locked->GetNodeType() == NodeType::TEXT_NODE) {
                tag = "#text";
            }
        }
    }

    // 跳过零高度元素（如 CodeMirror 的测量占位元素）
    // 但如果元素有绝对定位的子元素，仍然需要绘制（如 cm-selectionLayer）
    if (layout_info_.height <= 0) {
        // 检查是否有绝对定位的子元素需要渲染
        bool has_absolute_children = false;
        for (const auto& child : children_) {
            const auto& child_style = child->GetComputedStyle();
            if (child_style.position == "absolute" || child_style.position == "fixed") {
                has_absolute_children = true;
                break;
            }
        }
        if (!has_absolute_children) {
            needs_paint_ = false;
            return;
        }
    }

    // 统计：每次 Paint 调用
    extern std::atomic<int> g_paint_total_calls;
    extern std::atomic<int> g_paint_culled_calls;
    g_paint_total_calls++;

    // Enterprise-Grade Optimization: View Culling
    // Check if the object is visible in the current clip rect.
    // layout_info_ contains coordinates relative to the parent.
    // The canvas CTM is currently set to the parent's generic coordinate space.
    // So paint_rect matches the CTM directly.
    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);

    // 对于高度为0但有绝对定位子元素的容器（如 CodeMirror 的 cm-selectionLayer），
    // 不能跳过渲染，因为其绝对定位子元素可能需要渲染
    bool has_absolute_children_need_paint = false;
    if (layout_info_.height <= 0 || layout_info_.width <= 0) {
        for (const auto& child : children_) {
            const auto& child_style = child->GetComputedStyle();
            if (child_style.position == "absolute" || child_style.position == "fixed") {
                has_absolute_children_need_paint = true;
                break;
            }
        }
    }

    // Aggressive culling: Skip if completely outside the clip.
    // 但如果有绝对定位子元素且尺寸为0，不跳过
    if (!has_absolute_children_need_paint && canvas->quickReject(paint_rect.makeOutset(50, 50))) {
        g_paint_culled_calls++;  // 统计：被剔除的调用
        needs_paint_ = false;
        return;
    }

    // P1优化：更新绘制缓存（如果无效则重新计算）
    UpdatePaintCache();

    const auto& style = computed_style_;
    const auto& layout = layout_info_;
    const auto& cache = paint_cache_;  // 使用缓存的值

    // CSS visibility 处理
    // visibility: hidden 时，不绘制当前元素的内容，但仍需绘制子元素
    // 因为子元素可能有 visibility: visible 覆盖
    bool is_hidden = (style.visibility == "hidden");

    // 🐛 调试：追踪 clip 变化
    static bool debug_clip = std::getenv("MBLINK_DEBUG_SHADOW") != nullptr;
    if (debug_clip && style.position == "fixed") {
        SkRect local_clip = canvas->getLocalClipBounds();
        SkIRect device_clip = canvas->getDeviceClipBounds();
        SkMatrix matrix = canvas->getTotalMatrix();

        // 🐛 添加父元素信息
        auto parent = GetParent();
        if (parent) {
            const auto& parent_style = parent->GetComputedStyle();
            const auto& parent_layout = parent->GetLayoutInfo();
        } else {
        }

        // 🐛 添加调用栈
        PrintCallStack();

    }

    // 保存画布状态
    canvas->save();

    if (debug_clip && style.position == "fixed") {
        SkRect local_clip = canvas->getLocalClipBounds();
    }

    canvas->translate(layout.x, layout.y);

    if (debug_clip && style.position == "fixed") {
        SkRect local_clip = canvas->getLocalClipBounds();
    }

    // 应用 CSS opacity（使用 saveLayerAlpha 实现透明度）
    bool has_opacity = style.opacity < 1.0f;
    if (has_opacity) {
        // 关键修复：使用CSS指定的宽高，而不是layout.width/height
        // layout.width可能是shrink-to-fit的结果，不代表元素的实际渲染尺寸
        float width = layout.width;
        float height = layout.height;

        // 优先使用CSS明确指定的宽高
        if (style.width.unit == CSSUnit::PX && style.width.value > 0) {
            width = style.width.value;
        }
        if (style.height.unit == CSSUnit::PX && style.height.value > 0) {
            height = style.height.value;
        }

        // 如果有transform动画，需要扩展bounds以容纳transform后的内容
        // 使用保守的边距以确保不会裁剪（未来可以基于实际动画边界动态计算）
        const float kOpacityLayerMargin = 100.0f;
        SkRect bounds = SkRect::MakeLTRB(-kOpacityLayerMargin, -kOpacityLayerMargin,
                                         width + kOpacityLayerMargin, height + kOpacityLayerMargin);

        int alpha = static_cast<int>(style.opacity * 255);
        canvas->saveLayerAlpha(&bounds, alpha);
    }

    // 应用 CSS transform
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        SkRect element_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(element_rect, style.transform_origin);
        canvas->concat(transform_matrix);
    }

    // 应用 CSS clip-path
    if (style.clip_path.has_value() && !style.clip_path->IsNone()) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPath clip_path = style.clip_path->ToSkPath(bounds);
        canvas->clipPath(clip_path, true);  // true = anti-alias
    }

    // 创建盒模型 - 使用缓存的值
    Box box;

    // 使用缓存的 padding 值
    box.padding_left = cache.padding_left;
    box.padding_right = cache.padding_right;
    box.padding_top = cache.padding_top;
    box.padding_bottom = cache.padding_bottom;

    // 使用缓存的 border 宽度
    box.border_top_width = cache.border_top_width;
    box.border_right_width = cache.border_right_width;
    box.border_bottom_width = cache.border_bottom_width;
    box.border_left_width = cache.border_left_width;

    // 使用缓存的内容区域偏移
    box.content_x = cache.content_x;
    box.content_y = cache.content_y;
    box.content_width = layout.width - cache.border_left_width - cache.border_right_width
                        - cache.padding_left - cache.padding_right;
    box.content_height = layout.height - cache.border_top_width - cache.border_bottom_width
                         - cache.padding_top - cache.padding_bottom;

    // 创建样式映射
    std::unordered_map<std::string, std::string> styles;
    if (!style.background_color.empty()) {
        styles["background-color"] = style.background_color;
    }
    if (!style.background_image.empty()) {
        styles["background-image"] = style.background_image;
    }

    // 渲染器
    BoxRenderer renderer(canvas);

    // 获取关联的 DOM 节点
    auto node = GetNode();
    const bool is_textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node) != nullptr;
    auto paint_canvas_surface = [&]() {
        if (!node || node->GetNodeType() != NodeType::ELEMENT_NODE) {
            return;
        }

        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() != "canvas") {
            return;
        }

        auto canvas_element = std::dynamic_pointer_cast<HTMLCanvasElement>(element);
        if (!canvas_element) {
            return;
        }

        auto context_2d = canvas_element->GetContext2D();
        if (!context_2d) {
            return;
        }

        auto* surface = context_2d->GetSurface();
        if (!surface) {
            return;
        }

        auto image = surface->makeImageSnapshot();
        if (!image) {
            return;
        }

        SkRect dest_rect = SkRect::MakeXYWH(
            box.content_x, box.content_y,
            box.content_width, box.content_height
        );
        canvas->drawImageRect(image, dest_rect, SkSamplingOptions());
    };

    // 检查是否是 <hr> 元素
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->GetTagName() == "hr") {
            // 绘制水平线（使用相对坐标，因为已经 translate 过了）
            SkPaint line_paint;
            line_paint.setColor(style.border.color);
            line_paint.setStrokeWidth(style.border.width.ToPx());
            line_paint.setAntiAlias(true);

            float y = layout.height / 2;
            canvas->drawLine(0, y, layout.width, y, line_paint);
            if (has_opacity) {
                canvas->restore(); // 恢复 opacity layer
            }
            canvas->restore(); // 恢复 canvas 状态
            return; // 不绘制其他内容
        }
    }

    // visibility: hidden 时跳过自身内容绘制（阴影、背景、边框等）
    // 但仍需继续处理子元素，因为子元素可能有 visibility: visible
    if (!is_hidden) {

    // 渲染阴影（使用缓存优化）
    auto shadow_start = std::chrono::high_resolution_clock::now();
    if (!style.box_shadow.empty()) {
        // 🐛 实验：禁用阴影缓存，直接绘制
        // 原因：缓存可能导致圆角信息丢失
        // 调试日志已移除
        renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);

        /* 原缓存代码（暂时禁用）
        // 计算 shadow 参数的哈希值
        // 🐛 修复：必须包含 border_radius，否则不同圆角的元素会共享同一个阴影缓存
        size_t shadow_hash = 0;
        for (const auto& s : style.box_shadow) {
            shadow_hash ^= std::hash<float>{}(s.offset_x) + 0x9e3779b9;
            shadow_hash ^= std::hash<float>{}(s.offset_y) + 0x9e3779b9;
            shadow_hash ^= std::hash<float>{}(s.blur_radius) + 0x9e3779b9;
            shadow_hash ^= std::hash<float>{}(s.spread_radius) + 0x9e3779b9;
            shadow_hash ^= std::hash<uint32_t>{}(s.color) + 0x9e3779b9;
        }
        // 加入 border_radius 到哈希值中
        shadow_hash ^= std::hash<float>{}(style.border_radius.top_left.value) + 0x9e3779b9;
        shadow_hash ^= std::hash<int>{}(static_cast<int>(style.border_radius.top_left.unit)) + 0x9e3779b9;
        shadow_hash ^= std::hash<float>{}(style.border_radius.top_right.value) + 0x9e3779b9;
        shadow_hash ^= std::hash<int>{}(static_cast<int>(style.border_radius.top_right.unit)) + 0x9e3779b9;
        shadow_hash ^= std::hash<float>{}(style.border_radius.bottom_right.value) + 0x9e3779b9;
        shadow_hash ^= std::hash<int>{}(static_cast<int>(style.border_radius.bottom_right.unit)) + 0x9e3779b9;
        shadow_hash ^= std::hash<float>{}(style.border_radius.bottom_left.value) + 0x9e3779b9;
        shadow_hash ^= std::hash<int>{}(static_cast<int>(style.border_radius.bottom_left.unit)) + 0x9e3779b9;

        // 检查缓存是否有效
        if (shadow_cache_.IsValid(layout.width, layout.height, shadow_hash)) {
            // 使用缓存的阴影图像
            // 调试日志已移除
            canvas->drawImage(shadow_cache_.image,
                              shadow_cache_.draw_offset.x(),
                              shadow_cache_.draw_offset.y());
        } else {
            // 计算阴影边界（包含模糊扩展）
            float max_blur = 0, max_spread = 0, min_offset_x = 0, min_offset_y = 0;
            float max_offset_x = 0, max_offset_y = 0;
            for (const auto& s : style.box_shadow) {
                if (!s.inset) {
                    max_blur = std::max(max_blur, s.blur_radius);
                    max_spread = std::max(max_spread, s.spread_radius);
                    min_offset_x = std::min(min_offset_x, s.offset_x);
                    min_offset_y = std::min(min_offset_y, s.offset_y);
                    max_offset_x = std::max(max_offset_x, s.offset_x);
                    max_offset_y = std::max(max_offset_y, s.offset_y);
                }
            }

            // 阴影图像的边距（模糊半径 * 2 + spread + offset）
            float margin = max_blur * 2 + max_spread;
            float left_margin = margin - min_offset_x;
            float top_margin = margin - min_offset_y;
            float right_margin = margin + max_offset_x;
            float bottom_margin = margin + max_offset_y;

            int img_width = static_cast<int>(layout.width + left_margin + right_margin + 1);
            int img_height = static_cast<int>(layout.height + top_margin + bottom_margin + 1);

            // 调试日志已移除

            // 创建离屏 surface 绘制阴影
            SkImageInfo info = SkImageInfo::MakeN32Premul(img_width, img_height);
            auto surface = SkSurfaces::Raster(info);
            if (surface) {
                auto* shadow_canvas = surface->getCanvas();
                shadow_canvas->clear(SK_ColorTRANSPARENT);

                // 在离屏 canvas 上绘制阴影
                shadow_canvas->translate(left_margin, top_margin);

                // 调试日志已移除

                BoxRenderer shadow_renderer(shadow_canvas);
                shadow_renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);

                // 缓存结果
                shadow_cache_.image = surface->makeImageSnapshot();
                shadow_cache_.cached_width = layout.width;
                shadow_cache_.cached_height = layout.height;
                shadow_cache_.shadow_hash = shadow_hash;
                shadow_cache_.draw_offset = SkPoint::Make(-left_margin, -top_margin);

                // 绘制到主 canvas
                canvas->drawImage(shadow_cache_.image,
                                  shadow_cache_.draw_offset.x(),
                                  shadow_cache_.draw_offset.y());
            } else {
                // 回退：直接绘制（无缓存）
                renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);
            }
        }
        */ // 缓存代码结束
    }
    auto shadow_end = std::chrono::high_resolution_clock::now();
    g_paint_shadow_time += std::chrono::duration_cast<std::chrono::microseconds>(shadow_end - shadow_start).count();

    // 渲染背景（优先渐变，然后纯色）
    auto bg_start = std::chrono::high_resolution_clock::now();
    SkRect padding_box = box.GetPaddingBox();

    // 对于 body 元素，扩展背景绘制区域以覆盖整个 viewport
    // 这解决了浮点精度问题导致的边缘白线
    Box bg_box = box;
    if (IsBodyElement()) {
        // 向上取整确保完全覆盖 viewport
        bg_box.content_x = 0;
        bg_box.content_y = 0;
        bg_box.content_width = std::ceil(viewport_width_);
        bg_box.content_height = std::ceil(viewport_height_);
        bg_box.padding_top = bg_box.padding_right = bg_box.padding_bottom = bg_box.padding_left = 0;
        bg_box.border_top_width = bg_box.border_right_width = bg_box.border_bottom_width = bg_box.border_left_width = 0;
        padding_box = SkRect::MakeXYWH(0, 0, std::ceil(viewport_width_), std::ceil(viewport_height_));
    }

    // 优先检查多层渐变（CSS网格背景等）
    if (!style.background_linear_gradients.empty()) {
        // 先绘制背景色（如果有）
        if (!style.background_color.empty()) {
            renderer.RenderBackgroundAdvanced(IsBodyElement() ? bg_box : box, styles, &style.border_radius);
        }

        // 准备 background-sizes 向量
        // 如果 background_sizes 为空但有单个 background_size，则为所有渐变层使用该尺寸
        std::vector<CSSBackgroundSize> sizes_to_use = style.background_sizes;
        if (sizes_to_use.empty() && style.background_size.type != CSSBackgroundSize::Type::AUTO) {
            // 为每个渐变层复制相同的 background_size
            for (size_t i = 0; i < style.background_linear_gradients.size(); i++) {
                sizes_to_use.push_back(style.background_size);
            }
        }

        // 然后绘制多层渐变
        GradientRenderer::RenderMultipleLinearGradients(canvas, padding_box,
                                                         style.background_linear_gradients,
                                                         sizes_to_use);
    }
    else if (style.background_linear_gradient.has_value()) {
        GradientRenderer::RenderLinearGradient(canvas, padding_box, *style.background_linear_gradient);
    }
    else if (style.background_radial_gradient.has_value()) {
        GradientRenderer::RenderRadialGradient(canvas, padding_box, *style.background_radial_gradient);
    }
    else {
        renderer.RenderBackgroundAdvanced(IsBodyElement() ? bg_box : box, styles, &style.border_radius);
    }
    auto bg_end = std::chrono::high_resolution_clock::now();
    g_paint_bg_time += std::chrono::duration_cast<std::chrono::microseconds>(bg_end - bg_start).count();

    paint_canvas_surface();

    // 渲染边框 - 使用缓存的标志位
    if (cache.has_border) {
        SkRect border_box = box.GetBorderBox();

        // 使用缓存的圆角标志
        bool has_border_radius = cache.has_border_radius;

        if (has_border_radius) {
            // 有圆角：使用 RenderRoundedBorderAdvanced（支持每边独立属性）

            // 准备四边宽度数组 [top, right, bottom, left]
            float border_widths[4] = {
                box.border_top_width,
                box.border_right_width,
                box.border_bottom_width,
                box.border_left_width
            };

            // 准备四边样式数组
            CSSBorderStyle border_styles[4] = {
                style.border_top_style != CSSBorderStyle::NONE ? style.border_top_style : style.border.style,
                style.border_right_style != CSSBorderStyle::NONE ? style.border_right_style : style.border.style,
                style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_style : style.border.style,
                style.border_left_style != CSSBorderStyle::NONE ? style.border_left_style : style.border.style
            };

            // 准备四边颜色数组
            SkColor border_colors[4] = {
                style.border_top_style != CSSBorderStyle::NONE ? style.border_top_color : style.border.color,
                style.border_right_style != CSSBorderStyle::NONE ? style.border_right_color : style.border.color,
                style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_color : style.border.color,
                style.border_left_style != CSSBorderStyle::NONE ? style.border_left_color : style.border.color
            };

            renderer.RenderRoundedBorderAdvanced(box, border_widths, border_styles, border_colors, style.border_radius);

        } else {
            // 无圆角：使用原有的独立边框渲染逻辑

            // 边框绘制时需要向内偏移半个边框宽度
            // 因为 Skia 的线条是以指定坐标为中心绘制的
            float half_left = box.border_left_width / 2.0f;
            float half_right = box.border_right_width / 2.0f;
            float half_top = box.border_top_width / 2.0f;
            float half_bottom = box.border_bottom_width / 2.0f;

            // 检查是否是 fieldset 元素，需要特殊处理上边框
            bool is_fieldset = false;
            float legend_left = 0, legend_right = 0;
            RenderObject* legend_render = nullptr;

            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                if (element->GetTagName() == "fieldset") {
                    is_fieldset = true;
                    // 查找 legend 子元素的渲染对象
                    for (auto& child : children_) {
                        auto child_node = child->GetNode();
                        if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto child_elem = std::static_pointer_cast<Element>(child_node);
                            if (child_elem->GetTagName() == "legend") {
                                legend_render = child.get();
                                auto& legend_layout = child->GetLayoutInfo();
                                auto& legend_style = child->GetComputedStyle();

                                // 计算 legend 的实际渲染宽度
                                // 遍历 legend 的子元素，找到最右边的位置
                                float max_child_right = 0.0f;
                                for (const auto& grandchild : child->GetChildren()) {
                                    auto& gc_layout = grandchild->GetLayoutInfo();
                                    max_child_right = std::max(max_child_right, gc_layout.x + gc_layout.width);
                                }

                                float legend_padding_right = legend_style.padding.right.ToPx();
                                float legend_border_right = legend_style.border_right_width > 0 ?
                                    legend_style.border_right_width : legend_style.border.width.ToPx();

                                // legend_left 是 legend 的左边缘（相对于 fieldset border-box）
                                legend_left = legend_layout.x;
                                // legend_right 是 legend 的右边缘
                                // = legend_left + 子元素最右边位置 + 右侧 padding + 右侧 border
                                legend_right = legend_layout.x + max_child_right + legend_padding_right + legend_border_right;
                                break;
                            }
                        }
                    }
                }
            }

            // 渲染左边框
            if (box.border_left_width > 0) {
                CSSBorderStyle left_style = style.border_left_style != CSSBorderStyle::NONE ?
                                            style.border_left_style : style.border.style;
                SkColor left_color = style.border_left_style != CSSBorderStyle::NONE ?
                                     style.border_left_color : style.border.color;
                if (left_style != CSSBorderStyle::NONE) {
                    renderer.RenderBorderEdge(
                        border_box.left() + half_left, border_box.top(),
                        border_box.left() + half_left, border_box.bottom(),
                        box.border_left_width, left_style, left_color
                    );
                }
            }

            // 渲染右边框
            if (box.border_right_width > 0) {
                CSSBorderStyle right_style = style.border_right_style != CSSBorderStyle::NONE ?
                                             style.border_right_style : style.border.style;
                SkColor right_color = style.border_right_style != CSSBorderStyle::NONE ?
                                      style.border_right_color : style.border.color;
                if (right_style != CSSBorderStyle::NONE) {
                    renderer.RenderBorderEdge(
                        border_box.right() - half_right, border_box.top(),
                        border_box.right() - half_right, border_box.bottom(),
                        box.border_right_width, right_style, right_color
                    );
                }
            }

            // 渲染上边框 - fieldset 需要特殊处理（在 legend 位置断开）
            if (box.border_top_width > 0) {
                CSSBorderStyle top_style = style.border_top_style != CSSBorderStyle::NONE ?
                                           style.border_top_style : style.border.style;
                SkColor top_color = style.border_top_style != CSSBorderStyle::NONE ?
                                    style.border_top_color : style.border.color;
                if (top_style != CSSBorderStyle::NONE) {
                    if (is_fieldset && legend_render) {
                        // fieldset 上边框在 legend 位置断开
                        // 绘制 legend 左边的部分
                        if (legend_left > border_box.left()) {
                            renderer.RenderBorderEdge(
                                border_box.left(), border_box.top() + half_top,
                                legend_left, border_box.top() + half_top,
                                box.border_top_width, top_style, top_color
                            );
                        }
                        // 绘制 legend 右边的部分
                        if (legend_right < border_box.right()) {
                            renderer.RenderBorderEdge(
                                legend_right, border_box.top() + half_top,
                                border_box.right(), border_box.top() + half_top,
                                box.border_top_width, top_style, top_color
                            );
                        }
                    } else {
                        // 普通元素：绘制完整上边框
                        renderer.RenderBorderEdge(
                            border_box.left(), border_box.top() + half_top,
                            border_box.right(), border_box.top() + half_top,
                            box.border_top_width, top_style, top_color
                        );
                    }
                }
            }

            // 渲染下边框
            if (box.border_bottom_width > 0) {
                CSSBorderStyle bottom_style = style.border_bottom_style != CSSBorderStyle::NONE ?
                                              style.border_bottom_style : style.border.style;
                SkColor bottom_color = style.border_bottom_style != CSSBorderStyle::NONE ?
                                       style.border_bottom_color : style.border.color;
                if (bottom_style != CSSBorderStyle::NONE) {
                    renderer.RenderBorderEdge(
                        border_box.left(), border_box.bottom() - half_bottom,
                        border_box.right(), border_box.bottom() - half_bottom,
                        box.border_bottom_width, bottom_style, bottom_color
                    );
                }
            }
        }
    }

    // ========== 绘制 outline（焦点指示器）==========
    // outline 不占用布局空间，紧贴边框外边缘绘制（符合浏览器行为）
    PaintOutline(canvas);

    // 绘制列表项目符号（如果是<li>元素）
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element2 = std::static_pointer_cast<Element>(node);
        if (element2->GetTagName() == "li") {
            // 获取父元素（ul或ol）
            auto parent_node = element2->GetParentNode();
            std::string parent_tag = "";

            // 向上查找最近的 ul 或 ol 祖先
            auto ancestor = parent_node;
            std::shared_ptr<Element> list_element = nullptr;
            while (ancestor) {
                if (ancestor->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto ancestor_elem = std::static_pointer_cast<Element>(ancestor);
                    std::string tag = ancestor_elem->GetTagName();
                    if (tag == "ul" || tag == "ol") {
                        parent_tag = tag;
                        list_element = ancestor_elem;
                        break;
                    }
                }
                ancestor = ancestor->GetParentNode();
            }

            if (!parent_tag.empty() && list_element) {
                auto parent_element = list_element;

                // 计算当前<li>在列表中的索引
                int item_index = 1;

                // 检查 <li> 是否有 value 属性
                std::string value_attr = element2->GetAttribute("value");
                if (!value_attr.empty()) {
                    try {
                        item_index = std::stoi(value_attr);
                    } catch (...) {
                        // 忽略解析错误
                    }
                } else {
                    // 获取 ol 的 start 属性
                    std::string start_attr = parent_element->GetAttribute("start");
                    int start_index = 1;
                    if (!start_attr.empty()) {
                        try {
                            start_index = std::stoi(start_attr);
                        } catch (...) {}
                    }

                    // 计算当前<li>在列表中的位置
                    int position = 0;
                    auto siblings = parent_element->GetChildNodes();
                    for (const auto& sibling : siblings) {
                        if (sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
                            auto sibling_elem = std::static_pointer_cast<Element>(sibling);
                            if (sibling_elem->GetTagName() == "li") {
                                if (sibling_elem == element2) {
                                    break;
                                }
                                // 检查前面的 li 是否有 value 属性
                                std::string prev_value = sibling_elem->GetAttribute("value");
                                if (!prev_value.empty()) {
                                    try {
                                        start_index = std::stoi(prev_value) + 1;
                                        position = 0;
                                    } catch (...) {}
                                }
                                position++;
                            }
                        }
                    }
                    item_index = start_index + position;
                }

                // Use the new PaintListMarker function
                PaintListMarker(canvas, style, layout, box, item_index, parent_tag);
            }
        }
    }

    // 渲染表单控件特定内容
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        // 直接从node进行dynamic_cast，保留类型信息
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
        if (input_element) {
            PaintInputElement(canvas, input_element.get(), box);
        }

        // 渲染 textarea 元素
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
        if (textarea_element) {
            PaintTextAreaElement(canvas, textarea_element.get(), box);
        }

        // 渲染图片元素 - 保持与 RenderInlineBlock 一致的 object-fit/object-position 行为
        auto select_element = std::dynamic_pointer_cast<HTMLSelectElement>(node);
        if (select_element) {
            PaintSelectElement(canvas, select_element.get(), box);
            // A select paints its selected option as native control content.
            // Do not paint option children as ordinary document content.
            if (has_opacity) {
                canvas->restore();
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }

        auto audio_element = std::dynamic_pointer_cast<HTMLAudioElement>(node);
        if (audio_element) {
            PaintAudioElementControl(canvas, audio_element.get(), box, computed_style_);
            if (has_opacity) {
                canvas->restore();
            }
            canvas->restore();
            needs_paint_ = false;
            return;
        }

        auto image_element = std::dynamic_pointer_cast<HTMLImageElement>(node);
        if (image_element) {
            sk_sp<SkImage> image = image_element->GetSkImage();
            if (!image) {
                std::string src = image_element->GetSrc();
                if (!src.empty()) {
                    image = ImageLoader::LoadFromUrl(src);
                    if (image) {
                        image_element->SetSkImage(image);
                    }
                }
            }

            if (image) {
                float image_width = static_cast<float>(image->width());
                float image_height = static_cast<float>(image->height());

                SkRect container_rect = SkRect::MakeXYWH(
                    box.content_x,
                    box.content_y,
                    box.content_width,
                    box.content_height
                );

                ObjectFitResult fit_result = CalculateObjectFit(
                    image_width,
                    image_height,
                    container_rect,
                    style.object_fit,
                    style.object_position
                );

                if (!fit_result.src_rect.isEmpty() && !fit_result.dst_rect.isEmpty()) {
                    SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
                    canvas->drawImageRect(image, fit_result.src_rect, fit_result.dst_rect,
                                         sampling, nullptr, SkCanvas::kStrict_SrcRectConstraint);
                }
            }
        }

        // 渲染 terminal 元素
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element && element->GetTagName() == "terminal") {
            auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(node);
            if (terminal_element) {
                terminal_element->Render(canvas, box.content_x, box.content_y, box.content_width, box.content_height);
            }
        }

        // 渲染 logview 元素
        if (element && element->GetTagName() == "logview") {
            auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(node);
            if (logview_element) {
                logview_element->Render(canvas, box.content_x, box.content_y, box.content_width, box.content_height);
            }
        }
    }

    } // end of if (!is_hidden) - 自身内容绘制结束

    // 检查是否有 border-radius（用于 overflow 裁剪和 fieldset 渲染）
    bool has_border_radius = style.border_radius.top_left.value > 0 ||
                             style.border_radius.top_right.value > 0 ||
                             style.border_radius.bottom_left.value > 0 ||
                             style.border_radius.bottom_right.value > 0;

    // 应用 overflow 裁剪
    bool needs_clip = false;
    bool needs_scrollbar = false;
    float content_width = 0, content_height = 0;
    const float scrollbar_width = 12.0f;

    // 获取独立的 overflow-x 和 overflow-y 值
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    // 检查任一方向是否有 overflow 属性
    auto isOverflowSet = [](const std::string& v) {
        return v == "hidden" || v == "scroll" || v == "auto";
    };

    if (!is_textarea_element && (isOverflowSet(overflow_x) || isOverflowSet(overflow_y))) {
        needs_clip = true;

        // 检测布局宽度是否变化（如滚动条出现/消失导致可用宽度变化）
        // 当宽度变化时，需要重新计算 content_width_
        float current_layout_width = layout_info_.width;
        bool layout_width_changed = (last_layout_width_ != current_layout_width && last_layout_width_ > 0);
        if (layout_width_changed) {
            content_width_ = 0.0f;  // 清除缓存，强制重新计算
        }
        last_layout_width_ = current_layout_width;

        // 优化：只在布局改变后重新计算内容尺寸
        // 使用缓存的值，避免每次 Paint 都遍历整个子树
        if (content_width_ <= 0 || content_height_ <= 0 || needs_layout_) {
            auto calc_start = std::chrono::high_resolution_clock::now();
            content_width = CalculateContentWidth();
            content_height = CalculateContentHeight();
            auto calc_end = std::chrono::high_resolution_clock::now();
            auto calc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(calc_end - calc_start).count();
            if (calc_ms > 10) {
            }
            content_width_ = content_width;
            content_height_ = content_height;
        } else {
            content_width = content_width_;
            content_height = content_height_;
        }

        // 判断是否需要滚动条
        float effective_width = GetEffectiveVisibleWidth();
        float effective_height = GetEffectiveVisibleHeight();
        float visible_width = effective_width - box.border_left_width - box.border_right_width;
        float visible_height = effective_height - box.border_top_width - box.border_bottom_width;

        ScrollbarState scrollbar_state = ScrollbarController::ComputeState(
            content_width,
            content_height,
            visible_width,
            visible_height,
            overflow_x,
            overflow_y,
            scrollbar_width);

        // 使用容差值来避免浮点误差导致的滚动条误显示
        // 当内容高度和可见高度差异小于 1px 时，认为不需要滚动条
        bool needs_v_scroll = scrollbar_state.needs_vertical;
        bool needs_h_scroll = scrollbar_state.needs_horizontal;
        float content_area_width = scrollbar_state.content_area_width;
        float content_area_height = scrollbar_state.content_area_height;

        needs_scrollbar = needs_h_scroll || needs_v_scroll;

        // ✅ 修复：窗口或内容尺寸变化后，重新限制滚动位置
        // 场景1：用户滚动到底部后，窗口变高，此时max_scroll变小，
        //       需要自动调整scroll_y_以保持在有效范围内
        // 场景2：窗口最大化后不再需要滚动条，需要重置滚动位置
        // 注意：这里直接使用已计算的 content_width/height，避免再次调用 GetMaxScroll
        float max_scroll_x = ScrollMaxForAxis(content_width, content_area_width, needs_h_scroll);
        float max_scroll_y = ScrollMaxForAxis(content_height, content_area_height, needs_v_scroll);

        // 只在超出范围时调整（避免不必要的重绘标记）
        if (scroll_x_ > max_scroll_x || scroll_y_ > max_scroll_y) {
            scroll_x_ = std::max(0.0f, std::min(scroll_x_, max_scroll_x));
            scroll_y_ = std::max(0.0f, std::min(scroll_y_, max_scroll_y));
            // 注意：这里不调用MarkNeedsPaint()，因为我们已经在Paint中了
        }

        float clip_width = content_area_width;
        float clip_height = content_area_height;

        SkRect clip_rect = SkRect::MakeXYWH(
            box.border_left_width,
            box.border_top_width,
            clip_width,
            clip_height
        );
        canvas->save();

        // 根据是否有 border-radius 选择裁剪方式
        if (has_border_radius) {
            // 使用圆角裁剪
            float box_width = clip_rect.width();
            float box_height = clip_rect.height();
            float base_size = std::min(box_width, box_height);

            SkRRect rrect;
            float tl = style.border_radius.top_left.ToPx(base_size);
            float tr = style.border_radius.top_right.ToPx(base_size);
            float br = style.border_radius.bottom_right.ToPx(base_size);
            float bl = style.border_radius.bottom_left.ToPx(base_size);
            SkVector radii[4] = {
                {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
            };
            rrect.setRectRadii(clip_rect, radii);
            canvas->clipRRect(rrect, SkClipOp::kIntersect, true);
        } else {
            // 使用矩形裁剪
            canvas->clipRect(clip_rect, SkClipOp::kIntersect, true);
        }

        // 应用滚动偏移
        // 注意：始终在 Paint 中应用滚动偏移
        // 这确保没有独立层的子元素能正确滚动
        canvas->translate(-scroll_x_, -scroll_y_);

    }

    // 按 z-index 排序子元素
    // 使用 stable_sort 保持相同 z-index 元素的原始顺序（DOM 顺序）
    std::vector<std::shared_ptr<RenderObject>> sorted_children = children_;
    std::stable_sort(sorted_children.begin(), sorted_children.end(),
        [](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
            return a->GetComputedStyle().z_index < b->GetComputedStyle().z_index;
        });

    // 检查是否是 fieldset 元素
    bool is_fieldset_element = false;
    RenderObject* legend_child = nullptr;
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::static_pointer_cast<Element>(node);
        is_fieldset_element = (elem->GetTagName() == "fieldset");

        // 查找 legend 子元素
        if (is_fieldset_element) {
            for (auto& child : children_) {
                auto child_node = child->GetNode();
                if (child_node && child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto child_elem = std::static_pointer_cast<Element>(child_node);
                    if (child_elem->GetTagName() == "legend") {
                        legend_child = child.get();
                        break;
                    }
                }
            }
        }
    }

    // 对于 fieldset，先在裁剪区域外绘制 legend
    // legend 的布局 y 坐标是 0（与 fieldset 的 y 坐标相同，用于 getBoundingClientRect）
    // 但是绘制时需要将 legend 居中于 fieldset 的上边框线
    if (is_fieldset_element && legend_child) {
        auto& legend_layout = legend_child->GetLayoutInfo();
        float original_y = legend_layout.y;

        // 计算绘制时的 y 坐标：legend 的垂直中心应该在 border_top / 2 的位置
        float legend_half_height = legend_layout.height / 2.0f;
        float paint_y = box.border_top_width / 2.0f - legend_half_height;

        // 临时修改位置进行渲染
        legend_layout.y = paint_y;
        legend_child->Paint(canvas);

        // 恢复原始位置（保持布局一致性）
        legend_layout.y = original_y;
    }

    // 绘制其他子元素（fieldset 的非 legend 子元素需要裁剪）
    if (is_fieldset_element && has_border_radius) {
        canvas->save();
        SkRect clip_rect = box.GetPaddingBox();

        // 修复：计算 border-radius 百分比的基准尺寸
        float box_width = clip_rect.width();
        float box_height = clip_rect.height();
        float base_size = std::min(box_width, box_height);

        SkRRect rrect;
        float tl = style.border_radius.top_left.ToPx(base_size);
        float tr = style.border_radius.top_right.ToPx(base_size);
        float br = style.border_radius.bottom_right.ToPx(base_size);
        float bl = style.border_radius.bottom_left.ToPx(base_size);
        SkVector radii[4] = {
            {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
        };
        rrect.setRectRadii(clip_rect, radii);
        canvas->clipRRect(rrect, SkClipOp::kIntersect, true);
    }

    auto children_start = std::chrono::high_resolution_clock::now();

    // 收集直接子元素中的 fixed 元素，稍后在滚动条之后绘制
    // 注意：如果 fixed 元素被提升为独立合成层，会在 HasOwnCompositorLayer() 检查中跳过
    std::vector<std::shared_ptr<RenderObject>> fixed_children;

    // 🐛 调试：追踪父元素的 Paint() 调用
    if (debug_clip) {
        // 检查是否有 fixed 子元素
        bool has_fixed_child = false;
        for (const auto& child : sorted_children) {
            if (child->GetComputedStyle().position == "fixed") {
                has_fixed_child = true;
                break;
            }
        }

        if (has_fixed_child) {
            SkRect local_clip = canvas->getLocalClipBounds();
        }
    }

    for (auto& child : sorted_children) {
        // 跳过已经绘制的 legend
        if (is_fieldset_element && child.get() == legend_child) {
            continue;
        }

        // 获取子元素样式，用于后续检查
        const auto& child_style = child->GetComputedStyle();
        bool is_fixed = (child_style.position == "fixed");
        bool is_fixed_or_absolute = (is_fixed || child_style.position == "absolute");

        // 🐛 调试：追踪 fixed 元素的处理
        if (debug_clip && is_fixed) {
        }

        // 关键修复：跳过有独立合成层的子元素
        // 这些子元素会在自己的层中单独光栅化，不应该在父层中绘制
        // 否则会导致重影（元素被绘制两次）
        if (child->HasOwnCompositorLayer()) {
            if (debug_clip && is_fixed) {
            }
            continue;
        }

        // position: fixed 元素延迟到滚动条之后绘制
        if (is_fixed) {
            if (debug_clip) {
            }
            fixed_children.push_back(child);
            continue;
        }

        // 增量绘制优化：提前检查子节点是否与当前裁剪区域相交
        // 注意：对于 position: absolute 且高 z-index 的元素，不能使用 quickReject
        if (!is_fixed_or_absolute || child_style.z_index < 100) {
            const auto& child_layout = child->GetLayoutInfo();
            SkRect child_rect = SkRect::MakeXYWH(
                child_layout.x, child_layout.y,
                child_layout.width, child_layout.height
            );
            if (canvas->quickReject(child_rect.makeOutset(50, 50))) {
                continue;
            }
        }

        // 绘制非 fixed 子元素
        if (debug_hover_bug) {
            std::string child_tag = "unknown";
            std::string child_id = "";
            if (auto child_node = child->GetNode()) {
                if (child_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto elem = std::static_pointer_cast<Element>(child_node);
                    child_tag = elem->GetTagName();
                    child_id = elem->GetAttribute("id");
                } else if (child_node->GetNodeType() == NodeType::TEXT_NODE) {
                    child_tag = "#text";
                }
            }
            const auto& child_layout = child->GetLayoutInfo();
        }
        child->Paint(canvas);
    }
    auto children_end = std::chrono::high_resolution_clock::now();
    g_paint_children_time += std::chrono::duration_cast<std::chrono::microseconds>(children_end - children_start).count();

    // 恢复 fieldset 的圆角裁剪状态
    if (is_fieldset_element && has_border_radius) {
        canvas->restore();
    }

    // 恢复 overflow 裁剪状态和滚动偏移
    if (needs_clip) {
        canvas->restore();
    }

    // 绘制滚动条 (在裁剪区域外绘制，但在 fixed 元素之前)
    if (needs_scrollbar) {
        // For body element, scrollbar should be drawn relative to viewport, not body
        // Save current transform and adjust for body's margin
        bool is_body = IsBodyElement();
        if (is_body) {
            canvas->save();
            // Translate back by body's position to draw scrollbar relative to viewport
            canvas->translate(-layout_info_.x, -layout_info_.y);
        }

        // 对于 body 元素使用视口尺寸
        float effective_width = GetEffectiveVisibleWidth();
        float effective_height = GetEffectiveVisibleHeight();
        float visible_width = effective_width - box.border_left_width - box.border_right_width;
        float visible_height = effective_height - box.border_top_width - box.border_bottom_width;

        // 获取独立的 overflow-x 和 overflow-y 值
        std::string overflow_x_sb = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
        std::string overflow_y_sb = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

        // 判断是否允许显示滚动条
        ScrollbarState scrollbar_state = ScrollbarController::ComputeState(
            content_width,
            content_height,
            visible_width,
            visible_height,
            overflow_x_sb,
            overflow_y_sb,
            scrollbar_width);

        // 使用与上面相同的逻辑判断是否需要滚动条
        bool needs_v_scroll = scrollbar_state.needs_vertical;
        bool needs_h_scroll = scrollbar_state.needs_horizontal;

        // 如果需要水平滚动条，调整高度并重新检查
        const float scrollbar_margin = 2.0f;
        const float corner_radius = 4.0f;

        // 滚动条颜色：优先使用 CSS scrollbar-color 属性，否则使用默认值
        SkColor track_color = SkColorSetRGB(241, 241, 241);  // 默认浅灰色轨道
        SkColor thumb_color = SkColorSetRGB(193, 193, 193);  // 默认深灰色滑块
        if (!style.scrollbar_color_auto) {
            thumb_color = style.scrollbar_thumb_color;
            track_color = style.scrollbar_track_color;
        }

        SkPaint track_paint;
        track_paint.setColor(track_color);
        track_paint.setAntiAlias(true);

        SkPaint thumb_paint;
        thumb_paint.setColor(thumb_color);
        thumb_paint.setAntiAlias(true);

        // 滚动条区域尺寸
        float scrollbar_area_width = effective_width;
        float scrollbar_area_height = effective_height;

        // 绘制水平滚动条
        if (needs_h_scroll) {
            float track_x = box.border_left_width;
            float track_y = scrollbar_area_height - box.border_bottom_width - scrollbar_width;
            float track_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);

            // 绘制轨道
            SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, track_width, scrollbar_width);
            canvas->drawRect(track_rect, track_paint);

            // 计算滑块尺寸和位置 - 使用减去滚动条后的可用宽度
            float available_content_width = visible_width - (needs_v_scroll ? scrollbar_width : 0);
            float scrollable_width = content_width - available_content_width;
            float thumb_ratio = available_content_width / content_width;
            float thumb_width = std::max(30.0f, (track_width - 2 * scrollbar_margin) * thumb_ratio);
            float available_track = track_width - thumb_width - 2 * scrollbar_margin;
            float scroll_ratio = scrollable_width > 0 ? scroll_x_ / scrollable_width : 0;
            float thumb_x = track_x + scrollbar_margin + available_track * scroll_ratio;

            SkRect thumb_rect = SkRect::MakeXYWH(
                thumb_x,
                track_y + scrollbar_margin,
                thumb_width,
                scrollbar_width - 2 * scrollbar_margin
            );
            canvas->drawRoundRect(thumb_rect, corner_radius, corner_radius, thumb_paint);
        }

        // 绘制垂直滚动条
        if (needs_v_scroll) {
            float track_x = scrollbar_area_width - box.border_right_width - scrollbar_width;
            float track_y = box.border_top_width;
            float track_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);

            // 绘制轨道
            SkRect track_rect = SkRect::MakeXYWH(track_x, track_y, scrollbar_width, track_height);
            canvas->drawRect(track_rect, track_paint);

            // 计算滑块尺寸和位置 - 使用减去滚动条后的可用高度
            float available_content_height = visible_height - (needs_h_scroll ? scrollbar_width : 0);
            float scrollable_height = content_height - available_content_height;
            float thumb_ratio = available_content_height / content_height;
            float thumb_height = std::max(30.0f, (track_height - 2 * scrollbar_margin) * thumb_ratio);
            float available_track = track_height - thumb_height - 2 * scrollbar_margin;
            float scroll_ratio = scrollable_height > 0 ? scroll_y_ / scrollable_height : 0;
            float thumb_y = track_y + scrollbar_margin + available_track * scroll_ratio;

            SkRect thumb_rect = SkRect::MakeXYWH(
                track_x + scrollbar_margin,
                thumb_y,
                scrollbar_width - 2 * scrollbar_margin,
                thumb_height
            );
            canvas->drawRoundRect(thumb_rect, corner_radius, corner_radius, thumb_paint);
        }

        // 绘制滚动条角落（当两个滚动条都存在时）
        if (needs_h_scroll && needs_v_scroll) {
            float corner_x = scrollbar_area_width - box.border_right_width - scrollbar_width;
            float corner_y = scrollbar_area_height - box.border_bottom_width - scrollbar_width;
            SkRect corner_rect = SkRect::MakeXYWH(corner_x, corner_y, scrollbar_width, scrollbar_width);
            canvas->drawRect(corner_rect, track_paint);
        }

        // Restore transform for body element
        if (is_body) {
            canvas->restore();
        }
    }

    // 绘制 position: fixed 元素（在滚动条之后）
    // 注意：如果 fixed 元素被提升为独立合成层，这里的 fixed_children 会是空的
    // 因为它们在上面的循环中被 HasOwnCompositorLayer() 跳过了

    // 🐛 调试：检查 fixed_children 数量
    if (debug_clip) {
    }

    // 🐛 调试：在 fixed 循环前查看 clip
    if (debug_clip && !fixed_children.empty()) {
        SkRect local_clip = canvas->getLocalClipBounds();
        SkIRect device_clip = canvas->getDeviceClipBounds();
        SkMatrix matrix = canvas->getTotalMatrix();
    }

    for (auto& child : fixed_children) {
        // 获取当前 canvas 的变换矩阵
        SkMatrix current_matrix = canvas->getTotalMatrix();

        // 提取 DPI 缩放因子（假设是均匀缩放）
        float scale_x = current_matrix.getScaleX();
        float scale_y = current_matrix.getScaleY();

        // 🐛 实验：使用 setMatrix 代替 resetMatrix + scale
        // 原因：resetMatrix 可能影响 MaskFilter 的行为
        canvas->save();

        if (debug_clip) {
            SkRect local_clip = canvas->getLocalClipBounds();
            SkIRect device_clip = canvas->getDeviceClipBounds();
        }

        // 创建一个只包含 DPI 缩放的矩阵
        SkMatrix fixed_matrix;
        fixed_matrix.setScale(scale_x, scale_y);
        canvas->setMatrix(fixed_matrix);

        if (debug_clip) {
            SkRect local_clip = canvas->getLocalClipBounds();
            SkMatrix matrix = canvas->getTotalMatrix();
        }

        // 绘制 fixed 元素（使用其视口绝对坐标）
        child->Paint(canvas);

        // 恢复之前的状态（包括裁剪区域和变换矩阵）
        canvas->restore();
    }

    // ========== 绘制 contentEditable 光标 ==========
    // 参考 textarea 的光标渲染实现
    if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        if (element->IsContentEditable() && element->HasPseudoClass("focus")) {
            PaintContentEditableCaret(canvas, element.get(), box);
        }
    }

    // 恢复 opacity layer（如果有）
    if (has_opacity) {
        canvas->restore();
    }

    // 恢复画布状态
    canvas->restore();

    needs_paint_ = false;
}

void RenderBlock::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    if (!input) return;

    InputType type = input->GetInputType();

    // 处理文本类型的input
    if (type == InputType::Text || type == InputType::Password ||
        type == InputType::Email || type == InputType::Tel ||
        type == InputType::Url || type == InputType::Search ||
        type == InputType::Number) {
        FormElementPaintParams params;
        params.font_family = computed_style_.font_family;
        params.font_size = computed_style_.font_size;
        params.text_color = computed_style_.color;
        auto element = std::static_pointer_cast<Element>(GetNode());
        params.has_focus = element && element->HasPseudoClass("focus");
        params.cursor_visible = IsCursorVisible();

        FormElementPainter painter(canvas);
        painter.PaintTextInput(input, box, params, type == InputType::Password);
        return;
    }

    if (type == InputType::Checkbox || type == InputType::Radio) {
        bool checked = input->GetChecked();
        float cx = box.content_x + box.content_width / 2;
        float cy = box.content_y + box.content_height / 2;

        if (type == InputType::Checkbox) {
            // 绘制 checkbox 方框边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);

            float size = std::min(box.content_width, box.content_height);
            float half = size / 2;
            SkRect checkbox_rect = SkRect::MakeXYWH(cx - half, cy - half, size, size);

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            canvas->drawRoundRect(checkbox_rect, 2, 2, bg_paint);

            // 边框
            canvas->drawRoundRect(checkbox_rect, 2, 2, border_paint);

            // 如果选中，绘制勾选标记
            if (checked) {
                SkPaint check_paint;
                check_paint.setColor(SK_ColorBLACK);
                check_paint.setStrokeWidth(2);
                check_paint.setStyle(SkPaint::kStroke_Style);
                check_paint.setAntiAlias(true);

                SkPath check_path;
                check_path.moveTo(cx - 4, cy);
                check_path.lineTo(cx - 1, cy + 3);
                check_path.lineTo(cx + 4, cy - 3);
                canvas->drawPath(check_path, check_paint);
            }
        }
        else if (type == InputType::Radio) {
            // 绘制 radio 外圆环
            float radius = std::min(box.content_width, box.content_height) / 2;

            // 背景
            SkPaint bg_paint;
            bg_paint.setColor(SK_ColorWHITE);
            bg_paint.setStyle(SkPaint::kFill_Style);
            bg_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, bg_paint);

            // 边框
            SkPaint border_paint;
            border_paint.setColor(SkColorSetRGB(118, 118, 118));
            border_paint.setStrokeWidth(1);
            border_paint.setStyle(SkPaint::kStroke_Style);
            border_paint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, radius, border_paint);

            // 如果选中，绘制内圆点
            if (checked) {
                SkPaint dot_paint;
                dot_paint.setColor(SK_ColorBLACK);
                dot_paint.setStyle(SkPaint::kFill_Style);
                dot_paint.setAntiAlias(true);

                float inner_radius = radius / 2;
                canvas->drawCircle(cx, cy, inner_radius, dot_paint);
            }
        }
    }
}

void RenderBlock::PaintSelectElement(SkCanvas* canvas, HTMLSelectElement* select, const Box& box) {
    if (!select) return;

    FormElementPaintParams params;
    params.font_family = computed_style_.font_family;
    params.font_size = computed_style_.font_size;
    params.text_color = computed_style_.color;

    auto element = std::static_pointer_cast<Element>(GetNode());
    params.has_focus = element && element->HasPseudoClass("focus");

    FormElementPainter painter(canvas);
    painter.PaintSelectElement(select, box, params);
}

void RenderBlock::PaintTextAreaElement(SkCanvas* canvas, HTMLTextAreaElement* textarea, const Box& box) {
    textarea_painter::Paint(canvas, textarea, this, box, IsCursorVisible());
}

void RenderBlock::PaintContentEditableCaret(SkCanvas* canvas, Element* element, const Box& box) {
    (void)box;
    if (!canvas || !element) return;

    auto editable_root = std::static_pointer_cast<Element>(element->shared_from_this());
    if (GetContentEditableEditingHost(editable_root) != editable_root) {
        return;
    }

    auto document = std::dynamic_pointer_cast<Document>(element->GetOwnerDocument());
    if (!document) {
        return;
    }

    auto selection = document->GetSelection();
    if (!selection || !selection->IsCollapsed()) {
        return;
    }

    if (!IsCursorVisible()) {
        return;
    }

    auto focus_node = selection->GetComputedFocusNode();
    int focus_offset = selection->GetComputedFocusOffset();
    if (!IsNodeInsideEditingHost(focus_node, editable_root.get())) {
        return;
    }

    auto element_rect = element->GetBoundingClientRect();
    auto caret_rect = ComputeContentEditableCaretRect(editable_root, focus_node, focus_offset);
    if (!caret_rect.valid) {
        return;
    }

    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(2.0f);
    cursor_paint.setAntiAlias(true);
    const float local_x = caret_rect.x - element_rect.x;
    const float local_y = caret_rect.y - element_rect.y;
    canvas->drawLine(local_x,
                     local_y,
                     local_x,
                     local_y + caret_rect.height,
                     cursor_paint);
    return;
}


}  // namespace mblink
