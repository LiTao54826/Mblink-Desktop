/**
 * @file test_event_dispatch.cpp
 * @brief 事件分发集成测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"
#include "dom/event.h"

namespace lightui {
namespace test {

class EventDispatchTest : public DOMTestBase {};

// ========== 基本事件测试 ==========

TEST_F(EventDispatchTest, EventCreation) {
    Event event("click");
    
    EXPECT_EQ(event.GetType(), "click");
    EXPECT_TRUE(event.GetBubbles());
    EXPECT_TRUE(event.GetCancelable());
}

TEST_F(EventDispatchTest, EventNoBubbles) {
    Event event("focus", false, true);
    
    EXPECT_EQ(event.GetType(), "focus");
    EXPECT_FALSE(event.GetBubbles());
}

TEST_F(EventDispatchTest, StopPropagation) {
    Event event("click");
    
    EXPECT_FALSE(event.IsPropagationStopped());
    event.StopPropagation();
    EXPECT_TRUE(event.IsPropagationStopped());
}

TEST_F(EventDispatchTest, PreventDefault) {
    Event event("click");
    
    EXPECT_FALSE(event.IsDefaultPrevented());
    event.PreventDefault();
    EXPECT_TRUE(event.IsDefaultPrevented());
}

TEST_F(EventDispatchTest, StopImmediatePropagation) {
    Event event("click");
    
    EXPECT_FALSE(event.IsImmediatePropagationStopped());
    event.StopImmediatePropagation();
    EXPECT_TRUE(event.IsImmediatePropagationStopped());
}

// ========== 事件监听器测试 ==========

TEST_F(EventDispatchTest, AddEventListener) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    bool called = false;
    elem->AddEventListener("click", [&called](std::shared_ptr<Event> e) {
        called = true;
    });
    
    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);
    
    EXPECT_TRUE(called);
}

TEST_F(EventDispatchTest, MultipleListeners) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    int count = 0;
    elem->AddEventListener("click", [&count](std::shared_ptr<Event> e) { count++; });
    elem->AddEventListener("click", [&count](std::shared_ptr<Event> e) { count++; });
    elem->AddEventListener("click", [&count](std::shared_ptr<Event> e) { count++; });
    
    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);
    
    EXPECT_EQ(count, 3);
}

TEST_F(EventDispatchTest, RemoveEventListener) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    int count = 0;
    auto id = elem->AddEventListener("click", [&count](std::shared_ptr<Event> e) {
        count++;
    });
    
    // 第一次触发
    elem->DispatchEvent(std::make_shared<Event>("click"));
    EXPECT_EQ(count, 1);
    
    // 移除监听器
    elem->RemoveEventListener("click", id);
    
    // 第二次触发
    elem->DispatchEvent(std::make_shared<Event>("click"));
    EXPECT_EQ(count, 1);  // 不应该增加
}

TEST_F(EventDispatchTest, OnceOption) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    int count = 0;
    elem->AddEventListener("click", [&count](std::shared_ptr<Event> e) {
        count++;
    }, false, true);  // once = true
    
    // 多次触发
    elem->DispatchEvent(std::make_shared<Event>("click"));
    elem->DispatchEvent(std::make_shared<Event>("click"));
    elem->DispatchEvent(std::make_shared<Event>("click"));
    
    // 只应该执行一次
    EXPECT_EQ(count, 1);
}

TEST_F(EventDispatchTest, DifferentEventTypes) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    bool clickCalled = false;
    bool mouseoverCalled = false;
    
    elem->AddEventListener("click", [&clickCalled](std::shared_ptr<Event> e) {
        clickCalled = true;
    });
    
    elem->AddEventListener("mouseover", [&mouseoverCalled](std::shared_ptr<Event> e) {
        mouseoverCalled = true;
    });
    
    // 只触发 click
    elem->DispatchEvent(std::make_shared<Event>("click"));
    
    EXPECT_TRUE(clickCalled);
    EXPECT_FALSE(mouseoverCalled);
}

// ========== 鼠标事件测试 ==========

TEST_F(EventDispatchTest, MouseEventCreation) {
    MouseEvent event("click", 100, 200, 0);
    
    EXPECT_EQ(event.GetType(), "click");
    EXPECT_EQ(event.GetClientX(), 100);
    EXPECT_EQ(event.GetClientY(), 200);
    EXPECT_EQ(event.GetButton(), 0);
}

// ========== 键盘事件测试 ==========

TEST_F(EventDispatchTest, KeyboardEventCreation) {
    KeyboardEvent event("keydown", "a", "KeyA", 65, true, false, false, false, false);
    
    EXPECT_EQ(event.GetType(), "keydown");
    EXPECT_EQ(event.GetKey(), "a");
    EXPECT_EQ(event.GetCode(), "KeyA");
    EXPECT_EQ(event.GetKeyCode(), 65);
    EXPECT_TRUE(event.GetCtrlKey());
    EXPECT_FALSE(event.GetShiftKey());
}

TEST_F(EventDispatchTest, KeyboardEventModifierState) {
    KeyboardEvent event("keydown", "a", "KeyA", 65, true, true, false, false, false);
    
    EXPECT_TRUE(event.GetModifierState("Control"));
    EXPECT_TRUE(event.GetModifierState("Shift"));
    EXPECT_FALSE(event.GetModifierState("Alt"));
    EXPECT_FALSE(event.GetModifierState("Meta"));
}

// ========== 动画事件测试 ==========

TEST_F(EventDispatchTest, AnimationEventCreation) {
    AnimationEvent event("animationend", "slide-in", 0.5f, "::before");
    
    EXPECT_EQ(event.GetType(), "animationend");
    EXPECT_EQ(event.GetAnimationName(), "slide-in");
    EXPECT_FLOAT_EQ(event.GetElapsedTime(), 0.5f);
    EXPECT_EQ(event.GetPseudoElement(), "::before");
}

// ========== 自定义事件测试 ==========

TEST_F(EventDispatchTest, CustomEvent) {
    auto doc = CreateDocument();
    auto elem = doc->CreateElement("div");
    
    bool customEventReceived = false;
    
    elem->AddEventListener("myCustomEvent", [&customEventReceived](std::shared_ptr<Event> e) {
        customEventReceived = true;
    });
    
    auto customEvent = std::make_shared<Event>("myCustomEvent");
    elem->DispatchEvent(customEvent);
    
    EXPECT_TRUE(customEventReceived);
}

} // namespace test
} // namespace lightui
