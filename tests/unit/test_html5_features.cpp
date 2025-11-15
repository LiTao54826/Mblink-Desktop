#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>

using namespace lightui;

/**
 * @brief HTML5 标准特性测试
 * 
 * 测试 HTML5 标准特性，包括 DOCTYPE、特殊元素、HTML 实体等
 */
class HTML5FeaturesTest : public ::testing::Test {
protected:
    LexborDocument doc;
};

// ========== DOCTYPE 处理测试 ==========

TEST_F(HTML5FeaturesTest, HTML5Doctype) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Test</title></head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 检查文档模式
    std::string mode = doc.GetDocumentMode();
    EXPECT_EQ(mode, "no-quirks");  // HTML5 DOCTYPE 应该是标准模式
    EXPECT_FALSE(doc.IsQuirksMode());
    
    // 检查 DOCTYPE
    std::string doctype = doc.GetDoctype();
    EXPECT_EQ(doctype, "html");
}

TEST_F(HTML5FeaturesTest, HTML4StrictDoctype) {
    std::string html = R"(
        <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
        <html>
        <head><title>Test</title></head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // HTML4 Strict 应该是标准模式
    std::string mode = doc.GetDocumentMode();
    EXPECT_EQ(mode, "no-quirks");
    EXPECT_FALSE(doc.IsQuirksMode());
}

TEST_F(HTML5FeaturesTest, HTML4TransitionalDoctype) {
    std::string html = R"(
        <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
        <html>
        <head><title>Test</title></head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // HTML4 Transitional 可能是 quirks 或 limited-quirks 模式
    std::string mode = doc.GetDocumentMode();
    EXPECT_FALSE(mode.empty());
}

TEST_F(HTML5FeaturesTest, NoDoctype) {
    std::string html = R"(
        <html>
        <head><title>Test</title></head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 没有 DOCTYPE 应该是 quirks 模式
    std::string mode = doc.GetDocumentMode();
    EXPECT_EQ(mode, "quirks");
    EXPECT_TRUE(doc.IsQuirksMode());
}

// ========== 特殊元素处理测试 ==========

TEST_F(HTML5FeaturesTest, ScriptTagParsing) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <script>
                var x = 1;
                var y = "<div>Not HTML</div>";
                if (x < 10 && y > 5) {
                    console.log("test");
                }
            </script>
        </head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto scripts = doc.QuerySelectorAll("script");
    ASSERT_EQ(scripts.size(), 1);
    
    // 验证 script 内容被正确解析（不作为 HTML）
    std::string content = scripts[0]->GetTextContent();
    EXPECT_TRUE(content.find("var x = 1") != std::string::npos);
    EXPECT_TRUE(content.find("<div>Not HTML</div>") != std::string::npos);
}

TEST_F(HTML5FeaturesTest, StyleTagParsing) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body { margin: 0; }
                .test { color: red; }
                /* Comment */
            </style>
        </head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto styles = doc.QuerySelectorAll("style");
    ASSERT_EQ(styles.size(), 1);
    
    // 验证 style 内容被正确解析
    std::string content = styles[0]->GetTextContent();
    EXPECT_TRUE(content.find("body { margin: 0; }") != std::string::npos);
    EXPECT_TRUE(content.find(".test { color: red; }") != std::string::npos);
}

TEST_F(HTML5FeaturesTest, TemplateTagParsing) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <template id="my-template">
                <div class="template-content">
                    <p>Template content</p>
                </div>
            </template>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto templates = doc.QuerySelectorAll("template");
    ASSERT_EQ(templates.size(), 1);
    EXPECT_EQ(templates[0]->GetId(), "my-template");
}

TEST_F(HTML5FeaturesTest, SVGNamespace) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <svg width="100" height="100">
                <circle cx="50" cy="50" r="40" fill="red" />
            </svg>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto svgs = doc.QuerySelectorAll("svg");
    ASSERT_EQ(svgs.size(), 1);
    EXPECT_EQ(svgs[0]->GetAttribute("width"), "100");
    EXPECT_EQ(svgs[0]->GetAttribute("height"), "100");
    
    auto circles = doc.QuerySelectorAll("circle");
    ASSERT_EQ(circles.size(), 1);
    EXPECT_EQ(circles[0]->GetAttribute("cx"), "50");
}

// ========== HTML 实体解析测试 ==========

TEST_F(HTML5FeaturesTest, NamedEntities) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <p>&lt;div&gt; &amp; &quot; &apos; &nbsp;</p>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    
    std::string content = paragraphs[0]->GetTextContent();
    // 验证实体被正确解析
    EXPECT_TRUE(content.find("<div>") != std::string::npos);
    EXPECT_TRUE(content.find("&") != std::string::npos);
    EXPECT_TRUE(content.find("\"") != std::string::npos);
}

TEST_F(HTML5FeaturesTest, NumericEntities) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <p>&#65; &#66; &#67;</p>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    
    std::string content = paragraphs[0]->GetTextContent();
    // &#65; = A, &#66; = B, &#67; = C
    EXPECT_TRUE(content.find("A") != std::string::npos);
    EXPECT_TRUE(content.find("B") != std::string::npos);
    EXPECT_TRUE(content.find("C") != std::string::npos);
}

TEST_F(HTML5FeaturesTest, HexEntities) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <p>&#x41; &#x42; &#x43;</p>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    
    std::string content = paragraphs[0]->GetTextContent();
    // &#x41; = A, &#x42; = B, &#x43; = C
    EXPECT_TRUE(content.find("A") != std::string::npos);
    EXPECT_TRUE(content.find("B") != std::string::npos);
    EXPECT_TRUE(content.find("C") != std::string::npos);
}

TEST_F(HTML5FeaturesTest, ComplexEntities) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <p>&copy; &reg; &trade; &euro; &pound;</p>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    
    // 验证复杂实体被解析（即使我们不检查具体字符）
    std::string content = paragraphs[0]->GetTextContent();
    EXPECT_FALSE(content.empty());
}

// ========== 注释处理测试 ==========

TEST_F(HTML5FeaturesTest, HTMLComments) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <!-- This is a comment -->
            <p>Content</p>
            <!-- Another comment -->
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    EXPECT_EQ(paragraphs[0]->GetTextContent(), "Content");
}

TEST_F(HTML5FeaturesTest, ConditionalComments) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <!--[if IE]>
            <link rel="stylesheet" href="ie.css">
            <![endif]-->
        </head>
        <body><p>Content</p></body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 条件注释应该被当作普通注释处理
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
}

// ========== CDATA 处理测试 ==========

TEST_F(HTML5FeaturesTest, CDATASection) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <script>
            //<![CDATA[
            var x = 1 < 2 && 3 > 2;
            //]]>
            </script>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto scripts = doc.QuerySelectorAll("script");
    ASSERT_EQ(scripts.size(), 1);
}

// ========== 自闭合标签测试 ==========

TEST_F(HTML5FeaturesTest, SelfClosingTags) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <img src="test.jpg" alt="Test" />
            <br />
            <hr />
            <input type="text" />
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto imgs = doc.QuerySelectorAll("img");
    ASSERT_EQ(imgs.size(), 1);
    EXPECT_EQ(imgs[0]->GetAttribute("src"), "test.jpg");
    
    auto brs = doc.QuerySelectorAll("br");
    ASSERT_EQ(brs.size(), 1);
    
    auto hrs = doc.QuerySelectorAll("hr");
    ASSERT_EQ(hrs.size(), 1);
    
    auto inputs = doc.QuerySelectorAll("input");
    ASSERT_EQ(inputs.size(), 1);
    EXPECT_EQ(inputs[0]->GetAttribute("type"), "text");
}

// ========== 布尔属性测试 ==========

TEST_F(HTML5FeaturesTest, BooleanAttributes) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <input type="checkbox" checked>
            <input type="text" disabled>
            <input type="text" readonly>
            <select multiple>
                <option selected>Option 1</option>
            </select>
        </body>
        </html>
    )";
    
    ASSERT_TRUE(doc.ParseHTML(html));
    
    auto inputs = doc.QuerySelectorAll("input");
    ASSERT_EQ(inputs.size(), 3);
    
    // 布尔属性应该存在
    EXPECT_TRUE(inputs[0]->HasAttribute("checked"));
    EXPECT_TRUE(inputs[1]->HasAttribute("disabled"));
    EXPECT_TRUE(inputs[2]->HasAttribute("readonly"));
}

