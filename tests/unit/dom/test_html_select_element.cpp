#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "test_utils/mock_objects.h"
#include "dom/elements/html_select_element.h"
#include "dom/elements/html_option_element.h"
#include "render/css/style_resolver.h"
#include "render/objects/render_object.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

#include <algorithm>
#include <cmath>

namespace mblink {
namespace test {

namespace {

bool ContainsDarkPixel(sk_sp<SkSurface> surface, const SkRect& search_rect) {
    if (!surface) {
        return false;
    }

    SkPixmap pixels;
    if (!surface->peekPixels(&pixels)) {
        return false;
    }

    const int left = std::max(0, static_cast<int>(std::floor(search_rect.left())));
    const int top = std::max(0, static_cast<int>(std::floor(search_rect.top())));
    const int right = std::min(pixels.width(), static_cast<int>(std::ceil(search_rect.right())));
    const int bottom = std::min(pixels.height(), static_cast<int>(std::ceil(search_rect.bottom())));

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const SkColor color = pixels.getColor(x, y);
            if (SkColorGetA(color) > 0 &&
                SkColorGetR(color) < 120 &&
                SkColorGetG(color) < 120 &&
                SkColorGetB(color) < 120) {
                return true;
            }
        }
    }

    return false;
}

bool ContainsRedPixel(sk_sp<SkSurface> surface, const SkRect& search_rect) {
    if (!surface) {
        return false;
    }

    SkPixmap pixels;
    if (!surface->peekPixels(&pixels)) {
        return false;
    }

    const int left = std::max(0, static_cast<int>(std::floor(search_rect.left())));
    const int top = std::max(0, static_cast<int>(std::floor(search_rect.top())));
    const int right = std::min(pixels.width(), static_cast<int>(std::ceil(search_rect.right())));
    const int bottom = std::min(pixels.height(), static_cast<int>(std::ceil(search_rect.bottom())));

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const SkColor color = pixels.getColor(x, y);
            if (SkColorGetA(color) > 0 &&
                SkColorGetR(color) > 180 &&
                SkColorGetG(color) < 100 &&
                SkColorGetB(color) < 100) {
                return true;
            }
        }
    }

    return false;
}

} // namespace

class HTMLSelectElementTest : public DOMTestBase {};

TEST_F(HTMLSelectElementTest, SetValueSelectsExistingOption) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));

    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    select->SetValue("b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
    EXPECT_FALSE(optionA->GetSelected());
}

TEST_F(HTMLSelectElementTest, ValueAttributeSelectsExistingOption) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));

    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    select->SetAttribute("value", "b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
}

TEST_F(HTMLSelectElementTest, PendingValueAppliedWhenOptionAppendedLater) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    select->SetAttribute("value", "b");

    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");

    select->AppendChild(optionA);
    EXPECT_EQ(select->GetValue(), "");
    EXPECT_EQ(select->GetSelectedIndex(), -1);

    select->AppendChild(optionB);

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
}

TEST_F(HTMLSelectElementTest, ProgrammaticSetValueDoesNotTriggerChange) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    MockEventListener listener;
    select->AddEventListener("change", listener.GetListener());

    select->SetValue("b");
    EXPECT_EQ(listener.GetCallCount(), 0);

    select->SetAttribute("value", "a");
    EXPECT_EQ(listener.GetCallCount(), 0);
}

TEST_F(HTMLSelectElementTest, UserSelectionHelpersTriggerChange) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);
    select->SetSelectedIndex(0);

    MockEventListener listener;
    select->AddEventListener("change", listener.GetListener());

    select->SelectNextOption();
    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(listener.GetCallCount(), 1);

    select->SetHoveredIndex(0);
    select->SelectHoveredOption();
    EXPECT_EQ(select->GetValue(), "a");
    EXPECT_EQ(listener.GetCallCount(), 2);
}

TEST_F(HTMLSelectElementTest, UserSelectionHelpersTriggerInputAndChange) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);
    select->SetSelectedIndex(0);

    MockEventListener input_listener;
    MockEventListener change_listener;
    select->AddEventListener("input", input_listener.GetListener());
    select->AddEventListener("change", change_listener.GetListener());

    select->SelectNextOption();

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(input_listener.GetCallCount(), 1);
    EXPECT_EQ(change_listener.GetCallCount(), 1);
    ASSERT_NE(input_listener.GetLastEvent(), nullptr);
    ASSERT_NE(change_listener.GetLastEvent(), nullptr);
    EXPECT_EQ(input_listener.GetLastEvent()->GetType(), "input");
    EXPECT_EQ(change_listener.GetLastEvent()->GetType(), "change");
}

TEST_F(HTMLSelectElementTest, SelectionChangeMarksRenderObjectForPaint) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    body->AppendChild(select);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    auto select_render = select->GetRenderObject();
    ASSERT_NE(select_render, nullptr);

    select_render->ClearNeedsPaint();
    select_render->ClearNeedsLayout();
    EXPECT_FALSE(select_render->NeedsPaint());
    EXPECT_FALSE(select_render->NeedsLayout());

    select->SetValue("b");

    EXPECT_TRUE(select_render->NeedsPaint());
    EXPECT_TRUE(select_render->NeedsLayout());
    EXPECT_EQ(select->GetValue(), "b");
}

TEST_F(HTMLSelectElementTest, OptionCollectionChangeMarksRenderObjectForPaintAndLayout) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    option->SetAttribute("value", "a");
    select->AppendChild(option);

    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    body->AppendChild(select);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    auto select_render = select->GetRenderObject();
    ASSERT_NE(select_render, nullptr);

    select_render->ClearNeedsPaint();
    select_render->ClearNeedsLayout();
    EXPECT_FALSE(select_render->NeedsPaint());
    EXPECT_FALSE(select_render->NeedsLayout());

    auto new_option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    new_option->SetAttribute("value", "b");
    select->AppendChild(new_option);

    EXPECT_TRUE(select_render->NeedsPaint());
    EXPECT_TRUE(select_render->NeedsLayout());
}

TEST_F(HTMLSelectElementTest, PendingValueReappliedWhenOptionValueChanges) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    option->SetAttribute("value", "a");
    select->AppendChild(option);

    select->SetAttribute("value", "b");
    EXPECT_EQ(select->GetValue(), "");
    EXPECT_EQ(select->GetSelectedIndex(), -1);

    option->SetAttribute("value", "b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 0);
    EXPECT_TRUE(option->GetSelected());
}

TEST_F(HTMLSelectElementTest, BlockDisplaySelectPaintsSelectedTextAndArrow) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    ASSERT_NE(select, nullptr);
    select->SetAttribute("style",
                         "display: block; width: 180px; height: 34px; "
                         "font-size: 14px; color: #111111; background: #ffffff; "
                         "border: 1px solid #767676; padding: 0 10px;");

    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    ASSERT_NE(option, nullptr);
    option->SetAttribute("value", "default");
    option->SetText("Default Speaker");
    select->AppendChild(option);
    select->SetSelectedIndex(0);

    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    body->AppendChild(select);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    auto select_render = select->GetRenderObject();
    ASSERT_NE(select_render, nullptr);
    EXPECT_EQ(select_render->GetType(), RenderObjectType::BLOCK);

    render_root->Layout(320.0f, 160.0f);
    render_root->UpdateViewportBounds();

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(320, 160));
    ASSERT_NE(surface, nullptr);
    auto canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root->Paint(canvas);

    const auto& layout = select_render->GetLayoutInfo();
    ASSERT_TRUE(layout.is_laid_out);

    SkRect text_rect = SkRect::MakeXYWH(layout.x + 12.0f,
                                        layout.y + 4.0f,
                                        layout.width - 40.0f,
                                        layout.height - 8.0f);
    SkRect arrow_rect = SkRect::MakeXYWH(layout.x + layout.width - 24.0f,
                                         layout.y + 6.0f,
                                         18.0f,
                                         layout.height - 12.0f);

    EXPECT_TRUE(ContainsDarkPixel(surface, text_rect));
    EXPECT_TRUE(ContainsDarkPixel(surface, arrow_rect));
}

TEST_F(HTMLSelectElementTest, BlockDisplaySelectDoesNotPaintOptionChildren) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    ASSERT_NE(select, nullptr);
    select->SetAttribute("style",
                         "display: block; width: 180px; height: 34px; "
                         "font-size: 14px; color: #111111; background: #ffffff; "
                         "border: 1px solid #767676; padding: 0 10px;");

    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    ASSERT_NE(option, nullptr);
    option->SetAttribute("value", "default");
    option->SetAttribute("style", "font-size: 36px; color: #ff0000;");
    option->SetText("Ghost Speaker");
    select->AppendChild(option);
    select->SetSelectedIndex(0);

    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    body->AppendChild(select);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    auto select_render = select->GetRenderObject();
    ASSERT_NE(select_render, nullptr);
    EXPECT_EQ(select_render->GetType(), RenderObjectType::BLOCK);

    render_root->Layout(320.0f, 160.0f);
    render_root->UpdateViewportBounds();

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(320, 160));
    ASSERT_NE(surface, nullptr);
    auto canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root->Paint(canvas);

    EXPECT_TRUE(ContainsDarkPixel(surface, SkRect::MakeXYWH(0.0f, 0.0f, 220.0f, 60.0f)));
    EXPECT_FALSE(ContainsRedPixel(surface, SkRect::MakeWH(320.0f, 160.0f)));
}

} // namespace test
} // namespace mblink
