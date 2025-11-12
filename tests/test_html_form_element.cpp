/**
 * @file test_html_form_element.cpp
 * @brief HTML Form元素单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/dom/html_form_element.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_select_element.h"
#include "core/dom/html_option_element.h"
#include "core/dom/html_button_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 辅助函数 ==========

std::shared_ptr<HTMLFormElement> CreateTestForm() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    return form;
}

// ========== 基础属性测试 ==========

TEST(HTMLFormElement, Construction) {
    auto form = CreateTestForm();
    ASSERT_NE(form, nullptr);
    EXPECT_EQ(form->GetTagName(), "form");
}

TEST(HTMLFormElement, ActionProperty) {
    auto form = CreateTestForm();
    
    form->SetAction("/submit");
    EXPECT_EQ(form->GetAction(), "/submit");
    EXPECT_EQ(form->GetAttribute("action"), "/submit");
}

TEST(HTMLFormElement, MethodProperty) {
    auto form = CreateTestForm();
    
    // 默认是get
    EXPECT_EQ(form->GetMethod(), "get");
    
    // 设置为post
    form->SetMethod("post");
    EXPECT_EQ(form->GetMethod(), "post");
    EXPECT_EQ(form->GetAttribute("method"), "post");
    
    // 设置为GET（大小写不敏感）
    form->SetMethod("GET");
    EXPECT_EQ(form->GetMethod(), "get");
    
    // 无效值应该被忽略
    form->SetMethod("invalid");
    EXPECT_EQ(form->GetMethod(), "get");  // 保持原值
}

TEST(HTMLFormElement, EnctypeProperty) {
    auto form = CreateTestForm();
    
    // 默认值
    EXPECT_EQ(form->GetEnctype(), "application/x-www-form-urlencoded");
    
    // 设置为multipart/form-data
    form->SetEnctype("multipart/form-data");
    EXPECT_EQ(form->GetEnctype(), "multipart/form-data");
    
    // 设置为text/plain
    form->SetEnctype("text/plain");
    EXPECT_EQ(form->GetEnctype(), "text/plain");
    
    // 无效值应该被忽略
    form->SetEnctype("invalid");
    EXPECT_EQ(form->GetEnctype(), "text/plain");  // 保持原值
}

TEST(HTMLFormElement, TargetProperty) {
    auto form = CreateTestForm();
    
    form->SetTarget("_blank");
    EXPECT_EQ(form->GetTarget(), "_blank");
    EXPECT_EQ(form->GetAttribute("target"), "_blank");
}

// ========== 表单控件集合测试 ==========

TEST(HTMLFormElement, GetElements) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加各种表单控件
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input);
    
    auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(doc->CreateElement("textarea"));
    form->AppendChild(textarea);
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    form->AppendChild(select);
    
    auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));
    form->AppendChild(button);
    
    auto elements = form->GetElements();
    EXPECT_EQ(elements.size(), 4u);
    EXPECT_EQ(form->GetLength(), 4);
}

// ========== 表单数据收集测试 ==========

TEST(HTMLFormElement, FormDataURLEncoded) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加input
    auto input1 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input1);
    input1->SetAttribute("name", "username");
    input1->SetValue("john");
    
    auto input2 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input2);
    input2->SetAttribute("name", "email");
    input2->SetValue("john@example.com");
    
    std::string data = form->GetFormDataURLEncoded();
    EXPECT_EQ(data, "username=john&email=john%40example.com");
}

TEST(HTMLFormElement, FormDataWithCheckbox) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加checkbox
    auto checkbox1 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(checkbox1);
    checkbox1->SetAttribute("type", "checkbox");
    checkbox1->SetAttribute("name", "agree");
    checkbox1->SetValue("yes");
    checkbox1->SetChecked(true);
    
    auto checkbox2 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(checkbox2);
    checkbox2->SetAttribute("type", "checkbox");
    checkbox2->SetAttribute("name", "newsletter");
    checkbox2->SetValue("yes");
    checkbox2->SetChecked(false);  // 未选中，不应该包含在数据中
    
    std::string data = form->GetFormDataURLEncoded();
    EXPECT_EQ(data, "agree=yes");
}

TEST(HTMLFormElement, FormDataWithSelect) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    form->AppendChild(select);
    select->SetAttribute("name", "country");
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option1);
    option1->SetValue("us");
    option1->SetText("United States");
    
    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option2);
    option2->SetValue("cn");
    option2->SetText("China");
    option2->SetSelected(true);
    
    std::string data = form->GetFormDataURLEncoded();
    EXPECT_EQ(data, "country=cn");
}

TEST(HTMLFormElement, FormDataJSON) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    auto input1 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input1);
    input1->SetAttribute("name", "username");
    input1->SetValue("john");
    
    auto input2 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input2);
    input2->SetAttribute("name", "age");
    input2->SetValue("30");
    
    std::string json = form->GetFormDataJSON();
    // JSON格式可能有不同的键顺序，所以检查包含关系
    EXPECT_TRUE(json.find("\"username\":\"john\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"age\":\"30\"") != std::string::npos);
}

// ========== 表单验证测试 ==========

TEST(HTMLFormElement, CheckValidity) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加required input
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input);
    input->SetAttribute("required", "");
    input->SetValue("");
    
    // 空值应该无效
    EXPECT_FALSE(form->CheckValidity());
    
    // 设置值后应该有效
    input->SetValue("test");
    EXPECT_TRUE(form->CheckValidity());
}

// ========== 表单重置测试 ==========

TEST(HTMLFormElement, Reset) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加input with default value
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input);
    input->SetAttribute("value", "default");
    input->SetValue("changed");
    
    EXPECT_EQ(input->GetValue(), "changed");
    
    // 重置表单
    form->Reset();
    
    EXPECT_EQ(input->GetValue(), "default");
}

TEST(HTMLFormElement, ResetCheckbox) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加checkbox with default checked
    auto checkbox = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(checkbox);
    checkbox->SetAttribute("type", "checkbox");
    checkbox->SetAttribute("checked", "");
    checkbox->SetChecked(false);  // 改变状态
    
    EXPECT_FALSE(checkbox->GetChecked());
    
    // 重置表单
    form->Reset();
    
    EXPECT_TRUE(checkbox->GetChecked());
}

// ========== 性能测试 ==========

TEST(HTMLFormElement, PerformanceFormDataCollection) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);
    
    // 添加100个input
    for (int i = 0; i < 100; ++i) {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
        form->AppendChild(input);
        input->SetAttribute("name", "field" + std::to_string(i));
        input->SetValue("value" + std::to_string(i));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        auto data = form->GetFormDataURLEncoded();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average form data collection time (100 fields): "
              << duration.count() / 100.0 << " microseconds" << std::endl;

    EXPECT_LT(duration.count() / 100.0, 2000.0);  // 平均应该小于2ms
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

