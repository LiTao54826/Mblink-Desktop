#include <gtest/gtest.h>

#include "core/dom/elements/html_input_element.h"
#include "core/editing/input_edit_command.h"
#include "core/editing/input_ime_geometry.h"
#include "core/event/dispatch/mouse_event_dispatcher.h"
#include "core/render/input/input_text_viewport.h"
#include "core/render/input/text_edit_metrics.h"
#include "core/render/text/font_manager.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"

namespace mbink {
namespace test {
namespace {

SkFont TestFont() {
    FontDescriptor desc;
    desc.family = "Arial";
    desc.size = 16.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    return FontManager::GetInstance().LoadFont(desc);
}

InputPaintModel ModelForText(const std::string& text, int caret_position) {
    InputPaintModel model;
    model.value = text;
    model.display_text = text;
    model.visual_text = text;
    model.selection_start = caret_position;
    model.selection_end = caret_position;
    model.caret_position = caret_position;
    return model;
}

float WidthOf(const InputPaintModel& model, const SkFont& font) {
    return text_edit_metrics::MeasureTextWidth(model.visual_text,
                                               font,
                                               model.is_password && !model.is_placeholder);
}

}  // namespace

TEST(InputTextViewportTest, ReturnsZeroScrollWhenContentFitsVisibleWidth) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("short", 5);

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        WidthOf(model, font) + 20.0f,
        0.0f,
        50.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_FLOAT_EQ(viewport.scroll_left, 0.0f);
    EXPECT_FLOAT_EQ(viewport.max_scroll_left, 0.0f);
}

TEST(InputTextViewportTest, ScrollsRightToKeepCaretEndVisible) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("abcdefghijklmnopqrstuvwxyz", 26);
    const float visible_width = std::max(1.0f, WidthOf(model, font) * 0.5f);

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        visible_width,
        0.0f,
        0.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_GT(viewport.scroll_left, 0.0f);
    EXPECT_LE(viewport.scroll_left, viewport.max_scroll_left);
}

TEST(InputTextViewportTest, ScrollsBackLeftWhenCaretMovesBeforeViewport) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("abcdefghijklmnopqrstuvwxyz", 1);

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        40.0f,
        0.0f,
        80.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_LT(viewport.scroll_left, 80.0f);
    EXPECT_GE(viewport.scroll_left, 0.0f);
}

TEST(InputTextViewportTest, ReservesSpinnerWidthForNumberInput) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("1234567890", 10);

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        80.0f,
        input_text_viewport::kNumberSpinnerReservedWidth,
        0.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_FLOAT_EQ(viewport.visible_width, 64.0f);
}

TEST(InputTextViewportTest, UsesMaskedCharacterWidthForPasswordInput) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("secret", 6);
    model.is_password = true;
    model.display_text = "******";
    model.visual_text = "******";

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        WidthOf(model, font) * 0.5f,
        0.0f,
        0.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_GT(viewport.scroll_left, 0.0f);
    EXPECT_FLOAT_EQ(viewport.text_width,
                    text_edit_metrics::MeasureTextWidth("******", font, false));
}

TEST(InputTextViewportTest, UsesCompositionStartAsActiveEdge) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("abcdefghijklmnopqrstuvwxyz", 26);
    model.has_composition = true;
    model.composition_start = 2;
    model.composition_end = 4;
    model.selection_start = 2;
    model.selection_end = 4;
    model.caret_position = 4;

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        40.0f,
        0.0f,
        80.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_LT(viewport.scroll_left, 80.0f);
}

TEST(InputTextViewportTest, UsesSelectionFocusAsActiveEdgeForReversedSelection) {
    SkFont font = TestFont();
    InputPaintModel model = ModelForText("abcdefghijklmnopqrstuvwxyz", 3);
    model.selection_start = 20;
    model.selection_end = 3;
    model.caret_position = 3;

    auto viewport = input_text_viewport::Resolve({
        &model,
        &font,
        40.0f,
        0.0f,
        90.0f,
        input_text_viewport::ActiveCharPosition(model)
    });

    EXPECT_LT(viewport.scroll_left, 90.0f);
}

TEST(InputTextViewportTest, ConvertsVisibleXToContentXUsingResolvedScroll) {
    input_text_viewport::ViewportState viewport;
    viewport.visible_width = 50.0f;
    viewport.scroll_left = 25.0f;

    EXPECT_FLOAT_EQ(input_text_viewport::ContentXFromVisibleX(10.0f, viewport), 35.0f);
    EXPECT_FLOAT_EQ(input_text_viewport::ContentXFromVisibleX(100.0f, viewport), 75.0f);
}

TEST(InputImeGeometryTest, SubtractsResolvedScrollFromCaretOffset) {
    SkFont font = TestFont();
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    InputPaintModel model = ModelForText("abcdefghijklmnopqrstuvwxyz", 26);
    const float visible_width = std::max(1.0f, WidthOf(model, font) * 0.5f);

    auto geometry = input_ime_geometry::ResolveInputGeometry({
        &model,
        &font,
        &metrics,
        10.0f,
        20.0f,
        visible_width,
        10.0f,
        0.0f,
        0.0f
    });

    EXPECT_FLOAT_EQ(geometry.area_x, 10.0f);
    EXPECT_FLOAT_EQ(geometry.area_w, visible_width);
    EXPECT_LE(geometry.caret_offset, geometry.area_w);
    EXPECT_GT(geometry.resolved_scroll_left, 0.0f);
}

TEST(InputImeGeometryTest, UsesCompositionStartForCaretOffset) {
    SkFont font = TestFont();
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    InputPaintModel model = ModelForText("abcdef", 6);
    model.has_composition = true;
    model.composition_start = 2;
    model.composition_end = 5;

    auto geometry = input_ime_geometry::ResolveInputGeometry({
        &model,
        &font,
        &metrics,
        0.0f,
        0.0f,
        200.0f,
        10.0f,
        0.0f,
        0.0f
    });

    float expected = text_edit_metrics::MeasurePrefixWidth(model.visual_text, 2, font, false);
    EXPECT_FLOAT_EQ(geometry.caret_offset, expected);
}

TEST(InputImeGeometryTest, ReservesSpinnerWidthInNumberInputAreaWidth) {
    SkFont font = TestFont();
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    InputPaintModel model = ModelForText("123", 3);

    auto geometry = input_ime_geometry::ResolveInputGeometry({
        &model,
        &font,
        &metrics,
        0.0f,
        0.0f,
        80.0f,
        10.0f,
        0.0f,
        input_text_viewport::kNumberSpinnerReservedWidth
    });

    EXPECT_FLOAT_EQ(geometry.area_w, 64.0f);
}

TEST(InputElementHorizontalScrollTest, DirectionalSelectionPreservesAnchorAndFocusOrder) {
    auto input = std::make_shared<HTMLInputElement>();
    input->SetValue("abcdefghijklmnopqrstuvwxyz");

    EXPECT_TRUE(input->ExecuteEditCommand(InputEditCommand::SetSelection(8, 3)));

    auto state = input->GetEditState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->selection_anchor, 8);
    EXPECT_EQ(state->selection_focus, 3);
    EXPECT_EQ(state->caret_position, 3);
    EXPECT_EQ(input->GetSelectionStart(), 3);
    EXPECT_EQ(input->GetSelectionEnd(), 8);
}

TEST(InputElementHorizontalScrollTest, SetSelectionDirectionalPreservesReverseDragDirection) {
    auto input = std::make_shared<HTMLInputElement>();
    input->SetValue("abcdefghijklmnopqrstuvwxyz");

    input->SetSelectionDirectional(20, 4);

    auto state = input->GetEditState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->selection_anchor, 20);
    EXPECT_EQ(state->selection_focus, 4);
    EXPECT_EQ(state->caret_position, 4);
}

TEST(InputElementHorizontalScrollTest, SetValueClampsSelectionWithoutReordering) {
    auto input = std::make_shared<HTMLInputElement>();
    input->SetValue("abcdefghijklmnopqrstuvwxyz");
    input->SetSelectionDirectional(20, 4);

    input->SetValue("short");

    auto state = input->GetEditState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->selection_anchor, 5);
    EXPECT_EQ(state->selection_focus, 4);
    EXPECT_EQ(state->caret_position, 4);
}

TEST(InputMouseInteractionTest, ClickInScrolledInputUsesVisiblePointerPosition) {
    MouseEventDispatcher dispatcher;
    auto input = std::make_shared<HTMLInputElement>();
    input->SetValue("abcdefghijklmnopqrstuvwxyz");

    dispatcher.HandleInputMouseInteraction(input,
                                           5.0f,
                                           SDL_EVENT_MOUSE_BUTTON_DOWN,
                                           16.0f,
                                           "Arial",
                                           40.0f);
    int unscrolled_caret = input->GetEditState()->caret_position;

    input->SetScrollLeft(80.0f);
    dispatcher.HandleInputMouseInteraction(input,
                                           5.0f,
                                           SDL_EVENT_MOUSE_BUTTON_DOWN,
                                           16.0f,
                                           "Arial",
                                           40.0f);
    int scrolled_caret = input->GetEditState()->caret_position;

    EXPECT_GT(scrolled_caret, unscrolled_caret);
}

TEST(InputMouseInteractionTest, DraggingPastRightEdgeAutoScrollsAndExtendsSelection) {
    MouseEventDispatcher dispatcher;
    auto input = std::make_shared<HTMLInputElement>();
    input->SetValue("abcdefghijklmnopqrstuvwxyz");

    dispatcher.HandleInputMouseInteraction(input,
                                           0.0f,
                                           SDL_EVENT_MOUSE_BUTTON_DOWN,
                                           16.0f,
                                           "Arial",
                                           40.0f);
    dispatcher.HandleInputMouseInteraction(input,
                                           80.0f,
                                           SDL_EVENT_MOUSE_MOTION,
                                           16.0f,
                                           "Arial",
                                           40.0f);

    auto state = input->GetEditState();
    ASSERT_NE(state, nullptr);
    EXPECT_GT(input->GetScrollLeft(), 0.0f);
    EXPECT_EQ(state->selection_anchor, input->GetDragStartPos());
    EXPECT_GT(state->selection_focus, state->selection_anchor);
}

}  // namespace test
}  // namespace mbink
