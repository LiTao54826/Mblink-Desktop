/**
 * @file focus_manager.h
 * @brief 焦点管理器
 *
 * 参考：RmlUi/Source/Core/Context.cpp - OnFocusChange
 * 参考：Chrome/Blink - Document::SetFocusedElement
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "core/dom/observers/dom_observer.h"

namespace mblink {

// 前向声明
class Element;
class Document;
class Window;
class Node;
class EditorInputSession;

/**
 * @brief 焦点管理器
 *
 * 负责管理元素焦点状态，支持：
 * - Focus()/Blur()方法
 * - Tab键导航（tabindex支持）
 * - focus/blur事件自动发送
 * - :focus/:focus-visible伪类自动设置
 *
 * 参考：RmlUi的焦点管理机制
 * 参考：Chrome/Blink - 焦点元素被移除时自动清除焦点
 */
class FocusManager : public DOMObserver {
public:
    FocusManager();
    ~FocusManager();

    /**
     * @brief 设置窗口（用于SDL文本输入）
     * @param window 窗口指针
     */

    void SetEditorInputSession(EditorInputSession* session) { editor_input_session_ = session; }
    void SetWindow(Window* window) { window_ = window; }

    /**
     * @brief 刷新当前焦点元素的 IME 文本输入区域
     *
     * 将可编辑控件的可见输入区域与插入点位置同步给 SDL，
     * 使平台输入法候选窗能够贴近当前光标显示。
     */
    void UpdateTextInputArea();

    /**
     * @brief 设置焦点到指定元素
     * @param element 要获得焦点的元素
     * @param focus_visible 是否显示焦点指示器（键盘导航时为true）
     * @return true表示成功设置焦点
     */
    bool SetFocus(std::shared_ptr<Element> element, bool focus_visible = false);

    /**
     * @brief 获取焦点请求序号
     *
     * 每次显式 focus/blur/clear 都会递增，用于鼠标事件收尾时判断 JS 是否已经处理焦点。
     */
    uint64_t GetFocusChangeSerial() const { return focus_change_serial_; }

    /**
     * @brief 移除焦点
     * @param element 要失去焦点的元素
     */
    void Blur(std::shared_ptr<Element> element);

    /**
     * @brief 获取当前焦点元素
     * @return 当前焦点元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> GetFocusElement() const;

    /**
     * @brief Tab键导航到下一个可聚焦元素
     * @param current_document 当前文档
     * @param reverse 是否反向导航（Shift+Tab）
     * @return true表示成功导航
     */
    bool TabToNextFocusableElement(std::shared_ptr<Document> current_document, bool reverse = false);

    /**
     * @brief 清除焦点（通常在文档卸载时调用）
     */
    void ClearFocus();

    // ========== DOMObserver 接口 ==========

    /**
     * @brief 节点被移除时调用
     *
     * 如果被移除的节点是焦点元素或包含焦点元素，则清除焦点
     * 参考 Chrome/Blink 行为
     */
    void OnNodeRemoved(Node* node, Node* parent) override;

    /**
     * @brief 处理autofocus属性（在文档加载完成时调用）
     * @param document 文档
     * @return true表示找到并聚焦了autofocus元素
     *
     * 查找第一个有autofocus属性的可聚焦元素并设置焦点
     */
    bool ProcessAutofocus(std::shared_ptr<Document> document);

    /**
     * @brief 查找可聚焦的元素（从当前元素向上查找）
     * @param element 起始元素
     * @return 可聚焦的元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> FindFocusableElement(std::shared_ptr<Element> element);

    /**
     * @brief 检查元素是否可聚焦
     * @param element 要检查的元素
     * @return true表示可聚焦
     */
    bool IsFocusable(std::shared_ptr<Element> element);

private:

    /**
     * @brief 收集所有可聚焦元素（按tabindex排序）
     * @param root 根元素
     * @param focusable_elements 输出：可聚焦元素列表
     */
    void CollectFocusableElements(std::shared_ptr<Element> root,
                                  std::vector<std::shared_ptr<Element>>& focusable_elements);

    /**
     * @brief 获取元素的tabindex
     * @param element 元素
     * @return tabindex值，-1表示不可通过Tab导航
     */
    int GetTabIndex(std::shared_ptr<Element> element);

    /**
     * @brief 发送焦点变化事件
     * @param old_focus 旧焦点元素
     * @param new_focus 新焦点元素
     * @param focus_visible 是否显示焦点指示器
     */
    void SendFocusEvents(std::shared_ptr<Element> old_focus,
                        std::shared_ptr<Element> new_focus,
                        bool focus_visible);

    /**
     * @brief 注册到文档的观察者管理器
     * @param document 要注册的文档
     */
    void RegisterWithDocument(std::shared_ptr<Document> document);

    /**
     * @brief 从当前文档的观察者管理器注销
     */
    void UnregisterFromDocument();

private:
    // 当前焦点元素（弱引用，避免循环引用）
    std::weak_ptr<Element> focus_element_;

    uint64_t focus_change_serial_ = 0;

    // 窗口指针（用于SDL文本输入）
    Window* window_ = nullptr;

    EditorInputSession* editor_input_session_ = nullptr;

    // 当前注册的文档（用于自动注销）
    std::weak_ptr<Document> registered_document_;
};

} // namespace mblink

