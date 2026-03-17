/**
 * @file keyboard_event_dispatcher.cpp
 * @brief 键盘事件分发器实现
 *
 * 从 event_loop.cpp 提取的键盘事件处理逻辑。
 * 负责处理键盘按下、抬起、文本输入等事件。
 */

#include "keyboard_event_dispatcher.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/event.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/editing/clipboard_manager.h"
#include "core/editing/contenteditable_controller.h"
#include "core/editing/contenteditable_handler.h"
#include "core/editing/input_edit_command.h"
#include "core/editing/input_edit_state.h"
#include "core/editing/textarea_editing_controller.h"
#include "core/event/input/focus_manager.h"
#include "core/event/input/keyboard_utils.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/devtools/devtools_manager.h"

#include <iostream>

namespace lightui {

KeyboardEventDispatcher::KeyboardEventDispatcher() = default;

KeyboardEventDispatcher::~KeyboardEventDispatcher() = default;

void KeyboardEventDispatcher::SetManagers(FocusManager* focus_manager,
                                           ContentEditableHandler* contenteditable_handler,
                                           ContentEditableController* contenteditable_controller,
                                           ClipboardManager* clipboard_manager) {
    focus_manager_ = focus_manager;
    contenteditable_handler_ = contenteditable_handler;
    contenteditable_controller_ = contenteditable_controller;
    clipboard_manager_ = clipboard_manager;
}

bool KeyboardEventDispatcher::HandleKeyboardEvent(const SDL_Event& event,
                                                   std::shared_ptr<Window> window,
                                                   std::shared_ptr<Document> document) {
    if (!window || !document) {
        return false;
    }

    // 首先检查 DevTools 快捷键
    if (event.type == SDL_EVENT_KEY_DOWN) {
        SDL_Keymod mod = SDL_GetModState();
        bool ctrl_key = (mod & SDL_KMOD_CTRL) != 0;
        bool shift_key = (mod & SDL_KMOD_SHIFT) != 0;
        bool alt_key = (mod & SDL_KMOD_ALT) != 0;
        
        // 将 SDL 键码转换为 DOM keyCode
        int key_code = SDLKeycodeToKeyCode(event.key.key);
        
        auto& devtools = DevToolsManager::GetInstance();
        if (devtools.HandleKeyboardShortcut(key_code, ctrl_key, shift_key, alt_key)) {
            // DevTools 消费了这个快捷键
            window->SetNeedsRepaint();
            return true;
        }
    }

    // 获取焦点元素
    auto focus_element = focus_manager_ ? focus_manager_->GetFocusElement() : nullptr;
    if (!focus_element) {
        // 没有焦点元素，不分发键盘事件
        return false;
    }
    

    // 获取修饰键状态
    SDL_Keymod mod = SDL_GetModState();
    bool ctrl_key = (mod & SDL_KMOD_CTRL) != 0;
    bool shift_key = (mod & SDL_KMOD_SHIFT) != 0;
    bool alt_key = (mod & SDL_KMOD_ALT) != 0;
    bool meta_key = (mod & SDL_KMOD_GUI) != 0;

    // 处理不同类型的键盘事件
    if (event.type == SDL_EVENT_KEY_DOWN) {
        HandleKeyDown(event, focus_element, document, window, ctrl_key, shift_key, alt_key, meta_key);
        return true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        HandleKeyUp(event, focus_element, ctrl_key, shift_key, alt_key, meta_key);
        return true;
    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
        HandleTextInput(event, focus_element, document);
        return true;
    } else if (event.type == SDL_EVENT_TEXT_EDITING) {
        HandleTextEditing(event, focus_element, document);
        return true;
    }

    return false;
}

void KeyboardEventDispatcher::HandleKeyDown(const SDL_Event& event,
                                             std::shared_ptr<Element> focus_element,
                                             std::shared_ptr<Document> /*document*/,
                                             std::shared_ptr<Window> window,
                                             bool ctrl_key, bool shift_key,
                                             bool alt_key, bool meta_key) {
    std::string key = SDLKeycodeToKey(event.key.key, shift_key);
    std::string code = SDLScancodeToCode(event.key.scancode);
    int key_code = SDLKeycodeToKeyCode(event.key.key);
    bool repeat = event.key.repeat;

    auto keydown_event = std::make_shared<KeyboardEvent>(
        "keydown",
        key,
        code,
        key_code,
        ctrl_key,
        shift_key,
        alt_key,
        meta_key,
        repeat
    );

    focus_element->DispatchEvent(keydown_event);

    // 处理剪贴板快捷键 (Ctrl+C/X/V) - 分发 copy/cut/paste 事件
    // 浏览器会在 Ctrl+C/X/V 时自动触发 copy/cut/paste 事件
    if (ctrl_key && !alt_key && !shift_key) {
        std::string clipboard_event_type;
        if (key_code == 67) {  // 'C' - Copy
            clipboard_event_type = "copy";
        } else if (key_code == 88) {  // 'X' - Cut
            clipboard_event_type = "cut";
        } else if (key_code == 86) {  // 'V' - Paste
            clipboard_event_type = "paste";
        }

        if (!clipboard_event_type.empty()) {
            // 创建并分发剪贴板事件；只有事件显式阻止默认行为时才停止后续默认编辑命令
            auto clipboard_event = std::make_shared<ClipboardEvent>(clipboard_event_type, "");
            focus_element->DispatchEvent(clipboard_event);
            if (clipboard_event->IsDefaultPrevented() || keydown_event->IsDefaultPrevented()) {
                return;
            }
        }
    }

    // 如果事件未被阻止，处理表单元素的键盘输入
    if (!keydown_event->IsDefaultPrevented()) {
        // 检查是否是表单元素
        auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focus_element);
        auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element);
        auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(focus_element);

        if (input_element) {
            bool handled = false;
            if (shift_key && key == "ArrowLeft") {
                handled = input_element->ExecuteEditCommand(InputEditCommand::ExtendSelectionLeft());
            } else if (shift_key && key == "ArrowRight") {
                handled = input_element->ExecuteEditCommand(InputEditCommand::ExtendSelectionRight());
            } else if (shift_key && key == "Home") {
                handled = input_element->ExecuteEditCommand(InputEditCommand::ExtendSelectionToStart());
            } else if (shift_key && key == "End") {
                handled = input_element->ExecuteEditCommand(InputEditCommand::ExtendSelectionToEnd());
            } else if (key == "Backspace") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::DeleteBackward));
            } else if (key == "Delete") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::DeleteForward));
            } else if (key == "ArrowLeft") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretLeft));
            } else if (key == "ArrowRight") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretRight));
            } else if (key == "Home") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretToStart));
            } else if (key == "End") {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretToEnd));
            } else if (ctrl_key && (key == "a" || key == "A")) {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::SelectAll));
            } else if (ctrl_key && (key == "x" || key == "X")) {
                handled = input_element->ExecuteEditCommand(MakeCommand(InputEditCommandType::CutSelection));
            } else if (ctrl_key && (key == "v" || key == "V")) {
                char* clipboard_text = SDL_GetClipboardText();
                if (clipboard_text && clipboard_text[0] != '\0') {
                    handled = input_element->ExecuteEditCommand(InputEditCommand::PasteText(clipboard_text));
                }
                SDL_free(clipboard_text);
            }

            if (!handled) {
                input_element->HandleKeyPress(key, ctrl_key);
            }
            if (focus_manager_) {
                focus_manager_->UpdateTextInputArea();
            }
        } else if (textarea_element) {
            textarea_element->HandleKeyPress(key, ctrl_key, shift_key);
            if (focus_manager_) {
                focus_manager_->UpdateTextInputArea();
            }
        } else if (terminal_element) {
            // Terminal 元素：将按键转换为终端序列并发送
            int modifiers = (ctrl_key ? 1 : 0) | (shift_key ? 2 : 0) | (alt_key ? 4 : 0);
            terminal_element->HandleKeyInput(key, modifiers);
        } else {
            // 检查是否是 logview 元素
            auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(focus_element);
            if (logview_element) {
                logview_element->OnKeyDown(key, ctrl_key, shift_key);
                // 触发重绘
                window->SetNeedsRepaint();
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
                }
            } else {
                // 检查是否是 contentEditable 元素或其子元素
                auto element = std::dynamic_pointer_cast<Element>(focus_element);
                if (element && element->IsContentEditable()) {
                    if (contenteditable_controller_) {
                        contenteditable_controller_->HandleKeyDown(element, key_code, ctrl_key, shift_key, alt_key);
                    }
                }
            }
        }
    }
}

void KeyboardEventDispatcher::HandleKeyUp(const SDL_Event& event,
                                           std::shared_ptr<Element> focus_element,
                                           bool ctrl_key, bool shift_key,
                                           bool alt_key, bool meta_key) {
    std::string key = SDLKeycodeToKey(event.key.key, shift_key);
    std::string code = SDLScancodeToCode(event.key.scancode);
    int key_code = SDLKeycodeToKeyCode(event.key.key);

    auto keyup_event = std::make_shared<KeyboardEvent>(
        "keyup",
        key,
        code,
        key_code,
        ctrl_key,
        shift_key,
        alt_key,
        meta_key,
        false
    );

    focus_element->DispatchEvent(keyup_event);
}

void KeyboardEventDispatcher::HandleTextInput(const SDL_Event& event,
                                               std::shared_ptr<Element> focus_element,
                                               std::shared_ptr<Document> document) {
    std::string text = event.text.text;

    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focus_element);
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element);
    auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(focus_element);
    CompositionCommandData composition_data;

    if (input_element) {
        auto edit_state = input_element->GetEditState();
        if (edit_state && edit_state->HasActiveComposition()) {
            input_element->ExecuteEditCommand(
                InputEditCommand::CommitComposition(text,
                                                    edit_state->composition_state.start,
                                                    edit_state->composition_state.end));
        } else {
            input_element->ExecuteEditCommand(InputEditCommand::InsertText(text));
        }
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
    } else if (textarea_element) {
        auto edit_state = textarea_element->GetEditState();
        if (edit_state && edit_state->HasActiveComposition()) {
            composition_data.text = text;
            composition_data.start = edit_state->composition_state.start;
            composition_data.end = edit_state->composition_state.end;
            TextAreaEditingController controller(textarea_element.get(), edit_state);
            controller.CommitComposition(composition_data);
        } else {
            textarea_element->HandleTextInput(text);
        }
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
    } else if (terminal_element) {
        terminal_element->SendInput(text);
    } else {
        auto element = std::dynamic_pointer_cast<Element>(focus_element);
        if (element && element->IsContentEditable()) {
            if (contenteditable_handler_ && document) {
                if (contenteditable_handler_->HasActiveComposition(document)) {
                    contenteditable_handler_->CommitComposition(document, text);
                } else {
                    contenteditable_handler_->InsertText(document, text);
                }
                if (focus_manager_) {
                    focus_manager_->UpdateTextInputArea();
                }
            }
        }
    }
}

void KeyboardEventDispatcher::HandleTextEditing(const SDL_Event& event,
                                                std::shared_ptr<Element> focus_element,
                                                std::shared_ptr<Document> document) {
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(focus_element);
    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element);
    auto contenteditable_element = std::dynamic_pointer_cast<Element>(focus_element);

    std::string text = event.edit.text ? event.edit.text : "";

    if (input_element) {
        auto edit_state = input_element->GetEditState();
        if (!edit_state) {
            return;
        }

        if (text.empty()) {
            if (edit_state->HasActiveComposition()) {
                input_element->ExecuteEditCommand(InputEditCommand::CancelComposition());
            }
            if (focus_manager_) {
                focus_manager_->UpdateTextInputArea();
            }
            return;
        }

        int start = edit_state->GetSelectionStart();
        int end = edit_state->GetSelectionEnd();
        if (edit_state->HasActiveComposition()) {
            start = edit_state->composition_state.start;
            end = edit_state->composition_state.end;
            input_element->ExecuteEditCommand(InputEditCommand::UpdateComposition(text, start, end));
        } else {
            input_element->ExecuteEditCommand(InputEditCommand::StartComposition(text, start, end));
        }
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
        return;
    }

    if (textarea_element) {
        auto edit_state = textarea_element->GetEditState();
        if (!edit_state) {
            return;
        }

        TextAreaEditingController controller(textarea_element.get(), edit_state);
        if (text.empty()) {
            controller.CancelComposition();
            if (focus_manager_) {
                focus_manager_->UpdateTextInputArea();
            }
            return;
        }

        CompositionCommandData composition_data;
        composition_data.text = text;
        composition_data.start = edit_state->HasActiveComposition()
            ? edit_state->composition_state.start
            : edit_state->GetSelectionStart();
        composition_data.end = edit_state->HasActiveComposition()
            ? edit_state->composition_state.end
            : edit_state->GetSelectionEnd();

        if (edit_state->HasActiveComposition()) {
            controller.UpdateComposition(composition_data);
        } else {
            controller.StartComposition(composition_data);
        }
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
        return;
    }

    if (!contenteditable_element || !contenteditable_element->IsContentEditable() || !contenteditable_handler_ || !document) {
        return;
    }

    auto selection = document->GetSelection();
    int start = selection ? selection->GetFocusOffset() : 0;
    int end = start;
    if (contenteditable_handler_->HasActiveComposition(document)) {
        const auto composition = contenteditable_handler_->GetCompositionState(document);
        start = composition.start;
        end = composition.end;
    }

    if (text.empty()) {
        contenteditable_handler_->CancelComposition(document);
    } else if (contenteditable_handler_->HasActiveComposition(document)) {
        contenteditable_handler_->UpdateComposition(document, text, start, end);
    } else {
        contenteditable_handler_->StartComposition(document, text, start, end);
    }

    if (focus_manager_) {
        focus_manager_->UpdateTextInputArea();
    }
}

std::shared_ptr<Element> KeyboardEventDispatcher::GetFocusElement() const {
    if (focus_manager_) {
        return focus_manager_->GetFocusElement();
    }
    return nullptr;
}

} // namespace lightui
