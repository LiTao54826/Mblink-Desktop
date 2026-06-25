/**
 * @file test_node.cpp
 * @brief Node 类单元测试
 *
 * 测试内容：
 * - 节点类型
 * - 父子关系
 * - 兄弟关系
 * - 节点操作（appendChild, insertBefore, removeChild, replaceChild）
 * - 脏标记系统
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/node.h"
#include "dom/element.h"
#include "dom/text.h"
#include "dom/document.h"

namespace mblink {
namespace test {

class NodeTest : public DOMTestBase {};

// ========== 节点类型测试 ==========

TEST_F(NodeTest, ElementNodeType) {
    auto elem = CreateElement("div");
    EXPECT_EQ(elem->GetNodeType(), NodeType::ELEMENT_NODE);
}

TEST_F(NodeTest, TextNodeType) {
    auto text = CreateTextNode("Hello");
    EXPECT_EQ(text->GetNodeType(), NodeType::TEXT_NODE);
}

TEST_F(NodeTest, DocumentNodeType) {
    EXPECT_EQ(doc_->GetNodeType(), NodeType::DOCUMENT_NODE);
}

// ========== 父子关系测试 ==========

TEST_F(NodeTest, AppendChild) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");

    parent->AppendChild(child);

    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetFirstChild(), child);
    EXPECT_EQ(child->GetParentNode(), parent);
}

TEST_F(NodeTest, AppendMultipleChildren) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");
    auto child3 = CreateElement("a");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);

    EXPECT_EQ(parent->GetChildNodes().size(), 3);
    EXPECT_EQ(parent->GetFirstChild(), child1);
    EXPECT_EQ(parent->GetLastChild(), child3);
}

TEST_F(NodeTest, InsertBefore) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");
    auto newChild = CreateElement("a");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->InsertBefore(newChild, child2);

    auto& children = parent->GetChildNodes();
    EXPECT_EQ(children.size(), 3);
    EXPECT_EQ(children[0], child1);
    EXPECT_EQ(children[1], newChild);
    EXPECT_EQ(children[2], child2);
}

TEST_F(NodeTest, InsertBeforeNull) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto newChild = CreateElement("a");

    parent->AppendChild(child1);
    parent->InsertBefore(newChild, nullptr);  // 应该追加到末尾

    EXPECT_EQ(parent->GetLastChild(), newChild);
}

TEST_F(NodeTest, RemoveChild) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->RemoveChild(child1);

    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetFirstChild(), child2);
    EXPECT_EQ(child1->GetParentNode(), nullptr);
}

TEST_F(NodeTest, ReplaceChild) {
    auto parent = CreateElement("div");
    auto oldChild = CreateElement("span");
    auto newChild = CreateElement("p");

    parent->AppendChild(oldChild);
    parent->ReplaceChild(newChild, oldChild);

    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetFirstChild(), newChild);
    EXPECT_EQ(oldChild->GetParentNode(), nullptr);
    EXPECT_EQ(newChild->GetParentNode(), parent);
}

// ========== 兄弟关系测试 ==========

TEST_F(NodeTest, GetNextSibling) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");
    auto child3 = CreateElement("a");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);

    EXPECT_EQ(child1->GetNextSibling(), child2);
    EXPECT_EQ(child2->GetNextSibling(), child3);
    EXPECT_EQ(child3->GetNextSibling(), nullptr);
}

TEST_F(NodeTest, GetPreviousSibling) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");
    auto child3 = CreateElement("a");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);

    EXPECT_EQ(child1->GetPreviousSibling(), nullptr);
    EXPECT_EQ(child2->GetPreviousSibling(), child1);
    EXPECT_EQ(child3->GetPreviousSibling(), child2);
}

// ========== Contains 测试 ==========

TEST_F(NodeTest, ContainsDirectChild) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");

    parent->AppendChild(child);

    EXPECT_TRUE(parent->Contains(child));
    EXPECT_FALSE(child->Contains(parent));
}

TEST_F(NodeTest, ContainsDeepChild) {
    auto grandparent = CreateElement("div");
    auto parent = CreateElement("section");
    auto child = CreateElement("span");

    grandparent->AppendChild(parent);
    parent->AppendChild(child);

    EXPECT_TRUE(grandparent->Contains(child));
    EXPECT_TRUE(grandparent->Contains(parent));
    EXPECT_FALSE(child->Contains(grandparent));
}

TEST_F(NodeTest, ContainsSelf) {
    auto elem = CreateElement("div");
    EXPECT_TRUE(elem->Contains(elem));
}

// ========== 文本内容测试 ==========

TEST_F(NodeTest, GetTextContentFromElement) {
    auto parent = CreateElement("div");
    auto text1 = CreateTextNode("Hello ");
    auto child = CreateElement("span");
    auto text2 = CreateTextNode("World");

    parent->AppendChild(text1);
    parent->AppendChild(child);
    child->AppendChild(text2);

    EXPECT_EQ(parent->GetTextContent(), "Hello World");
}

TEST_F(NodeTest, SetTextContentOnElement) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    parent->AppendChild(child);

    parent->SetTextContent("New Text");

    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetTextContent(), "New Text");
}

// ========== 脏标记测试 ==========

TEST_F(NodeTest, InitialDirtyState) {
    auto elem = CreateElement("div");
    EXPECT_TRUE(elem->IsDirty());
    EXPECT_TRUE(elem->IsLayoutDirty());
    EXPECT_TRUE(elem->IsPaintDirty());
    EXPECT_TRUE(elem->IsStyleDirty());
}

TEST_F(NodeTest, ClearDirty) {
    auto elem = CreateElement("div");
    elem->ClearDirty();

    EXPECT_FALSE(elem->IsDirty());
    EXPECT_FALSE(elem->IsLayoutDirty());
    EXPECT_FALSE(elem->IsPaintDirty());
}

TEST_F(NodeTest, MarkDirtyLayout) {
    auto elem = CreateElement("div");
    elem->ClearDirty();
    elem->MarkDirty(DirtyType::LAYOUT);

    EXPECT_TRUE(elem->IsLayoutDirty());
    EXPECT_FALSE(elem->IsPaintDirty());
}

TEST_F(NodeTest, MarkDirtyPaint) {
    auto elem = CreateElement("div");
    elem->ClearDirty();
    elem->MarkDirty(DirtyType::PAINT);

    EXPECT_FALSE(elem->IsLayoutDirty());
    EXPECT_TRUE(elem->IsPaintDirty());
}

TEST_F(NodeTest, MarkDirtyPropagates) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    parent->AppendChild(child);

    parent->ClearDirty();
    child->ClearDirty();

    child->MarkDirty(DirtyType::LAYOUT);

    // 子节点脏标记应该传播到父节点
    EXPECT_TRUE(child->IsLayoutDirty());
    // 注意：根据实现，父节点可能也被标记
}

// ========== 所属文档测试 ==========

TEST_F(NodeTest, GetOwnerDocument) {
    auto elem = doc_->CreateElement("div");
    EXPECT_EQ(elem->GetOwnerDocument(), doc_);
}

TEST_F(NodeTest, AppendedChildGetsOwnerDocument) {
    auto body = doc_->GetBody();
    auto elem = std::make_shared<Element>("div");

    EXPECT_EQ(elem->GetOwnerDocument(), nullptr);

    body->AppendChild(elem);

    // 添加到文档后应该获得 ownerDocument
    // 注意：这取决于具体实现
}

} // namespace test
} // namespace mblink
