/**
 * @file inline_box.cpp
 * @brief InlineBox 结构的实现
 */

#include "inline_box.h"
#include "core/render/objects/render_object.h"
#include <cmath>

namespace mblink {

InlineBox InlineBox::CreateTextBox(RenderObject* render_obj) {
    InlineBox box(InlineBoxType::TEXT);
    box.render_object = render_obj;
    if (render_obj) {
        box.style = &render_obj->GetComputedStyle();
    }
    return box;
}

InlineBox InlineBox::CreateAtomicBox(RenderObject* render_obj, float w, float h, float bl) {
    InlineBox box(InlineBoxType::ATOMIC);
    box.render_object = render_obj;
    box.width = w;
    box.height = h;
    box.baseline = bl;
    if (render_obj) {
        box.style = &render_obj->GetComputedStyle();
    }
    return box;
}

InlineBox InlineBox::CreateInlineStart(RenderObject* render_obj) {
    InlineBox box(InlineBoxType::INLINE_START);
    box.render_object = render_obj;

    if (render_obj) {
        box.style = &render_obj->GetComputedStyle();
        const auto& style = render_obj->GetComputedStyle();

        // 读取左侧盒模型（用于内联元素首端）
        // 传入 font_size 以正确处理 em/rem 等单位
        box.margin_left = style.margin.left.ToPx(0.0f, style.font_size);
        box.padding_left = style.padding.left.ToPx(0.0f, style.font_size);

        // 优先使用分侧边框宽度，回退到统一 border.width
        box.border_left = style.border_left_width;
        if (box.border_left == 0.0f) {
            box.border_left = style.border.width.ToPx(0.0f, style.font_size);
        }

        // 标记盒本身无内容宽度
        box.width = 0.0f;

        // [Bug1 Fix] 设置 INLINE_START 的 strut 高度：
        // CSS 规范要求每个内联盒都有一个基于自身 font-size 和 line-height 的 strut，
        // 让 inline 元素（如 <span>）自身的 line-height 能参与行高计算。
        // line_height_multiplier 若为默认值 1.2 则使用浏览器 normal 估算值（~1.156）。
        float lh_multiplier = style.line_height;
        float effective_lh;
        if (std::abs(lh_multiplier - 1.2f) < 0.001f) {
            effective_lh = style.font_size * 1.156f;
        } else {
            effective_lh = style.font_size * lh_multiplier;
        }
        box.height = effective_lh;
        box.line_height_multiplier = lh_multiplier;
        // skia_ascent/descent 按 80/20 估算，与 CalculateLineMetrics 的 strut 逻辑一致
        box.skia_ascent = effective_lh * 0.8f;
        box.skia_descent = effective_lh * 0.2f;
        box.baseline = box.skia_ascent;
    }

    return box;
}

InlineBox InlineBox::CreateInlineEnd(RenderObject* render_obj) {
    InlineBox box(InlineBoxType::INLINE_END);
    box.render_object = render_obj;

    if (render_obj) {
        box.style = &render_obj->GetComputedStyle();
        const auto& style = render_obj->GetComputedStyle();

        // 读取右侧盒模型（用于内联元素末端）
        box.margin_right = style.margin.right.ToPx(0.0f, style.font_size);
        box.padding_right = style.padding.right.ToPx(0.0f, style.font_size);

        // 优先使用分侧边框宽度，回退到统一 border.width
        box.border_right = style.border_right_width;
        if (box.border_right == 0.0f) {
            box.border_right = style.border.width.ToPx(0.0f, style.font_size);
        }

        // 标记盒本身无内容宽度
        box.width = 0.0f;
    }

    return box;
}

} // namespace mblink

