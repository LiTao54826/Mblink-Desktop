#pragma once

#include "core/render/input/input_paint_model.h"

class SkFont;
struct SkFontMetrics;

namespace mblink::input_ime_geometry {

struct Request {
    const InputPaintModel* model = nullptr;
    const SkFont* font = nullptr;
    const SkFontMetrics* metrics = nullptr;
    float content_x = 0.0f;
    float content_y = 0.0f;
    float content_w = 0.0f;
    float font_size = 0.0f;
    float scroll_left = 0.0f;
    float spinner_reserved_width = 0.0f;
};

struct Result {
    float area_x = 0.0f;
    float area_y = 0.0f;
    float area_w = 1.0f;
    float area_h = 1.0f;
    float caret_x = 0.0f;
    float caret_offset = 0.0f;
    float resolved_scroll_left = 0.0f;
};

Result ResolveInputGeometry(const Request& request);

}  // namespace mblink::input_ime_geometry
