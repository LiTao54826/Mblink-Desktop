/**
 * @file window_dom_observer.h
 * @brief Window 的 DOM 观察者
 * 
 * 从 window.cpp 提取的 DOM 观察者类。
 * 监听 DOM 变化并触发窗口重绘。
 */

#ifndef LIGHTUI_WINDOW_DOM_OBSERVER_H
#define LIGHTUI_WINDOW_DOM_OBSERVER_H

#include "core/dom/dom_observer.h"
#include <memory>
#include <string>

namespace lightui {

class Window;
class Node;
class Element;

/**
 * @brief Window 的 DOM 观察者
 *
 * 监听 DOM 变化并触发窗口重绘
 */
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window);

    void OnNodeAdded(Node* node, Node* parent) override;
    void OnNodeRemoved(Node* node, Node* parent) override;
    void OnAttributeChanged(Element* element,
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override;
    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override;
    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override;
    void OnSubtreeModified(Node* root) override;
    void OnPseudoClassChanged(std::shared_ptr<Element> element,
                             const std::string& pseudo_class,
                             bool activate) override;

private:
    /**
     * @brief 检查节点是否在批量更新中
     */
    bool IsInBatch(Node* node) const;

    Window* window_;
};

}  // namespace lightui

#endif  // LIGHTUI_WINDOW_DOM_OBSERVER_H
