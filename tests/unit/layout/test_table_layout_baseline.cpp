#include <gtest/gtest.h>

#include "dom/document.h"
#include "dom/element.h"
#include "dom/text.h"
#include "layout/native_layout_engine.h"
#include "render/css/style_resolver.h"
#include "render/objects/render_object.h"
#include "test_utils/test_helpers.h"
#include "include/core/SkSurface.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mbink {
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

std::vector<std::shared_ptr<RenderObject>> FindRenderObjectsByType(
    const std::shared_ptr<RenderObject>& root,
    RenderObjectType type) {
    std::vector<std::shared_ptr<RenderObject>> result;

    std::function<void(const std::shared_ptr<RenderObject>&)> walk =
        [&](const std::shared_ptr<RenderObject>& object) {
            if (!object) {
                return;
            }
            if (object->GetType() == type) {
                result.push_back(object);
            }
            for (const auto& child : object->GetChildren()) {
                walk(child);
            }
        };

    walk(root);
    return result;
}

std::shared_ptr<RenderObject> FirstChildOfType(
    const std::shared_ptr<RenderObject>& root,
    RenderObjectType type) {
    auto matches = FindRenderObjectsByType(root, type);
    return matches.empty() ? nullptr : matches.front();
}

}  // namespace

class TableLayoutBaselineTest : public DOMTestBase {
protected:
    void TearDown() override {
        render_root_.reset();
        layout_engine_.reset();
        DOMTestBase::TearDown();
    }

    void LoadAndLayout(const std::string& html, float width = 800.0f, float height = 600.0f) {
        doc_ = CreateDocumentFromHTML(html);
        ASSERT_NE(doc_, nullptr);
        ASSERT_NE(doc_->GetBody(), nullptr);

        RenderTreeBuilder builder;
        builder.SetDocument(doc_.get());
        render_root_ = builder.BuildRenderTree(doc_->GetBody());
        ASSERT_NE(render_root_, nullptr);

        layout_engine_ = std::make_unique<NativeLayoutEngine>();
        layout_engine_->BuildLayoutTree(render_root_);
        layout_engine_->ComputeLayout(width, height);
        layout_engine_->GetLayoutInfo(render_root_);
    }

    std::shared_ptr<RenderObject> RenderObjectForId(const std::string& id) const {
        return FindRenderObjectByElementId(render_root_, id);
    }

    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    std::shared_ptr<RenderObject> render_root_;
};

TEST_F(TableLayoutBaselineTest, CreatesRenderObjectsForSectionedTable) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table">
                <caption id="caption">People</caption>
                <thead id="head">
                    <tr id="header-row">
                        <th id="name-header">Name</th>
                        <th id="age-header">Age</th>
                    </tr>
                </thead>
                <tbody id="body">
                    <tr id="row-a">
                        <td id="name-a">Alice</td>
                        <td id="age-a">31</td>
                    </tr>
                    <tr id="row-b">
                        <td id="name-b">Bob</td>
                        <td id="age-b">28</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto table = RenderObjectForId("table");
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->GetType(), RenderObjectType::TABLE);

    ASSERT_NE(RenderObjectForId("caption"), nullptr);
    EXPECT_EQ(RenderObjectForId("caption")->GetType(), RenderObjectType::TABLE_CAPTION);

    ASSERT_NE(RenderObjectForId("head"), nullptr);
    EXPECT_EQ(RenderObjectForId("head")->GetType(), RenderObjectType::TABLE_HEADER_GROUP);

    ASSERT_NE(RenderObjectForId("body"), nullptr);
    EXPECT_EQ(RenderObjectForId("body")->GetType(), RenderObjectType::TABLE_ROW_GROUP);

    EXPECT_EQ(FindRenderObjectsByType(table, RenderObjectType::TABLE_ROW).size(), 3);
    EXPECT_EQ(FindRenderObjectsByType(table, RenderObjectType::TABLE_CELL).size(), 6);
}

TEST_F(TableLayoutBaselineTest, LaysOutRowsAndCellsWithStableGeometry) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table" style="width: 400px; border-spacing: 0;">
                <tbody>
                    <tr id="row-a">
                        <td id="cell-a1">Alpha</td>
                        <td id="cell-a2">Beta</td>
                    </tr>
                    <tr id="row-b">
                        <td id="cell-b1">Gamma</td>
                        <td id="cell-b2">Delta</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto table = RenderObjectForId("table");
    ASSERT_NE(table, nullptr);
    const auto& table_layout = table->GetLayoutInfo();
    EXPECT_TRUE(table_layout.is_laid_out);
    EXPECT_FLOAT_EQ(table_layout.width, 400.0f);
    EXPECT_GT(table_layout.height, 0.0f);

    auto first_row = RenderObjectForId("row-a");
    auto second_row = RenderObjectForId("row-b");
    ASSERT_NE(first_row, nullptr);
    ASSERT_NE(second_row, nullptr);
    EXPECT_TRUE(first_row->GetLayoutInfo().is_laid_out);
    EXPECT_TRUE(second_row->GetLayoutInfo().is_laid_out);
    EXPECT_LT(first_row->GetLayoutInfo().y, second_row->GetLayoutInfo().y);

    auto first_cell = RenderObjectForId("cell-a1");
    auto second_cell = RenderObjectForId("cell-a2");
    ASSERT_NE(first_cell, nullptr);
    ASSERT_NE(second_cell, nullptr);
    EXPECT_TRUE(first_cell->GetLayoutInfo().is_laid_out);
    EXPECT_TRUE(second_cell->GetLayoutInfo().is_laid_out);
    EXPECT_GT(first_cell->GetLayoutInfo().width, 0.0f);
    EXPECT_GT(first_cell->GetLayoutInfo().height, 0.0f);
    EXPECT_LT(first_cell->GetLayoutInfo().x, second_cell->GetLayoutInfo().x);
}

TEST_F(TableLayoutBaselineTest, PreservesColspanAndRowspanOnRenderCells) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table">
                <tbody>
                    <tr>
                        <td id="span-columns" colspan="2">wide</td>
                        <td id="rowspan-cell" rowspan="2">tall</td>
                    </tr>
                    <tr>
                        <td id="cell-b1">B1</td>
                        <td id="cell-b2">B2</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto colspan_cell = std::dynamic_pointer_cast<RenderTableCell>(RenderObjectForId("span-columns"));
    auto rowspan_cell = std::dynamic_pointer_cast<RenderTableCell>(RenderObjectForId("rowspan-cell"));
    ASSERT_NE(colspan_cell, nullptr);
    ASSERT_NE(rowspan_cell, nullptr);

    EXPECT_EQ(colspan_cell->GetColSpan(), 2);
    EXPECT_EQ(rowspan_cell->GetRowSpan(), 2);
    EXPECT_GT(colspan_cell->GetLayoutInfo().width, RenderObjectForId("cell-b1")->GetLayoutInfo().width);
    EXPECT_GT(rowspan_cell->GetLayoutInfo().height, RenderObjectForId("cell-b1")->GetLayoutInfo().height);
}

TEST_F(TableLayoutBaselineTest, ParsesCommonTableCssForFutureStickyAndSelectionWork) {
    LoadAndLayout(R"(
        <html>
        <body>
            <div id="scroller" style="width: 220px; height: 120px; overflow: auto;">
                <table id="table" style="border-collapse: collapse; user-select: text;">
                    <thead>
                        <tr>
                            <th id="sticky-header"
                                style="position: sticky; top: 0; background-color: #ffffff;">
                                Header
                            </th>
                            <th>Other</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                            <td id="sticky-column"
                                style="position: sticky; left: 0; user-select: none;">
                                Locked
                            </td>
                            <td>Value</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto table = RenderObjectForId("table");
    auto header = RenderObjectForId("sticky-header");
    auto column = RenderObjectForId("sticky-column");
    ASSERT_NE(table, nullptr);
    ASSERT_NE(header, nullptr);
    ASSERT_NE(column, nullptr);

    EXPECT_EQ(table->GetComputedStyle().border_collapse, "collapse");
    EXPECT_EQ(table->GetComputedStyle().user_select, "text");

    EXPECT_EQ(header->GetComputedStyle().position, "sticky");
    EXPECT_EQ(header->GetComputedStyle().top.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(header->GetComputedStyle().top.value, 0.0f);

    EXPECT_EQ(column->GetComputedStyle().position, "sticky");
    EXPECT_EQ(column->GetComputedStyle().left.unit, CSSUnit::PX);
    EXPECT_FLOAT_EQ(column->GetComputedStyle().left.value, 0.0f);
    EXPECT_EQ(column->GetComputedStyle().user_select, "none");
}

TEST_F(TableLayoutBaselineTest, ColAndColgroupDomExistButDoNotYetCreateColumnRenderObjects) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table">
                <colgroup id="group">
                    <col id="first-col" span="2" style="background-color: #ffeecc;">
                </colgroup>
                <tbody>
                    <tr>
                        <td id="cell-a">A</td>
                        <td id="cell-b">B</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    ASSERT_NE(doc_->GetElementById("group"), nullptr);
    ASSERT_NE(doc_->GetElementById("first-col"), nullptr);
    EXPECT_EQ(RenderObjectForId("group"), nullptr);
    EXPECT_EQ(RenderObjectForId("first-col"), nullptr);

    auto table = RenderObjectForId("table");
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(FindRenderObjectsByType(table, RenderObjectType::TABLE_CELL).size(), 2);
}

TEST_F(TableLayoutBaselineTest, ColAndColgroupStylesParticipateInColumnLayoutAndPaint) {
    LoadAndLayout(R"(
        <html>
        <body style="margin: 0;">
            <table id="table" style="width: 300px; table-layout: fixed; border-spacing: 0; border: 0;">
                <colgroup id="group" style="background-color: #ff0000;">
                    <col id="first-col" span="2" style="width: 120px; background-color: #00ff00;">
                    <col id="third-col" style="width: 60px;">
                </colgroup>
                <tbody>
                    <tr>
                        <td id="cell-a" style="height: 40px;">A</td>
                        <td id="cell-b">B</td>
                        <td id="cell-c">C</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    ASSERT_NE(doc_->GetElementById("group"), nullptr);
    ASSERT_NE(doc_->GetElementById("first-col"), nullptr);
    EXPECT_EQ(RenderObjectForId("group"), nullptr);
    EXPECT_EQ(RenderObjectForId("first-col"), nullptr);

    auto table = std::dynamic_pointer_cast<RenderTable>(RenderObjectForId("table"));
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->GetColumnCount(), 3u);
    EXPECT_NEAR(table->GetColumnWidths()[0], 120.0f, 0.5f);
    EXPECT_NEAR(table->GetColumnWidths()[1], 120.0f, 0.5f);
    EXPECT_NEAR(table->GetColumnWidths()[2], 60.0f, 0.5f);

    auto first_cell = RenderObjectForId("cell-a");
    auto second_cell = RenderObjectForId("cell-b");
    auto third_cell = RenderObjectForId("cell-c");
    ASSERT_NE(first_cell, nullptr);
    ASSERT_NE(second_cell, nullptr);
    ASSERT_NE(third_cell, nullptr);
    EXPECT_NEAR(second_cell->GetLayoutInfo().x - first_cell->GetLayoutInfo().x, 120.0f, 0.5f);
    EXPECT_NEAR(third_cell->GetLayoutInfo().x - second_cell->GetLayoutInfo().x, 120.0f, 0.5f);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(500, 200));
    ASSERT_NE(surface, nullptr);
    auto* canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    render_root_->Paint(canvas);

    SkPixmap pixels;
    ASSERT_TRUE(surface->peekPixels(&pixels));

    const auto table_bounds = table->GetViewportBoundingRect();
    SkColor first_color = pixels.getColor(
        static_cast<int>(table_bounds.x() + 20.0f),
        static_cast<int>(table_bounds.y() + 20.0f));
    SkColor third_color = pixels.getColor(
        static_cast<int>(table_bounds.x() + 260.0f),
        static_cast<int>(table_bounds.y() + 20.0f));

    EXPECT_LT(SkColorGetR(first_color), 80);
    EXPECT_GT(SkColorGetG(first_color), 200);
    EXPECT_LT(SkColorGetB(first_color), 80);
    EXPECT_GT(SkColorGetR(third_color), 200);
    EXPECT_LT(SkColorGetG(third_color), 80);
    EXPECT_LT(SkColorGetB(third_color), 80);
}

TEST_F(TableLayoutBaselineTest, TableLayoutFixedUsesColumnWidthsBeforeCellContent) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table" style="width: 300px; table-layout: fixed; border-spacing: 0;">
                <colgroup>
                    <col style="width: 180px;">
                </colgroup>
                <tbody>
                    <tr>
                        <td id="cell-a" style="border: 0; padding: 0;">A</td>
                        <td id="cell-b" style="border: 0; padding: 0;">Very very very wide cell content</td>
                        <td id="cell-c" style="border: 0; padding: 0;">C</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto table = std::dynamic_pointer_cast<RenderTable>(RenderObjectForId("table"));
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->GetColumnCount(), 3u);
    EXPECT_NEAR(table->GetColumnWidths()[0], 180.0f, 0.5f);
    EXPECT_NEAR(table->GetColumnWidths()[1], 60.0f, 0.5f);
    EXPECT_NEAR(table->GetColumnWidths()[2], 60.0f, 0.5f);

    EXPECT_NEAR(RenderObjectForId("cell-a")->GetLayoutInfo().width, 180.0f, 0.5f);
    EXPECT_NEAR(RenderObjectForId("cell-b")->GetLayoutInfo().width, 60.0f, 0.5f);
    EXPECT_NEAR(RenderObjectForId("cell-c")->GetLayoutInfo().width, 60.0f, 0.5f);
}

TEST_F(TableLayoutBaselineTest, BorderSpacingZeroDoesNotLeaveDefaultGaps) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table" style="width: 200px; border-spacing: 0;">
                <tbody>
                    <tr id="row-a">
                        <td id="cell-a">A</td>
                        <td id="cell-b">B</td>
                    </tr>
                    <tr id="row-b">
                        <td id="cell-c">C</td>
                        <td id="cell-d">D</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto cell_a = RenderObjectForId("cell-a");
    auto cell_b = RenderObjectForId("cell-b");
    auto row_a = RenderObjectForId("row-a");
    auto row_b = RenderObjectForId("row-b");
    ASSERT_NE(cell_a, nullptr);
    ASSERT_NE(cell_b, nullptr);
    ASSERT_NE(row_a, nullptr);
    ASSERT_NE(row_b, nullptr);

    EXPECT_NEAR(cell_b->GetLayoutInfo().x,
                cell_a->GetLayoutInfo().x + cell_a->GetLayoutInfo().width,
                0.5f);
    EXPECT_NEAR(row_b->GetLayoutInfo().y,
                row_a->GetLayoutInfo().y + row_a->GetLayoutInfo().height,
                0.5f);
}

TEST_F(TableLayoutBaselineTest, AutoTableHonorsMinWidthAndCellMinWidthsForOverflow) {
    LoadAndLayout(R"(
        <html>
        <body>
            <div id="scroller" style="width: 220px; height: 120px; overflow: auto;">
                <table id="table" style="min-width: 620px; border-spacing: 0;">
                    <tbody>
                        <tr>
                            <td id="cell-a" style="min-width: 140px; padding: 10px;">A</td>
                            <td id="cell-b" style="min-width: 140px; padding: 10px;">B</td>
                            <td id="cell-c" style="min-width: 140px; padding: 10px;">C</td>
                            <td id="cell-d" style="min-width: 140px; padding: 10px;">D</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </body>
        </html>
    )");

    auto scroller = RenderObjectForId("scroller");
    auto table = RenderObjectForId("table");
    auto cell_a = RenderObjectForId("cell-a");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(table, nullptr);
    ASSERT_NE(cell_a, nullptr);

    EXPECT_GE(table->GetLayoutInfo().width, 620.0f);
    EXPECT_GT(table->GetLayoutInfo().width, scroller->GetLayoutInfo().width);
    EXPECT_GE(cell_a->GetLayoutInfo().width, 160.0f);
}

TEST_F(TableLayoutBaselineTest, AutoTableMinWidthInsideFlexScrollerKeepsScrollableGeometry) {
    LoadAndLayout(R"(
        <html>
        <head>
            <style>
                body { margin: 0; }
                main {
                    display: flex;
                    flex-direction: column;
                    gap: 24px;
                    padding: 24px;
                }
                #scroller {
                    width: 520px;
                    height: 260px;
                    overflow: auto;
                }
                table {
                    border-spacing: 0;
                    min-width: 920px;
                    user-select: text;
                }
                th,
                td {
                    min-width: 140px;
                    padding: 10px 12px;
                    white-space: nowrap;
                }
                thead th {
                    position: sticky;
                    top: 0;
                }
                th:first-child,
                td:first-child {
                    position: sticky;
                    left: 0;
                }
            </style>
        </head>
        <body>
            <main>
                <div id="scroller">
                    <table id="table">
                        <caption>Browser table element validation</caption>
                        <thead>
                            <tr>
                                <th>Name</th>
                                <th>Region</th>
                                <th>Status</th>
                                <th>Owner</th>
                                <th>Updated</th>
                                <th>Notes</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td id="first-cell">Alpha</td>
                                <td>North</td>
                                <td>Ready</td>
                                <td>Lee</td>
                                <td>2026-05-21</td>
                                <td>Selectable cell text</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </main>
        </body>
        </html>
    )", 900.0f, 640.0f);

    auto scroller = RenderObjectForId("scroller");
    auto table = RenderObjectForId("table");
    auto first_cell = RenderObjectForId("first-cell");
    ASSERT_NE(scroller, nullptr);
    ASSERT_NE(table, nullptr);
    ASSERT_NE(first_cell, nullptr);

    EXPECT_GE(table->GetLayoutInfo().width, 920.0f);
    EXPECT_GT(table->GetLayoutInfo().width, scroller->GetLayoutInfo().width);
    EXPECT_GT(table->GetLayoutInfo().height, 0.0f);
    EXPECT_GE(first_cell->GetLayoutInfo().width, 164.0f);
}

TEST_F(TableLayoutBaselineTest, CaptionSideBottomPlacesCaptionAfterRows) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table" style="width: 240px;">
                <caption id="caption" style="caption-side: bottom;">Summary</caption>
                <tbody>
                    <tr id="row-a">
                        <td id="cell-a">A</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto caption = RenderObjectForId("caption");
    auto row = RenderObjectForId("row-a");
    ASSERT_NE(caption, nullptr);
    ASSERT_NE(row, nullptr);
    EXPECT_GT(caption->GetLayoutInfo().y, row->GetLayoutInfo().y);
}

TEST_F(TableLayoutBaselineTest, RowspanZeroSpansRemainingRowsInSection) {
    LoadAndLayout(R"(
        <html>
        <body>
            <table id="table" style="border-spacing: 0;">
                <tbody>
                    <tr id="row-a">
                        <td id="rowspan-cell" rowspan="0">All remaining</td>
                        <td id="cell-a">A</td>
                    </tr>
                    <tr id="row-b">
                        <td id="cell-b">B</td>
                    </tr>
                    <tr id="row-c">
                        <td id="cell-c">C</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto rowspan_cell = std::dynamic_pointer_cast<RenderTableCell>(RenderObjectForId("rowspan-cell"));
    auto row_a = RenderObjectForId("row-a");
    auto row_b = RenderObjectForId("row-b");
    auto row_c = RenderObjectForId("row-c");
    ASSERT_NE(rowspan_cell, nullptr);
    ASSERT_NE(row_a, nullptr);
    ASSERT_NE(row_b, nullptr);
    ASSERT_NE(row_c, nullptr);

    EXPECT_EQ(rowspan_cell->GetRowSpan(), 0);
    float expected_height = row_a->GetLayoutInfo().height +
                            row_b->GetLayoutInfo().height +
                            row_c->GetLayoutInfo().height;
    EXPECT_NEAR(rowspan_cell->GetLayoutInfo().height, expected_height, 0.5f);
    EXPECT_GT(RenderObjectForId("cell-b")->GetLayoutInfo().x,
              rowspan_cell->GetLayoutInfo().x + rowspan_cell->GetLayoutInfo().width - 0.5f);
}

}  // namespace test
}  // namespace mbink
