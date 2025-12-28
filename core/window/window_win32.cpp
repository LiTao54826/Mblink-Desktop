/**
 * @file window_win32.cpp
 * @brief Windows 平台特定代码实现
 * 
 * 从 window.cpp 提取的 Windows 子类化窗口过程。
 * 用于拦截可能导致闪烁的消息。
 */

#ifdef _WIN32

// 必须在 windows.h 之前定义，避免 min/max 宏与 Skia 冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "window_win32.h"
#include "window.h"
#include "display_backend.h"
#include <unordered_map>
#include <iostream>
#include <cstdlib>

namespace lightui {
namespace win32 {

// Windows 子类化窗口过程，用于拦截可能导致闪烁的消息
static std::unordered_map<HWND, WNDPROC> g_original_wndprocs;
static std::unordered_map<HWND, Window*> g_hwnd_to_window;

// 调试：是否启用消息日志
static bool g_debug_messages = false;
static int g_paint_count = 0;
static int g_present_count = 0;
static DWORD g_last_stats_time = 0;

// 缓存窗口大小，用于检测虚假的大小变化
static std::unordered_map<HWND, RECT> g_window_rects;

static const char* GetMessageName(UINT msg) {
    switch (msg) {
        case WM_PAINT: return "WM_PAINT";
        case WM_ERASEBKGND: return "WM_ERASEBKGND";
        case WM_NCPAINT: return "WM_NCPAINT";
        case WM_NCACTIVATE: return "WM_NCACTIVATE";
        case WM_NCHITTEST: return "WM_NCHITTEST";
        case WM_SETCURSOR: return "WM_SETCURSOR";
        case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
        case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
        case WM_SYNCPAINT: return "WM_SYNCPAINT";
        case WM_SETREDRAW: return "WM_SETREDRAW";
        case WM_NCMOUSEMOVE: return "WM_NCMOUSEMOVE";
        case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
        case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
        case WM_SIZE: return "WM_SIZE";
        case WM_MOVE: return "WM_MOVE";
        case WM_ACTIVATE: return "WM_ACTIVATE";
        case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
        case WM_DWMCOMPOSITIONCHANGED: return "WM_DWMCOMPOSITIONCHANGED";
        default: return nullptr;
    }
}

void PrintStats() {
    DWORD now = GetTickCount();
    if (now - g_last_stats_time >= 1000) {
        if (g_debug_messages && (g_paint_count > 0 || g_present_count > 0)) {
            std::cout << "[Stats] WM_PAINT: " << g_paint_count
                      << "/s, Present: " << g_present_count << "/s" << std::endl;
        }
        g_paint_count = 0;
        g_present_count = 0;
        g_last_stats_time = now;
    }
}

void SetDebugMessages(bool enable) {
    g_debug_messages = enable;
}

void IncrementPresentCount() {
    g_present_count++;
}

static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto proc_it = g_original_wndprocs.find(hwnd);

    if (proc_it == g_original_wndprocs.end()) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // 调试输出
    if (g_debug_messages) {
        const char* name = GetMessageName(msg);
        if (name) {
            std::cout << "[WndProc] " << name << " (0x" << std::hex << msg << std::dec << ")" << std::endl;
        }
    }

    // 查找关联的 Window 对象
    auto window_it = g_hwnd_to_window.find(hwnd);
    Window* window = (window_it != g_hwnd_to_window.end()) ? window_it->second : nullptr;

    switch (msg) {
        case WM_ERASEBKGND:
            // 阻止 Windows 擦除背景，避免闪烁
            return 1;

        case WM_WINDOWPOSCHANGING: {
            // 关键修复：阻止虚拟机中的虚假大小变化
            WINDOWPOS* wp = (WINDOWPOS*)lParam;

            if (g_debug_messages && !(wp->flags & SWP_NOSIZE)) {
                auto rect_it = g_window_rects.find(hwnd);
                if (rect_it != g_window_rects.end()) {
                    RECT& cached = rect_it->second;
                    int cached_width = cached.right - cached.left;
                    int cached_height = cached.bottom - cached.top;
                    std::cout << "[WndProc] WINDOWPOSCHANGING: cached=" << cached_width << "x" << cached_height
                              << " new=" << wp->cx << "x" << wp->cy
                              << " flags=0x" << std::hex << wp->flags << std::dec << std::endl;
                }
            }

            // 获取缓存的窗口位置
            auto rect_it = g_window_rects.find(hwnd);
            if (rect_it != g_window_rects.end()) {
                RECT& cached = rect_it->second;
                int cached_width = cached.right - cached.left;
                int cached_height = cached.bottom - cached.top;

                // 如果大小没有真正变化，阻止它（包括相同大小的情况）
                if (!(wp->flags & SWP_NOSIZE)) {
                    int dw = abs(wp->cx - cached_width);
                    int dh = abs(wp->cy - cached_height);
                    // 阻止所有小于等于 5 像素的变化（虚假变化）
                    if (dw <= 5 && dh <= 5) {
                        wp->flags |= SWP_NOSIZE;
                        if (g_debug_messages && (dw > 0 || dh > 0)) {
                            std::cout << "[WndProc] Blocked resize: dw=" << dw << ", dh=" << dh << std::endl;
                        }
                    }
                }
            }
            break;
        }

        case WM_WINDOWPOSCHANGED: {
            // 更新缓存的窗口大小
            WINDOWPOS* wp = (WINDOWPOS*)lParam;
            if (!(wp->flags & SWP_NOSIZE)) {
                RECT rect;
                GetWindowRect(hwnd, &rect);
                g_window_rects[hwnd] = rect;
            }
            break;
        }

        case WM_NCACTIVATE: {
            // 尝试：阻止非客户区激活导致的重绘循环
            if (g_debug_messages) {
                std::cout << "[WndProc] WM_NCACTIVATE: wParam=" << wParam << std::endl;
            }
            // 关键：使用 lParam = -1 告诉系统不要重绘非客户区
            // 但仍然调用 DefWindowProc 来更新窗口状态
            return DefWindowProc(hwnd, msg, wParam, (LPARAM)-1);
        }

        case WM_NCPAINT: {
            // 关键：完全接管非客户区绘制
            // 先让系统绘制标题栏
            LRESULT result = CallWindowProc(proc_it->second, hwnd, msg, wParam, lParam);
            // 然后阻止后续的客户区重绘
            ValidateRect(hwnd, NULL);
            return result;
        }

        case WM_PAINT: {
            g_paint_count++;
            PrintStats();

            // 检查是否使用 PaintMode 后端
            if (window && window->GetPaintModeBackend()) {
                // PaintMode: 在 WM_PAINT 中实际绘制
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                window->GetPaintModeBackend()->OnPaint(hdc, &ps.rcPaint);
                EndPaint(hwnd, &ps);
                return 0;
            }

            // 其他后端: 验证窗口区域但不绘制，我们的渲染循环会处理
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SETREDRAW:
            // 忽略 SetRedraw 调用，防止闪烁
            return 0;

        case WM_SYNCPAINT:
            // 忽略同步绘制请求
            return 0;

        case WM_SIZE: {
            // 关键：检测循环并阻止
            static DWORD last_size_time = 0;
            static int size_count = 0;
            DWORD now = GetTickCount();

            if (now - last_size_time < 100) {
                size_count++;
                if (size_count > 2) {
                    // 在 100ms 内收到超过 2 次 WM_SIZE，可能是循环
                    if (g_debug_messages) {
                        std::cout << "[WndProc] Blocked WM_SIZE loop (count=" << size_count << ")" << std::endl;
                    }
                    return 0;  // 不传递给 SDL
                }
            } else {
                size_count = 1;
            }
            last_size_time = now;
            break;
        }

        case WM_NCDESTROY:
            // 清理缓存
            g_window_rects.erase(hwnd);
            break;
    }

    return CallWindowProc(proc_it->second, hwnd, msg, wParam, lParam);
}

void SubclassWindow(HWND hwnd, Window* window) {
    if (!hwnd) return;
    
    // 保存原始窗口过程
    WNDPROC original = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SubclassWndProc);
    g_original_wndprocs[hwnd] = original;
    g_hwnd_to_window[hwnd] = window;
    
    // 缓存初始窗口大小
    RECT rect;
    GetWindowRect(hwnd, &rect);
    g_window_rects[hwnd] = rect;
}

void UnsubclassWindow(HWND hwnd) {
    if (!hwnd) return;
    
    auto it = g_original_wndprocs.find(hwnd);
    if (it != g_original_wndprocs.end()) {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)it->second);
        g_original_wndprocs.erase(it);
    }
    g_hwnd_to_window.erase(hwnd);
    g_window_rects.erase(hwnd);
}

}  // namespace win32
}  // namespace lightui

#endif  // _WIN32
