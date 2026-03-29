#pragma once

#include <memory>
#include <string>

#include "textarea_edit_state.h"

namespace mbink {

class HTMLTextAreaElement;
struct CompositionCommandData;

class TextAreaEditingController {
public:
    TextAreaEditingController(HTMLTextAreaElement* owner,
                              std::shared_ptr<TextAreaEditState> edit_state);
    ~TextAreaEditingController() = default;

    bool ReplaceSelectionText(const std::string& text);
    bool DeleteBackward();
    bool DeleteForward();
    bool SetSelection(int anchor, int focus);
    bool SetCaret(int position);
    bool StartComposition(const CompositionCommandData& data);
    bool UpdateComposition(const CompositionCommandData& data);
    bool CommitComposition(const CompositionCommandData& data);
    bool CancelComposition();

private:
    bool NormalizeAndReplaceSelection(const std::string& text);
    void NotifyInputEvent();
    void RequestRepaint();
    void MarkScrollToCursor();

    HTMLTextAreaElement* owner_ = nullptr;
    std::shared_ptr<TextAreaEditState> edit_state_;
};

}  // namespace mbink

