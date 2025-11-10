/**
 * @file dom_observer.cpp
 * @brief DOM 观察者实现
 */

#include "dom_observer.h"
#include <algorithm>

namespace lightui {

void DOMObserverManager::AddObserver(DOMObserver* observer) {
    if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void DOMObserverManager::RemoveObserver(DOMObserver* observer) {
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void DOMObserverManager::RemoveAllObservers() {
    observers_.clear();
}

void DOMObserverManager::NotifyNodeAdded(Node* node, Node* parent) {
    for (auto observer : observers_) {
        observer->OnNodeAdded(node, parent);
    }
}

void DOMObserverManager::NotifyNodeRemoved(Node* node, Node* parent) {
    for (auto observer : observers_) {
        observer->OnNodeRemoved(node, parent);
    }
}

void DOMObserverManager::NotifyAttributeChanged(Element* element,
                                               const std::string& name,
                                               const std::string& old_value,
                                               const std::string& new_value) {
    for (auto observer : observers_) {
        observer->OnAttributeChanged(element, name, old_value, new_value);
    }
}

void DOMObserverManager::NotifyStyleChanged(Element* element,
                                           const std::string& property,
                                           const std::string& old_value,
                                           const std::string& new_value) {
    for (auto observer : observers_) {
        observer->OnStyleChanged(element, property, old_value, new_value);
    }
}

void DOMObserverManager::NotifyTextChanged(Node* node,
                                          const std::string& old_text,
                                          const std::string& new_text) {
    for (auto observer : observers_) {
        observer->OnTextChanged(node, old_text, new_text);
    }
}

void DOMObserverManager::NotifySubtreeModified(Node* root) {
    for (auto observer : observers_) {
        observer->OnSubtreeModified(root);
    }
}

} // namespace lightui

