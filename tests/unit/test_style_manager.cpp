/**
 * @file test_style_manager.cpp
 * @brief StyleManager 类的单元测试
 */

#include <gtest/gtest.h>
#include "core/lexbor/style_manager.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "core/lexbor/lexbor_document.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <fstream>

using namespace lightui;

// ========== 基础功能测试 ==========

TEST(StyleManagerTest, Constructor) {
    StyleManager manager;
    EXPECT_EQ(manager.GetStyleSheetCount(), 0);
}

TEST(StyleManagerTest, AddStyleSheet) {
    StyleManager manager;
    
    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(".test { color: red; }");
    
    manager.AddStyleSheet(sheet);
    EXPECT_EQ(manager.GetStyleSheetCount(), 1);
}

TEST(StyleManagerTest, RemoveStyleSheet) {
    StyleManager manager;
    
    auto sheet1 = std::make_shared<LexborStyleSheet>();
    auto sheet2 = std::make_shared<LexborStyleSheet>();
    
    manager.AddStyleSheet(sheet1);
    manager.AddStyleSheet(sheet2);
    EXPECT_EQ(manager.GetStyleSheetCount(), 2);
    
    ASSERT_TRUE(manager.RemoveStyleSheet(sheet1));
    EXPECT_EQ(manager.GetStyleSheetCount(), 1);
    
    EXPECT_FALSE(manager.RemoveStyleSheet(sheet1)); // 已经移除
}

TEST(StyleManagerTest, ClearStyleSheets) {
    StyleManager manager;
    
    auto sheet1 = std::make_shared<LexborStyleSheet>();
    auto sheet2 = std::make_shared<LexborStyleSheet>();
    
    manager.AddStyleSheet(sheet1);
    manager.AddStyleSheet(sheet2);
    EXPECT_EQ(manager.GetStyleSheetCount(), 2);
    
    manager.ClearStyleSheets();
    EXPECT_EQ(manager.GetStyleSheetCount(), 0);
}

// ========== 样式表优先级测试 ==========

TEST(StyleManagerTest, StyleSheetPriority) {
    StyleManager manager;
    
    auto sheet1 = std::make_shared<LexborStyleSheet>();
    sheet1->ParseCSS(".test { color: red; }");
    
    auto sheet2 = std::make_shared<LexborStyleSheet>();
    sheet2->ParseCSS(".test { color: blue; }");
    
    // 添加时指定优先级
    manager.AddStyleSheet(sheet1, 10);
    manager.AddStyleSheet(sheet2, 20); // 优先级更高
    
    EXPECT_EQ(manager.GetStyleSheetCount(), 2);
}

// ========== 内联样式解析测试 ==========

TEST(StyleManagerTest, ParseInlineStyle) {
    StyleManager manager;
    
    std::string style = "color: red; font-size: 14px;";
    auto declarations = manager.ParseInlineStyle(style);
    
    EXPECT_EQ(declarations.size(), 2);
    EXPECT_EQ(declarations["color"], "red");
    EXPECT_EQ(declarations["font-size"], "14px");
}

TEST(StyleManagerTest, ParseInlineStyleWithSpaces) {
    StyleManager manager;
    
    std::string style = "  color  :  red  ;  font-size  :  14px  ;  ";
    auto declarations = manager.ParseInlineStyle(style);
    
    EXPECT_EQ(declarations.size(), 2);
    EXPECT_EQ(declarations["color"], "red");
    EXPECT_EQ(declarations["font-size"], "14px");
}

TEST(StyleManagerTest, ParseInlineStyleEmpty) {
    StyleManager manager;
    
    auto declarations = manager.ParseInlineStyle("");
    EXPECT_EQ(declarations.size(), 0);
}

// ========== CSS文件加载测试 ==========

TEST(StyleManagerTest, LoadCSSFile) {
    // 创建临时CSS文件
    std::string filename = "test_style_manager.css";
    std::ofstream file(filename);
    file << ".test { color: red; }\n";
    file << "#header { background: blue; }\n";
    file.close();
    
    StyleManager manager;
    ASSERT_TRUE(manager.LoadCSSFile(filename));
    EXPECT_EQ(manager.GetStyleSheetCount(), 1);
    
    // 清理
    std::remove(filename.c_str());
}

TEST(StyleManagerTest, LoadCSSFileNotFound) {
    StyleManager manager;
    EXPECT_FALSE(manager.LoadCSSFile("nonexistent_file.css"));
}

// ========== 规则匹配测试 ==========

TEST(StyleManagerTest, GetMatchingRulesTagSelector) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS("div { color: red; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div); // 添加到文档以设置owner_document
    auto rules = manager.GetMatchingRules(div.get());

    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0]->selector, "div");
}

TEST(StyleManagerTest, GetMatchingRulesClassSelector) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(".container { display: flex; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("class", "container");

    auto rules = manager.GetMatchingRules(div.get());

    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0]->selector, ".container");
}

TEST(StyleManagerTest, GetMatchingRulesIDSelector) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS("#header { background: blue; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("id", "header");

    auto rules = manager.GetMatchingRules(div.get());

    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0]->selector, "#header");
}

TEST(StyleManagerTest, GetMatchingRulesNoMatch) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(".container { display: flex; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    // 没有设置class属性

    auto rules = manager.GetMatchingRules(div.get());
    EXPECT_EQ(rules.size(), 0);
}

// ========== 样式计算测试 ==========

TEST(StyleManagerTest, ComputeStyleSimple) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(".test { color: red; font-size: 14px; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("class", "test");

    auto computed = manager.ComputeStyle(div.get());

    EXPECT_EQ(computed.size(), 2);
    EXPECT_EQ(computed["color"], "red");
    EXPECT_EQ(computed["font-size"], "14px");
}

TEST(StyleManagerTest, ComputeStyleWithInline) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(".test { color: red; font-size: 14px; }");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("class", "test");
    div->SetAttribute("style", "color: blue;"); // 内联样式覆盖

    auto computed = manager.ComputeStyle(div.get());

    EXPECT_EQ(computed["color"], "blue"); // 内联样式优先
    EXPECT_EQ(computed["font-size"], "14px");
}

TEST(StyleManagerTest, ComputeStyleMultipleRules) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto sheet = std::make_shared<LexborStyleSheet>();
    sheet->ParseCSS(R"(
        div { color: red; }
        .container { font-size: 14px; }
    )");
    manager.AddStyleSheet(sheet);

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("class", "container");

    auto computed = manager.ComputeStyle(div.get());

    EXPECT_GE(computed.size(), 2);
    EXPECT_EQ(computed["color"], "red");
    EXPECT_EQ(computed["font-size"], "14px");
}

TEST(StyleManagerTest, ComputeStyleEmpty) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    auto computed = manager.ComputeStyle(div.get());

    EXPECT_EQ(computed.size(), 0);
}

// ========== <style>元素解析测试 ==========

TEST(StyleManagerTest, ParseStyleElement) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto style_elem = std::make_shared<Element>("style");
    doc->AppendChild(style_elem);
    style_elem->SetTextContent(".test { color: red; }");

    ASSERT_TRUE(manager.ParseStyleElement(style_elem.get()));
    EXPECT_EQ(manager.GetStyleSheetCount(), 1);
}

TEST(StyleManagerTest, ParseStyleElementEmpty) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto style_elem = std::make_shared<Element>("style");
    doc->AppendChild(style_elem);
    // 没有设置内容

    EXPECT_FALSE(manager.ParseStyleElement(style_elem.get()));
}

TEST(StyleManagerTest, ParseStyleElementWrongTag) {
    auto doc = std::make_shared<Document>();
    StyleManager manager(doc.get());

    auto div_elem = std::make_shared<Element>("div");
    doc->AppendChild(div_elem);
    div_elem->SetTextContent(".test { color: red; }");

    EXPECT_FALSE(manager.ParseStyleElement(div_elem.get()));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

