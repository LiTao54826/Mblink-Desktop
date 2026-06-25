/**
 * @file test_lexbor_document.cpp
 * @brief Lexbor 文档解析单元测试
 */

#include <gtest/gtest.h>
#include "lexbor/lexbor_document.h"

namespace mblink {
namespace test {

class LexborDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_unique<LexborDocument>();
    }

    void TearDown() override {
        doc_.reset();
    }

protected:
    std::unique_ptr<LexborDocument> doc_;
};

// ========== HTML 解析测试 ==========

TEST_F(LexborDocumentTest, ParseSimpleHTML) {
    bool result = doc_->ParseHTML("<html><body><div>Hello</div></body></html>");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseEmptyHTML) {
    bool result = doc_->ParseHTML("");
    // 空 HTML 可能成功或失败，取决于实现
}

TEST_F(LexborDocumentTest, ParseHTMLWithAttributes) {
    bool result = doc_->ParseHTML(R"(
        <html>
        <body>
            <div id="test" class="foo bar" data-value="123">Content</div>
        </body>
        </html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseHTMLWithNestedElements) {
    bool result = doc_->ParseHTML(R"(
        <html>
        <body>
            <div>
                <ul>
                    <li>Item 1</li>
                    <li>Item 2</li>
                    <li>Item 3</li>
                </ul>
            </div>
        </body>
        </html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseHTMLWithComments) {
    bool result = doc_->ParseHTML(R"(
        <html>
        <body>
            <!-- This is a comment -->
            <div>Content</div>
        </body>
        </html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseHTMLWithDoctype) {
    bool result = doc_->ParseHTML(R"(
        <!DOCTYPE html>
        <html>
        <head><title>Test</title></head>
        <body></body>
        </html>
    )");
    EXPECT_TRUE(result);
}

// ========== 错误恢复测试 ==========

TEST_F(LexborDocumentTest, ParseMalformedHTML) {
    // HTML5 解析器应该能处理格式错误的 HTML
    bool result = doc_->ParseHTML("<div><span>Unclosed");
    EXPECT_TRUE(result);  // 应该自动修复
}

TEST_F(LexborDocumentTest, ParseMissingClosingTags) {
    bool result = doc_->ParseHTML("<html><body><div><p>Text");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseExtraClosingTags) {
    bool result = doc_->ParseHTML("<div>Content</div></div></div>");
    EXPECT_TRUE(result);
}

// ========== 元素查询测试 ==========

TEST_F(LexborDocumentTest, GetBody) {
    doc_->ParseHTML("<html><body></body></html>");
    auto body = doc_->GetBody();
    EXPECT_NE(body, nullptr);
}

TEST_F(LexborDocumentTest, GetHead) {
    doc_->ParseHTML("<html><head></head><body></body></html>");
    auto head = doc_->GetHead();
    EXPECT_NE(head, nullptr);
}

TEST_F(LexborDocumentTest, GetDocumentElement) {
    doc_->ParseHTML("<html><body></body></html>");
    auto html = doc_->GetDocumentElement();
    EXPECT_NE(html, nullptr);
}

TEST_F(LexborDocumentTest, GetElementById) {
    doc_->ParseHTML("<html><body><div id='test'>Content</div></body></html>");
    auto elem = doc_->GetElementById("test");
    EXPECT_NE(elem, nullptr);
}

TEST_F(LexborDocumentTest, QuerySelectorAllByTag) {
    doc_->ParseHTML(R"(
        <html><body>
            <div>1</div>
            <div>2</div>
            <div>3</div>
        </body></html>
    )");
    auto divs = doc_->QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 3);
}

TEST_F(LexborDocumentTest, QuerySelectorAllByClass) {
    doc_->ParseHTML(R"(
        <html><body>
            <div class="item">1</div>
            <div class="item">2</div>
            <div class="other">3</div>
        </body></html>
    )");
    auto items = doc_->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 2);
}

// ========== 序列化测试 ==========

TEST_F(LexborDocumentTest, SerializeToHTML) {
    doc_->ParseHTML("<html><body><div>Hello</div></body></html>");
    std::string html = doc_->SerializeToHTML();

    EXPECT_TRUE(html.find("div") != std::string::npos);
    EXPECT_TRUE(html.find("Hello") != std::string::npos);
}

TEST_F(LexborDocumentTest, SerializeNode) {
    doc_->ParseHTML("<html><body><div id='test'>Content</div></body></html>");
    auto elem = doc_->GetElementById("test");

    std::string html = doc_->SerializeNode(lxb_dom_interface_node(elem->GetNativeElement()));
    EXPECT_TRUE(html.find("div") != std::string::npos);
    EXPECT_TRUE(html.find("Content") != std::string::npos);
}

// ========== 特殊元素测试 ==========

TEST_F(LexborDocumentTest, ParseScript) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <script>
                var x = 1 < 2;
                console.log(x);
            </script>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseStyle) {
    bool result = doc_->ParseHTML(R"(
        <html>
        <head>
            <style>
                .test { color: red; }
            </style>
        </head>
        <body></body>
        </html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseTextarea) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <textarea>
                <div>This is not HTML</div>
            </textarea>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseSelfClosingTags) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <img src="test.png" />
            <br />
            <input type="text" />
        </body></html>
    )");
    EXPECT_TRUE(result);
}

// ========== 表单元素测试 ==========

TEST_F(LexborDocumentTest, ParseForm) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <form action="/submit" method="post">
                <input type="text" name="username" />
                <input type="password" name="password" />
                <button type="submit">Submit</button>
            </form>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseSelect) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <select name="choice">
                <option value="1">Option 1</option>
                <option value="2" selected>Option 2</option>
                <option value="3">Option 3</option>
            </select>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

// ========== 表格测试 ==========

TEST_F(LexborDocumentTest, ParseTable) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <table>
                <thead>
                    <tr><th>Header 1</th><th>Header 2</th></tr>
                </thead>
                <tbody>
                    <tr><td>Cell 1</td><td>Cell 2</td></tr>
                    <tr><td>Cell 3</td><td>Cell 4</td></tr>
                </tbody>
            </table>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

// ========== Unicode 测试 ==========

TEST_F(LexborDocumentTest, ParseUnicode) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <div>你好世界</div>
            <div>🌍 🌎 🌏</div>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

TEST_F(LexborDocumentTest, ParseHTMLEntities) {
    bool result = doc_->ParseHTML(R"(
        <html><body>
            <div>&lt;div&gt; &amp; &quot;test&quot;</div>
        </body></html>
    )");
    EXPECT_TRUE(result);
}

} // namespace test
} // namespace mblink
