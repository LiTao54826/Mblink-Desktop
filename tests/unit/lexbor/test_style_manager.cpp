/**
 * @file test_style_manager.cpp
 * @brief StyleManager 单元测试
 */

#include <gtest/gtest.h>
#include "lexbor/style_manager.h"

namespace mbink {
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
} // namespace mbink
