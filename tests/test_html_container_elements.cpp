/**
 * @file test_html_container_elements.cpp
 * @brief HTML容器元素单元测试（div, span, p, h1-h6）
 */

#include <gtest/gtest.h>
#include "core/dom/html_div_element.h"
#include "core/dom/html_span_element.h"
#include "core/dom/html_paragraph_element.h"
#include "core/dom/html_heading_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"
#include <chrono>

using namespace lightui;

// ========== HTMLDivElement测试 ==========

TEST(HTMLDivElement, Construction) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    ASSERT_NE(div, nullptr);
    EXPECT_EQ(div->GetTagName(), "div");
}

TEST(HTMLDivElement, AppendChild) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    auto span = doc->CreateElement("span");
    
    div->AppendChild(span);
    EXPECT_EQ(div->GetChildNodes().size(), 1u);
    EXPECT_EQ(div->GetChildNodes()[0], span);
}

TEST(HTMLDivElement, SetTextContent) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    doc->GetBody()->AppendChild(div);
    
    div->SetTextContent("Hello, World!");
    EXPECT_EQ(div->GetTextContent(), "Hello, World!");
}

TEST(HTMLDivElement, SetAttribute) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    
    div->SetAttribute("class", "container");
    EXPECT_EQ(div->GetAttribute("class"), "container");
    
    div->SetAttribute("id", "main");
    EXPECT_EQ(div->GetAttribute("id"), "main");
}

// ========== HTMLSpanElement测试 ==========

TEST(HTMLSpanElement, Construction) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto span = std::dynamic_pointer_cast<HTMLSpanElement>(doc->CreateElement("span"));
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(span->GetTagName(), "span");
}

TEST(HTMLSpanElement, AppendChild) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto span = std::dynamic_pointer_cast<HTMLSpanElement>(doc->CreateElement("span"));
    auto text = doc->CreateTextNode("Text");
    
    span->AppendChild(text);
    EXPECT_EQ(span->GetChildNodes().size(), 1u);
}

TEST(HTMLSpanElement, SetTextContent) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto span = std::dynamic_pointer_cast<HTMLSpanElement>(doc->CreateElement("span"));
    doc->GetBody()->AppendChild(span);
    
    span->SetTextContent("Inline text");
    EXPECT_EQ(span->GetTextContent(), "Inline text");
}

TEST(HTMLSpanElement, SetAttribute) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto span = std::dynamic_pointer_cast<HTMLSpanElement>(doc->CreateElement("span"));
    
    span->SetAttribute("class", "highlight");
    EXPECT_EQ(span->GetAttribute("class"), "highlight");
}

// ========== HTMLParagraphElement测试 ==========

TEST(HTMLParagraphElement, Construction) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto p = std::dynamic_pointer_cast<HTMLParagraphElement>(doc->CreateElement("p"));
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->GetTagName(), "p");
}

TEST(HTMLParagraphElement, SetTextContent) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto p = std::dynamic_pointer_cast<HTMLParagraphElement>(doc->CreateElement("p"));
    doc->GetBody()->AppendChild(p);
    
    p->SetTextContent("This is a paragraph.");
    EXPECT_EQ(p->GetTextContent(), "This is a paragraph.");
}

TEST(HTMLParagraphElement, SetAttribute) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto p = std::dynamic_pointer_cast<HTMLParagraphElement>(doc->CreateElement("p"));
    
    p->SetAttribute("class", "intro");
    EXPECT_EQ(p->GetAttribute("class"), "intro");
}

// ========== HTMLHeadingElement测试 ==========

TEST(HTMLHeadingElement, ConstructionH1) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h1 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h1"));
    ASSERT_NE(h1, nullptr);
    EXPECT_EQ(h1->GetTagName(), "h1");
    EXPECT_EQ(h1->GetLevel(), 1);
}

TEST(HTMLHeadingElement, ConstructionH2) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h2 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h2"));
    ASSERT_NE(h2, nullptr);
    EXPECT_EQ(h2->GetTagName(), "h2");
    EXPECT_EQ(h2->GetLevel(), 2);
}

TEST(HTMLHeadingElement, ConstructionH3) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h3 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h3"));
    ASSERT_NE(h3, nullptr);
    EXPECT_EQ(h3->GetTagName(), "h3");
    EXPECT_EQ(h3->GetLevel(), 3);
}

TEST(HTMLHeadingElement, ConstructionH4) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h4 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h4"));
    ASSERT_NE(h4, nullptr);
    EXPECT_EQ(h4->GetTagName(), "h4");
    EXPECT_EQ(h4->GetLevel(), 4);
}

TEST(HTMLHeadingElement, ConstructionH5) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h5 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h5"));
    ASSERT_NE(h5, nullptr);
    EXPECT_EQ(h5->GetTagName(), "h5");
    EXPECT_EQ(h5->GetLevel(), 5);
}

TEST(HTMLHeadingElement, ConstructionH6) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h6 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h6"));
    ASSERT_NE(h6, nullptr);
    EXPECT_EQ(h6->GetTagName(), "h6");
    EXPECT_EQ(h6->GetLevel(), 6);
}

TEST(HTMLHeadingElement, SetTextContent) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h1 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h1"));
    doc->GetBody()->AppendChild(h1);
    
    h1->SetTextContent("Main Title");
    EXPECT_EQ(h1->GetTextContent(), "Main Title");
}

TEST(HTMLHeadingElement, SetAttribute) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto h1 = std::dynamic_pointer_cast<HTMLHeadingElement>(doc->CreateElement("h1"));
    
    h1->SetAttribute("class", "title");
    EXPECT_EQ(h1->GetAttribute("class"), "title");
}

// ========== 嵌套测试 ==========

TEST(HTMLContainerElements, NestedStructure) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建嵌套结构：div > p > span
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    auto p = std::dynamic_pointer_cast<HTMLParagraphElement>(doc->CreateElement("p"));
    auto span = std::dynamic_pointer_cast<HTMLSpanElement>(doc->CreateElement("span"));
    
    doc->GetBody()->AppendChild(div);
    div->AppendChild(p);
    p->AppendChild(span);
    span->SetTextContent("Nested text");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1u);
    EXPECT_EQ(p->GetChildNodes().size(), 1u);
    EXPECT_EQ(span->GetTextContent(), "Nested text");
}

TEST(HTMLContainerElements, MultipleChildren) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto div = std::dynamic_pointer_cast<HTMLDivElement>(doc->CreateElement("div"));
    doc->GetBody()->AppendChild(div);
    
    // 添加多个子元素
    auto h1 = doc->CreateElement("h1");
    auto p1 = doc->CreateElement("p");
    auto p2 = doc->CreateElement("p");
    auto span = doc->CreateElement("span");
    
    div->AppendChild(h1);
    div->AppendChild(p1);
    div->AppendChild(p2);
    div->AppendChild(span);
    
    EXPECT_EQ(div->GetChildNodes().size(), 4u);
}

// ========== 性能测试 ==========

TEST(HTMLContainerElements, PerformanceCreation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto div = doc->CreateElement("div");
        auto span = doc->CreateElement("span");
        auto p = doc->CreateElement("p");
        auto h1 = doc->CreateElement("h1");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double avg_time = static_cast<double>(duration.count()) / iterations / 4;
    
    std::cout << "Average container element creation time: " << avg_time << " microseconds" << std::endl;
    
    // 性能要求：平均创建时间应该小于20微秒
    EXPECT_LT(avg_time, 20.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

