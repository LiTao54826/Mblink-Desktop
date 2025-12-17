/**
 * @file test_dom_integration.cpp
 * @brief DOM 集成测试
 *
 * 测试 DOM 各模块之间的协作
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"
#include "dom/text.h"

namespace lightui {
namespace test {

class DOMIntegrationTest : public DOMTestBase {};

// ========== 基本 DOM 操作测试 ==========

TEST_F(DOMIntegrationTest, CreateDocument) {
    auto doc = CreateDocument();
    EXPECT_NE(doc, nullptr);
    EXPECT_NE(doc->GetBody(), nullptr);
}

TEST_F(DOMIntegrationTest, CreateAndAppendElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto div = doc->CreateElement("div");
    div->SetAttribute("id", "test");
    body->AppendChild(div);
    
    auto found = doc->GetElementById("test");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->GetTagName(), "div");
}

TEST_F(DOMIntegrationTest, SetAndGetAttribute) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetAttribute("id", "myId");
    div->SetAttribute("class", "myClass");
    div->SetAttribute("data-value", "123");
    
    EXPECT_EQ(div->GetAttribute("id"), "myId");
    EXPECT_EQ(div->GetAttribute("class"), "myClass");
    EXPECT_EQ(div->GetAttribute("data-value"), "123");
}

TEST_F(DOMIntegrationTest, ClassOperations) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->AddClass("foo");
    div->AddClass("bar");
    
    EXPECT_TRUE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("bar"));
    
    div->RemoveClass("foo");
    EXPECT_FALSE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("bar"));
    
    div->ToggleClass("baz");
    EXPECT_TRUE(div->HasClass("baz"));
    
    div->ToggleClass("baz");
    EXPECT_FALSE(div->HasClass("baz"));
}

TEST_F(DOMIntegrationTest, StyleOperations) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetStyle("color", "red");
    div->SetStyle("font-size", "16px");
    
    EXPECT_EQ(div->GetStyle("color"), "red");
    EXPECT_EQ(div->GetStyle("font-size"), "16px");
}

TEST_F(DOMIntegrationTest, TextContent) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetTextContent("Hello World");
    EXPECT_EQ(div->GetTextContent(), "Hello World");
}

TEST_F(DOMIntegrationTest, ChildNodeOperations) {
    auto doc = CreateDocument();
    auto parent = doc->CreateElement("div");
    
    auto child1 = doc->CreateElement("span");
    auto child2 = doc->CreateElement("span");
    auto child3 = doc->CreateElement("span");
    
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);
    
    auto& children = parent->GetChildNodes();
    EXPECT_EQ(children.size(), 3);
    
    parent->RemoveChild(child2);
    EXPECT_EQ(parent->GetChildNodes().size(), 2);
}

TEST_F(DOMIntegrationTest, QuerySelector) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto container = doc->CreateElement("div");
    container->SetAttribute("id", "container");
    container->AddClass("wrapper");
    body->AppendChild(container);
    
    auto item1 = doc->CreateElement("span");
    item1->AddClass("item");
    container->AppendChild(item1);
    
    auto item2 = doc->CreateElement("span");
    item2->AddClass("item");
    container->AppendChild(item2);
    
    // 通过 ID 查询
    auto found = doc->GetElementById("container");
    EXPECT_NE(found, nullptr);
    
    // 通过选择器查询
    auto items = container->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 2);
}

TEST_F(DOMIntegrationTest, CloneNode) {
    auto doc = CreateDocument();
    auto original = doc->CreateElement("div");
    original->SetAttribute("id", "original");
    original->AddClass("test");
    
    auto child = doc->CreateElement("span");
    child->SetTextContent("Child");
    original->AppendChild(child);
    
    // 深度克隆
    auto clone = std::dynamic_pointer_cast<Element>(original->CloneNode(true));
    EXPECT_NE(clone, nullptr);
    EXPECT_TRUE(clone->HasClass("test"));
    EXPECT_EQ(clone->GetChildNodes().size(), 1);
}

TEST_F(DOMIntegrationTest, InnerHTML) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    div->SetInnerHTML("<span>Test</span>");
    
    auto innerHTML = div->GetInnerHTML();
    EXPECT_TRUE(innerHTML.find("span") != std::string::npos);
}

} // namespace test
} // namespace lightui
