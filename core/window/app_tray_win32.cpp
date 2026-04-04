#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include "app_tray.h"
#include "core/utils/encoding_utils.h"

#include <shellapi.h>
#include <windowsx.h>
#include <unordered_map>
#include <utility>

namespace {
constexpr UINT kTrayCallbackMessage = WM_APP + 0x3A1;
constexpr UINT kTrayMenuBaseId = 40000;
}

namespace mbink {

class Win32AppTray final : public AppTray {
public:
    explicit Win32AppTray(AppTrayConfig config) : config_(std::move(config)) {}
    ~Win32AppTray() override { Destroy(); }
    bool Create() override;
    void Destroy() override;
    void SetTooltip(const std::string& tooltip) override;
    void SetMenuItems(const std::vector<AppTrayMenuItem>& items) override { menu_items_ = items; }
    void SetLeftClickHandler(std::function<void()> handler) override { on_left_click_ = std::move(handler); }
    void SetMenuItemHandler(std::function<void(const std::string&)> handler) override { on_menu_item_ = std::move(handler); }
private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool CreateMessageWindow();
    void ShowContextMenu();
    void HandleCommand(UINT command_id);
    HICON LoadTrayIcon() const;
    AppTrayConfig config_;
    std::vector<AppTrayMenuItem> menu_items_;
    std::function<void()> on_left_click_;
    std::function<void(const std::string&)> on_menu_item_;
    HWND hwnd_ = nullptr;
    NOTIFYICONDATAW nid_{};
    std::unordered_map<UINT, std::string> command_map_;
};

bool Win32AppTray::CreateMessageWindow() {
    static const wchar_t* kClassName = L"MBinkAppTrayWindow";
    static bool registered = false;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registered) {
        WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = instance; wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW); if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        registered = true;
    }
    hwnd_ = CreateWindowExW(0, kClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, this);
    return hwnd_ != nullptr;
}

HICON Win32AppTray::LoadTrayIcon() const {
    if (config_.owner_native_window) {
        auto owner = static_cast<HWND>(config_.owner_native_window);
        if (auto icon = reinterpret_cast<HICON>(SendMessage(owner, WM_GETICON, ICON_SMALL, 0))) return icon;
        if (auto cls = reinterpret_cast<HICON>(GetClassLongPtr(owner, GCLP_HICONSM))) return cls;
    }
    return LoadIcon(nullptr, IDI_APPLICATION);
}

bool Win32AppTray::Create() {
    if (hwnd_ || !CreateMessageWindow()) return hwnd_ != nullptr;
    nid_.cbSize = sizeof(nid_); nid_.hWnd = hwnd_; nid_.uID = 1; nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = kTrayCallbackMessage; nid_.hIcon = LoadTrayIcon(); SetTooltip(config_.tooltip);
    return Shell_NotifyIconW(NIM_ADD, &nid_) == TRUE;
}

void Win32AppTray::Destroy() {
    if (nid_.hWnd) Shell_NotifyIconW(NIM_DELETE, &nid_);
    ZeroMemory(&nid_, sizeof(nid_)); command_map_.clear();
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void Win32AppTray::SetTooltip(const std::string& tooltip) {
    config_.tooltip = tooltip;
    if (!hwnd_) return;
    auto wide = utils::UTF8ToWide(tooltip);
    wcsncpy_s(nid_.szTip, wide.c_str(), _TRUNCATE);
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(nid_.hWnd ? NIM_MODIFY : NIM_ADD, &nid_);
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
}

void Win32AppTray::HandleCommand(UINT command_id) {
    auto it = command_map_.find(command_id);
    if (it != command_map_.end() && on_menu_item_) on_menu_item_(it->second);
}

void Win32AppTray::ShowContextMenu() {
    HMENU menu = CreatePopupMenu(); if (!menu) return; command_map_.clear(); UINT next_id = kTrayMenuBaseId;
    for (const auto& item : menu_items_) {
        if (item.type == AppTrayMenuItemType::Separator) { AppendMenuW(menu, MF_SEPARATOR, 0, nullptr); continue; }
        UINT flags = MF_STRING | (item.enabled ? MF_ENABLED : MF_GRAYED) | (item.checked ? MF_CHECKED : 0);
        auto label = utils::UTF8ToWide(item.label); AppendMenuW(menu, flags, next_id, label.c_str()); command_map_[next_id++] = item.id;
    }
    POINT pt{}; GetCursorPos(&pt); SetForegroundWindow(hwnd_);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    if (cmd != 0) HandleCommand(cmd); DestroyMenu(menu);
}

LRESULT CALLBACK Win32AppTray::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<Win32AppTray*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam); auto* instance = static_cast<Win32AppTray*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instance)); return TRUE;
    }
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
    if (msg == WM_COMMAND) { self->HandleCommand(LOWORD(wParam)); return 0; }
    if (msg == kTrayCallbackMessage) {
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) { if (self->on_left_click_) self->on_left_click_(); return 0; }
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) { self->ShowContextMenu(); return 0; }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::unique_ptr<AppTray> CreateWin32AppTray(const AppTrayConfig& config) {
    return std::make_unique<Win32AppTray>(config);
}

}  // namespace mbink

#endif
