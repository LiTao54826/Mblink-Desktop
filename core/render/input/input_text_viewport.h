#pragma once

#include "input_paint_model.h"

class SkFont;

namespace mblink::input_text_viewport {

constexpr float kNumberSpinnerReservedWidth = 16.0f;
constexpr float kCaretMargin = 2.0f;

struct ViewportRequest {
    const InputPaintModel* model = nullptr;
    const SkFont* font = nullptr;
    float content_width = 0.0f;
    float spinner_reserved_width = 0.0f;
    float scroll_left = 0.0f;
    int active_char_pos = 0;
    bool ensure_active_visible = true;
};

struct ViewportState {
    float visible_width = 0.0f;
    float content_width = 0.0f;
    float text_width = 0.0f;
    float max_scroll_left = 0.0f;
    float scroll_left = 0.0f;
    int active_char_pos = 0;
};

float VisibleWidth(float content_width, float spinner_reserved_width);
int ActiveCharPosition(const InputPaintModel& model);
ViewportState Resolve(const ViewportRequest& request);
float TextOriginX(float content_x, const ViewportState& viewport);
float ContentXFromVisibleX(float local_x, const ViewportState& viewport);
float ClampVisibleX(float local_x, const ViewportState& viewport);

}  // namespace mblink::input_text_viewport
