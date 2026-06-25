/**
 * @file keyboard_utils.cpp
 * @brief 键盘工具函数实现
 * 
 * 参考：
 * - W3C UI Events - KeyboardEvent key Values
 * - W3C UI Events - KeyboardEvent code Values
 * - SDL3 - SDL_Keycode
 */

#include "keyboard_utils.h"
#include <cctype>

namespace mblink {

std::string SDLKeycodeToKey(SDL_Keycode keycode, bool shift) {
    // 参考：W3C UI Events - KeyboardEvent.key
    // https://www.w3.org/TR/uievents-key/

    // 调试输出

    // 特殊键
    switch (keycode) {
        // 修饰键
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            return "Shift";
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            return "Control";
        case SDLK_LALT:
        case SDLK_RALT:
            return "Alt";
        case SDLK_LGUI:
        case SDLK_RGUI:
            return "Meta";

        // 导航键
        case SDLK_DOWN:
            return "ArrowDown";
        case SDLK_LEFT:
            return "ArrowLeft";
        case SDLK_RIGHT:
            return "ArrowRight";
        case SDLK_UP:
            return "ArrowUp";
        case SDLK_END:
            return "End";
        case SDLK_HOME:
            return "Home";
        case SDLK_PAGEDOWN:
            return "PageDown";
        case SDLK_PAGEUP:
            return "PageUp";

        // 编辑键
        case SDLK_BACKSPACE:
            return "Backspace";
        case SDLK_DELETE:
            return "Delete";
        case SDLK_INSERT:
            return "Insert";

        // UI键
        case SDLK_ESCAPE:
            return "Escape";
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:  // 小键盘回车
            return "Enter";
        case SDLK_TAB:
            return "Tab";
        case SDLK_SPACE:
            return " ";

        // 功能键
        case SDLK_F1: return "F1";
        case SDLK_F2: return "F2";
        case SDLK_F3: return "F3";
        case SDLK_F4: return "F4";
        case SDLK_F5: return "F5";
        case SDLK_F6: return "F6";
        case SDLK_F7: return "F7";
        case SDLK_F8: return "F8";
        case SDLK_F9: return "F9";
        case SDLK_F10: return "F10";
        case SDLK_F11: return "F11";
        case SDLK_F12: return "F12";
            
        // 锁定键
        case SDLK_CAPSLOCK:
            return "CapsLock";
        case SDLK_NUMLOCKCLEAR:
            return "NumLock";
        case SDLK_SCROLLLOCK:
            return "ScrollLock";
            
        default:
            break;
    }
    
    // 字母和数字键
    // SDL3中字母键是小写的（SDLK_A到SDLK_Z，值为0x61-0x7a，即97-122）
    if (keycode >= SDLK_A && keycode <= SDLK_Z) {
        char c = static_cast<char>(keycode);  // 已经是小写
        if (shift) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return std::string(1, c);
    }

    if (keycode >= SDLK_0 && keycode <= SDLK_9) {
        if (shift) {
            // Shift + 数字键 = 符号
            const char* symbols = ")!@#$%^&*(";
            return std::string(1, symbols[keycode - SDLK_0]);
        }
        return std::string(1, static_cast<char>(keycode));
    }
    
    // 符号键（考虑Shift）
    switch (keycode) {
        case SDLK_MINUS:
            return shift ? "_" : "-";
        case SDLK_EQUALS:
            return shift ? "+" : "=";
        case SDLK_LEFTBRACKET:
            return shift ? "{" : "[";
        case SDLK_RIGHTBRACKET:
            return shift ? "}" : "]";
        case SDLK_BACKSLASH:
            return shift ? "|" : "\\";
        case SDLK_SEMICOLON:
            return shift ? ":" : ";";
        case SDLK_APOSTROPHE:
            return shift ? "\"" : "'";
        case SDLK_GRAVE:
            return shift ? "~" : "`";
        case SDLK_COMMA:
            return shift ? "<" : ",";
        case SDLK_PERIOD:
            return shift ? ">" : ".";
        case SDLK_SLASH:
            return shift ? "?" : "/";
        default:
            break;
    }
    
    // 未知键
    return "Unidentified";
}

std::string SDLScancodeToCode(SDL_Scancode scancode) {
    // 参考：W3C UI Events - KeyboardEvent.code
    // https://www.w3.org/TR/uievents-code/
    
    // 字母键
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        char c = static_cast<char>('A' + (scancode - SDL_SCANCODE_A));
        return "Key" + std::string(1, c);
    }
    
    // 数字键
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        char c = static_cast<char>('1' + (scancode - SDL_SCANCODE_1));
        return "Digit" + std::string(1, c);
    }
    if (scancode == SDL_SCANCODE_0) {
        return "Digit0";
    }
    
    // 特殊键
    switch (scancode) {
        // 修饰键
        case SDL_SCANCODE_LSHIFT: return "ShiftLeft";
        case SDL_SCANCODE_RSHIFT: return "ShiftRight";
        case SDL_SCANCODE_LCTRL: return "ControlLeft";
        case SDL_SCANCODE_RCTRL: return "ControlRight";
        case SDL_SCANCODE_LALT: return "AltLeft";
        case SDL_SCANCODE_RALT: return "AltRight";
        case SDL_SCANCODE_LGUI: return "MetaLeft";
        case SDL_SCANCODE_RGUI: return "MetaRight";
            
        // 导航键
        case SDL_SCANCODE_DOWN: return "ArrowDown";
        case SDL_SCANCODE_LEFT: return "ArrowLeft";
        case SDL_SCANCODE_RIGHT: return "ArrowRight";
        case SDL_SCANCODE_UP: return "ArrowUp";
        case SDL_SCANCODE_END: return "End";
        case SDL_SCANCODE_HOME: return "Home";
        case SDL_SCANCODE_PAGEDOWN: return "PageDown";
        case SDL_SCANCODE_PAGEUP: return "PageUp";
            
        // 编辑键
        case SDL_SCANCODE_BACKSPACE: return "Backspace";
        case SDL_SCANCODE_DELETE: return "Delete";
        case SDL_SCANCODE_INSERT: return "Insert";
            
        // UI键
        case SDL_SCANCODE_ESCAPE: return "Escape";
        case SDL_SCANCODE_RETURN: return "Enter";
        case SDL_SCANCODE_TAB: return "Tab";
        case SDL_SCANCODE_SPACE: return "Space";
            
        // 功能键
        case SDL_SCANCODE_F1: return "F1";
        case SDL_SCANCODE_F2: return "F2";
        case SDL_SCANCODE_F3: return "F3";
        case SDL_SCANCODE_F4: return "F4";
        case SDL_SCANCODE_F5: return "F5";
        case SDL_SCANCODE_F6: return "F6";
        case SDL_SCANCODE_F7: return "F7";
        case SDL_SCANCODE_F8: return "F8";
        case SDL_SCANCODE_F9: return "F9";
        case SDL_SCANCODE_F10: return "F10";
        case SDL_SCANCODE_F11: return "F11";
        case SDL_SCANCODE_F12: return "F12";
            
        // 锁定键
        case SDL_SCANCODE_CAPSLOCK: return "CapsLock";
        case SDL_SCANCODE_NUMLOCKCLEAR: return "NumLock";
        case SDL_SCANCODE_SCROLLLOCK: return "ScrollLock";
            
        // 符号键
        case SDL_SCANCODE_MINUS: return "Minus";
        case SDL_SCANCODE_EQUALS: return "Equal";
        case SDL_SCANCODE_LEFTBRACKET: return "BracketLeft";
        case SDL_SCANCODE_RIGHTBRACKET: return "BracketRight";
        case SDL_SCANCODE_BACKSLASH: return "Backslash";
        case SDL_SCANCODE_SEMICOLON: return "Semicolon";
        case SDL_SCANCODE_APOSTROPHE: return "Quote";
        case SDL_SCANCODE_GRAVE: return "Backquote";
        case SDL_SCANCODE_COMMA: return "Comma";
        case SDL_SCANCODE_PERIOD: return "Period";
        case SDL_SCANCODE_SLASH: return "Slash";
            
        default:
            break;
    }
    
    return "Unidentified";
}

int SDLKeycodeToKeyCode(SDL_Keycode keycode) {
    // 参考：已废弃的keyCode值
    // 字母键：65-90 (A-Z)
    // 数字键：48-57 (0-9)

    // SDL3中字母键是小写的（SDLK_a到SDLK_z，值为97-122）
    // 需要转换为大写的 keyCode（65-90）
    if (keycode >= SDLK_A && keycode <= SDLK_Z) {
        return keycode - 32;  // 转换为大写 (97 -> 65, 122 -> 90)
    }

    if (keycode >= SDLK_0 && keycode <= SDLK_9) {
        return keycode;
    }
    
    // 特殊键
    switch (keycode) {
        case SDLK_RETURN: return 13;
        case SDLK_ESCAPE: return 27;
        case SDLK_BACKSPACE: return 8;
        case SDLK_TAB: return 9;
        case SDLK_SPACE: return 32;
        case SDLK_DELETE: return 46;
        case SDLK_LEFT: return 37;
        case SDLK_UP: return 38;
        case SDLK_RIGHT: return 39;
        case SDLK_DOWN: return 40;
        case SDLK_PAGEUP: return 33;
        case SDLK_PAGEDOWN: return 34;
        case SDLK_END: return 35;
        case SDLK_HOME: return 36;
        case SDLK_INSERT: return 45;
        case SDLK_F1: return 112;
        case SDLK_F2: return 113;
        case SDLK_F3: return 114;
        case SDLK_F4: return 115;
        case SDLK_F5: return 116;
        case SDLK_F6: return 117;
        case SDLK_F7: return 118;
        case SDLK_F8: return 119;
        case SDLK_F9: return 120;
        case SDLK_F10: return 121;
        case SDLK_F11: return 122;
        case SDLK_F12: return 123;
        default:
            return 0;
    }
}

} // namespace mblink

