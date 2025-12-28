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
#include "core/editing/clipboard_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/event/focus_manager.h"
#include "core/event/keyboard_event.h"
#include "core/window/window.h"

namespace lightui {

KeyboardEventDispatcher::KeyboardEventDispatcher() = default;

KeyboardEventDispatcher::~KeyboardEventDispatcher() = default;

void KeyboardEventDispatcher::SetManagers(FocusManager* focus_manager,
                                           ContentEditableHandler* contenteditable_handler,
                                           ClipboardManager* clipboard_manager) {
    focus_manager_ = focus_manager;
    contenteditable_handler_ = contenteditable_handler;
    clipboard_manager_ = clipboard_manager;
}

bool KeyboardEventDispatcher::HandleKeyboardEvent(const SDL_Event& event,
                                                   std::shared_ptr<Window> window,
                                                   std::shared_ptr<Document> document) {
    // 当前为占位实现
    // 完整的事件处理逻辑仍在 EventLoop::HandleKeyboardEventForDOM 中
    // 未来可以逐步迁移到这里
    (void)event;
    (void)window;
    (void)document;
    return false;
}

bool KeyboardEventDispatcher::HandleTextInputEvent(const SDL_Event& event,
                                                    std::shared_ptr<Window> window,
                                                    std::shared_ptr<Document> document) {
    // 当前为占位实现
    (void)event;
    (void)window;
    (void)document;
    return false;
}

std::shared_ptr<Element> KeyboardEventDispatcher::GetFocusElement() const {
    if (focus_manager_) {
        return focus_manager_->GetFocusElement();
    }
    return nullptr;
}

bool KeyboardEventDispatcher::HandleShortcut(SDL_Keycode keycode,
                                              bool ctrl,
                                              bool shift,
                                              bool alt,
                                              std::shared_ptr<Document> document) {
    if (!ctrl || !document) {
        return false;
    }

    // 处理剪贴板快捷键
    if (clipboard_manager_) {
        // Ctrl+C: 复制
        if (keycode == SDLK_C && !shift && !alt) {
            clipboard_manager_->Copy(document);
            return true;
        }
        // Ctrl+V: 粘贴
        if (keycode == SDLK_V && !shift && !alt) {
            clipboard_manager_->Paste(document);
            return true;
        }
        // Ctrl+X: 剪切
        if (keycode == SDLK_X && !shift && !alt) {
            clipboard_manager_->Cut(document);
            return true;
        }
    }

    // Ctrl+A: 全选（未实现）
    // Ctrl+Z: 撤销（未实现）
    // Ctrl+Y: 重做（未实现）

    return false;
}

std::string KeyboardEventDispatcher::SDLKeycodeToDOMKey(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_RETURN:    return "Enter";
        case SDLK_ESCAPE:    return "Escape";
        case SDLK_BACKSPACE: return "Backspace";
        case SDLK_TAB:       return "Tab";
        case SDLK_SPACE:     return " ";
        case SDLK_DELETE:    return "Delete";
        case SDLK_UP:        return "ArrowUp";
        case SDLK_DOWN:      return "ArrowDown";
        case SDLK_LEFT:      return "ArrowLeft";
        case SDLK_RIGHT:     return "ArrowRight";
        case SDLK_HOME:      return "Home";
        case SDLK_END:       return "End";
        case SDLK_PAGEUP:    return "PageUp";
        case SDLK_PAGEDOWN:  return "PageDown";
        case SDLK_INSERT:    return "Insert";
        case SDLK_F1:        return "F1";
        case SDLK_F2:        return "F2";
        case SDLK_F3:        return "F3";
        case SDLK_F4:        return "F4";
        case SDLK_F5:        return "F5";
        case SDLK_F6:        return "F6";
        case SDLK_F7:        return "F7";
        case SDLK_F8:        return "F8";
        case SDLK_F9:        return "F9";
        case SDLK_F10:       return "F10";
        case SDLK_F11:       return "F11";
        case SDLK_F12:       return "F12";
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:    return "Shift";
        case SDLK_LCTRL:
        case SDLK_RCTRL:     return "Control";
        case SDLK_LALT:
        case SDLK_RALT:      return "Alt";
        default:
            if (keycode >= 32 && keycode < 127) {
                return std::string(1, static_cast<char>(keycode));
            }
            return "Unidentified";
    }
}

std::string KeyboardEventDispatcher::SDLScancodeToDOMCode(SDL_Scancode scancode) {
    switch (scancode) {
        case SDL_SCANCODE_A: return "KeyA";
        case SDL_SCANCODE_B: return "KeyB";
        case SDL_SCANCODE_C: return "KeyC";
        case SDL_SCANCODE_D: return "KeyD";
        case SDL_SCANCODE_E: return "KeyE";
        case SDL_SCANCODE_F: return "KeyF";
        case SDL_SCANCODE_G: return "KeyG";
        case SDL_SCANCODE_H: return "KeyH";
        case SDL_SCANCODE_I: return "KeyI";
        case SDL_SCANCODE_J: return "KeyJ";
        case SDL_SCANCODE_K: return "KeyK";
        case SDL_SCANCODE_L: return "KeyL";
        case SDL_SCANCODE_M: return "KeyM";
        case SDL_SCANCODE_N: return "KeyN";
        case SDL_SCANCODE_O: return "KeyO";
        case SDL_SCANCODE_P: return "KeyP";
        case SDL_SCANCODE_Q: return "KeyQ";
        case SDL_SCANCODE_R: return "KeyR";
        case SDL_SCANCODE_S: return "KeyS";
        case SDL_SCANCODE_T: return "KeyT";
        case SDL_SCANCODE_U: return "KeyU";
        case SDL_SCANCODE_V: return "KeyV";
        case SDL_SCANCODE_W: return "KeyW";
        case SDL_SCANCODE_X: return "KeyX";
        case SDL_SCANCODE_Y: return "KeyY";
        case SDL_SCANCODE_Z: return "KeyZ";
        case SDL_SCANCODE_1: return "Digit1";
        case SDL_SCANCODE_2: return "Digit2";
        case SDL_SCANCODE_3: return "Digit3";
        case SDL_SCANCODE_4: return "Digit4";
        case SDL_SCANCODE_5: return "Digit5";
        case SDL_SCANCODE_6: return "Digit6";
        case SDL_SCANCODE_7: return "Digit7";
        case SDL_SCANCODE_8: return "Digit8";
        case SDL_SCANCODE_9: return "Digit9";
        case SDL_SCANCODE_0: return "Digit0";
        case SDL_SCANCODE_RETURN: return "Enter";
        case SDL_SCANCODE_ESCAPE: return "Escape";
        case SDL_SCANCODE_BACKSPACE: return "Backspace";
        case SDL_SCANCODE_TAB: return "Tab";
        case SDL_SCANCODE_SPACE: return "Space";
        default:
            return "Unidentified";
    }
}

} // namespace lightui
