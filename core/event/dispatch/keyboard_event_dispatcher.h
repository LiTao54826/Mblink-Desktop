/**
 * @file keyboard_event_dispatcher.h
 * @brief 键盘事件分发器
 *
 * 从 event_loop.cpp 提取的键盘事件处理逻辑。
 * 负责处理键盘按下、抬起、文本输入等事件。
 *
 * 参考：
 * - W3C UI Events - KeyboardEvent
 * - RmlUi/Source/Core/Context.cpp - ProcessKeyDown, ProcessKeyUp
 */

#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <functional>

namespace lightui {

// 前向声明
class Element;
class Document;
class Window;
class FocusManager;
class ContentEditableHandler;
class ClipboardManager;

/**
 * @brief 键盘事件分发器
 *
 * 负责：
 * 1. 处理键盘按下/抬起事件
 * 2. 处理文本输入事件
 * 3. 处理快捷键（Ctrl+C/V/X 等）
 * 4. 分发事件到焦点元素
 */
class KeyboardEventDispatcher {
public:
    /**
     * @brief 构造函数
     */
    KeyboardEventDispatcher();

    /**
     * @brief 析构函数
     */
    ~KeyboardEventDispatcher();

    /**
     * @brief 设置依赖的管理器
     * @param focus_manager 焦点管理器
     * @param contenteditable_handler 可编辑内容处理器
     * @param clipboard_manager 剪贴板管理器
     */
    void SetManagers(FocusManager* focus_manager,
                     ContentEditableHandler* contenteditable_handler,
                     ClipboardManager* clipboard_manager);

    /**
     * @brief 处理键盘事件
     * @param event SDL 键盘事件
     * @param window 目标窗口
     * @param document 目标文档
     * @return true 如果事件被处理
     */
    bool HandleKeyboardEvent(const SDL_Event& event,
                              std::shared_ptr<Window> window,
                              std::shared_ptr<Document> document);

    /**
     * @brief 处理文本输入事件
     * @param event SDL 文本输入事件
     * @param window 目标窗口
     * @param document 目标文档
     * @return true 如果事件被处理
     */
    bool HandleTextInputEvent(const SDL_Event& event,
                               std::shared_ptr<Window> window,
                               std::shared_ptr<Document> document);

    /**
     * @brief 获取当前焦点元素
     * @return 当前焦点元素
     */
    std::shared_ptr<Element> GetFocusElement() const;

private:
    /**
     * @brief 处理快捷键
     * @param keycode 按键码
     * @param ctrl 是否按住 Ctrl
     * @param shift 是否按住 Shift
     * @param alt 是否按住 Alt
     * @param document 目标文档
     * @return true 如果快捷键被处理
     */
    bool HandleShortcut(SDL_Keycode keycode,
                        bool ctrl,
                        bool shift,
                        bool alt,
                        std::shared_ptr<Document> document);

    /**
     * @brief 将 SDL 按键码转换为 DOM key 字符串
     * @param keycode SDL 按键码
     * @return DOM key 字符串
     */
    static std::string SDLKeycodeToDOMKey(SDL_Keycode keycode);

    /**
     * @brief 将 SDL 按键码转换为 DOM code 字符串
     * @param scancode SDL 扫描码
     * @return DOM code 字符串
     */
    static std::string SDLScancodeToDOMCode(SDL_Scancode scancode);

private:
    // 依赖的管理器（不拥有所有权）
    FocusManager* focus_manager_ = nullptr;
    ContentEditableHandler* contenteditable_handler_ = nullptr;
    ClipboardManager* clipboard_manager_ = nullptr;
};

} // namespace lightui
