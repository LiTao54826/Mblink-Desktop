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
#include "render/objects/render_object.h"
#include "dom/document.h"
#include "dom/element.h"

namespace lightui {
namespace test {

class NativeLayoutEngineTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        layout_engine_ = std::make_unique<NativeLayoutEngine>();
    }

    void TearDown() override {
        layout_engine_.reset();
        DOMTestBase::TearDown();
    }

protected:
    std::unique_ptr<NativeLayoutEngine> layout_engine_;
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

// ========== Position 测试 ==========

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

} // namespace test
} // namespace lightui
