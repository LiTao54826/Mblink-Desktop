#include "input_ime_geometry.h"

#include "core/render/input/input_text_viewport.h"
#include "core/render/input/text_edit_metrics.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"

#include <algorithm>

namespace mblink::input_ime_geometry {

Result ResolveInputGeometry(const Request& request) {
    Result result;
    result.area_x = request.content_x;
    result.area_y = request.content_y;
    result.area_w = std::max(1.0f, input_text_viewport::VisibleWidth(
        request.content_w,
        request.spinner_reserved_width));

    if (request.metrics) {
        result.area_h = std::max(1.0f,
                                 std::max(request.font_size,
                                          -request.metrics->fAscent + request.metrics->fDescent));
    } else {
        result.area_h = std::max(1.0f, request.font_size);
    }

    result.caret_x = result.area_x;
    if (!request.model || !request.font) {
        result.caret_offset = 0.0f;
        return result;
    }

    int active_char = request.model->HasComposition()
        ? request.model->composition_start
        : input_text_viewport::ActiveCharPosition(*request.model);
    auto viewport = input_text_viewport::Resolve({
        request.model,
        request.font,
        request.content_w,
        request.spinner_reserved_width,
        request.scroll_left,
        active_char
    });
    result.resolved_scroll_left = viewport.scroll_left;

    const bool mask_as_password = request.model->is_password && !request.model->is_placeholder;
    float prefix_width = text_edit_metrics::MeasurePrefixWidth(request.model->visual_text,
                                                               active_char,
                                                               *request.font,
                                                               mask_as_password);
    result.caret_x = request.content_x + prefix_width - viewport.scroll_left;
    result.caret_offset = std::clamp(result.caret_x - result.area_x, 0.0f, result.area_w);
    return result;
}

}  // namespace mblink::input_ime_geometry
