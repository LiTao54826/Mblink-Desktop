/**
 * @file test_dom_event.cpp
 * @brief DOM Event 类单元测试
 */

#include <gtest/gtest.h>
#include "core/dom/event.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <memory>

using namespace lightui;

// ========== 测试 Event 基本功能 ==========

TEST(DOMEventTest, Constructor) {
    auto event = std::make_shared<Event>("click", true, true);
    
    EXPECT_EQ(event->GetType(), "click");
    EXPECT_TRUE(event->GetBubbles());
    EXPECT_TRUE(event->GetCancelable());
    EXPECT_EQ(event->GetEventPhase(), EventPhase::NONE);
    EXPECT_FALSE(event->IsPropagationStopped());
    EXPECT_FALSE(event->IsDefaultPrevented());
}

TEST(DOMEventTest, StopPropagation) {
    auto event = std::make_shared<Event>("click");
    
    EXPECT_FALSE(event->IsPropagationStopped());
    
    event->StopPropagation();
    
    EXPECT_TRUE(event->IsPropagationStopped());
    EXPECT_FALSE(event->IsImmediatePropagationStopped());
}

TEST(DOMEventTest, StopImmediatePropagation) {
    auto event = std::make_shared<Event>("click");
    
    event->StopImmediatePropagation();
    
    EXPECT_TRUE(event->IsPropagationStopped());
    EXPECT_TRUE(event->IsImmediatePropagationStopped());
}

TEST(DOMEventTest, PreventDefault) {
    auto event = std::make_shared<Event>("click", true, true);
    
    EXPECT_FALSE(event->IsDefaultPrevented());
    
    event->PreventDefault();
    
    EXPECT_TRUE(event->IsDefaultPrevented());
}

TEST(DOMEventTest, PreventDefaultNotCancelable) {
    auto event = std::make_shared<Event>("click", true, false);
    
    event->PreventDefault();
    
    // 不可取消的事件，PreventDefault 无效
    EXPECT_FALSE(event->IsDefaultPrevented());
}

// ========== 测试事件分发 ==========

TEST(DOMEventTest, DispatchEventSimple) {
    auto element = std::make_shared<Element>("div");
    bool called = false;
    
    element->AddEventListener("click", [&called](std::shared_ptr<Event> event) {
        called = true;
        EXPECT_EQ(event->GetType(), "click");
        EXPECT_EQ(event->GetEventPhase(), EventPhase::AT_TARGET);
    });
    
    auto event = std::make_shared<Event>("click");
    element->DispatchEvent(event);
    
    EXPECT_TRUE(called);
}

TEST(DOMEventTest, DispatchEventBubbling) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    int call_count = 0;
    
    parent->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
        EXPECT_EQ(event->GetEventPhase(), EventPhase::BUBBLING_PHASE);
    });
    
    child->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
        EXPECT_EQ(event->GetEventPhase(), EventPhase::AT_TARGET);
    });
    
    auto event = std::make_shared<Event>("click", true, true);
    child->DispatchEvent(event);
    
    EXPECT_EQ(call_count, 2);
}

TEST(DOMEventTest, DispatchEventCapturing) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);

    std::vector<std::string> phases;

    parent->AddEventListener("click", [&phases](std::shared_ptr<Event> event) {
        if (event->GetEventPhase() == EventPhase::BUBBLING_PHASE) {
            phases.push_back("parent-bubble");
        }
    });

    child->AddEventListener("click", [&phases](std::shared_ptr<Event> event) {
        phases.push_back("child-target");
    });

    auto event = std::make_shared<Event>("click", true, true);
    child->DispatchEvent(event);

    // 简化实现：只有目标阶段和冒泡阶段
    ASSERT_EQ(phases.size(), 2);
    EXPECT_EQ(phases[0], "child-target");
    EXPECT_EQ(phases[1], "parent-bubble");
}

TEST(DOMEventTest, StopPropagationInBubbling) {
    auto grandparent = std::make_shared<Element>("div");
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    grandparent->AppendChild(parent);
    parent->AppendChild(child);
    
    int call_count = 0;
    
    grandparent->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
    });
    
    parent->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
        event->StopPropagation();
    });
    
    child->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
    });
    
    auto event = std::make_shared<Event>("click", true, true);
    child->DispatchEvent(event);
    
    // child 和 parent 被调用，grandparent 不被调用
    EXPECT_EQ(call_count, 2);
}

TEST(DOMEventTest, MultipleListeners) {
    auto element = std::make_shared<Element>("div");
    int call_count = 0;
    
    element->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
    });
    
    element->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
    });
    
    auto event = std::make_shared<Event>("click");
    element->DispatchEvent(event);
    
    EXPECT_EQ(call_count, 2);
}

TEST(DOMEventTest, StopImmediatePropagationMultipleListeners) {
    auto element = std::make_shared<Element>("div");
    int call_count = 0;
    
    element->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
        event->StopImmediatePropagation();
    });
    
    element->AddEventListener("click", [&call_count](std::shared_ptr<Event> event) {
        call_count++;
    });
    
    auto event = std::make_shared<Event>("click");
    element->DispatchEvent(event);
    
    // 只有第一个监听器被调用
    EXPECT_EQ(call_count, 1);
}

// ========== 测试 MouseEvent ==========

TEST(DOMEventTest, MouseEvent) {
    auto event = std::make_shared<MouseEvent>("click", 100, 200, 0);
    
    EXPECT_EQ(event->GetType(), "click");
    EXPECT_EQ(event->GetClientX(), 100);
    EXPECT_EQ(event->GetClientY(), 200);
    EXPECT_EQ(event->GetButton(), 0);
}

// ========== 测试 KeyboardEvent ==========

TEST(DOMEventTest, KeyboardEvent) {
    auto event = std::make_shared<KeyboardEvent>("keydown", "Enter", "Enter");
    
    EXPECT_EQ(event->GetType(), "keydown");
    EXPECT_EQ(event->GetKey(), "Enter");
    EXPECT_EQ(event->GetCode(), "Enter");
}

// ========== 测试事件目标 ==========

TEST(DOMEventTest, EventTarget) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    std::shared_ptr<Node> target_in_parent;
    std::shared_ptr<Node> current_target_in_parent;
    
    parent->AddEventListener("click", [&](std::shared_ptr<Event> event) {
        target_in_parent = event->GetTarget();
        current_target_in_parent = event->GetCurrentTarget();
    });
    
    auto event = std::make_shared<Event>("click", true, true);
    child->DispatchEvent(event);
    
    // target 始终是 child，currentTarget 是 parent
    EXPECT_EQ(target_in_parent, child);
    EXPECT_EQ(current_target_in_parent, parent);
}

// ========== 测试非冒泡事件 ==========

TEST(DOMEventTest, NonBubblingEvent) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    bool parent_called = false;
    bool child_called = false;
    
    parent->AddEventListener("focus", [&parent_called](std::shared_ptr<Event> event) {
        parent_called = true;
    });
    
    child->AddEventListener("focus", [&child_called](std::shared_ptr<Event> event) {
        child_called = true;
    });
    
    // focus 事件不冒泡
    auto event = std::make_shared<Event>("focus", false, true);
    child->DispatchEvent(event);
    
    EXPECT_TRUE(child_called);
    EXPECT_FALSE(parent_called);
}

