/**
 * @file test_dom_query.cpp
 * @brief DOM 查询功能测试
 */

#include <gtest/gtest.h>
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"

using namespace lightui;

// ========== QuerySelector 测试 ==========

TEST(DOMQueryTest, QuerySelectorByTagName) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    auto child3 = std::make_shared<Element>("span");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    root->AppendChild(child3);
    
    auto result = root->QuerySelector("span");
    EXPECT_EQ(result, child1);  // 返回第一个匹配的
}

TEST(DOMQueryTest, QuerySelectorById) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("div");
    auto child2 = std::make_shared<Element>("div");
    
    child1->SetAttribute("id", "first");
    child2->SetAttribute("id", "second");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    
    auto result = root->QuerySelector("#second");
    EXPECT_EQ(result, child2);
}

TEST(DOMQueryTest, QuerySelectorByClass) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("div");
    auto child2 = std::make_shared<Element>("div");
    
    child1->AddClass("foo");
    child2->AddClass("bar");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    
    auto result = root->QuerySelector(".bar");
    EXPECT_EQ(result, child2);
}

TEST(DOMQueryTest, QuerySelectorByAttribute) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("input");
    auto child2 = std::make_shared<Element>("input");
    
    child1->SetAttribute("type", "text");
    child2->SetAttribute("type", "password");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    
    auto result = root->QuerySelector("[type=\"password\"]");
    EXPECT_EQ(result, child2);
}

TEST(DOMQueryTest, QuerySelectorWildcard) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    
    auto result = root->QuerySelector("*");
    EXPECT_EQ(result, child1);  // 返回第一个元素
}

TEST(DOMQueryTest, QuerySelectorNotFound) {
    auto root = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    root->AppendChild(child);
    
    auto result = root->QuerySelector("p");
    EXPECT_EQ(result, nullptr);
}

TEST(DOMQueryTest, QuerySelectorNested) {
    auto root = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("div");
    auto grandchild = std::make_shared<Element>("span");
    
    grandchild->SetAttribute("id", "target");
    child->AppendChild(grandchild);
    root->AppendChild(child);
    
    auto result = root->QuerySelector("#target");
    EXPECT_EQ(result, grandchild);
}

// ========== QuerySelectorAll 测试 ==========

TEST(DOMQueryTest, QuerySelectorAllByTagName) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    auto child3 = std::make_shared<Element>("span");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    root->AppendChild(child3);
    
    auto results = root->QuerySelectorAll("span");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], child1);
    EXPECT_EQ(results[1], child3);
}

TEST(DOMQueryTest, QuerySelectorAllByClass) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("div");
    auto child2 = std::make_shared<Element>("div");
    auto child3 = std::make_shared<Element>("div");
    
    child1->AddClass("foo");
    child2->AddClass("bar");
    child3->AddClass("foo");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    root->AppendChild(child3);
    
    auto results = root->QuerySelectorAll(".foo");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], child1);
    EXPECT_EQ(results[1], child3);
}

TEST(DOMQueryTest, QuerySelectorAllEmpty) {
    auto root = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    root->AppendChild(child);
    
    auto results = root->QuerySelectorAll("p");
    EXPECT_EQ(results.size(), 0);
}

TEST(DOMQueryTest, QuerySelectorAllNested) {
    auto root = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("div");
    auto child2 = std::make_shared<Element>("div");
    auto grandchild1 = std::make_shared<Element>("span");
    auto grandchild2 = std::make_shared<Element>("span");
    
    child1->AppendChild(grandchild1);
    child2->AppendChild(grandchild2);
    root->AppendChild(child1);
    root->AppendChild(child2);
    
    auto results = root->QuerySelectorAll("span");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], grandchild1);
    EXPECT_EQ(results[1], grandchild2);
}

// ========== Matches 测试 ==========

TEST(DOMQueryTest, MatchesTagName) {
    auto element = std::make_shared<Element>("div");
    EXPECT_TRUE(element->Matches("div"));
    EXPECT_FALSE(element->Matches("span"));
}

TEST(DOMQueryTest, MatchesId) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("id", "myId");
    
    EXPECT_TRUE(element->Matches("#myId"));
    EXPECT_FALSE(element->Matches("#otherId"));
}

TEST(DOMQueryTest, MatchesClass) {
    auto element = std::make_shared<Element>("div");
    element->AddClass("foo");
    element->AddClass("bar");
    
    EXPECT_TRUE(element->Matches(".foo"));
    EXPECT_TRUE(element->Matches(".bar"));
    EXPECT_FALSE(element->Matches(".baz"));
}

TEST(DOMQueryTest, MatchesAttribute) {
    auto element = std::make_shared<Element>("input");
    element->SetAttribute("type", "text");
    
    EXPECT_TRUE(element->Matches("[type=\"text\"]"));
    EXPECT_FALSE(element->Matches("[type=\"password\"]"));
}

TEST(DOMQueryTest, MatchesWildcard) {
    auto element = std::make_shared<Element>("div");
    EXPECT_TRUE(element->Matches("*"));
}

// ========== Closest 测试 ==========

TEST(DOMQueryTest, ClosestSelf) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("id", "target");
    
    auto result = element->Closest("#target");
    EXPECT_EQ(result, element);
}

TEST(DOMQueryTest, ClosestParent) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->SetAttribute("id", "parent");
    parent->AppendChild(child);
    
    auto result = child->Closest("#parent");
    EXPECT_EQ(result, parent);
}

TEST(DOMQueryTest, ClosestGrandparent) {
    auto grandparent = std::make_shared<Element>("div");
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    grandparent->AddClass("container");
    grandparent->AppendChild(parent);
    parent->AppendChild(child);
    
    auto result = child->Closest(".container");
    EXPECT_EQ(result, grandparent);
}

TEST(DOMQueryTest, ClosestNotFound) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    auto result = child->Closest(".notfound");
    EXPECT_EQ(result, nullptr);
}

// ========== innerHTML 测试 ==========

TEST(DOMQueryTest, GetInnerHTMLEmpty) {
    auto element = std::make_shared<Element>("div");
    EXPECT_EQ(element->GetInnerHTML(), "");
}

TEST(DOMQueryTest, GetInnerHTMLText) {
    auto element = std::make_shared<Element>("div");
    auto text = std::make_shared<Text>("Hello");
    element->AppendChild(text);
    
    EXPECT_EQ(element->GetInnerHTML(), "Hello");
}

TEST(DOMQueryTest, GetInnerHTMLElement) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    auto text = std::make_shared<Text>("Hello");
    
    child->AppendChild(text);
    parent->AppendChild(child);
    
    EXPECT_EQ(parent->GetInnerHTML(), "<span>Hello</span>");
}

TEST(DOMQueryTest, GetInnerHTMLWithAttributes) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    child->SetAttribute("id", "myId");
    child->AddClass("myClass");
    
    auto text = std::make_shared<Text>("Hello");
    child->AppendChild(text);
    parent->AppendChild(child);
    
    std::string html = parent->GetInnerHTML();
    EXPECT_TRUE(html.find("id=\"myId\"") != std::string::npos);
    EXPECT_TRUE(html.find("class=\"myClass\"") != std::string::npos);
    EXPECT_TRUE(html.find("Hello") != std::string::npos);
}

TEST(DOMQueryTest, SetInnerHTMLText) {
    auto element = std::make_shared<Element>("div");
    element->SetInnerHTML("Hello World");
    
    EXPECT_EQ(element->GetChildNodes().size(), 1);
    EXPECT_EQ(element->GetTextContent(), "Hello World");
}

TEST(DOMQueryTest, SetInnerHTMLEmpty) {
    auto element = std::make_shared<Element>("div");
    auto text = std::make_shared<Text>("Hello");
    element->AppendChild(text);
    
    element->SetInnerHTML("");
    EXPECT_EQ(element->GetChildNodes().size(), 0);
}

TEST(DOMQueryTest, SetInnerHTMLReplace) {
    auto element = std::make_shared<Element>("div");
    auto text1 = std::make_shared<Text>("Old");
    element->AppendChild(text1);
    
    element->SetInnerHTML("New");
    EXPECT_EQ(element->GetChildNodes().size(), 1);
    EXPECT_EQ(element->GetTextContent(), "New");
}

