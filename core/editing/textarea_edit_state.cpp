#include "textarea_edit_state.h"

#include <algorithm>

namespace lightui {

int TextAreaEditState::GetSelectionStart() const {
    return std::min(selection_anchor, selection_focus);
}

int TextAreaEditState::GetSelectionEnd() const {
    return std::max(selection_anchor, selection_focus);
}

bool TextAreaEditState::HasSelection() const {
    return selection_anchor != selection_focus;
}

bool TextAreaEditState::HasActiveComposition() const {
    return composition_state.active;
}

void TextAreaEditState::SetText(const std::string& new_text) {
    if (text == new_text) {
        return;
    }
    text = new_text;
    ++revision_id;
    MarkDirty(EditDirtyFlags::TextChanged);
}

void TextAreaEditState::SetSelection(int anchor, int focus) {
    if (selection_anchor == anchor && selection_focus == focus) {
        return;
    }

    bool caret_changed = caret_position != focus;
    selection_anchor = anchor;
    selection_focus = focus;
    caret_position = focus;
    MarkDirty(EditDirtyFlags::SelectionChanged);
    if (caret_changed) {
        MarkDirty(EditDirtyFlags::CaretChanged);
    }
}

void TextAreaEditState::SetCaretPosition(int position) {
    if (selection_anchor == position && selection_focus == position && caret_position == position) {
        return;
    }

    selection_anchor = position;
    selection_focus = position;
    caret_position = position;
    MarkDirty(EditDirtyFlags::SelectionChanged | EditDirtyFlags::CaretChanged);
}

void TextAreaEditState::SetPreferredColumn(int column) {
    if (preferred_column == column) {
        return;
    }
    preferred_column = column;
    MarkDirty(EditDirtyFlags::GeometryChanged);
}

void TextAreaEditState::MarkDirty(EditDirtyFlags flag) {
    dirty_flags |= flag;
}

void TextAreaEditState::ClearDirtyFlags() {
    dirty_flags = EditDirtyFlags::None;
}

std::shared_ptr<TextAreaEditState> CreateTextAreaEditState() {
    return std::make_shared<TextAreaEditState>();
}

}  // namespace lightui

