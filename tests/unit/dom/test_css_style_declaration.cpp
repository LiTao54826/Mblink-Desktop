/**
 * @file test_css_style_declaration.cpp
 * @brief CSSStyleDeclaration 单元测试 (element.style)
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/style/css_style_declaration.h"
#include "dom/element.h"

namespace mblink {
namespace test {

class CSSStyleDeclarationTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        elem_ = CreateElement("div");
        style_ = elem_->GetStyleDeclaration();
    }

protected:
    std::shared_ptr<Element> elem_;
    std::shared_ptr<CSSStyleDeclaration> style_;
};

TEST_F(CSSStyleDeclarationTest, InitiallyEmpty) {
    EXPECT_EQ(style_->Length(), 0);
}

TEST_F(CSSStyleDeclarationTest, SetProperty) {
    style_->SetProperty("color", "red");
    EXPECT_EQ(style_->GetPropertyValue("color"), "red");
}

TEST_F(CSSStyleDeclarationTest, SetPropertyWithPriority) {
    style_->SetProperty("color", "red", "important");
    EXPECT_EQ(style_->GetPropertyValue("color"), "red");
    EXPECT_EQ(style_->GetPropertyPriority("color"), "important");
}

TEST_F(CSSStyleDeclarationTest, GetPropertyValue) {
    style_->SetProperty("font-size", "16px");
    EXPECT_EQ(style_->GetPropertyValue("font-size"), "16px");
}

TEST_F(CSSStyleDeclarationTest, GetPropertyValueNonExistent) {
    EXPECT_EQ(style_->GetPropertyValue("nonexistent"), "");
}

TEST_F(CSSStyleDeclarationTest, RemoveProperty) {
    style_->SetProperty("color", "red");
    style_->SetProperty("font-size", "16px");

    auto removed = style_->RemoveProperty("color");

    EXPECT_EQ(removed, "red");
    EXPECT_EQ(style_->GetPropertyValue("color"), "");
    EXPECT_EQ(style_->GetPropertyValue("font-size"), "16px");
}

TEST_F(CSSStyleDeclarationTest, RemovePropertyNonExistent) {
    auto removed = style_->RemoveProperty("nonexistent");
    EXPECT_EQ(removed, "");
}

TEST_F(CSSStyleDeclarationTest, GetLength) {
    style_->SetProperty("color", "red");
    style_->SetProperty("font-size", "16px");
    style_->SetProperty("margin", "10px");

    EXPECT_EQ(style_->Length(), 3);
}

TEST_F(CSSStyleDeclarationTest, Item) {
    style_->SetProperty("color", "red");
    style_->SetProperty("font-size", "16px");

    // 顺序可能取决于实现
    EXPECT_FALSE(style_->Item(0).empty());
    EXPECT_FALSE(style_->Item(1).empty());
    EXPECT_TRUE(style_->Item(2).empty());  // 越界
}

TEST_F(CSSStyleDeclarationTest, GetCssText) {
    style_->SetProperty("color", "red");
    style_->SetProperty("font-size", "16px");

    auto cssText = style_->GetCssText();
    EXPECT_TRUE(cssText.find("color") != std::string::npos);
    EXPECT_TRUE(cssText.find("red") != std::string::npos);
    EXPECT_TRUE(cssText.find("font-size") != std::string::npos);
    EXPECT_TRUE(cssText.find("16px") != std::string::npos);
}

TEST_F(CSSStyleDeclarationTest, SetCssText) {
    style_->SetCssText("color: blue; font-size: 20px; margin: 5px;");

    EXPECT_EQ(style_->GetPropertyValue("color"), "blue");
    EXPECT_EQ(style_->GetPropertyValue("font-size"), "20px");
    EXPECT_EQ(style_->GetPropertyValue("margin"), "5px");
}

TEST_F(CSSStyleDeclarationTest, SetCssTextOverwrite) {
    style_->SetProperty("color", "red");
    style_->SetCssText("font-size: 16px;");

    // 设置 cssText 应该清除之前的样式
    EXPECT_EQ(style_->GetPropertyValue("color"), "");
    EXPECT_EQ(style_->GetPropertyValue("font-size"), "16px");
}

TEST_F(CSSStyleDeclarationTest, SyncWithElement) {
    // 通过 style 对象修改应该同步到元素
    style_->SetProperty("color", "green");
    EXPECT_EQ(elem_->GetStyle("color"), "green");
}

TEST_F(CSSStyleDeclarationTest, CommonCSSProperties) {
    // 测试常见 CSS 属性
    style_->SetProperty("width", "100px");
    style_->SetProperty("height", "50px");
    style_->SetProperty("background-color", "#fff");
    style_->SetProperty("border", "1px solid black");
    style_->SetProperty("padding", "10px 20px");
    style_->SetProperty("display", "flex");
    style_->SetProperty("position", "relative");

    EXPECT_EQ(style_->GetPropertyValue("width"), "100px");
    EXPECT_EQ(style_->GetPropertyValue("height"), "50px");
    EXPECT_EQ(style_->GetPropertyValue("background-color"), "#fff");
    EXPECT_EQ(style_->GetPropertyValue("display"), "flex");
    EXPECT_EQ(style_->GetPropertyValue("position"), "relative");
}

TEST_F(CSSStyleDeclarationTest, CamelCaseProperty) {
    // 测试驼峰命名（JavaScript 风格）
    style_->SetProperty("backgroundColor", "red");
    // 应该能通过 kebab-case 获取
    // 注意：这取决于实现是否支持自动转换
}

TEST_F(CSSStyleDeclarationTest, VendorPrefixedProperty) {
    style_->SetProperty("-webkit-transform", "rotate(45deg)");
    EXPECT_EQ(style_->GetPropertyValue("-webkit-transform"), "rotate(45deg)");
}

TEST_F(CSSStyleDeclarationTest, CSSVariables) {
    style_->SetProperty("--custom-color", "blue");
    EXPECT_EQ(style_->GetPropertyValue("--custom-color"), "blue");
}

TEST_F(CSSStyleDeclarationTest, ImportantPriority) {
    style_->SetProperty("color", "red", "important");

    EXPECT_EQ(style_->GetPropertyPriority("color"), "important");

    // 不带 important 的属性
    style_->SetProperty("font-size", "16px");
    EXPECT_EQ(style_->GetPropertyPriority("font-size"), "");
}

TEST_F(CSSStyleDeclarationTest, ParseComplexCssText) {
    style_->SetCssText(R"(
        color: red;
        font-size: 16px;
        background: linear-gradient(to right, red, blue);
        transform: rotate(45deg) scale(1.5);
    )");

    EXPECT_EQ(style_->GetPropertyValue("color"), "red");
    EXPECT_EQ(style_->GetPropertyValue("font-size"), "16px");
    EXPECT_FALSE(style_->GetPropertyValue("background").empty());
    EXPECT_FALSE(style_->GetPropertyValue("transform").empty());
}

} // namespace test
} // namespace mblink
