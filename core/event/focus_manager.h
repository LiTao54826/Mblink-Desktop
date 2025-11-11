/**
 * @file focus_manager.h
 * @brief 焦点管理器
 * 
 * 参考：RmlUi/Source/Core/Context.cpp - OnFocusChange
 */

#pragma once

#include <memory>
#include <vector>

namespace lightui {

// 前向声明
class Element;
class Document;

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
 */
class FocusManager {
public:
    FocusManager();
    ~FocusManager();

    /**
     * @brief 设置焦点到指定元素
     * @param element 要获得焦点的元素
     * @param focus_visible 是否显示焦点指示器（键盘导航时为true）
     * @return true表示成功设置焦点
     */
    bool SetFocus(std::shared_ptr<Element> element, bool focus_visible = false);

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

    /**
     * @brief 处理autofocus属性（在文档加载完成时调用）
     * @param document 文档
     * @return true表示找到并聚焦了autofocus元素
     *
     * 查找第一个有autofocus属性的可聚焦元素并设置焦点
     */
    bool ProcessAutofocus(std::shared_ptr<Document> document);

private:
    /**
     * @brief 查找可聚焦的元素
     * @param element 起始元素
     * @return 可聚焦的元素，如果没有则返回nullptr
     */
    std::shared_ptr<Element> FindFocusableElement(std::shared_ptr<Element> element);

    /**
     * @brief 收集所有可聚焦元素（按tabindex排序）
     * @param root 根元素
     * @param focusable_elements 输出：可聚焦元素列表
     */
    void CollectFocusableElements(std::shared_ptr<Element> root, 
                                  std::vector<std::shared_ptr<Element>>& focusable_elements);

    /**
     * @brief 检查元素是否可聚焦
     * @param element 要检查的元素
     * @return true表示可聚焦
     */
    bool IsFocusable(std::shared_ptr<Element> element);

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

private:
    // 当前焦点元素（弱引用，避免循环引用）
    std::weak_ptr<Element> focus_element_;
};

} // namespace lightui

