/**
 * @file mock_objects.h
 * @brief 测试用 Mock 对象
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "core/dom/element.h"
#include "core/dom/event.h"
#include "core/dom/observers/dom_observer.h"

namespace mbink {
namespace test {

/**
 * @brief Mock 事件监听器
 *
 * 记录事件调用信息，用于验证事件分发
 */
class MockEventListener {
public:
    MockEventListener() = default;

    // 获取监听器函数
    EventListener GetListener();

    // 检查是否被调用
    bool WasCalled() const { return call_count_ > 0; }

    // 获取调用次数
    int GetCallCount() const { return call_count_; }

    // 获取最后一次事件
    std::shared_ptr<Event> GetLastEvent() const { return last_event_; }

    // 获取所有事件
    const std::vector<std::shared_ptr<Event>>& GetAllEvents() const { return events_; }

    // 重置状态
    void Reset();

private:
    int call_count_ = 0;
    std::shared_ptr<Event> last_event_;
    std::vector<std::shared_ptr<Event>> events_;
};

/**
 * @brief Mock DOM 观察者
 *
 * 记录 DOM 变化，用于验证 DOM 操作
 */
class MockDOMObserver : public DOMObserver {
public:
    void OnNodeAdded(Node* node, Node* parent) override;
    void OnNodeRemoved(Node* node, Node* parent) override;
    void OnAttributeChanged(Element* element,
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override;
    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override;
    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override;
    void OnSubtreeModified(Node* root) override;

    // 检查方法
    int GetAddCount() const { return add_count_; }
    int GetRemoveCount() const { return remove_count_; }
    int GetAttributeChangeCount() const { return attr_change_count_; }
    int GetTextChangeCount() const { return text_change_count_; }
    int GetStyleChangeCount() const { return style_change_count_; }
    int GetSubtreeModifiedCount() const { return subtree_modified_count_; }

    // 重置
    void Reset();

private:
    int add_count_ = 0;
    int remove_count_ = 0;
    int attr_change_count_ = 0;
    int text_change_count_ = 0;
    int style_change_count_ = 0;
    int subtree_modified_count_ = 0;
};

} // namespace test
} // namespace mbink
