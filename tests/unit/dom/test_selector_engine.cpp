/**
 * @file test_selector_engine.cpp
 * @brief CSS 选择器引擎单元测试
 *
 * 测试内容：
 * - 基本选择器 (标签、ID、类)
 * - 组合选择器 (后代、子代、相邻兄弟、通用兄弟)
 * - 属性选择器
 * - 伪类选择器
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/selector_engine.h"
#include "dom/element.h"
#include "dom/document.h"

namespace lightui {
namespace test {

class SelectorEngineTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        BuildTestDOM();
    }

    void BuildTestDOM() {
        // 构建测试 DOM 结构
        // <div id="container" class="wrapper">
        //   <div class="item first">
        //     <span class="text">Text 1</span>
        //   </div>
        //   <div class="item">
        //     <span class="text highlight">Text 2</span>
        //   </div>
        //   <p class="item last" data-index="3">Text 3</p>
        // </div>

        auto body = doc_->GetBody();

        container_ = doc_->CreateElement("div");
        container_->SetAttribute("id", "container");
        container_->AddClass("wrapper");
        body->AppendChild(container_);

        item1_ = doc_->CreateElement("div");
        item1_->AddClass("item");
        item1_->AddClass("first");
        container_->AppendChild(item1_);

        span1_ = doc_->CreateElement("span");
        span1_->AddClass("text");
        span1_->AppendChild(doc_->CreateTextNode("Text 1"));
        item1_->AppendChild(span1_);

        item2_ = doc_->CreateElement("div");
        item2_->AddClass("item");
        container_->AppendChild(item2_);

        span2_ = doc_->CreateElement("span");
        span2_->AddClass("text");
        span2_->AddClass("highlight");
        span2_->AppendChild(doc_->CreateTextNode("Text 2"));
        item2_->AppendChild(span2_);

        item3_ = doc_->CreateElement("p");
        item3_->AddClass("item");
        item3_->AddClass("last");
        item3_->SetAttribute("data-index", "3");
        item3_->AppendChild(doc_->CreateTextNode("Text 3"));
        container_->AppendChild(item3_);
    }

protected:
    std::shared_ptr<Element> container_;
    std::shared_ptr<Element> item1_;
    std::shared_ptr<Element> item2_;
    std::shared_ptr<Element> item3_;
    std::shared_ptr<Element> span1_;
    std::shared_ptr<Element> span2_;
};

// ========== 基本选择器测试 ==========

TEST_F(SelectorEngineTest, TagSelector) {
    auto results = container_->QuerySelectorAll("div");
    EXPECT_EQ(results.size(), 2);  // item1, item2
}

TEST_F(SelectorEngineTest, IdSelector) {
    auto result = doc_->GetBody()->QuerySelector("#container");
    EXPECT_EQ(result, container_);
}

TEST_F(SelectorEngineTest, ClassSelector) {
    auto results = container_->QuerySelectorAll(".item");
    EXPECT_EQ(results.size(), 3);  // item1, item2, item3
}

TEST_F(SelectorEngineTest, MultipleClassSelector) {
    auto results = container_->QuerySelectorAll(".text.highlight");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], span2_);
}

TEST_F(SelectorEngineTest, UniversalSelector) {
    auto results = item1_->QuerySelectorAll("*");
    EXPECT_GE(results.size(), 1);  // 至少有 span1_
}

// ========== 组合选择器测试 ==========

TEST_F(SelectorEngineTest, DescendantSelector) {
    auto results = container_->QuerySelectorAll("div span");
    EXPECT_EQ(results.size(), 2);  // span1_, span2_
}

TEST_F(SelectorEngineTest, ChildSelector) {
    auto results = container_->QuerySelectorAll("div > span");
    EXPECT_EQ(results.size(), 2);  // span1_, span2_ (直接子元素)
}

TEST_F(SelectorEngineTest, AdjacentSiblingSelector) {
    // .first + .item 应该匹配 item2_
    auto results = container_->QuerySelectorAll(".first + .item");
    EXPECT_EQ(results.size(), 1);
    if (!results.empty()) {
        EXPECT_EQ(results[0], item2_);
    }
}

TEST_F(SelectorEngineTest, GeneralSiblingSelector) {
    // .first ~ .item 应该匹配 item2_ 和 item3_
    auto results = container_->QuerySelectorAll(".first ~ .item");
    EXPECT_EQ(results.size(), 2);
}

// ========== 属性选择器测试 ==========

TEST_F(SelectorEngineTest, AttributeExistsSelector) {
    auto results = container_->QuerySelectorAll("[data-index]");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], item3_);
}

TEST_F(SelectorEngineTest, AttributeEqualsSelector) {
    auto results = container_->QuerySelectorAll("[data-index='3']");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], item3_);
}

TEST_F(SelectorEngineTest, AttributeContainsSelector) {
    // [class*="high"] 应该匹配包含 "high" 的 class
    auto results = container_->QuerySelectorAll("[class*='high']");
    EXPECT_GE(results.size(), 1);
}

TEST_F(SelectorEngineTest, AttributeStartsWithSelector) {
    // [class^="item"] 应该匹配 class 以 "item" 开头的元素
    auto results = container_->QuerySelectorAll("[class^='item']");
    EXPECT_GE(results.size(), 1);
}

TEST_F(SelectorEngineTest, AttributeEndsWithSelector) {
    // [class$="last"] 应该匹配 class 以 "last" 结尾的元素
    auto results = container_->QuerySelectorAll("[class$='last']");
    EXPECT_GE(results.size(), 1);
}

// ========== 伪类选择器测试 ==========

TEST_F(SelectorEngineTest, FirstChildPseudoClass) {
    auto results = container_->QuerySelectorAll(":first-child");
    EXPECT_GE(results.size(), 1);
    // item1_ 应该是 container_ 的第一个子元素
}

TEST_F(SelectorEngineTest, LastChildPseudoClass) {
    auto results = container_->QuerySelectorAll(":last-child");
    EXPECT_GE(results.size(), 1);
    // item3_ 应该是 container_ 的最后一个子元素
}

TEST_F(SelectorEngineTest, NthChildPseudoClass) {
    auto results = container_->QuerySelectorAll(":nth-child(2)");
    EXPECT_GE(results.size(), 1);
}

TEST_F(SelectorEngineTest, NotPseudoClass) {
    auto results = container_->QuerySelectorAll(".item:not(.first)");
    EXPECT_EQ(results.size(), 2);  // item2_, item3_
}

// ========== Matches 测试 ==========

TEST_F(SelectorEngineTest, MatchesSimple) {
    EXPECT_TRUE(container_->Matches("#container"));
    EXPECT_TRUE(container_->Matches(".wrapper"));
    EXPECT_TRUE(container_->Matches("div"));
    EXPECT_FALSE(container_->Matches("span"));
}

TEST_F(SelectorEngineTest, MatchesCompound) {
    EXPECT_TRUE(container_->Matches("div#container"));
    EXPECT_TRUE(container_->Matches("div.wrapper"));
    EXPECT_TRUE(item1_->Matches("div.item.first"));
}

// ========== Closest 测试 ==========

TEST_F(SelectorEngineTest, ClosestSelf) {
    auto result = span1_->Closest("span");
    EXPECT_EQ(result, span1_);
}

TEST_F(SelectorEngineTest, ClosestParent) {
    auto result = span1_->Closest(".item");
    EXPECT_EQ(result, item1_);
}

TEST_F(SelectorEngineTest, ClosestAncestor) {
    auto result = span1_->Closest("#container");
    EXPECT_EQ(result, container_);
}

TEST_F(SelectorEngineTest, ClosestNotFound) {
    auto result = span1_->Closest(".nonexistent");
    EXPECT_EQ(result, nullptr);
}

// ========== 复杂选择器测试 ==========

TEST_F(SelectorEngineTest, ComplexSelector1) {
    // div.item > span.text
    auto results = container_->QuerySelectorAll("div.item > span.text");
    EXPECT_EQ(results.size(), 2);
}

TEST_F(SelectorEngineTest, ComplexSelector2) {
    // #container .item:first-child span
    auto results = doc_->GetBody()->QuerySelectorAll("#container .item:first-child span");
    EXPECT_GE(results.size(), 1);
}

TEST_F(SelectorEngineTest, MultipleSelectors) {
    // div, p (逗号分隔的多个选择器)
    auto results = container_->QuerySelectorAll("div, p");
    EXPECT_EQ(results.size(), 3);  // item1_, item2_, item3_
}

} // namespace test
} // namespace lightui
