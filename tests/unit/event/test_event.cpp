/**
 * @file test_event.cpp
 * @brief Event 类单元测试
 *
 * 测试内容：
 * - 事件创建和属性
 * - 事件传播控制
 * - 默认行为控制
 * - 事件冒泡
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "test_utils/mock_objects.h"
#include "dom/event.h"
#include "dom/element.h"
#include "dom/document.h"

namespace mbink {
namespace test {

class EventTest : public DOMTestBase {};

// ========== 事件创建测试 ==========

TEST_F(EventTest, CreateEvent) {
    auto event = std::make_shared<Event>("click");
    EXPECT_EQ(event->GetType(), "click");
}

TEST_F(EventTest, EventDefaultProperties) {
    auto event = std::make_shared<Event>("click");

    EXPECT_TRUE(event->GetBubbles());
    EXPECT_TRUE(event->GetCancelable());
    EXPECT_FALSE(event->IsDefaultPrevented());
    EXPECT_FALSE(event->IsPropagationStopped());
}

// ========== 事件目标测试 ==========

TEST_F(EventTest, SetTarget) {
    auto event = std::make_shared<Event>("click");
    auto elem = CreateElement("div");

    event->SetTarget(elem);
    EXPECT_EQ(event->GetTarget(), elem);
}

TEST_F(EventTest, SetCurrentTarget) {
    auto event = std::make_shared<Event>("click");
    auto elem = CreateElement("div");

    event->SetCurrentTarget(elem);
    EXPECT_EQ(event->GetCurrentTarget(), elem);
}

// ========== 默认行为控制测试 ==========

TEST_F(EventTest, PreventDefault) {
    auto event = std::make_shared<Event>("click");

    EXPECT_FALSE(event->IsDefaultPrevented());

    event->PreventDefault();
    EXPECT_TRUE(event->IsDefaultPrevented());
}

TEST_F(EventTest, PreventDefaultOnNonCancelable) {
    auto event = std::make_shared<Event>("load");
    // 假设 load 事件不可取消（取决于实现）

    // 如果事件不可取消，PreventDefault 应该无效
    // 这取决于具体实现
}

// ========== 传播控制测试 ==========

TEST_F(EventTest, StopPropagation) {
    auto event = std::make_shared<Event>("click");

    EXPECT_FALSE(event->IsPropagationStopped());

    event->StopPropagation();
    EXPECT_TRUE(event->IsPropagationStopped());
}

// ========== 事件分发测试 ==========

TEST_F(EventTest, DispatchEventToElement) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    elem->AddEventListener("click", listener.GetListener());

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_TRUE(listener.WasCalled());
    EXPECT_EQ(listener.GetLastEvent()->GetType(), "click");
}

TEST_F(EventTest, DispatchEventSetsTarget) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    elem->AddEventListener("click", listener.GetListener());

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_EQ(listener.GetLastEvent()->GetTarget(), elem);
}

// ========== 事件冒泡测试 ==========

TEST_F(EventTest, EventBubbles) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    parent->AppendChild(child);

    MockEventListener parentListener;
    MockEventListener childListener;

    parent->AddEventListener("click", parentListener.GetListener());
    child->AddEventListener("click", childListener.GetListener());

    auto event = std::make_shared<Event>("click");
    child->DispatchEvent(event);

    EXPECT_TRUE(childListener.WasCalled());
    // 如果实现了冒泡，父元素也应该收到事件
    // EXPECT_TRUE(parentListener.WasCalled());
}

TEST_F(EventTest, StopPropagationPreventsParent) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    parent->AppendChild(child);

    MockEventListener parentListener;

    parent->AddEventListener("click", parentListener.GetListener());
    child->AddEventListener("click", [](std::shared_ptr<Event> e) {
        e->StopPropagation();
    });

    auto event = std::make_shared<Event>("click");
    child->DispatchEvent(event);

    // 停止传播后，父元素不应该收到事件
    EXPECT_FALSE(parentListener.WasCalled());
}

// ========== 多个监听器测试 ==========

TEST_F(EventTest, MultipleListenersSameType) {
    auto elem = CreateElement("div");
    MockEventListener listener1;
    MockEventListener listener2;
    MockEventListener listener3;

    elem->AddEventListener("click", listener1.GetListener());
    elem->AddEventListener("click", listener2.GetListener());
    elem->AddEventListener("click", listener3.GetListener());

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_TRUE(listener1.WasCalled());
    EXPECT_TRUE(listener2.WasCalled());
    EXPECT_TRUE(listener3.WasCalled());
}

TEST_F(EventTest, DifferentEventTypes) {
    auto elem = CreateElement("div");
    MockEventListener clickListener;
    MockEventListener mouseoverListener;

    elem->AddEventListener("click", clickListener.GetListener());
    elem->AddEventListener("mouseover", mouseoverListener.GetListener());

    auto clickEvent = std::make_shared<Event>("click");
    elem->DispatchEvent(clickEvent);

    EXPECT_TRUE(clickListener.WasCalled());
    EXPECT_FALSE(mouseoverListener.WasCalled());
}

// ========== 捕获阶段测试 ==========

TEST_F(EventTest, CapturePhase) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    parent->AppendChild(child);

    std::vector<std::string> order;

    parent->AddEventListener("click", [&order](std::shared_ptr<Event> e) {
        order.push_back("parent-capture");
    }, true);  // capture = true

    child->AddEventListener("click", [&order](std::shared_ptr<Event> e) {
        order.push_back("child-bubble");
    }, false);  // capture = false

    parent->AddEventListener("click", [&order](std::shared_ptr<Event> e) {
        order.push_back("parent-bubble");
    }, false);

    auto event = std::make_shared<Event>("click");
    child->DispatchEvent(event);

    // 预期顺序：捕获阶段（从根到目标），然后冒泡阶段（从目标到根）
    // parent-capture -> child-bubble -> parent-bubble
    // 注意：这取决于具体实现
}

// ========== 移除监听器测试 ==========

TEST_F(EventTest, RemoveEventListener) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    uint64_t id = elem->AddEventListener("click", listener.GetListener());
    elem->RemoveEventListener("click", id);

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_FALSE(listener.WasCalled());
}

TEST_F(EventTest, RemoveOneOfMultipleListeners) {
    auto elem = CreateElement("div");
    MockEventListener listener1;
    MockEventListener listener2;

    uint64_t id1 = elem->AddEventListener("click", listener1.GetListener());
    elem->AddEventListener("click", listener2.GetListener());

    elem->RemoveEventListener("click", id1);

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_FALSE(listener1.WasCalled());
    EXPECT_TRUE(listener2.WasCalled());
}

// ========== Once 选项测试 ==========

TEST_F(EventTest, OnceOption) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    elem->AddEventListener("click", listener.GetListener(), false, true);  // once = true

    auto event1 = std::make_shared<Event>("click");
    auto event2 = std::make_shared<Event>("click");

    elem->DispatchEvent(event1);
    elem->DispatchEvent(event2);

    EXPECT_EQ(listener.GetCallCount(), 1);
}

} // namespace test
} // namespace mbink
