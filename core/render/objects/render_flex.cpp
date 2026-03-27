/**
 * @file render_flex.cpp
 * @brief Flex渲染对象实现（块级flex容器）
 */

#include "render_flex.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/painters/background_painter.h"
#include "core/render/painters/border_painter.h"
#include "core/render/utils/gradient_renderer.h"
#include "core/render/utils/shadow_renderer.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include <algorithm>
#include <iostream>

namespace lightui {

void RenderFlex::Layout(float parent_width, float parent_height) {
    // 🔍 DEBUG: 输出 Layout 调用
    static bool debug_layout = std::getenv("DEBUG_INCREMENTAL_PAINT") != nullptr;
    if (debug_layout) {
        auto node = GetNode();
        std::string tag_name = "?";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            tag_name = elem->GetTagName();
        }
    }

    // flex 元素使用 flex 布局逻辑
    LayoutAsFlex(parent_width, parent_height);

    if (debug_layout) {

        // 输出子元素的布局信息
        for (size_t i = 0; i < children_.size(); i++) {
            auto& child = children_[i];
            if (child->GetType() == RenderObjectType::TEXT) {
                auto text_child = std::dynamic_pointer_cast<RenderText>(child);
                if (text_child) {
                    std::string text = text_child->GetText();
                    if (text.length() > 20) text = text.substr(0, 20) + "...";
                    const auto& child_layout = child->GetLayoutInfo();
                }
            }
        }
    }
}

void RenderFlex::LayoutAsFlex(float parent_width, float parent_height) {
    const auto& style = computed_style_;

    // 🎯 关键差异：RenderFlex 是块级元素，不需要 dimensions_externally_set 检查
    // 因为它不会被 IFC 布局引擎处理

    // 计算宽度 - 块级flex容器默认占满父容器宽度
    float width = parent_width;
    if (!style.width.IsAuto()) {
        width = style.width.ToPx(parent_width, style.font_size);
    }

    // 应用 min-width 和 max-width
    if (!style.min_width.IsZero()) {
        float min_w = style.min_width.ToPx(parent_width, style.font_size);
        width = std::max(width, min_w);
    }
    if (style.max_width.unit != CSSUnit::NONE) {
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
    if (!style.height.IsAuto()) {
        height = style.height.ToPx(parent_height, style.font_size);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    } else if (flex_target_main_size_ >= 0) {
        // flex container 通过 flex-grow/flex-shrink 分配了固定的主轴尺寸
        // 对于 column 方向的父容器，flex_target_main_size_ 是分配给本元素的高度
        // 对于 row 方向的父容器，flex_target_main_size_ 是分配给本元素的宽度（此处不影响 height）
        // 需要判断：如果父容器是 column 方向，则 flex_target_main_size_ 代表高度
        // 简化处理：当 height 是 auto 且有 flex_target_main_size_ 时，用它作为内容高度
        content_height = flex_target_main_size_;
        height = content_height + padding_top + padding_bottom + border_top + border_bottom;
    }

    // 确定 flex 方向
    bool is_row = (style.flex_direction == "row" || style.flex_direction == "row-reverse");
    bool is_reverse = (style.flex_direction == "row-reverse" || style.flex_direction == "column-reverse");

    // 第一遍：布局所有子元素获取尺寸
    float total_main_size = 0;
    float max_cross_size = 0;

    // 当父 flex 容器自身进入布局流程时，子尺寸必须参与本轮计算，
    // 否则 justify-content/align-items 可能读取到过期子尺寸。
    const bool parent_layout_pass = needs_layout_ || child_needs_layout_;

    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();
        bool should_layout_child = child->NeedsLayout() || parent_layout_pass;

        // 跳过 position:absolute/fixed 子元素 - 它们不参与 flex 尺寸计算
        // 但仍需布局以计算自身尺寸
        if (child_style.position == "absolute" || child_style.position == "fixed") {
            if (should_layout_child) {
                child->Layout(content_width, content_height > 0 ? content_height : 0);
            }
            continue;
        }

        // 🔍 DEBUG: 输出调用栈
        static bool debug_layout = std::getenv("DEBUG_INCREMENTAL_PAINT") != nullptr;
        if (debug_layout && child->GetType() == RenderObjectType::TEXT) {
            auto text_child = std::dynamic_pointer_cast<RenderText>(child);
            if (text_child) {
                std::string text = text_child->GetText();
                if (text.length() > 20) text = text.substr(0, 20) + "...";
            }
        }

        if (should_layout_child) {
            child->Layout(content_width, content_height > 0 ? content_height : 0);
        }

        if (debug_layout && child->GetType() == RenderObjectType::TEXT) {
            auto text_child = std::dynamic_pointer_cast<RenderText>(child);
            if (text_child) {
                std::string text = text_child->GetText();
                if (text.length() > 20) text = text.substr(0, 20) + "...";
            }
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

    // 如果高度是 auto 且没有被 flex container 分配固定尺寸，根据内容计算
    if (style.height.IsAuto() && flex_target_main_size_ < 0) {
        if (is_row) {
            content_height = max_cross_size;
        } else {
            content_height = total_main_size;
        }
        height = content_height + padding_top + padding_bottom + border_top + border_bottom;
    }

    // 应用 min-height 和 max-height
    if (!style.min_height.IsZero()) {
        float min_h = style.min_height.ToPx(parent_height, style.font_size);
        height = std::max(height, min_h);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    }
    if (style.max_height.unit != CSSUnit::NONE) {
        float max_h = style.max_height.ToPx(parent_height, style.font_size);
        height = std::min(height, max_h);
        content_height = height - padding_top - padding_bottom - border_top - border_bottom;
    }

    // 计算主轴可用空间
    float main_size = is_row ? content_width : content_height;
    float cross_size = is_row ? content_height : content_width;

    // === flex-grow / flex-shrink 分配 ===
    // 收集参与 flex 布局的子元素信息，使用 flex-basis 计算 base_main_size
    struct FlexChildInfo {
        size_t index;
        float flex_grow;
        float flex_shrink;
        float base_main_size;  // flex base size（基于 flex-basis）
        float margin_main;     // 主轴方向 margin 总和
    };
    std::vector<FlexChildInfo> flex_children;
    float total_flex_grow = 0.0f;
    float total_flex_shrink_scaled = 0.0f;
    float total_base_main = 0.0f;  // 基于 flex-basis 的总主轴尺寸

    for (size_t i = 0; i < children_.size(); ++i) {
        auto& cs = children_[i]->GetComputedStyle();
        if (cs.position == "absolute" || cs.position == "fixed") continue;

        auto& cl = children_[i]->GetLayoutInfo();
        float child_natural_main = is_row ? cl.width : cl.height;

        // 根据 flex-basis 确定 base_main_size
        // flex-basis: auto → 使用自然尺寸（布局后的实际尺寸）
        // flex-basis: 0%, 100px 等 → 使用 flex-basis 计算值
        float base_main;
        if (cs.flex_basis.IsAuto()) {
            base_main = child_natural_main;
        } else {
            base_main = cs.flex_basis.ToPx(main_size, cs.font_size);
        }

        float m_start = is_row ?
            cs.margin.left.ToPx(width, cs.font_size) :
            cs.margin.top.ToPx(width, cs.font_size);
        float m_end = is_row ?
            cs.margin.right.ToPx(width, cs.font_size) :
            cs.margin.bottom.ToPx(width, cs.font_size);

        total_flex_grow += cs.flex_grow;
        if (base_main > 0) {
            total_flex_shrink_scaled += cs.flex_shrink * base_main;
        }
        total_base_main += base_main + m_start + m_end;
        flex_children.push_back({i, cs.flex_grow, cs.flex_shrink, base_main, m_start + m_end});
    }

    // 基于 flex-basis 计算 free_space（而非自然尺寸）
    float free_space = main_size - total_base_main;

    // 🔍 DEBUG: 写文件日志（定位问题后删除）
    {
        static FILE* dbg = nullptr;
        if (!dbg) dbg = fopen("D:\\code\\C\\MBink\\flex_debug.log", "w");
        if (dbg) {
            auto nd = GetNode();
            std::string tn = "?", cn = "";
            if (nd && nd->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto el = std::static_pointer_cast<Element>(nd);
                tn = el->GetTagName(); cn = el->GetAttribute("class");
            }
            fprintf(dbg, "[FLEX] <%s class='%s'> dir=%s main_size=%.1f total_natural=%.1f total_base=%.1f free=%.1f content_h=%.1f h=%.1f parent_h=%.1f flex_target=%.1f\n",
                tn.c_str(), cn.c_str(), style.flex_direction.c_str(), main_size, total_main_size, total_base_main, free_space, content_height, height, parent_height, flex_target_main_size_);
            for (auto& fi : flex_children) {
                auto& cs2 = children_[fi.index]->GetComputedStyle();
                auto& cl2 = children_[fi.index]->GetLayoutInfo();
                auto cn2 = children_[fi.index]->GetNode();
                std::string t2 = "?", c2 = "";
                if (cn2 && cn2->GetNodeType() == NodeType::ELEMENT_NODE) {
                    auto e2 = std::static_pointer_cast<Element>(cn2);
                    t2 = e2->GetTagName(); c2 = e2->GetAttribute("class");
                }
                fprintf(dbg, "  [%zu] <%s class='%s'> grow=%.1f shrink=%.1f basis_unit=%d basis_val=%.1f base_main=%.1f natural_h=%.1f\n",
                    fi.index, t2.c_str(), c2.c_str(), fi.flex_grow, fi.flex_shrink,
                    (int)cs2.flex_basis.unit, cs2.flex_basis.value, fi.base_main_size, cl2.height);
            }
            fprintf(dbg, "  => will %s (free=%.1f grow_total=%.1f shrink_total=%.1f)\n",
                (free_space > 0 && total_flex_grow > 0) ? "GROW" : (free_space < 0 && total_flex_shrink_scaled > 0) ? "SHRINK" : "NONE",
                free_space, total_flex_grow, total_flex_shrink_scaled);
            fflush(dbg);
        }
    }

    if (free_space > 0 && total_flex_grow > 0) {
        // Growing: 按 flex-grow 比例分配剩余空间
        for (auto& item : flex_children) {
            if (item.flex_grow <= 0) continue;
            float extra = free_space * (item.flex_grow / total_flex_grow);
            float new_main = item.base_main_size + extra;
            auto& child = children_[item.index];
            // 告诉子元素它被 flex container 分配了固定的主轴尺寸
            child->SetFlexTargetMainSize(new_main);
            if (is_row) {
                child->Layout(new_main, content_height > 0 ? content_height : 0);
            } else {
                child->Layout(content_width, new_main);
            }
            child->SetFlexTargetMainSize(-1.0f);  // 重置，避免污染后续布局
        }
        // 重新计算 total_main_size 和 free_space
        total_main_size = 0;
        for (auto& item : flex_children) {
            auto& cl = children_[item.index]->GetLayoutInfo();
            float child_main = is_row ? cl.width : cl.height;
            total_main_size += child_main + item.margin_main;
        }
        free_space = main_size - total_main_size;
    } else if (free_space < 0 && total_flex_shrink_scaled > 0) {
        // Shrinking: 按 flex-shrink * base_size 比例收缩
        for (auto& item : flex_children) {
            if (item.flex_shrink <= 0 || item.base_main_size <= 0) continue;
            float shrink_ratio = (item.flex_shrink * item.base_main_size) / total_flex_shrink_scaled;
            float shrink_amount = (-free_space) * shrink_ratio;
            float new_main = std::max(0.0f, item.base_main_size - shrink_amount);
            if (new_main != item.base_main_size) {
                auto& child = children_[item.index];
                child->SetFlexTargetMainSize(new_main);
                if (is_row) {
                    child->Layout(new_main, content_height > 0 ? content_height : 0);
                } else {
                    child->Layout(content_width, new_main);
                }
                child->SetFlexTargetMainSize(-1.0f);
            }
        }
        // 重新计算 total_main_size 和 free_space
        total_main_size = 0;
        for (auto& item : flex_children) {
            auto& cl = children_[item.index]->GetLayoutInfo();
            float child_main = is_row ? cl.width : cl.height;
            total_main_size += child_main + item.margin_main;
        }
        free_space = main_size - total_main_size;
    }

    // 根据 justify-content 计算主轴起始位置
    float main_start = 0;
    float gap = 0;

    // 计算参与 flex 布局的子元素数量（排除 absolute/fixed）
    size_t num_children = flex_children.size();

    if (style.justify_content == "flex-start" || style.justify_content == "start") {
        main_start = is_reverse ? free_space : 0;
    } else if (style.justify_content == "flex-end" || style.justify_content == "end") {
        main_start = is_reverse ? 0 : free_space;
    } else if (style.justify_content == "center") {
        // Safe center: 防止 free_space 为负时内容向上溢出
        main_start = std::max(0.0f, free_space / 2.0f);
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

    // 第二遍：设置子元素位置
    float current_main = main_start;

    for (size_t i = 0; i < children_.size(); ++i) {
        size_t idx = is_reverse ? (children_.size() - 1 - i) : i;
        auto& child = children_[idx];
        auto& child_layout = child->GetLayoutInfo();
        auto& child_style = child->GetComputedStyle();

        // 跳过 position:absolute/fixed 子元素 - 第三遍单独处理
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
        // CSS flex 的默认交叉轴行为应接近 stretch。
        // 当前样式默认值是 normal，若不映射为 stretch，子项会按内容宽度收缩，
        // 导致如 CodeMirror gutter 这类 column flex 容器中的行号元素宽度抖动。
        if (align.empty() || align == "normal") {
            align = "stretch";
        }

        if (align == "flex-start" || align == "start") {
            cross_offset = 0;
        } else if (align == "flex-end" || align == "end") {
            cross_offset = cross_free_space;
        } else if (align == "center") {
            cross_offset = cross_free_space / 2.0f;
        } else if (align == "stretch") {
            cross_offset = 0;
            if (cross_free_space > 0) {
                if (is_row) {
                    child_layout.height += cross_free_space;
                } else {
                    child_layout.width += cross_free_space;
                }
            }
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

    // 第三遍：处理 position:absolute/fixed 子元素的定位
    // absolute/fixed 子元素相对于容器的 content box 定位
    for (auto& child : children_) {
        auto& child_style = child->GetComputedStyle();
        if (child_style.position != "absolute" && child_style.position != "fixed") {
            continue;
        }

        auto& child_layout = child->GetLayoutInfo();

        bool has_left = !child_style.left.IsAuto();
        bool has_top = !child_style.top.IsAuto();
        bool has_right = !child_style.right.IsAuto();
        bool has_bottom = !child_style.bottom.IsAuto();

        float container_w = content_width;
        float container_h = content_height;

        // 水平定位
        if (has_left) {
            child_layout.x = padding_left + border_left + child_style.left.ToPx(container_w, child_style.font_size);
        } else if (has_right) {
            child_layout.x = padding_left + border_left + container_w - child_layout.width - child_style.right.ToPx(container_w, child_style.font_size);
        } else {
            child_layout.x = padding_left + border_left;
        }

        // 垂直定位
        if (has_top) {
            child_layout.y = padding_top + border_top + child_style.top.ToPx(container_h, child_style.font_size);
        } else if (has_bottom) {
            child_layout.y = padding_top + border_top + container_h - child_layout.height - child_style.bottom.ToPx(container_h, child_style.font_size);
        } else {
            child_layout.y = padding_top + border_top;
        }
    }

    // 设置布局信息
    layout_info_.width = width;
    layout_info_.height = height;

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

void RenderFlex::Paint(SkCanvas* canvas) {
    // 复用 RenderInlineFlex 的 Paint 逻辑
    // 因为块级和内联的绘制逻辑是相同的

    static bool debug_paint = std::getenv("DEBUG_INCREMENTAL_PAINT") != nullptr;

    if (!canvas) {
        if (debug_paint) {
        }
        return;
    }

    if (debug_paint) {
        auto node = GetNode();
        std::string tag_name = "?";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            tag_name = elem->GetTagName();
        }
    }

    // 保存画布状态
    canvas->save();
    canvas->translate(layout_info_.x, layout_info_.y);

    const auto& style = computed_style_;
    const auto& layout = layout_info_;

    // 应用 CSS opacity
    bool has_opacity = style.opacity < 1.0f;
    if (has_opacity) {
        SkRect bounds = SkRect::MakeWH(layout.width, layout.height);
        int alpha = static_cast<int>(style.opacity * 255);
        canvas->saveLayerAlpha(&bounds, alpha);
    }

    // 应用 CSS transform
    if (style.transform.has_value() && !style.transform->IsEmpty()) {
        SkRect element_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(element_rect, style.transform_origin);
        canvas->concat(transform_matrix);
    }

    // 创建盒模型
    Box box;
    box.padding_left = style.padding.left.ToPx(layout.width, style.font_size);
    box.padding_right = style.padding.right.ToPx(layout.width, style.font_size);
    box.padding_top = style.padding.top.ToPx(layout.width, style.font_size);
    box.padding_bottom = style.padding.bottom.ToPx(layout.width, style.font_size);

    box.border_left_width = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    box.border_right_width = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    box.border_top_width = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    box.border_bottom_width = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();

    box.content_x = box.border_left_width + box.padding_left;
    box.content_y = box.border_top_width + box.padding_top;
    box.content_width = layout.width - box.border_left_width - box.border_right_width - box.padding_left - box.padding_right;
    box.content_height = layout.height - box.border_top_width - box.border_bottom_width - box.padding_top - box.padding_bottom;

    // 渲染器
    BoxRenderer renderer(canvas);

    // 渲染阴影
    if (!style.box_shadow.empty()) {
        renderer.RenderBoxShadow(box, style.box_shadow, &style.border_radius);
    }

    // 渲染背景
    SkRect padding_box = box.GetPaddingBox();

    // 背景色作为最底层
    if (!style.background_color.empty() && style.background_color != "transparent") {
        SkPaint bg_paint;
        bg_paint.setColor(CSSValue::ParseColor(style.background_color));
        bg_paint.setAntiAlias(true);

        float box_width = padding_box.width();
        float box_height = padding_box.height();
        float base_size = std::min(box_width, box_height);

        float tl = style.border_radius.top_left.ToPx(base_size);
        float tr = style.border_radius.top_right.ToPx(base_size);
        float br = style.border_radius.bottom_right.ToPx(base_size);
        float bl = style.border_radius.bottom_left.ToPx(base_size);

        if (tl > 0 || tr > 0 || br > 0 || bl > 0) {
            SkRRect rrect;
            SkVector radii[4] = {
                {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
            };
            rrect.setRectRadii(padding_box, radii);
            canvas->drawRRect(rrect, bg_paint);
        } else {
            canvas->drawRect(padding_box, bg_paint);
        }
    }

    // 渲染渐变（如果有）
    if (!style.background_linear_gradients.empty()) {
        for (int i = static_cast<int>(style.background_linear_gradients.size()) - 1; i >= 0; i--) {
            const auto& gradient = style.background_linear_gradients[i];
            GradientRenderer::RenderLinearGradient(canvas, padding_box, gradient);
        }
    } else if (style.background_linear_gradient.has_value()) {
        GradientRenderer::RenderLinearGradient(canvas, padding_box, *style.background_linear_gradient);
    } else if (style.background_radial_gradient.has_value()) {
        GradientRenderer::RenderRadialGradient(canvas, padding_box, *style.background_radial_gradient);
    }

    // 渲染边框
    bool has_border = (box.border_left_width > 0 || box.border_right_width > 0 ||
                       box.border_top_width > 0 || box.border_bottom_width > 0);
    if (has_border) {
        float border_widths[4] = {
            box.border_top_width, box.border_right_width,
            box.border_bottom_width, box.border_left_width
        };
        CSSBorderStyle border_styles[4] = {
            style.border_top_style != CSSBorderStyle::NONE ? style.border_top_style : style.border.style,
            style.border_right_style != CSSBorderStyle::NONE ? style.border_right_style : style.border.style,
            style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_style : style.border.style,
            style.border_left_style != CSSBorderStyle::NONE ? style.border_left_style : style.border.style
        };
        SkColor border_colors[4] = {
            style.border_top_style != CSSBorderStyle::NONE ? style.border_top_color : style.border.color,
            style.border_right_style != CSSBorderStyle::NONE ? style.border_right_color : style.border.color,
            style.border_bottom_style != CSSBorderStyle::NONE ? style.border_bottom_color : style.border.color,
            style.border_left_style != CSSBorderStyle::NONE ? style.border_left_color : style.border.color
        };
        renderer.RenderRoundedBorderAdvanced(box, border_widths, border_styles, border_colors, style.border_radius);
    }

    // 应用 overflow 裁剪（与 RenderBlock 行为对齐）
    bool needs_clip = false;
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    auto isOverflowSet = [](const std::string& v) {
        return v == "hidden" || v == "scroll" || v == "auto";
    };
    if (isOverflowSet(overflow_x) || isOverflowSet(overflow_y)) {
        needs_clip = true;
        SkRect clip_rect = SkRect::MakeXYWH(
            box.border_left_width,
            box.border_top_width,
            layout.width - box.border_left_width - box.border_right_width,
            layout.height - box.border_top_width - box.border_bottom_width
        );
        canvas->save();
        canvas->clipRect(clip_rect, SkClipOp::kIntersect, true);
    }

    // 绘制子元素
    // ⚠️ 关键：有独立合成层的子元素必须跳过，避免父层重复绘制造成重影/双实例
    for (auto& child : children_) {
        if (child->HasOwnCompositorLayer()) {
            continue;
        }
        child->Paint(canvas);
    }

    // 恢复 overflow 裁剪状态
    if (needs_clip) {
        canvas->restore();
    }

    // 恢复画布状态
    if (has_opacity) {
        canvas->restore(); // 恢复 opacity layer
    }
    canvas->restore(); // 恢复 translate

    needs_paint_ = false;
}

} // namespace lightui
