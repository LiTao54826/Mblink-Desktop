#pragma once

#include <memory>

#include "input_edit_command.h"

namespace mblink {

class HTMLInputElement;
struct InputEditState;
enum class EditDirtyFlags : unsigned int;

class InputEditingController {
public:
    InputEditingController(HTMLInputElement* owner,
                           std::shared_ptr<InputEditState> edit_state);
    ~InputEditingController() = default;

    bool ApplyCommand(const InputEditCommand& command);
    EditDirtyFlags GetDirtyFlags() const;
    void ClearDirtyFlags();

private:
    bool ApplyInsertText(const std::string& text);
    bool ApplyDeleteBackward();
    bool ApplyDeleteForward();
    bool ApplyMoveCaretLeft();
    bool ApplyMoveCaretRight();
    bool ApplyMoveCaretToStart();
    bool ApplyMoveCaretToEnd();
    bool ApplyExtendSelectionLeft();
    bool ApplyExtendSelectionRight();
    bool ApplyExtendSelectionToStart();
    bool ApplyExtendSelectionToEnd();
    bool ApplySelectAll();
    bool ApplySetSelection(int anchor, int focus);
    bool ApplySetCaret(int position);
    bool ApplyCutSelection();
    bool ApplyStartComposition(const CompositionCommandData& data);
    bool ApplyUpdateComposition(const CompositionCommandData& data);
    bool ApplyCommitComposition(const CompositionCommandData& data);
    bool ApplyCancelComposition();

    bool ExtendSelectionTo(int focus);
    bool ReplaceSelectionText(const std::string& text);
    void RequestRepaint();
    void NotifyInputEvent();

private:
    HTMLInputElement* owner_ = nullptr;
    std::shared_ptr<InputEditState> edit_state_;
};

}  // namespace mblink

