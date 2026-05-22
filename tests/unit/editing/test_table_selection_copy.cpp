#include <gtest/gtest.h>

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/selection/range.h"
#include "core/dom/selection/selection.h"
#include "core/dom/text.h"
#include "core/editing/clipboard_manager.h"
#include "core/editing/selection_manager.h"
#include "test_utils/test_helpers.h"

#include <memory>
#include <string>

namespace mbink {
namespace test {

namespace {

std::shared_ptr<Text> TextChildForId(const std::shared_ptr<Document>& document,
                                     const std::string& id) {
    auto element = document->GetElementById(id);
    if (!element) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<Text>(element->GetFirstChild());
}

}  // namespace

class TableSelectionCopyTest : public DOMTestBase {
protected:
    std::shared_ptr<Document> LoadTableDocument() {
        return CreateDocumentFromHTML(
            "<html><body>"
            "<table id='copy-source-table'>"
            "<tbody>"
            "<tr><td id='a'>Alpha</td><td id='b'>Beta</td></tr>"
            "<tr><td id='c'>Gamma</td><td id='d'>Delta</td></tr>"
            "</tbody>"
            "</table>"
            "</body></html>");
    }
};

TEST_F(TableSelectionCopyTest, SameCellRangeUsesRawSelectedText) {
    auto document = LoadTableDocument();
    auto alpha = TextChildForId(document, "a");
    ASSERT_NE(alpha, nullptr);

    auto range = document->CreateRange();
    range->SetStart(alpha, 1);
    range->SetEnd(alpha, 4);

    EXPECT_EQ(range->ToString(), "lph");
}

TEST_F(TableSelectionCopyTest, ClipboardCopiesSelectedCellsWithTabs) {
    auto document = LoadTableDocument();
    auto alpha = TextChildForId(document, "a");
    auto beta = TextChildForId(document, "b");
    ASSERT_NE(alpha, nullptr);
    ASSERT_NE(beta, nullptr);

    SelectionManager selection_manager;
    ClipboardManager clipboard(&selection_manager, nullptr);

    auto selection = selection_manager.GetSelection(document);
    ASSERT_NE(selection, nullptr);
    selection->SetBaseAndExtent(alpha, 0, beta, 4);

    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Alpha\tBeta");
}

TEST_F(TableSelectionCopyTest, ClipboardCopiesSelectedRowsWithNewlines) {
    auto document = LoadTableDocument();
    auto alpha = TextChildForId(document, "a");
    auto delta = TextChildForId(document, "d");
    ASSERT_NE(alpha, nullptr);
    ASSERT_NE(delta, nullptr);

    SelectionManager selection_manager;
    ClipboardManager clipboard(&selection_manager, nullptr);

    auto selection = selection_manager.GetSelection(document);
    ASSERT_NE(selection, nullptr);
    selection->SetBaseAndExtent(alpha, 0, delta, 5);

    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Alpha\tBeta\nGamma\tDelta");
}

TEST_F(TableSelectionCopyTest, ClipboardShortcutCopiesTableSelection) {
    auto document = LoadTableDocument();
    auto alpha = TextChildForId(document, "a");
    auto beta = TextChildForId(document, "b");
    ASSERT_NE(alpha, nullptr);
    ASSERT_NE(beta, nullptr);

    SelectionManager selection_manager;
    ClipboardManager clipboard(&selection_manager, nullptr);

    auto selection = selection_manager.GetSelection(document);
    ASSERT_NE(selection, nullptr);
    selection->SetBaseAndExtent(alpha, 0, beta, 4);

    ASSERT_TRUE(clipboard.HandleKeyboardShortcut(document, 67, true, false));
    EXPECT_EQ(clipboard.GetText(), "Alpha\tBeta");
}

TEST_F(TableSelectionCopyTest, EmptyCellsStillContributeTabSeparators) {
    auto document = CreateDocumentFromHTML(
        "<html><body>"
        "<table><tbody><tr><td id='a'></td><td id='b'>Beta</td></tr></tbody></table>"
        "</body></html>");
    auto empty_cell = document->GetElementById("a");
    auto beta = TextChildForId(document, "b");
    ASSERT_NE(empty_cell, nullptr);
    ASSERT_NE(beta, nullptr);

    SelectionManager selection_manager;
    ClipboardManager clipboard(&selection_manager, nullptr);

    auto selection = selection_manager.GetSelection(document);
    ASSERT_NE(selection, nullptr);
    selection->SetBaseAndExtent(empty_cell, 0, beta, 4);

    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "\tBeta");
}

TEST_F(TableSelectionCopyTest, BackwardTableSelectionCopiesInDocumentOrder) {
    auto document = LoadTableDocument();
    auto alpha = TextChildForId(document, "a");
    auto delta = TextChildForId(document, "d");
    ASSERT_NE(alpha, nullptr);
    ASSERT_NE(delta, nullptr);

    SelectionManager selection_manager;
    ClipboardManager clipboard(&selection_manager, nullptr);

    auto selection = selection_manager.GetSelection(document);
    ASSERT_NE(selection, nullptr);
    selection->SetBaseAndExtent(delta, 5, alpha, 0);

    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Alpha\tBeta\nGamma\tDelta");
}

}  // namespace test
}  // namespace mbink
