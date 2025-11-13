/**
 * @file test_html_label_element.cpp
 * @brief HTML Label元素单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/dom/html_label_element.h"
#include "core/dom/html_input_element.h"
#include "core/dom/html_button_element.h"
#include "core/dom/html_select_element.h"
#include "core/dom/html_textarea_element.h"
#include "core/dom/html_form_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 辅助函数 ==========

std::shared_ptr<HTMLLabelElement> CreateTestLabel() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    return label;
}

// ========== 基础属性测试 ==========

TEST(HTMLLabelElement, Construction) {
    auto label = CreateTestLabel();
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->GetTagName(), "label");
    EXPECT_EQ(label->GetHtmlFor(), "");
}

TEST(HTMLLabelElement, HtmlForProperty) {
    auto label = CreateTestLabel();
    
    label->SetHtmlFor("username");
    EXPECT_EQ(label->GetHtmlFor(), "username");
    EXPECT_EQ(label->GetAttribute("for"), "username");
}

TEST(HTMLLabelElement, SetAttributeFor) {
    auto label = CreateTestLabel();
    
    label->SetAttribute("for", "password");
    EXPECT_EQ(label->GetHtmlFor(), "password");
}

TEST(HTMLLabelElement, RemoveAttributeFor) {
    auto label = CreateTestLabel();
    
    label->SetAttribute("for", "email");
    EXPECT_EQ(label->GetHtmlFor(), "email");
    
    label->RemoveAttribute("for");
    EXPECT_EQ(label->GetHtmlFor(), "");
}

// ========== 控件关联测试 ==========

TEST(HTMLLabelElement, GetControlById) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建input并添加到document（先添加再设置id，确保id注册）
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(input);
    input->SetAttribute("id", "username");

    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("username");
    doc->GetBody()->AppendChild(label);

    // 验证关联
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, input);
}

TEST(HTMLLabelElement, GetControlByDescendant) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    
    // 创建input作为label的子元素
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    label->AppendChild(input);
    
    // 验证关联（没有for属性，应该找到后代input）
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, input);
}

TEST(HTMLLabelElement, GetControlPriorityForOverDescendant) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建两个input（先添加再设置id）
    auto input1 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(input1);
    input1->SetAttribute("id", "input1");

    auto input2 = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(input2);

    // 创建label，for指向input1，但包含input2作为子元素
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("input1");
    label->AppendChild(input2);
    doc->GetBody()->AppendChild(label);

    // 验证关联（for属性优先级高于后代元素）
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, input1);
}

TEST(HTMLLabelElement, GetControlNoControl) {
    auto label = CreateTestLabel();
    
    // 没有关联控件
    auto control = label->GetControl();
    EXPECT_EQ(control, nullptr);
}

// ========== 表单关联测试 ==========

TEST(HTMLLabelElement, GetFormThroughControl) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建form
    auto form = std::dynamic_pointer_cast<HTMLFormElement>(doc->CreateElement("form"));
    doc->GetBody()->AppendChild(form);

    // 创建input并添加到form（先添加再设置id）
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    form->AppendChild(input);
    input->SetAttribute("id", "username");

    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("username");
    doc->GetBody()->AppendChild(label);

    // 验证label的form属性
    auto label_form = label->GetForm();
    ASSERT_NE(label_form, nullptr);
    EXPECT_EQ(label_form, form);
}

// ========== 点击处理测试 ==========

TEST(HTMLLabelElement, HandleClickFocusControl) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建input并添加到document（先添加再设置id）
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(input);
    input->SetAttribute("id", "username");

    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("username");
    doc->GetBody()->AppendChild(label);

    // 监听focus事件
    bool focus_fired = false;
    input->AddEventListener("focus", [&](std::shared_ptr<Event> event) {
        focus_fired = true;
    });

    // 点击label
    label->HandleClick();

    EXPECT_TRUE(focus_fired);
}

TEST(HTMLLabelElement, HandleClickToggleCheckbox) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建checkbox并添加到document（先添加再设置属性）
    auto checkbox = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(checkbox);
    checkbox->SetAttribute("type", "checkbox");
    checkbox->SetAttribute("id", "agree");

    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("agree");
    doc->GetBody()->AppendChild(label);

    // 初始状态：未选中
    EXPECT_FALSE(checkbox->GetChecked());

    // 点击label，应该选中
    label->HandleClick();
    EXPECT_TRUE(checkbox->GetChecked());

    // 再次点击label，应该取消选中
    label->HandleClick();
    EXPECT_FALSE(checkbox->GetChecked());
}

TEST(HTMLLabelElement, HandleClickActivateRadio) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    // 创建radio并添加到document（先添加再设置属性）
    auto radio = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    doc->GetBody()->AppendChild(radio);
    radio->SetAttribute("type", "radio");
    radio->SetAttribute("id", "option1");

    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    label->SetHtmlFor("option1");
    doc->GetBody()->AppendChild(label);

    // 初始状态：未选中
    EXPECT_FALSE(radio->GetChecked());

    // 点击label，应该选中
    label->HandleClick();
    EXPECT_TRUE(radio->GetChecked());

    // 再次点击label，应该保持选中（radio不能取消选中）
    label->HandleClick();
    EXPECT_TRUE(radio->GetChecked());
}

// ========== 可标签化元素测试 ==========

TEST(HTMLLabelElement, LabelableButton) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    
    // 创建button作为label的子元素
    auto button = std::dynamic_pointer_cast<HTMLButtonElement>(doc->CreateElement("button"));
    label->AppendChild(button);
    
    // 验证关联
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, button);
}

TEST(HTMLLabelElement, LabelableSelect) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    
    // 创建select作为label的子元素
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    label->AppendChild(select);
    
    // 验证关联
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, select);
}

TEST(HTMLLabelElement, LabelableTextarea) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    
    // 创建textarea作为label的子元素
    auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(doc->CreateElement("textarea"));
    label->AppendChild(textarea);
    
    // 验证关联
    auto control = label->GetControl();
    ASSERT_NE(control, nullptr);
    EXPECT_EQ(control, textarea);
}

TEST(HTMLLabelElement, NotLabelableHiddenInput) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    // 创建label
    auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
    doc->GetBody()->AppendChild(label);
    
    // 创建hidden input作为label的子元素
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc->CreateElement("input"));
    input->SetAttribute("type", "hidden");
    label->AppendChild(input);
    
    // 验证关联（hidden input不可标签化）
    auto control = label->GetControl();
    EXPECT_EQ(control, nullptr);
}

// ========== React兼容性测试 ==========

TEST(HTMLLabelElement, ReactAttributeUpdate) {
    auto label = CreateTestLabel();
    
    // 模拟React更新for属性
    label->SetAttribute("for", "username");
    EXPECT_EQ(label->GetHtmlFor(), "username");
    
    // 模拟React移除for属性
    label->RemoveAttribute("for");
    EXPECT_EQ(label->GetHtmlFor(), "");
}

// ========== 性能测试 ==========

TEST(HTMLLabelElement, PerformanceCreation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto label = std::dynamic_pointer_cast<HTMLLabelElement>(doc->CreateElement("label"));
        doc->GetBody()->AppendChild(label);
        label->SetHtmlFor("control" + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average label creation time: " 
              << duration.count() / 1000.0 << " microseconds" << std::endl;
    
    EXPECT_LT(duration.count() / 1000.0, 100.0);  // 平均应该小于100μs
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

