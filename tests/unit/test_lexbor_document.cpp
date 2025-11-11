/**
 * @file test_lexbor_document.cpp
 * @brief Lexbor Document 包装类单元测试
 */

#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 基础功能测试 ==========

TEST(LexborDocumentTest, CreateDocument) {
    LexborDocument doc;
    EXPECT_NE(doc.GetNativeDocument(), nullptr);
}

TEST(LexborDocumentTest, ParseSimpleHTML) {
    LexborDocument doc;
    bool success = doc.ParseHTML("<html><body><div>Hello</div></body></html>");
    EXPECT_TRUE(success);
}

TEST(LexborDocumentTest, GetBody) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div>Hello</div></body></html>");
    
    LexborElement* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetTagName(), "body");
}

TEST(LexborDocumentTest, GetDocumentElement) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div>Hello</div></body></html>");
    
    LexborElement* html = doc.GetDocumentElement();
    ASSERT_NE(html, nullptr);
    EXPECT_EQ(html->GetTagName(), "html");
}

// ========== 元素创建测试 ==========

TEST(LexborDocumentTest, CreateElement) {
    LexborDocument doc;
    doc.ParseHTML("<html><body></body></html>");
    
    LexborElement* div = doc.CreateElement("div");
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->GetTagName(), "div");
}

TEST(LexborDocumentTest, CreateTextNode) {
    LexborDocument doc;
    doc.ParseHTML("<html><body></body></html>");
    
    LexborText* text = doc.CreateTextNode("Hello World");
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetData(), "Hello World");
}

// ========== 属性操作测试 ==========

TEST(LexborElementTest, GetSetAttribute) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test' class='container'></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    // 获取属性
    EXPECT_EQ(div->GetAttribute("id"), "test");
    EXPECT_EQ(div->GetAttribute("class"), "container");
    
    // 设置属性
    div->SetAttribute("data-value", "123");
    EXPECT_EQ(div->GetAttribute("data-value"), "123");
}

TEST(LexborElementTest, HasAttribute) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test'></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    EXPECT_TRUE(div->HasAttribute("id"));
    EXPECT_FALSE(div->HasAttribute("class"));
}

TEST(LexborElementTest, RemoveAttribute) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test' class='container'></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    div->RemoveAttribute("class");
    EXPECT_FALSE(div->HasAttribute("class"));
    EXPECT_TRUE(div->HasAttribute("id"));
}

// ========== ID 和 Class 操作测试 ==========

TEST(LexborElementTest, GetSetId) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    div->SetId("my-id");
    EXPECT_EQ(div->GetId(), "my-id");
}

TEST(LexborElementTest, GetSetClassName) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    div->SetClassName("btn btn-primary");
    EXPECT_EQ(div->GetClassName(), "btn btn-primary");
}

TEST(LexborElementTest, HasClass) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div class='btn btn-primary'></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    EXPECT_TRUE(div->HasClass("btn"));
    EXPECT_TRUE(div->HasClass("btn-primary"));
    EXPECT_FALSE(div->HasClass("btn-secondary"));
}

TEST(LexborElementTest, AddRemoveClass) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div class='btn'></div></body></html>");
    
    LexborElement* div = doc.QuerySelector("div");
    ASSERT_NE(div, nullptr);
    
    // 添加 class
    div->AddClass("btn-primary");
    EXPECT_TRUE(div->HasClass("btn-primary"));
    
    // 移除 class
    div->RemoveClass("btn");
    EXPECT_FALSE(div->HasClass("btn"));
    EXPECT_TRUE(div->HasClass("btn-primary"));
}

// ========== 选择器测试 ==========

TEST(LexborDocumentTest, GetElementById) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test'>Hello</div></body></html>");
    
    LexborElement* elem = doc.GetElementById("test");
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetId(), "test");
}

TEST(LexborDocumentTest, QuerySelectorById) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test'>Hello</div></body></html>");
    
    LexborElement* elem = doc.QuerySelector("#test");
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetId(), "test");
}

TEST(LexborDocumentTest, QuerySelectorByClass) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div class='container'>Hello</div></body></html>");
    
    LexborElement* elem = doc.QuerySelector(".container");
    ASSERT_NE(elem, nullptr);
    EXPECT_TRUE(elem->HasClass("container"));
}

TEST(LexborDocumentTest, QuerySelectorByTag) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div>Hello</div></body></html>");
    
    LexborElement* elem = doc.QuerySelector("div");
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetTagName(), "div");
}

TEST(LexborDocumentTest, QuerySelectorComplex) {
    LexborDocument doc;
    doc.ParseHTML(R"(
        <html>
        <body>
            <div class='container'>
                <button class='btn btn-primary'>Button 1</button>
                <button class='btn btn-secondary'>Button 2</button>
            </div>
        </body>
        </html>
    )");
    
    // 复杂选择器
    LexborElement* elem = doc.QuerySelector("div.container > button.btn-primary");
    ASSERT_NE(elem, nullptr);
    EXPECT_TRUE(elem->HasClass("btn-primary"));
}

TEST(LexborDocumentTest, QuerySelectorAll) {
    LexborDocument doc;
    doc.ParseHTML(R"(
        <html>
        <body>
            <div class='item'>Item 1</div>
            <div class='item'>Item 2</div>
            <div class='item'>Item 3</div>
        </body>
        </html>
    )");
    
    auto elems = doc.QuerySelectorAll(".item");
    EXPECT_EQ(elems.size(), 3);
    
    for (auto* elem : elems) {
        EXPECT_TRUE(elem->HasClass("item"));
    }
}

// ========== 文本内容测试 ==========

TEST(LexborTextTest, GetSetData) {
    LexborDocument doc;
    doc.ParseHTML("<html><body></body></html>");
    
    LexborText* text = doc.CreateTextNode("Hello");
    ASSERT_NE(text, nullptr);
    
    EXPECT_EQ(text->GetData(), "Hello");
    
    text->SetData("World");
    EXPECT_EQ(text->GetData(), "World");
}

// ========== HTML 序列化测试 ==========

TEST(LexborDocumentTest, SerializeToHTML) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><div id='test'>Hello</div></body></html>");
    
    std::string html = doc.SerializeToHTML();
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("test"), std::string::npos);
    EXPECT_NE(html.find("Hello"), std::string::npos);
}

// ========== 移动语义测试 ==========

TEST(LexborDocumentTest, MoveConstructor) {
    LexborDocument doc1;
    doc1.ParseHTML("<html><body><div>Hello</div></body></html>");
    
    LexborDocument doc2(std::move(doc1));
    EXPECT_NE(doc2.GetNativeDocument(), nullptr);
    EXPECT_EQ(doc1.GetNativeDocument(), nullptr);  // 已移动
}

TEST(LexborDocumentTest, MoveAssignment) {
    LexborDocument doc1;
    doc1.ParseHTML("<html><body><div>Hello</div></body></html>");
    
    LexborDocument doc2;
    doc2 = std::move(doc1);
    
    EXPECT_NE(doc2.GetNativeDocument(), nullptr);
    EXPECT_EQ(doc1.GetNativeDocument(), nullptr);  // 已移动
}

// ========== 主函数 ==========

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

