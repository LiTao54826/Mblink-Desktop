#include "input_text_viewport.h"

#include "text_edit_metrics.h"
#include "include/core/SkFont.h"

#include <algorithm>

namespace mbink::input_text_viewport {

namespace {

bool MaskAsPassword(const InputPaintModel& model) {
    return model.is_password && !model.is_placeholder;
}

float ActiveEdgeX(const InputPaintModel& model, int active_char_pos, const SkFont& font) {
    return text_edit_metrics::MeasurePrefixWidth(model.visual_text,
                                                 active_char_pos,
                                                 font,
                                                 MaskAsPassword(model));
}

}  // namespace

float VisibleWidth(float content_width, float spinner_reserved_width) {
    return std::max(0.0f, content_width - std::max(0.0f, spinner_reserved_width));
}

int ActiveCharPosition(const InputPaintModel& model) {
    if (model.HasComposition()) {
        return model.composition_start;
    }
    return model.VisibleCaretPosition();
}

ViewportState Resolve(const ViewportRequest& request) {
    ViewportState state;
    state.content_width = std::max(0.0f, request.content_width);
    state.visible_width = VisibleWidth(request.content_width, request.spinner_reserved_width);
    state.scroll_left = std::max(0.0f, request.scroll_left);
    state.active_char_pos = request.active_char_pos;

    if (!request.model || !request.font || state.visible_width <= 0.0f) {
        state.scroll_left = 0.0f;
        return state;
    }

    const InputPaintModel& model = *request.model;
    state.text_width = text_edit_metrics::MeasureTextWidth(model.visual_text,
                                                           *request.font,
                                                           MaskAsPassword(model));
    state.max_scroll_left = std::max(0.0f, state.text_width - state.visible_width);
    state.scroll_left = std::min(state.scroll_left, state.max_scroll_left);

    const int active_pos = std::max(0, request.active_char_pos);
    state.active_char_pos = active_pos;
    if (request.ensure_active_visible) {
        const float active_x = ActiveEdgeX(model, active_pos, *request.font);
        const float right_limit = state.scroll_left + state.visible_width - kCaretMargin;
        if (active_x > right_limit) {
            state.scroll_left = active_x - state.visible_width + kCaretMargin;
        } else if (active_x < state.scroll_left + kCaretMargin) {
            state.scroll_left = std::max(0.0f, active_x - kCaretMargin);
        }
    }

    state.scroll_left = std::clamp(state.scroll_left, 0.0f, state.max_scroll_left);
    return state;
}

float TextOriginX(float content_x, const ViewportState& viewport) {
    return content_x - viewport.scroll_left;
}

float ContentXFromVisibleX(float local_x, const ViewportState& viewport) {
    return ClampVisibleX(local_x, viewport) + viewport.scroll_left;
}

float ClampVisibleX(float local_x, const ViewportState& viewport) {
    if (viewport.visible_width <= 0.0f) {
        return 0.0f;
    }
    return std::clamp(local_x, 0.0f, viewport.visible_width);
}

}  // namespace mbink::input_text_viewport
