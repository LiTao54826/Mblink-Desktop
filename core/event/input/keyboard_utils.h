/**
 * @file keyboard_utils.h
 * @brief 键盘工具函数 - SDL按键到W3C按键名称映射
 * 
 * 参考：
 * - W3C UI Events - KeyboardEvent key Values
 * - W3C UI Events - KeyboardEvent code Values
 * - SDL3 - SDL_Keycode
 */

#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace mbink {

/**
 * @brief 将SDL按键码转换为W3C key值
 * @param keycode SDL按键码
 * @param shift 是否按下Shift键
 * @return W3C key值（如"a", "Enter", "ArrowUp"）
 * 
 * 参考：W3C UI Events - KeyboardEvent.key
 */
std::string SDLKeycodeToKey(SDL_Keycode keycode, bool shift = false);

/**
 * @brief 将SDL扫描码转换为W3C code值
 * @param scancode SDL扫描码
 * @return W3C code值（如"KeyA", "Enter", "ArrowUp"）
 * 
 * 参考：W3C UI Events - KeyboardEvent.code
 */
std::string SDLScancodeToCode(SDL_Scancode scancode);

/**
 * @brief 将SDL按键码转换为keyCode值（已废弃但保留兼容性）
 * @param keycode SDL按键码
 * @return keyCode值
 */
int SDLKeycodeToKeyCode(SDL_Keycode keycode);

} // namespace mbink

