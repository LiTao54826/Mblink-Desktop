#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>

using namespace lightui;

/**
 * @brief 表单元素测试
 * 
 * 测试各种表单元素的解析和属性处理
 */
class FormElementsTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Form Elements Test</title></head>
            <body>
                <form id="testForm" action="/submit" method="post">
                    <!-- Text inputs -->
                    <input type="text" id="username" name="username" value="John" required />
                    <input type="password" id="password" name="password" required />
                    <input type="email" id="email" name="email" value="john@example.com" />
                    <input type="tel" id="phone" name="phone" />
                    <input type="url" id="website" name="website" />
                    <input type="search" id="search" name="search" />
                    
                    <!-- Number inputs -->
                    <input type="number" id="age" name="age" min="0" max="120" value="25" />
                    <input type="range" id="volume" name="volume" min="0" max="100" value="50" />
                    
                    <!-- Date/Time inputs -->
                    <input type="date" id="birthdate" name="birthdate" />
                    <input type="time" id="time" name="time" />
                    <input type="datetime-local" id="datetime" name="datetime" />
                    <input type="month" id="month" name="month" />
                    <input type="week" id="week" name="week" />
                    
                    <!-- Checkboxes -->
                    <input type="checkbox" id="agree" name="agree" checked />
                    <input type="checkbox" id="newsletter" name="newsletter" />
                    
                    <!-- Radio buttons -->
                    <input type="radio" id="male" name="gender" value="male" checked />
                    <input type="radio" id="female" name="gender" value="female" />
                    <input type="radio" id="other" name="gender" value="other" />
                    
                    <!-- File input -->
                    <input type="file" id="avatar" name="avatar" accept="image/*" />
                    <input type="file" id="documents" name="documents" multiple />
                    
                    <!-- Hidden input -->
                    <input type="hidden" id="token" name="token" value="abc123" />
                    
                    <!-- Color input -->
                    <input type="color" id="color" name="color" value="#ff0000" />
                    
                    <!-- Textarea -->
                    <textarea id="bio" name="bio" rows="5" cols="40">Hello World</textarea>
                    <textarea id="empty" name="empty"></textarea>
                    
                    <!-- Select -->
                    <select id="country" name="country">
                        <option value="us">United States</option>
                        <option value="uk" selected>United Kingdom</option>
                        <option value="ca">Canada</option>
                    </select>
                    
                    <select id="languages" name="languages" multiple>
                        <option value="en" selected>English</option>
                        <option value="es">Spanish</option>
                        <option value="fr" selected>French</option>
                    </select>
                    
                    <!-- Buttons -->
                    <button type="submit" id="submitBtn">Submit</button>
                    <button type="reset" id="resetBtn">Reset</button>
                    <button type="button" id="customBtn" disabled>Custom</button>
                    <input type="submit" id="submitInput" value="Submit Input" />
                    <input type="reset" id="resetInput" value="Reset Input" />
                    <input type="button" id="buttonInput" value="Button Input" />
                    
                    <!-- Label -->
                    <label for="username">Username:</label>
                    <label>
                        <input type="checkbox" name="inline" />
                        Inline Label
                    </label>
                    
                    <!-- Fieldset and Legend -->
                    <fieldset id="personalInfo">
                        <legend>Personal Information</legend>
                        <input type="text" name="firstName" />
                    </fieldset>
                    
                    <!-- Datalist -->
                    <input list="browsers" name="browser" />
                    <datalist id="browsers">
                        <option value="Chrome" />
                        <option value="Firefox" />
                        <option value="Safari" />
                    </datalist>
                    
                    <!-- Output -->
                    <output id="result" name="result" for="age volume">75</output>
                </form>
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== 表单元素测试 ==========

TEST_F(FormElementsTest, FormElement) {
    auto form = doc.QuerySelector("#testForm");
    ASSERT_NE(form, nullptr);
    EXPECT_EQ(form->GetAttribute("action"), "/submit");
    EXPECT_EQ(form->GetAttribute("method"), "post");
}

// ========== 文本输入测试 ==========

TEST_F(FormElementsTest, TextInput) {
    auto input = doc.QuerySelector("#username");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "text");
    EXPECT_EQ(input->GetAttribute("name"), "username");
    EXPECT_EQ(input->GetAttribute("value"), "John");
    EXPECT_EQ(input->GetAttribute("required"), "");
}

TEST_F(FormElementsTest, PasswordInput) {
    auto input = doc.QuerySelector("#password");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "password");
}

TEST_F(FormElementsTest, EmailInput) {
    auto input = doc.QuerySelector("#email");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "email");
    EXPECT_EQ(input->GetAttribute("value"), "john@example.com");
}

TEST_F(FormElementsTest, TelInput) {
    auto input = doc.QuerySelector("#phone");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "tel");
}

TEST_F(FormElementsTest, UrlInput) {
    auto input = doc.QuerySelector("#website");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "url");
}

TEST_F(FormElementsTest, SearchInput) {
    auto input = doc.QuerySelector("#search");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "search");
}

// ========== 数字输入测试 ==========

TEST_F(FormElementsTest, NumberInput) {
    auto input = doc.QuerySelector("#age");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "number");
    EXPECT_EQ(input->GetAttribute("min"), "0");
    EXPECT_EQ(input->GetAttribute("max"), "120");
    EXPECT_EQ(input->GetAttribute("value"), "25");
}

TEST_F(FormElementsTest, RangeInput) {
    auto input = doc.QuerySelector("#volume");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "range");
    EXPECT_EQ(input->GetAttribute("min"), "0");
    EXPECT_EQ(input->GetAttribute("max"), "100");
    EXPECT_EQ(input->GetAttribute("value"), "50");
}

// ========== 日期/时间输入测试 ==========

TEST_F(FormElementsTest, DateInput) {
    auto input = doc.QuerySelector("#birthdate");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "date");
}

TEST_F(FormElementsTest, TimeInput) {
    auto input = doc.QuerySelector("#time");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "time");
}

TEST_F(FormElementsTest, DateTimeLocalInput) {
    auto input = doc.QuerySelector("#datetime");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "datetime-local");
}

TEST_F(FormElementsTest, MonthInput) {
    auto input = doc.QuerySelector("#month");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "month");
}

TEST_F(FormElementsTest, WeekInput) {
    auto input = doc.QuerySelector("#week");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "week");
}

// ========== 复选框和单选按钮测试 ==========

TEST_F(FormElementsTest, CheckboxChecked) {
    auto input = doc.QuerySelector("#agree");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "checkbox");
    EXPECT_EQ(input->GetAttribute("checked"), "");
}

TEST_F(FormElementsTest, CheckboxUnchecked) {
    auto input = doc.QuerySelector("#newsletter");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "checkbox");
    EXPECT_EQ(input->GetAttribute("checked"), "");  // 未选中时属性不存在，返回空字符串
}

TEST_F(FormElementsTest, RadioButtonChecked) {
    auto input = doc.QuerySelector("#male");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "radio");
    EXPECT_EQ(input->GetAttribute("name"), "gender");
    EXPECT_EQ(input->GetAttribute("value"), "male");
    EXPECT_EQ(input->GetAttribute("checked"), "");
}

TEST_F(FormElementsTest, RadioButtonGroup) {
    auto radios = doc.QuerySelectorAll("input[name='gender']");
    EXPECT_EQ(radios.size(), 3);
}

// ========== 文件输入测试 ==========

TEST_F(FormElementsTest, FileInput) {
    auto input = doc.QuerySelector("#avatar");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "file");
    EXPECT_EQ(input->GetAttribute("accept"), "image/*");
}

TEST_F(FormElementsTest, FileInputMultiple) {
    auto input = doc.QuerySelector("#documents");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "file");
    EXPECT_EQ(input->GetAttribute("multiple"), "");
}

// ========== 其他输入类型测试 ==========

TEST_F(FormElementsTest, HiddenInput) {
    auto input = doc.QuerySelector("#token");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "hidden");
    EXPECT_EQ(input->GetAttribute("value"), "abc123");
}

TEST_F(FormElementsTest, ColorInput) {
    auto input = doc.QuerySelector("#color");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "color");
    EXPECT_EQ(input->GetAttribute("value"), "#ff0000");
}

// ========== Textarea 测试 ==========

TEST_F(FormElementsTest, TextareaWithContent) {
    auto textarea = doc.QuerySelector("#bio");
    ASSERT_NE(textarea, nullptr);
    EXPECT_EQ(textarea->GetAttribute("rows"), "5");
    EXPECT_EQ(textarea->GetAttribute("cols"), "40");
    EXPECT_EQ(textarea->GetTextContent(), "Hello World");
}

TEST_F(FormElementsTest, TextareaEmpty) {
    auto textarea = doc.QuerySelector("#empty");
    ASSERT_NE(textarea, nullptr);
    EXPECT_EQ(textarea->GetTextContent(), "");
}

// ========== Select 测试 ==========

TEST_F(FormElementsTest, SelectSingle) {
    auto select = doc.QuerySelector("#country");
    ASSERT_NE(select, nullptr);
    
    auto options = select->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 3);
}

TEST_F(FormElementsTest, SelectMultiple) {
    auto select = doc.QuerySelector("#languages");
    ASSERT_NE(select, nullptr);
    EXPECT_EQ(select->GetAttribute("multiple"), "");
    
    auto options = select->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 3);
}

TEST_F(FormElementsTest, OptionSelected) {
    auto option = doc.QuerySelector("#country option[value='uk']");
    ASSERT_NE(option, nullptr);
    EXPECT_EQ(option->GetAttribute("selected"), "");
}

// ========== 按钮测试 ==========

TEST_F(FormElementsTest, ButtonSubmit) {
    auto button = doc.QuerySelector("#submitBtn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetAttribute("type"), "submit");
    EXPECT_EQ(button->GetTextContent(), "Submit");
}

TEST_F(FormElementsTest, ButtonReset) {
    auto button = doc.QuerySelector("#resetBtn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetAttribute("type"), "reset");
}

TEST_F(FormElementsTest, ButtonDisabled) {
    auto button = doc.QuerySelector("#customBtn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetAttribute("disabled"), "");
}

TEST_F(FormElementsTest, InputSubmit) {
    auto input = doc.QuerySelector("#submitInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "submit");
    EXPECT_EQ(input->GetAttribute("value"), "Submit Input");
}

// ========== Label 测试 ==========

TEST_F(FormElementsTest, LabelFor) {
    auto label = doc.QuerySelector("label[for='username']");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->GetTextContent(), "Username:");
}

TEST_F(FormElementsTest, LabelInline) {
    auto labels = doc.QuerySelectorAll("label");
    EXPECT_GE(labels.size(), 2);
}

// ========== Fieldset 和 Legend 测试 ==========

TEST_F(FormElementsTest, Fieldset) {
    auto fieldset = doc.QuerySelector("#personalInfo");
    ASSERT_NE(fieldset, nullptr);
    
    auto legend = fieldset->QuerySelector("legend");
    ASSERT_NE(legend, nullptr);
    EXPECT_EQ(legend->GetTextContent(), "Personal Information");
}

// ========== Datalist 测试 ==========

TEST_F(FormElementsTest, Datalist) {
    auto datalist = doc.QuerySelector("#browsers");
    ASSERT_NE(datalist, nullptr);
    
    auto options = datalist->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 3);
}

// ========== Output 测试 ==========

TEST_F(FormElementsTest, Output) {
    auto output = doc.QuerySelector("#result");
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(output->GetAttribute("for"), "age volume");
    EXPECT_EQ(output->GetTextContent(), "75");
}

// ========== 查询测试 ==========

TEST_F(FormElementsTest, QueryAllInputs) {
    auto inputs = doc.QuerySelectorAll("input");
    EXPECT_GE(inputs.size(), 20);  // 至少有 20 个 input 元素
}

TEST_F(FormElementsTest, QueryRequiredInputs) {
    auto required = doc.QuerySelectorAll("input[required]");
    EXPECT_EQ(required.size(), 2);  // username 和 password
}

TEST_F(FormElementsTest, QueryCheckedInputs) {
    auto checked = doc.QuerySelectorAll("input:checked");
    EXPECT_EQ(checked.size(), 2);  // agree 和 male
}

TEST_F(FormElementsTest, QueryDisabledButtons) {
    auto disabled = doc.QuerySelectorAll("button:disabled");
    EXPECT_EQ(disabled.size(), 1);  // customBtn
}

