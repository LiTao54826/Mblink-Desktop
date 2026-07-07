#pragma once

#include "include/core/SkFont.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace mblink {

class HTMLTextAreaElement;
class RenderObject;

namespace textarea_metrics {

struct LineSpan {
    size_t byte_start = 0;
    size_t byte_end = 0;
    int char_start = 0;
    int char_end = 0;
};

struct BoxMetrics {
    SkFont font;
    float line_height = 1.0f;
    float border_left = 0.0f;
    float border_right = 0.0f;
    float border_top = 0.0f;
    float border_bottom = 0.0f;
    float padding_left = 0.0f;
    float padding_right = 0.0f;
    float padding_top = 0.0f;
    float padding_bottom = 0.0f;
    float content_left = 0.0f;
    float content_top = 0.0f;
    float content_width = 0.0f;
    float content_height = 0.0f;
    float visible_width = 0.0f;
    float visible_height = 0.0f;
    float client_width = 0.0f;
    float client_height = 0.0f;
    float scroll_width = 0.0f;
    float scroll_height = 0.0f;
    float text_content_height = 0.0f;
    float max_line_width = 0.0f;
    float max_scroll_x = 0.0f;
    float max_scroll_y = 0.0f;
    bool need_v_scrollbar = false;
    bool need_h_scrollbar = false;
};

bool ResolveBox(const HTMLTextAreaElement* textarea,
                const RenderObject* render_object,
                BoxMetrics& metrics);

float ClampScrollTop(float scroll_top, const BoxMetrics& metrics);
float ClampScrollLeft(float scroll_left, const BoxMetrics& metrics);

std::vector<LineSpan> BuildLineSpans(const std::string& value);
std::string LineText(const std::string& value, const LineSpan& line);
std::pair<std::string, int> LineForHitTest(const std::string& value, int target_line);
std::pair<int, int> FindLineColumn(const std::vector<LineSpan>& lines, int char_pos);
std::pair<int, int> VisibleLineRange(float scroll_top,
                                     float visible_height,
                                     float line_height,
                                     int line_count);

}  // namespace textarea_metrics
}  // namespace mblink
