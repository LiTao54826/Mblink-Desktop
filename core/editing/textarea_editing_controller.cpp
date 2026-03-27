#include "textarea_editing_controller.h"

#include "input_edit_command.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/utils/utf8_utils.h"

namespace lightui {

TextAreaEditingController::TextAreaEditingController(
    HTMLTextAreaElement* owner,
    std::shared_ptr<TextAreaEditState> edit_state)
    : owner_(owner), edit_state_(std::move(edit_state)) {
}

bool TextAreaEditingController::ReplaceSelectionText(const std::string& text) {
    return NormalizeAndReplaceSelection(text);
}

bool TextAreaEditingController::DeleteBackward() {
    if (!edit_state_) return false;
    if (edit_state_->HasSelection()) return NormalizeAndReplaceSelection("");
    int start = edit_state_->GetSelectionStart();
    if (start <= 0) return false;

    std::string value = edit_state_->text;
    size_t prev_byte = utf8::CharPosToBytePos(value, start - 1);
    size_t curr_byte = utf8::CharPosToBytePos(value, start);
    edit_state_->SetText(value.substr(0, prev_byte) + value.substr(curr_byte));
    edit_state_->SetCaretPosition(start - 1);
    MarkScrollToCursor();
    NotifyInputEvent();
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::DeleteForward() {
    if (!edit_state_) return false;
    if (edit_state_->HasSelection()) return NormalizeAndReplaceSelection("");
    int start = edit_state_->GetSelectionStart();
    if (start >= static_cast<int>(utf8::CharCount(edit_state_->text))) return false;

    std::string value = edit_state_->text;
    size_t curr_byte = utf8::CharPosToBytePos(value, start);
    size_t next_byte = utf8::CharPosToBytePos(value, start + 1);
    edit_state_->SetText(value.substr(0, curr_byte) + value.substr(next_byte));
    edit_state_->SetCaretPosition(start);
    MarkScrollToCursor();
    NotifyInputEvent();
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::SetSelection(int anchor, int focus) {
    if (!edit_state_) return false;
    int len = static_cast<int>(utf8::CharCount(edit_state_->text));
    anchor = std::clamp(anchor, 0, len);
    focus = std::clamp(focus, 0, len);
    edit_state_->SetSelection(anchor, focus);
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::SetCaret(int position) {
    return SetSelection(position, position);
}

bool TextAreaEditingController::StartComposition(const CompositionCommandData& data) {
    if (!edit_state_) return false;
    edit_state_->composition_state.active = true;
    edit_state_->composition_state.text = data.text;
    edit_state_->composition_state.start = data.start;
    edit_state_->composition_state.end = data.end;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::UpdateComposition(const CompositionCommandData& data) {
    if (!edit_state_ || !edit_state_->HasActiveComposition()) return false;
    edit_state_->composition_state.text = data.text;
    edit_state_->composition_state.start = data.start;
    edit_state_->composition_state.end = data.end;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::CommitComposition(const CompositionCommandData& data) {
    if (!edit_state_) return false;
    if (!data.text.empty() && !NormalizeAndReplaceSelection(data.text)) {
        return false;
    }

    edit_state_->composition_state.active = false;
    edit_state_->composition_state.text.clear();
    edit_state_->composition_state.start = edit_state_->caret_position;
    edit_state_->composition_state.end = edit_state_->caret_position;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::CancelComposition() {
    if (!edit_state_ || !edit_state_->HasActiveComposition()) return false;
    edit_state_->composition_state.active = false;
    edit_state_->composition_state.text.clear();
    edit_state_->composition_state.start = edit_state_->caret_position;
    edit_state_->composition_state.end = edit_state_->caret_position;
    edit_state_->MarkDirty(EditDirtyFlags::CompositionChanged);
    RequestRepaint();
    return true;
}

bool TextAreaEditingController::NormalizeAndReplaceSelection(const std::string& text) {
    if (!edit_state_) return false;

    std::string normalized = text;
    size_t pos = 0;
    while ((pos = normalized.find("\r\n", pos)) != std::string::npos) {
        normalized.replace(pos, 2, "\n");
        pos += 1;
    }
    pos = 0;
    while ((pos = normalized.find('\r', pos)) != std::string::npos) {
        normalized.erase(pos, 1);
    }

    std::string value = edit_state_->text;
    int start = edit_state_->GetSelectionStart();
    int end = edit_state_->GetSelectionEnd();
    size_t start_byte = utf8::CharPosToBytePos(value, start);
    size_t end_byte = utf8::CharPosToBytePos(value, end);
    value = value.substr(0, start_byte) + normalized + value.substr(end_byte);
    edit_state_->SetText(value);
    edit_state_->SetCaretPosition(start + static_cast<int>(utf8::CharCount(normalized)));
    MarkScrollToCursor();
    NotifyInputEvent();
    RequestRepaint();
    return true;
}

void TextAreaEditingController::NotifyInputEvent() {
    if (owner_) owner_->TriggerInputEvent();
}

void TextAreaEditingController::RequestRepaint() {
    if (owner_) owner_->RequestTextAreaRepaint();
}

void TextAreaEditingController::MarkScrollToCursor() {
    if (owner_) owner_->MarkScrollToCursor();
}

}  // namespace lightui

