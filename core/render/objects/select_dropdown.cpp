/**
 * @file select_dropdown.cpp
 * @brief Select 下拉菜单管理器实现
 */

#include "select_dropdown.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/utils/color.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/core/SkMaskFilter.h"
#include "include/effects/SkBlurMaskFilter.h"

#ifdef DrawText
#undef DrawText
#endif

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace mbink {
namespace {
constexpr float kItemHeight = 20.0f;
constexpr float kItemPaddingX = 12.0f;
constexpr float kOptgroupIndent = 16.0f;
constexpr float kDropdownMaxHeight = 320.0f;
constexpr float kScrollbarWidth = 8.0f;
constexpr float kViewportMargin = 4.0f;
constexpr float kBorderInset = 1.0f;
constexpr float kWheelStep = 36.0f;
constexpr float kCornerRadius = 6.0f;
constexpr float kDropdownDirtyOutset = 20.0f;

struct DropdownRow {
    std::string text;
    bool is_label = false;
    bool disabled = false;
    long option_index = -1;
    float indent = 0.0f;
};

float Clampf(float value, float min_value, float max_value) {
    return std::max(min_value, std::min(value, max_value));
}

std::string GetOptionText(const std::shared_ptr<Element>& option) {
    if (!option) return "";
    for (const auto& child : option->GetChildNodes()) {
        if (child->GetNodeType() == NodeType::TEXT_NODE) {
            auto text = std::static_pointer_cast<Text>(child);
            return text ? text->GetData() : std::string();
        }
    }
    return "";
}

std::vector<DropdownRow> BuildRows(const std::shared_ptr<HTMLSelectElement>& select) {
    std::vector<DropdownRow> rows;
    if (!select) return rows;

    long option_index = 0;
    for (const auto& child : select->GetChildNodes()) {
        if (!child || child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
        auto elem = std::static_pointer_cast<Element>(child);
        if (!elem) continue;

        const std::string tag = elem->GetTagName();
        if (tag == "optgroup") {
            const std::string label = elem->GetAttribute("label");
            if (!label.empty()) {
                rows.push_back({label, true, true, -1, 0.0f});
            }
            for (const auto& option_child : elem->GetChildNodes()) {
                if (!option_child || option_child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
                auto option_elem = std::static_pointer_cast<Element>(option_child);
                if (!option_elem || option_elem->GetTagName() != "option") continue;
                rows.push_back({GetOptionText(option_elem), false, option_elem->HasAttribute("disabled"), option_index, kOptgroupIndent});
                ++option_index;
            }
        } else if (tag == "option") {
            rows.push_back({GetOptionText(elem), false, elem->HasAttribute("disabled"), option_index, 0.0f});
            ++option_index;
        }
    }
    return rows;
}

std::shared_ptr<Window> FindOwnerWindow(const std::shared_ptr<HTMLSelectElement>& select) {
    if (!select) return nullptr;
    auto owner_document = select->GetOwnerDocument();
    if (!owner_document) return nullptr;

    for (const auto& window : WindowManager::Instance().GetAllWindows()) {
        if (window && window->GetDocument().get() == owner_document.get()) {
            return window;
        }
    }
    return nullptr;
}

SkRect GetWindowViewportRect(const std::shared_ptr<HTMLSelectElement>& select, const SkRect& trigger_rect) {
    auto owner_window = FindOwnerWindow(select);
    if (!owner_window) {
        return SkRect::MakeLTRB(0.0f, 0.0f,
                                std::max(trigger_rect.right() + 400.0f, 800.0f),
                                std::max(trigger_rect.bottom() + 400.0f, 600.0f));
    }

    int width = 0;
    int height = 0;
    owner_window->GetSize(&width, &height);

    // GetSize() 返回的已经是 CSS 逻辑尺寸；下拉框布局/绘制也使用 CSS 坐标，
    // 这里不能再按 DPI 缩放重复换算，否则高 DPI 下可视区域会被错误缩小。
    return SkRect::MakeXYWH(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
}

float GetMaxScroll(const SelectDropdownInfo& info) {
    return std::max(0.0f, info.content_height - info.viewport_rect.height());
}

SkRect DropdownPaintBounds(const SelectDropdownInfo& info) {
    SkRect bounds = info.dropdown_rect;
    if (bounds.isEmpty()) {
        return SkRect::MakeEmpty();
    }

    // The popup is painted on the final window surface, not into the retained
    // main-content cache. Include blur and antialias fringe so partial presents
    // overwrite stale popup border/scrollbar pixels when the popup moves/closes.
    bounds.outset(kDropdownDirtyOutset, kDropdownDirtyOutset);
    return bounds;
}

void QueueDropdownRepaint(const SelectDropdownInfo& info, RepaintReason reason) {
    auto select = info.select_element.lock();
    auto owner_window = FindOwnerWindow(select);
    if (!owner_window) {
        return;
    }

    SkRect dirty_bounds = DropdownPaintBounds(info);
    if (!dirty_bounds.isEmpty()) {
        owner_window->AddDirtyRect(dirty_bounds);
    }
    if (!info.trigger_rect.isEmpty()) {
        owner_window->AddDirtyRect(info.trigger_rect);
    }
    owner_window->SetNeedsRepaintFor(reason);
}

void EnsureSelectedOptionVisible(SelectDropdownInfo& info,
                                 const std::vector<DropdownRow>& rows,
                                 long selected_index) {
    if (selected_index < 0) return;
    float row_top = 0.0f;
    for (const auto& row : rows) {
        if (row.option_index == selected_index) {
            const float row_bottom = row_top + kItemHeight;
            if (row_top < info.scroll_offset) {
                info.scroll_offset = row_top;
            } else if (row_bottom > info.scroll_offset + info.viewport_rect.height()) {
                info.scroll_offset = row_bottom - info.viewport_rect.height();
            }
            info.scroll_offset = Clampf(info.scroll_offset, 0.0f, GetMaxScroll(info));
            return;
        }
        row_top += kItemHeight;
    }
}

void RecomputeDropdownLayout(SelectDropdownInfo& info,
                             const std::shared_ptr<HTMLSelectElement>& select,
                             const SkRect& trigger_rect,
                             bool scroll_to_selected) {
    FontDescriptor desc;
    desc.family = "Microsoft YaHei, Arial, sans-serif";
    desc.size = 13.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    FontDescriptor bold_desc = desc;
    bold_desc.weight = FontWeight::BOLD;
    SkFont bold_font = FontManager::GetInstance().LoadFont(bold_desc);

    auto rows = BuildRows(select);
    float max_row_width = 0.0f;
    for (const auto& row : rows) {
        const SkFont& row_font = row.is_label ? bold_font : font;
        float row_width = row.indent + TextRenderer::MeasureMixedTextWidth(row.text, row_font);
        max_row_width = std::max(max_row_width, row_width);
    }

    SkRect window_viewport = GetWindowViewportRect(select, trigger_rect);
    float viewport_left = window_viewport.left() + kViewportMargin;
    float viewport_top = window_viewport.top() + kViewportMargin;
    float viewport_right = window_viewport.right() - kViewportMargin;
    float viewport_bottom = window_viewport.bottom() - kViewportMargin;

    info.trigger_rect = trigger_rect;
    info.content_height = static_cast<float>(rows.size()) * kItemHeight;

    float ideal_height = std::min(std::max(info.content_height, kItemHeight), kDropdownMaxHeight);
    float available_below = std::max(0.0f, viewport_bottom - (trigger_rect.bottom() + 1.0f));
    float available_above = std::max(0.0f, (trigger_rect.top() - 1.0f) - viewport_top);

    bool open_above = false;
    if (available_below >= ideal_height) {
        open_above = false;
    } else if (available_above >= ideal_height) {
        open_above = true;
    } else {
        open_above = available_above > available_below;
    }

    float available_height = std::max(1.0f, open_above ? available_above : available_below);
    float dropdown_height = std::min(ideal_height, available_height);

    bool needs_scrollbar = info.content_height > dropdown_height + 0.5f;
    float preferred_width = std::max(trigger_rect.width(),
                                     max_row_width + kItemPaddingX * 2.0f +
                                         (needs_scrollbar ? (kScrollbarWidth + 6.0f) : 0.0f));
    float available_width = std::max(1.0f, viewport_right - viewport_left);
    float dropdown_width = std::min(preferred_width, available_width);

    float dropdown_x = Clampf(trigger_rect.left(), viewport_left, viewport_right - dropdown_width);
    float dropdown_y = open_above ? (trigger_rect.top() - 1.0f - dropdown_height)
                                  : (trigger_rect.bottom() + 1.0f);
    dropdown_y = Clampf(dropdown_y, viewport_top, viewport_bottom - dropdown_height);

    info.open_above = open_above;
    info.dropdown_rect = SkRect::MakeXYWH(dropdown_x, dropdown_y, dropdown_width, dropdown_height);
    info.viewport_rect = SkRect::MakeLTRB(info.dropdown_rect.left() + kBorderInset,
                                          info.dropdown_rect.top() + kBorderInset,
                                          info.dropdown_rect.right() - kBorderInset,
                                          info.dropdown_rect.bottom() - kBorderInset);

    if (scroll_to_selected) {
        info.scroll_offset = 0.0f;
        EnsureSelectedOptionVisible(info, rows, select ? select->GetSelectedIndex() : -1);
    } else {
        info.scroll_offset = Clampf(info.scroll_offset, 0.0f, GetMaxScroll(info));
    }
}
}


SelectDropdownManager& SelectDropdownManager::Instance() {
    static SelectDropdownManager instance;
    return instance;
}

void SelectDropdownManager::OpenDropdown(std::shared_ptr<HTMLSelectElement> select, const SkRect& trigger_rect) {
    if (!select) return;

    if (current_dropdown_.is_open) {
        QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton);
        auto old_select = current_dropdown_.select_element.lock();
        if (old_select) {
            old_select->SetDropdownOpen(false);
        }
    }

    current_dropdown_ = {};
    current_dropdown_.select_element = select;
    current_dropdown_.is_open = true;

    select->SetHoveredIndex(select->GetSelectedIndex());
    RecomputeDropdownLayout(current_dropdown_, select, trigger_rect, true);
    QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton);
}

void SelectDropdownManager::CloseDropdown() {
    if (!current_dropdown_.is_open) return;

    QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton);

    auto select = current_dropdown_.select_element.lock();
    if (select) {
        select->SetDropdownOpen(false);
        select->SetHoveredIndex(-1);
    }

    current_dropdown_ = {};
}

std::shared_ptr<HTMLSelectElement> SelectDropdownManager::GetActiveSelect() const {
    return current_dropdown_.select_element.lock();
}

void SelectDropdownManager::UpdatePosition(const SkRect& new_trigger_rect) {
    if (!current_dropdown_.is_open) return;

    auto select = current_dropdown_.select_element.lock();
    if (!select) {
        QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton);
        current_dropdown_ = {};
        return;
    }

    SelectDropdownInfo previous_dropdown = current_dropdown_;
    RecomputeDropdownLayout(current_dropdown_, select, new_trigger_rect, false);
    if (previous_dropdown.dropdown_rect != current_dropdown_.dropdown_rect ||
        previous_dropdown.trigger_rect != current_dropdown_.trigger_rect) {
        QueueDropdownRepaint(previous_dropdown, RepaintReason::MouseButton);
        QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton);
    }
}

void SelectDropdownManager::UpdatePositionFromRenderTree(std::shared_ptr<RenderObject> root_render) {
    if (!current_dropdown_.is_open || !root_render) return;

    auto select = current_dropdown_.select_element.lock();
    if (!select) return;

    std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> findSelectRenderObject;
    findSelectRenderObject = [&](std::shared_ptr<RenderObject> render_obj) -> std::shared_ptr<RenderObject> {
        if (!render_obj) return nullptr;

        auto node = render_obj->GetNode();
        if (node && node.get() == select.get()) {
            return render_obj;
        }

        for (const auto& child : render_obj->GetChildren()) {
            auto result = findSelectRenderObject(child);
            if (result) return result;
        }

        return nullptr;
    };

    auto select_render = findSelectRenderObject(root_render);
    if (!select_render) return;

    auto rect = select->GetBoundingClientRect();
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }

    SkRect new_trigger_rect = SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height);
    UpdatePosition(new_trigger_rect);
}

void SelectDropdownManager::Paint(SkCanvas* canvas) {
    if (!current_dropdown_.is_open || !canvas) return;

    auto select = current_dropdown_.select_element.lock();
    if (!select) {
        current_dropdown_ = {};
        return;
    }

    const SkRect& rect = current_dropdown_.dropdown_rect;
    const SkRect& viewport = current_dropdown_.viewport_rect;
    auto rows = BuildRows(select);
    bool needs_scrollbar = current_dropdown_.content_height > viewport.height() + 0.5f;

    SkPaint shadow_paint;
    shadow_paint.setColor(SkColorSetARGB(36, 0, 0, 0));
    shadow_paint.setAntiAlias(true);
    shadow_paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, DROPDOWN_SHADOW_BLUR));
    canvas->drawRRect(SkRRect::MakeRectXY(rect.makeOffset(0.0f, 2.0f), kCornerRadius, kCornerRadius), shadow_paint);

    SkPaint bg_paint;
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(rect, kCornerRadius, kCornerRadius), bg_paint);

    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(210, 210, 210));
    border_paint.setAntiAlias(true);
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(1.0f);
    canvas->drawRRect(SkRRect::MakeRectXY(rect, kCornerRadius, kCornerRadius), border_paint);

    FontDescriptor desc;
    desc.family = "Microsoft YaHei, Arial, sans-serif";
    desc.size = 13.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);
    FontDescriptor bold_desc = desc;
    bold_desc.weight = FontWeight::BOLD;
    SkFont bold_font = FontManager::GetInstance().LoadFont(bold_desc);
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    float text_height = -font_metrics.fAscent + font_metrics.fDescent;
    float content_right = viewport.right() - (needs_scrollbar ? (kScrollbarWidth + 4.0f) : 0.0f);
    SkRect content_rect = SkRect::MakeLTRB(viewport.left(), viewport.top(), content_right, viewport.bottom());

    canvas->save();
    canvas->clipRRect(SkRRect::MakeRectXY(rect, kCornerRadius, kCornerRadius));
    canvas->clipRect(content_rect);

    TextRenderer text_renderer(canvas);
    float row_top = 0.0f;
    long hovered_index = select->GetHoveredIndex();
    long selected_index = select->GetSelectedIndex();
    for (const auto& row : rows) {
        float draw_top = viewport.top() + row_top - current_dropdown_.scroll_offset;
        float draw_bottom = draw_top + kItemHeight;
        if (draw_bottom >= viewport.top() && draw_top <= viewport.bottom()) {
            SkRect row_rect = SkRect::MakeLTRB(content_rect.left(), draw_top, content_rect.right(), draw_bottom);
            if (!row.is_label) {
                if (row.option_index == hovered_index) {
                    SkPaint hover_paint;
                    hover_paint.setColor(SkColorSetRGB(0, 120, 212));
                    canvas->drawRect(row_rect, hover_paint);
                } else if (row.option_index == selected_index) {
                    SkPaint selected_paint;
                    selected_paint.setColor(SkColorSetRGB(232, 240, 254));
                    canvas->drawRect(row_rect, selected_paint);
                }
            }

            mbink::Paint text_paint;
            if (row.is_label) {
                text_paint.SetColor(SkColorSetRGB(90, 90, 90));
            } else if (row.option_index == hovered_index) {
                text_paint.SetColor(SK_ColorWHITE);
            } else if (row.disabled) {
                text_paint.SetColor(SkColorSetRGB(160, 160, 160));
            } else {
                text_paint.SetColor(SK_ColorBLACK);
            }

            float text_y = draw_top + (kItemHeight - text_height) * 0.5f - font_metrics.fAscent;
            const SkFont& row_font = row.is_label ? bold_font : font;
            text_renderer.DrawTextWithEmoji(row.text, content_rect.left() + kItemPaddingX + row.indent, text_y, row_font, text_paint);
        }
        row_top += kItemHeight;
    }
    canvas->restore();

    if (needs_scrollbar && current_dropdown_.content_height > 0.0f) {
        float track_right = viewport.right() - 2.0f;
        float track_left = track_right - kScrollbarWidth;
        SkRect track_rect = SkRect::MakeLTRB(track_left, viewport.top() + 2.0f, track_right, viewport.bottom() - 2.0f);
        float thumb_height = std::max(24.0f, viewport.height() * viewport.height() / current_dropdown_.content_height);
        float thumb_range = std::max(0.0f, track_rect.height() - thumb_height);
        float thumb_top = track_rect.top();
        float max_scroll = GetMaxScroll(current_dropdown_);
        if (max_scroll > 0.0f) {
            thumb_top += (current_dropdown_.scroll_offset / max_scroll) * thumb_range;
        }
        SkRect thumb_rect = SkRect::MakeLTRB(track_left + 1.0f, thumb_top, track_right - 1.0f, thumb_top + thumb_height);

        SkPaint thumb_paint;
        thumb_paint.setColor(SkColorSetARGB(120, 120, 120, 120));
        thumb_paint.setAntiAlias(true);
        canvas->drawRRect(SkRRect::MakeRectXY(thumb_rect, 3.0f, 3.0f), thumb_paint);
    }
}

bool SelectDropdownManager::HandleMouseMove(float x, float y) {
    if (!current_dropdown_.is_open) return false;

    auto select = current_dropdown_.select_element.lock();
    if (!select) return false;
    if (!current_dropdown_.dropdown_rect.contains(x, y)) return false;

    const SkRect& viewport = current_dropdown_.viewport_rect;
    if (x < viewport.left() || y < viewport.top() || y > viewport.bottom()) {
        select->SetHoveredIndex(-1);
        return true;
    }

    bool needs_scrollbar = current_dropdown_.content_height > viewport.height() + 0.5f;
    float content_right = viewport.right() - (needs_scrollbar ? (kScrollbarWidth + 4.0f) : 0.0f);
    if (x > content_right) {
        select->SetHoveredIndex(-1);
        return true;
    }

    float content_y = current_dropdown_.scroll_offset + (y - viewport.top());
    auto rows = BuildRows(select);
    long hovered_index = -1;
    for (size_t i = 0; i < rows.size(); ++i) {
        float row_top = static_cast<float>(i) * kItemHeight;
        if (content_y >= row_top && content_y < row_top + kItemHeight) {
            if (!rows[i].is_label && !rows[i].disabled) {
                hovered_index = rows[i].option_index;
            }
            break;
        }
    }
    select->SetHoveredIndex(hovered_index);
    return true;
}

bool SelectDropdownManager::HandleClick(float x, float y) {
    if (!current_dropdown_.is_open) return false;

    auto select = current_dropdown_.select_element.lock();
    if (!select) {
        CloseDropdown();
        return false;
    }

    if (!current_dropdown_.dropdown_rect.contains(x, y)) {
        CloseDropdown();
        return false;
    }

    HandleMouseMove(x, y);
    if (select->GetHoveredIndex() >= 0) {
        select->SelectHoveredOption();
        CloseDropdown();
    }
    return true;
}

bool SelectDropdownManager::HandleWheel(float delta_y) {
    if (!current_dropdown_.is_open) return false;

    float max_scroll = GetMaxScroll(current_dropdown_);
    if (max_scroll <= 0.0f) return false;

    float next_offset = current_dropdown_.scroll_offset + (-delta_y * kWheelStep);
    current_dropdown_.scroll_offset = Clampf(next_offset, 0.0f, max_scroll);
    return true;
}

bool SelectDropdownManager::HitTest(float x, float y) const {
    if (!current_dropdown_.is_open) return false;
    return current_dropdown_.dropdown_rect.contains(x, y);
}

} // namespace mbink

