/**
 * @file test_cascade_engine.cpp
 * @brief CascadeEngine 类的单元测试
 */

#include <gtest/gtest.h>
#include "core/lexbor/cascade_engine.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "core/lexbor/lexbor_document.h"
#include "core/dom/document.h"
#include "core/dom/element.h"

using namespace lightui;

// ========== 优先级计算测试 ==========

TEST(CascadeEngineTest, CalculateSpecificityElement) {
    CascadeEngine engine;
    
    auto spec = engine.CalculateSpecificity("div");
    EXPECT_EQ(spec.inline_style, 0);
    EXPECT_EQ(spec.id_count, 0);
    EXPECT_EQ(spec.class_count, 0);
    EXPECT_GE(spec.element_count, 1);
}

TEST(CascadeEngineTest, CalculateSpecificityClass) {
    CascadeEngine engine;
    
    auto spec = engine.CalculateSpecificity(".container");
    EXPECT_EQ(spec.inline_style, 0);
    EXPECT_EQ(spec.id_count, 0);
    EXPECT_EQ(spec.class_count, 1);
}

TEST(CascadeEngineTest, CalculateSpecificityID) {
    CascadeEngine engine;
    
    auto spec = engine.CalculateSpecificity("#header");
    EXPECT_EQ(spec.inline_style, 0);
    EXPECT_EQ(spec.id_count, 1);
    EXPECT_EQ(spec.class_count, 0);
}

TEST(CascadeEngineTest, CalculateSpecificityComplex) {
    CascadeEngine engine;
    
    auto spec = engine.CalculateSpecificity("div.container#main");
    EXPECT_EQ(spec.inline_style, 0);
    EXPECT_EQ(spec.id_count, 1);
    EXPECT_EQ(spec.class_count, 1);
    EXPECT_GE(spec.element_count, 1);
}

TEST(CascadeEngineTest, SpecificityComparison) {
    Specificity spec1(0, 1, 0, 0); // #id
    Specificity spec2(0, 0, 1, 0); // .class
    Specificity spec3(0, 0, 0, 1); // element
    
    EXPECT_GT(spec1.Compare(spec2), 0); // ID > class
    EXPECT_GT(spec2.Compare(spec3), 0); // class > element
    EXPECT_GT(spec1.Compare(spec3), 0); // ID > element
}

TEST(CascadeEngineTest, SpecificityInlineStyle) {
    Specificity inline_spec(1, 0, 0, 0);
    Specificity id_spec(0, 1, 0, 0);

    EXPECT_GT(inline_spec.Compare(id_spec), 0); // inline > ID
}

// ========== 级联规则测试 ==========

TEST(CascadeEngineTest, ApplyCascadeSimple) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    // 创建规则
    std::vector<const CSSRule*> rules;
    CSSRule rule1;
    rule1.selector = "div";
    rule1.declarations["color"] = "red";
    rules.push_back(&rule1);
    
    auto style = engine.ApplyCascade(div.get(), rules);
    
    EXPECT_EQ(style["color"], "red");
}

TEST(CascadeEngineTest, ApplyCascadeMultipleRules) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::vector<const CSSRule*> rules;
    
    CSSRule rule1;
    rule1.selector = "div";
    rule1.declarations["color"] = "red";
    rule1.declarations["font-size"] = "14px";
    rules.push_back(&rule1);
    
    CSSRule rule2;
    rule2.selector = ".container";
    rule2.declarations["color"] = "blue"; // 应该被覆盖
    rules.push_back(&rule2);
    
    auto style = engine.ApplyCascade(div.get(), rules);
    
    EXPECT_EQ(style["font-size"], "14px");
}

TEST(CascadeEngineTest, ApplyCascadeWithInlineStyle) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("style", "color: green;");
    
    std::vector<const CSSRule*> rules;
    CSSRule rule1;
    rule1.selector = "div";
    rule1.declarations["color"] = "red";
    rules.push_back(&rule1);
    
    auto style = engine.ApplyCascade(div.get(), rules);
    
    EXPECT_EQ(style["color"], "green"); // 内联样式优先
}

TEST(CascadeEngineTest, ApplyCascadeSpecificityOrder) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    div->SetAttribute("id", "main");
    div->SetAttribute("class", "container");
    
    std::vector<const CSSRule*> rules;
    
    CSSRule rule1;
    rule1.selector = "div";
    rule1.declarations["color"] = "red";
    rules.push_back(&rule1);
    
    CSSRule rule2;
    rule2.selector = ".container";
    rule2.declarations["color"] = "blue";
    rules.push_back(&rule2);
    
    CSSRule rule3;
    rule3.selector = "#main";
    rule3.declarations["color"] = "green";
    rules.push_back(&rule3);
    
    auto style = engine.ApplyCascade(div.get(), rules);
    
    EXPECT_EQ(style["color"], "green"); // ID选择器优先级最高
}

// ========== 继承测试 ==========

TEST(CascadeEngineTest, IsInheritableProperty) {
    CascadeEngine engine;
    
    EXPECT_TRUE(engine.IsInheritableProperty("color"));
    EXPECT_TRUE(engine.IsInheritableProperty("font-size"));
    EXPECT_FALSE(engine.IsInheritableProperty("background-color"));
    EXPECT_FALSE(engine.IsInheritableProperty("width"));
}

TEST(CascadeEngineTest, GetInitialValue) {
    CascadeEngine engine;
    
    EXPECT_EQ(engine.GetInitialValue("color"), "black");
    EXPECT_EQ(engine.GetInitialValue("font-size"), "16px");
    EXPECT_EQ(engine.GetInitialValue("display"), "inline");
}

TEST(CascadeEngineTest, ApplyInheritanceWithInitialValues) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::map<std::string, std::string> base_style;
    base_style["color"] = "red";
    
    auto style = engine.ApplyInheritance(div.get(), base_style);
    
    EXPECT_EQ(style["color"], "red");
    // 应该有初始值
    EXPECT_FALSE(style["font-size"].empty());
}

TEST(CascadeEngineTest, ApplyInheritanceFromParent) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto parent = std::make_shared<Element>("div");
    doc->AppendChild(parent);
    parent->SetAttribute("style", "color: blue;");
    
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    std::map<std::string, std::string> base_style;
    // child没有设置color
    
    auto style = engine.ApplyInheritance(child.get(), base_style);
    
    // 应该从父元素继承color
    EXPECT_EQ(style["color"], "blue");
}

// ========== 完整样式计算测试 ==========

TEST(CascadeEngineTest, ComputeStyleSimple) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::vector<const CSSRule*> rules;
    CSSRule rule1;
    rule1.selector = "div";
    rule1.declarations["color"] = "red";
    rule1.declarations["font-size"] = "14px";
    rules.push_back(&rule1);
    
    auto style = engine.ComputeStyle(div.get(), rules);
    
    EXPECT_EQ(style["color"], "red");
    EXPECT_EQ(style["font-size"], "14px");
}

TEST(CascadeEngineTest, ComputeStyleWithInheritance) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto parent = std::make_shared<Element>("div");
    doc->AppendChild(parent);
    parent->SetAttribute("style", "color: blue;");
    
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    std::vector<const CSSRule*> rules;
    // 没有规则匹配child
    
    auto style = engine.ComputeStyle(child.get(), rules);
    
    // 应该继承父元素的color
    EXPECT_EQ(style["color"], "blue");
}

TEST(CascadeEngineTest, ComputeStyleComplex) {
    auto doc = std::make_shared<Document>();
    CascadeEngine engine;
    
    auto parent = std::make_shared<Element>("div");
    doc->AppendChild(parent);
    parent->SetAttribute("style", "color: blue;");
    
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    child->SetAttribute("class", "highlight");
    child->SetAttribute("style", "font-weight: bold;");
    
    std::vector<const CSSRule*> rules;
    
    CSSRule rule1;
    rule1.selector = ".highlight";
    rule1.declarations["background-color"] = "yellow";
    rules.push_back(&rule1);
    
    auto style = engine.ComputeStyle(child.get(), rules);
    
    EXPECT_EQ(style["color"], "blue"); // 继承
    EXPECT_EQ(style["font-weight"], "bold"); // 内联样式
    EXPECT_EQ(style["background-color"], "yellow"); // CSS规则
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

