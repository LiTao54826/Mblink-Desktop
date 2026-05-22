#include <gtest/gtest.h>

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/selection/range.h"
#include "core/dom/selection/selection.h"
#include "core/dom/text.h"
#include "core/editing/clipboard_manager.h"
#include "core/editing/selection_manager.h"
#include "core/event/dispatch/mouse_event_dispatcher.h"
#include "core/render/css/style_resolver.h"
#include "core/window/window.h"
#include "test_utils/test_helpers.h"

#include <SDL3/SDL.h>
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

#include <algorithm>
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

std::shared_ptr<RenderObject> BuildRenderTreeForDocument(
    const std::shared_ptr<Document>& document,
    float width = 800.0f,
    float height = 600.0f) {
    if (!document || !document->GetBody()) {
        return nullptr;
    }

    RenderTreeBuilder builder;
    builder.SetDocument(document.get());
    auto root = builder.BuildRenderTree(document->GetBody());
    if (!root) {
        return nullptr;
    }

    root->Layout(width, height);
    root->UpdateViewportBounds();
    return root;
}

std::shared_ptr<RenderObject> FindRenderObjectByElementId(
    const std::shared_ptr<RenderObject>& root,
    const std::string& id) {
    if (!root) {
        return nullptr;
    }

    auto element = std::dynamic_pointer_cast<Element>(root->GetNode());
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

std::shared_ptr<RenderObject> FirstTextRenderObject(
    const std::shared_ptr<RenderObject>& root) {
    if (!root) {
        return nullptr;
    }
    if (root->GetType() == RenderObjectType::TEXT) {
        return root;
    }
    for (const auto& child : root->GetChildren()) {
        auto found = FirstTextRenderObject(child);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

SDL_Event MouseButtonEvent(Uint32 type, float x, float y) {
    SDL_Event event{};
    event.type = type;
    event.button.windowID = 1;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    return event;
}

SDL_Event MouseMotionEvent(float x, float y) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = 1;
    event.motion.x = x;
    event.motion.y = y;
    return event;
}

void DragSelectText(RenderObject* text_render,
                    Window* window,
                    const std::shared_ptr<Document>& document,
                    std::shared_ptr<RenderObject> root_render,
                    MouseEventDispatcher* dispatcher) {
    ASSERT_NE(text_render, nullptr);
    ASSERT_NE(window, nullptr);
    ASSERT_NE(dispatcher, nullptr);

    text_render->UpdateViewportBounds();
    const auto& text_bounds = text_render->GetViewportBounds();
    ASSERT_TRUE(text_bounds.valid);

    const float display_scale = window->GetDisplayScale();
    const float y = text_bounds.y + text_bounds.height * 0.5f;
    const float start_x = text_bounds.x + text_bounds.width * 0.20f;
    const float end_x = text_bounds.x + text_bounds.width * 0.80f;

    EXPECT_TRUE(dispatcher->HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, start_x * display_scale, y * display_scale),
        window->shared_from_this(),
        document,
        root_render));
    EXPECT_TRUE(dispatcher->HandleMouseEvent(
        MouseMotionEvent(end_x * display_scale, y * display_scale),
        window->shared_from_this(),
        document,
        root_render));
    EXPECT_TRUE(dispatcher->HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_UP, end_x * display_scale, y * display_scale),
        window->shared_from_this(),
        document,
        root_render));
}

bool ContainsSelectionBluePixel(sk_sp<SkSurface> surface, const SkRect& search_rect) {
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
            const int red = SkColorGetR(color);
            const int green = SkColorGetG(color);
            const int blue = SkColorGetB(color);
            if (blue > red + 25 && blue > green + 10 && green > red) {
                return true;
            }
        }
    }
    return false;
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

TEST_F(TableSelectionCopyTest, MouseDragSelectionCopiesPlainTableCellText) {
    auto document = LoadTableDocument();
    auto root_render = BuildRenderTreeForDocument(document);
    ASSERT_NE(root_render, nullptr);

    auto cell = FindRenderObjectByElementId(root_render, "a");
    ASSERT_NE(cell, nullptr);
    auto text_render = FirstTextRenderObject(cell);
    ASSERT_NE(text_render, nullptr);

    text_render->UpdateViewportBounds();
    const auto& text_bounds = text_render->GetViewportBounds();
    ASSERT_TRUE(text_bounds.valid);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);

    SelectionManager selection_manager;
    MouseEventDispatcher dispatcher;
    dispatcher.SetManagers(nullptr, &selection_manager, nullptr, nullptr);

    const float display_scale = window->GetDisplayScale();
    const float y = text_bounds.y + text_bounds.height * 0.5f;
    const float start_x = text_bounds.x;
    const float end_x = text_bounds.x + text_bounds.width;

    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, start_x * display_scale, y * display_scale),
        window,
        document,
        root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseMotionEvent(end_x * display_scale, y * display_scale), window, document, root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_UP, end_x * display_scale, y * display_scale),
        window,
        document,
        root_render));

    ClipboardManager clipboard(&selection_manager, nullptr);
    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Alpha");
}

TEST_F(TableSelectionCopyTest, MouseDragSelectionProvidesPaintRectsAndVisibleHighlight) {
    auto document = LoadTableDocument();
    auto root_render = BuildRenderTreeForDocument(document);
    ASSERT_NE(root_render, nullptr);

    auto cell = FindRenderObjectByElementId(root_render, "a");
    ASSERT_NE(cell, nullptr);
    auto text_render = FirstTextRenderObject(cell);
    ASSERT_NE(text_render, nullptr);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);

    SelectionManager selection_manager;
    MouseEventDispatcher dispatcher;
    dispatcher.SetManagers(nullptr, &selection_manager, nullptr, nullptr);

    DragSelectText(text_render.get(), window.get(), document, root_render, &dispatcher);

    const auto rects = selection_manager.GetSelectionRects(document);
    ASSERT_FALSE(rects.empty());
    EXPECT_GT(rects.front().width, 0.0f);
    EXPECT_GT(rects.front().height, 0.0f);
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_FALSE(window->GetDirtyRects().empty());

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_NE(surface, nullptr);
    auto canvas = surface->getCanvas();
    ASSERT_NE(canvas, nullptr);
    canvas->clear(SK_ColorWHITE);
    root_render->Paint(canvas);

    const SkRect selection_rect = SkRect::MakeXYWH(rects.front().x,
                                                  rects.front().y,
                                                  rects.front().width,
                                                  rects.front().height);
    EXPECT_TRUE(ContainsSelectionBluePixel(surface, selection_rect.makeOutset(2.0f, 2.0f)));
}

TEST_F(TableSelectionCopyTest, MouseDragPastTextEdgeCopiesPlainTableCellText) {
    auto document = LoadTableDocument();
    auto root_render = BuildRenderTreeForDocument(document);
    ASSERT_NE(root_render, nullptr);

    auto cell = FindRenderObjectByElementId(root_render, "a");
    ASSERT_NE(cell, nullptr);
    auto text_render = FirstTextRenderObject(cell);
    ASSERT_NE(text_render, nullptr);

    text_render->UpdateViewportBounds();
    const auto& text_bounds = text_render->GetViewportBounds();
    ASSERT_TRUE(text_bounds.valid);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);

    SelectionManager selection_manager;
    MouseEventDispatcher dispatcher;
    dispatcher.SetManagers(nullptr, &selection_manager, nullptr, nullptr);

    const float display_scale = window->GetDisplayScale();
    const float y = text_bounds.y + text_bounds.height * 0.5f;
    const float start_x = text_bounds.x;
    const float end_x = text_bounds.x + text_bounds.width + 12.0f;

    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, start_x * display_scale, y * display_scale),
        window,
        document,
        root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseMotionEvent(end_x * display_scale, y * display_scale), window, document, root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_UP, end_x * display_scale, y * display_scale),
        window,
        document,
        root_render));

    ClipboardManager clipboard(&selection_manager, nullptr);
    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Alpha");
}

TEST_F(TableSelectionCopyTest, DoubleClickSelectsAdjacentStringInTableCell) {
    auto document = CreateDocumentFromHTML(
        "<html><body>"
        "<table><tbody><tr><td id='a'>Alpha Beta</td></tr></tbody></table>"
        "</body></html>");
    auto root_render = BuildRenderTreeForDocument(document);
    ASSERT_NE(root_render, nullptr);

    auto cell = FindRenderObjectByElementId(root_render, "a");
    ASSERT_NE(cell, nullptr);
    auto text_render = FirstTextRenderObject(cell);
    ASSERT_NE(text_render, nullptr);

    text_render->UpdateViewportBounds();
    const auto& text_bounds = text_render->GetViewportBounds();
    ASSERT_TRUE(text_bounds.valid);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);

    SelectionManager selection_manager;
    MouseEventDispatcher dispatcher;
    dispatcher.SetManagers(nullptr, &selection_manager, nullptr, nullptr);

    const float display_scale = window->GetDisplayScale();
    const float x = text_bounds.x + text_bounds.width * 0.75f;
    const float y = text_bounds.y + text_bounds.height * 0.5f;

    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, x * display_scale, y * display_scale),
        window,
        document,
        root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_UP, x * display_scale, y * display_scale),
        window,
        document,
        root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, x * display_scale, y * display_scale),
        window,
        document,
        root_render));
    EXPECT_TRUE(dispatcher.HandleMouseEvent(
        MouseButtonEvent(SDL_EVENT_MOUSE_BUTTON_UP, x * display_scale, y * display_scale),
        window,
        document,
        root_render));

    ClipboardManager clipboard(&selection_manager, nullptr);
    ASSERT_TRUE(clipboard.Copy(document));
    EXPECT_EQ(clipboard.GetText(), "Beta");
}

}  // namespace test
}  // namespace mbink
