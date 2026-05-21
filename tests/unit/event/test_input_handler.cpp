/**
 * @file test_input_handler.cpp
 * @brief InputHandler 单元测试
 */

#include <gtest/gtest.h>
#include "event/input/input_handler.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/event/input/hit_test_controller.h"
#include "core/event/dispatch/mouse_event_dispatcher.h"
#include "core/render/objects/render_object.h"
#include "core/window/window.h"

namespace mbink {
namespace test {

class InputHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler_ = std::make_unique<InputHandler>();
    }

    void TearDown() override {
        handler_.reset();
    }

protected:
    std::unique_ptr<InputHandler> handler_;
};

TEST_F(InputHandlerTest, InitialMousePosition) {
    int x, y;
    handler_->GetMousePosition(&x, &y);

    // 初始位置应该是 (0, 0) 或某个默认值
    EXPECT_GE(x, 0);
    EXPECT_GE(y, 0);
}

TEST_F(InputHandlerTest, HandleMouseMoveEvent) {
    SDL_Event event;
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 100.0f;
    event.motion.y = 200.0f;
    event.motion.windowID = 1;

    handler_->HandleSDLEvent(event);

    int x, y;
    handler_->GetMousePosition(&x, &y);

    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 200);
}

TEST_F(InputHandlerTest, MouseButtonState) {
    // 初始状态所有按钮都未按下
    EXPECT_FALSE(handler_->IsMouseButtonDown(MouseButton::LEFT));
    EXPECT_FALSE(handler_->IsMouseButtonDown(MouseButton::MIDDLE));
    EXPECT_FALSE(handler_->IsMouseButtonDown(MouseButton::RIGHT));
}

TEST_F(InputHandlerTest, HandleMouseButtonEvent) {
    SDL_Event event;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.windowID = 1;

    handler_->HandleSDLEvent(event);

    EXPECT_TRUE(handler_->IsMouseButtonDown(MouseButton::LEFT));

    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    handler_->HandleSDLEvent(event);

    EXPECT_FALSE(handler_->IsMouseButtonDown(MouseButton::LEFT));
}

TEST_F(InputHandlerTest, KeyState) {
    // 初始状态所有键都未按下
    EXPECT_FALSE(handler_->IsScancodeDown(SDL_SCANCODE_A));
    EXPECT_FALSE(handler_->IsScancodeDown(SDL_SCANCODE_SPACE));
}

TEST_F(InputHandlerTest, HandleKeyEvent) {
    SDL_Event event;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    event.key.scancode = SDL_SCANCODE_A;
    event.key.windowID = 1;
    event.key.repeat = false;

    handler_->HandleSDLEvent(event);

    EXPECT_TRUE(handler_->IsScancodeDown(SDL_SCANCODE_A));

    event.type = SDL_EVENT_KEY_UP;
    handler_->HandleSDLEvent(event);

    EXPECT_FALSE(handler_->IsScancodeDown(SDL_SCANCODE_A));
}

TEST_F(InputHandlerTest, MouseCallback) {
    bool callback_called = false;
    InputMouseEvent received_event;

    handler_->SetMouseCallback([&](const InputMouseEvent& e) {
        callback_called = true;
        received_event = e;
    });

    SDL_Event event;
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 100.0f;
    event.motion.y = 200.0f;
    event.motion.windowID = 1;

    handler_->HandleSDLEvent(event);

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_event.type, MouseEventType::MOVE);
    EXPECT_EQ(received_event.x, 100);
    EXPECT_EQ(received_event.y, 200);
}

TEST_F(InputHandlerTest, KeyboardCallback) {
    bool callback_called = false;
    KeyEvent received_event;

    handler_->SetKeyboardCallback([&](const KeyEvent& e) {
        callback_called = true;
        received_event = e;
    });

    SDL_Event event;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    event.key.scancode = SDL_SCANCODE_A;
    event.key.windowID = 1;
    event.key.repeat = false;

    handler_->HandleSDLEvent(event);

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_event.type, KeyEventType::DOWN);
    EXPECT_EQ(received_event.scancode, SDL_SCANCODE_A);
}

namespace {

std::shared_ptr<RenderObject> CreateHitTestBox(const std::shared_ptr<Element>& element,
                                               float x,
                                               float y,
                                               float width,
                                               float height) {
    auto render_object = std::make_shared<RenderObject>(RenderObjectType::BLOCK);
    render_object->SetNode(element);
    auto& layout = render_object->GetLayoutInfo();
    layout.x = x;
    layout.y = y;
    layout.width = width;
    layout.height = height;
    layout.is_laid_out = true;
    render_object->UpdateViewportBounds();
    return render_object;
}

}  // namespace

TEST(MouseEventDispatcherTest, DisabledFormControlSuppressesMouseEventsAndStates) {
    auto document = std::make_shared<Document>();
    document->Initialize();

    auto root_element = document->CreateElement("div");
    auto disabled_button = document->CreateElement("button");
    disabled_button->SetAttribute("disabled", "true");
    root_element->AppendChild(disabled_button);

    auto root_render = CreateHitTestBox(root_element, 0.0f, 0.0f, 300.0f, 200.0f);
    auto button_render = CreateHitTestBox(disabled_button, 20.0f, 20.0f, 120.0f, 40.0f);
    root_render->AppendChild(button_render);
    button_render->UpdateViewportBounds();

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    const float display_scale = window->GetDisplayScale();
    const float hit_x = 30.0f * display_scale;
    const float hit_y = 30.0f * display_scale;

    HitTestController hit_test_controller;
    auto hit_result = hit_test_controller.HitTest(root_render, 30.0f, 30.0f);
    ASSERT_TRUE(hit_result.IsValid());
    ASSERT_TRUE(hit_result.element);
    EXPECT_EQ(hit_result.element->GetTagName(), "button");

    int mouseover_count = 0;
    int mouseenter_count = 0;
    int mousemove_count = 0;
    int mousedown_count = 0;
    int mouseup_count = 0;
    int click_count = 0;
    int root_mouseover_count = 0;
    int root_mouseenter_count = 0;
    int root_mousemove_count = 0;
    disabled_button->AddEventListener("mouseover", [&](std::shared_ptr<Event>) { ++mouseover_count; });
    disabled_button->AddEventListener("mouseenter", [&](std::shared_ptr<Event>) { ++mouseenter_count; });
    disabled_button->AddEventListener("mousemove", [&](std::shared_ptr<Event>) { ++mousemove_count; });
    disabled_button->AddEventListener("mousedown", [&](std::shared_ptr<Event>) { ++mousedown_count; });
    disabled_button->AddEventListener("mouseup", [&](std::shared_ptr<Event>) { ++mouseup_count; });
    disabled_button->AddEventListener("click", [&](std::shared_ptr<Event>) { ++click_count; });
    root_element->AddEventListener("mouseover", [&](std::shared_ptr<Event>) { ++root_mouseover_count; });
    root_element->AddEventListener("mouseenter", [&](std::shared_ptr<Event>) { ++root_mouseenter_count; });
    root_element->AddEventListener("mousemove", [&](std::shared_ptr<Event>) { ++root_mousemove_count; });

    MouseEventDispatcher dispatcher;

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = 1;
    motion.motion.x = hit_x;
    motion.motion.y = hit_y;
    EXPECT_TRUE(dispatcher.HandleMouseEvent(motion, window, document, root_render));

    SDL_Event down{};
    down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.windowID = 1;
    down.button.button = SDL_BUTTON_LEFT;
    down.button.x = hit_x;
    down.button.y = hit_y;
    EXPECT_TRUE(dispatcher.HandleMouseEvent(down, window, document, root_render));

    SDL_Event up{};
    up.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.windowID = 1;
    up.button.button = SDL_BUTTON_LEFT;
    up.button.x = hit_x;
    up.button.y = hit_y;
    EXPECT_TRUE(dispatcher.HandleMouseEvent(up, window, document, root_render));

    EXPECT_EQ(mouseover_count, 0);
    EXPECT_EQ(mouseenter_count, 0);
    EXPECT_EQ(mousemove_count, 0);
    EXPECT_EQ(mousedown_count, 0);
    EXPECT_EQ(mouseup_count, 0);
    EXPECT_EQ(click_count, 0);
    EXPECT_FALSE(disabled_button->HasPseudoClass("hover"));
    EXPECT_FALSE(disabled_button->HasPseudoClass("active"));
    EXPECT_TRUE(root_element->HasPseudoClass("hover"));
    EXPECT_EQ(root_mouseover_count, 1);
    EXPECT_EQ(root_mouseenter_count, 1);
    EXPECT_EQ(root_mousemove_count, 0);
    EXPECT_EQ(dispatcher.GetHoverElement(), root_element);
}

} // namespace test
} // namespace mbink
