#include <gtest/gtest.h>

#include "dom/document.h"
#include "dom/element.h"
#include "event/input/hit_test_controller.h"
#include "layout/native_layout_engine.h"
#include "render/css/style_resolver.h"
#include "render/objects/render_object.h"
#include "test_utils/test_helpers.h"
#include "include/core/SkSurface.h"

#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace mblink {
namespace test {

namespace {

std::shared_ptr<RenderObject> FindRenderObjectByElementId(
    const std::shared_ptr<RenderObject>& root,
    const std::string& id) {
    if (!root) {
        return nullptr;
    }

    auto node = root->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element && element->GetAttribute("id") == id) {
        return root;
    }

    for (const auto& child : root->GetChildren()) {
        auto found = FindRenderObjectByElementId(child, id);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

std::string StickyTableFixtureHtml() {
    std::ostringstream html;
    html << R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 220px; height: 120px; overflow: auto;">
                <table id="table" style="width: 900px; border-spacing: 0;">
                    <thead>
                        <tr>
                            <th id="corner-cell"
                                style="position: sticky; top: 0; left: 0; width: 140px; padding: 12px; background-color: #ff0000;">
                                Corner
                            </th>
                            <th id="sticky-header"
                                style="position: sticky; top: 0; width: 140px; padding: 12px; background-color: #ffffff;">
                                Header 2
                            </th>
                            <th style="position: sticky; top: 0; width: 140px; padding: 12px; background-color: #ffffff;">
                                Header 3
                            </th>
                            <th style="position: sticky; top: 0; width: 140px; padding: 12px; background-color: #ffffff;">
                                Header 4
                            </th>
                        </tr>
                    </thead>
                    <tbody>
    )";

    for (int row = 0; row < 8; ++row) {
        html << "<tr>";
        html << "<td id=\"first-column-" << row << "\" "
             << "style=\"position: sticky; left: 0; width: 140px; padding: 12px; background-color: #ff0000;\">"
             << "Row " << row << "</td>";
        for (int col = 1; col < 4; ++col) {
            const char* id_attr = (row == 3 && col == 2) ? " id=\"regular-cell\"" : "";
            html << "<td" << id_attr << " style=\"width: 140px; padding: 12px;\">"
                 << "R" << row << " C" << col << "</td>";
        }
        html << "</tr>";
    }

    html << R"(
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )";
    return html.str();
}

std::string StickyTableCssFixtureHtml() {
    std::ostringstream html;
    html << R"(
        <html>
        <head>
            <style>
                body { margin: 0; }
                #scroller { width: 220px; height: 120px; overflow: auto; }
                #table { width: 900px; border-spacing: 0; }
                th, td { width: 140px; padding: 12px; }
                thead th { position: sticky; top: 0; background-color: #ffffff; }
                th:first-child,
                td:first-child {
                    position: sticky;
                    left: 0;
                    background-color: #ff0000;
                    z-index: 2;
                }
                thead th:first-child { z-index: 3; }
            </style>
        </head>
        <body>
            <div id="scroller">
                <table id="table">
                    <thead>
                        <tr>
                            <th id="css-corner-cell">Corner</th>
                            <th id="css-sticky-header">Header 2</th>
                            <th>Header 3</th>
                            <th>Header 4</th>
                        </tr>
                    </thead>
                    <tbody>
    )";

    for (int row = 0; row < 8; ++row) {
        html << "<tr>";
        html << "<td id=\"css-first-column-" << row << "\">Row " << row << "</td>";
        for (int col = 1; col < 4; ++col) {
            html << "<td>R" << row << " C" << col << "</td>";
        }
        html << "</tr>";
    }

    html << R"(
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )";
    return html.str();
}

}  // namespace

class TableStickyLayoutTest : public DOMTestBase {
protected:
    void TearDown() override {
        render_root_.reset();
        layout_engine_.reset();
        DOMTestBase::TearDown();
    }

    void LoadStickyFixtureHtml(const std::string& html) {
        doc_ = CreateDocumentFromHTML(html);
        ASSERT_NE(doc_, nullptr);
        ASSERT_NE(doc_->GetBody(), nullptr);

        RenderTreeBuilder builder;
        builder.SetDocument(doc_.get());
        render_root_ = builder.BuildRenderTree(doc_->GetBody());
        ASSERT_NE(render_root_, nullptr);

        layout_engine_ = std::make_unique<NativeLayoutEngine>();
        layout_engine_->BuildLayoutTree(render_root_);
        layout_engine_->ComputeLayout(800.0f, 600.0f);
        layout_engine_->GetLayoutInfo(render_root_);
    }

    void LoadStickyFixture() {
        LoadStickyFixtureHtml(StickyTableFixtureHtml());
    }

    void LoadStickyCssFixture() {
        LoadStickyFixtureHtml(StickyTableCssFixtureHtml());
    }

    std::shared_ptr<RenderObject> RenderObjectForId(const std::string& id) const {
        return FindRenderObjectByElementId(render_root_, id);
    }

    ViewportBounds BoundsFor(const std::shared_ptr<RenderObject>& object) const {
        object->UpdateViewportBounds();
        return object->GetViewportBounds();
    }

    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    std::shared_ptr<RenderObject> render_root_;
};

TEST_F(TableStickyLayoutTest, HeaderCellRemainsAtScrollContainerTopAfterVerticalScroll) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    auto header = RenderObjectForId("sticky-header");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(header, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    const auto before_header_bounds = BoundsFor(header);

    scroller->SetScrollY(80.0f);

    const auto after_header_bounds = BoundsFor(header);
    EXPECT_NEAR(after_header_bounds.y, scroller_bounds.y, 0.5f);
    EXPECT_NEAR(after_header_bounds.x, before_header_bounds.x, 0.5f);
}

TEST_F(TableStickyLayoutTest, FirstColumnCellRemainsAtScrollContainerLeftAfterHorizontalScroll) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    auto first_column = RenderObjectForId("first-column-3");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(first_column, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    const auto before_column_bounds = BoundsFor(first_column);

    scroller->SetScrollX(120.0f);

    const auto after_column_bounds = BoundsFor(first_column);
    EXPECT_NEAR(after_column_bounds.x, scroller_bounds.x, 0.5f);
    EXPECT_NEAR(after_column_bounds.y, before_column_bounds.y, 0.5f);
}

TEST_F(TableStickyLayoutTest, FirstColumnCellRemainsAtScrollContainerLeftAfterLargeHorizontalScroll) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    auto first_column = RenderObjectForId("first-column-3");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(first_column, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    const auto before_column_bounds = BoundsFor(first_column);

    scroller->SetScrollX(360.0f);

    const auto after_column_bounds = BoundsFor(first_column);
    EXPECT_NEAR(after_column_bounds.x, scroller_bounds.x, 0.5f);
    EXPECT_NEAR(after_column_bounds.y, before_column_bounds.y, 0.5f);
}

TEST_F(TableStickyLayoutTest, FirstColumnPaintsAfterLargeHorizontalScroll) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    ASSERT_NE(scroller, nullptr);
    const auto scroller_bounds = BoundsFor(scroller);

    scroller->SetScrollX(360.0f);
    render_root_->InvalidateDescendantViewportBounds();

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_NE(surface, nullptr);
    auto* canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root_->Paint(canvas);

    SkPixmap pixels;
    ASSERT_TRUE(surface->peekPixels(&pixels));

    SkColor color = pixels.getColor(
        static_cast<int>(scroller_bounds.x + 12.0f),
        static_cast<int>(scroller_bounds.y + 72.0f));
    EXPECT_GT(SkColorGetR(color), 200);
    EXPECT_LT(SkColorGetG(color), 80);
    EXPECT_LT(SkColorGetB(color), 80);
}

TEST_F(TableStickyLayoutTest, CssFirstChildColumnPaintsAfterLargeHorizontalScroll) {
    LoadStickyCssFixture();

    auto scroller = RenderObjectForId("scroller");
    auto first_column = RenderObjectForId("css-first-column-0");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(first_column, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    const auto before_column_bounds = BoundsFor(first_column);

    scroller->SetScrollX(360.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto after_column_bounds = BoundsFor(first_column);
    EXPECT_NEAR(after_column_bounds.x, scroller_bounds.x, 0.5f);
    EXPECT_NEAR(after_column_bounds.y, before_column_bounds.y, 0.5f);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_NE(surface, nullptr);
    auto* canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root_->Paint(canvas);

    SkPixmap pixels;
    ASSERT_TRUE(surface->peekPixels(&pixels));

    SkColor color = pixels.getColor(
        static_cast<int>(scroller_bounds.x + 12.0f),
        static_cast<int>(scroller_bounds.y + 72.0f));
    EXPECT_GT(SkColorGetR(color), 200);
    EXPECT_LT(SkColorGetG(color), 80);
    EXPECT_LT(SkColorGetB(color), 80);

    HitTestController controller;
    auto result = controller.HitTest(render_root_, scroller_bounds.x + 8.0f, scroller_bounds.y + 72.0f);
    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "css-first-column-0");
}

TEST_F(TableStickyLayoutTest, CornerCellSticksOnBothAxes) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    auto corner = RenderObjectForId("corner-cell");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(corner, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);

    scroller->SetScrollX(120.0f);
    scroller->SetScrollY(80.0f);

    const auto corner_bounds = BoundsFor(corner);
    EXPECT_NEAR(corner_bounds.x, scroller_bounds.x, 0.5f);
    EXPECT_NEAR(corner_bounds.y, scroller_bounds.y, 0.5f);
}

TEST_F(TableStickyLayoutTest, NonStickyCellsScrollUnderStickyCellsNormally) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    auto regular = RenderObjectForId("regular-cell");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(regular, nullptr);

    const auto before_regular_bounds = BoundsFor(regular);

    scroller->SetScrollX(120.0f);
    scroller->SetScrollY(80.0f);

    const auto after_regular_bounds = BoundsFor(regular);
    EXPECT_NEAR(after_regular_bounds.x, before_regular_bounds.x - 120.0f, 0.5f);
    EXPECT_NEAR(after_regular_bounds.y, before_regular_bounds.y - 80.0f, 0.5f);
}

TEST_F(TableStickyLayoutTest, HitTestingUsesVisibleStickyCellPositionAfterScroll) {
    LoadStickyFixture();

    auto scroller = RenderObjectForId("scroller");
    ASSERT_NE(scroller, nullptr);
    const auto scroller_bounds = BoundsFor(scroller);

    scroller->SetScrollX(120.0f);
    scroller->SetScrollY(80.0f);

    HitTestController controller;
    auto result = controller.HitTest(render_root_, scroller_bounds.x + 8.0f, scroller_bounds.y + 8.0f);

    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "corner-cell");
}

TEST_F(TableStickyLayoutTest, StickyTablePaintAndHitTestHonorZIndexWhenCellsOverlap) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 140px; height: 96px; overflow: auto;">
                <table id="table" style="width: 480px; border-spacing: 0;">
                    <tbody>
                        <tr>
                            <td id="low-sticky"
                                style="position: sticky; left: 0; z-index: 1; width: 120px; height: 48px; padding: 0; background-color: #ff0000;">
                                Low
                            </td>
                            <td id="high-sticky"
                                style="position: sticky; left: 0; z-index: 5; width: 120px; height: 48px; padding: 0; text-align: right; background-color: #0000ff;">
                                <button id="high-sticky-button" type="button"
                                    style="display: block; width: 60px; height: 32px; padding: 0;">
                                    High
                                </button>
                            </td>
                            <td style="width: 120px; height: 48px; padding: 0;">Tail</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto high_sticky = RenderObjectForId("high-sticky");
    auto high_sticky_button = RenderObjectForId("high-sticky-button");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(high_sticky, nullptr);
    ASSERT_NE(high_sticky_button, nullptr);
    const auto scroller_bounds = BoundsFor(scroller);

    scroller->SetScrollX(160.0f);
    render_root_->InvalidateDescendantViewportBounds();

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_NE(surface, nullptr);
    auto* canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root_->Paint(canvas);

    SkPixmap pixels;
    ASSERT_TRUE(surface->peekPixels(&pixels));

    SkColor color = pixels.getColor(
        static_cast<int>(scroller_bounds.x + 8.0f),
        static_cast<int>(scroller_bounds.y + 12.0f));
    EXPECT_LT(SkColorGetR(color), 80);
    EXPECT_LT(SkColorGetG(color), 80);
    EXPECT_GT(SkColorGetB(color), 180);

    HitTestController controller;
    const auto button_bounds = BoundsFor(high_sticky_button);
    const auto sticky_offset = high_sticky->ComputeStickyOffset();
    auto result = controller.HitTest(render_root_,
                                     button_bounds.x + sticky_offset.x() + 4.0f,
                                     button_bounds.y + sticky_offset.y() + 4.0f);
    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "high-sticky-button");
}

TEST_F(TableStickyLayoutTest, StickyTableHitTestingPreservesNestedElementTargetAfterScroll) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 180px; height: 96px; overflow: auto;">
                <table id="table" style="width: 520px; border-spacing: 0;">
                    <tbody>
                        <tr>
                            <td id="sticky-cell"
                                style="position: sticky; left: 0; z-index: 2; width: 140px; padding: 8px; background-color: #ff0000;">
                                <button id="sticky-button" type="button"
                                    style="display: block; width: 96px; height: 32px; padding: 0;">
                                    Open
                                </button>
                            </td>
                            <td style="width: 160px; padding: 8px;">Middle</td>
                            <td style="width: 160px; padding: 8px;">Tail</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto sticky_button = RenderObjectForId("sticky-button");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(sticky_button, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    scroller->SetScrollX(160.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto button_bounds = BoundsFor(sticky_button);
    EXPECT_LT(button_bounds.x, scroller_bounds.x);

    HitTestController controller;
    auto result = controller.HitTest(render_root_, scroller_bounds.x + 16.0f, scroller_bounds.y + 16.0f);
    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "sticky-button");
}

TEST_F(TableStickyLayoutTest, MultipleLeftStickyColumnsKeepExplicitOffsetsAfterHorizontalScroll) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 360px; height: 96px; overflow: auto;">
                <table id="table" style="width: 720px; border-spacing: 0; table-layout: fixed;">
                    <tbody>
                        <tr>
                            <td id="left-sticky-1"
                                style="position: sticky; left: 0; z-index: 4; width: 120px; height: 48px; padding: 0; background-color: #ff0000;">
                                Left 1
                            </td>
                            <td id="left-sticky-2"
                                style="position: sticky; left: 120px; z-index: 4; width: 120px; height: 48px; padding: 0; background-color: #00ff00;">
                                Left 2
                            </td>
                            <td id="middle-cell" style="width: 120px; height: 48px; padding: 0;">Middle</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 1</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 2</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 3</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto left_sticky_1 = RenderObjectForId("left-sticky-1");
    auto left_sticky_2 = RenderObjectForId("left-sticky-2");
    auto middle_cell = RenderObjectForId("middle-cell");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(left_sticky_1, nullptr);
    ASSERT_NE(left_sticky_2, nullptr);
    ASSERT_NE(middle_cell, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    const auto left_1_width = BoundsFor(left_sticky_1).width;

    scroller->SetScrollX(260.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto left_1_bounds = BoundsFor(left_sticky_1);
    const auto left_2_bounds = BoundsFor(left_sticky_2);
    const auto middle_bounds = BoundsFor(middle_cell);
    EXPECT_NEAR(left_1_bounds.x, scroller_bounds.x, 0.5f);
    EXPECT_NEAR(left_2_bounds.x, scroller_bounds.x + left_1_width, 0.5f);
    EXPECT_LT(middle_bounds.x, left_2_bounds.x);

    HitTestController controller;
    auto first_result = controller.HitTest(render_root_,
                                           left_1_bounds.x + left_1_bounds.width * 0.5f,
                                           left_1_bounds.y + left_1_bounds.height * 0.5f);
    ASSERT_TRUE(first_result.IsValid());
    ASSERT_NE(first_result.element, nullptr);
    EXPECT_EQ(first_result.element->GetAttribute("id"), "left-sticky-1");

    auto second_result = controller.HitTest(render_root_,
                                            left_2_bounds.x + left_2_bounds.width * 0.5f,
                                            left_2_bounds.y + left_2_bounds.height * 0.5f);
    ASSERT_TRUE(second_result.IsValid());
    ASSERT_NE(second_result.element, nullptr);
    EXPECT_EQ(second_result.element->GetAttribute("id"), "left-sticky-2");
}

TEST_F(TableStickyLayoutTest, MultipleRightStickyColumnsKeepExplicitOffsetsAfterHorizontalScroll) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 360px; height: 96px; overflow: auto;">
                <table id="table" style="width: 720px; border-spacing: 0; table-layout: fixed;">
                    <tbody>
                        <tr>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 1</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 2</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 3</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 4</td>
                            <td id="right-sticky-2"
                                style="position: sticky; right: 120px; z-index: 4; width: 120px; height: 48px; padding: 0; background-color: #0000ff;">
                                Right 2
                            </td>
                            <td id="right-sticky-1"
                                style="position: sticky; right: 0; z-index: 5; width: 120px; height: 48px; padding: 0; background-color: #ff00ff;">
                                Right 1
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto right_sticky_2 = RenderObjectForId("right-sticky-2");
    auto right_sticky_1 = RenderObjectForId("right-sticky-1");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(right_sticky_2, nullptr);
    ASSERT_NE(right_sticky_1, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);

    scroller->SetScrollX(180.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto right_2_bounds = BoundsFor(right_sticky_2);
    const auto right_1_bounds = BoundsFor(right_sticky_1);
    EXPECT_NEAR(right_1_bounds.x + right_1_bounds.width,
                scroller_bounds.x + scroller_bounds.width,
                0.5f);
    EXPECT_NEAR(right_2_bounds.x + right_2_bounds.width, right_1_bounds.x, 0.5f);

    HitTestController controller;
    auto penultimate_result = controller.HitTest(render_root_,
                                                 right_2_bounds.x + right_2_bounds.width * 0.5f,
                                                 right_2_bounds.y + right_2_bounds.height * 0.5f);
    ASSERT_TRUE(penultimate_result.IsValid());
    ASSERT_NE(penultimate_result.element, nullptr);
    EXPECT_EQ(penultimate_result.element->GetAttribute("id"), "right-sticky-2");

    auto last_result = controller.HitTest(render_root_,
                                          right_1_bounds.x + right_1_bounds.width * 0.5f,
                                          right_1_bounds.y + right_1_bounds.height * 0.5f);
    ASSERT_TRUE(last_result.IsValid());
    ASSERT_NE(last_result.element, nullptr);
    EXPECT_EQ(last_result.element->GetAttribute("id"), "right-sticky-1");
}

TEST_F(TableStickyLayoutTest, RightStickyColumnsAttachAtInitialHorizontalScrollWhenNaturallyOffscreen) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 360px; height: 96px; overflow: auto;">
                <table id="table" style="width: 720px; border-spacing: 0; table-layout: fixed;">
                    <tbody>
                        <tr>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 1</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 2</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 3</td>
                            <td style="width: 120px; height: 48px; padding: 0;">Data 4</td>
                            <td id="right-sticky-2"
                                style="position: sticky; right: 120px; z-index: 4; width: 120px; height: 48px; padding: 0; background-color: #0000ff;">
                                Right 2
                            </td>
                            <td id="right-sticky-1"
                                style="position: sticky; right: 0; z-index: 5; width: 120px; height: 48px; padding: 0; background-color: #ff00ff;">
                                Right 1
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto right_sticky_2 = RenderObjectForId("right-sticky-2");
    auto right_sticky_1 = RenderObjectForId("right-sticky-1");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(right_sticky_2, nullptr);
    ASSERT_NE(right_sticky_1, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    ASSERT_GT(scroller->GetMaxScrollX(), 0.0f);

    const auto right_2_bounds = BoundsFor(right_sticky_2);
    const auto right_1_bounds = BoundsFor(right_sticky_1);
    EXPECT_NEAR(right_1_bounds.x + right_1_bounds.width,
                scroller_bounds.x + scroller_bounds.width,
                0.5f);
    EXPECT_NEAR(right_2_bounds.x + right_2_bounds.width, right_1_bounds.x, 0.5f);
}

TEST_F(TableStickyLayoutTest, RightStickyColumnStaysAtScrollContainerRightAfterHorizontalScroll) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 220px; height: 120px; overflow: auto;">
                <table id="table" style="width: 640px; border-spacing: 0;">
                    <tbody>
                        <tr>
                            <td style="width: 120px; padding: 12px;">A</td>
                            <td style="width: 120px; padding: 12px;">B</td>
                            <td id="right-sticky"
                                style="position: sticky; right: 0; z-index: 4; width: 120px; padding: 12px; background-color: #00ff00;">
                                <button id="right-sticky-button" type="button"
                                    style="display: block; width: 96px; height: 32px; padding: 0;">
                                    Action
                                </button>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto right_sticky = RenderObjectForId("right-sticky");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(right_sticky, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    scroller->SetScrollX(240.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto sticky_bounds = BoundsFor(right_sticky);
    EXPECT_NEAR(sticky_bounds.x + sticky_bounds.width, scroller_bounds.x + scroller_bounds.width, 0.5f);

    HitTestController controller;
    auto result = controller.HitTest(render_root_,
                                     scroller_bounds.x + scroller_bounds.width - 116.0f,
                                     scroller_bounds.y + 24.0f);
    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "right-sticky-button");
}

TEST_F(TableStickyLayoutTest, BottomStickyFooterStaysAtScrollContainerBottomAfterVerticalScroll) {
    LoadStickyFixtureHtml(R"(
        <html>
        <body style="margin: 0;">
            <div id="scroller" style="width: 220px; height: 120px; overflow: auto;">
                <table id="table" style="width: 220px; border-spacing: 0;">
                    <tbody>
                        <tr><td style="padding: 24px 0;">A</td></tr>
                        <tr><td style="padding: 24px 0;">B</td></tr>
                        <tr><td style="padding: 24px 0;">C</td></tr>
                        <tr>
                            <td id="bottom-sticky"
                                style="position: sticky; bottom: 0; z-index: 4; padding: 24px 0; background-color: #0000ff;">
                                <button id="bottom-sticky-button" type="button"
                                    style="display: block; width: 96px; height: 32px; padding: 0;">
                                    Total
                                </button>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto bottom_sticky = RenderObjectForId("bottom-sticky");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(bottom_sticky, nullptr);

    const auto scroller_bounds = BoundsFor(scroller);
    scroller->SetScrollY(32.0f);
    render_root_->InvalidateDescendantViewportBounds();

    const auto sticky_bounds = BoundsFor(bottom_sticky);
    EXPECT_NEAR(sticky_bounds.y + sticky_bounds.height, scroller_bounds.y + scroller_bounds.height, 0.5f);

    HitTestController controller;
    auto result = controller.HitTest(render_root_,
                                     scroller_bounds.x + 24.0f,
                                     scroller_bounds.y + scroller_bounds.height - 48.0f);
    ASSERT_TRUE(result.IsValid());
    ASSERT_NE(result.element, nullptr);
    EXPECT_EQ(result.element->GetAttribute("id"), "bottom-sticky-button");
}

}  // namespace test
}  // namespace mblink
