#pragma once

#include <algorithm>
#include <memory>

#include "render_object.h"

namespace mbink {

inline void MarkPositionedSubtreeNeedsLayout(const std::shared_ptr<RenderObject>& obj) {
    if (!obj) {
        return;
    }

    obj->MarkNeedsLayout(false);
    for (const auto& child : obj->GetChildren()) {
        MarkPositionedSubtreeNeedsLayout(child);
    }
}

inline void LayoutPositionedChild(const std::shared_ptr<RenderObject>& child,
                                  float containing_width,
                                  float containing_height,
                                  float offset_x,
                                  float offset_y) {
    if (!child) {
        return;
    }

    auto& style = child->GetComputedStyle();
    if (style.position != "absolute" && style.position != "fixed") {
        return;
    }

    const bool is_fixed = style.position == "fixed";
    const float container_w = is_fixed && RenderObject::GetViewportWidth() > 0.0f
                                  ? RenderObject::GetViewportWidth()
                                  : containing_width;
    const float container_h = is_fixed && RenderObject::GetViewportHeight() > 0.0f
                                  ? RenderObject::GetViewportHeight()
                                  : containing_height;
    const float base_x = is_fixed ? 0.0f : offset_x;
    const float base_y = is_fixed ? 0.0f : offset_y;

    const bool has_left = !style.left.IsAuto();
    const bool has_top = !style.top.IsAuto();
    const bool has_right = !style.right.IsAuto();
    const bool has_bottom = !style.bottom.IsAuto();

    const float left = has_left ? style.left.ToPx(container_w, style.font_size) : 0.0f;
    const float right = has_right ? style.right.ToPx(container_w, style.font_size) : 0.0f;
    const float top = has_top ? style.top.ToPx(container_h, style.font_size) : 0.0f;
    const float bottom = has_bottom ? style.bottom.ToPx(container_h, style.font_size) : 0.0f;

    const bool old_has_external_width = child->HasExternalLayoutWidth();
    const bool old_has_external_height = child->HasExternalLayoutHeight();
    const float old_external_width = child->GetExternalLayoutWidth();
    const float old_external_height = child->GetExternalLayoutHeight();

    child->ClearExternalLayoutConstraints();

    float width = child->GetLayoutInfo().width;
    float height = child->GetLayoutInfo().height;

    if (has_left && has_right && style.width.IsAuto()) {
        width = std::max(0.0f, container_w - left - right);
        child->SetExternalLayoutWidth(width);
    }
    if (has_top && has_bottom && style.height.IsAuto()) {
        height = std::max(0.0f, container_h - top - bottom);
        child->SetExternalLayoutHeight(height);
    }

    const bool constraints_changed =
        old_has_external_width != child->HasExternalLayoutWidth() ||
        old_has_external_height != child->HasExternalLayoutHeight() ||
        (child->HasExternalLayoutWidth() && old_external_width != child->GetExternalLayoutWidth()) ||
        (child->HasExternalLayoutHeight() && old_external_height != child->GetExternalLayoutHeight());
    if (constraints_changed) {
        MarkPositionedSubtreeNeedsLayout(child);
    }

    child->Layout(child->HasExternalLayoutWidth() ? child->GetExternalLayoutWidth() : container_w,
                  child->HasExternalLayoutHeight() ? child->GetExternalLayoutHeight() : container_h);
    width = child->GetLayoutInfo().width;
    height = child->GetLayoutInfo().height;

    auto& layout = child->GetLayoutInfo();

    if (has_left) {
        layout.x = base_x + left;
    } else if (has_right) {
        layout.x = base_x + container_w - width - right;
    } else {
        layout.x = base_x;
    }

    if (has_top) {
        layout.y = base_y + top;
    } else if (has_bottom) {
        layout.y = base_y + container_h - height - bottom;
    } else {
        layout.y = base_y;
    }
}

}  // namespace mbink
