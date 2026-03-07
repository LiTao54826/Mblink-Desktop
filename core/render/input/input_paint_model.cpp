#include "input_paint_model.h"

#include "core/dom/elements/html_input_element.h"

namespace lightui {

InputPaintModel InputPaintModel::FromInputElement(const HTMLInputElement* input) {
    InputPaintModel model;
    if (!input) {
        return model;
    }

    model.is_password = input->GetInputType() == InputType::Password;
    model.value = input->GetValue();
    model.display_text = model.value;
    model.selection_start = input->GetSelectionStart();
    model.selection_end = input->GetSelectionEnd();
    model.caret_position = input->GetSelectionEnd();

    if (model.value.empty()) {
        model.display_text = input->GetPlaceholder();
        model.is_placeholder = true;
    } else if (model.is_password) {
        model.display_text = std::string(model.value.size(), '*');
    }

    return model;
}

}  // namespace lightui

