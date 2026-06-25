/**
 * @file test_text.cpp
 * @brief Text 节点单元测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/text.h"
#include "dom/element.h"

namespace mblink {
namespace test {

class TextTest : public DOMTestBase {};

TEST_F(TextTest, CreateTextNode) {
    auto text = CreateTextNode("Hello World");
    EXPECT_EQ(text->GetNodeType(), NodeType::TEXT_NODE);
}

TEST_F(TextTest, GetData) {
    auto text = CreateTextNode("Test Data");
    EXPECT_EQ(text->GetData(), "Test Data");
}

TEST_F(TextTest, SetData) {
    auto text = CreateTextNode("Original");
    text->SetData("Modified");
    EXPECT_EQ(text->GetData(), "Modified");
}

TEST_F(TextTest, GetTextContent) {
    auto text = CreateTextNode("Content");
    EXPECT_EQ(text->GetTextContent(), "Content");
}

TEST_F(TextTest, SetTextContent) {
    auto text = CreateTextNode("Original");
    text->SetTextContent("New Content");
    EXPECT_EQ(text->GetData(), "New Content");
}

TEST_F(TextTest, EmptyText) {
    auto text = CreateTextNode("");
    EXPECT_EQ(text->GetData(), "");
}

TEST_F(TextTest, WhitespaceText) {
    auto text = CreateTextNode("   \n\t  ");
    EXPECT_EQ(text->GetData(), "   \n\t  ");
}

TEST_F(TextTest, UnicodeText) {
    auto text = CreateTextNode("你好世界 🌍");
    EXPECT_EQ(text->GetData(), "你好世界 🌍");
}

TEST_F(TextTest, CloneNode) {
    auto text = CreateTextNode("Clone Me");
    auto clone = text->CloneNode(false);

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetNodeType(), NodeType::TEXT_NODE);

    auto textClone = std::dynamic_pointer_cast<Text>(clone);
    EXPECT_NE(textClone, nullptr);
    EXPECT_EQ(textClone->GetData(), "Clone Me");
}

TEST_F(TextTest, TextInElement) {
    auto elem = CreateElement("div");
    auto text = CreateTextNode("Inside Element");

    elem->AppendChild(text);

    EXPECT_EQ(text->GetParentNode(), elem);
    EXPECT_EQ(elem->GetTextContent(), "Inside Element");
}

TEST_F(TextTest, MultipleTextNodes) {
    auto elem = CreateElement("div");
    auto text1 = CreateTextNode("Hello ");
    auto text2 = CreateTextNode("World");

    elem->AppendChild(text1);
    elem->AppendChild(text2);

    EXPECT_EQ(elem->GetTextContent(), "Hello World");
}

} // namespace test
} // namespace mblink
