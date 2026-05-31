#include "text_edit_metrics.h"

#include "core/render/text/text_renderer.h"
#include "core/utils/utf8_utils.h"

#include <algorithm>

namespace mbink::text_edit_metrics {

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

std::vector<int> ComputeRenderedLineStartOffsets(const std::string& text,
                                                 const std::vector<std::string>& lines) {
    std::vector<int> starts;
    starts.reserve(lines.size());

    size_t search_byte = 0;
    int fallback_char = 0;
    for (const auto& line : lines) {
        if (line.empty()) {
            starts.push_back(fallback_char);
            continue;
        }

        const size_t found = text.find(line, search_byte);
        if (found == std::string::npos) {
            starts.push_back(fallback_char);
            fallback_char += static_cast<int>(utf8::CharCount(line));
            search_byte = utf8::CharPosToBytePos(text, static_cast<size_t>(fallback_char));
            continue;
        }

        const int start = static_cast<int>(utf8::BytePosToCharPos(text, found));
        starts.push_back(start);
        search_byte = found + line.size();
        fallback_char = static_cast<int>(utf8::BytePosToCharPos(text, search_byte));
    }

    return starts;
}

}  // namespace mbink::text_edit_metrics
