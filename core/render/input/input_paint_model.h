#pragma once

#include <string>

namespace lightui {

class HTMLInputElement;

struct InputPaintModel {
    std::string value;
    std::string display_text;
    int selection_start = 0;
    int selection_end = 0;
    int caret_position = 0;
    bool is_placeholder = false;
    bool is_password = false;

    bool HasSelection() const { return selection_start != selection_end; }

    static InputPaintModel FromInputElement(const HTMLInputElement* input);
};

}  // namespace lightui

