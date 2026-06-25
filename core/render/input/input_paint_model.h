#pragma once

#include <string>

namespace mblink {

class HTMLInputElement;

struct InputPaintModel {
    std::string value;
    std::string display_text;
    std::string visual_text;
    int selection_start = 0;
    int selection_end = 0;
    int caret_position = 0;
    int composition_start = 0;
    int composition_end = 0;
    bool is_placeholder = false;
    bool is_password = false;
    bool has_composition = false;

    bool HasSelection() const { return selection_start != selection_end; }
    bool HasComposition() const { return has_composition && composition_end >= composition_start; }
    int VisibleSelectionStart() const { return HasComposition() ? composition_start : selection_start; }
    int VisibleSelectionEnd() const { return HasComposition() ? composition_end : selection_end; }
    int VisibleCaretPosition() const { return HasComposition() ? composition_end : caret_position; }

    static InputPaintModel FromInputElement(const HTMLInputElement* input);
};

}  // namespace mblink

