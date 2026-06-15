/**
 * @file test_native_layout_engine.cpp
 * @brief NativeLayoutEngine 单元测试
 *
 * 测试内容：
 * - 布局树构建
 * - Block 布局
 * - Flexbox 布局
 * - 增量布局
 */
#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "layout/native_layout_engine.h"
#include "render/css/style_resolver.h"
#include "render/objects/render_object.h"
#include "render/text/font_manager.h"
#include "render/text/text_renderer.h"
#include "lexbor/style_manager.h"
#include "dom/document.h"
#include "dom/element.h"
#include <fstream>
#include <sstream>


namespace mbink {
namespace test {

class NativeLayoutEngineTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        layout_engine_ = std::make_unique<NativeLayoutEngine>();
    }

    void TearDown() override {
        render_root_.reset();
        layout_engine_.reset();
        DOMTestBase::TearDown();
    }

    void BuildAndLayoutFrom(std::shared_ptr<Node> root, float width, float height) {
        RenderTreeBuilder builder;
        builder.SetDocument(doc_.get());
        render_root_ = builder.BuildRenderTree(root);
        layout_engine_->BuildLayoutTree(render_root_);
        layout_engine_->ComputeLayout(width, height);
        layout_engine_->GetLayoutInfo(render_root_);
    }

    void BuildAndLayout(float width, float height) {
        BuildAndLayoutFrom(doc_->GetBody(), width, height);
    }

protected:

static std::string ReadTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    EXPECT_TRUE(file.is_open()) << "Failed to open file: " << path;
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    std::shared_ptr<RenderObject> render_root_;
};



// ========== 基本功能测试 ==========

TEST_F(NativeLayoutEngineTest, CreateEngine) {
    EXPECT_NE(layout_engine_, nullptr);
}

TEST_F(NativeLayoutEngineTest, Clear) {
    layout_engine_->Clear();
    // 清除后应该没有元素
}

// ========== Block 布局测试 ==========

TEST_F(NativeLayoutEngineTest, BlockLayoutBasic) {
    // 创建简单的 block 布局
    auto body = doc_->GetBody();
    auto div1 = doc_->CreateElement("div");
    auto div2 = doc_->CreateElement("div");

    div1->SetStyle("width", "100px");
    div1->SetStyle("height", "50px");
    div2->SetStyle("width", "100px");
    div2->SetStyle("height", "50px");

    body->AppendChild(div1);
    body->AppendChild(div2);

    // 构建渲染树并布局
    // 注意：这需要完整的渲染管线支持
}

TEST_F(NativeLayoutEngineTest, BlockLayoutAutoWidth) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");

    div->SetStyle("height", "100px");
    // width 默认为 auto，应该填充父容器

    body->AppendChild(div);
}

TEST_F(NativeLayoutEngineTest, BlockLayoutMargin) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");

    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");
    div->SetStyle("margin", "10px");

    body->AppendChild(div);
}

TEST_F(NativeLayoutEngineTest, BlockLayoutPadding) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");

    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");
    div->SetStyle("padding", "20px");

    body->AppendChild(div);
}

// ========== Flexbox 布局测试 ==========

TEST_F(NativeLayoutEngineTest, FlexLayoutRow) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("flex-direction", "row");
    container->SetStyle("width", "300px");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");
    auto item3 = doc_->CreateElement("div");

    item1->SetStyle("width", "100px");
    item1->SetStyle("height", "50px");
    item2->SetStyle("width", "100px");
    item2->SetStyle("height", "50px");
    item3->SetStyle("width", "100px");
    item3->SetStyle("height", "50px");

    container->AppendChild(item1);
    container->AppendChild(item2);
    container->AppendChild(item3);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexLayoutColumn) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("flex-direction", "column");
    container->SetStyle("height", "300px");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");

    item1->SetStyle("height", "100px");
    item2->SetStyle("height", "100px");

    container->AppendChild(item1);
    container->AppendChild(item2);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexLayoutJustifyContent) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("justify-content", "space-between");
    container->SetStyle("width", "300px");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");

    item1->SetStyle("width", "50px");
    item2->SetStyle("width", "50px");

    container->AppendChild(item1);
    container->AppendChild(item2);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexLayoutAlignItems) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("align-items", "center");
    container->SetStyle("height", "200px");

    auto item = doc_->CreateElement("div");
    item->SetStyle("width", "50px");
    item->SetStyle("height", "50px");

    container->AppendChild(item);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexGrow) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("width", "300px");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");

    item1->SetStyle("flex-grow", "1");
    item2->SetStyle("flex-grow", "2");

    container->AppendChild(item1);
    container->AppendChild(item2);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexShrink) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("width", "200px");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");

    item1->SetStyle("width", "150px");
    item1->SetStyle("flex-shrink", "1");
    item2->SetStyle("width", "150px");
    item2->SetStyle("flex-shrink", "2");

    container->AppendChild(item1);
    container->AppendChild(item2);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexBasis) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("width", "300px");

    auto item = doc_->CreateElement("div");
    item->SetStyle("flex-basis", "100px");

    container->AppendChild(item);
    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, FlexWrap) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");

    container->SetStyle("display", "flex");
    container->SetStyle("flex-wrap", "wrap");
    container->SetStyle("width", "200px");

    for (int i = 0; i < 5; i++) {
        auto item = doc_->CreateElement("div");
        item->SetStyle("width", "80px");
        item->SetStyle("height", "50px");
        container->AppendChild(item);
    }

    body->AppendChild(container);
}

TEST_F(NativeLayoutEngineTest, ColumnFlexUsesFinalNestedWrapHeightForFollowingSiblings) {
    auto body = doc_->GetBody();
    auto stack = doc_->CreateElement("div");
    auto wrap = doc_->CreateElement("div");
    auto button_row = doc_->CreateElement("div");
    auto button = doc_->CreateElement("button");

    body->SetStyle("margin", "0");
    stack->SetStyle("display", "flex");
    stack->SetStyle("flex-direction", "column");
    stack->SetStyle("gap", "12px");
    stack->SetStyle("width", "520px");

    wrap->SetStyle("display", "flex");
    wrap->SetStyle("flex-wrap", "wrap");
    wrap->SetStyle("gap", "10px");
    wrap->SetStyle("width", "520px");

    for (int i = 0; i < 5; ++i) {
        auto field = doc_->CreateElement("div");
        auto label = doc_->CreateElement("label");
        auto control = doc_->CreateElement(i == 1 ? "input" : "select");
        label->AppendChild(doc_->CreateTextNode("Field"));
        field->SetStyle("display", "flex");
        field->SetStyle("flex-direction", "column");
        field->SetStyle("gap", "6px");
        field->SetStyle("flex", i == 4 ? "1 1 520px" : "1 1 220px");
        field->SetStyle("min-width", i == 4 ? "0" : "220px");
        control->SetStyle("height", "38px");
        field->AppendChild(label);
        field->AppendChild(control);
        wrap->AppendChild(field);
    }

    button->SetStyle("height", "34px");
    button->AppendChild(doc_->CreateTextNode("Save"));
    button_row->AppendChild(button);
    stack->AppendChild(wrap);
    stack->AppendChild(button_row);
    body->AppendChild(stack);

    BuildAndLayout(640.0f, 480.0f);

    ASSERT_NE(wrap->GetRenderObject(), nullptr);
    ASSERT_NE(button_row->GetRenderObject(), nullptr);

    const auto& wrap_info = wrap->GetRenderObject()->GetLayoutInfo();
    const auto& button_row_info = button_row->GetRenderObject()->GetLayoutInfo();

    EXPECT_GT(wrap_info.height, 160.0f);
    EXPECT_GE(button_row_info.y, wrap_info.y + wrap_info.height + 11.5f);
}

// ========== Position 测试 ==========

TEST_F(NativeLayoutEngineTest, ColumnFlexStretchItemKeepsCrossSizeInsideParent) {
    auto body = doc_->GetBody();
    auto panel = doc_->CreateElement("section");
    auto header = doc_->CreateElement("div");
    auto scroller = doc_->CreateElement("div");
    auto content = doc_->CreateElement("div");

    body->SetStyle("margin", "0");
    panel->SetStyle("display", "flex");
    panel->SetStyle("flex-direction", "column");
    panel->SetStyle("width", "523px");
    panel->SetStyle("height", "687px");
    panel->SetStyle("overflow", "hidden");

    header->SetStyle("height", "57px");

    scroller->SetStyle("flex", "1 1 auto");
    scroller->SetStyle("min-width", "0");
    scroller->SetStyle("min-height", "0");
    scroller->SetStyle("overflow-y", "auto");
    scroller->SetStyle("padding", "12px");

    content->SetStyle("height", "900px");
    content->SetStyle("width", "100%");

    scroller->AppendChild(content);
    panel->AppendChild(header);
    panel->AppendChild(scroller);
    body->AppendChild(panel);

    BuildAndLayout(640.0f, 760.0f);

    ASSERT_NE(panel->GetRenderObject(), nullptr);
    ASSERT_NE(scroller->GetRenderObject(), nullptr);

    const auto& panel_info = panel->GetRenderObject()->GetLayoutInfo();
    const auto& scroller_info = scroller->GetRenderObject()->GetLayoutInfo();

    EXPECT_LE(scroller_info.x + scroller_info.width, panel_info.x + panel_info.width + 0.5f);
    EXPECT_LE(scroller_info.width, panel_info.width + 0.5f);
}

TEST_F(NativeLayoutEngineTest, ColumnFlexStretchScrollerStaysInsideParentAfterIncrementalLayout) {
    auto body = doc_->GetBody();
    auto panel = doc_->CreateElement("section");
    auto header = doc_->CreateElement("div");
    auto scroller = doc_->CreateElement("div");
    auto content = doc_->CreateElement("div");
    auto text = doc_->CreateTextNode("initial");

    body->SetStyle("margin", "0");

    panel->SetStyle("display", "flex");
    panel->SetStyle("flex-direction", "column");
    panel->SetStyle("width", "523px");
    panel->SetStyle("height", "687px");
    panel->SetStyle("border", "1px solid #d8dee8");
    panel->SetStyle("overflow", "hidden");

    header->SetStyle("height", "57px");
    header->SetStyle("flex-shrink", "0");

    scroller->SetStyle("display", "flex");
    scroller->SetStyle("flex-direction", "column");
    scroller->SetStyle("gap", "12px");
    scroller->SetStyle("flex", "1 1 auto");
    scroller->SetStyle("min-width", "0");
    scroller->SetStyle("min-height", "0");
    scroller->SetStyle("overflow-x", "hidden");
    scroller->SetStyle("overflow-y", "auto");
    scroller->SetStyle("padding", "12px");

    content->SetStyle("height", "900px");
    content->SetStyle("width", "100%");
    content->AppendChild(text);

    scroller->AppendChild(content);
    panel->AppendChild(header);
    panel->AppendChild(scroller);
    body->AppendChild(panel);

    BuildAndLayout(640.0f, 760.0f);

    ASSERT_NE(panel->GetRenderObject(), nullptr);
    ASSERT_NE(scroller->GetRenderObject(), nullptr);
    ASSERT_NE(text->GetRenderObject(), nullptr);

    const auto initial_panel_info = panel->GetRenderObject()->GetLayoutInfo();
    const auto initial_scroller_info = scroller->GetRenderObject()->GetLayoutInfo();

    EXPECT_LE(initial_scroller_info.x + initial_scroller_info.width,
              initial_panel_info.x + initial_panel_info.width + 0.5f);
    EXPECT_LE(initial_scroller_info.width, initial_panel_info.width + 0.5f);

    auto text_render = std::dynamic_pointer_cast<RenderText>(text->GetRenderObject());
    ASSERT_NE(text_render, nullptr);
    text_render->SetText("updated after event");
    layout_engine_->UpdateContentVersion(text_render.get());
    layout_engine_->MarkNeedsLayout(text_render.get());

    EXPECT_TRUE(layout_engine_->ComputeIncrementalLayout(640.0f, 760.0f));
    layout_engine_->GetLayoutInfo(render_root_);

    const auto& panel_info = panel->GetRenderObject()->GetLayoutInfo();
    const auto& scroller_info = scroller->GetRenderObject()->GetLayoutInfo();

    EXPECT_LE(scroller_info.x + scroller_info.width, panel_info.x + panel_info.width + 0.5f);
    EXPECT_LE(scroller_info.width, panel_info.width + 0.5f);
}

TEST_F(NativeLayoutEngineTest, AppLikeRowFlexColumnScrollerStaysInsideCard) {
    auto body = doc_->GetBody();
    auto app = doc_->CreateElement("main");
    auto titlebar = doc_->CreateElement("div");
    auto content = doc_->CreateElement("section");
    auto panel_form = doc_->CreateElement("section");
    auto panel_history = doc_->CreateElement("section");
    auto form_head = doc_->CreateElement("div");
    auto history_head = doc_->CreateElement("div");
    auto form_body = doc_->CreateElement("div");
    auto history_body = doc_->CreateElement("div");
    auto tall_content = doc_->CreateElement("div");
    auto text = doc_->CreateTextNode("initial");

    body->SetStyle("margin", "0");
    body->SetStyle("width", "100%");
    body->SetStyle("height", "100%");
    body->SetStyle("overflow", "hidden");

    app->SetStyle("display", "flex");
    app->SetStyle("flex-direction", "column");
    app->SetStyle("width", "100%");
    app->SetStyle("height", "100%");
    app->SetStyle("min-width", "0");
    app->SetStyle("min-height", "0");
    app->SetStyle("overflow", "hidden");

    titlebar->SetStyle("height", "44px");
    titlebar->SetStyle("min-height", "44px");
    titlebar->SetStyle("flex", "0 0 44px");

    content->SetStyle("display", "flex");
    content->SetStyle("align-items", "stretch");
    content->SetStyle("gap", "14px");
    content->SetStyle("padding", "14px");
    content->SetStyle("flex", "1 1 auto");
    content->SetStyle("min-width", "0");
    content->SetStyle("min-height", "0");
    content->SetStyle("overflow", "hidden");

    auto apply_panel_style = [](const std::shared_ptr<Element>& panel) {
        panel->SetStyle("display", "flex");
        panel->SetStyle("flex-direction", "column");
        panel->SetStyle("flex", "1 1 0");
        panel->SetStyle("border", "1px solid #d8dee8");
        panel->SetStyle("min-width", "0");
        panel->SetStyle("min-height", "0");
        panel->SetStyle("overflow", "hidden");
    };
    apply_panel_style(panel_form);
    apply_panel_style(panel_history);
    panel_form->SetStyle("flex-basis", "46%");
    panel_form->SetStyle("min-width", "360px");
    panel_history->SetStyle("flex-basis", "54%");
    panel_history->SetStyle("min-width", "430px");

    form_head->SetStyle("height", "57px");
    form_head->SetStyle("flex", "0 0 auto");
    history_head->SetStyle("height", "80px");
    history_head->SetStyle("flex", "0 0 auto");

    form_body->SetStyle("display", "flex");
    form_body->SetStyle("flex-direction", "column");
    form_body->SetStyle("gap", "12px");
    form_body->SetStyle("flex", "1 1 auto");
    form_body->SetStyle("min-width", "0");
    form_body->SetStyle("min-height", "0");
    form_body->SetStyle("overflow-x", "hidden");
    form_body->SetStyle("overflow-y", "auto");
    form_body->SetStyle("padding", "12px");

    history_body->SetStyle("flex", "1 1 auto");
    history_body->SetStyle("min-width", "0");
    history_body->SetStyle("min-height", "0");
    history_body->SetStyle("overflow-y", "auto");
    history_body->SetStyle("padding", "12px");

    tall_content->SetStyle("height", "900px");
    tall_content->SetStyle("width", "100%");
    tall_content->AppendChild(text);

    form_body->AppendChild(tall_content);
    panel_form->AppendChild(form_head);
    panel_form->AppendChild(form_body);
    panel_history->AppendChild(history_head);
    panel_history->AppendChild(history_body);
    content->AppendChild(panel_form);
    content->AppendChild(panel_history);
    app->AppendChild(titlebar);
    app->AppendChild(content);
    body->AppendChild(app);

    BuildAndLayoutFrom(body, 1180.0f, 760.0f);

    ASSERT_NE(panel_form->GetRenderObject(), nullptr);
    ASSERT_NE(form_body->GetRenderObject(), nullptr);

    const auto initial_panel_info = panel_form->GetRenderObject()->GetLayoutInfo();
    const auto initial_body_info = form_body->GetRenderObject()->GetLayoutInfo();

    EXPECT_LE(initial_body_info.x + initial_body_info.width,
              initial_panel_info.x + initial_panel_info.width + 0.5f);
    EXPECT_LE(initial_body_info.width, initial_panel_info.width + 0.5f);

    auto text_render = std::dynamic_pointer_cast<RenderText>(text->GetRenderObject());
    ASSERT_NE(text_render, nullptr);
    text_render->SetText("updated after event");
    layout_engine_->UpdateContentVersion(text_render.get());
    layout_engine_->MarkNeedsLayout(text_render.get());

    EXPECT_TRUE(layout_engine_->ComputeIncrementalLayout(1180.0f, 760.0f));
    layout_engine_->GetLayoutInfo(render_root_);

    const auto& panel_info = panel_form->GetRenderObject()->GetLayoutInfo();
    const auto& body_info = form_body->GetRenderObject()->GetLayoutInfo();

    EXPECT_LE(body_info.x + body_info.width, panel_info.x + panel_info.width + 0.5f);
    EXPECT_LE(body_info.width, panel_info.width + 0.5f);
}

TEST_F(NativeLayoutEngineTest, AppLikeCssRowFlexColumnScrollerStaysInsideCard) {
    auto body = doc_->GetBody();
    auto app = doc_->CreateElement("main");
    auto titlebar = doc_->CreateElement("div");
    auto content = doc_->CreateElement("section");
    auto panel_form = doc_->CreateElement("section");
    auto panel_history = doc_->CreateElement("section");
    auto form_head = doc_->CreateElement("div");
    auto history_head = doc_->CreateElement("div");
    auto form_body = doc_->CreateElement("div");
    auto history_body = doc_->CreateElement("div");
    auto tall_content = doc_->CreateElement("div");

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        *, *::before, *::after { box-sizing: border-box; }
        body {
            margin: 0;
            width: 100%;
            height: 100%;
            overflow: hidden;
        }
        .app {
            width: 100%;
            height: 100%;
            min-width: 0;
            min-height: 0;
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }
        .titlebar {
            flex: 0 0 44px;
            min-height: 44px;
        }
        .content {
            flex: 1 1 auto;
            min-width: 0;
            min-height: 0;
            display: flex;
            align-items: stretch;
            gap: 14px;
            padding: 14px;
            overflow: hidden;
        }
        .panel {
            flex: 1 1 0;
            border: 1px solid #d8dee8;
            min-width: 0;
            min-height: 0;
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }
        .panel-form { flex-basis: 46%; min-width: 360px; }
        .panel-history { flex-basis: 54%; min-width: 430px; }
        .panel-head {
            padding: 13px 14px;
            border-bottom: 1px solid #d8dee8;
            display: flex;
            min-width: 0;
            flex: 0 0 auto;
            flex-wrap: wrap;
        }
        .panel-body {
            flex: 1 1 auto;
            min-width: 0;
            min-height: 0;
            overflow-x: hidden;
            overflow-y: auto;
            padding: 12px;
        }
        .stack {
            display: flex;
            flex-direction: column;
            align-items: stretch;
            gap: 12px;
            min-width: 0;
        }
        .tall-content {
            height: 900px;
            width: 100%;
        }
    )"));

    app->SetAttribute("class", "app");
    titlebar->SetAttribute("class", "titlebar");
    content->SetAttribute("class", "content");
    panel_form->SetAttribute("class", "panel panel-form");
    panel_history->SetAttribute("class", "panel panel-history");
    form_head->SetAttribute("class", "panel-head");
    history_head->SetAttribute("class", "panel-head");
    form_body->SetAttribute("class", "panel-body stack");
    history_body->SetAttribute("class", "panel-body");
    tall_content->SetAttribute("class", "tall-content");

    tall_content->AppendChild(doc_->CreateTextNode("content"));
    form_body->AppendChild(tall_content);
    panel_form->AppendChild(form_head);
    panel_form->AppendChild(form_body);
    panel_history->AppendChild(history_head);
    panel_history->AppendChild(history_body);
    content->AppendChild(panel_form);
    content->AppendChild(panel_history);
    app->AppendChild(titlebar);
    app->AppendChild(content);
    body->AppendChild(app);

    BuildAndLayoutFrom(body, 1180.0f, 760.0f);

    ASSERT_NE(panel_form->GetRenderObject(), nullptr);
    ASSERT_NE(form_head->GetRenderObject(), nullptr);
    ASSERT_NE(form_body->GetRenderObject(), nullptr);

    const auto& panel_info = panel_form->GetRenderObject()->GetLayoutInfo();
    const auto& head_info = form_head->GetRenderObject()->GetLayoutInfo();
    const auto& body_info = form_body->GetRenderObject()->GetLayoutInfo();
    const float panel_content_width = panel_info.width - 2.0f;

    EXPECT_LE(head_info.x + head_info.width, panel_info.x + panel_info.width + 0.5f);
    EXPECT_LE(body_info.x + body_info.width, panel_info.x + panel_info.width + 0.5f);
    EXPECT_LE(head_info.width, panel_info.width + 0.5f);
    EXPECT_LE(body_info.width, panel_info.width + 0.5f);
    EXPECT_NEAR(head_info.width, panel_content_width, 0.5f);
    EXPECT_NEAR(body_info.width, panel_content_width, 0.5f);
}

TEST_F(NativeLayoutEngineTest, PositionRelative) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");

    div->SetStyle("position", "relative");
    div->SetStyle("top", "10px");
    div->SetStyle("left", "20px");
    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");

    body->AppendChild(div);
}

TEST_F(NativeLayoutEngineTest, PositionAbsolute) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");
    auto child = doc_->CreateElement("div");

    container->SetStyle("position", "relative");
    container->SetStyle("width", "200px");
    container->SetStyle("height", "200px");

    child->SetStyle("position", "absolute");
    child->SetStyle("top", "10px");
    child->SetStyle("right", "10px");
    child->SetStyle("width", "50px");
    child->SetStyle("height", "50px");

    container->AppendChild(child);
    body->AppendChild(container);
}

// ========== 增量布局测试 ==========

TEST_F(NativeLayoutEngineTest, IncrementalLayout) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");

    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");

    body->AppendChild(div);

    // 首次布局
    // layout_engine_->ComputeLayout(800, 600);

    // 修改样式
    div->SetStyle("width", "200px");

    // 增量布局应该只更新脏节点
    // bool updated = layout_engine_->ComputeIncrementalLayout(800, 600);
    // EXPECT_TRUE(updated);
}

// ========== 嵌套布局测试 ==========

TEST_F(NativeLayoutEngineTest, NestedFlexbox) {
    auto body = doc_->GetBody();

    auto outer = doc_->CreateElement("div");
    outer->SetStyle("display", "flex");
    outer->SetStyle("flex-direction", "column");
    outer->SetStyle("width", "400px");
    outer->SetStyle("height", "400px");

    auto inner = doc_->CreateElement("div");
    inner->SetStyle("display", "flex");
    inner->SetStyle("flex-direction", "row");
    inner->SetStyle("flex-grow", "1");

    auto item1 = doc_->CreateElement("div");
    auto item2 = doc_->CreateElement("div");

    item1->SetStyle("flex-grow", "1");
    item2->SetStyle("flex-grow", "1");

    inner->AppendChild(item1);
    inner->AppendChild(item2);
    outer->AppendChild(inner);
    body->AppendChild(outer);
}

TEST_F(NativeLayoutEngineTest, ColumnFlexAutoMarginScrollChildFillsRemainingHeight) {
    auto body = doc_->GetBody();
    auto shell = doc_->CreateElement("div");
    auto aside = doc_->CreateElement("aside");
    auto menu = doc_->CreateElement("div");
    auto footer = doc_->CreateElement("div");
    auto main = doc_->CreateElement("main");

    shell->SetStyle("display", "flex");
    shell->SetStyle("width", "800px");
    shell->SetStyle("height", "600px");

    aside->SetStyle("display", "flex");
    aside->SetStyle("flex-direction", "column");
    aside->SetStyle("width", "240px");

    auto header = doc_->CreateElement("div");
    header->SetStyle("height", "100px");
    header->SetStyle("flex-shrink", "0");

    menu->SetStyle("display", "flex");
    menu->SetStyle("flex-direction", "column");
    menu->SetStyle("flex-grow", "1");
    menu->SetStyle("flex-shrink", "1");
    menu->SetStyle("flex-basis", "0px");
    menu->SetStyle("overflow-y", "auto");

    footer->SetStyle("height", "80px");
    footer->SetStyle("margin-top", "auto");
    footer->SetStyle("flex-shrink", "0");

    main->SetStyle("flex-grow", "1");

    aside->AppendChild(header);
    aside->AppendChild(menu);
    aside->AppendChild(footer);
    shell->AppendChild(aside);
    shell->AppendChild(main);
    body->AppendChild(shell);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(aside->GetRenderObject(), nullptr);
    ASSERT_NE(menu->GetRenderObject(), nullptr);
    ASSERT_NE(footer->GetRenderObject(), nullptr);

    const auto& aside_info = aside->GetRenderObject()->GetLayoutInfo();
    const auto& menu_info = menu->GetRenderObject()->GetLayoutInfo();
    const auto& footer_info = footer->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(aside_info.height, 600.0f);
    EXPECT_FLOAT_EQ(menu_info.y, 100.0f);
    EXPECT_FLOAT_EQ(menu_info.height, 420.0f);
    EXPECT_FLOAT_EQ(footer_info.y, 520.0f);
    EXPECT_FLOAT_EQ(footer_info.y + footer_info.height, aside_info.height);
    EXPECT_FLOAT_EQ(menu_info.y + menu_info.height, footer_info.y);
}

TEST_F(NativeLayoutEngineTest, PercentHeightDefiniteNativeFlexBoundaryUsesParentContentHeight) {
    auto body = doc_->GetBody();
    auto root = doc_->CreateElement("div");
    auto shell = doc_->CreateElement("div");
    auto aside = doc_->CreateElement("aside");
    auto fill = doc_->CreateElement("div");
    auto main = doc_->CreateElement("main");

    body->SetStyle("height", "100%");
    body->SetStyle("margin", "0");

    root->SetStyle("height", "100%");
    root->SetStyle("width", "100%");

    shell->SetStyle("display", "flex");
    shell->SetStyle("height", "100%");
    shell->SetStyle("width", "100%");

    aside->SetStyle("display", "flex");
    aside->SetStyle("flex-direction", "column");
    aside->SetStyle("width", "240px");
    aside->SetStyle("padding", "20px 10px");

    fill->SetStyle("height", "100%");
    fill->SetStyle("width", "100%");

    main->SetStyle("flex", "1");

    aside->AppendChild(fill);
    shell->AppendChild(aside);
    shell->AppendChild(main);
    root->AppendChild(shell);
    body->AppendChild(root);

    BuildAndLayoutFrom(body, 900.0f, 640.0f);

    ASSERT_NE(shell->GetRenderObject(), nullptr);
    ASSERT_NE(aside->GetRenderObject(), nullptr);
    ASSERT_NE(fill->GetRenderObject(), nullptr);

    const auto& shell_info = shell->GetRenderObject()->GetLayoutInfo();
    const auto& aside_info = aside->GetRenderObject()->GetLayoutInfo();
    const auto& fill_info = fill->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(shell_info.height, 640.0f);
    EXPECT_FLOAT_EQ(aside_info.height, 640.0f);
    EXPECT_FLOAT_EQ(fill_info.y, 20.0f);
    EXPECT_FLOAT_EQ(fill_info.height, 600.0f);
}

TEST_F(NativeLayoutEngineTest, OverflowAutoUsesDefiniteContainerHeightInNativeFlexShell) {
    auto body = doc_->GetBody();
    auto shell = doc_->CreateElement("div");
    auto pane = doc_->CreateElement("div");
    auto scroller = doc_->CreateElement("div");
    auto item = doc_->CreateElement("div");
    auto footer = doc_->CreateElement("div");
    auto main = doc_->CreateElement("main");

    body->SetStyle("height", "100%");
    body->SetStyle("margin", "0");

    shell->SetStyle("display", "flex");
    shell->SetStyle("height", "100%");
    shell->SetStyle("width", "100%");

    pane->SetStyle("display", "flex");
    pane->SetStyle("flex-direction", "column");
    pane->SetStyle("width", "240px");

    scroller->SetStyle("display", "flex");
    scroller->SetStyle("flex-direction", "column");
    scroller->SetStyle("flex", "1");
    scroller->SetStyle("overflow-y", "auto");

    item->SetStyle("height", "700px");

    footer->SetStyle("height", "80px");
    footer->SetStyle("margin-top", "auto");
    footer->SetStyle("flex-shrink", "0");

    main->SetStyle("flex", "1");

    scroller->AppendChild(item);
    pane->AppendChild(scroller);
    pane->AppendChild(footer);
    shell->AppendChild(pane);
    shell->AppendChild(main);
    body->AppendChild(shell);

    BuildAndLayoutFrom(body, 900.0f, 640.0f);

    ASSERT_NE(pane->GetRenderObject(), nullptr);
    ASSERT_NE(scroller->GetRenderObject(), nullptr);
    ASSERT_NE(footer->GetRenderObject(), nullptr);

    const auto& pane_info = pane->GetRenderObject()->GetLayoutInfo();
    const auto& scroller_info = scroller->GetRenderObject()->GetLayoutInfo();
    const auto& footer_info = footer->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(pane_info.height, 640.0f);
    EXPECT_FLOAT_EQ(scroller_info.height, 560.0f);
    EXPECT_FLOAT_EQ(footer_info.y, 560.0f);
    EXPECT_FLOAT_EQ(scroller_info.y + scroller_info.height, footer_info.y);
}

TEST_F(NativeLayoutEngineTest, RealPageLikeColumnFlexAutoMarginScrollChildFillsRemainingHeight) {
    auto body = doc_->GetBody();
    auto root = doc_->CreateElement("div");
    auto shell = doc_->CreateElement("div");
    auto aside = doc_->CreateElement("aside");
    auto header = doc_->CreateElement("div");
    auto menu = doc_->CreateElement("div");
    auto footer = doc_->CreateElement("div");
    auto footerTitle = doc_->CreateElement("div");
    auto main = doc_->CreateElement("main");

    body->SetStyle("height", "100%");
    body->SetStyle("margin", "0");

    root->SetAttribute("id", "root");
    root->SetStyle("height", "100%");
    root->SetStyle("width", "100%");

    shell->SetStyle("display", "flex");
    shell->SetStyle("height", "100%");
    shell->SetStyle("width", "100%");
    shell->SetStyle("overflow", "hidden");

    aside->SetStyle("display", "flex");
    aside->SetStyle("flex-direction", "column");
    aside->SetStyle("width", "240px");
    aside->SetStyle("gap", "8px");
    aside->SetStyle("padding", "24px 16px");

    header->SetStyle("padding", "0 8px 24px 8px");
    header->SetStyle("flex-shrink", "0");
    auto logo = doc_->CreateElement("div");
    logo->SetStyle("height", "32px");
    header->AppendChild(logo);

    menu->SetStyle("display", "flex");
    menu->SetStyle("flex-direction", "column");
    menu->SetStyle("gap", "4px");
    menu->SetStyle("flex", "1");
    menu->SetStyle("overflow-y", "auto");
    for (int i = 0; i < 4; ++i) {
        auto item = doc_->CreateElement("button");
        item->SetStyle("display", "flex");
        item->SetStyle("padding", "10px 16px");
        item->SetStyle("height", "40px");
        menu->AppendChild(item);
    }

    footer->SetStyle("display", "flex");
    footer->SetStyle("flex-direction", "column");
    footer->SetStyle("gap", "8px");
    footer->SetStyle("margin-top", "auto");
    footer->SetStyle("padding-top", "16px");
    footer->SetStyle("border-top", "1px solid rgba(9,30,66,0.08)");
    footer->SetStyle("flex-shrink", "0");
    footerTitle->SetStyle("height", "24px");
    footer->AppendChild(footerTitle);
    for (int i = 0; i < 6; ++i) {
        auto action = doc_->CreateElement("button");
        action->SetStyle("height", "32px");
        footer->AppendChild(action);
    }

    main->SetStyle("display", "flex");
    main->SetStyle("flex-direction", "column");
    main->SetStyle("flex", "1");
    main->SetStyle("gap", "20px");
    main->SetStyle("min-width", "0");
    main->SetStyle("padding", "24px 32px");
    main->SetStyle("overflow-y", "auto");
    main->SetStyle("margin", "12px 12px 12px 0");
    auto panel = doc_->CreateElement("div");
    panel->SetStyle("height", "720px");
    main->AppendChild(panel);

    aside->AppendChild(header);
    aside->AppendChild(menu);
    aside->AppendChild(footer);
    shell->AppendChild(aside);
    shell->AppendChild(main);
    root->AppendChild(shell);
    body->AppendChild(root);

    BuildAndLayoutFrom(body, 1280.0f, 800.0f);

    ASSERT_NE(root->GetRenderObject(), nullptr);
    ASSERT_NE(shell->GetRenderObject(), nullptr);
    ASSERT_NE(aside->GetRenderObject(), nullptr);
    ASSERT_NE(menu->GetRenderObject(), nullptr);
    ASSERT_NE(footer->GetRenderObject(), nullptr);

    const auto& root_info = root->GetRenderObject()->GetLayoutInfo();
    const auto& shell_info = shell->GetRenderObject()->GetLayoutInfo();
    const auto& aside_info = aside->GetRenderObject()->GetLayoutInfo();
    const auto& menu_info = menu->GetRenderObject()->GetLayoutInfo();
    const auto& footer_info = footer->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(root_info.height, 800.0f);
    EXPECT_FLOAT_EQ(shell_info.height, 800.0f);
    EXPECT_FLOAT_EQ(aside_info.height, 800.0f);
    EXPECT_FLOAT_EQ(footer_info.y + footer_info.height, aside_info.height - 24.0f);
    EXPECT_FLOAT_EQ(footer_info.y - (menu_info.y + menu_info.height), 8.0f);
    EXPECT_GT(menu_info.height, 0.0f);
}

TEST_F(NativeLayoutEngineTest, GridItemPercentHeightFlexSidebarUsesGridAreaHeight) {
    auto body = doc_->GetBody();
    auto root = doc_->CreateElement("div");
    auto shell = doc_->CreateElement("div");
    auto aside = doc_->CreateElement("aside");
    auto menu = doc_->CreateElement("nav");
    auto footer = doc_->CreateElement("footer");
    auto main = doc_->CreateElement("main");
    auto workspace_card = doc_->CreateElement("section");

    body->SetStyle("width", "100%");
    body->SetStyle("height", "100%");
    body->SetStyle("margin", "0");

    root->SetAttribute("id", "root");
    root->SetStyle("width", "100%");
    root->SetStyle("height", "100%");

    shell->SetStyle("display", "grid");
    shell->SetStyle("grid-template-columns", "264px minmax(0, 1fr)");
    shell->SetStyle("width", "100%");
    shell->SetStyle("height", "100%");
    shell->SetStyle("overflow", "hidden");

    aside->SetStyle("display", "flex");
    aside->SetStyle("flex-direction", "column");
    aside->SetStyle("width", "100%");
    aside->SetStyle("height", "100%");
    aside->SetStyle("min-height", "100%");
    aside->SetStyle("max-height", "100%");
    aside->SetStyle("padding", "24px 18px");
    aside->SetStyle("box-sizing", "border-box");

    menu->SetStyle("display", "flex");
    menu->SetStyle("flex-direction", "column");
    menu->SetStyle("flex", "1 1 auto");
    menu->SetStyle("overflow-y", "auto");
    for (int i = 0; i < 7; ++i) {
        auto item = doc_->CreateElement("button");
        item->SetStyle("height", "36px");
        item->SetStyle("flex-shrink", "0");
        menu->AppendChild(item);
    }

    footer->SetStyle("height", "82px");
    footer->SetStyle("margin-top", "auto");
    footer->SetStyle("flex-shrink", "0");

    main->SetStyle("width", "100%");
    main->SetStyle("height", "100%");
    main->SetStyle("display", "flex");
    main->SetStyle("flex-direction", "column");
    main->SetStyle("padding", "24px 32px");
    main->SetStyle("box-sizing", "border-box");
    main->SetStyle("overflow-y", "auto");

    workspace_card->SetStyle("height", "460px");
    workspace_card->SetStyle("flex-shrink", "0");
    main->AppendChild(workspace_card);

    aside->AppendChild(menu);
    aside->AppendChild(footer);
    shell->AppendChild(aside);
    shell->AppendChild(main);
    root->AppendChild(shell);
    body->AppendChild(root);

    BuildAndLayoutFrom(body, 1280.0f, 760.0f);

    ASSERT_NE(shell->GetRenderObject(), nullptr);
    ASSERT_NE(aside->GetRenderObject(), nullptr);
    ASSERT_NE(menu->GetRenderObject(), nullptr);
    ASSERT_NE(footer->GetRenderObject(), nullptr);
    ASSERT_NE(main->GetRenderObject(), nullptr);

    const auto& shell_info = shell->GetRenderObject()->GetLayoutInfo();
    const auto& aside_info = aside->GetRenderObject()->GetLayoutInfo();
    const auto& menu_info = menu->GetRenderObject()->GetLayoutInfo();
    const auto& footer_info = footer->GetRenderObject()->GetLayoutInfo();
    const auto& main_info = main->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(shell_info.width, 1280.0f);
    EXPECT_FLOAT_EQ(shell_info.height, 760.0f);
    EXPECT_FLOAT_EQ(aside_info.width, 264.0f);
    EXPECT_FLOAT_EQ(aside_info.height, 760.0f);
    EXPECT_FLOAT_EQ(main_info.width, 1016.0f);
    EXPECT_FLOAT_EQ(main_info.height, 760.0f);
    EXPECT_FLOAT_EQ(footer_info.y + footer_info.height, 736.0f);
    EXPECT_FLOAT_EQ(menu_info.y + menu_info.height, footer_info.y);
}

TEST_F(NativeLayoutEngineTest, PercentMinMaxHeightResolvesAgainstDefiniteParentHeight) {
    auto body = doc_->GetBody();
    auto host = doc_->CreateElement("div");
    auto min_child = doc_->CreateElement("div");
    auto max_child = doc_->CreateElement("div");

    host->SetStyle("width", "300px");
    host->SetStyle("height", "400px");

    min_child->SetStyle("min-height", "50%");
    min_child->SetStyle("width", "100px");
    min_child->AppendChild(doc_->CreateTextNode("min"));

    max_child->SetStyle("height", "350px");
    max_child->SetStyle("max-height", "25%");
    max_child->SetStyle("width", "100px");
    max_child->AppendChild(doc_->CreateTextNode("max"));

    host->AppendChild(min_child);
    host->AppendChild(max_child);
    body->AppendChild(host);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(min_child->GetRenderObject(), nullptr);
    ASSERT_NE(max_child->GetRenderObject(), nullptr);

    const auto& min_info = min_child->GetRenderObject()->GetLayoutInfo();
    const auto& max_info = max_child->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(min_info.height, 200.0f);
    EXPECT_FLOAT_EQ(max_info.height, 100.0f);
}

TEST_F(NativeLayoutEngineTest, PercentMinMaxHeightWithIndefiniteParentDoesNotClampToZero) {
    auto body = doc_->GetBody();
    auto host = doc_->CreateElement("div");
    auto child = doc_->CreateElement("div");

    host->SetStyle("width", "300px");

    child->SetStyle("height", "120px");
    child->SetStyle("min-height", "50%");
    child->SetStyle("max-height", "50%");
    child->SetStyle("width", "100px");
    child->AppendChild(doc_->CreateTextNode("content"));

    host->AppendChild(child);
    body->AppendChild(host);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(child->GetRenderObject(), nullptr);

    const auto& child_info = child->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(child_info.height, 120.0f);
}

TEST_F(NativeLayoutEngineTest, AbsoluteChildVerticalPaddingPercentUsesContainingBlockWidth) {
    auto body = doc_->GetBody();
    auto host = doc_->CreateElement("div");
    auto text = doc_->CreateTextNode("inline");
    auto abs = doc_->CreateElement("span");

    host->SetStyle("position", "relative");
    host->SetStyle("width", "300px");
    host->SetStyle("height", "200px");

    abs->SetStyle("position", "absolute");
    abs->SetStyle("width", "40px");
    abs->SetStyle("height", "20px");
    abs->SetStyle("padding-top", "10%");
    abs->SetStyle("padding-bottom", "10%");

    host->AppendChild(text);
    host->AppendChild(abs);
    body->AppendChild(host);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(abs->GetRenderObject(), nullptr);

    const auto& abs_info = abs->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(abs_info.height, 80.0f);
}

TEST_F(NativeLayoutEngineTest, FixedChildVerticalPaddingPercentUsesViewportWidth) {
    auto body = doc_->GetBody();
    auto host = doc_->CreateElement("div");
    auto text = doc_->CreateTextNode("inline");
    auto fixed = doc_->CreateElement("span");

    host->SetStyle("width", "300px");
    host->SetStyle("height", "200px");

    fixed->SetStyle("position", "fixed");
    fixed->SetStyle("width", "40px");
    fixed->SetStyle("height", "20px");
    fixed->SetStyle("padding-top", "10%");
    fixed->SetStyle("padding-bottom", "10%");

    host->AppendChild(text);
    host->AppendChild(fixed);
    body->AppendChild(host);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(fixed->GetRenderObject(), nullptr);

    const auto& fixed_info = fixed->GetRenderObject()->GetLayoutInfo();

    EXPECT_FLOAT_EQ(fixed_info.height, 180.0f);
}

TEST_F(NativeLayoutEngineTest, GridTrackStyleUpdateInvalidatesIncrementalLayout) {
    auto body = doc_->GetBody();
    auto grid = doc_->CreateElement("section");
    std::vector<std::shared_ptr<Element>> items;
    std::vector<std::shared_ptr<Element>> actions;

    grid->SetStyle("display", "grid");
    grid->SetStyle("grid-template-columns", "repeat(auto-fit, minmax(190px, 1fr))");
    grid->SetStyle("grid-auto-rows", "minmax(134px, auto)");
    grid->SetStyle("gap", "10px");
    grid->SetStyle("width", "856px");

    for (int i = 0; i < 8; ++i) {
        auto item = doc_->CreateElement("article");
        auto action = doc_->CreateElement("button");
        auto label = doc_->CreateTextNode("Action");

        item->SetStyle("display", "grid");
        item->SetStyle("grid-template-rows", "auto minmax(0, 1fr) auto");
        item->SetStyle("gap", "8px");
        item->SetStyle("padding", "12px");
        item->SetStyle("height", "134px");

        action->SetStyle("width", "100%");
        action->AppendChild(label);
        item->AppendChild(action);

        items.push_back(item);
        actions.push_back(action);
        grid->AppendChild(item);
    }
    body->AppendChild(grid);

    BuildAndLayout(1180.0f, 820.0f);

    ASSERT_NE(grid->GetRenderObject(), nullptr);
    ASSERT_NE(items[4]->GetRenderObject(), nullptr);
    ASSERT_NE(actions[0]->GetRenderObject(), nullptr);

    const auto normal_item = items[4]->GetRenderObject()->GetLayoutInfo();
    const auto normal_action = actions[0]->GetRenderObject()->GetLayoutInfo();

    ComputedStyle compact_style = grid->GetRenderObject()->GetComputedStyle();
    compact_style.grid_template_columns = "repeat(auto-fit, minmax(150px, 1fr))";
    compact_style.grid_auto_rows = "minmax(104px, auto)";
    compact_style.column_gap = CSSLength(7.0f, CSSUnit::PX);
    compact_style.row_gap = CSSLength(7.0f, CSSUnit::PX);

    layout_engine_->UpdateStyle(grid->GetRenderObject().get(), compact_style);
    EXPECT_TRUE(layout_engine_->ComputeIncrementalLayout(1180.0f, 820.0f));
    layout_engine_->GetLayoutInfo(render_root_);

    const auto compact_item = items[4]->GetRenderObject()->GetLayoutInfo();
    const auto compact_action = actions[0]->GetRenderObject()->GetLayoutInfo();

    EXPECT_NEAR(compact_item.x, 690.4f, 0.5f);
    EXPECT_NEAR(compact_item.y, 0.0f, 0.5f);
    EXPECT_NE(compact_item.x, normal_item.x);
    EXPECT_LT(compact_action.width, normal_action.width);
}

TEST_F(NativeLayoutEngineTest, GridButtonLaysOutDirectTextChildrenCenteredAfterWrap) {
    auto body = doc_->GetBody();
    auto stack = doc_->CreateElement("div");
    auto refresh_button = doc_->CreateElement("button");
    auto refresh_label = doc_->CreateTextNode("Refresh status");
    auto vscode_button = doc_->CreateElement("button");
    auto vscode_label = doc_->CreateTextNode("Run VS Code again");

    body->SetStyle("margin", "0");

    stack->SetStyle("display", "grid");
    stack->SetStyle("gap", "10px");
    stack->SetStyle("width", "160px");

    auto apply_button_style = [](const std::shared_ptr<Element>& button) {
        button->SetStyle("display", "grid");
        button->SetStyle("place-items", "center");
        button->SetStyle("text-align", "center");
        button->SetStyle("width", "118px");
        button->SetStyle("min-height", "44px");
        button->SetStyle("padding", "6px 12px");
        button->SetStyle("box-sizing", "border-box");
        button->SetStyle("font-size", "14px");
        button->SetStyle("line-height", "1.25");
        button->SetStyle("font-weight", "600");
        button->SetStyle("white-space", "normal");
    };

    apply_button_style(refresh_button);
    apply_button_style(vscode_button);

    refresh_button->AppendChild(refresh_label);
    vscode_button->AppendChild(vscode_label);
    stack->AppendChild(refresh_button);
    stack->AppendChild(vscode_button);
    body->AppendChild(stack);

    BuildAndLayout(320.0f, 200.0f);

    FontDescriptor desc;
    desc.family = "Arial";
    desc.size = 14.0f;
    desc.weight = FontWeight::BOLD;
    SkFont font = FontManager::GetInstance().LoadFont(desc);
    TextRenderer text_renderer(nullptr);

    auto expect_wrapped_button_centered =
        [&](const std::shared_ptr<Element>& button,
            const std::shared_ptr<Node>& label,
            const std::vector<std::string>& expected_lines) {
            ASSERT_NE(button->GetRenderObject(), nullptr);
            ASSERT_NE(label->GetRenderObject(), nullptr);

            const auto& button_info = button->GetRenderObject()->GetLayoutInfo();
            const auto& label_info = label->GetRenderObject()->GetLayoutInfo();
            auto render_label = std::dynamic_pointer_cast<RenderText>(label->GetRenderObject());
            ASSERT_NE(render_label, nullptr);

            EXPECT_NEAR(button_info.width, 118.0f, 0.5f);
            EXPECT_GE(button_info.height, 44.0f);
            EXPECT_GT(label_info.width, 0.0f);
            EXPECT_GT(label_info.height, 0.0f);
            EXPECT_GT(label_info.x, 0.0f);
            EXPECT_GT(label_info.y, 0.0f);
            EXPECT_LE(label_info.x + label_info.width, button_info.width);
            EXPECT_LE(label_info.y + label_info.height, button_info.height);

            ASSERT_EQ(render_label->GetWrappedLines(), expected_lines);
            ASSERT_EQ(render_label->GetWrappedLineXOffsets().size(), expected_lines.size());

            const float content_center_x = 12.0f + (118.0f - 24.0f) / 2.0f;
            for (size_t i = 0; i < expected_lines.size(); ++i) {
                const float line_width = text_renderer.MeasureTextWidthWithEmoji(expected_lines[i], font);
                const float painted_center_x =
                    label_info.x + render_label->GetWrappedLineXOffsets()[i] + line_width / 2.0f;
                EXPECT_NEAR(painted_center_x, content_center_x, 1.0f)
                    << "line " << i << " should be centered in the button content box";
            }
        };

    expect_wrapped_button_centered(refresh_button, refresh_label, {"Refresh", "status"});
    expect_wrapped_button_centered(vscode_button, vscode_label, {"Run VS Code", "again"});
}

TEST_F(NativeLayoutEngineTest, InlineBlockTextChangeInvalidatesNearestLayoutNode) {
    auto body = doc_->GetBody();
    auto header = doc_->CreateElement("header");
    auto spacer = doc_->CreateElement("div");
    auto button = doc_->CreateElement("button");
    auto text = doc_->CreateTextNode("Short");

    header->SetStyle("display", "grid");
    header->SetStyle("grid-template-columns", "minmax(0, 1fr) auto");
    header->SetStyle("width", "600px");

    button->SetStyle("padding", "7px 11px");
    button->SetStyle("border", "1px solid #9fb3c8");
    button->AppendChild(text);

    header->AppendChild(spacer);
    header->AppendChild(button);
    body->AppendChild(header);

    BuildAndLayout(800.0f, 600.0f);

    ASSERT_NE(button->GetRenderObject(), nullptr);
    ASSERT_NE(text->GetRenderObject(), nullptr);

    const float initial_width = button->GetRenderObject()->GetLayoutInfo().width;

    auto text_render = text->GetRenderObject();
    auto render_text = std::dynamic_pointer_cast<RenderText>(text_render);
    ASSERT_NE(render_text, nullptr);

    render_text->SetText("Much longer button label");
    layout_engine_->UpdateContentVersion(render_text.get());
    layout_engine_->MarkNeedsLayout(render_text.get());

    EXPECT_TRUE(layout_engine_->ComputeIncrementalLayout(800.0f, 600.0f));
    layout_engine_->GetLayoutInfo(render_root_);

    const float updated_width = button->GetRenderObject()->GetLayoutInfo().width;

    EXPECT_GT(updated_width, initial_width + 40.0f);
}

} // namespace test
} // namespace mbink
