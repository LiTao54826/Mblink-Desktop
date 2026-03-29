/**
 * @file window_manager.h
 * @brief 窗口管理器
 * 
 * 功能：
 * - 管理所有窗口实例
 * - 提供窗口查找和枚举
 * - 处理窗口间通信
 * - 全局事件分发
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <SDL3/SDL.h>
#include "window.h"

namespace mbink {

/**
 * @brief 窗口管理器（单例）
 * 
 * 管理应用程序中的所有窗口，提供窗口查找、枚举和通信功能
 */
class WindowManager {
public:
    /**
     * @brief 获取单例实例
     * @return WindowManager 实例引用
     */
    static WindowManager& Instance();
    
    // 禁止拷贝和赋值
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    
    /**
     * @brief 注册窗口
     * @param window 窗口智能指针
     */
    void RegisterWindow(std::shared_ptr<Window> window);
    
    /**
     * @brief 注销窗口
     * @param window 窗口智能指针
     */
    void UnregisterWindow(std::shared_ptr<Window> window);
    
    /**
     * @brief 通过 SDL 窗口 ID 查找窗口
     * @param window_id SDL 窗口 ID
     * @return 窗口智能指针，未找到返回 nullptr
     */
    std::shared_ptr<Window> FindWindowByID(Uint32 window_id);
    
    /**
     * @brief 通过 SDL_Window 指针查找窗口
     * @param sdl_window SDL 窗口指针
     * @return 窗口智能指针，未找到返回 nullptr
     */
    std::shared_ptr<Window> FindWindowBySDLWindow(SDL_Window* sdl_window);
    
    /**
     * @brief 获取所有窗口
     * @return 窗口列表
     */
    std::vector<std::shared_ptr<Window>> GetAllWindows() const;
    
    /**
     * @brief 获取窗口数量
     * @return 窗口数量
     */
    size_t GetWindowCount() const { return windows_.size(); }
    
    /**
     * @brief 检查是否有窗口
     * @return true 表示有窗口
     */
    bool HasWindows() const { return !windows_.empty(); }
    
    /**
     * @brief 处理所有窗口的 SDL 事件
     * @param event SDL 事件
     * @return true 表示事件已被处理
     */
    bool HandleEvent(const SDL_Event& event);
    
    /**
     * @brief 关闭所有窗口
     */
    void CloseAllWindows();
    
    /**
     * @brief 向所有窗口广播消息
     * @param message 消息内容
     * @param data 消息数据
     */
    void BroadcastMessage(const std::string& message, void* data = nullptr);
    
    /**
     * @brief 设置消息处理器
     * @param handler 消息处理函数
     */
    void SetMessageHandler(std::function<void(const std::string&, void*)> handler) {
        message_handler_ = handler;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    WindowManager() = default;
    
    /**
     * @brief 析构函数
     */
    ~WindowManager() = default;
    
    // 窗口列表（使用 weak_ptr 避免循环引用）
    std::vector<std::weak_ptr<Window>> windows_;
    
    // SDL 窗口 ID 到窗口的映射
    std::unordered_map<Uint32, std::weak_ptr<Window>> window_id_map_;
    
    // 消息处理器
    std::function<void(const std::string&, void*)> message_handler_;
    
    /**
     * @brief 清理已销毁的窗口
     */
    void CleanupDestroyedWindows();
};

} // namespace mbink

