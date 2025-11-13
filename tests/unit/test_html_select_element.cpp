/**
 * @file test_html_select_element.cpp
 * @brief HTML Select和Option元素单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/dom/html_select_element.h"
#include "core/dom/html_option_element.h"
#include "core/dom/html_form_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 辅助函数 ==========

std::shared_ptr<HTMLSelectElement> CreateTestSelect() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    doc->GetBody()->AppendChild(select);
    return select;
}

std::shared_ptr<HTMLOptionElement> CreateTestOption(const std::string& text = "", const std::string& value = "") {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    // 必须添加到document中，这样GetOwnerDocument()才能正常工作
    doc->GetBody()->AppendChild(option);
    if (!text.empty()) {
        option->SetText(text);
    }
    if (!value.empty()) {
        option->SetValue(value);
    }
    return option;
}

// ========== HTMLOptionElement 测试 ==========

TEST(HTMLOptionElement, Construction) {
    auto option = CreateTestOption();
    ASSERT_NE(option, nullptr);
    EXPECT_EQ(option->GetTagName(), "option");
    EXPECT_FALSE(option->GetDisabled());
    EXPECT_FALSE(option->GetSelected());
}

TEST(HTMLOptionElement, TextProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    doc->GetBody()->AppendChild(option);

    option->SetText("Option 1");
    EXPECT_EQ(option->GetText(), "Option 1");

    option->SetText("Updated");
    EXPECT_EQ(option->GetText(), "Updated");
}

TEST(HTMLOptionElement, ValueProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    doc->GetBody()->AppendChild(option);

    // 没有value属性时，返回text
    option->SetText("Display Text");
    EXPECT_EQ(option->GetValue(), "Display Text");

    // 设置value属性后，返回value
    option->SetValue("value1");
    EXPECT_EQ(option->GetValue(), "value1");
    EXPECT_EQ(option->GetAttribute("value"), "value1");
}

TEST(HTMLOptionElement, DisabledState) {
    auto option = CreateTestOption();
    
    EXPECT_FALSE(option->GetDisabled());
    EXPECT_TRUE(option->HasPseudoClass(":enabled"));
    EXPECT_FALSE(option->HasPseudoClass(":disabled"));
    
    option->SetDisabled(true);
    EXPECT_TRUE(option->GetDisabled());
    EXPECT_EQ(option->GetAttribute("disabled"), "");
    EXPECT_FALSE(option->HasPseudoClass(":enabled"));
    EXPECT_TRUE(option->HasPseudoClass(":disabled"));
    
    option->SetDisabled(false);
    EXPECT_FALSE(option->GetDisabled());
    EXPECT_TRUE(option->HasPseudoClass(":enabled"));
    EXPECT_FALSE(option->HasPseudoClass(":disabled"));
}

TEST(HTMLOptionElement, LabelProperty) {
    auto option = CreateTestOption("Text Content");
    
    // 没有label属性时，返回text
    EXPECT_EQ(option->GetLabel(), "Text Content");
    
    // 设置label属性后，返回label
    option->SetLabel("Label Text");
    EXPECT_EQ(option->GetLabel(), "Label Text");
    EXPECT_EQ(option->GetAttribute("label"), "Label Text");
}

// ========== HTMLSelectElement 测试 ==========

TEST(HTMLSelectElement, Construction) {
    auto select = CreateTestSelect();
    ASSERT_NE(select, nullptr);
    EXPECT_EQ(select->GetTagName(), "select");
    EXPECT_FALSE(select->GetDisabled());
    EXPECT_FALSE(select->GetMultiple());
    EXPECT_EQ(select->GetType(), "select-one");
}

TEST(HTMLSelectElement, MultipleProperty) {
    auto select = CreateTestSelect();
    
    EXPECT_FALSE(select->GetMultiple());
    EXPECT_EQ(select->GetType(), "select-one");
    
    select->SetMultiple(true);
    EXPECT_TRUE(select->GetMultiple());
    EXPECT_EQ(select->GetAttribute("multiple"), "");
    EXPECT_EQ(select->GetType(), "select-multiple");
    
    select->SetMultiple(false);
    EXPECT_FALSE(select->GetMultiple());
    EXPECT_EQ(select->GetType(), "select-one");
}

TEST(HTMLSelectElement, DisabledState) {
    auto select = CreateTestSelect();
    
    EXPECT_FALSE(select->GetDisabled());
    EXPECT_TRUE(select->HasPseudoClass(":enabled"));
    
    select->SetDisabled(true);
    EXPECT_TRUE(select->GetDisabled());
    EXPECT_EQ(select->GetAttribute("disabled"), "");
    EXPECT_TRUE(select->HasPseudoClass(":disabled"));
    
    select->SetDisabled(false);
    EXPECT_FALSE(select->GetDisabled());
    EXPECT_TRUE(select->HasPseudoClass(":enabled"));
}

TEST(HTMLSelectElement, NameProperty) {
    auto select = CreateTestSelect();
    
    select->SetName("country");
    EXPECT_EQ(select->GetName(), "country");
    EXPECT_EQ(select->GetAttribute("name"), "country");
}

TEST(HTMLSelectElement, SizeProperty) {
    auto select = CreateTestSelect();
    
    EXPECT_EQ(select->GetSize(), 0u);
    
    select->SetSize(5);
    EXPECT_EQ(select->GetSize(), 5u);
    EXPECT_EQ(select->GetAttribute("size"), "5");
}

TEST(HTMLSelectElement, RequiredProperty) {
    auto select = CreateTestSelect();
    
    EXPECT_FALSE(select->GetRequired());
    
    select->SetRequired(true);
    EXPECT_TRUE(select->GetRequired());
    EXPECT_EQ(select->GetAttribute("required"), "");
    
    select->SetRequired(false);
    EXPECT_FALSE(select->GetRequired());
}

// ========== Select + Option 集成测试 ==========

TEST(HTMLSelectElement, AddOptions) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    doc->GetBody()->AppendChild(select);
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option1);  // 先添加到DOM树
    option1->SetText("Option 1");  // 然后设置text
    option1->SetValue("1");

    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option2);  // 先添加到DOM树
    option2->SetText("Option 2");  // 然后设置text
    option2->SetValue("2");
    
    auto options = select->GetOptions();
    EXPECT_EQ(options.size(), 2u);
    EXPECT_EQ(select->GetLength(), 2u);
    EXPECT_EQ(options[0]->GetText(), "Option 1");
    EXPECT_EQ(options[1]->GetText(), "Option 2");
}

TEST(HTMLSelectElement, SingleSelection) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    doc->GetBody()->AppendChild(select);
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option1->SetValue("1");
    select->AppendChild(option1);
    
    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option2->SetValue("2");
    select->AppendChild(option2);
    
    // 初始没有选中
    EXPECT_EQ(select->GetSelectedIndex(), -1);
    EXPECT_EQ(select->GetValue(), "");
    
    // 选中第一个
    select->SetSelectedIndex(0);
    EXPECT_EQ(select->GetSelectedIndex(), 0);
    EXPECT_EQ(select->GetValue(), "1");
    EXPECT_TRUE(option1->GetSelected());
    EXPECT_FALSE(option2->GetSelected());
    
    // 选中第二个（第一个应该自动取消）
    select->SetSelectedIndex(1);
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_EQ(select->GetValue(), "2");
    EXPECT_FALSE(option1->GetSelected());
    EXPECT_TRUE(option2->GetSelected());
}

TEST(HTMLSelectElement, SetValueByValue) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    doc->GetBody()->AppendChild(select);
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option1->SetValue("apple");
    select->AppendChild(option1);
    
    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option2->SetValue("banana");
    select->AppendChild(option2);
    
    select->SetValue("banana");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(option2->GetSelected());
    EXPECT_FALSE(option1->GetSelected());
}

TEST(HTMLSelectElement, MultipleSelection) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    select->SetMultiple(true);
    doc->GetBody()->AppendChild(select);
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option1->SetValue("1");
    select->AppendChild(option1);
    
    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option2->SetValue("2");
    select->AppendChild(option2);
    
    auto option3 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option3->SetValue("3");
    select->AppendChild(option3);
    
    // 选中多个
    option1->SetSelected(true);
    option3->SetSelected(true);
    
    auto selected = select->GetSelectedOptions();
    EXPECT_EQ(selected.size(), 2u);
    EXPECT_TRUE(option1->GetSelected());
    EXPECT_FALSE(option2->GetSelected());
    EXPECT_TRUE(option3->GetSelected());
    
    // selectedIndex返回第一个选中的
    EXPECT_EQ(select->GetSelectedIndex(), 0);
}

TEST(HTMLSelectElement, Validation) {
    auto select = CreateTestSelect();
    
    // 非required时总是有效
    EXPECT_TRUE(select->CheckValidity());
    
    // required但没有选中时无效
    select->SetRequired(true);
    EXPECT_FALSE(select->CheckValidity());
    
    // 添加并选中option后有效
    auto doc = select->GetOwnerDocument();
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    option->SetValue("test");
    select->AppendChild(option);
    select->SetSelectedIndex(0);
    EXPECT_TRUE(select->CheckValidity());
}

TEST(HTMLSelectElement, OptionIndex) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    doc->GetBody()->AppendChild(select);
    
    auto option1 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option1);
    
    auto option2 = std::dynamic_pointer_cast<HTMLOptionElement>(doc->CreateElement("option"));
    select->AppendChild(option2);
    
    EXPECT_EQ(option1->GetIndex(), 0);
    EXPECT_EQ(option2->GetIndex(), 1);
}

// ========== 性能测试 ==========

TEST(HTMLSelectElement, PerformanceCreation) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto doc = std::make_shared<Document>();
        doc->Initialize();
        auto select = std::dynamic_pointer_cast<HTMLSelectElement>(doc->CreateElement("select"));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average select creation time: " << duration.count() / 1000.0 << " microseconds" << std::endl;

    EXPECT_LT(duration.count() / 1000.0, 200.0);  // 平均应该小于200微秒
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

