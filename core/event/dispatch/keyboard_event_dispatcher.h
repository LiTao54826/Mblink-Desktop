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

namespace mbink {

// 前向声明
class Element;
class Document;
class Window;
class FocusManager;
class ClipboardManager;
class ContentEditableController;
class EditorInputSession;

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
     * @param contenteditable_controller 可编辑内容控制器
     * @param clipboard_manager 剪贴板管理器
     */
    void SetManagers(FocusManager* focus_manager,
                     ClipboardManager* clipboard_manager,
                     ContentEditableController* contenteditable_controller,
                     EditorInputSession* editor_input_session);

    /**
     * @brief 处理键盘事件（keydown/keyup/textinput）
     * @param event SDL 键盘事件
     * @param window 目标窗口
     * @param document 目标文档
     * @return true 如果事件被处理
     */
    bool HandleKeyboardEvent(const SDL_Event& event,
                              std::shared_ptr<Window> window,
                              std::shared_ptr<Document> document);

    /**
     * @brief 获取当前焦点元素
     * @return 当前焦点元素
     */
    std::shared_ptr<Element> GetFocusElement() const;

private:
    /**
     * @brief 处理 keydown 事件
     */
    void HandleKeyDown(const SDL_Event& event,
                       std::shared_ptr<Element> focus_element,
                       std::shared_ptr<Document> document,
                       std::shared_ptr<Window> window,
                       bool ctrl_key, bool shift_key, bool alt_key, bool meta_key);

    /**
     * @brief 处理 keyup 事件
     */
    void HandleKeyUp(const SDL_Event& event,
                     std::shared_ptr<Element> focus_element,
                     bool ctrl_key, bool shift_key, bool alt_key, bool meta_key);

    /**
     * @brief 处理文本输入事件
     */
    void HandleTextInput(const SDL_Event& event,
                         std::shared_ptr<Element> focus_element,
                         std::shared_ptr<Document> document);

    /**
     * @brief 处理 IME 预编辑事件
     */
    void HandleTextEditing(const SDL_Event& event,
                           std::shared_ptr<Element> focus_element,
                           std::shared_ptr<Document> document);

private:
    // 依赖的管理器（不拥有所有权）
    FocusManager* focus_manager_ = nullptr;
    ClipboardManager* clipboard_manager_ = nullptr;
    ContentEditableController* contenteditable_controller_ = nullptr;
    EditorInputSession* editor_input_session_ = nullptr;
};

} // namespace mbink
