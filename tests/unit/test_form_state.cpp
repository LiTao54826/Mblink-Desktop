#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>

using namespace lightui;

/**
 * @brief 表单状态管理测试
 * 
 * 测试表单元素的状态管理和数据收集
 */
class FormStateTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Form State Test</title></head>
            <body>
                <form id="userForm" name="userForm" action="/submit" method="post">
                    <!-- 文本输入 -->
                    <input type="text" id="firstName" name="firstName" value="John" />
                    <input type="text" id="lastName" name="lastName" value="Doe" />
                    <input type="email" id="email" name="email" value="john.doe@example.com" />
                    
                    <!-- 数字输入 -->
                    <input type="number" id="age" name="age" value="30" />
                    <input type="range" id="rating" name="rating" value="7" min="0" max="10" />
                    
                    <!-- 复选框 -->
                    <input type="checkbox" id="subscribe" name="subscribe" value="yes" checked />
                    <input type="checkbox" id="terms" name="terms" value="agreed" />
                    
                    <!-- 单选按钮 -->
                    <input type="radio" id="genderMale" name="gender" value="male" checked />
                    <input type="radio" id="genderFemale" name="gender" value="female" />
                    <input type="radio" id="genderOther" name="gender" value="other" />
                    
                    <!-- 选择框 -->
                    <select id="country" name="country">
                        <option value="us">United States</option>
                        <option value="uk" selected>United Kingdom</option>
                        <option value="ca">Canada</option>
                    </select>
                    
                    <select id="languages" name="languages" multiple>
                        <option value="en" selected>English</option>
                        <option value="es">Spanish</option>
                        <option value="fr" selected>French</option>
                        <option value="de">German</option>
                    </select>
                    
                    <!-- 文本域 -->
                    <textarea id="bio" name="bio">Software Developer</textarea>
                    
                    <!-- 隐藏字段 -->
                    <input type="hidden" id="userId" name="userId" value="12345" />
                    
                    <!-- 按钮 -->
                    <button type="submit" id="submitBtn">Submit</button>
                    <button type="reset" id="resetBtn">Reset</button>
                </form>
                
                <!-- 多个表单 -->
                <form id="loginForm" name="loginForm">
                    <input type="text" id="username" name="username" value="admin" />
                    <input type="password" id="password" name="password" value="secret" />
                </form>
                
                <!-- 表单外的元素 -->
                <input type="text" id="outsideInput" name="outsideInput" value="outside" />
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== 表单基本信息测试 ==========

TEST_F(FormStateTest, FormAttributes) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    EXPECT_EQ(form->GetAttribute("name"), "userForm");
    EXPECT_EQ(form->GetAttribute("action"), "/submit");
    EXPECT_EQ(form->GetAttribute("method"), "post");
}

TEST_F(FormStateTest, MultipleFormsExist) {
    auto forms = doc.QuerySelectorAll("form");
    EXPECT_EQ(forms.size(), 2);  // userForm 和 loginForm
}

// ========== 表单元素值测试 ==========

TEST_F(FormStateTest, TextInputValue) {
    auto input = doc.QuerySelector("#firstName");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("value"), "John");
    
    auto lastName = doc.QuerySelector("#lastName");
    ASSERT_NE(lastName, nullptr);
    EXPECT_EQ(lastName->GetAttribute("value"), "Doe");
}

TEST_F(FormStateTest, EmailValue) {
    auto input = doc.QuerySelector("#email");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("value"), "john.doe@example.com");
}

TEST_F(FormStateTest, NumberValue) {
    auto input = doc.QuerySelector("#age");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("value"), "30");
}

TEST_F(FormStateTest, RangeValue) {
    auto input = doc.QuerySelector("#rating");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("value"), "7");
    EXPECT_EQ(input->GetAttribute("min"), "0");
    EXPECT_EQ(input->GetAttribute("max"), "10");
}

// ========== 复选框状态测试 ==========

TEST_F(FormStateTest, CheckboxCheckedState) {
    auto subscribe = doc.QuerySelector("#subscribe");
    ASSERT_NE(subscribe, nullptr);
    EXPECT_EQ(subscribe->GetAttribute("checked"), "");
    EXPECT_EQ(subscribe->GetAttribute("value"), "yes");
}

TEST_F(FormStateTest, CheckboxUncheckedState) {
    auto terms = doc.QuerySelector("#terms");
    ASSERT_NE(terms, nullptr);
    EXPECT_EQ(terms->GetAttribute("checked"), "");  // 未选中时属性不存在
    EXPECT_EQ(terms->GetAttribute("value"), "agreed");
}

TEST_F(FormStateTest, AllCheckboxes) {
    auto checkboxes = doc.QuerySelectorAll("input[type='checkbox']");
    EXPECT_EQ(checkboxes.size(), 2);
}

// ========== 单选按钮状态测试 ==========

TEST_F(FormStateTest, RadioButtonCheckedState) {
    auto male = doc.QuerySelector("#genderMale");
    ASSERT_NE(male, nullptr);
    EXPECT_EQ(male->GetAttribute("checked"), "");
    EXPECT_EQ(male->GetAttribute("value"), "male");
}

TEST_F(FormStateTest, RadioButtonGroup) {
    auto radios = doc.QuerySelectorAll("input[name='gender']");
    EXPECT_EQ(radios.size(), 3);

    // 检查只有一个被选中（使用 :checked 伪类）
    auto checked = doc.QuerySelectorAll("input[name='gender']:checked");
    EXPECT_EQ(checked.size(), 1);
}

TEST_F(FormStateTest, RadioButtonValues) {
    auto male = doc.QuerySelector("#genderMale");
    auto female = doc.QuerySelector("#genderFemale");
    auto other = doc.QuerySelector("#genderOther");
    
    ASSERT_NE(male, nullptr);
    ASSERT_NE(female, nullptr);
    ASSERT_NE(other, nullptr);
    
    EXPECT_EQ(male->GetAttribute("value"), "male");
    EXPECT_EQ(female->GetAttribute("value"), "female");
    EXPECT_EQ(other->GetAttribute("value"), "other");
}

// ========== 选择框状态测试 ==========

TEST_F(FormStateTest, SelectSingleValue) {
    auto select = doc.QuerySelector("#country");
    ASSERT_NE(select, nullptr);
    
    auto options = select->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 3);
    
    // 检查选中的选项
    auto selectedOption = select->QuerySelector("option[selected]");
    ASSERT_NE(selectedOption, nullptr);
    EXPECT_EQ(selectedOption->GetAttribute("value"), "uk");
}

TEST_F(FormStateTest, SelectMultipleValues) {
    auto select = doc.QuerySelector("#languages");
    ASSERT_NE(select, nullptr);
    EXPECT_EQ(select->GetAttribute("multiple"), "");
    
    auto options = select->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 4);
    
    // 检查选中的选项
    auto selectedOptions = select->QuerySelectorAll("option[selected]");
    EXPECT_EQ(selectedOptions.size(), 2);  // en 和 fr
}

// ========== 文本域测试 ==========

TEST_F(FormStateTest, TextareaValue) {
    auto textarea = doc.QuerySelector("#bio");
    ASSERT_NE(textarea, nullptr);
    EXPECT_EQ(textarea->GetTextContent(), "Software Developer");
}

// ========== 隐藏字段测试 ==========

TEST_F(FormStateTest, HiddenFieldValue) {
    auto hidden = doc.QuerySelector("#userId");
    ASSERT_NE(hidden, nullptr);
    EXPECT_EQ(hidden->GetAttribute("type"), "hidden");
    EXPECT_EQ(hidden->GetAttribute("value"), "12345");
}

// ========== 表单数据收集测试 ==========

TEST_F(FormStateTest, CollectAllFormInputs) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    // 收集所有输入元素
    auto inputs = form->QuerySelectorAll("input");
    EXPECT_GE(inputs.size(), 10);  // 至少有 10 个 input 元素
}

TEST_F(FormStateTest, CollectFormInputsByName) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    // 按名称查找
    auto firstName = form->QuerySelector("input[name='firstName']");
    ASSERT_NE(firstName, nullptr);
    EXPECT_EQ(firstName->GetAttribute("value"), "John");
}

TEST_F(FormStateTest, CollectAllFormElements) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    // 收集所有表单元素
    auto inputs = form->QuerySelectorAll("input");
    auto selects = form->QuerySelectorAll("select");
    auto textareas = form->QuerySelectorAll("textarea");
    auto buttons = form->QuerySelectorAll("button");
    
    EXPECT_GE(inputs.size(), 10);
    EXPECT_EQ(selects.size(), 2);
    EXPECT_EQ(textareas.size(), 1);
    EXPECT_EQ(buttons.size(), 2);
}

// ========== 表单按钮测试 ==========

TEST_F(FormStateTest, SubmitButton) {
    auto button = doc.QuerySelector("#submitBtn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetAttribute("type"), "submit");
    EXPECT_EQ(button->GetTextContent(), "Submit");
}

TEST_F(FormStateTest, ResetButton) {
    auto button = doc.QuerySelector("#resetBtn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetAttribute("type"), "reset");
    EXPECT_EQ(button->GetTextContent(), "Reset");
}

// ========== 多表单测试 ==========

TEST_F(FormStateTest, LoginFormElements) {
    auto form = doc.QuerySelector("#loginForm");
    ASSERT_NE(form, nullptr);
    
    auto username = form->QuerySelector("#username");
    auto password = form->QuerySelector("#password");
    
    ASSERT_NE(username, nullptr);
    ASSERT_NE(password, nullptr);
    
    EXPECT_EQ(username->GetAttribute("value"), "admin");
    EXPECT_EQ(password->GetAttribute("value"), "secret");
}

TEST_F(FormStateTest, FormIsolation) {
    // 确保表单外的元素不被包含
    auto userForm = doc.QuerySelector("#userForm");
    auto outsideInput = userForm->QuerySelector("#outsideInput");
    
    EXPECT_EQ(outsideInput, nullptr);  // 表单外的元素不应该被找到
}

// ========== 表单元素查询测试 ==========

TEST_F(FormStateTest, QueryInputsByType) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    auto textInputs = form->QuerySelectorAll("input[type='text']");
    auto checkboxes = form->QuerySelectorAll("input[type='checkbox']");
    auto radios = form->QuerySelectorAll("input[type='radio']");
    
    EXPECT_EQ(textInputs.size(), 2);  // firstName, lastName
    EXPECT_EQ(checkboxes.size(), 2);  // subscribe, terms
    EXPECT_EQ(radios.size(), 3);      // gender options
}

TEST_F(FormStateTest, QueryCheckedElements) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    auto checked = form->QuerySelectorAll("input:checked");
    EXPECT_EQ(checked.size(), 2);  // subscribe checkbox 和 genderMale radio
}

TEST_F(FormStateTest, QuerySelectedOptions) {
    auto form = doc.QuerySelector("#userForm");
    ASSERT_NE(form, nullptr);
    
    auto selected = form->QuerySelectorAll("option[selected]");
    EXPECT_EQ(selected.size(), 3);  // uk (country) + en, fr (languages)
}

// ========== 表单名称测试 ==========

TEST_F(FormStateTest, FormNameAttribute) {
    auto userForm = doc.QuerySelector("#userForm");
    auto loginForm = doc.QuerySelector("#loginForm");
    
    ASSERT_NE(userForm, nullptr);
    ASSERT_NE(loginForm, nullptr);
    
    EXPECT_EQ(userForm->GetAttribute("name"), "userForm");
    EXPECT_EQ(loginForm->GetAttribute("name"), "loginForm");
}

TEST_F(FormStateTest, QueryFormByName) {
    auto form = doc.QuerySelector("form[name='userForm']");
    ASSERT_NE(form, nullptr);
    EXPECT_EQ(form->GetId(), "userForm");
}

