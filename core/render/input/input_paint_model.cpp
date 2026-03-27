#include "input_paint_model.h"

#include "core/dom/elements/html_input_element.h"
#include "core/utils/utf8_utils.h"

#include <algorithm>

namespace lightui {

InputPaintModel InputPaintModel::FromInputElement(const HTMLInputElement* input) {
    InputPaintModel model;
    if (!input) {
        return model;
    }

    model.is_password = input->GetInputType() == InputType::Password;
    model.value = input->GetValue();
    model.display_text = model.value;
    model.visual_text = model.value;
    model.selection_start = input->GetSelectionStart();
    model.selection_end = input->GetSelectionEnd();
    model.caret_position = input->GetSelectionEnd();

    auto edit_state = input->GetEditState();
    if (edit_state && edit_state->HasActiveComposition()) {
        const auto& composition = edit_state->composition_state;
        int start = std::max(0, composition.start);
        int end = std::max(start, composition.end);
        size_t start_byte = utf8::CharPosToBytePos(model.value, start);
        size_t end_byte = utf8::CharPosToBytePos(model.value, end);
        model.visual_text = model.value.substr(0, start_byte) + composition.text + model.value.substr(end_byte);
        model.composition_start = start;
        model.composition_end = start + static_cast<int>(utf8::CharCount(composition.text));
        model.selection_start = model.composition_start;
        model.selection_end = model.composition_end;
        model.caret_position = model.composition_end;
        model.has_composition = true;
    }

    if (model.value.empty() && !model.has_composition) {
        model.display_text = input->GetPlaceholder();
        model.visual_text = model.display_text;
        model.is_placeholder = true;
    } else if (model.is_password) {
        model.display_text = std::string(model.value.size(), '*');
        model.visual_text = model.has_composition
            ? std::string(model.visual_text.size(), '*')
            : model.display_text;
    } else {
        model.display_text = model.visual_text;
    }

    return model;
}

}  // namespace lightui
