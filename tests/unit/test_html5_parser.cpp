/**
 * @file test_html5_parser.cpp
 * @brief HTML5 解析器测试
 * 
 * 测试 HTML5 解析的各种场景，包括：
 * - 基础解析
 * - 错误处理
 * - 边界情况
 * - HTML5 标准特性
 */

#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <memory>

using namespace lightui;

// ========== 基础解析测试 ==========

TEST(HTML5ParserTest, ParseEmptyHTML) {
    LexborDocument doc;
    
    // 空字符串应该能解析（创建空文档）
    EXPECT_TRUE(doc.ParseHTML(""));
    EXPECT_FALSE(doc.HasErrors());
}

TEST(HTML5ParserTest, ParseMinimalHTML) {
    LexborDocument doc;
    
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Test</title></head>
        <body></body>
        </html>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    EXPECT_FALSE(doc.HasErrors());
    
    auto* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetTagName(), "body");
}

TEST(HTML5ParserTest, ParseWithoutDoctype) {
    LexborDocument doc;
    
    std::string html = R"(
        <html>
        <body><h1>Hello</h1></body>
        </html>
    )";
    
    // 没有 DOCTYPE 也应该能解析（quirks mode）
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
}

TEST(HTML5ParserTest, ParseFragmentHTML) {
    LexborDocument doc;
    
    // 只有片段，没有完整结构
    std::string html = "<div><p>Hello</p></div>";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
    
    // 片段应该被放入 body
    auto children = body->GetChildren();
    EXPECT_GT(children.size(), 0);
}

TEST(HTML5ParserTest, ParseNestedElements) {
    LexborDocument doc;
    
    std::string html = R"(
        <div id="outer">
            <div id="middle">
                <div id="inner">
                    <p>Deep nesting</p>
                </div>
            </div>
        </div>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* outer = doc.GetElementById("outer");
    ASSERT_NE(outer, nullptr);
    
    auto* middle = doc.GetElementById("middle");
    ASSERT_NE(middle, nullptr);
    
    auto* inner = doc.GetElementById("inner");
    ASSERT_NE(inner, nullptr);
}

// ========== 错误处理测试 ==========

TEST(HTML5ParserTest, ParseMalformedHTML) {
    LexborDocument doc;
    
    // 未闭合的标签
    std::string html = "<div><p>Unclosed paragraph</div>";
    
    // Lexbor 应该能容错处理
    EXPECT_TRUE(doc.ParseHTML(html));
}

TEST(HTML5ParserTest, ParseInvalidNesting) {
    LexborDocument doc;
    
    // 无效的嵌套（p 里面不能有 div）
    std::string html = "<p><div>Invalid nesting</div></p>";
    
    // Lexbor 应该能容错处理
    EXPECT_TRUE(doc.ParseHTML(html));
}

TEST(HTML5ParserTest, ParseMismatchedTags) {
    LexborDocument doc;
    
    // 标签不匹配
    std::string html = "<div><span>Text</div></span>";
    
    // Lexbor 应该能容错处理
    EXPECT_TRUE(doc.ParseHTML(html));
}

TEST(HTML5ParserTest, ParseExtraClosingTags) {
    LexborDocument doc;
    
    // 多余的闭合标签
    std::string html = "<div>Text</div></div>";
    
    // Lexbor 应该能容错处理
    EXPECT_TRUE(doc.ParseHTML(html));
}

// ========== HTML 实体测试 ==========

TEST(HTML5ParserTest, ParseNamedEntities) {
    LexborDocument doc;
    
    std::string html = "<p>&lt;&gt;&amp;&quot;&nbsp;</p>";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* p = doc.QuerySelector("p");
    ASSERT_NE(p, nullptr);
    
    std::string text = p->GetTextContent();
    // 实体应该被解码
    EXPECT_NE(text.find("<"), std::string::npos);
    EXPECT_NE(text.find(">"), std::string::npos);
    EXPECT_NE(text.find("&"), std::string::npos);
}

TEST(HTML5ParserTest, ParseNumericEntities) {
    LexborDocument doc;
    
    std::string html = "<p>&#65;&#66;&#67;</p>";  // ABC
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* p = doc.QuerySelector("p");
    ASSERT_NE(p, nullptr);
    
    std::string text = p->GetTextContent();
    EXPECT_NE(text.find("A"), std::string::npos);
    EXPECT_NE(text.find("B"), std::string::npos);
    EXPECT_NE(text.find("C"), std::string::npos);
}

TEST(HTML5ParserTest, ParseHexEntities) {
    LexborDocument doc;
    
    std::string html = "<p>&#x41;&#x42;&#x43;</p>";  // ABC
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* p = doc.QuerySelector("p");
    ASSERT_NE(p, nullptr);
    
    std::string text = p->GetTextContent();
    EXPECT_NE(text.find("A"), std::string::npos);
}

// ========== 特殊元素测试 ==========

TEST(HTML5ParserTest, ParseScriptTag) {
    LexborDocument doc;
    
    std::string html = R"(
        <html>
        <head>
            <script>
                console.log("Hello");
            </script>
        </head>
        <body></body>
        </html>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* script = doc.QuerySelector("script");
    ASSERT_NE(script, nullptr);
}

TEST(HTML5ParserTest, ParseStyleTag) {
    LexborDocument doc;
    
    std::string html = R"(
        <html>
        <head>
            <style>
                body { margin: 0; }
            </style>
        </head>
        <body></body>
        </html>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* style = doc.QuerySelector("style");
    ASSERT_NE(style, nullptr);
}

TEST(HTML5ParserTest, ParseComments) {
    LexborDocument doc;
    
    std::string html = R"(
        <!-- This is a comment -->
        <div>Content</div>
        <!-- Another comment -->
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
}

// ========== DOCTYPE 测试 ==========

TEST(HTML5ParserTest, ParseHTML5Doctype) {
    LexborDocument doc;
    
    std::string html = "<!DOCTYPE html><html><body></body></html>";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    EXPECT_FALSE(doc.HasErrors());
}

TEST(HTML5ParserTest, ParseHTML4Doctype) {
    LexborDocument doc;
    
    std::string html = R"(
        <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
        <html><body></body></html>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
}

TEST(HTML5ParserTest, ParseXHTMLDoctype) {
    LexborDocument doc;
    
    std::string html = R"(
        <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
        <html><body></body></html>
    )";
    
    EXPECT_TRUE(doc.ParseHTML(html));
}

// ========== 边界情况测试 ==========

TEST(HTML5ParserTest, ParseVeryLargeHTML) {
    LexborDocument doc;
    
    // 生成大量元素
    std::string html = "<html><body>";
    for (int i = 0; i < 1000; i++) {
        html += "<div id='div" + std::to_string(i) + "'>Content " + std::to_string(i) + "</div>";
    }
    html += "</body></html>";
    
    EXPECT_TRUE(doc.ParseHTML(html));
    
    auto* div0 = doc.GetElementById("div0");
    ASSERT_NE(div0, nullptr);
    
    auto* div999 = doc.GetElementById("div999");
    ASSERT_NE(div999, nullptr);
}

TEST(HTML5ParserTest, ParseDeeplyNestedHTML) {
    LexborDocument doc;
    
    // 深度嵌套
    std::string html = "<html><body>";
    for (int i = 0; i < 100; i++) {
        html += "<div>";
    }
    html += "Deep content";
    for (int i = 0; i < 100; i++) {
        html += "</div>";
    }
    html += "</body></html>";
    
    EXPECT_TRUE(doc.ParseHTML(html));
}

// ========== 文件解析测试 ==========

TEST(HTML5ParserTest, ParseNonExistentFile) {
    LexborDocument doc;
    
    EXPECT_FALSE(doc.ParseHTMLFile("non_existent_file.html"));
    EXPECT_TRUE(doc.HasErrors());
    
    auto errors = doc.GetErrors();
    EXPECT_GT(errors.size(), 0);
}

// ========== 序列化测试 ==========

TEST(HTML5ParserTest, SerializeToHTML) {
    LexborDocument doc;
    
    std::string html = "<div id='test'><p>Hello</p></div>";
    EXPECT_TRUE(doc.ParseHTML(html));
    
    std::string serialized = doc.SerializeToHTML();
    EXPECT_FALSE(serialized.empty());
    
    // 序列化的 HTML 应该包含原始内容
    EXPECT_NE(serialized.find("test"), std::string::npos);
    EXPECT_NE(serialized.find("Hello"), std::string::npos);
}

