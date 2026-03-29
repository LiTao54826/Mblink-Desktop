#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace mbink {

struct InputEditState;

enum class EditDirtyFlags : uint32_t {
    None = 0,
    TextChanged = 1 << 0,
    SelectionChanged = 1 << 1,
    CaretChanged = 1 << 2,
    CompositionChanged = 1 << 3,
    StyleChanged = 1 << 4,
    GeometryChanged = 1 << 5,
};

EditDirtyFlags operator|(EditDirtyFlags lhs, EditDirtyFlags rhs);
EditDirtyFlags operator&(EditDirtyFlags lhs, EditDirtyFlags rhs);
EditDirtyFlags& operator|=(EditDirtyFlags& lhs, EditDirtyFlags rhs);
bool HasDirtyFlag(EditDirtyFlags flags, EditDirtyFlags flag);

struct CompositionState {
    bool active = false;
    int start = 0;
    int end = 0;
    std::string text;
};

struct InputEditState {
    std::string text;
    int selection_anchor = 0;
    int selection_focus = 0;
    int caret_position = 0;
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
    void MarkDirty(EditDirtyFlags flag);
    void ClearDirtyFlags();
};

std::shared_ptr<InputEditState> CreateInputEditState();

}  // namespace mbink

