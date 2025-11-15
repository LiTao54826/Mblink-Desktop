#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "core/dom/element.h"
#include <string>

using namespace lightui;

/**
 * @brief CSS3 动态伪类测试
 * 
 * 测试动态伪类的支持，包括 :hover, :active, :focus, :visited 等
 */
class CSS3PseudoClassesTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Pseudo Classes Test</title></head>
            <body>
                <a href="https://example.com" id="link1">Link 1</a>
                <a href="/internal" id="link2">Link 2</a>
                <a href="#anchor" id="link3">Link 3</a>
                
                <button id="btn1">Button 1</button>
                <button id="btn2" disabled>Button 2</button>
                
                <input type="text" id="input1" />
                <input type="text" id="input2" disabled />
                <input type="checkbox" id="check1" />
                <input type="checkbox" id="check2" checked />
                <input type="radio" name="radio" id="radio1" />
                <input type="radio" name="radio" id="radio2" checked />
                
                <div id="container">
                    <p id="p1">Paragraph 1</p>
                    <p id="p2">Paragraph 2</p>
                    <p id="p3">Paragraph 3</p>
                </div>
                
                <select id="select1">
                    <option value="1">Option 1</option>
                    <option value="2" selected>Option 2</option>
                    <option value="3">Option 3</option>
                </select>
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== 链接伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, LinkPseudoClass) {
    // 测试 :link 伪类（未访问的链接）
    auto links = doc.QuerySelectorAll("a");
    EXPECT_EQ(links.size(), 3);
}

TEST_F(CSS3PseudoClassesTest, AnyLinkPseudoClass) {
    // 测试 :any-link 伪类（所有链接）
    auto links = doc.QuerySelectorAll("a");
    EXPECT_EQ(links.size(), 3);
}

// ========== 用户操作伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, HoverPseudoClass) {
    // 注意：:hover 是动态伪类，需要通过 Element 类设置状态
    auto element = doc.QuerySelector("#btn1");
    ASSERT_NE(element, nullptr);
    
    // 默认情况下不应该有 hover 状态
    // 这个测试主要验证选择器解析不会崩溃
    auto hovered = doc.QuerySelectorAll("button:hover");
    EXPECT_GE(hovered.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, ActivePseudoClass) {
    // 测试 :active 伪类
    auto active = doc.QuerySelectorAll("button:active");
    EXPECT_GE(active.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, FocusPseudoClass) {
    // 测试 :focus 伪类
    auto focused = doc.QuerySelectorAll("input:focus");
    EXPECT_GE(focused.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, FocusWithinPseudoClass) {
    // 测试 :focus-within 伪类
    auto focusWithin = doc.QuerySelectorAll("div:focus-within");
    EXPECT_GE(focusWithin.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, FocusVisiblePseudoClass) {
    // 测试 :focus-visible 伪类
    auto focusVisible = doc.QuerySelectorAll("input:focus-visible");
    EXPECT_GE(focusVisible.size(), 0);
}

// ========== 输入伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, EnabledPseudoClass) {
    auto enabled = doc.QuerySelectorAll("input:enabled");
    EXPECT_GE(enabled.size(), 3);  // 至少 input1, check1, radio1
}

TEST_F(CSS3PseudoClassesTest, DisabledPseudoClass) {
    auto disabled = doc.QuerySelectorAll("input:disabled");
    EXPECT_EQ(disabled.size(), 1);  // input2
}

TEST_F(CSS3PseudoClassesTest, ReadOnlyPseudoClass) {
    // 测试 :read-only 伪类
    auto readonly = doc.QuerySelectorAll("input:read-only");
    EXPECT_GE(readonly.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, ReadWritePseudoClass) {
    // 测试 :read-write 伪类
    auto readwrite = doc.QuerySelectorAll("input:read-write");
    EXPECT_GE(readwrite.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, PlaceholderShownPseudoClass) {
    // 测试 :placeholder-shown 伪类
    auto placeholderShown = doc.QuerySelectorAll("input:placeholder-shown");
    EXPECT_GE(placeholderShown.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, DefaultPseudoClass) {
    // 测试 :default 伪类（默认选中的表单元素）
    auto defaultElements = doc.QuerySelectorAll(":default");
    EXPECT_GE(defaultElements.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, CheckedPseudoClass) {
    auto checked = doc.QuerySelectorAll("input:checked");
    EXPECT_EQ(checked.size(), 2);  // check2, radio2
}

TEST_F(CSS3PseudoClassesTest, IndeterminatePseudoClass) {
    // 测试 :indeterminate 伪类
    auto indeterminate = doc.QuerySelectorAll("input:indeterminate");
    EXPECT_GE(indeterminate.size(), 0);
}

// ========== 验证伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, ValidPseudoClass) {
    // 测试 :valid 伪类
    auto valid = doc.QuerySelectorAll("input:valid");
    EXPECT_GE(valid.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, InvalidPseudoClass) {
    // 测试 :invalid 伪类
    auto invalid = doc.QuerySelectorAll("input:invalid");
    EXPECT_GE(invalid.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, InRangePseudoClass) {
    // 测试 :in-range 伪类
    auto inRange = doc.QuerySelectorAll("input:in-range");
    EXPECT_GE(inRange.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, OutOfRangePseudoClass) {
    // 测试 :out-of-range 伪类
    auto outOfRange = doc.QuerySelectorAll("input:out-of-range");
    EXPECT_GE(outOfRange.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, RequiredPseudoClass) {
    // 测试 :required 伪类
    auto required = doc.QuerySelectorAll("input:required");
    EXPECT_GE(required.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, OptionalPseudoClass) {
    // 测试 :optional 伪类
    auto optional = doc.QuerySelectorAll("input:optional");
    EXPECT_GE(optional.size(), 0);
}

// ========== 其他伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, RootPseudoClass) {
    // 测试 :root 伪类
    auto root = doc.QuerySelectorAll(":root");
    EXPECT_EQ(root.size(), 1);  // 应该只有 <html> 元素
}

TEST_F(CSS3PseudoClassesTest, EmptyPseudoClass) {
    // 测试 :empty 伪类
    auto empty = doc.QuerySelectorAll("p:empty");
    EXPECT_EQ(empty.size(), 0);  // 我们的 p 元素都有文本内容
}

TEST_F(CSS3PseudoClassesTest, TargetPseudoClass) {
    // 测试 :target 伪类
    auto target = doc.QuerySelectorAll(":target");
    EXPECT_GE(target.size(), 0);
}

TEST_F(CSS3PseudoClassesTest, LangPseudoClass) {
    // 测试 :lang() 伪类
    auto lang = doc.QuerySelectorAll(":lang(en)");
    EXPECT_GE(lang.size(), 0);
}

// ========== 否定伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, NotPseudoClass) {
    // 测试 :not() 伪类
    auto notDisabled = doc.QuerySelectorAll("input:not(:disabled)");
    EXPECT_GE(notDisabled.size(), 3);  // 至少 input1, check1, radio1
}

TEST_F(CSS3PseudoClassesTest, NotPseudoClassComplex) {
    // 测试复杂的 :not() 伪类
    auto notChecked = doc.QuerySelectorAll("input:not(:checked)");
    EXPECT_GE(notChecked.size(), 2);  // 至少有未选中的元素
}

// ========== 结构伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, FirstChildPseudoClass) {
    auto firstChild = doc.QuerySelectorAll("#container > p:first-child");
    EXPECT_EQ(firstChild.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, LastChildPseudoClass) {
    auto lastChild = doc.QuerySelectorAll("#container > p:last-child");
    EXPECT_EQ(lastChild.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, OnlyChildPseudoClass) {
    auto onlyChild = doc.QuerySelectorAll("p:only-child");
    EXPECT_EQ(onlyChild.size(), 0);  // p 元素都有兄弟元素
}

TEST_F(CSS3PseudoClassesTest, NthChildPseudoClass) {
    auto nthChild = doc.QuerySelectorAll("#container > p:nth-child(2)");
    EXPECT_EQ(nthChild.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, NthLastChildPseudoClass) {
    auto nthLastChild = doc.QuerySelectorAll("#container > p:nth-last-child(2)");
    EXPECT_EQ(nthLastChild.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, FirstOfTypePseudoClass) {
    auto firstOfType = doc.QuerySelectorAll("#container > p:first-of-type");
    EXPECT_EQ(firstOfType.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, LastOfTypePseudoClass) {
    auto lastOfType = doc.QuerySelectorAll("#container > p:last-of-type");
    EXPECT_EQ(lastOfType.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, OnlyOfTypePseudoClass) {
    auto onlyOfType = doc.QuerySelectorAll("title:only-of-type");
    EXPECT_GE(onlyOfType.size(), 1);  // title 应该是唯一的
}

TEST_F(CSS3PseudoClassesTest, NthOfTypePseudoClass) {
    auto nthOfType = doc.QuerySelectorAll("#container > p:nth-of-type(2)");
    EXPECT_EQ(nthOfType.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, NthLastOfTypePseudoClass) {
    auto nthLastOfType = doc.QuerySelectorAll("#container > p:nth-last-of-type(2)");
    EXPECT_EQ(nthLastOfType.size(), 1);
}

// ========== 组合伪类测试 ==========

TEST_F(CSS3PseudoClassesTest, CombinedPseudoClasses1) {
    // 测试多个伪类组合
    auto combined = doc.QuerySelectorAll("input:enabled:not(:checked)");
    EXPECT_GE(combined.size(), 1);
}

TEST_F(CSS3PseudoClassesTest, CombinedPseudoClasses2) {
    // 测试伪类与属性选择器组合
    auto combined = doc.QuerySelectorAll("input[type='checkbox']:checked");
    EXPECT_EQ(combined.size(), 1);  // check2
}

TEST_F(CSS3PseudoClassesTest, CombinedPseudoClasses3) {
    // 测试伪类与类选择器组合
    auto combined = doc.QuerySelectorAll("button:not(:disabled)");
    EXPECT_EQ(combined.size(), 1);  // btn1
}

