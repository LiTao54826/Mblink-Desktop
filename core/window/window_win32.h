/**
 * @file window_win32.h
 * @brief Windows 平台特定代码
 * 
 * 从 window.cpp 提取的 Windows 子类化窗口过程。
 * 用于拦截可能导致闪烁的消息。
 */

#ifndef MBINK_WINDOW_WIN32_H
#define MBINK_WINDOW_WIN32_H

#ifdef _WIN32

// windows.h 已在 window.h 中 include
// 这里只需要确保 NOMINMAX 已定义
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace mbink {

class Window;

namespace win32 {

/**
 * @brief 子类化窗口，拦截闪烁相关消息
 * @param hwnd 窗口句柄
 * @param window 关联的 Window 对象
 */
void SubclassWindow(HWND hwnd, Window* window);

/**
 * @brief 移除窗口子类化
 * @param hwnd 窗口句柄
 */
void UnsubclassWindow(HWND hwnd);

/**
 * @brief 启用无边框窗口的 DWM 阴影效果
 * @param hwnd 窗口句柄
 *
 * 通过 DwmExtendFrameIntoClientArea 为无边框窗口添加系统阴影，
 * 使其看起来更像原生窗口。
 */
void EnableBorderlessShadow(HWND hwnd);

/**
 * @brief 设置调试消息开关
 * @param enable 是否启用
 */
void SetDebugMessages(bool enable);

/**
 * @brief 增加 Present 计数（用于统计）
 */
void IncrementPresentCount();

/**
 * @brief 打印统计信息
 */
void PrintStats();

}  // namespace win32
}  // namespace mbink

#endif  // _WIN32

#endif  // MBINK_WINDOW_WIN32_H
