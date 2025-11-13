/**
 * @file test_dom_html_content.cpp
 * @brief DOM innerHTML/outerHTML 功能测试
 */

#include <gtest/gtest.h>
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/text.h"
#include <memory>

using namespace lightui;

// ========== 辅助函数 ==========

// 创建一个已初始化的Document和一个添加到body的元素
std::pair<std::shared_ptr<Document>, std::shared_ptr<Element>> CreateTestElement(const std::string& tag_name) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto elem = doc->CreateElement(tag_name);
    doc->GetBody()->AppendChild(elem);
    return {doc, elem};
}

// ========== 测试 innerHTML ==========

TEST(DOMHTMLContentTest, GetInnerHTMLEmpty) {
    auto div = std::make_shared<Element>("div");
    
    std::string html = div->GetInnerHTML();
    
    EXPECT_EQ(html, "");
}

TEST(DOMHTMLContentTest, GetInnerHTMLText) {
    auto div = std::make_shared<Element>("div");
    auto text = std::make_shared<Text>("Hello World");
    div->AppendChild(text);
    
    std::string html = div->GetInnerHTML();
    
    EXPECT_EQ(html, "Hello World");
}

TEST(DOMHTMLContentTest, GetInnerHTMLTextWithEscape) {
    auto div = std::make_shared<Element>("div");
    auto text = std::make_shared<Text>("<script>alert('XSS')</script>");
    div->AppendChild(text);
    
    std::string html = div->GetInnerHTML();
    
    // 应该转义HTML特殊字符
    EXPECT_EQ(html, "&lt;script&gt;alert(&#39;XSS&#39;)&lt;/script&gt;");
}

TEST(DOMHTMLContentTest, GetInnerHTMLSingleElement) {
    auto div = std::make_shared<Element>("div");
    auto p = std::make_shared<Element>("p");
    auto text = std::make_shared<Text>("Hello");
    p->AppendChild(text);
    div->AppendChild(p);
    
    std::string html = div->GetInnerHTML();
    
    EXPECT_EQ(html, "<p>Hello</p>");
}

TEST(DOMHTMLContentTest, GetInnerHTMLNestedElements) {
    auto div = std::make_shared<Element>("div");
    auto p = std::make_shared<Element>("p");
    auto strong = std::make_shared<Element>("strong");
    auto text = std::make_shared<Text>("Bold Text");
    
    strong->AppendChild(text);
    p->AppendChild(strong);
    div->AppendChild(p);
    
    std::string html = div->GetInnerHTML();
    
    EXPECT_EQ(html, "<p><strong>Bold Text</strong></p>");
}

TEST(DOMHTMLContentTest, GetInnerHTMLWithAttributes) {
    auto div = std::make_shared<Element>("div");
    auto a = std::make_shared<Element>("a");
    a->SetAttribute("href", "https://example.com");
    a->SetAttribute("target", "_blank");
    auto text = std::make_shared<Text>("Link");
    a->AppendChild(text);
    div->AppendChild(a);
    
    std::string html = div->GetInnerHTML();
    
    // 应该包含属性
    EXPECT_TRUE(html.find("href=\"https://example.com\"") != std::string::npos);
    EXPECT_TRUE(html.find("target=\"_blank\"") != std::string::npos);
    EXPECT_TRUE(html.find("Link") != std::string::npos);
}

TEST(DOMHTMLContentTest, SetInnerHTMLEmpty) {
    auto [doc, div] = CreateTestElement("div");
    auto text = std::make_shared<Text>("Old content");
    div->AppendChild(text);
    
    div->SetInnerHTML("");
    
    EXPECT_EQ(div->GetChildNodes().size(), 0);
}

TEST(DOMHTMLContentTest, SetInnerHTMLText) {
    auto [doc, div] = CreateTestElement("div");
    
    div->SetInnerHTML("Hello World");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    auto text = std::dynamic_pointer_cast<Text>(div->GetChildNodes()[0]);
    EXPECT_NE(text, nullptr);
    EXPECT_EQ(text->GetData(), "Hello World");
}

TEST(DOMHTMLContentTest, SetInnerHTMLSingleElement) {
    auto [doc, div] = CreateTestElement("div");
    
    div->SetInnerHTML("<p>Hello</p>");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    auto p = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[0]);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(p->GetTagName(), "p");
    EXPECT_EQ(p->GetTextContent(), "Hello");
}

TEST(DOMHTMLContentTest, SetInnerHTMLMultipleElements) {
    auto [doc, div] = CreateTestElement("div");
    
    div->SetInnerHTML("<p>First</p><p>Second</p>");
    
    EXPECT_EQ(div->GetChildNodes().size(), 2);
    auto p1 = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[0]);
    auto p2 = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[1]);
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p1->GetTextContent(), "First");
    EXPECT_EQ(p2->GetTextContent(), "Second");
}

TEST(DOMHTMLContentTest, SetInnerHTMLNestedElements) {
    auto [doc, div] = CreateTestElement("div");
    
    div->SetInnerHTML("<p>Hello <strong>World</strong></p>");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    auto p = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[0]);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(p->GetChildNodes().size(), 2);
    
    auto text = std::dynamic_pointer_cast<Text>(p->GetChildNodes()[0]);
    auto strong = std::dynamic_pointer_cast<Element>(p->GetChildNodes()[1]);
    EXPECT_NE(text, nullptr);
    EXPECT_NE(strong, nullptr);
    EXPECT_EQ(text->GetData(), "Hello ");
    EXPECT_EQ(strong->GetTextContent(), "World");
}

TEST(DOMHTMLContentTest, SetInnerHTMLWithAttributes) {
    auto [doc, div] = CreateTestElement("div");
    
    div->SetInnerHTML("<a href=\"https://example.com\" target=\"_blank\">Link</a>");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    auto a = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[0]);
    EXPECT_NE(a, nullptr);
    EXPECT_EQ(a->GetAttribute("href"), "https://example.com");
    EXPECT_EQ(a->GetAttribute("target"), "_blank");
    EXPECT_EQ(a->GetTextContent(), "Link");
}

TEST(DOMHTMLContentTest, SetInnerHTMLReplacesContent) {
    auto [doc, div] = CreateTestElement("div");
    auto old_text = std::make_shared<Text>("Old content");
    div->AppendChild(old_text);
    
    div->SetInnerHTML("<p>New content</p>");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    auto p = std::dynamic_pointer_cast<Element>(div->GetChildNodes()[0]);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(p->GetTextContent(), "New content");
}

// ========== 测试 outerHTML ==========

TEST(DOMHTMLContentTest, GetOuterHTMLSimple) {
    auto div = std::make_shared<Element>("div");
    auto text = std::make_shared<Text>("Hello");
    div->AppendChild(text);
    
    std::string html = div->GetOuterHTML();
    
    EXPECT_EQ(html, "<div>Hello</div>");
}

TEST(DOMHTMLContentTest, GetOuterHTMLWithAttributes) {
    auto div = std::make_shared<Element>("div");
    div->SetAttribute("id", "container");
    div->SetAttribute("class", "main");
    auto text = std::make_shared<Text>("Content");
    div->AppendChild(text);
    
    std::string html = div->GetOuterHTML();
    
    EXPECT_TRUE(html.find("<div") != std::string::npos);
    EXPECT_TRUE(html.find("id=\"container\"") != std::string::npos);
    EXPECT_TRUE(html.find("class=\"main\"") != std::string::npos);
    EXPECT_TRUE(html.find("Content") != std::string::npos);
    EXPECT_TRUE(html.find("</div>") != std::string::npos);
}

TEST(DOMHTMLContentTest, GetOuterHTMLVoidElement) {
    auto br = std::make_shared<Element>("br");
    
    std::string html = br->GetOuterHTML();
    
    EXPECT_EQ(html, "<br />");
}

TEST(DOMHTMLContentTest, GetOuterHTMLInputElement) {
    auto input = std::make_shared<Element>("input");
    input->SetAttribute("type", "text");
    input->SetAttribute("value", "test");
    
    std::string html = input->GetOuterHTML();
    
    EXPECT_TRUE(html.find("<input") != std::string::npos);
    EXPECT_TRUE(html.find("type=\"text\"") != std::string::npos);
    EXPECT_TRUE(html.find("value=\"test\"") != std::string::npos);
    EXPECT_TRUE(html.find("/>") != std::string::npos);
}

TEST(DOMHTMLContentTest, SetOuterHTMLReplacesElement) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto parent = doc->CreateElement("div");
    auto child = doc->CreateElement("p");
    auto text = std::make_shared<Text>("Old");
    child->AppendChild(text);
    parent->AppendChild(child);
    doc->GetBody()->AppendChild(parent);
    
    child->SetOuterHTML("<span>New</span>");
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    auto span = std::dynamic_pointer_cast<Element>(parent->GetChildNodes()[0]);
    EXPECT_NE(span, nullptr);
    EXPECT_EQ(span->GetTagName(), "span");
    EXPECT_EQ(span->GetTextContent(), "New");
}

TEST(DOMHTMLContentTest, SetOuterHTMLMultipleElements) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto parent = doc->CreateElement("div");
    auto child = doc->CreateElement("p");
    parent->AppendChild(child);
    doc->GetBody()->AppendChild(parent);
    
    child->SetOuterHTML("<span>First</span><span>Second</span>");
    
    EXPECT_EQ(parent->GetChildNodes().size(), 2);
    auto span1 = std::dynamic_pointer_cast<Element>(parent->GetChildNodes()[0]);
    auto span2 = std::dynamic_pointer_cast<Element>(parent->GetChildNodes()[1]);
    EXPECT_NE(span1, nullptr);
    EXPECT_NE(span2, nullptr);
    EXPECT_EQ(span1->GetTextContent(), "First");
    EXPECT_EQ(span2->GetTextContent(), "Second");
}

// ========== 测试 innerHTML/outerHTML 往返 ==========

TEST(DOMHTMLContentTest, InnerHTMLRoundTrip) {
    auto [doc, div] = CreateTestElement("div");
    std::string original_html = "<p>Hello <strong>World</strong></p>";
    
    div->SetInnerHTML(original_html);
    std::string result_html = div->GetInnerHTML();
    
    EXPECT_EQ(result_html, original_html);
}

TEST(DOMHTMLContentTest, OuterHTMLRoundTrip) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto parent = doc->CreateElement("body");
    auto div = doc->CreateElement("div");
    div->SetAttribute("id", "test");
    auto text = std::make_shared<Text>("Content");
    div->AppendChild(text);
    parent->AppendChild(div);
    doc->GetBody()->AppendChild(parent);
    
    std::string original_html = div->GetOuterHTML();
    div->SetOuterHTML(original_html);
    
    auto new_div = std::dynamic_pointer_cast<Element>(parent->GetChildNodes()[0]);
    EXPECT_NE(new_div, nullptr);
    EXPECT_EQ(new_div->GetAttribute("id"), "test");
    EXPECT_EQ(new_div->GetTextContent(), "Content");
}

