#include <gtest/gtest.h>

#include "core/dom/elements/html_input_element.h"
#include "core/render/objects/render_inline_block.h"
#include "core/render/text/font_manager.h"
#include "include/core/SkFontMetrics.h"

namespace mbink {
namespace test {

namespace {

float TextGlyphHeight(const ComputedStyle& style) {
    FontDescriptor desc;
    desc.family = style.font_family;
    desc.size = style.font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;

    SkFont font = FontManager::GetInstance().LoadFont(desc);
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    return std::max(0.0f, -metrics.fAscent + metrics.fDescent);
}

}  // namespace

TEST(InputIntrinsicSizeTest, TextInputAutoHeightFitsFontMetrics) {
    auto input = std::make_shared<HTMLInputElement>();
    input->SetInputType(InputType::Text);

    RenderInlineBlock render;
    render.SetNode(input);

    ComputedStyle style;
    style.display = RenderObjectType::INLINE_BLOCK;
    style.font_family = "Arial";
    style.font_size = 16.0f;
    style.line_height = 1.2f;
    style.width = CSSLength(200, CSSUnit::PX);
    style.padding.top = CSSLength(2, CSSUnit::PX);
    style.padding.bottom = CSSLength(2, CSSUnit::PX);
    style.border.width = CSSLength(2, CSSUnit::PX);
    style.border.style = CSSBorderStyle::SOLID;
    render.SetComputedStyle(style);

    auto [width, height] = render.MeasureIntrinsicSize(400.0f);

    const float expected_min_height =
        TextGlyphHeight(style) +
        style.padding.top.ToPx(400.0f, style.font_size) +
        style.padding.bottom.ToPx(400.0f, style.font_size) +
        style.border.width.ToPx(400.0f, style.font_size) * 2.0f;

    EXPECT_GE(width, 200.0f);
    EXPECT_GE(height, expected_min_height);
}

TEST(InputIntrinsicSizeTest, EditableInputTypesUseTextMetrics) {
    for (InputType type : {InputType::Text, InputType::Password, InputType::Search,
                           InputType::Email, InputType::Tel, InputType::Url,
                           InputType::Number}) {
        auto input = std::make_shared<HTMLInputElement>();
        input->SetInputType(type);

        RenderInlineBlock render;
        render.SetNode(input);

        ComputedStyle style;
        style.display = RenderObjectType::INLINE_BLOCK;
        style.font_family = "Arial";
        style.font_size = 18.0f;
        style.line_height = 1.2f;
        style.width = CSSLength(200, CSSUnit::PX);
        style.padding.top = CSSLength(1, CSSUnit::PX);
        style.padding.bottom = CSSLength(1, CSSUnit::PX);
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        render.SetComputedStyle(style);

        auto [_, height] = render.MeasureIntrinsicSize(400.0f);
        const float expected_min_height =
            TextGlyphHeight(style) +
            style.padding.top.ToPx(400.0f, style.font_size) +
            style.padding.bottom.ToPx(400.0f, style.font_size) +
            style.border.width.ToPx(400.0f, style.font_size) * 2.0f;

        EXPECT_GE(height, expected_min_height) << "InputType=" << static_cast<int>(type);
    }
}

}  // namespace test
}  // namespace mbink
