/**
 * @file test_dom_lexbor_integration.cpp
 * @brief Document 类 Lexbor 集成测试
 */

#include <gtest/gtest.h>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/lexbor/lexbor_document.h"
#include <fstream>
#include <cstdio>

using namespace lightui;

// ========== 测试 LoadHTML ==========

TEST(DOMDocumentLexborIntegrationTest, LoadHTMLBasic) {
    auto doc = std::make_shared<Document>();
    
    std::string html = R"(
        <!DOCTYPE html>
        <html>
            <head><title>Test</title></head>
            <body>
                <div id="container" class="main">
                    <h1>Hello World</h1>
                    <p>This is a test.</p>
                </div>
            </body>
        </html>
    )";
    
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 验证文档结构
    auto html_elem = doc->GetDocumentElement();
    ASSERT_NE(html_elem, nullptr);
    EXPECT_EQ(html_elem->GetTagName(), "html");
    
    // 验证 body
    auto body = doc->GetBody();
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetTagName(), "body");
    
    // 验证 ID 查询
    auto container = doc->GetElementById("container");
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(container->GetTagName(), "div");
    EXPECT_EQ(container->GetAttribute("class"), "main");
    
    // 验证子元素
    auto children = container->GetChildNodes();
    EXPECT_GT(children.size(), 0);
}

TEST(DOMDocumentLexborIntegrationTest, LoadHTMLEmpty) {
    auto doc = std::make_shared<Document>();
    
    std::string html = "";
    
    ASSERT_TRUE(doc->LoadHTML(html));
}

TEST(DOMDocumentLexborIntegrationTest, LoadHTMLMalformed) {
    auto doc = std::make_shared<Document>();
    
    // Lexbor 可以处理格式错误的 HTML
    std::string html = "<div><p>Unclosed tags";
    
    ASSERT_TRUE(doc->LoadHTML(html));
    
    auto html_elem = doc->GetDocumentElement();
    ASSERT_NE(html_elem, nullptr);
}

// ========== 测试 LoadHTMLFile ==========

TEST(DOMDocumentLexborIntegrationTest, LoadHTMLFile) {
    auto doc = std::make_shared<Document>();
    
    // 创建临时 HTML 文件
    std::string file_path = "test_load_html.html";
    std::string html = R"(
        <!DOCTYPE html>
        <html>
            <body>
                <h1 id="title">Test Title</h1>
            </body>
        </html>
    )";
    
    std::ofstream file(file_path);
    file << html;
    file.close();
    
    // 加载文件
    ASSERT_TRUE(doc->LoadHTMLFile(file_path));
    
    // 验证内容
    auto title = doc->GetElementById("title");
    ASSERT_NE(title, nullptr);
    EXPECT_EQ(title->GetTagName(), "h1");
    
    // 清理
    std::remove(file_path.c_str());
}

TEST(DOMDocumentLexborIntegrationTest, LoadHTMLFileNotFound) {
    auto doc = std::make_shared<Document>();
    
    ASSERT_FALSE(doc->LoadHTMLFile("non_existent_file.html"));
}

// ========== 测试 SaveHTML ==========

TEST(DOMDocumentLexborIntegrationTest, SaveHTML) {
    auto doc = std::make_shared<Document>();
    
    std::string html = R"(<!DOCTYPE html>
<html>
<body>
<div id="test">Content</div>
</body>
</html>)";
    
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 保存 HTML
    std::string saved_html = doc->SaveHTML();
    
    // 验证保存的 HTML 包含关键内容
    EXPECT_NE(saved_html.find("<html"), std::string::npos);
    EXPECT_NE(saved_html.find("<body"), std::string::npos);
    EXPECT_NE(saved_html.find("id=\"test\""), std::string::npos);
    EXPECT_NE(saved_html.find("Content"), std::string::npos);
}

TEST(DOMDocumentLexborIntegrationTest, SaveHTMLAfterModification) {
    auto doc = std::make_shared<Document>();
    
    std::string html = "<html><body><div id=\"test\">Original</div></body></html>";
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 修改 DOM
    auto test_div = doc->GetElementById("test");
    ASSERT_NE(test_div, nullptr);
    
    auto new_p = doc->CreateElement("p");
    auto text = doc->CreateTextNode("New content");
    new_p->AppendChild(text);
    test_div->AppendChild(new_p);
    
    // 保存 HTML（应该触发 SyncToLexbor）
    std::string saved_html = doc->SaveHTML();
    
    // 验证新内容在保存的 HTML 中
    EXPECT_NE(saved_html.find("<p>"), std::string::npos);
    EXPECT_NE(saved_html.find("New content"), std::string::npos);
}

// ========== 测试 SyncFromLexbor ==========

TEST(DOMDocumentLexborIntegrationTest, SyncFromLexbor) {
    auto doc = std::make_shared<Document>();
    
    std::string html = R"(
        <html>
            <body>
                <div id="div1" class="test">
                    <span>Text 1</span>
                </div>
                <div id="div2">
                    <span>Text 2</span>
                </div>
            </body>
        </html>
    )";
    
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 验证同步后的 DOM 结构
    auto div1 = doc->GetElementById("div1");
    ASSERT_NE(div1, nullptr);
    EXPECT_EQ(div1->GetAttribute("class"), "test");
    
    auto div2 = doc->GetElementById("div2");
    ASSERT_NE(div2, nullptr);
    
    // 验证标签名查询
    auto divs = doc->GetElementsByTagName("div");
    EXPECT_EQ(divs.size(), 2);
}

// ========== 测试 SyncToLexbor ==========

TEST(DOMDocumentLexborIntegrationTest, SyncToLexbor) {
    auto doc = std::make_shared<Document>();
    
    std::string html = "<html><body><div id=\"container\"></div></body></html>";
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 修改 MBink DOM
    auto container = doc->GetElementById("container");
    ASSERT_NE(container, nullptr);
    
    auto new_elem = doc->CreateElement("p");
    new_elem->SetAttribute("id", "new-para");
    auto text = doc->CreateTextNode("New paragraph");
    new_elem->AppendChild(text);
    container->AppendChild(new_elem);
    
    // 保存 HTML（触发 SyncToLexbor）
    std::string saved_html = doc->SaveHTML();
    
    // 重新加载保存的 HTML
    auto doc2 = std::make_shared<Document>();
    ASSERT_TRUE(doc2->LoadHTML(saved_html));
    
    // 验证新元素存在
    auto new_para = doc2->GetElementById("new-para");
    ASSERT_NE(new_para, nullptr);
    EXPECT_EQ(new_para->GetTagName(), "p");
}

// ========== 测试双向同步 ==========

TEST(DOMDocumentLexborIntegrationTest, BidirectionalSync) {
    auto doc = std::make_shared<Document>();
    
    // 1. 从 HTML 加载（Lexbor → MBink）
    std::string html = "<html><body><div id=\"test\">Original</div></body></html>";
    ASSERT_TRUE(doc->LoadHTML(html));
    
    auto test_div = doc->GetElementById("test");
    ASSERT_NE(test_div, nullptr);
    
    // 2. 修改 MBink DOM
    test_div->SetAttribute("class", "modified");
    auto new_span = doc->CreateElement("span");
    auto text = doc->CreateTextNode("Added");
    new_span->AppendChild(text);
    test_div->AppendChild(new_span);
    
    // 3. 保存并重新加载（MBink → Lexbor → MBink）
    std::string saved_html = doc->SaveHTML();
    
    auto doc2 = std::make_shared<Document>();
    ASSERT_TRUE(doc2->LoadHTML(saved_html));
    
    // 4. 验证修改保留
    auto test_div2 = doc2->GetElementById("test");
    ASSERT_NE(test_div2, nullptr);
    EXPECT_EQ(test_div2->GetAttribute("class"), "modified");
    
    auto spans = doc2->GetElementsByTagName("span");
    EXPECT_EQ(spans.size(), 1);
}

// ========== 测试 Lexbor Dirty 标志 ==========

TEST(DOMDocumentLexborIntegrationTest, LexborDirtyFlag) {
    auto doc = std::make_shared<Document>();
    
    std::string html = "<html><body><div id=\"test\"></div></body></html>";
    ASSERT_TRUE(doc->LoadHTML(html));
    
    // 修改 DOM 应该标记为 dirty
    auto test_div = doc->GetElementById("test");
    ASSERT_NE(test_div, nullptr);
    
    auto new_elem = doc->CreateElement("p");
    test_div->AppendChild(new_elem);  // 这应该调用 MarkLexborDirty()
    
    // SaveHTML 应该触发 SyncToLexbor
    std::string saved_html = doc->SaveHTML();
    
    // 验证新元素在保存的 HTML 中
    EXPECT_NE(saved_html.find("<p>"), std::string::npos);
}

