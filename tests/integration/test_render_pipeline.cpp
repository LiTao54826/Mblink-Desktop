/**
 * @file test_render_pipeline.cpp
 * @brief 渲染管线集成测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"

namespace mbink {
namespace test {

class RenderPipelineTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
    }

    void TearDown() override {
        DOMTestBase::TearDown();
    }
};

// ========== 样式计算测试 ==========

TEST_F(RenderPipelineTest, InlineStyleParsing) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetStyle("width", "100px");
    div->SetStyle("height", "50px");
    div->SetStyle("margin", "10px");
    div->SetStyle("padding", "5px");
    
    EXPECT_EQ(div->GetStyle("width"), "100px");
    EXPECT_EQ(div->GetStyle("height"), "50px");
    EXPECT_EQ(div->GetStyle("margin"), "10px");
    EXPECT_EQ(div->GetStyle("padding"), "5px");
}

TEST_F(RenderPipelineTest, StyleModification) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetStyle("color", "red");
    EXPECT_EQ(div->GetStyle("color"), "red");
    
    div->SetStyle("color", "blue");
    EXPECT_EQ(div->GetStyle("color"), "blue");
}

TEST_F(RenderPipelineTest, MultipleStyleProperties) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetStyle("display", "flex");
    div->SetStyle("flex-direction", "column");
    div->SetStyle("justify-content", "center");
    div->SetStyle("align-items", "center");
    
    EXPECT_EQ(div->GetStyle("display"), "flex");
    EXPECT_EQ(div->GetStyle("flex-direction"), "column");
    EXPECT_EQ(div->GetStyle("justify-content"), "center");
    EXPECT_EQ(div->GetStyle("align-items"), "center");
}

// ========== 伪类测试 ==========

TEST_F(RenderPipelineTest, PseudoClassHover) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    EXPECT_FALSE(div->HasPseudoClass("hover"));
    
    div->SetPseudoClass("hover", true);
    EXPECT_TRUE(div->HasPseudoClass("hover"));
    
    div->SetPseudoClass("hover", false);
    EXPECT_FALSE(div->HasPseudoClass("hover"));
}

TEST_F(RenderPipelineTest, PseudoClassActive) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetPseudoClass("active", true);
    EXPECT_TRUE(div->HasPseudoClass("active"));
}

TEST_F(RenderPipelineTest, PseudoClassFocus) {
    auto doc = CreateDocument();
    auto input = doc->CreateElement("input");
    
    input->SetPseudoClass("focus", true);
    EXPECT_TRUE(input->HasPseudoClass("focus"));
}

TEST_F(RenderPipelineTest, MultiplePseudoClasses) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetPseudoClass("hover", true);
    div->SetPseudoClass("active", true);
    
    EXPECT_TRUE(div->HasPseudoClass("hover"));
    EXPECT_TRUE(div->HasPseudoClass("active"));
    
    auto pseudoClasses = div->GetActivePseudoClasses();
    EXPECT_EQ(pseudoClasses.size(), 2);
}

// ========== DOM 结构测试 ==========

TEST_F(RenderPipelineTest, NestedElements) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto container = doc->CreateElement("div");
    container->SetStyle("display", "flex");
    body->AppendChild(container);
    
    for (int i = 0; i < 3; i++) {
        auto item = doc->CreateElement("div");
        item->SetStyle("width", "100px");
        item->SetStyle("height", "100px");
        container->AppendChild(item);
    }
    
    EXPECT_EQ(container->GetChildNodes().size(), 3);
}

TEST_F(RenderPipelineTest, DeeplyNestedElements) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto current = body;
    for (int i = 0; i < 10; i++) {
        auto child = doc->CreateElement("div");
        child->SetAttribute("data-level", std::to_string(i));
        current->AppendChild(child);
        current = child;
    }
    
    // 验证最深层元素
    EXPECT_EQ(current->GetAttribute("data-level"), "9");
}

// ========== 属性变化测试 ==========

TEST_F(RenderPipelineTest, AttributeChange) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetAttribute("id", "test");
    EXPECT_EQ(div->GetAttribute("id"), "test");
    
    div->SetAttribute("id", "modified");
    EXPECT_EQ(div->GetAttribute("id"), "modified");
    
    div->RemoveAttribute("id");
    EXPECT_EQ(div->GetAttribute("id"), "");
}

TEST_F(RenderPipelineTest, ClassChange) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetClassName("foo bar baz");
    EXPECT_TRUE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("bar"));
    EXPECT_TRUE(div->HasClass("baz"));
    
    div->SetClassName("only-one");
    EXPECT_FALSE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("only-one"));
}

} // namespace test
} // namespace mbink
