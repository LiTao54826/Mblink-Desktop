#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>

using namespace lightui;

/**
 * @brief 表单验证测试
 * 
 * 测试 HTML5 表单验证功能
 */
class FormValidationTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Form Validation Test</title></head>
            <body>
                <form id="validationForm">
                    <!-- Required 验证 -->
                    <input type="text" id="requiredText" name="requiredText" required />
                    <input type="email" id="requiredEmail" name="requiredEmail" required />
                    <textarea id="requiredTextarea" name="requiredTextarea" required></textarea>
                    <select id="requiredSelect" name="requiredSelect" required>
                        <option value="">-- Select --</option>
                        <option value="1">Option 1</option>
                    </select>
                    
                    <!-- Pattern 验证 -->
                    <input type="text" id="patternInput" name="patternInput" 
                           pattern="[A-Za-z]{3,}" title="At least 3 letters" />
                    <input type="text" id="phonePattern" name="phonePattern" 
                           pattern="[0-9]{3}-[0-9]{3}-[0-9]{4}" 
                           title="Format: 123-456-7890" />
                    <input type="text" id="zipPattern" name="zipPattern" 
                           pattern="[0-9]{5}" title="5 digit zip code" />
                    
                    <!-- Min/Max 验证 (数字) -->
                    <input type="number" id="ageInput" name="ageInput" 
                           min="18" max="100" />
                    <input type="number" id="quantityInput" name="quantityInput" 
                           min="1" max="10" value="5" />
                    <input type="range" id="volumeInput" name="volumeInput" 
                           min="0" max="100" value="50" />
                    
                    <!-- Min/Max 验证 (日期) -->
                    <input type="date" id="startDate" name="startDate" 
                           min="2024-01-01" max="2024-12-31" />
                    <input type="time" id="startTime" name="startTime" 
                           min="09:00" max="17:00" />
                    
                    <!-- MinLength/MaxLength 验证 -->
                    <input type="text" id="usernameInput" name="usernameInput" 
                           minlength="3" maxlength="20" />
                    <input type="password" id="passwordInput" name="passwordInput" 
                           minlength="8" maxlength="50" />
                    <textarea id="bioTextarea" name="bioTextarea" 
                              minlength="10" maxlength="500"></textarea>
                    
                    <!-- Email 验证 -->
                    <input type="email" id="emailInput" name="emailInput" />
                    <input type="email" id="multipleEmails" name="multipleEmails" multiple />
                    
                    <!-- URL 验证 -->
                    <input type="url" id="websiteInput" name="websiteInput" />
                    
                    <!-- Tel 验证 -->
                    <input type="tel" id="phoneInput" name="phoneInput" />
                    
                    <!-- Step 验证 -->
                    <input type="number" id="stepInput" name="stepInput" 
                           min="0" max="100" step="5" />
                    <input type="range" id="stepRange" name="stepRange" 
                           min="0" max="1" step="0.1" />
                    
                    <!-- 组合验证 -->
                    <input type="email" id="combinedInput" name="combinedInput" 
                           required pattern="[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$" 
                           minlength="5" maxlength="100" />
                    
                    <!-- 自定义验证消息 -->
                    <input type="text" id="customMessage" name="customMessage" 
                           required title="This field is required!" />
                    
                    <!-- Disabled 和 Readonly -->
                    <input type="text" id="disabledInput" name="disabledInput" 
                           required disabled />
                    <input type="text" id="readonlyInput" name="readonlyInput" 
                           required readonly value="readonly value" />
                    
                    <!-- Checkbox 和 Radio 验证 -->
                    <input type="checkbox" id="agreeCheckbox" name="agreeCheckbox" required />
                    <input type="radio" id="genderMale" name="gender" value="male" required />
                    <input type="radio" id="genderFemale" name="gender" value="female" required />
                </form>
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== Required 验证测试 ==========

TEST_F(FormValidationTest, RequiredAttribute) {
    auto input = doc.QuerySelector("#requiredText");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("required"), "");
}

TEST_F(FormValidationTest, RequiredEmail) {
    auto input = doc.QuerySelector("#requiredEmail");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "email");
    EXPECT_EQ(input->GetAttribute("required"), "");
}

TEST_F(FormValidationTest, RequiredTextarea) {
    auto textarea = doc.QuerySelector("#requiredTextarea");
    ASSERT_NE(textarea, nullptr);
    EXPECT_EQ(textarea->GetAttribute("required"), "");
}

TEST_F(FormValidationTest, RequiredSelect) {
    auto select = doc.QuerySelector("#requiredSelect");
    ASSERT_NE(select, nullptr);
    EXPECT_EQ(select->GetAttribute("required"), "");
}

// ========== Pattern 验证测试 ==========

TEST_F(FormValidationTest, PatternAttribute) {
    auto input = doc.QuerySelector("#patternInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("pattern"), "[A-Za-z]{3,}");
    EXPECT_EQ(input->GetAttribute("title"), "At least 3 letters");
}

TEST_F(FormValidationTest, PhonePattern) {
    auto input = doc.QuerySelector("#phonePattern");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("pattern"), "[0-9]{3}-[0-9]{3}-[0-9]{4}");
}

TEST_F(FormValidationTest, ZipPattern) {
    auto input = doc.QuerySelector("#zipPattern");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("pattern"), "[0-9]{5}");
}

// ========== Min/Max 验证测试 (数字) ==========

TEST_F(FormValidationTest, MinMaxNumber) {
    auto input = doc.QuerySelector("#ageInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("min"), "18");
    EXPECT_EQ(input->GetAttribute("max"), "100");
}

TEST_F(FormValidationTest, QuantityMinMax) {
    auto input = doc.QuerySelector("#quantityInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("min"), "1");
    EXPECT_EQ(input->GetAttribute("max"), "10");
    EXPECT_EQ(input->GetAttribute("value"), "5");
}

TEST_F(FormValidationTest, RangeMinMax) {
    auto input = doc.QuerySelector("#volumeInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "range");
    EXPECT_EQ(input->GetAttribute("min"), "0");
    EXPECT_EQ(input->GetAttribute("max"), "100");
}

// ========== Min/Max 验证测试 (日期/时间) ==========

TEST_F(FormValidationTest, DateMinMax) {
    auto input = doc.QuerySelector("#startDate");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "date");
    EXPECT_EQ(input->GetAttribute("min"), "2024-01-01");
    EXPECT_EQ(input->GetAttribute("max"), "2024-12-31");
}

TEST_F(FormValidationTest, TimeMinMax) {
    auto input = doc.QuerySelector("#startTime");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "time");
    EXPECT_EQ(input->GetAttribute("min"), "09:00");
    EXPECT_EQ(input->GetAttribute("max"), "17:00");
}

// ========== MinLength/MaxLength 验证测试 ==========

TEST_F(FormValidationTest, UsernameLength) {
    auto input = doc.QuerySelector("#usernameInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("minlength"), "3");
    EXPECT_EQ(input->GetAttribute("maxlength"), "20");
}

TEST_F(FormValidationTest, PasswordLength) {
    auto input = doc.QuerySelector("#passwordInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("minlength"), "8");
    EXPECT_EQ(input->GetAttribute("maxlength"), "50");
}

TEST_F(FormValidationTest, TextareaLength) {
    auto textarea = doc.QuerySelector("#bioTextarea");
    ASSERT_NE(textarea, nullptr);
    EXPECT_EQ(textarea->GetAttribute("minlength"), "10");
    EXPECT_EQ(textarea->GetAttribute("maxlength"), "500");
}

// ========== Email 验证测试 ==========

TEST_F(FormValidationTest, EmailType) {
    auto input = doc.QuerySelector("#emailInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "email");
}

TEST_F(FormValidationTest, MultipleEmails) {
    auto input = doc.QuerySelector("#multipleEmails");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "email");
    EXPECT_EQ(input->GetAttribute("multiple"), "");
}

// ========== URL 验证测试 ==========

TEST_F(FormValidationTest, URLType) {
    auto input = doc.QuerySelector("#websiteInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "url");
}

// ========== Tel 验证测试 ==========

TEST_F(FormValidationTest, TelType) {
    auto input = doc.QuerySelector("#phoneInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "tel");
}

// ========== Step 验证测试 ==========

TEST_F(FormValidationTest, NumberStep) {
    auto input = doc.QuerySelector("#stepInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("step"), "5");
    EXPECT_EQ(input->GetAttribute("min"), "0");
    EXPECT_EQ(input->GetAttribute("max"), "100");
}

TEST_F(FormValidationTest, RangeStep) {
    auto input = doc.QuerySelector("#stepRange");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("step"), "0.1");
}

// ========== 组合验证测试 ==========

TEST_F(FormValidationTest, CombinedValidation) {
    auto input = doc.QuerySelector("#combinedInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("type"), "email");
    EXPECT_EQ(input->GetAttribute("required"), "");
    EXPECT_EQ(input->GetAttribute("pattern"), "[a-z0-9._%+-]+@[a-z0-9.-]+\\.[a-z]{2,}$");
    EXPECT_EQ(input->GetAttribute("minlength"), "5");
    EXPECT_EQ(input->GetAttribute("maxlength"), "100");
}

// ========== 自定义验证消息测试 ==========

TEST_F(FormValidationTest, CustomValidationMessage) {
    auto input = doc.QuerySelector("#customMessage");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("title"), "This field is required!");
}

// ========== Disabled 和 Readonly 测试 ==========

TEST_F(FormValidationTest, DisabledInput) {
    auto input = doc.QuerySelector("#disabledInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("disabled"), "");
    EXPECT_EQ(input->GetAttribute("required"), "");
}

TEST_F(FormValidationTest, ReadonlyInput) {
    auto input = doc.QuerySelector("#readonlyInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("readonly"), "");
    EXPECT_EQ(input->GetAttribute("value"), "readonly value");
}

// ========== Checkbox 和 Radio 验证测试 ==========

TEST_F(FormValidationTest, RequiredCheckbox) {
    auto checkbox = doc.QuerySelector("#agreeCheckbox");
    ASSERT_NE(checkbox, nullptr);
    EXPECT_EQ(checkbox->GetAttribute("type"), "checkbox");
    EXPECT_EQ(checkbox->GetAttribute("required"), "");
}

TEST_F(FormValidationTest, RequiredRadio) {
    auto radios = doc.QuerySelectorAll("input[name='gender']");
    EXPECT_EQ(radios.size(), 2);
    
    for (auto* radio : radios) {
        EXPECT_EQ(radio->GetAttribute("type"), "radio");
        EXPECT_EQ(radio->GetAttribute("required"), "");
    }
}

// ========== 查询测试 ==========

TEST_F(FormValidationTest, QueryAllRequired) {
    auto required = doc.QuerySelectorAll("[required]");
    EXPECT_GE(required.size(), 10);  // 至少有 10 个 required 元素
}

TEST_F(FormValidationTest, QueryAllPattern) {
    auto pattern = doc.QuerySelectorAll("[pattern]");
    EXPECT_EQ(pattern.size(), 4);  // patternInput, phonePattern, zipPattern, combinedInput
}

TEST_F(FormValidationTest, QueryAllMinMax) {
    auto min = doc.QuerySelectorAll("[min]");
    EXPECT_GE(min.size(), 5);  // 至少有 5 个带 min 属性的元素
    
    auto max = doc.QuerySelectorAll("[max]");
    EXPECT_GE(max.size(), 5);  // 至少有 5 个带 max 属性的元素
}

TEST_F(FormValidationTest, QueryAllLength) {
    auto minlength = doc.QuerySelectorAll("[minlength]");
    EXPECT_EQ(minlength.size(), 4);  // usernameInput, passwordInput, bioTextarea, combinedInput
    
    auto maxlength = doc.QuerySelectorAll("[maxlength]");
    EXPECT_EQ(maxlength.size(), 4);
}

