/**
 * @file test_input_handler.cpp
 * @brief InputHandler 单元测试
 */

#include <gtest/gtest.h>
#include "event/input_handler.h"

namespace lightui {
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

} // namespace test
} // namespace lightui
