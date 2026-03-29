#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "input_edit_state.h"

namespace mbink {

struct TextAreaEditState {
    std::string text;
    int selection_anchor = 0;
    int selection_focus = 0;
    int caret_position = 0;
    int preferred_column = -1;
    CompositionState composition_state;
    EditDirtyFlags dirty_flags = EditDirtyFlags::None;
    uint64_t revision_id = 0;

    int GetSelectionStart() const;
    int GetSelectionEnd() const;
    bool HasSelection() const;
    bool HasActiveComposition() const;

    void SetText(const std::string& new_text);
    void SetSelection(int anchor, int focus);
    void SetCaretPosition(int position);
    void SetPreferredColumn(int column);
    void MarkDirty(EditDirtyFlags flag);
    void ClearDirtyFlags();
};

std::shared_ptr<TextAreaEditState> CreateTextAreaEditState();

}  // namespace mbink

