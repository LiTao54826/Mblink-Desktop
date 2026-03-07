#include "input_editing_controller.h"

#include "input_edit_state.h"
#include "core/dom/elements/html_input_element.h"
#include "core/utils/utf8_utils.h"
#include <SDL3/SDL.h>

namespace lightui {

InputEditingController::InputEditingController(HTMLInputElement* owner,
                                               std::shared_ptr<InputEditState> edit_state)
    : owner_(owner), edit_state_(std::move(edit_state)) {
}

bool InputEditingController::ApplyCommand(const InputEditCommand& command) {
    if (!edit_state_) return false;

    switch (command.type) {
        case InputEditCommandType::InsertText:
        case InputEditCommandType::PasteText:
            return ApplyInsertText(std::get<InsertTextCommandData>(command.payload).text);
        case InputEditCommandType::DeleteBackward:
            return ApplyDeleteBackward();
        case InputEditCommandType::DeleteForward:
            return ApplyDeleteForward();
        case InputEditCommandType::MoveCaretLeft:
            return ApplyMoveCaretLeft();
        case InputEditCommandType::MoveCaretRight:
            return ApplyMoveCaretRight();
        case InputEditCommandType::MoveCaretToStart:
            return ApplyMoveCaretToStart();
        case InputEditCommandType::MoveCaretToEnd:
            return ApplyMoveCaretToEnd();
        case InputEditCommandType::ExtendSelectionLeft:
            return ApplyExtendSelectionLeft();
        case InputEditCommandType::ExtendSelectionRight:
            return ApplyExtendSelectionRight();
        case InputEditCommandType::ExtendSelectionToStart:
            return ApplyExtendSelectionToStart();
        case InputEditCommandType::ExtendSelectionToEnd:
            return ApplyExtendSelectionToEnd();
        case InputEditCommandType::SelectAll:
            return ApplySelectAll();
        case InputEditCommandType::SetSelection: {
            const auto data = std::get<SelectionCommandData>(command.payload);
            return ApplySetSelection(data.anchor, data.focus);
        }
        case InputEditCommandType::SetCaret:
            return ApplySetCaret(std::get<CaretCommandData>(command.payload).position);
        case InputEditCommandType::CutSelection:
            return ApplyCutSelection();
        case InputEditCommandType::ReplaceSelection:
            return ReplaceSelectionText(std::get<ReplaceSelectionCommandData>(command.payload).text);
        case InputEditCommandType::StartComposition:
            return ApplyStartComposition(std::get<CompositionCommandData>(command.payload));
        case InputEditCommandType::UpdateComposition:
            return ApplyUpdateComposition(std::get<CompositionCommandData>(command.payload));
        case InputEditCommandType::CommitComposition:
            return ApplyCommitComposition(std::get<CompositionCommandData>(command.payload));
        case InputEditCommandType::CancelComposition:
            return ApplyCancelComposition();
        default:
            return false;
    }
}

EditDirtyFlags InputEditingController::GetDirtyFlags() const { return edit_state_ ? edit_state_->dirty_flags : EditDirtyFlags::None; }
void InputEditingController::ClearDirtyFlags() { if (edit_state_) edit_state_->ClearDirtyFlags(); }

bool InputEditingController::ApplyInsertText(const std::string& text) { return ReplaceSelectionText(text); }

bool InputEditingController::ApplyDeleteBackward() {
    if (!edit_state_) return false;
    int start = edit_state_->GetSelectionStart();
    const std::string value = edit_state_->text;
    if (edit_state_->HasSelection()) return ReplaceSelectionText("");
    if (start <= 0) return false;
    size_t prev_byte = utf8::CharPosToBytePos(value, start - 1);
    size_t curr_byte = utf8::CharPosToBytePos(value, start);
    edit_state_->SetText(value.substr(0, prev_byte) + value.substr(curr_byte));
    edit_state_->SetCaretPosition(start - 1);
    NotifyInputEvent(); RequestRepaint(); return true;
}

bool InputEditingController::ApplyDeleteForward() {
    if (!edit_state_) return false;
    int start = edit_state_->GetSelectionStart();
    const std::string value = edit_state_->text;
    if (edit_state_->HasSelection()) return ReplaceSelectionText("");
    if (start >= static_cast<int>(utf8::CharCount(value))) return false;
    size_t curr_byte = utf8::CharPosToBytePos(value, start);
    size_t next_byte = utf8::CharPosToBytePos(value, start + 1);
    edit_state_->SetText(value.substr(0, curr_byte) + value.substr(next_byte));
    edit_state_->SetCaretPosition(start);
    NotifyInputEvent(); RequestRepaint(); return true;
}

bool InputEditingController::ApplyMoveCaretLeft() {
    if (!edit_state_) return false;
    if (edit_state_->HasSelection()) return ApplySetCaret(edit_state_->GetSelectionStart());
    return ApplySetCaret(edit_state_->GetSelectionStart() - 1);
}

bool InputEditingController::ApplyMoveCaretRight() {
    if (!edit_state_) return false;
    if (edit_state_->HasSelection()) return ApplySetCaret(edit_state_->GetSelectionEnd());
    return ApplySetCaret(edit_state_->GetSelectionEnd() + 1);
}

bool InputEditingController::ApplyMoveCaretToStart() { return ApplySetCaret(0); }
bool InputEditingController::ApplyMoveCaretToEnd() { return ApplySetCaret(static_cast<int>(utf8::CharCount(edit_state_->text))); }
bool InputEditingController::ApplyExtendSelectionLeft() {
    if (!edit_state_) return false;
    return ExtendSelectionTo(edit_state_->selection_focus - 1);
}

bool InputEditingController::ApplyExtendSelectionRight() {
    if (!edit_state_) return false;
    return ExtendSelectionTo(edit_state_->selection_focus + 1);
}

bool InputEditingController::ApplyExtendSelectionToStart() { return ExtendSelectionTo(0); }
bool InputEditingController::ApplyExtendSelectionToEnd() { return ExtendSelectionTo(static_cast<int>(utf8::CharCount(edit_state_->text))); }
bool InputEditingController::ApplySelectAll() { return ApplySetSelection(0, static_cast<int>(utf8::CharCount(edit_state_->text))); }

bool InputEditingController::ApplySetSelection(int anchor, int focus) {
    if (!edit_state_) return false;
    int len = static_cast<int>(utf8::CharCount(edit_state_->text));
    anchor = anchor < 0 ? 0 : (anchor > len ? len : anchor);
    focus = focus < 0 ? 0 : (focus > len ? len : focus);
    edit_state_->SetSelection(anchor, focus); RequestRepaint(); return true;
}

bool InputEditingController::ApplySetCaret(int position) { return ApplySetSelection(position, position); }

bool InputEditingController::ExtendSelectionTo(int focus) {
    if (!edit_state_) return false;
    return ApplySetSelection(edit_state_->selection_anchor, focus);
}

bool InputEditingController::ApplyCutSelection() {
    if (!edit_state_ || !edit_state_->HasSelection()) return false;
    std::string selected = utf8::SubstrByChar(edit_state_->text, edit_state_->GetSelectionStart(), edit_state_->GetSelectionEnd());
    SDL_SetClipboardText(selected.c_str());
    return ReplaceSelectionText("");
}

bool InputEditingController::ApplyStartComposition(const CompositionCommandData& data) {
    if (!edit_state_) return false;
    edit_state_->composition_state.active = true;
    edit_state_->composition_state.text = data.text;
    edit_state_->composition_state.start = data.start;
    edit_state_->composition_state.end = data.end;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool InputEditingController::ApplyUpdateComposition(const CompositionCommandData& data) {
    if (!edit_state_ || !edit_state_->HasActiveComposition()) return false;
    edit_state_->composition_state.text = data.text;
    edit_state_->composition_state.start = data.start;
    edit_state_->composition_state.end = data.end;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool InputEditingController::ApplyCommitComposition(const CompositionCommandData& data) {
    if (!edit_state_) return false;

    if (!data.text.empty()) {
        if (!ReplaceSelectionText(data.text)) {
            return false;
        }
    }

    edit_state_->composition_state.active = false;
    edit_state_->composition_state.text.clear();
    edit_state_->composition_state.start = edit_state_->caret_position;
    edit_state_->composition_state.end = edit_state_->caret_position;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool InputEditingController::ApplyCancelComposition() {
    if (!edit_state_ || !edit_state_->HasActiveComposition()) return false;
    edit_state_->composition_state.active = false;
    edit_state_->composition_state.text.clear();
    edit_state_->composition_state.start = edit_state_->caret_position;
    edit_state_->composition_state.end = edit_state_->caret_position;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool InputEditingController::ReplaceSelectionText(const std::string& text) {
    if (!edit_state_) return false;
    std::string value = edit_state_->text;
    int start = edit_state_->GetSelectionStart();
    int end = edit_state_->GetSelectionEnd();
    size_t start_byte = utf8::CharPosToBytePos(value, start);
    size_t end_byte = utf8::CharPosToBytePos(value, end);
    value = value.substr(0, start_byte) + text + value.substr(end_byte);
    edit_state_->SetText(value);
    edit_state_->SetCaretPosition(start + static_cast<int>(utf8::CharCount(text)));
    NotifyInputEvent(); RequestRepaint(); return true;
}

void InputEditingController::RequestRepaint() {
    if (owner_) {
        owner_->RequestInputRepaint();
    }
}
void InputEditingController::NotifyInputEvent() { if (owner_) owner_->TriggerInputEvent(); }

}  // namespace lightui

