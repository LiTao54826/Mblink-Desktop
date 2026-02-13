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
#include <dwmapi.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#undef SubclassWindow  // windowsx.h 定义了 SubclassWindow 宏，与我们的函数名冲突
#pragma comment(lib, "dwmapi.lib")

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
        }
    }

    // 查找关联的 Window 对象
    auto window_it = g_hwnd_to_window.find(hwnd);
    Window* window = (window_it != g_hwnd_to_window.end()) ? window_it->second : nullptr;

    switch (msg) {
        case WM_GETMINMAXINFO: {
            // 窗口最小/最大尺寸限制
            if (window) {
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                int min_w = 0, min_h = 0, max_w = 0, max_h = 0;
                window->GetMinSize(&min_w, &min_h);
                window->GetMaxSize(&max_w, &max_h);

                if (min_w > 0 || min_h > 0) {
                    if (min_w > 0) mmi->ptMinTrackSize.x = min_w;
                    if (min_h > 0) mmi->ptMinTrackSize.y = min_h;
                }
                if (max_w > 0 || max_h > 0) {
                    if (max_w > 0) mmi->ptMaxTrackSize.x = max_w;
                    if (max_h > 0) mmi->ptMaxTrackSize.y = max_h;
                }
            }
            // 继续传递给原始窗口过程，让系统也处理
            break;
        }

        case WM_NCHITTEST: {
            // 无边框窗口的自定义 Hit-Test
            // 实现窗口拖拽和边缘调整大小
            if (window && window->IsBorderless()) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &pt);

                RECT rc;
                GetClientRect(hwnd, &rc);

                const int BORDER_WIDTH = window->GetResizeBorderWidth();

                // 检查是否在调整大小边缘区域
                bool at_left   = pt.x < BORDER_WIDTH;
                bool at_right  = pt.x >= rc.right - BORDER_WIDTH;
                bool at_top    = pt.y < BORDER_WIDTH;
                bool at_bottom = pt.y >= rc.bottom - BORDER_WIDTH;

                // 四个角优先（角的区域更大，更容易抓取）
                if (at_top && at_left)     return HTTOPLEFT;
                if (at_top && at_right)    return HTTOPRIGHT;
                if (at_bottom && at_left)  return HTBOTTOMLEFT;
                if (at_bottom && at_right) return HTBOTTOMRIGHT;

                // 四条边
                if (at_left)   return HTLEFT;
                if (at_right)  return HTRIGHT;
                if (at_top)    return HTTOP;
                if (at_bottom) return HTBOTTOM;

                // 检查是否在 CSS -webkit-window-control 区域
                // 注意：不返回 HTCLOSE/HTMINBUTTON/HTMAXBUTTON，
                // 因为那会触发 Windows 绘制系统按钮图标。
                // 改为返回 HTCLIENT，在 WM_LBUTTONUP 中手动处理。
                std::string control = window->HitTestWindowControl(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                if (!control.empty()) {
                    return HTCLIENT;
                }

                // 检查是否在 CSS -webkit-app-region: drag 区域
                if (window->HitTestDragRegion(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
                    return HTCAPTION;
                }

                return HTCLIENT;
            }
            break;
        }

        case WM_LBUTTONUP: {
            // 处理 CSS -webkit-window-control 按钮点击
            if (window && window->IsBorderless()) {
                POINT screen_pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ClientToScreen(hwnd, &screen_pt);
                std::string control = window->HitTestWindowControl(screen_pt.x, screen_pt.y);
                if (!control.empty()) {
                    if (control == "close") {
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                    } else if (control == "minimize") {
                        ShowWindow(hwnd, SW_MINIMIZE);
                    } else if (control == "maximize") {
                        if (IsZoomed(hwnd)) {
                            ShowWindow(hwnd, SW_RESTORE);
                        } else {
                            ShowWindow(hwnd, SW_MAXIMIZE);
                        }
                    }
                    return 0;
                }
            }
            break;
        }

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

            // LayeredWindow 后端：不需要 WM_PAINT 绘制
            // UpdateLayeredWindow 直接操作窗口像素，WM_PAINT 只需验证区域
            if (window && window->IsTransparent()) {
                ValidateRect(hwnd, NULL);
                return 0;
            }

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
            // 修复：仅过滤“短时间内重复且尺寸完全相同”的噪声 WM_SIZE，
            // 不能拦截 maximize/restore 等真实窗口状态切换事件。
            static DWORD last_size_time = 0;
            static int size_count = 0;
            static UINT last_size_type = 0;
            static int last_width = -1;
            static int last_height = -1;

            DWORD now = GetTickCount();
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            bool is_state_change =
                (wParam == SIZE_MAXIMIZED) ||
                (wParam == SIZE_RESTORED) ||
                (wParam == SIZE_MINIMIZED);

            bool same_size_event =
                (width == last_width) &&
                (height == last_height) &&
                (wParam == last_size_type);

            if (!is_state_change && same_size_event && (now - last_size_time < 100)) {
                size_count++;
                // 只在明显重复风暴时拦截，避免吞掉真实 resize
                if (size_count > 6) {
                    if (g_debug_messages) {
                    }
                    return 0;  // 不传递给 SDL
                }
            } else {
                size_count = 1;
            }

            last_size_time = now;
            last_size_type = static_cast<UINT>(wParam);
            last_width = width;
            last_height = height;
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

void EnableBorderlessShadow(HWND hwnd) {
    if (!hwnd) return;

    // 通过 DwmExtendFrameIntoClientArea 为无边框窗口添加系统阴影
    // bottom margin = 1 即可触发 DWM 绘制阴影，而不会影响客户区布局
    MARGINS margins = { 0, 0, 0, 1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

}  // namespace win32
}  // namespace lightui

#endif  // _WIN32
