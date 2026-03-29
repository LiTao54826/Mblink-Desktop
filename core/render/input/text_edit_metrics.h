#pragma once

#include <string>

class SkFont;

namespace mbink::text_edit_metrics {

float MeasureTextWidth(const std::string& text,
                       const SkFont& font,
                       bool mask_as_password = false);

float MeasurePrefixWidth(const std::string& text,
                         int char_pos,
                         const SkFont& font,
                         bool mask_as_password = false);

int HitTestTextPosition(const std::string& text,
                        float local_x,
                        const SkFont& font,
                        bool mask_as_password = false);

}  // namespace mbink::text_edit_metrics

