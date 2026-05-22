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
#include "core/editing/editor_input_session.h"
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

namespace mbink {

KeyboardEventDispatcher::KeyboardEventDispatcher() = default;

KeyboardEventDispatcher::~KeyboardEventDispatcher() = default;

void KeyboardEventDispatcher::SetManagers(FocusManager* focus_manager,
                                           ClipboardManager* clipboard_manager,
                                           ContentEditableController* contenteditable_controller,
                                           EditorInputSession* editor_input_session) {
    focus_manager_ = focus_manager;
    clipboard_manager_ = clipboard_manager;
    contenteditable_controller_ = contenteditable_controller;
    editor_input_session_ = editor_input_session;
}

bool KeyboardEventDispatcher::HandleKeyboardEvent(const SDL_Event& event,
                                                   std::shared_ptr<Window> window,
                                                   std::shared_ptr<Document> document) {
    if (!window || !document) {
        return false;
    }

    if (focus_manager_) {
        focus_manager_->SetWindow(window.get());
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
            window->SetNeedsRepaintFor(RepaintReason::DevTools);
            return true;
        }
    }

    // 获取修饰键状态
    SDL_Keymod mod = SDL_GetModState();
    bool ctrl_key = (mod & SDL_KMOD_CTRL) != 0;
    bool shift_key = (mod & SDL_KMOD_SHIFT) != 0;
    bool alt_key = (mod & SDL_KMOD_ALT) != 0;
    bool meta_key = (mod & SDL_KMOD_GUI) != 0;

    // 获取焦点元素
    auto focus_element = focus_manager_ ? focus_manager_->GetFocusElement() : nullptr;

    // 处理不同类型的键盘事件
    if (event.type == SDL_EVENT_KEY_DOWN) {
        const bool is_plain_tab = event.key.key == SDLK_TAB && !ctrl_key && !alt_key && !meta_key;
        if (!focus_element && !is_plain_tab) {
            int key_code = SDLKeycodeToKeyCode(event.key.key);
            if ((ctrl_key || meta_key) && !alt_key && !shift_key && key_code == 67 &&
                clipboard_manager_ &&
                clipboard_manager_->HandleKeyboardShortcut(document, key_code, ctrl_key, meta_key)) {
                return true;
            }
            return false;
        }
        HandleKeyDown(event, focus_element, document, window, ctrl_key, shift_key, alt_key, meta_key);
        return true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        if (!focus_element) {
            return false;
        }
        HandleKeyUp(event, focus_element, ctrl_key, shift_key, alt_key, meta_key);
        return true;
    } else if (event.type == SDL_EVENT_TEXT_INPUT) {
        if (!focus_element) {
            return false;
        }
        HandleTextInput(event, focus_element, document);
        return true;
    } else if (event.type == SDL_EVENT_TEXT_EDITING) {
        if (!focus_element) {
            return false;
        }
        HandleTextEditing(event, focus_element, document);
        return true;
    }

    return false;
}

void KeyboardEventDispatcher::HandleKeyDown(const SDL_Event& event,
                                             std::shared_ptr<Element> focus_element,
                                             std::shared_ptr<Document> document,
                                             std::shared_ptr<Window> window,
                                             bool ctrl_key, bool shift_key,
                                             bool alt_key, bool meta_key) {
    std::string key = SDLKeycodeToKey(event.key.key, shift_key);
    std::string code = SDLScancodeToCode(event.key.scancode);
    int key_code = SDLKeycodeToKeyCode(event.key.key);
    bool repeat = event.key.repeat;

    std::shared_ptr<KeyboardEvent> keydown_event;
    bool default_prevented = false;

    if (focus_element) {
        keydown_event = std::make_shared<KeyboardEvent>(
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
        default_prevented = keydown_event->IsDefaultPrevented();
    }

    if (event.key.key == SDLK_TAB && !ctrl_key && !alt_key && !meta_key) {
        if (!default_prevented && focus_manager_) {
            focus_manager_->TabToNextFocusableElement(document, shift_key);
        }
        return;
    }

    if (!focus_element) {
        return;
    }

    // 处理剪贴板快捷键 (Ctrl+C/X/V) - 先分发事件，再执行默认行为
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
            bool is_contenteditable_target = false;
            if (auto element = std::dynamic_pointer_cast<Element>(focus_element)) {
                is_contenteditable_target = element->IsContentEditable();
            }

            auto clipboard_event = std::make_shared<ClipboardEvent>(clipboard_event_type, "");
            focus_element->DispatchEvent(clipboard_event);
            if (clipboard_event->IsDefaultPrevented() || default_prevented) {
                return;
            }

            bool can_run_default_clipboard_action = is_contenteditable_target || key_code == 67;
            if (can_run_default_clipboard_action &&
                clipboard_manager_ &&
                clipboard_manager_->HandleKeyboardShortcut(document, key_code, ctrl_key, meta_key)) {
                if (focus_manager_) {
                    focus_manager_->UpdateTextInputArea();
                }
                return;
            }
        }
    }

    if (editor_input_session_ && editor_input_session_->HandleKeyDown(event, focus_element, document, window, clipboard_manager_, contenteditable_controller_, ctrl_key, shift_key, alt_key, meta_key, default_prevented)) {
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
        return;
    }

    // 如果事件未被阻止，处理表单元素的键盘输入
    if (!default_prevented) {
        // 检查是否是表单元素
        auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(focus_element);
        if (terminal_element) {
            // Terminal 元素：将按键转换为终端序列并发送
            int modifiers = (ctrl_key ? 1 : 0) | (shift_key ? 2 : 0) | (alt_key ? 4 : 0);
            terminal_element->HandleKeyInput(key, modifiers);
        } else {
            // 检查是否是 logview 元素
            auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(focus_element);
            if (logview_element) {
                logview_element->OnKeyDown(key, ctrl_key, shift_key);
                // 触发重绘
                window->SetNeedsRepaintFor(RepaintReason::KeyboardInput);
                if (auto pipeline = window->GetRenderPipeline()) {
                    pipeline->ForceRasterize();
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

    auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(focus_element);
    if (editor_input_session_ && editor_input_session_->HandleTextInput(event, focus_element, document, contenteditable_controller_)) {
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
    } else if (terminal_element) {
        terminal_element->SendInput(text);
    }
}

void KeyboardEventDispatcher::HandleTextEditing(const SDL_Event& event,
                                                std::shared_ptr<Element> focus_element,
                                                std::shared_ptr<Document> document) {
    if (editor_input_session_ && editor_input_session_->HandleTextEditing(event, focus_element, document, contenteditable_controller_)) {
        if (focus_manager_) {
            focus_manager_->UpdateTextInputArea();
        }
        return;
    }
}

std::shared_ptr<Element> KeyboardEventDispatcher::GetFocusElement() const {
    if (focus_manager_) {
        return focus_manager_->GetFocusElement();
    }
    return nullptr;
}

} // namespace mbink
