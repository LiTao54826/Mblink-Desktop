/**
 * @file test_dom_document.cpp
 * @brief DOM Document 类单元测试
 */

#include <gtest/gtest.h>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include <memory>

using namespace lightui;

// ========== 测试 Document 构造 ==========

TEST(DOMDocumentTest, Constructor) {
    auto doc = std::make_shared<Document>();
    
    EXPECT_EQ(doc->GetNodeType(), NodeType::DOCUMENT_NODE);
    EXPECT_EQ(doc->GetDocumentElement(), nullptr);
    EXPECT_EQ(doc->GetBody(), nullptr);
}

// ========== 测试工厂方法 ==========

TEST(DOMDocumentTest, CreateElement) {
    auto doc = std::make_shared<Document>();
    auto element = doc->CreateElement("div");
    
    EXPECT_NE(element, nullptr);
    EXPECT_EQ(element->GetTagName(), "div");
    EXPECT_EQ(element->GetNodeType(), NodeType::ELEMENT_NODE);
}

TEST(DOMDocumentTest, CreateTextNode) {
    auto doc = std::make_shared<Document>();
    auto text = doc->CreateTextNode("Hello World");
    
    EXPECT_NE(text, nullptr);
    EXPECT_EQ(text->GetData(), "Hello World");
    EXPECT_EQ(text->GetNodeType(), NodeType::TEXT_NODE);
}

TEST(DOMDocumentTest, CreateHtmlElement) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    
    // html 元素自动成为 documentElement
    EXPECT_EQ(doc->GetDocumentElement(), html);
}

// ========== 测试 body 属性 ==========

TEST(DOMDocumentTest, SetBody) {
    auto doc = std::make_shared<Document>();
    auto body = doc->CreateElement("body");
    
    doc->SetBody(body);
    
    EXPECT_EQ(doc->GetBody(), body);
}

// ========== 测试 ID 映射 ==========

TEST(DOMDocumentTest, RegisterElementId) {
    auto doc = std::make_shared<Document>();
    auto element = doc->CreateElement("div");
    
    doc->RegisterElementId("test-id", element);
    
    auto found = doc->GetElementById("test-id");
    EXPECT_EQ(found, element);
}

TEST(DOMDocumentTest, UnregisterElementId) {
    auto doc = std::make_shared<Document>();
    auto element = doc->CreateElement("div");
    
    doc->RegisterElementId("test-id", element);
    doc->UnregisterElementId("test-id");
    
    auto found = doc->GetElementById("test-id");
    EXPECT_EQ(found, nullptr);
}

TEST(DOMDocumentTest, GetElementByIdNotFound) {
    auto doc = std::make_shared<Document>();
    
    auto found = doc->GetElementById("non-existent");
    EXPECT_EQ(found, nullptr);
}

// ========== 测试 GetElementsByTagName ==========

TEST(DOMDocumentTest, GetElementsByTagName) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    auto body = doc->CreateElement("body");
    auto div1 = doc->CreateElement("div");
    auto div2 = doc->CreateElement("div");
    auto span = doc->CreateElement("span");
    
    html->AppendChild(body);
    body->AppendChild(div1);
    body->AppendChild(div2);
    body->AppendChild(span);
    
    auto divs = doc->GetElementsByTagName("div");
    
    EXPECT_EQ(divs.size(), 2);
    EXPECT_EQ(divs[0], div1);
    EXPECT_EQ(divs[1], div2);
}

TEST(DOMDocumentTest, GetElementsByTagNameEmpty) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    
    auto divs = doc->GetElementsByTagName("div");
    
    EXPECT_EQ(divs.size(), 0);
}

TEST(DOMDocumentTest, GetElementsByTagNameNested) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    auto body = doc->CreateElement("body");
    auto div1 = doc->CreateElement("div");
    auto div2 = doc->CreateElement("div");
    
    html->AppendChild(body);
    body->AppendChild(div1);
    div1->AppendChild(div2);
    
    auto divs = doc->GetElementsByTagName("div");
    
    EXPECT_EQ(divs.size(), 2);
}

// ========== 测试 GetElementsByClassName ==========

TEST(DOMDocumentTest, GetElementsByClassName) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    auto body = doc->CreateElement("body");
    auto div1 = doc->CreateElement("div");
    auto div2 = doc->CreateElement("div");
    auto div3 = doc->CreateElement("div");
    
    div1->SetClassName("test");
    div2->SetClassName("test");
    div3->SetClassName("other");
    
    html->AppendChild(body);
    body->AppendChild(div1);
    body->AppendChild(div2);
    body->AppendChild(div3);
    
    auto elements = doc->GetElementsByClassName("test");
    
    EXPECT_EQ(elements.size(), 2);
    EXPECT_EQ(elements[0], div1);
    EXPECT_EQ(elements[1], div2);
}

TEST(DOMDocumentTest, GetElementsByClassNameEmpty) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    
    auto elements = doc->GetElementsByClassName("test");
    
    EXPECT_EQ(elements.size(), 0);
}

// ========== 测试 CloneNode ==========

TEST(DOMDocumentTest, CloneNodeShallow) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    
    auto cloned_doc = std::dynamic_pointer_cast<Document>(doc->CloneNode(false));
    
    EXPECT_NE(cloned_doc, nullptr);
    EXPECT_NE(cloned_doc, doc);
    EXPECT_EQ(cloned_doc->GetDocumentElement(), nullptr);
}

TEST(DOMDocumentTest, CloneNodeDeep) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    auto body = doc->CreateElement("body");
    html->AppendChild(body);
    
    auto cloned_doc = std::dynamic_pointer_cast<Document>(doc->CloneNode(true));
    
    EXPECT_NE(cloned_doc, nullptr);
    EXPECT_NE(cloned_doc, doc);
    EXPECT_NE(cloned_doc->GetDocumentElement(), nullptr);
    EXPECT_NE(cloned_doc->GetDocumentElement(), html);
}

// ========== 测试完整的 DOM 树 ==========

TEST(DOMDocumentTest, CompleteDocumentTree) {
    auto doc = std::make_shared<Document>();
    
    // 创建 HTML 结构
    auto html = doc->CreateElement("html");
    auto head = doc->CreateElement("head");
    auto body = doc->CreateElement("body");
    auto title = doc->CreateElement("title");
    auto title_text = doc->CreateTextNode("Test Page");
    auto div = doc->CreateElement("div");
    auto text = doc->CreateTextNode("Hello World");
    
    // 构建树
    html->AppendChild(head);
    html->AppendChild(body);
    head->AppendChild(title);
    title->AppendChild(title_text);
    body->AppendChild(div);
    div->AppendChild(text);
    
    doc->SetBody(body);
    
    // 验证结构
    EXPECT_EQ(doc->GetDocumentElement(), html);
    EXPECT_EQ(doc->GetBody(), body);
    EXPECT_EQ(html->GetChildNodes().size(), 2);
    EXPECT_EQ(body->GetChildNodes().size(), 1);
    EXPECT_EQ(div->GetTextContent(), "Hello World");
}

// ========== 测试 ID 自动管理 ==========

TEST(DOMDocumentTest, ElementIdIntegration) {
    auto doc = std::make_shared<Document>();
    auto html = doc->CreateElement("html");
    auto div1 = doc->CreateElement("div");
    auto div2 = doc->CreateElement("div");
    
    div1->SetAttribute("id", "first");
    div2->SetAttribute("id", "second");
    
    // 手动注册 ID
    doc->RegisterElementId("first", div1);
    doc->RegisterElementId("second", div2);
    
    html->AppendChild(div1);
    html->AppendChild(div2);
    
    EXPECT_EQ(doc->GetElementById("first"), div1);
    EXPECT_EQ(doc->GetElementById("second"), div2);
}

