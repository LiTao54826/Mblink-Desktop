/**
 * @file input_handler.cpp
 * @brief 输入事件处理器实现
 */

#include "input_handler.h"

namespace lightui {

InputHandler::InputHandler()
    : mouse_x_(0)
    , mouse_y_(0)
    , mouse_button_state_(0)
    , keyboard_state_(SDL_GetKeyboardState(nullptr))
{
}

bool InputHandler::HandleSDLEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
            return HandleMouseEvent(event);
            
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_TEXT_EDITING:
            return HandleKeyboardEvent(event);

        default:
            return false;
    }
}

void InputHandler::SetMouseCallback(std::function<void(const InputMouseEvent&)> callback) {
    mouse_callback_ = callback;
}

void InputHandler::SetKeyboardCallback(std::function<void(const KeyEvent&)> callback) {
    keyboard_callback_ = callback;
}

void InputHandler::GetMousePosition(int* x, int* y) const {
    if (x) *x = mouse_x_;
    if (y) *y = mouse_y_;
}

bool InputHandler::IsMouseButtonDown(MouseButton button) const {
    return (mouse_button_state_ & SDL_BUTTON_MASK(static_cast<int>(button))) != 0;
}

bool InputHandler::IsKeyDown(SDL_Keycode key) const {
    SDL_Scancode scancode = SDL_GetScancodeFromKey(key, nullptr);
    return IsScancodeDown(scancode);
}

bool InputHandler::IsScancodeDown(SDL_Scancode scancode) const {
    if (!keyboard_state_) {
        return false;
    }
    return keyboard_state_[scancode] != 0;
}

bool InputHandler::HandleMouseEvent(const SDL_Event& event) {
    InputMouseEvent mouse_event;
    mouse_event.window_id = event.window.windowID;
    
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
            mouse_event.type = MouseEventType::MOVE;
            mouse_event.x = static_cast<int>(event.motion.x);
            mouse_event.y = static_cast<int>(event.motion.y);
            mouse_event.button = 0;
            mouse_event.wheel_x = 0;
            mouse_event.wheel_y = 0;
            
            mouse_x_ = mouse_event.x;
            mouse_y_ = mouse_event.y;
            mouse_button_state_ = event.motion.state;
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mouse_event.type = MouseEventType::DOWN;
            mouse_event.x = static_cast<int>(event.button.x);
            mouse_event.y = static_cast<int>(event.button.y);
            mouse_event.button = event.button.button;
            mouse_event.wheel_x = 0;
            mouse_event.wheel_y = 0;
            
            mouse_x_ = mouse_event.x;
            mouse_y_ = mouse_event.y;
            mouse_button_state_ |= SDL_BUTTON_MASK(event.button.button);
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_UP:
            mouse_event.type = MouseEventType::UP;
            mouse_event.x = static_cast<int>(event.button.x);
            mouse_event.y = static_cast<int>(event.button.y);
            mouse_event.button = event.button.button;
            mouse_event.wheel_x = 0;
            mouse_event.wheel_y = 0;
            
            mouse_x_ = mouse_event.x;
            mouse_y_ = mouse_event.y;
            mouse_button_state_ &= ~SDL_BUTTON_MASK(event.button.button);
            break;
            
        case SDL_EVENT_MOUSE_WHEEL:
            mouse_event.type = MouseEventType::WHEEL;
            mouse_event.x = static_cast<int>(event.wheel.mouse_x);
            mouse_event.y = static_cast<int>(event.wheel.mouse_y);
            mouse_event.button = 0;
            mouse_event.wheel_x = static_cast<int>(event.wheel.x);
            mouse_event.wheel_y = static_cast<int>(event.wheel.y);
            break;
            
        default:
            return false;
    }
    
    if (mouse_callback_) {
        mouse_callback_(mouse_event);
    }
    
    return true;
}

bool InputHandler::HandleKeyboardEvent(const SDL_Event& event) {
    KeyEvent key_event;
    key_event.window_id = event.window.windowID;
    
    bool ctrl, shift, alt;
    GetModifierKeys(ctrl, shift, alt);
    key_event.ctrl = ctrl;
    key_event.shift = shift;
    key_event.alt = alt;
    
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            key_event.type = KeyEventType::DOWN;
            key_event.key = event.key.key;
            key_event.scancode = event.key.scancode;
            key_event.repeat = event.key.repeat;
            break;
            
        case SDL_EVENT_KEY_UP:
            key_event.type = KeyEventType::UP;
            key_event.key = event.key.key;
            key_event.scancode = event.key.scancode;
            key_event.repeat = false;
            break;
            
        case SDL_EVENT_TEXT_INPUT:
            key_event.type = KeyEventType::TEXT;
            key_event.key = SDLK_UNKNOWN;
            key_event.scancode = SDL_SCANCODE_UNKNOWN;
            key_event.text = event.text.text;
            key_event.repeat = false;
            break;
            
        default:
            return false;
    }
    
    if (keyboard_callback_) {
        keyboard_callback_(key_event);
    }
    
    return true;
}

void InputHandler::GetModifierKeys(bool& ctrl, bool& shift, bool& alt) const {
    SDL_Keymod mod = SDL_GetModState();
    ctrl = (mod & SDL_KMOD_CTRL) != 0;
    shift = (mod & SDL_KMOD_SHIFT) != 0;
    alt = (mod & SDL_KMOD_ALT) != 0;
}

} // namespace lightui

