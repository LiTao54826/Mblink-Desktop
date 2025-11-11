/**
 * @file test_lexbor_stylesheet.cpp
 * @brief LexborStyleSheet 类的单元测试
 */

#include <gtest/gtest.h>
#include "core/lexbor/lexbor_stylesheet.h"
#include <fstream>

using namespace lightui;

// ========== 基础功能测试 ==========

TEST(LexborStyleSheetTest, Constructor) {
    LexborStyleSheet sheet;
    EXPECT_EQ(sheet.GetRuleCount(), 0);
    EXPECT_FALSE(sheet.HasErrors());
}

TEST(LexborStyleSheetTest, ParseSimpleCSS) {
    LexborStyleSheet sheet;
    std::string css = ".test { color: red; }";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->selector, ".test");
    EXPECT_EQ(rule->declarations.size(), 1);
    EXPECT_EQ(rule->declarations.at("color"), "red");
}

TEST(LexborStyleSheetTest, ParseMultipleRules) {
    LexborStyleSheet sheet;
    std::string css = R"(
        .container { display: flex; }
        .container > p { color: blue; }
        #header { background: red; }
    )";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    EXPECT_EQ(sheet.GetRuleCount(), 3);
}

TEST(LexborStyleSheetTest, ParseEmptyCSS) {
    LexborStyleSheet sheet;
    ASSERT_TRUE(sheet.ParseCSS(""));
    EXPECT_EQ(sheet.GetRuleCount(), 0);
}

TEST(LexborStyleSheetTest, ParseComplexSelectors) {
    LexborStyleSheet sheet;
    std::string css = R"(
        div.container { margin: 10px; }
        ul > li:first-child { font-weight: bold; }
        a[href^="https"] { color: green; }
    )";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    EXPECT_GE(sheet.GetRuleCount(), 3);
}

// ========== 属性声明测试 ==========

TEST(LexborStyleSheetTest, ParseMultipleDeclarations) {
    LexborStyleSheet sheet;
    std::string css = R"(
        .box {
            width: 100px;
            height: 200px;
            background-color: blue;
            border: 1px solid black;
        }
    )";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_GE(rule->declarations.size(), 4);
}

TEST(LexborStyleSheetTest, ParseShorthandProperties) {
    LexborStyleSheet sheet;
    std::string css = ".test { margin: 10px 20px; padding: 5px; }";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_GE(rule->declarations.size(), 1);
}

// ========== 文件解析测试 ==========

TEST(LexborStyleSheetTest, ParseCSSFile) {
    // 创建临时 CSS 文件
    std::string filename = "test_temp.css";
    std::ofstream file(filename);
    file << ".test { color: red; }\n";
    file << "#header { background: blue; }\n";
    file.close();
    
    LexborStyleSheet sheet;
    ASSERT_TRUE(sheet.ParseCSSFile(filename));
    EXPECT_EQ(sheet.GetRuleCount(), 2);
    
    // 清理
    std::remove(filename.c_str());
}

TEST(LexborStyleSheetTest, ParseCSSFileNotFound) {
    LexborStyleSheet sheet;
    EXPECT_FALSE(sheet.ParseCSSFile("nonexistent_file.css"));
    EXPECT_TRUE(sheet.HasErrors());
}

// ========== 规则访问测试 ==========

TEST(LexborStyleSheetTest, GetRule) {
    LexborStyleSheet sheet;
    sheet.ParseCSS(".test { color: red; }");
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->selector, ".test");
    
    const CSSRule* invalid = sheet.GetRule(999);
    EXPECT_EQ(invalid, nullptr);
}

TEST(LexborStyleSheetTest, GetRules) {
    LexborStyleSheet sheet;
    sheet.ParseCSS(".a { color: red; } .b { color: blue; }");
    
    const auto& rules = sheet.GetRules();
    EXPECT_EQ(rules.size(), 2);
}

// ========== 规则修改测试 ==========

TEST(LexborStyleSheetTest, AddRule) {
    LexborStyleSheet sheet;
    
    std::map<std::string, std::string> declarations;
    declarations["color"] = "red";
    declarations["font-size"] = "14px";
    
    ASSERT_TRUE(sheet.AddRule(".test", declarations));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->selector, ".test");
    EXPECT_EQ(rule->declarations.size(), 2);
}

TEST(LexborStyleSheetTest, RemoveRule) {
    LexborStyleSheet sheet;
    sheet.ParseCSS(".a { color: red; } .b { color: blue; }");
    
    EXPECT_EQ(sheet.GetRuleCount(), 2);
    ASSERT_TRUE(sheet.RemoveRule(0));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
    
    EXPECT_FALSE(sheet.RemoveRule(999));
}

TEST(LexborStyleSheetTest, ClearRules) {
    LexborStyleSheet sheet;
    sheet.ParseCSS(".a { color: red; } .b { color: blue; }");
    
    EXPECT_EQ(sheet.GetRuleCount(), 2);
    sheet.ClearRules();
    EXPECT_EQ(sheet.GetRuleCount(), 0);
}

// ========== 优先级测试 ==========

TEST(LexborStyleSheetTest, SpecificityIDSelector) {
    LexborStyleSheet sheet;
    sheet.ParseCSS("#header { color: red; }");
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_GE(rule->specificity, 100);
}

TEST(LexborStyleSheetTest, SpecificityClassSelector) {
    LexborStyleSheet sheet;
    sheet.ParseCSS(".container { color: red; }");
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_GE(rule->specificity, 10);
    EXPECT_LT(rule->specificity, 100);
}

TEST(LexborStyleSheetTest, SpecificityTagSelector) {
    LexborStyleSheet sheet;
    sheet.ParseCSS("div { color: red; }");
    
    const CSSRule* rule = sheet.GetRule(0);
    ASSERT_NE(rule, nullptr);
    EXPECT_GE(rule->specificity, 1);
    EXPECT_LT(rule->specificity, 10);
}

// ========== 序列化测试 ==========

TEST(LexborStyleSheetTest, SerializeToCSS) {
    LexborStyleSheet sheet;
    std::string css = ".test { color: red; }";
    
    ASSERT_TRUE(sheet.ParseCSS(css));
    std::string serialized = sheet.SerializeToCSS();
    
    EXPECT_FALSE(serialized.empty());
    EXPECT_NE(serialized.find(".test"), std::string::npos);
}

TEST(LexborStyleSheetTest, SerializeEmptyStyleSheet) {
    LexborStyleSheet sheet;
    std::string serialized = sheet.SerializeToCSS();
    EXPECT_TRUE(serialized.empty());
}

// ========== 移动语义测试 ==========

TEST(LexborStyleSheetTest, MoveConstructor) {
    LexborStyleSheet sheet1;
    sheet1.ParseCSS(".test { color: red; }");
    
    LexborStyleSheet sheet2(std::move(sheet1));
    EXPECT_EQ(sheet2.GetRuleCount(), 1);
}

TEST(LexborStyleSheetTest, MoveAssignment) {
    LexborStyleSheet sheet1;
    sheet1.ParseCSS(".test { color: red; }");
    
    LexborStyleSheet sheet2;
    sheet2 = std::move(sheet1);
    EXPECT_EQ(sheet2.GetRuleCount(), 1);
}

// ========== 错误处理测试 ==========

TEST(LexborStyleSheetTest, ErrorHandling) {
    LexborStyleSheet sheet;
    EXPECT_FALSE(sheet.HasErrors());
    
    // 尝试解析不存在的文件
    sheet.ParseCSSFile("nonexistent.css");
    EXPECT_TRUE(sheet.HasErrors());
    EXPECT_GT(sheet.GetErrors().size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

