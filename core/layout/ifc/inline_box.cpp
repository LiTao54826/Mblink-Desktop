/**
 * @file inline_box.cpp
 * @brief InlineBox 结构的实现
 */

#include "inline_box.h"
#include "render/render_object.h"

namespace lightui {

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
        
        // 从样式中获取左侧的边距、内边距和边框
        box.margin_left = style.margin.left.ToPx();
        box.padding_left = style.padding.left.ToPx();
        box.border_left = style.border.width.ToPx();
        
        // 宽度为0（只包含边距等，没有内容）
        box.width = 0;
    }
    
    return box;
}

InlineBox InlineBox::CreateInlineEnd(RenderObject* render_obj) {
    InlineBox box(InlineBoxType::INLINE_END);
    box.render_object = render_obj;
    
    if (render_obj) {
        box.style = &render_obj->GetComputedStyle();
        const auto& style = render_obj->GetComputedStyle();
        
        // 从样式中获取右侧的边距、内边距和边框
        box.margin_right = style.margin.right.ToPx();
        box.padding_right = style.padding.right.ToPx();
        box.border_right = style.border.width.ToPx();
        
        // 宽度为0（只包含边距等，没有内容）
        box.width = 0;
    }
    
    return box;
}

} // namespace lightui

