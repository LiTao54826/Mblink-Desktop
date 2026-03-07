#include "input_edit_state.h"

#include <algorithm>

namespace lightui {

namespace {
constexpr uint32_t ToBits(EditDirtyFlags flags) {
    return static_cast<uint32_t>(flags);
}
}

EditDirtyFlags operator|(EditDirtyFlags lhs, EditDirtyFlags rhs) {
    return static_cast<EditDirtyFlags>(ToBits(lhs) | ToBits(rhs));
}

EditDirtyFlags operator&(EditDirtyFlags lhs, EditDirtyFlags rhs) {
    return static_cast<EditDirtyFlags>(ToBits(lhs) & ToBits(rhs));
}

EditDirtyFlags& operator|=(EditDirtyFlags& lhs, EditDirtyFlags rhs) {
    lhs = lhs | rhs;
    return lhs;
}

bool HasDirtyFlag(EditDirtyFlags flags, EditDirtyFlags flag) {
    return ToBits(flags & flag) != 0;
}

int InputEditState::GetSelectionStart() const {
    return std::min(selection_anchor, selection_focus);
}

int InputEditState::GetSelectionEnd() const {
    return std::max(selection_anchor, selection_focus);
}

bool InputEditState::HasSelection() const {
    return selection_anchor != selection_focus;
}

bool InputEditState::HasActiveComposition() const {
    return composition_state.active;
}

void InputEditState::SetText(const std::string& new_text) {
    if (text == new_text) {
        return;
    }
    text = new_text;
    ++revision_id;
    MarkDirty(EditDirtyFlags::TextChanged);
}

void InputEditState::SetSelection(int anchor, int focus) {
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

void InputEditState::SetCaretPosition(int position) {
    if (selection_anchor == position && selection_focus == position && caret_position == position) {
        return;
    }

    selection_anchor = position;
    selection_focus = position;
    caret_position = position;
    MarkDirty(EditDirtyFlags::SelectionChanged | EditDirtyFlags::CaretChanged);
}

void InputEditState::MarkDirty(EditDirtyFlags flag) {
    dirty_flags |= flag;
}

void InputEditState::ClearDirtyFlags() {
    dirty_flags = EditDirtyFlags::None;
}

std::shared_ptr<InputEditState> CreateInputEditState() {
    return std::make_shared<InputEditState>();
}


}  // namespace lightui

