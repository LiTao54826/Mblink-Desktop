#pragma once

#include <string>
#include <variant>

namespace mblink {

enum class InputEditCommandType {
    InsertText,
    DeleteBackward,
    DeleteForward,
    ReplaceSelection,
    MoveCaretLeft,
    MoveCaretRight,
    MoveCaretToStart,
    MoveCaretToEnd,
    ExtendSelectionLeft,
    ExtendSelectionRight,
    ExtendSelectionToStart,
    ExtendSelectionToEnd,
    SelectAll,
    SetSelection,
    SetCaret,
    PasteText,
    CutSelection,
    StartComposition,
    UpdateComposition,
    CommitComposition,
    CancelComposition,
};

struct InsertTextCommandData {
    std::string text;
};

struct ReplaceSelectionCommandData {
    std::string text;
};

struct SelectionCommandData {
    int anchor = 0;
    int focus = 0;
};

struct CaretCommandData {
    int position = 0;
};

struct CompositionCommandData {
    std::string text;
    int start = 0;
    int end = 0;
};

using InputEditCommandPayload = std::variant<std::monostate,
                                             InsertTextCommandData,
                                             ReplaceSelectionCommandData,
                                             SelectionCommandData,
                                             CaretCommandData,
                                             CompositionCommandData>;

struct InputEditCommand {
    InputEditCommandType type = InputEditCommandType::InsertText;
    InputEditCommandPayload payload;

    static InputEditCommand InsertText(std::string text);
    static InputEditCommand ReplaceSelection(std::string text);
    static InputEditCommand PasteText(std::string text);
    static InputEditCommand SetSelection(int anchor, int focus);
    static InputEditCommand SetCaret(int position);
    static InputEditCommand ExtendSelectionLeft();
    static InputEditCommand ExtendSelectionRight();
    static InputEditCommand ExtendSelectionToStart();
    static InputEditCommand ExtendSelectionToEnd();
    static InputEditCommand StartComposition(std::string text, int start, int end);
    static InputEditCommand UpdateComposition(std::string text, int start, int end);
    static InputEditCommand CommitComposition(std::string text, int start, int end);
    static InputEditCommand CancelComposition();
};

InputEditCommand MakeCommand(InputEditCommandType type);

}  // namespace mblink
