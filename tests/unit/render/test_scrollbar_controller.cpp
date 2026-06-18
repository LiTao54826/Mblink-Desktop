#include <gtest/gtest.h>

#include "core/render/objects/scrollbar_controller.h"

namespace mbink {
namespace test {

TEST(ScrollbarControllerTest, VerticalOverflowDoesNotCreateHorizontalScrollbar) {
    const ScrollbarState state = ScrollbarController::ComputeState(
        200.0f,
        320.0f,
        200.0f,
        160.0f,
        "auto",
        "auto",
        12.0f);

    EXPECT_TRUE(state.needs_vertical);
    EXPECT_FALSE(state.needs_horizontal);
    EXPECT_FLOAT_EQ(state.content_area_width, 188.0f);
    EXPECT_FLOAT_EQ(ScrollMaxForAxis(200.0f, state.content_area_width, state.needs_horizontal), 0.0f);
    EXPECT_GT(ScrollMaxForAxis(320.0f, state.content_area_height, state.needs_vertical), 0.0f);
}

TEST(ScrollbarControllerTest, OnePixelToleranceSuppressesTinyOverflow) {
    const ScrollbarState state = ScrollbarController::ComputeState(
        200.5f,
        160.5f,
        200.0f,
        160.0f,
        "auto",
        "auto",
        12.0f);

    EXPECT_FALSE(state.needs_vertical);
    EXPECT_FALSE(state.needs_horizontal);
}

TEST(ScrollbarControllerTest, HorizontalScrollbarCanTriggerVerticalScrollbar) {
    const ScrollbarState state = ScrollbarController::ComputeState(
        260.0f,
        155.0f,
        200.0f,
        160.0f,
        "auto",
        "auto",
        12.0f);

    EXPECT_TRUE(state.needs_horizontal);
    EXPECT_TRUE(state.needs_vertical);
    EXPECT_FLOAT_EQ(state.content_area_width, 188.0f);
    EXPECT_FLOAT_EQ(state.content_area_height, 148.0f);
}

} // namespace test
} // namespace mbink
