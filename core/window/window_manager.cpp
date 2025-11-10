/**
 * @file window_manager.cpp
 * @brief 窗口管理器实现
 */

#include "window_manager.h"
#include <algorithm>

namespace lightui {

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

void WindowManager::RegisterWindow(std::shared_ptr<Window> window) {
    if (!window) {
        return;
    }
    
    // 清理已销毁的窗口
    CleanupDestroyedWindows();
    
    // 添加到窗口列表
    windows_.push_back(window);
    
    // 添加到 ID 映射
    if (window->GetSDLWindow()) {
        Uint32 window_id = SDL_GetWindowID(window->GetSDLWindow());
        window_id_map_[window_id] = window;
    }
}

void WindowManager::UnregisterWindow(std::shared_ptr<Window> window) {
    if (!window) {
        return;
    }
    
    // 从 ID 映射中移除
    if (window->GetSDLWindow()) {
        Uint32 window_id = SDL_GetWindowID(window->GetSDLWindow());
        window_id_map_.erase(window_id);
    }
    
    // 从窗口列表中移除
    windows_.erase(
        std::remove_if(windows_.begin(), windows_.end(),
            [&window](const std::weak_ptr<Window>& weak_win) {
                auto win = weak_win.lock();
                return !win || win == window;
            }),
        windows_.end()
    );
}

std::shared_ptr<Window> WindowManager::FindWindowByID(Uint32 window_id) {
    auto it = window_id_map_.find(window_id);
    if (it != window_id_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::shared_ptr<Window> WindowManager::FindWindowBySDLWindow(SDL_Window* sdl_window) {
    if (!sdl_window) {
        return nullptr;
    }
    
    Uint32 window_id = SDL_GetWindowID(sdl_window);
    return FindWindowByID(window_id);
}

std::vector<std::shared_ptr<Window>> WindowManager::GetAllWindows() const {
    std::vector<std::shared_ptr<Window>> result;
    
    for (const auto& weak_win : windows_) {
        if (auto win = weak_win.lock()) {
            result.push_back(win);
        }
    }
    
    return result;
}

bool WindowManager::HandleEvent(const SDL_Event& event) {
    // 窗口事件
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        auto window = FindWindowByID(event.window.windowID);
        if (window) {
            return window->HandleSDLEvent(event);
        }
    }
    
    // 其他事件可以在这里处理
    // 例如：鼠标事件、键盘事件等
    
    return false;
}

void WindowManager::CloseAllWindows() {
    auto windows = GetAllWindows();
    for (auto& window : windows) {
        window->SetShouldClose(true);
    }
}

void WindowManager::BroadcastMessage(const std::string& message, void* data) {
    if (message_handler_) {
        message_handler_(message, data);
    }
    
    // 可以在这里添加更复杂的消息分发逻辑
    // 例如：向每个窗口发送消息
}

void WindowManager::CleanupDestroyedWindows() {
    // 移除已销毁的窗口
    windows_.erase(
        std::remove_if(windows_.begin(), windows_.end(),
            [](const std::weak_ptr<Window>& weak_win) {
                return weak_win.expired();
            }),
        windows_.end()
    );
    
    // 清理 ID 映射中的无效条目
    for (auto it = window_id_map_.begin(); it != window_id_map_.end();) {
        if (it->second.expired()) {
            it = window_id_map_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace lightui

