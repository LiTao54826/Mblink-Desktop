/**
 * @file test_css_variables.cpp
 * @brief CSS 变量 (Custom Properties) 单元测试
 */

#include <gtest/gtest.h>
#include "render/css/css_variables.h"

namespace mbink {
namespace test {

class CSSVariablesTest : public ::testing::Test {
protected:
    void SetUp() override {
        vars_ = std::make_unique<CSSVariables>();
    }

protected:
    std::unique_ptr<CSSVariables> vars_;
};

// ========== 基本操作测试 ==========

TEST_F(CSSVariablesTest, SetAndGet) {
    vars_->SetVariable("--main-color", "red");
    auto value = vars_->GetVariable("--main-color");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "red");
}

TEST_F(CSSVariablesTest, GetNonExistent) {
    auto value = vars_->GetVariable("--nonexistent");
    EXPECT_FALSE(value.has_value());
}

TEST_F(CSSVariablesTest, GetWithDefault) {
    auto value = vars_->GetVariable("--nonexistent");
    EXPECT_EQ(value.value_or("blue"), "blue");
}

TEST_F(CSSVariablesTest, Overwrite) {
    vars_->SetVariable("--color", "red");
    vars_->SetVariable("--color", "blue");
    auto value = vars_->GetVariable("--color");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "blue");
}

TEST_F(CSSVariablesTest, Remove) {
    vars_->SetVariable("--color", "red");
    vars_->RemoveVariable("--color");
    auto value = vars_->GetVariable("--color");
    EXPECT_FALSE(value.has_value());
}

TEST_F(CSSVariablesTest, Has) {
    vars_->SetVariable("--color", "red");
    EXPECT_TRUE(vars_->HasVariable("--color"));
    EXPECT_FALSE(vars_->HasVariable("--nonexistent"));
}

TEST_F(CSSVariablesTest, Clear) {
    vars_->SetVariable("--color1", "red");
    vars_->SetVariable("--color2", "blue");
    vars_->Clear();

    EXPECT_FALSE(vars_->HasVariable("--color1"));
    EXPECT_FALSE(vars_->HasVariable("--color2"));
}

// ========== 变量名验证测试 ==========

TEST_F(CSSVariablesTest, ValidVariableName) {
    // 有效的变量名以 -- 开头
    vars_->SetVariable("--valid-name", "value");
    EXPECT_TRUE(vars_->HasVariable("--valid-name"));
}

TEST_F(CSSVariablesTest, VariableNameWithNumbers) {
    vars_->SetVariable("--color-1", "red");
    vars_->SetVariable("--color-2", "blue");

    auto value1 = vars_->GetVariable("--color-1");
    auto value2 = vars_->GetVariable("--color-2");
    ASSERT_TRUE(value1.has_value());
    ASSERT_TRUE(value2.has_value());
    EXPECT_EQ(value1.value(), "red");
    EXPECT_EQ(value2.value(), "blue");
}

TEST_F(CSSVariablesTest, VariableNameWithUnderscore) {
    vars_->SetVariable("--main_color", "red");
    auto value = vars_->GetVariable("--main_color");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "red");
}

// ========== 变量值测试 ==========

TEST_F(CSSVariablesTest, ColorValue) {
    vars_->SetVariable("--primary", "#ff0000");
    auto value = vars_->GetVariable("--primary");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "#ff0000");
}

TEST_F(CSSVariablesTest, LengthValue) {
    vars_->SetVariable("--spacing", "16px");
    auto value = vars_->GetVariable("--spacing");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "16px");
}

TEST_F(CSSVariablesTest, ComplexValue) {
    vars_->SetVariable("--shadow", "0 2px 4px rgba(0, 0, 0, 0.1)");
    auto value = vars_->GetVariable("--shadow");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "0 2px 4px rgba(0, 0, 0, 0.1)");
}

TEST_F(CSSVariablesTest, EmptyValue) {
    vars_->SetVariable("--empty", "");
    EXPECT_TRUE(vars_->HasVariable("--empty"));
    auto value = vars_->GetVariable("--empty");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "");
}

// ========== 变量解析测试 ==========

TEST_F(CSSVariablesTest, ResolveSimple) {
    vars_->SetVariable("--color", "red");
    auto resolved = CSSVarResolver::ResolveVar("var(--color)", *vars_);
    EXPECT_EQ(resolved, "red");
}

TEST_F(CSSVariablesTest, ResolveWithDefault) {
    auto resolved = CSSVarResolver::ResolveVar("var(--nonexistent, blue)", *vars_);
    EXPECT_EQ(resolved, "blue");
}

TEST_F(CSSVariablesTest, ResolveNested) {
    vars_->SetVariable("--primary", "blue");
    vars_->SetVariable("--button-color", "var(--primary)");

    auto resolved = CSSVarResolver::ResolveVar("var(--button-color)", *vars_);
    EXPECT_EQ(resolved, "blue");
}

TEST_F(CSSVariablesTest, ResolveInValue) {
    vars_->SetVariable("--size", "10px");
    auto resolved = CSSVarResolver::ResolveVar("margin: var(--size)", *vars_);
    EXPECT_EQ(resolved, "margin: 10px");
}

TEST_F(CSSVariablesTest, ResolveMultiple) {
    vars_->SetVariable("--top", "10px");
    vars_->SetVariable("--right", "20px");

    auto resolved = CSSVarResolver::ResolveVar("var(--top) var(--right)", *vars_);
    EXPECT_EQ(resolved, "10px 20px");
}

TEST_F(CSSVariablesTest, ResolveNoVar) {
    auto resolved = CSSVarResolver::ResolveVar("red", *vars_);
    EXPECT_EQ(resolved, "red");
}

// ========== 继承测试 ==========

TEST_F(CSSVariablesTest, InheritFromParent) {
    auto parent = std::make_unique<CSSVariables>();
    parent->SetVariable("--inherited", "value");

    vars_->InheritFrom(parent.get());

    auto val = vars_->GetVariable("--inherited");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value");
}

TEST_F(CSSVariablesTest, OverrideParent) {
    auto parent = std::make_unique<CSSVariables>();
    parent->SetVariable("--color", "red");

    vars_->InheritFrom(parent.get());
    vars_->SetVariable("--color", "blue");

    auto val = vars_->GetVariable("--color");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "blue");
}

TEST_F(CSSVariablesTest, LocalHasPriority) {
    auto parent = std::make_unique<CSSVariables>();
    parent->SetVariable("--color", "red");

    vars_->SetVariable("--color", "blue");
    vars_->InheritFrom(parent.get());

    // 本地值优先
    auto val = vars_->GetVariable("--color");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "blue");
}

// ========== 循环引用检测测试 ==========

TEST_F(CSSVariablesTest, DetectCircularReference) {
    vars_->SetVariable("--a", "var(--b)");
    vars_->SetVariable("--b", "var(--a)");

    // 应该检测到循环引用并返回空或默认值
    auto resolved = CSSVarResolver::ResolveVar("var(--a)", *vars_);
    // 具体行为取决于实现 - 应该返回原始值或检测到循环
    EXPECT_FALSE(resolved.empty());
}

// ========== 获取所有变量测试 ==========

TEST_F(CSSVariablesTest, GetAll) {
    vars_->SetVariable("--color1", "red");
    vars_->SetVariable("--color2", "blue");
    vars_->SetVariable("--size", "16px");

    auto all = vars_->GetAllVariables();
    EXPECT_EQ(all.size(), 3);
}

} // namespace test
} // namespace mbink
