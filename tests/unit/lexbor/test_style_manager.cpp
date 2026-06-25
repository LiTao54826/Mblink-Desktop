/**
 * @file test_style_manager.cpp
 * @brief StyleManager 单元测试
 */

#include <gtest/gtest.h>
#include "lexbor/style_manager.h"
#include "dom/document.h"
#include "dom/element.h"
#include "render/css/style_resolver.h"

namespace mblink {
namespace test {

class StyleManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        style_manager_ = std::make_unique<StyleManager>();
    }

    void TearDown() override {
        style_manager_.reset();
    }

protected:
    std::unique_ptr<StyleManager> style_manager_;
};

// ========== CSS 解析测试 ==========

TEST_F(StyleManagerTest, ParseSimpleCSS) {
    bool result = style_manager_->ParseCSSString(R"(
        .test {
            color: red;
            font-size: 16px;
        }
    )");
    EXPECT_TRUE(result);
}

TEST_F(StyleManagerTest, ParseMultipleRules) {
    bool result = style_manager_->ParseCSSString(R"(
        .class1 { color: red; }
        .class2 { color: blue; }
        #id1 { color: green; }
    )");
    EXPECT_TRUE(result);
}

TEST_F(StyleManagerTest, ParseComplexSelectors) {
    bool result = style_manager_->ParseCSSString(R"(
        div.container > p.text { color: red; }
        ul li:first-child { font-weight: bold; }
        a:hover { text-decoration: underline; }
    )");
    EXPECT_TRUE(result);
}

// ========== 内联样式解析测试 ==========

TEST_F(StyleManagerTest, ParseInlineStyleBasic) {
    auto styles = style_manager_->ParseInlineStyle("color: red; font-size: 16px;");
    EXPECT_EQ(styles.size(), 2);
    EXPECT_EQ(styles["color"], "red");
    EXPECT_EQ(styles["font-size"], "16px");
}

TEST_F(StyleManagerTest, ParseInlineStyleEmpty) {
    auto styles = style_manager_->ParseInlineStyle("");
    EXPECT_EQ(styles.size(), 0);
}

TEST_F(StyleManagerTest, ParseInlineStyleSingleProperty) {
    auto styles = style_manager_->ParseInlineStyle("color: blue");
    EXPECT_EQ(styles.size(), 1);
    EXPECT_EQ(styles["color"], "blue");
}

TEST_F(StyleManagerTest, StructuralPseudoClassesMatchElementChildPosition) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto list = doc->CreateElement("ul");
    auto first_item = doc->CreateElement("li");
    auto second_item = doc->CreateElement("li");
    auto third_item = doc->CreateElement("li");
    doc->GetBody()->AppendChild(list);
    list->AppendChild(first_item);
    list->AppendChild(doc->CreateTextNode("ignored text"));
    list->AppendChild(second_item);
    list->AppendChild(third_item);

    ASSERT_TRUE(style_manager_->ParseCSSString(R"(
        li:first-child { color: red; }
        li:last-child { background-color: blue; }
        li:nth-child(2) { font-weight: bold; }
        li:nth-child(odd) { text-decoration: underline; }
    )"));

    auto first_style = style_manager_->ComputeStyle(first_item.get());
    auto second_style = style_manager_->ComputeStyle(second_item.get());
    auto third_style = style_manager_->ComputeStyle(third_item.get());

    EXPECT_EQ(first_style["color"], "red");
    EXPECT_EQ(first_style["text-decoration"], "underline");
    EXPECT_FALSE(first_style.contains("font-weight"));
    EXPECT_FALSE(first_style.contains("background-color"));

    EXPECT_EQ(second_style["font-weight"], "bold");
    EXPECT_FALSE(second_style.contains("color"));
    EXPECT_FALSE(second_style.contains("text-decoration"));

    EXPECT_EQ(third_style["background-color"], "blue");
    EXPECT_EQ(third_style["text-decoration"], "underline");
}

TEST_F(StyleManagerTest, TableStickyFirstColumnSelectorComputesStandardProperties) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto table = doc->CreateElement("table");
    auto thead = doc->CreateElement("thead");
    auto row = doc->CreateElement("tr");
    auto first_header = doc->CreateElement("th");
    auto second_header = doc->CreateElement("th");
    doc->GetBody()->AppendChild(table);
    table->AppendChild(thead);
    thead->AppendChild(row);
    row->AppendChild(first_header);
    row->AppendChild(second_header);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        th:first-child,
        td:first-child {
            position: sticky;
            left: 0;
            z-index: 2;
        }
        thead th:first-child {
            z-index: 3;
        }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc->GetStyleManager());

    auto first_style = resolver.ResolveStyle(first_header, nullptr);
    auto second_style = resolver.ResolveStyle(second_header, nullptr);

    EXPECT_EQ(first_style.position, "sticky");
    EXPECT_FLOAT_EQ(first_style.left.ToPx(), 0.0f);
    EXPECT_EQ(first_style.z_index, 3);

    EXPECT_NE(second_style.position, "sticky");
    EXPECT_EQ(second_style.z_index, 0);
}

TEST_F(StyleManagerTest, SelectorListSpecificityDoesNotLeakAcrossBranches) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto row = doc->CreateElement("tr");
    auto first_header = doc->CreateElement("th");
    doc->GetBody()->AppendChild(row);
    row->AppendChild(first_header);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        #unmatched-selector,
        th:first-child {
            z-index: 1;
        }
        tr th:first-child {
            z-index: 3;
        }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc->GetStyleManager());

    auto style = resolver.ResolveStyle(first_header, nullptr);
    EXPECT_EQ(style.z_index, 3);
}

TEST_F(StyleManagerTest, NthChildExpressionWithSpacesMatchesAsSingleSelector) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto list = doc->CreateElement("ul");
    auto first_item = doc->CreateElement("li");
    auto second_item = doc->CreateElement("li");
    auto third_item = doc->CreateElement("li");
    doc->GetBody()->AppendChild(list);
    list->AppendChild(first_item);
    list->AppendChild(second_item);
    list->AppendChild(third_item);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        ul li:nth-child(2n + 1) { color: red; }
    )"));

    auto first_style = doc->GetStyleManager()->ComputeStyle(first_item.get());
    auto second_style = doc->GetStyleManager()->ComputeStyle(second_item.get());
    auto third_style = doc->GetStyleManager()->ComputeStyle(third_item.get());

    EXPECT_EQ(first_style["color"], "red");
    EXPECT_FALSE(second_style.contains("color"));
    EXPECT_EQ(third_style["color"], "red");
}

TEST_F(StyleManagerTest, ChainedPseudoClassesAllHaveToMatch) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto row = doc->CreateElement("tr");
    auto first_header = doc->CreateElement("th");
    doc->GetBody()->AppendChild(row);
    row->AppendChild(first_header);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        th:first-child:hover { color: red; }
    )"));

    auto without_hover = doc->GetStyleManager()->ComputeStyle(first_header.get());
    EXPECT_FALSE(without_hover.contains("color"));

    first_header->SetPseudoClass("hover", true);
    auto with_hover = doc->GetStyleManager()->ComputeStyle(first_header.get());
    EXPECT_EQ(with_hover["color"], "red");
}

TEST_F(StyleManagerTest, SelectorListSplitsOnlyTopLevelCommas) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto target = doc->CreateElement("div");
    target->SetAttribute("data-label", "a,b");
    doc->GetBody()->AppendChild(target);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        div[data-label="a,b"], span { color: red; }
    )"));

    auto style = doc->GetStyleManager()->ComputeStyle(target.get());
    EXPECT_EQ(style["color"], "red");
}

TEST_F(StyleManagerTest, UniversalSelectorListWithPseudoElementsMatchesElementBranch) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto target = doc->CreateElement("div");
    doc->GetBody()->AppendChild(target);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        *, *::before, *::after {
            box-sizing: border-box;
        }
    )"));

    auto rules = doc->GetStyleManager()->GetMatchingRules(target.get());
    ASSERT_FALSE(rules.empty());

    auto style = doc->GetStyleManager()->ComputeStyle(target.get());
    ASSERT_TRUE(style.contains("box-sizing"));
    EXPECT_EQ(style["box-sizing"], "border-box");
}

TEST_F(StyleManagerTest, LaterSameSpecificityRuleWinsBySourceOrder) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto row = doc->CreateElement("tr");
    auto first_header = doc->CreateElement("th");
    doc->GetBody()->AppendChild(row);
    row->AppendChild(first_header);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(R"(
        th:first-child { z-index: 1; }
        th:first-child { z-index: 4; }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc->GetStyleManager());

    auto style = resolver.ResolveStyle(first_header, nullptr);
    EXPECT_EQ(style.z_index, 4);
}

TEST_F(StyleManagerTest, LaterSamePriorityStyleSheetWinsWithSameSpecificity) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto target = doc->CreateElement("div");
    target->SetAttribute("class", "target");
    doc->GetBody()->AppendChild(target);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(".target { color: red; }", 0, "first"));
    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(".target { color: blue; }", 0, "second"));

    auto style = doc->GetStyleManager()->ComputeStyle(target.get());
    EXPECT_EQ(style["color"], "blue");
}

TEST_F(StyleManagerTest, HigherPriorityStyleSheetWinsOverLaterLowerPriorityStyleSheet) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto target = doc->CreateElement("div");
    target->SetAttribute("class", "target");
    doc->GetBody()->AppendChild(target);

    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(".target { color: blue; }", 10, "high"));
    ASSERT_TRUE(doc->GetStyleManager()->ParseCSSString(".target { color: red; }", 0, "low"));

    auto style = doc->GetStyleManager()->ComputeStyle(target.get());
    EXPECT_EQ(style["color"], "blue");
}

// ========== 样式表管理测试 ==========

TEST_F(StyleManagerTest, AddStyleSheet) {
    auto sheet = std::make_shared<LexborStyleSheet>();
    style_manager_->AddStyleSheet(sheet);
    EXPECT_EQ(style_manager_->GetStyleSheetCount(), 1);
}

TEST_F(StyleManagerTest, RemoveStyleSheet) {
    auto sheet = std::make_shared<LexborStyleSheet>();
    style_manager_->AddStyleSheet(sheet);
    EXPECT_EQ(style_manager_->GetStyleSheetCount(), 1);
    
    bool removed = style_manager_->RemoveStyleSheet(sheet);
    EXPECT_TRUE(removed);
    EXPECT_EQ(style_manager_->GetStyleSheetCount(), 0);
}

TEST_F(StyleManagerTest, ClearStyleSheets) {
    auto sheet1 = std::make_shared<LexborStyleSheet>();
    auto sheet2 = std::make_shared<LexborStyleSheet>();
    style_manager_->AddStyleSheet(sheet1);
    style_manager_->AddStyleSheet(sheet2);
    EXPECT_EQ(style_manager_->GetStyleSheetCount(), 2);
    
    style_manager_->ClearStyleSheets();
    EXPECT_EQ(style_manager_->GetStyleSheetCount(), 0);
}

} // namespace test
} // namespace mblink
