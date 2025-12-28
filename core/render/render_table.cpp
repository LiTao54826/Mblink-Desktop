/**
 * @file render_table.cpp
 * @brief 表格渲染对象实现
 * 
 * 从 render_object.cpp 提取的表格相关渲染类实现，包括：
 * - RenderTable: 表格元素
 * - RenderTableRowGroup: 表格行组 (thead, tbody, tfoot)
 * - RenderTableRow: 表格行
 * - RenderTableCell: 表格单元格
 * - RenderTableCaption: 表格标题
 */

#include "render_object.h"
#include "painters/box_renderer.h"
#include "color.h"
#include "core/dom/element.h"
#include <algorithm>
#include <limits>

namespace lightui {

// ========== RenderTable 实现 ==========

void RenderTable::CollectColumnStyles() {
    column_background_colors_.clear();

    // 遍历子元素查找 colgroup 和 col
    for (auto& child : children_) {
        // 检查是否是 colgroup（通过检查 DOM 元素的标签名）
        auto node = child->GetNode();
        if (!node) continue;

        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) continue;

        std::string tag_name = element->GetTagName();
        std::transform(tag_name.begin(), tag_name.end(), tag_name.begin(), ::tolower);

        if (tag_name == "colgroup") {
            // colgroup 的背景色（作为默认值应用到其下的 col）
            std::string colgroup_bg = child->GetComputedStyle().background_color;

            // 遍历 colgroup 中的 col 元素
            for (auto& col_child : child->GetChildren()) {
                auto col_node = col_child->GetNode();
                if (!col_node) continue;

                auto col_element = std::dynamic_pointer_cast<Element>(col_node);
                if (!col_element) continue;

                std::string col_tag = col_element->GetTagName();
                std::transform(col_tag.begin(), col_tag.end(), col_tag.begin(), ::tolower);

                if (col_tag == "col") {
                    // 获取 col 的背景色，如果没有则使用 colgroup 的
                    std::string col_bg = col_child->GetComputedStyle().background_color;
                    if (col_bg.empty() || col_bg == "transparent") {
                        col_bg = colgroup_bg;
                    }

                    // 检查 span 属性
                    int span = 1;
                    std::string span_str = col_element->GetAttribute("span");
                    if (!span_str.empty()) {
                        try {
                            span = std::stoi(span_str);
                            if (span < 1) span = 1;
                        } catch (...) {
                            span = 1;
                        }
                    }

                    // 为每个跨越的列添加背景色
                    for (int i = 0; i < span; ++i) {
                        column_background_colors_.push_back(col_bg);
                    }
                }
            }
        }
        else if (tag_name == "col") {
            // 直接的 col 元素（没有 colgroup 包裹）
            std::string col_bg = child->GetComputedStyle().background_color;

            // 检查 span 属性
            int span = 1;
            std::string span_str = element->GetAttribute("span");
            if (!span_str.empty()) {
                try {
                    span = std::stoi(span_str);
                    if (span < 1) span = 1;
                } catch (...) {
                    span = 1;
                }
            }

            // 为每个跨越的列添加背景色
            for (int i = 0; i < span; ++i) {
                column_background_colors_.push_back(col_bg);
            }
        }
    }
}

void RenderTable::CalculateColumnWidths(float available_width) {
    // 收集所有行中的单元格来确定列数和宽度
    std::vector<float> min_widths;
    std::vector<float> preferred_widths;
    size_t max_columns = 0;

    // 辅助函数：计算一行的实际列数（考虑 colspan）
    auto count_row_columns = [](std::shared_ptr<RenderObject> row) -> size_t {
        size_t col_count = 0;
        for (auto& cell : row->GetChildren()) {
            int col_span = 1;
            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                col_span = table_cell->GetColSpan();
                if (col_span < 1) col_span = 1;
            }
            col_count += col_span;
        }
        return col_count;
    };

    // 遍历所有子元素（可能是 thead, tbody, tfoot 或直接的 tr）
    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();

        // 处理行组（thead, tbody, tfoot）
        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            for (auto& row : child->GetChildren()) {
                if (row->GetType() == RenderObjectType::TABLE_ROW) {
                    max_columns = std::max(max_columns, count_row_columns(row));
                }
            }
        }
        // 处理直接的行（tr）
        else if (child_type == RenderObjectType::TABLE_ROW) {
            max_columns = std::max(max_columns, count_row_columns(child));
        }
    }

    if (max_columns == 0) {
        column_widths_.clear();
        return;
    }

    // 初始化列宽度数组
    min_widths.resize(max_columns, 0.0f);
    preferred_widths.resize(max_columns, 0.0f);

    // 存储 colspan 单元格信息，用于后续处理
    struct ColspanCellInfo {
        size_t start_col;
        int col_span;
        float min_width;
        float preferred_width;
    };
    std::vector<ColspanCellInfo> colspan_cells;

    // 第一遍：处理 colspan=1 的单元格，收集 colspan>1 的单元格信息
    auto process_row_pass1 = [&](std::shared_ptr<RenderObject> row) {
        auto& cells = row->GetChildren();
        size_t logical_col = 0;
        for (size_t i = 0; i < cells.size() && logical_col < max_columns; ++i) {
            auto& cell = cells[i];
            const auto& cell_style = cell->GetComputedStyle();

            int col_span = 1;
            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                col_span = table_cell->GetColSpan();
                if (col_span < 1) col_span = 1;
            }

            float cell_padding_left = cell_style.padding.left.ToPx(available_width, cell_style.font_size);
            float cell_padding_right = cell_style.padding.right.ToPx(available_width, cell_style.font_size);
            float cell_border_left = cell_style.border.width.ToPx();
            float cell_border_right = cell_style.border.width.ToPx();
            float cell_extra = cell_padding_left + cell_padding_right + cell_border_left + cell_border_right;

            float content_min_width = 0;
            float content_preferred_width = 0;

            for (auto& cell_child : cell->GetChildren()) {
                cell_child->Layout(10000.0f, 0);
                auto& child_layout = cell_child->GetLayoutInfo();
                content_min_width = std::max(content_min_width, child_layout.width);
                content_preferred_width = std::max(content_preferred_width, child_layout.width);
            }

            if (cell_style.width.unit == CSSUnit::PX) {
                content_preferred_width = std::max(content_preferred_width, cell_style.width.value);
            }

            if (col_span == 1) {
                // colspan=1 的单元格直接设置列宽
                min_widths[logical_col] = std::max(min_widths[logical_col], content_min_width + cell_extra);
                preferred_widths[logical_col] = std::max(preferred_widths[logical_col], content_preferred_width + cell_extra);
            } else {
                // colspan>1 的单元格，记录下来后续处理
                colspan_cells.push_back({logical_col, col_span, content_min_width + cell_extra, content_preferred_width + cell_extra});
            }

            logical_col += col_span;
        }
    };

    // 遍历所有行（第一遍）
    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();

        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            for (auto& row : child->GetChildren()) {
                if (row->GetType() == RenderObjectType::TABLE_ROW) {
                    process_row_pass1(row);
                }
            }
        }
        else if (child_type == RenderObjectType::TABLE_ROW) {
            process_row_pass1(child);
        }
    }

    // 第二遍：处理 colspan>1 的单元格
    for (const auto& info : colspan_cells) {
        float current_total_min = 0;
        float current_total_pref = 0;
        for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
            current_total_min += min_widths[info.start_col + j];
            current_total_pref += preferred_widths[info.start_col + j];
        }

        if (info.min_width > current_total_min) {
            float extra = info.min_width - current_total_min;
            float extra_per_col = extra / info.col_span;
            for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
                min_widths[info.start_col + j] += extra_per_col;
            }
        }
        if (info.preferred_width > current_total_pref) {
            float extra = info.preferred_width - current_total_pref;
            float extra_per_col = extra / info.col_span;
            for (int j = 0; j < info.col_span && (info.start_col + j) < max_columns; ++j) {
                preferred_widths[info.start_col + j] += extra_per_col;
            }
        }
    }

    // 计算表格边框和间距
    const auto& style = computed_style_;
    bool is_collapse = (style.border_collapse == "collapse");

    float table_border_left = is_collapse ? 0 : style.border.width.ToPx();
    float table_border_right = is_collapse ? 0 : style.border.width.ToPx();
    float table_padding_left = style.padding.left.ToPx(available_width, style.font_size);
    float table_padding_right = style.padding.right.ToPx(available_width, style.font_size);

    float table_extra = table_border_left + table_border_right + table_padding_left + table_padding_right;
    float available_for_columns = available_width - table_extra;

    float total_min_width = 0;
    float total_preferred_width = 0;
    for (size_t i = 0; i < max_columns; ++i) {
        total_min_width += min_widths[i];
        total_preferred_width += preferred_widths[i];
    }

    // 分配列宽度
    column_widths_.resize(max_columns);

    bool has_explicit_width = (style.width.unit == CSSUnit::PX || style.width.unit == CSSUnit::PERCENT);

    if (has_explicit_width) {
        if (total_preferred_width <= available_for_columns) {
            float extra_space = available_for_columns - total_preferred_width;
            for (size_t i = 0; i < max_columns; ++i) {
                float ratio = (total_preferred_width > 0) ? (preferred_widths[i] / total_preferred_width) : (1.0f / max_columns);
                column_widths_[i] = preferred_widths[i] + extra_space * ratio;
            }
        }
        else if (total_min_width <= available_for_columns) {
            float extra_space = available_for_columns - total_min_width;
            float total_extra_needed = total_preferred_width - total_min_width;

            for (size_t i = 0; i < max_columns; ++i) {
                float extra_needed = preferred_widths[i] - min_widths[i];
                float extra = (total_extra_needed > 0) ? (extra_needed / total_extra_needed * extra_space) : 0;
                column_widths_[i] = min_widths[i] + extra;
            }
        }
        else {
            column_widths_ = min_widths;
        }
    } else {
        for (size_t i = 0; i < max_columns; ++i) {
            column_widths_[i] = preferred_widths[i];
        }
    }
}


void RenderTable::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    bool has_explicit_width = (style.width.unit == CSSUnit::PX || style.width.unit == CSSUnit::PERCENT);
    bool is_collapse = (style.border_collapse == "collapse");

    float border_spacing = 0;
    if (!is_collapse) {
        if (style.border_spacing.value > 0) {
            border_spacing = style.border_spacing.ToPx();
        } else {
            border_spacing = 2.0f;
        }
    }

    float border_width = style.border.width.ToPx();
    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);

    float effective_border = is_collapse ? 0 : border_width;

    CollectColumnStyles();

    float width;
    if (has_explicit_width) {
        if (style.width.unit == CSSUnit::PX) {
            width = style.width.value;
        } else {
            width = style.width.value / 100.0f * parent_width;
        }
        CalculateColumnWidths(width);
    } else {
        CalculateColumnWidths(parent_width);

        float total_column_width = 0;
        for (float col_w : column_widths_) {
            total_column_width += col_w;
        }

        size_t num_columns = column_widths_.size();
        float total_spacing = is_collapse ? 0 : (border_spacing * (num_columns + 1));

        width = total_column_width + total_spacing + padding_left + padding_right + effective_border * 2;
        width = std::min(width, parent_width);
    }

    float current_y = padding_top + effective_border + (is_collapse ? 0 : border_spacing);
    float content_width = width - effective_border * 2 - padding_left - padding_right;

    // 先布局 caption
    for (auto& child : children_) {
        if (child->GetType() == RenderObjectType::TABLE_CAPTION) {
            child->Layout(content_width, 0);
            auto& child_layout = child->GetLayoutInfo();
            child_layout.x = padding_left + effective_border;
            child_layout.y = current_y;
            current_y += child_layout.height;
        }
    }

    // 收集所有行
    std::vector<std::shared_ptr<RenderTableRow>> all_rows;
    std::vector<std::shared_ptr<RenderObject>> row_groups;

    auto collect_rows = [&](std::shared_ptr<RenderObject> container) {
        for (auto& child : container->GetChildren()) {
            if (child->GetType() == RenderObjectType::TABLE_ROW) {
                auto table_row = std::dynamic_pointer_cast<RenderTableRow>(child);
                if (table_row) {
                    all_rows.push_back(table_row);
                }
            }
        }
    };

    for (auto& child : children_) {
        RenderObjectType child_type = child->GetType();
        if (child_type == RenderObjectType::TABLE_ROW_GROUP ||
            child_type == RenderObjectType::TABLE_HEADER_GROUP ||
            child_type == RenderObjectType::TABLE_FOOTER_GROUP) {
            row_groups.push_back(child);
            collect_rows(child);
        } else if (child_type == RenderObjectType::TABLE_ROW) {
            auto table_row = std::dynamic_pointer_cast<RenderTableRow>(child);
            if (table_row) {
                all_rows.push_back(table_row);
            }
        }
    }

    // 跟踪 rowspan 占据的单元格
    std::vector<RenderTableRow::RowspanCell> active_rowspans;

    struct RowspanInfo {
        std::shared_ptr<RenderTableCell> cell;
        size_t start_row;
        int row_span;
    };
    std::vector<RowspanInfo> rowspan_cells;

    // 第一遍：布局所有行
    for (size_t row_idx = 0; row_idx < all_rows.size(); ++row_idx) {
        auto& table_row = all_rows[row_idx];

        table_row->SetColumnWidths(column_widths_);
        table_row->SetBorderSpacing(border_spacing);
        table_row->SetBorderCollapse(is_collapse);
        table_row->SetRowspanOccupiedCols(active_rowspans);

        table_row->Layout(content_width, 0);
        auto& row_layout = table_row->GetLayoutInfo();
        row_layout.y = current_y;
        current_y += row_layout.height;

        // 更新 active_rowspans
        for (auto it = active_rowspans.begin(); it != active_rowspans.end(); ) {
            it->remaining_rows--;
            if (it->remaining_rows <= 0) {
                it = active_rowspans.erase(it);
            } else {
                ++it;
            }
        }

        // 添加当前行中新的 rowspan 单元格
        size_t logical_col = 0;
        for (auto& cell : table_row->GetChildren()) {
            while (true) {
                bool occupied = false;
                for (const auto& rs : active_rowspans) {
                    if (rs.col_index == logical_col) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied) break;
                logical_col++;
            }

            auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
            if (table_cell) {
                int col_span = table_cell->GetColSpan();
                int row_span = table_cell->GetRowSpan();
                if (col_span < 1) col_span = 1;
                if (row_span < 1) row_span = 1;

                if (row_span > 1) {
                    RowspanInfo info;
                    info.cell = table_cell;
                    info.start_row = row_idx;
                    info.row_span = row_span;
                    rowspan_cells.push_back(info);

                    for (int c = 0; c < col_span; ++c) {
                        RenderTableRow::RowspanCell rs_cell;
                        rs_cell.col_index = logical_col + c;
                        rs_cell.remaining_rows = row_span - 1;
                        rs_cell.cell = std::dynamic_pointer_cast<RenderObject>(table_cell);
                        active_rowspans.push_back(rs_cell);
                    }
                }

                logical_col += col_span;
            } else {
                logical_col++;
            }
        }

        if (!is_collapse) {
            current_y += border_spacing;
        }
    }

    // 第二遍：更新 rowspan 单元格的高度
    for (const auto& info : rowspan_cells) {
        size_t end_row = std::min(info.start_row + info.row_span, all_rows.size());
        if (end_row <= info.start_row) continue;

        float start_y = all_rows[info.start_row]->GetLayoutInfo().y;
        float end_y = all_rows[end_row - 1]->GetLayoutInfo().y +
                      all_rows[end_row - 1]->GetLayoutInfo().height;
        float total_height = end_y - start_y;

        if (!is_collapse && info.row_span > 1) {
            total_height += border_spacing * (info.row_span - 1);
        }

        auto& cell_layout = info.cell->GetLayoutInfo();
        float cell_width = cell_layout.width;
        float cell_x = cell_layout.x;
        float cell_y = cell_layout.y;

        info.cell->Layout(cell_width, total_height);

        cell_layout.x = cell_x;
        cell_layout.y = cell_y;
        cell_layout.height = total_height;
    }

    // 设置独立行的 x 坐标
    for (auto& child : children_) {
        if (child->GetType() == RenderObjectType::TABLE_ROW) {
            auto& child_layout = child->GetLayoutInfo();
            child_layout.x = padding_left + effective_border + (is_collapse ? 0 : border_spacing);
        }
    }

    // 设置行组的布局信息
    for (auto& group : row_groups) {
        float min_y = std::numeric_limits<float>::max();
        float max_y = 0;
        for (auto& row : group->GetChildren()) {
            if (row->GetType() == RenderObjectType::TABLE_ROW) {
                auto& row_layout = row->GetLayoutInfo();
                min_y = std::min(min_y, row_layout.y);
                max_y = std::max(max_y, row_layout.y + row_layout.height);
                row_layout.x = 0;
            }
        }
        auto& group_layout = group->GetLayoutInfo();
        group_layout.x = padding_left + effective_border + (is_collapse ? 0 : border_spacing);
        group_layout.y = min_y;
        group_layout.width = content_width;
        group_layout.height = max_y - min_y;
        group_layout.is_laid_out = true;

        for (auto& row : group->GetChildren()) {
            if (row->GetType() == RenderObjectType::TABLE_ROW) {
                auto& row_layout = row->GetLayoutInfo();
                row_layout.y -= min_y;
            }
        }
    }

    float height = current_y + padding_bottom + effective_border;

    if (style.height.unit == CSSUnit::PX) {
        height = std::max(height, style.height.value);
    }

    layout_info_.width = width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTable::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(50, 50))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    bool is_collapse = (style.border_collapse == "collapse");

    canvas->save();
    canvas->translate(layout.x, layout.y);

    float border_w = style.border.width.ToPx();

    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_top_width = border_w;
    box.border_right_width = border_w;
    box.border_bottom_width = border_w;
    box.border_left_width = border_w;
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.padding_left - box.padding_right - box.border_left_width - box.border_right_width;
    box.content_height = layout.height - box.padding_top - box.padding_bottom - box.border_top_width - box.border_bottom_width;

    BoxRenderer renderer(canvas);

    SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    for (auto& child : children_) {
        child->Paint(canvas);
    }

    if (style.border.style != CSSBorderStyle::NONE && border_w > 0) {
        char width_str[32];
        snprintf(width_str, sizeof(width_str), "%.0fpx", border_w);
        std::string border_width = width_str;
        std::string border_style = "solid";
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        std::string border_color = color_str;
        renderer.RenderBorder(box, border_width, border_style, border_color);
    }

    canvas->restore();
    needs_paint_ = false;
}


// ========== RenderTableRowGroup 实现 ==========

void RenderTableRowGroup::Layout(float parent_width, float parent_height) {
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableRowGroup::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    const auto& style = computed_style_;
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableRow 实现 ==========

void RenderTableRow::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);
    float border_width = style.border.width.ToPx();

    float spacing = border_collapse_ ? 0 : border_spacing_;

    auto is_col_occupied = [this](size_t col) -> bool {
        for (const auto& rs : rowspan_occupied_cols_) {
            if (rs.col_index == col) {
                return true;
            }
        }
        return false;
    };

    auto get_occupied_width = [this, &spacing](size_t col) -> float {
        if (col < column_widths_.size()) {
            float w = column_widths_[col];
            if (!border_collapse_) {
                w += spacing;
            }
            return w;
        }
        return 0;
    };

    float current_x = 0;
    float max_height = 0;

    auto& cells = children_;
    size_t logical_col = 0;

    for (size_t i = 0; i < cells.size(); ++i) {
        auto& cell = cells[i];

        while (is_col_occupied(logical_col)) {
            current_x += get_occupied_width(logical_col);
            logical_col++;
        }

        int col_span = 1;
        auto table_cell = std::dynamic_pointer_cast<RenderTableCell>(cell);
        if (table_cell) {
            col_span = table_cell->GetColSpan();
            if (col_span < 1) col_span = 1;
            table_cell->SetColumnIndex(logical_col);
        }

        float cell_width = 0;
        int actual_cols = 0;
        for (int j = 0; j < col_span && (logical_col + j) < column_widths_.size(); ++j) {
            size_t target_col = logical_col + j;
            while (is_col_occupied(target_col) && target_col < column_widths_.size()) {
                target_col++;
            }
            if (target_col >= column_widths_.size()) break;

            cell_width += column_widths_[target_col];
            actual_cols++;
            if (actual_cols > 1 && !border_collapse_) {
                cell_width += spacing;
            }
        }

        if (cell_width == 0) {
            cell_width = 100.0f;
        }

        cell->Layout(cell_width, parent_height);
        auto& cell_layout = cell->GetLayoutInfo();

        cell_layout.x = current_x;
        cell_layout.y = 0;
        cell_layout.width = cell_width;

        current_x += cell_width;
        if (!border_collapse_) {
            current_x += spacing;
        }
        max_height = std::max(max_height, cell_layout.height);

        logical_col += col_span;
    }

    while (is_col_occupied(logical_col) && logical_col < column_widths_.size()) {
        current_x += get_occupied_width(logical_col);
        logical_col++;
    }

    for (auto& cell : cells) {
        auto& cell_layout = cell->GetLayoutInfo();
        cell_layout.height = max_height;
    }

    layout_info_.width = current_x;
    layout_info_.height = max_height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableRow::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    const auto& style = computed_style_;
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableCell 实现 ==========

void RenderTableCell::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);
    float border_width = style.border.width.ToPx();

    float content_width = parent_width - padding_left - padding_right - border_width * 2;

    float content_height = 0;
    for (auto& child : children_) {
        child->Layout(content_width, 0);
        content_height += child->GetLayoutInfo().height;
    }

    float natural_height = content_height + padding_top + padding_bottom + border_width * 2;
    float height = std::max(natural_height, parent_height);

    float available_height = height - padding_top - padding_bottom - border_width * 2;
    float vertical_offset = 0;

    const std::string& v_align = style.vertical_align;
    if (v_align == "middle") {
        vertical_offset = (available_height - content_height) / 2;
    } else if (v_align == "bottom") {
        vertical_offset = available_height - content_height;
    } else {
        vertical_offset = 0;
    }
    if (vertical_offset < 0) vertical_offset = 0;

    float current_y = padding_top + border_width + vertical_offset;
    for (auto& child : children_) {
        auto& child_layout = child->GetLayoutInfo();

        float horizontal_offset = 0;
        const std::string& t_align = style.text_align;
        if (t_align == "center") {
            horizontal_offset = (content_width - child_layout.width) / 2;
        } else if (t_align == "right") {
            horizontal_offset = content_width - child_layout.width;
        }
        if (horizontal_offset < 0) horizontal_offset = 0;

        child_layout.x = padding_left + border_width + horizontal_offset;
        child_layout.y = current_y;
        current_y += child_layout.height;
    }

    layout_info_.width = parent_width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableCell::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    float border_w = style.border.width.ToPx();

    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);
    box.border_top_width = border_w;
    box.border_right_width = border_w;
    box.border_bottom_width = border_w;
    box.border_left_width = border_w;
    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.padding_left - box.padding_right - box.border_left_width - box.border_right_width;
    box.content_height = layout.height - box.padding_top - box.padding_bottom - box.border_top_width - box.border_bottom_width;

    BoxRenderer renderer(canvas);

    SkRect bounds = SkRect::MakeWH(layout.width, layout.height);

    std::string effective_bg_color;

    auto parent = GetParent();
    while (parent) {
        if (parent->GetType() == RenderObjectType::TABLE) {
            auto table = std::dynamic_pointer_cast<RenderTable>(parent);
            if (table) {
                std::string col_bg = table->GetColumnBackgroundColor(column_index_);
                if (!col_bg.empty() && col_bg != "transparent") {
                    effective_bg_color = col_bg;
                }
            }
            break;
        }
        parent = parent->GetParent();
    }

    if (!style.background_color.empty() && style.background_color != "transparent") {
        effective_bg_color = style.background_color;
    }

    if (!effective_bg_color.empty()) {
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(effective_bg_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    if (style.border.style != CSSBorderStyle::NONE && border_w > 0) {
        char width_str[32];
        snprintf(width_str, sizeof(width_str), "%.0fpx", border_w);
        std::string border_width = width_str;
        std::string border_style = "solid";
        char color_str[8];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X",
                 SkColorGetR(style.border.color),
                 SkColorGetG(style.border.color),
                 SkColorGetB(style.border.color));
        std::string border_color = color_str;
        renderer.RenderBorder(box, border_width, border_style, border_color);
    }

    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

// ========== RenderTableCaption 实现 ==========

void RenderTableCaption::Layout(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    float padding_left = style.padding.left.ToPx(parent_width, style.font_size);
    float padding_right = style.padding.right.ToPx(parent_width, style.font_size);
    float padding_top = style.padding.top.ToPx(parent_width, style.font_size);
    float padding_bottom = style.padding.bottom.ToPx(parent_width, style.font_size);

    float content_width = parent_width - padding_left - padding_right;

    float content_height = 0;
    for (auto& child : children_) {
        child->Layout(content_width, 0);
        auto& child_layout = child->GetLayoutInfo();

        float horizontal_offset = 0;
        const std::string& t_align = style.text_align;
        if (t_align == "center") {
            horizontal_offset = (content_width - child_layout.width) / 2;
        } else if (t_align == "right") {
            horizontal_offset = content_width - child_layout.width;
        }

        child_layout.x = padding_left + horizontal_offset;
        child_layout.y = padding_top + content_height;
        content_height += child_layout.height;
    }

    float height = content_height + padding_top + padding_bottom;

    layout_info_.width = parent_width;
    layout_info_.height = height;
    layout_info_.is_laid_out = true;
    needs_layout_ = false;
}

void RenderTableCaption::Paint(SkCanvas* canvas) {
    if (!canvas) {
        needs_paint_ = false;
        return;
    }

    SkRect paint_rect = SkRect::MakeXYWH(layout_info_.x, layout_info_.y, layout_info_.width, layout_info_.height);
    if (canvas->quickReject(paint_rect.makeOutset(20, 20))) {
        needs_paint_ = false;
        return;
    }

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    canvas->save();
    canvas->translate(layout.x, layout.y);

    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        SkPaint bg_paint;
        bg_paint.setColor(Color::Parse(style.background_color));
        bg_paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bg_paint);
    }

    for (auto& child : children_) {
        child->Paint(canvas);
    }

    canvas->restore();
    needs_paint_ = false;
}

} // namespace lightui
