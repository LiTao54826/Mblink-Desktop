#include "text_edit_metrics.h"

#include "core/render/text/text_renderer.h"
#include "core/utils/utf8_utils.h"

#include <algorithm>

namespace lightui::text_edit_metrics {

float MeasureTextWidth(const std::string& text,
                       const SkFont& font,
                       bool mask_as_password) {
    if (text.empty()) {
        return 0.0f;
    }

    TextRenderer renderer(nullptr);
    if (mask_as_password) {
        return renderer.MeasureTextWidthWithEmoji(
            std::string(utf8::CharCount(text), '*'), font);
    }
    return renderer.MeasureTextWidthWithEmoji(text, font);
}

float MeasurePrefixWidth(const std::string& text,
                         int char_pos,
                         const SkFont& font,
                         bool mask_as_password) {
    const int total_chars = static_cast<int>(utf8::CharCount(text));
    const int clamped = std::max(0, std::min(char_pos, total_chars));
    if (clamped == 0) {
        return 0.0f;
    }
    if (mask_as_password) {
        return MeasureTextWidth(std::string(clamped, '*'), font, false);
    }
    return MeasureTextWidth(utf8::SubstrByChar(text, 0, clamped), font, false);
}

int HitTestTextPosition(const std::string& text,
                        float local_x,
                        const SkFont& font,
                        bool mask_as_password) {
    const int total_chars = static_cast<int>(utf8::CharCount(text));
    if (local_x <= 0.0f || total_chars == 0) {
        return 0;
    }

    TextRenderer renderer(nullptr);
    float accumulated_width = 0.0f;
    for (int i = 0; i < total_chars; ++i) {
        std::string char_str = mask_as_password
            ? std::string(1, '*')
            : utf8::SubstrByChar(text, i, i + 1);
        float char_width = renderer.MeasureTextWidthWithEmoji(char_str, font);
        if (local_x < accumulated_width + char_width / 2.0f) {
            return i;
        }
        accumulated_width += char_width;
    }
    return total_chars;
}

}  // namespace lightui::text_edit_metrics

