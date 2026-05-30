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
#include "core/event/dispatch/wheel_event_dispatcher.h"
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

void UpdateViewportBoundsRecursive(const std::shared_ptr<RenderObject>& object) {
    if (!object) {
        return;
    }

    object->UpdateViewportBounds();
    for (const auto& child : object->GetChildren()) {
        UpdateViewportBoundsRecursive(child);
    }
}

std::shared_ptr<RenderObject> FindRenderObjectById(
    const std::shared_ptr<RenderObject>& object,
    const std::string& id) {
    if (!object) {
        return nullptr;
    }

    auto element = std::dynamic_pointer_cast<Element>(object->GetNode());
    if (element && element->GetAttribute("id") == id) {
        return object;
    }

    for (const auto& child : object->GetChildren()) {
        auto found = FindRenderObjectById(child, id);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

SDL_Event CreateWheelEvent(std::shared_ptr<Window> window,
                           float x,
                           float y,
                           float wheel_y) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.windowID = window && window->GetSDLWindow() ?
        SDL_GetWindowID(window->GetSDLWindow()) : 1;
    event.wheel.mouse_x = x * (window ? window->GetDisplayScale() : 1.0f);
    event.wheel.mouse_y = y * (window ? window->GetDisplayScale() : 1.0f);
    event.wheel.x = 0.0f;
    event.wheel.y = wheel_y;
    return event;
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

TEST(MouseEventDispatcherTest, FixedModalHorizontalScrollbarConsumesMouseDown) {
    auto document = std::make_shared<Document>();
    document->Initialize();

    auto body = document->GetBody();
    ASSERT_TRUE(body);

    auto underlying_button = document->CreateElement("button");
    auto overlay = document->CreateElement("div");
    auto dialog = document->CreateElement("div");
    auto scroller = document->CreateElement("div");
    auto content = document->CreateElement("div");

    body->AppendChild(underlying_button);
    body->AppendChild(overlay);
    overlay->AppendChild(dialog);
    dialog->AppendChild(scroller);
    scroller->AppendChild(content);

    auto root_render = CreateHitTestBox(body, 0.0f, 0.0f, 800.0f, 600.0f);
    auto button_render = CreateHitTestBox(underlying_button, 100.0f, 412.0f, 220.0f, 40.0f);
    auto overlay_render = CreateHitTestBox(overlay, 0.0f, 0.0f, 800.0f, 600.0f);
    auto dialog_render = CreateHitTestBox(dialog, 80.0f, 80.0f, 640.0f, 420.0f);
    auto scroller_render = CreateHitTestBox(scroller, 20.0f, 80.0f, 580.0f, 260.0f);
    auto content_render = CreateHitTestBox(content, 0.0f, 0.0f, 1120.0f, 600.0f);

    overlay_render->GetComputedStyle().position = "fixed";
    overlay_render->GetComputedStyle().z_index = 99999;
    dialog_render->GetComputedStyle().overflow = "hidden";
    scroller_render->GetComputedStyle().overflow_x = "auto";
    scroller_render->GetComputedStyle().overflow_y = "auto";
    scroller_render->SetContentSize(1120.0f, 600.0f);

    root_render->AppendChild(button_render);
    root_render->AppendChild(overlay_render);
    overlay_render->AppendChild(dialog_render);
    dialog_render->AppendChild(scroller_render);
    scroller_render->AppendChild(content_render);
    UpdateViewportBoundsRecursive(root_render);

    int underlying_mousedown_count = 0;
    underlying_button->AddEventListener("mousedown", [&](std::shared_ptr<Event>) {
        ++underlying_mousedown_count;
    });

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);

    const float display_scale = window->GetDisplayScale();
    const float scroller_viewport_x = 100.0f;
    const float scroller_viewport_y = 160.0f;
    const float hit_x = scroller_viewport_x + 40.0f;
    const float hit_y = scroller_viewport_y + 260.0f - RenderObject::GetScrollbarWidth() * 0.5f;

    HitTestController hit_test_controller;
    auto hit_result = hit_test_controller.HitTest(root_render, hit_x, hit_y);
    ASSERT_TRUE(hit_result.IsValid());
    ASSERT_TRUE(hit_result.render_object);
    auto hit_ancestor = hit_result.render_object;
    bool hit_inside_scroller = false;
    while (hit_ancestor) {
        if (hit_ancestor == scroller_render) {
            hit_inside_scroller = true;
            break;
        }
        hit_ancestor = hit_ancestor->GetParent();
    }
    EXPECT_TRUE(hit_inside_scroller);

    MouseEventDispatcher dispatcher;
    SDL_Event down{};
    down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.windowID = 1;
    down.button.button = SDL_BUTTON_LEFT;
    down.button.x = hit_x * display_scale;
    down.button.y = hit_y * display_scale;

    EXPECT_TRUE(dispatcher.HandleMouseEvent(down, window, document, root_render));
    EXPECT_EQ(dispatcher.GetScrollbarDraggingElement(), scroller_render);
    EXPECT_EQ(scroller_render->GetDraggingScrollbar(), RenderObject::ScrollbarHitArea::HorizontalTrack);
    EXPECT_EQ(underlying_mousedown_count, 0);

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = 1;
    motion.motion.x = (hit_x + 80.0f) * display_scale;
    motion.motion.y = hit_y * display_scale;

    EXPECT_TRUE(dispatcher.HandleMouseEvent(motion, window, document, root_render));
    EXPECT_GT(scroller_render->GetScrollX(), 0.0f);
}

TEST(WheelEventDispatcherTest, FixedModalConsumesWheelOutsideScrollableArea) {
    auto document = std::make_shared<Document>();
    document->Initialize();

    auto body = document->GetBody();
    ASSERT_TRUE(body);

    body->SetStyle("margin", "0");
    body->SetStyle("overflow-y", "auto");

    auto page_content = document->CreateElement("div");
    page_content->SetStyle("height", "1800px");
    page_content->AppendChild(document->CreateTextNode("page content"));

    auto overlay = document->CreateElement("div");
    overlay->SetAttribute("id", "overlay");
    overlay->SetStyle("position", "fixed");
    overlay->SetStyle("left", "0px");
    overlay->SetStyle("top", "0px");
    overlay->SetStyle("width", "800px");
    overlay->SetStyle("height", "600px");
    overlay->SetStyle("z-index", "9000");

    auto dialog = document->CreateElement("div");
    dialog->SetAttribute("id", "dialog");
    dialog->SetStyle("position", "fixed");
    dialog->SetStyle("left", "100px");
    dialog->SetStyle("top", "100px");
    dialog->SetStyle("width", "520px");
    dialog->SetStyle("height", "360px");
    dialog->SetStyle("z-index", "9001");
    dialog->SetStyle("overflow", "hidden");

    auto header = document->CreateElement("button");
    header->SetAttribute("id", "dialog-header");
    header->SetStyle("height", "56px");
    header->SetStyle("width", "160px");
    header->AppendChild(document->CreateTextNode("Close"));

    auto scroller = document->CreateElement("div");
    scroller->SetAttribute("id", "modal-scroller");
    scroller->SetStyle("width", "480px");
    scroller->SetStyle("height", "180px");
    scroller->SetStyle("overflow-y", "auto");

    auto modal_content = document->CreateElement("div");
    modal_content->SetStyle("height", "780px");
    modal_content->AppendChild(document->CreateTextNode("modal content"));

    body->AppendChild(page_content);
    body->AppendChild(overlay);
    body->AppendChild(dialog);
    dialog->AppendChild(header);
    dialog->AppendChild(scroller);
    scroller->AppendChild(modal_content);

    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    window->Render();

    auto root_render = window->GetCachedRenderTree();
    ASSERT_TRUE(root_render);
    auto body_render = body->GetRenderObject();
    auto scroller_render = FindRenderObjectById(root_render, "modal-scroller");
    ASSERT_TRUE(body_render);
    ASSERT_TRUE(scroller_render);
    scroller_render->SetContentSize(480.0f, 780.0f);

    ASSERT_GT(body_render->GetMaxScrollY(), 0.0f);
    ASSERT_GT(scroller_render->GetMaxScrollY(), 0.0f);

    WheelEventDispatcher dispatcher;

    if (!scroller_render->GetViewportBounds().valid) {
        scroller_render->UpdateViewportBounds();
    }

    const auto& scroller_bounds = scroller_render->GetViewportBounds();
    ASSERT_TRUE(scroller_bounds.valid);

    HitTestController hit_test_controller;
    auto scroller_hit = hit_test_controller.HitTest(
        root_render, scroller_bounds.x + 12.0f, scroller_bounds.y + 12.0f);
    ASSERT_TRUE(scroller_hit.IsValid());
    ASSERT_TRUE(scroller_hit.render_object);
    auto hit_ancestor = scroller_hit.render_object;
    bool hit_inside_scroller = false;
    while (hit_ancestor) {
        if (hit_ancestor == scroller_render) {
            hit_inside_scroller = true;
            break;
        }
        hit_ancestor = hit_ancestor->GetParent();
    }
    if (!hit_inside_scroller) {
        auto hit_element = std::dynamic_pointer_cast<Element>(scroller_hit.render_object->GetNode());
        std::cout << "scroller bounds: " << scroller_bounds.x << ", " << scroller_bounds.y
                  << " " << scroller_bounds.width << "x" << scroller_bounds.height
                  << " hit tag: " << (hit_element ? hit_element->GetTagName() : "<none>")
                  << " local: " << scroller_hit.local_x << ", " << scroller_hit.local_y
                  << std::endl;
    }
    ASSERT_TRUE(hit_inside_scroller);

    auto scroller_event = CreateWheelEvent(
        window, scroller_bounds.x + 12.0f, scroller_bounds.y + 12.0f, -1.0f);
    EXPECT_TRUE(dispatcher.HandleWheelEvent(scroller_event, window, document));
    EXPECT_GT(scroller_render->GetScrollY(), 0.0f);
    EXPECT_EQ(body_render->GetScrollY(), 0.0f);

    const float scroller_scroll_y = scroller_render->GetScrollY();
    auto overlay_event = CreateWheelEvent(window, 40.0f, 40.0f, -1.0f);
    EXPECT_TRUE(dispatcher.HandleWheelEvent(overlay_event, window, document));
    EXPECT_EQ(scroller_render->GetScrollY(), scroller_scroll_y);
    EXPECT_EQ(body_render->GetScrollY(), 0.0f);
}

TEST(WheelEventDispatcherTest, SmallFixedTopLayerDoesNotBlockPageWheel) {
    auto document = std::make_shared<Document>();
    document->Initialize();

    auto body = document->GetBody();
    ASSERT_TRUE(body);

    body->SetStyle("margin", "0");
    body->SetStyle("overflow-y", "auto");

    auto page_content = document->CreateElement("div");
    page_content->SetStyle("height", "1800px");
    page_content->AppendChild(document->CreateTextNode("page content"));

    auto floating_button = document->CreateElement("button");
    floating_button->SetAttribute("id", "floating-button");
    floating_button->SetStyle("position", "fixed");
    floating_button->SetStyle("left", "24px");
    floating_button->SetStyle("top", "24px");
    floating_button->SetStyle("width", "160px");
    floating_button->SetStyle("height", "48px");
    floating_button->SetStyle("z-index", "9000");
    floating_button->AppendChild(document->CreateTextNode("Floating"));

    body->AppendChild(page_content);
    body->AppendChild(floating_button);

    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    window->Render();

    auto body_render = body->GetRenderObject();
    ASSERT_TRUE(body_render);
    ASSERT_GT(body_render->GetMaxScrollY(), 0.0f);

    WheelEventDispatcher dispatcher;
    auto event = CreateWheelEvent(window, 48.0f, 48.0f, -1.0f);
    EXPECT_TRUE(dispatcher.HandleWheelEvent(event, window, document));
    EXPECT_GT(body_render->GetScrollY(), 0.0f);
}

} // namespace test
} // namespace mbink
