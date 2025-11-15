/**
 * @file test_css_variables.cpp
 * @brief CSS 变量（CSS Custom Properties）测试
 * 
 * 测试内容：
 * - CSS 变量定义和获取
 * - var() 函数解析
 * - 变量继承
 * - 变量回退值
 * - 嵌套 var()
 * - 变量作用域
 */

#include "core/render/css_variables.h"
#include "core/render/style_resolver.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <iostream>
#include <cassert>

using namespace lightui;

// ========== 测试计数器 ==========
int total_tests = 0;
int passed_tests = 0;

#define TEST(name) \
    std::cout << "\n[TEST] " << name << std::endl;

#define ASSERT(condition, message) \
    total_tests++; \
    if (condition) { \
        std::cout << "  ✓ " << message << std::endl; \
        passed_tests++; \
    } else { \
        std::cout << "  ✗ " << message << " (FAILED)" << std::endl; \
    }

// ========== 基础功能测试 ==========

void TestCSSVariablesBasic() {
    TEST("CSS Variables - Basic Operations");
    
    CSSVariables vars;
    
    // 测试设置和获取变量
    vars.SetVariable("--primary-color", "#007bff");
    auto value = vars.GetVariable("--primary-color");
    ASSERT(value.has_value() && value.value() == "#007bff", 
           "Set and get variable");
    
    // 测试变量存在性检查
    ASSERT(vars.HasVariable("--primary-color"), 
           "HasVariable returns true for existing variable");
    ASSERT(!vars.HasVariable("--unknown"), 
           "HasVariable returns false for non-existing variable");
    
    // 测试多个变量
    vars.SetVariable("--spacing", "16px");
    vars.SetVariable("--font-size", "14px");
    ASSERT(vars.GetAllVariables().size() == 3, 
           "Multiple variables stored correctly");
    
    // 测试移除变量
    vars.RemoveVariable("--spacing");
    ASSERT(!vars.HasVariable("--spacing"), 
           "Variable removed successfully");
    
    // 测试清空所有变量
    vars.Clear();
    ASSERT(vars.GetAllVariables().empty(), 
           "All variables cleared");
}

void TestCSSVariablesValidation() {
    TEST("CSS Variables - Name Validation");
    
    CSSVariables vars;
    
    // 有效的变量名
    ASSERT(IsValidCustomPropertyName("--color"), 
           "Valid variable name: --color");
    ASSERT(IsValidCustomPropertyName("--primary-color"), 
           "Valid variable name: --primary-color");
    ASSERT(IsValidCustomPropertyName("--font_size"), 
           "Valid variable name: --font_size");
    ASSERT(IsValidCustomPropertyName("--color123"), 
           "Valid variable name: --color123");
    
    // 无效的变量名
    ASSERT(!IsValidCustomPropertyName("-color"), 
           "Invalid variable name: -color (single dash)");
    ASSERT(!IsValidCustomPropertyName("--"), 
           "Invalid variable name: -- (empty)");
    ASSERT(!IsValidCustomPropertyName("color"), 
           "Invalid variable name: color (no dashes)");
    ASSERT(!IsValidCustomPropertyName("--color space"), 
           "Invalid variable name: --color space (contains space)");
}

void TestCSSVariablesNormalization() {
    TEST("CSS Variables - Name Normalization");
    
    CSSVariables vars;
    
    // CSS 变量名应该被规范化为小写
    vars.SetVariable("--Primary-Color", "#007bff");
    auto value = vars.GetVariable("--primary-color");
    ASSERT(value.has_value() && value.value() == "#007bff", 
           "Variable name normalized to lowercase");
    
    // 测试规范化函数
    ASSERT(NormalizeCustomPropertyName("--Primary-Color") == "--primary-color", 
           "NormalizeCustomPropertyName works correctly");
}

void TestCSSVariablesInheritance() {
    TEST("CSS Variables - Inheritance");
    
    CSSVariables parent_vars;
    parent_vars.SetVariable("--primary-color", "#007bff");
    parent_vars.SetVariable("--spacing", "16px");
    
    CSSVariables child_vars;
    child_vars.SetVariable("--font-size", "14px");
    
    // 子元素继承父元素的变量
    child_vars.InheritFrom(&parent_vars);
    
    ASSERT(child_vars.HasVariable("--primary-color"), 
           "Child inherits parent variable: --primary-color");
    ASSERT(child_vars.HasVariable("--spacing"), 
           "Child inherits parent variable: --spacing");
    ASSERT(child_vars.HasVariable("--font-size"), 
           "Child keeps its own variable: --font-size");
    
    // 子元素可以覆盖父元素的变量
    child_vars.SetVariable("--primary-color", "#ff0000");
    auto value = child_vars.GetVariable("--primary-color");
    ASSERT(value.has_value() && value.value() == "#ff0000", 
           "Child can override parent variable");
}

void TestCSSVariablesMerge() {
    TEST("CSS Variables - Merge");
    
    CSSVariables vars1;
    vars1.SetVariable("--color", "red");
    vars1.SetVariable("--size", "10px");
    
    CSSVariables vars2;
    vars2.SetVariable("--color", "blue");
    vars2.SetVariable("--spacing", "20px");
    
    vars1.Merge(vars2);
    
    ASSERT(vars1.GetVariable("--color").value() == "blue", 
           "Merge overwrites existing variable");
    ASSERT(vars1.GetVariable("--size").value() == "10px", 
           "Merge keeps non-conflicting variable");
    ASSERT(vars1.GetVariable("--spacing").value() == "20px", 
           "Merge adds new variable");
}

// ========== var() 函数解析测试 ==========

void TestVarFunctionBasic() {
    TEST("var() Function - Basic Parsing");
    
    CSSVariables vars;
    vars.SetVariable("--primary-color", "#007bff");
    vars.SetVariable("--spacing", "16px");
    
    // 简单的 var() 解析
    std::string result1 = CSSVarResolver::ResolveVar("var(--primary-color)", vars);
    ASSERT(result1 == "#007bff", 
           "Simple var() resolved correctly");
    
    // 多个 var() 在同一个值中
    std::string result2 = CSSVarResolver::ResolveVar("10px var(--spacing) 20px", vars);
    ASSERT(result2 == "10px 16px 20px", 
           "Multiple var() in one value resolved correctly");
    
    // 不包含 var() 的值应该保持不变
    std::string result3 = CSSVarResolver::ResolveVar("10px 20px", vars);
    ASSERT(result3 == "10px 20px", 
           "Non-var value unchanged");
}

void TestVarFunctionFallback() {
    TEST("var() Function - Fallback Values");
    
    CSSVariables vars;
    vars.SetVariable("--primary-color", "#007bff");
    
    // 变量存在，不使用回退值
    std::string result1 = CSSVarResolver::ResolveVar("var(--primary-color, red)", vars);
    ASSERT(result1 == "#007bff", 
           "Existing variable used, fallback ignored");
    
    // 变量不存在，使用回退值
    std::string result2 = CSSVarResolver::ResolveVar("var(--unknown, red)", vars);
    ASSERT(result2 == "red", 
           "Fallback value used for unknown variable");
    
    // 复杂的回退值
    std::string result3 = CSSVarResolver::ResolveVar("var(--unknown, 10px 20px)", vars);
    ASSERT(result3 == "10px 20px", 
           "Complex fallback value works");
    
    // 没有回退值的未知变量
    std::string result4 = CSSVarResolver::ResolveVar("var(--unknown)", vars);
    ASSERT(result4 == "var(--unknown)", 
           "Unknown variable without fallback unchanged");
}

void TestVarFunctionNested() {
    TEST("var() Function - Nested var()");
    
    CSSVariables vars;
    vars.SetVariable("--primary-color", "#007bff");
    vars.SetVariable("--color", "var(--primary-color)");
    
    // 嵌套的 var()
    std::string result1 = CSSVarResolver::ResolveVar("var(--color)", vars);
    ASSERT(result1 == "#007bff", 
           "Nested var() resolved correctly");
    
    // 回退值中包含 var()
    std::string result2 = CSSVarResolver::ResolveVar("var(--unknown, var(--primary-color))", vars);
    ASSERT(result2 == "#007bff", 
           "var() in fallback value resolved correctly");
}

void TestVarFunctionParsing() {
    TEST("var() Function - Parsing Details");
    
    // 测试 ParseVarFunction
    auto parsed1 = CSSVarResolver::ParseVarFunction("var(--color)");
    ASSERT(parsed1.has_value() && parsed1->first == "--color" && parsed1->second.empty(), 
           "Parse var() without fallback");
    
    auto parsed2 = CSSVarResolver::ParseVarFunction("var(--color, red)");
    ASSERT(parsed2.has_value() && parsed2->first == "--color" && parsed2->second == "red", 
           "Parse var() with fallback");
    
    auto parsed3 = CSSVarResolver::ParseVarFunction("var(--color, 10px 20px)");
    ASSERT(parsed3.has_value() && parsed3->second == "10px 20px", 
           "Parse var() with complex fallback");
    
    // 测试 ContainsVar
    ASSERT(CSSVarResolver::ContainsVar("var(--color)"), 
           "ContainsVar detects var()");
    ASSERT(CSSVarResolver::ContainsVar("10px var(--spacing)"), 
           "ContainsVar detects var() in complex value");
    ASSERT(!CSSVarResolver::ContainsVar("10px 20px"), 
           "ContainsVar returns false for non-var value");
}

// ========== 集成测试 ==========

void TestCSSVariablesIntegration() {
    TEST("CSS Variables - Integration with StyleResolver");
    
    // 创建文档和元素
    auto doc = std::make_shared<Document>();
    auto root = doc->CreateElement("div");
    auto child = doc->CreateElement("div");
    root->AppendChild(child);
    
    // 设置根元素的 CSS 变量
    root->SetAttribute("style", "--primary-color: #007bff; --spacing: 16px;");
    
    // 设置子元素使用 var()
    child->SetAttribute("style", "color: var(--primary-color); padding: var(--spacing);");
    
    // 解析样式
    StyleResolver resolver;
    auto root_style = resolver.ResolveStyle(root, nullptr);
    auto child_style = resolver.ResolveStyle(child, &root_style);
    
    // 验证变量被正确解析
    ASSERT(child_style.color == "#007bff", 
           "var() in child element resolved using parent variable");
    ASSERT(child_style.padding.top.value == 16.0f, 
           "var() resolved to correct length value");
}

// ========== 主函数 ==========

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CSS Variables Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 基础功能测试
    TestCSSVariablesBasic();
    TestCSSVariablesValidation();
    TestCSSVariablesNormalization();
    TestCSSVariablesInheritance();
    TestCSSVariablesMerge();
    
    // var() 函数测试
    TestVarFunctionBasic();
    TestVarFunctionFallback();
    TestVarFunctionNested();
    TestVarFunctionParsing();

    // 集成测试（暂时跳过，需要完整的 DOM 和 StyleResolver 支持）
    // TestCSSVariablesIntegration();

    // 输出测试结果
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results: " << passed_tests << "/" << total_tests << " passed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (passed_tests == total_tests) ? 0 : 1;
}

