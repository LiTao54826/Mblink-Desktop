/**
 * @file test_html_button_element.cpp
 * @brief HTML Button元素单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/dom/html_button_element.h"
#include "core/dom/html_form_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 辅助函数 ==========

// 创建一个已初始化的Document和一个button元素
std::shared_ptr<HTMLButtonElement> CreateTestButton() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));
    doc->GetBody()->AppendChild(button);
    return button;
}

// ========== 基础功能测试 ==========

TEST(HTMLButtonElement, Construction) {
    auto button = CreateTestButton();

    EXPECT_EQ(button->GetTagName(), "button");
    EXPECT_EQ(button->GetType(), "submit");
    EXPECT_FALSE(button->GetDisabled());
    EXPECT_EQ(button->GetName(), "");
    EXPECT_EQ(button->GetValue(), "");
}

TEST(HTMLButtonElement, TypeAttribute) {
    auto button = CreateTestButton();

    // 测试所有有效类型
    button->SetType("submit");
    EXPECT_EQ(button->GetType(), "submit");
    EXPECT_EQ(button->GetAttribute("type"), "submit");

    button->SetType("reset");
    EXPECT_EQ(button->GetType(), "reset");
    EXPECT_EQ(button->GetAttribute("type"), "reset");

    button->SetType("button");
    EXPECT_EQ(button->GetType(), "button");
    EXPECT_EQ(button->GetAttribute("type"), "button");

    // 测试无效类型（应该使用默认值submit）
    button->SetType("invalid");
    EXPECT_EQ(button->GetType(), "submit");
}

TEST(HTMLButtonElement, TypeAttributeCaseInsensitive) {
    auto button = CreateTestButton();

    // 测试大小写不敏感
    button->SetType("SUBMIT");
    EXPECT_EQ(button->GetType(), "submit");

    button->SetType("Reset");
    EXPECT_EQ(button->GetType(), "reset");

    button->SetType("BUTTON");
    EXPECT_EQ(button->GetType(), "button");
}

TEST(HTMLButtonElement, DisabledState) {
    auto button = CreateTestButton();

    // 初始状态
    EXPECT_FALSE(button->GetDisabled());
    EXPECT_FALSE(button->HasPseudoClass(":disabled"));
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));

    // 设置禁用
    button->SetDisabled(true);
    EXPECT_TRUE(button->GetDisabled());
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));
    EXPECT_FALSE(button->HasPseudoClass(":enabled"));
    EXPECT_EQ(button->GetAttribute("disabled"), "");

    // 取消禁用
    button->SetDisabled(false);
    EXPECT_FALSE(button->GetDisabled());
    EXPECT_FALSE(button->HasPseudoClass(":disabled"));
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));
    EXPECT_FALSE(button->HasAttribute("disabled"));
}

TEST(HTMLButtonElement, NameAttribute) {
    auto button = CreateTestButton();

    button->SetName("submit-button");
    EXPECT_EQ(button->GetName(), "submit-button");
    EXPECT_EQ(button->GetAttribute("name"), "submit-button");

    button->SetName("another-name");
    EXPECT_EQ(button->GetName(), "another-name");
}

TEST(HTMLButtonElement, ValueAttribute) {
    auto button = CreateTestButton();

    button->SetValue("Submit Form");
    EXPECT_EQ(button->GetValue(), "Submit Form");
    EXPECT_EQ(button->GetAttribute("value"), "Submit Form");

    button->SetValue("Click Me");
    EXPECT_EQ(button->GetValue(), "Click Me");
}

// ========== 表单关联测试 ==========

TEST(HTMLButtonElement, FormAssociation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));

    // 添加button到form
    form->AppendChild(button);
    doc->GetBody()->AppendChild(form);

    // 验证关联
    auto associated_form = button->GetForm();
    EXPECT_NE(associated_form, nullptr);
    EXPECT_EQ(associated_form, form);
}

TEST(HTMLButtonElement, NoFormAssociation) {
    auto button = CreateTestButton();

    // 没有父元素时应该返回nullptr（button在body下，不在form下）
    auto associated_form = button->GetForm();
    EXPECT_EQ(associated_form, nullptr);
}

TEST(HTMLButtonElement, NestedFormAssociation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    auto div = doc->CreateElement("div");
    auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));

    // 嵌套结构: form > div > button
    form->AppendChild(div);
    div->AppendChild(button);
    doc->GetBody()->AppendChild(form);

    // 应该能找到祖先form
    auto associated_form = button->GetForm();
    EXPECT_NE(associated_form, nullptr);
    EXPECT_EQ(associated_form, form);
}

// ========== SetAttribute测试 ==========

TEST(HTMLButtonElement, SetAttributeDisabled) {
    auto button = CreateTestButton();

    // 通过SetAttribute设置disabled
    button->SetAttribute("disabled", "");
    EXPECT_TRUE(button->GetDisabled());

    button->SetAttribute("disabled", "disabled");
    EXPECT_TRUE(button->GetDisabled());

    // 移除disabled属性
    button->RemoveAttribute("disabled");
    EXPECT_FALSE(button->GetDisabled());
}

TEST(HTMLButtonElement, SetAttributeType) {
    auto button = CreateTestButton();

    button->SetAttribute("type", "reset");
    EXPECT_EQ(button->GetType(), "reset");

    button->SetAttribute("type", "button");
    EXPECT_EQ(button->GetType(), "button");
}

TEST(HTMLButtonElement, SetAttributeName) {
    auto button = CreateTestButton();

    button->SetAttribute("name", "my-button");
    EXPECT_EQ(button->GetName(), "my-button");
}

TEST(HTMLButtonElement, SetAttributeValue) {
    auto button = CreateTestButton();

    button->SetAttribute("value", "Click Here");
    EXPECT_EQ(button->GetValue(), "Click Here");
}

// ========== 验证API测试 ==========

TEST(HTMLButtonElement, CheckValidity) {
    auto button = CreateTestButton();

    // button元素总是有效的
    EXPECT_TRUE(button->CheckValidity());

    button->SetDisabled(true);
    EXPECT_TRUE(button->CheckValidity());
}

TEST(HTMLButtonElement, ReportValidity) {
    auto button = CreateTestButton();

    // button元素总是有效的
    EXPECT_TRUE(button->ReportValidity());
}

TEST(HTMLButtonElement, SetCustomValidity) {
    auto button = CreateTestButton();

    // 设置自定义验证消息（虽然button不使用它）
    button->SetCustomValidity("Custom error");

    // button仍然有效
    EXPECT_TRUE(button->CheckValidity());
}

// ========== 伪类状态测试 ==========

TEST(HTMLButtonElement, PseudoClassEnabled) {
    auto button = CreateTestButton();

    // 默认启用
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));
    EXPECT_FALSE(button->HasPseudoClass(":disabled"));
}

TEST(HTMLButtonElement, PseudoClassDisabled) {
    auto button = CreateTestButton();

    button->SetDisabled(true);
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));
    EXPECT_FALSE(button->HasPseudoClass(":enabled"));
}

TEST(HTMLButtonElement, PseudoClassToggle) {
    auto button = CreateTestButton();

    // 切换禁用状态
    button->SetDisabled(true);
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));

    button->SetDisabled(false);
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));

    button->SetDisabled(true);
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));
}

// ========== React兼容性测试 ==========

TEST(HTMLButtonElement, ReactAttributeUpdate) {
    auto button = CreateTestButton();

    // 模拟React更新属性
    button->SetAttribute("disabled", "true");
    EXPECT_TRUE(button->GetDisabled());

    button->SetAttribute("type", "reset");
    EXPECT_EQ(button->GetType(), "reset");

    button->SetAttribute("name", "react-button");
    EXPECT_EQ(button->GetName(), "react-button");

    button->SetAttribute("value", "React Value");
    EXPECT_EQ(button->GetValue(), "React Value");
}

TEST(HTMLButtonElement, ReactAttributeRemoval) {
    auto button = CreateTestButton();

    button->SetDisabled(true);
    EXPECT_TRUE(button->GetDisabled());

    // 模拟React移除disabled属性
    button->RemoveAttribute("disabled");
    EXPECT_FALSE(button->GetDisabled());
}

// ========== 性能测试 ==========

TEST(HTMLButtonElement, PerformanceCreation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 平均创建时间应该 < 1ms (1000微秒)
    double avg_time = duration.count() / 1000.0;
    EXPECT_LT(avg_time, 1000.0);

    std::cout << "Average button creation time: " << avg_time << " microseconds" << std::endl;
}

TEST(HTMLButtonElement, PerformanceAttributeUpdate) {
    auto button = CreateTestButton();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        button->SetDisabled(i % 2 == 0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 平均更新时间应该 < 0.1ms (100微秒)
    double avg_time = duration.count() / 10000.0;
    EXPECT_LT(avg_time, 100.0);

    std::cout << "Average attribute update time: " << avg_time << " microseconds" << std::endl;
}

