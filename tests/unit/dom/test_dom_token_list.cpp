/**
 * @file test_dom_token_list.cpp
 * @brief DOMTokenList 单元测试 (classList)
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/utils/dom_token_list.h"
#include "dom/element.h"

namespace lightui {
namespace test {

class DOMTokenListTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        elem_ = CreateElement("div");
        classList_ = elem_->GetClassList();
    }

protected:
    std::shared_ptr<Element> elem_;
    std::shared_ptr<DOMTokenList> classList_;
};

TEST_F(DOMTokenListTest, InitiallyEmpty) {
    EXPECT_EQ(classList_->Length(), 0);
}

TEST_F(DOMTokenListTest, Add) {
    classList_->Add("foo");
    EXPECT_EQ(classList_->Length(), 1);
    EXPECT_TRUE(classList_->Contains("foo"));
}

TEST_F(DOMTokenListTest, AddMultiple) {
    classList_->Add("foo");
    classList_->Add("bar");
    classList_->Add("baz");

    EXPECT_EQ(classList_->Length(), 3);
    EXPECT_TRUE(classList_->Contains("foo"));
    EXPECT_TRUE(classList_->Contains("bar"));
    EXPECT_TRUE(classList_->Contains("baz"));
}

TEST_F(DOMTokenListTest, AddDuplicate) {
    classList_->Add("foo");
    classList_->Add("foo");

    EXPECT_EQ(classList_->Length(), 1);
}

TEST_F(DOMTokenListTest, Remove) {
    classList_->Add("foo");
    classList_->Add("bar");
    classList_->Remove("foo");

    EXPECT_EQ(classList_->Length(), 1);
    EXPECT_FALSE(classList_->Contains("foo"));
    EXPECT_TRUE(classList_->Contains("bar"));
}

TEST_F(DOMTokenListTest, RemoveNonExistent) {
    classList_->Add("foo");
    classList_->Remove("bar");  // 不存在的 class

    EXPECT_EQ(classList_->Length(), 1);
    EXPECT_TRUE(classList_->Contains("foo"));
}

TEST_F(DOMTokenListTest, Toggle) {
    // 添加
    bool result1 = classList_->Toggle("active");
    EXPECT_TRUE(result1);
    EXPECT_TRUE(classList_->Contains("active"));

    // 移除
    bool result2 = classList_->Toggle("active");
    EXPECT_FALSE(result2);
    EXPECT_FALSE(classList_->Contains("active"));
}

TEST_F(DOMTokenListTest, ToggleWithForce) {
    // force = true，强制添加
    bool result1 = classList_->Toggle("active", true);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(classList_->Contains("active"));

    // force = true，已存在，保持
    bool result2 = classList_->Toggle("active", true);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(classList_->Contains("active"));

    // force = false，强制移除
    bool result3 = classList_->Toggle("active", false);
    EXPECT_FALSE(result3);
    EXPECT_FALSE(classList_->Contains("active"));
}

TEST_F(DOMTokenListTest, Contains) {
    classList_->Add("foo");

    EXPECT_TRUE(classList_->Contains("foo"));
    EXPECT_FALSE(classList_->Contains("bar"));
}

TEST_F(DOMTokenListTest, Item) {
    classList_->Add("foo");
    classList_->Add("bar");
    classList_->Add("baz");

    // 注意：顺序可能取决于实现
    EXPECT_FALSE(classList_->Item(0).empty());
    EXPECT_FALSE(classList_->Item(1).empty());
    EXPECT_FALSE(classList_->Item(2).empty());
    EXPECT_TRUE(classList_->Item(3).empty());  // 越界
}

TEST_F(DOMTokenListTest, GetValue) {
    classList_->Add("foo");
    classList_->Add("bar");

    auto value = classList_->Value();
    EXPECT_TRUE(value.find("foo") != std::string::npos);
    EXPECT_TRUE(value.find("bar") != std::string::npos);
}

TEST_F(DOMTokenListTest, SetValue) {
    classList_->SetValue("a b c");

    EXPECT_EQ(classList_->Length(), 3);
    EXPECT_TRUE(classList_->Contains("a"));
    EXPECT_TRUE(classList_->Contains("b"));
    EXPECT_TRUE(classList_->Contains("c"));
}

TEST_F(DOMTokenListTest, Replace) {
    classList_->Add("old");
    classList_->Add("other");

    bool result = classList_->Replace("old", "new");

    EXPECT_TRUE(result);
    EXPECT_FALSE(classList_->Contains("old"));
    EXPECT_TRUE(classList_->Contains("new"));
    EXPECT_TRUE(classList_->Contains("other"));
}

TEST_F(DOMTokenListTest, ReplaceNonExistent) {
    classList_->Add("foo");

    bool result = classList_->Replace("bar", "baz");

    EXPECT_FALSE(result);
    EXPECT_TRUE(classList_->Contains("foo"));
}

TEST_F(DOMTokenListTest, SyncWithElement) {
    // 通过 classList 修改应该同步到元素
    classList_->Add("test-class");
    EXPECT_TRUE(elem_->HasClass("test-class"));

    classList_->Remove("test-class");
    EXPECT_FALSE(elem_->HasClass("test-class"));
}

TEST_F(DOMTokenListTest, WhitespaceHandling) {
    classList_->SetValue("  foo   bar  baz  ");

    EXPECT_EQ(classList_->Length(), 3);
    EXPECT_TRUE(classList_->Contains("foo"));
    EXPECT_TRUE(classList_->Contains("bar"));
    EXPECT_TRUE(classList_->Contains("baz"));
}

TEST_F(DOMTokenListTest, EmptyToken) {
    // 空 token 应该被忽略或抛出异常
    classList_->Add("");
    EXPECT_EQ(classList_->Length(), 0);
}

} // namespace test
} // namespace lightui
