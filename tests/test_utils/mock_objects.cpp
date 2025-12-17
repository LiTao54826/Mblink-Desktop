/**
 * @file mock_objects.cpp
 * @brief Mock 对象实现
 */

#include "mock_objects.h"

namespace lightui {
namespace test {

// ========== MockEventListener ==========

EventListener MockEventListener::GetListener() {
    return [this](std::shared_ptr<Event> event) {
        call_count_++;
        last_event_ = event;
        events_.push_back(event);
    };
}

void MockEventListener::Reset() {
    call_count_ = 0;
    last_event_.reset();
    events_.clear();
}

// ========== MockDOMObserver ==========

void MockDOMObserver::OnNodeAdded(Node* node, Node* parent) {
    add_count_++;
}

void MockDOMObserver::OnNodeRemoved(Node* node, Node* parent) {
    remove_count_++;
}

void MockDOMObserver::OnAttributeChanged(Element* element,
                                         const std::string& name,
                                         const std::string& old_value,
                                         const std::string& new_value) {
    attr_change_count_++;
}

void MockDOMObserver::OnTextChanged(Node* node,
                                    const std::string& old_text,
                                    const std::string& new_text) {
    text_change_count_++;
}

void MockDOMObserver::OnStyleChanged(Element* element,
                                     const std::string& property,
                                     const std::string& old_value,
                                     const std::string& new_value) {
    style_change_count_++;
}

void MockDOMObserver::OnSubtreeModified(Node* root) {
    subtree_modified_count_++;
}

void MockDOMObserver::Reset() {
    add_count_ = 0;
    remove_count_ = 0;
    attr_change_count_ = 0;
    text_change_count_ = 0;
    style_change_count_ = 0;
    subtree_modified_count_ = 0;
}

} // namespace test
} // namespace lightui
