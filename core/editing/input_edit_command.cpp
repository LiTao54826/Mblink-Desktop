#include "input_edit_command.h"

#include <utility>

namespace mbink {

InputEditCommand MakeCommand(InputEditCommandType type) {
    InputEditCommand command;
    command.type = type;
    return command;
}

InputEditCommand InputEditCommand::InsertText(std::string text) {
    InputEditCommand command = MakeCommand(InputEditCommandType::InsertText);
    command.payload = InsertTextCommandData{std::move(text)};
    return command;
}

InputEditCommand InputEditCommand::ReplaceSelection(std::string text) {
    InputEditCommand command = MakeCommand(InputEditCommandType::ReplaceSelection);
    command.payload = ReplaceSelectionCommandData{std::move(text)};
    return command;
}

InputEditCommand InputEditCommand::PasteText(std::string text) {
    InputEditCommand command = MakeCommand(InputEditCommandType::PasteText);
    command.payload = InsertTextCommandData{std::move(text)};
    return command;
}

InputEditCommand InputEditCommand::SetSelection(int anchor, int focus) {
    InputEditCommand command = MakeCommand(InputEditCommandType::SetSelection);
    command.payload = SelectionCommandData{anchor, focus};
    return command;
}

InputEditCommand InputEditCommand::SetCaret(int position) {
    InputEditCommand command = MakeCommand(InputEditCommandType::SetCaret);
    command.payload = CaretCommandData{position};
    return command;
}

InputEditCommand InputEditCommand::ExtendSelectionLeft() {
    return MakeCommand(InputEditCommandType::ExtendSelectionLeft);
}

InputEditCommand InputEditCommand::ExtendSelectionRight() {
    return MakeCommand(InputEditCommandType::ExtendSelectionRight);
}

InputEditCommand InputEditCommand::ExtendSelectionToStart() {
    return MakeCommand(InputEditCommandType::ExtendSelectionToStart);
}

InputEditCommand InputEditCommand::ExtendSelectionToEnd() {
    return MakeCommand(InputEditCommandType::ExtendSelectionToEnd);
}

InputEditCommand InputEditCommand::StartComposition(std::string text, int start, int end) {
    InputEditCommand command = MakeCommand(InputEditCommandType::StartComposition);
    command.payload = CompositionCommandData{std::move(text), start, end};
    return command;
}

InputEditCommand InputEditCommand::UpdateComposition(std::string text, int start, int end) {
    InputEditCommand command = MakeCommand(InputEditCommandType::UpdateComposition);
    command.payload = CompositionCommandData{std::move(text), start, end};
    return command;
}

InputEditCommand InputEditCommand::CommitComposition(std::string text, int start, int end) {
    InputEditCommand command = MakeCommand(InputEditCommandType::CommitComposition);
    command.payload = CompositionCommandData{std::move(text), start, end};
    return command;
}

InputEditCommand InputEditCommand::CancelComposition() {
    return MakeCommand(InputEditCommandType::CancelComposition);
}

}  // namespace mbink

