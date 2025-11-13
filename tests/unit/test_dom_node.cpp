/**
 * @file test_dom_node.cpp
 * @brief DOM Node 类单元测试
 */

#include <gtest/gtest.h>
#include "core/dom/element.h"
#include "core/dom/text.h"
#include <memory>

using namespace lightui;

// ========== 测试 Node 构造和基本属性 ==========

TEST(DOMNodeTest, Constructor) {
    auto text = std::make_shared<Text>("Hello");
    
    EXPECT_EQ(text->GetNodeType(), NodeType::TEXT_NODE);
    EXPECT_EQ(text->GetParentNode(), nullptr);
    EXPECT_EQ(text->GetChildNodes().size(), 0);
    EXPECT_TRUE(text->IsDirty());
}

// ========== 测试 AppendChild ==========

TEST(DOMNodeTest, AppendChild) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(child->GetParentNode(), parent);
    EXPECT_EQ(parent->GetFirstChild(), child);
    EXPECT_EQ(parent->GetLastChild(), child);
    EXPECT_TRUE(parent->IsDirty());
}

TEST(DOMNodeTest, AppendMultipleChildren) {
    auto parent = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    auto child3 = std::make_shared<Text>("Hello");
    
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 3);
    EXPECT_EQ(parent->GetFirstChild(), child1);
    EXPECT_EQ(parent->GetLastChild(), child3);
    EXPECT_EQ(parent->GetChildNodes()[1], child2);
}

TEST(DOMNodeTest, AppendChildReparent) {
    auto parent1 = std::make_shared<Element>("div");
    auto parent2 = std::make_shared<Element>("section");
    auto child = std::make_shared<Element>("span");
    
    // 先添加到parent1
    parent1->AppendChild(child);
    EXPECT_EQ(parent1->GetChildNodes().size(), 1);
    EXPECT_EQ(child->GetParentNode(), parent1);
    
    // 再添加到parent2，应该自动从parent1移除
    parent2->AppendChild(child);
    EXPECT_EQ(parent1->GetChildNodes().size(), 0);
    EXPECT_EQ(parent2->GetChildNodes().size(), 1);
    EXPECT_EQ(child->GetParentNode(), parent2);
}

TEST(DOMNodeTest, AppendChildNull) {
    auto parent = std::make_shared<Element>("div");
    
    EXPECT_THROW(parent->AppendChild(nullptr), std::invalid_argument);
}

// ========== 测试 InsertBefore ==========

TEST(DOMNodeTest, InsertBefore) {
    auto parent = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    
    parent->AppendChild(child1);
    parent->InsertBefore(child2, child1);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 2);
    EXPECT_EQ(parent->GetChildNodes()[0], child2);
    EXPECT_EQ(parent->GetChildNodes()[1], child1);
    EXPECT_EQ(parent->GetFirstChild(), child2);
}

TEST(DOMNodeTest, InsertBeforeNull) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    // ref_child为nullptr时，等同于AppendChild
    parent->InsertBefore(child, nullptr);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetFirstChild(), child);
}

TEST(DOMNodeTest, InsertBeforeNotFound) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    auto ref_child = std::make_shared<Element>("p");
    
    EXPECT_THROW(parent->InsertBefore(child, ref_child), std::invalid_argument);
}

// ========== 测试 RemoveChild ==========

TEST(DOMNodeTest, RemoveChild) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    auto removed = parent->RemoveChild(child);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 0);
    EXPECT_EQ(child->GetParentNode(), nullptr);
    EXPECT_EQ(removed, child);
}

TEST(DOMNodeTest, RemoveChildNotFound) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    EXPECT_THROW(parent->RemoveChild(child), std::invalid_argument);
}

TEST(DOMNodeTest, RemoveChildNull) {
    auto parent = std::make_shared<Element>("div");
    
    EXPECT_THROW(parent->RemoveChild(nullptr), std::invalid_argument);
}

// ========== 测试 ReplaceChild ==========

TEST(DOMNodeTest, ReplaceChild) {
    auto parent = std::make_shared<Element>("div");
    auto old_child = std::make_shared<Element>("span");
    auto new_child = std::make_shared<Element>("p");
    
    parent->AppendChild(old_child);
    auto replaced = parent->ReplaceChild(new_child, old_child);
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetFirstChild(), new_child);
    EXPECT_EQ(new_child->GetParentNode(), parent);
    EXPECT_EQ(old_child->GetParentNode(), nullptr);
    EXPECT_EQ(replaced, old_child);
}

TEST(DOMNodeTest, ReplaceChildNotFound) {
    auto parent = std::make_shared<Element>("div");
    auto old_child = std::make_shared<Element>("span");
    auto new_child = std::make_shared<Element>("p");
    
    EXPECT_THROW(parent->ReplaceChild(new_child, old_child), std::invalid_argument);
}

// ========== 测试兄弟节点访问 ==========

TEST(DOMNodeTest, GetNextSibling) {
    auto parent = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    auto child3 = std::make_shared<Element>("a");
    
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);
    
    EXPECT_EQ(child1->GetNextSibling(), child2);
    EXPECT_EQ(child2->GetNextSibling(), child3);
    EXPECT_EQ(child3->GetNextSibling(), nullptr);
}

TEST(DOMNodeTest, GetPreviousSibling) {
    auto parent = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    auto child3 = std::make_shared<Element>("a");
    
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);
    
    EXPECT_EQ(child3->GetPreviousSibling(), child2);
    EXPECT_EQ(child2->GetPreviousSibling(), child1);
    EXPECT_EQ(child1->GetPreviousSibling(), nullptr);
}

// ========== 测试 Contains ==========

TEST(DOMNodeTest, Contains) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    auto grandchild = std::make_shared<Element>("a");
    
    parent->AppendChild(child);
    child->AppendChild(grandchild);
    
    EXPECT_TRUE(parent->Contains(child));
    EXPECT_TRUE(parent->Contains(grandchild));
    EXPECT_TRUE(child->Contains(grandchild));
    EXPECT_FALSE(child->Contains(parent));
    EXPECT_FALSE(grandchild->Contains(parent));
}

TEST(DOMNodeTest, ContainsNull) {
    auto parent = std::make_shared<Element>("div");
    
    EXPECT_FALSE(parent->Contains(nullptr));
}

// ========== 测试 GetTextContent / SetTextContent ==========

TEST(DOMNodeTest, GetTextContent) {
    auto div = std::make_shared<Element>("div");
    auto text1 = std::make_shared<Text>("Hello ");
    auto span = std::make_shared<Element>("span");
    auto text2 = std::make_shared<Text>("World");
    
    div->AppendChild(text1);
    div->AppendChild(span);
    span->AppendChild(text2);
    
    EXPECT_EQ(div->GetTextContent(), "Hello World");
}

TEST(DOMNodeTest, SetTextContent) {
    auto div = std::make_shared<Element>("div");
    auto child1 = std::make_shared<Element>("span");
    auto child2 = std::make_shared<Element>("p");
    
    div->AppendChild(child1);
    div->AppendChild(child2);
    
    div->SetTextContent("New Text");
    
    EXPECT_EQ(div->GetChildNodes().size(), 1);
    EXPECT_EQ(div->GetTextContent(), "New Text");
    EXPECT_EQ(div->GetFirstChild()->GetNodeType(), NodeType::TEXT_NODE);
}

TEST(DOMNodeTest, SetTextContentEmpty) {
    auto div = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    div->AppendChild(child);
    div->SetTextContent("");
    
    EXPECT_EQ(div->GetChildNodes().size(), 0);
    EXPECT_EQ(div->GetTextContent(), "");
}

// ========== 测试脏标记 ==========

TEST(DOMNodeTest, MarkDirty) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    
    // 清除脏标记
    parent->ClearDirty();
    child->ClearDirty();
    
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(child->IsDirty());
    
    // 标记child为脏，应该向上传播
    child->MarkDirty();
    
    EXPECT_TRUE(child->IsDirty());
    EXPECT_TRUE(parent->IsDirty());
}

// ========== 测试 Text 节点 ==========

TEST(DOMTextTest, Constructor) {
    auto text = std::make_shared<Text>("Hello World");
    
    EXPECT_EQ(text->GetNodeType(), NodeType::TEXT_NODE);
    EXPECT_EQ(text->GetData(), "Hello World");
    EXPECT_EQ(text->GetTextContent(), "Hello World");
}

TEST(DOMTextTest, SetData) {
    auto text = std::make_shared<Text>("Hello");
    
    text->SetData("World");
    
    EXPECT_EQ(text->GetData(), "World");
    EXPECT_EQ(text->GetTextContent(), "World");
}

TEST(DOMTextTest, CloneNode) {
    auto text = std::make_shared<Text>("Hello World");
    auto cloned = text->CloneNode(false);
    
    EXPECT_NE(cloned, text);
    EXPECT_EQ(cloned->GetNodeType(), NodeType::TEXT_NODE);
    EXPECT_EQ(cloned->GetTextContent(), "Hello World");
}

TEST(DOMTextTest, SetTextContent) {
    auto text = std::make_shared<Text>("Hello");
    
    text->SetTextContent("World");
    
    EXPECT_EQ(text->GetData(), "World");
    EXPECT_EQ(text->GetTextContent(), "World");
}

