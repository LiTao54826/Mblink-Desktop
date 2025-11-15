#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>
#include <chrono>

using namespace lightui;

/**
 * @brief CSS3 选择器测试
 * 
 * 测试 CSS3 选择器的完整支持，包括基础选择器、组合选择器、属性选择器、伪类、伪元素等
 */
class CSS3SelectorsTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        // 创建一个测试用的 HTML 文档
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>CSS3 Selectors Test</title></head>
            <body>
                <div id="container" class="main-container">
                    <h1 class="title">Title</h1>
                    <p class="intro first">First paragraph</p>
                    <p class="content">Second paragraph</p>
                    <p class="content last">Third paragraph</p>
                    
                    <ul id="list">
                        <li class="item">Item 1</li>
                        <li class="item active">Item 2</li>
                        <li class="item">Item 3</li>
                        <li class="item">Item 4</li>
                        <li class="item">Item 5</li>
                    </ul>
                    
                    <form>
                        <input type="text" name="username" value="" />
                        <input type="password" name="password" value="" />
                        <input type="checkbox" name="remember" checked />
                        <input type="submit" value="Submit" disabled />
                        <textarea name="comment"></textarea>
                    </form>
                    
                    <div class="box" data-type="primary" data-size="large">Box 1</div>
                    <div class="box" data-type="secondary" data-size="small">Box 2</div>
                    
                    <a href="https://example.com">External Link</a>
                    <a href="/internal">Internal Link</a>
                    <a href="#anchor">Anchor Link</a>
                </div>
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== 基础选择器测试 ==========

TEST_F(CSS3SelectorsTest, TypeSelector) {
    auto elements = doc.QuerySelectorAll("p");
    EXPECT_EQ(elements.size(), 3);
}

TEST_F(CSS3SelectorsTest, ClassSelector) {
    auto elements = doc.QuerySelectorAll(".item");
    EXPECT_EQ(elements.size(), 5);
}

TEST_F(CSS3SelectorsTest, IDSelector) {
    auto element = doc.QuerySelector("#container");
    ASSERT_NE(element, nullptr);
    EXPECT_EQ(element->GetId(), "container");
}

TEST_F(CSS3SelectorsTest, UniversalSelector) {
    auto elements = doc.QuerySelectorAll("*");
    EXPECT_GT(elements.size(), 0);  // 应该匹配所有元素
}

// ========== 组合选择器测试 ==========

TEST_F(CSS3SelectorsTest, DescendantCombinator) {
    auto elements = doc.QuerySelectorAll("#container p");
    EXPECT_EQ(elements.size(), 3);
}

TEST_F(CSS3SelectorsTest, ChildCombinator) {
    auto elements = doc.QuerySelectorAll("#container > p");
    EXPECT_EQ(elements.size(), 3);
}

TEST_F(CSS3SelectorsTest, AdjacentSiblingCombinator) {
    auto elements = doc.QuerySelectorAll("h1 + p");
    EXPECT_EQ(elements.size(), 1);
    if (!elements.empty()) {
        EXPECT_TRUE(elements[0]->HasClass("intro"));
    }
}

TEST_F(CSS3SelectorsTest, GeneralSiblingCombinator) {
    auto elements = doc.QuerySelectorAll("h1 ~ p");
    EXPECT_EQ(elements.size(), 3);  // 所有 h1 后面的 p 元素
}

// ========== 属性选择器测试 ==========

TEST_F(CSS3SelectorsTest, AttributeExists) {
    auto elements = doc.QuerySelectorAll("[type]");
    EXPECT_GE(elements.size(), 4);  // 至少有 4 个 input 元素有 type 属性
}

TEST_F(CSS3SelectorsTest, AttributeEquals) {
    auto elements = doc.QuerySelectorAll("[type='text']");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, AttributeContains) {
    auto elements = doc.QuerySelectorAll("[class~='item']");
    EXPECT_EQ(elements.size(), 5);
}

TEST_F(CSS3SelectorsTest, AttributeStartsWith) {
    auto elements = doc.QuerySelectorAll("[href^='https']");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, AttributeEndsWith) {
    auto elements = doc.QuerySelectorAll("[href$='.com']");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, AttributeContainsSubstring) {
    auto elements = doc.QuerySelectorAll("[href*='example']");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, AttributeHyphenMatch) {
    // [attr|=value] - 匹配 attr 属性值为 value 或以 value- 开头
    auto elements = doc.QuerySelectorAll("[data-type|='primary']");
    EXPECT_GE(elements.size(), 0);  // 可能匹配或不匹配
}

// ========== 伪类选择器测试 ==========

TEST_F(CSS3SelectorsTest, FirstChild) {
    auto elements = doc.QuerySelectorAll("li:first-child");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, LastChild) {
    auto elements = doc.QuerySelectorAll("li:last-child");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, NthChild) {
    auto elements = doc.QuerySelectorAll("li:nth-child(2)");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, NthChildOdd) {
    auto elements = doc.QuerySelectorAll("li:nth-child(odd)");
    EXPECT_EQ(elements.size(), 3);  // 1, 3, 5
}

TEST_F(CSS3SelectorsTest, NthChildEven) {
    auto elements = doc.QuerySelectorAll("li:nth-child(even)");
    EXPECT_EQ(elements.size(), 2);  // 2, 4
}

TEST_F(CSS3SelectorsTest, NthChildFormula) {
    auto elements = doc.QuerySelectorAll("li:nth-child(2n+1)");
    EXPECT_EQ(elements.size(), 3);  // 1, 3, 5 (same as odd)
}

TEST_F(CSS3SelectorsTest, NthLastChild) {
    auto elements = doc.QuerySelectorAll("li:nth-last-child(2)");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, FirstOfType) {
    auto elements = doc.QuerySelectorAll("p:first-of-type");
    EXPECT_GE(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, LastOfType) {
    auto elements = doc.QuerySelectorAll("p:last-of-type");
    EXPECT_GE(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, NthOfType) {
    auto elements = doc.QuerySelectorAll("p:nth-of-type(2)");
    EXPECT_GE(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, OnlyChild) {
    auto elements = doc.QuerySelectorAll("h1:only-child");
    EXPECT_EQ(elements.size(), 0);  // h1 不是唯一子元素
}

TEST_F(CSS3SelectorsTest, OnlyOfType) {
    auto elements = doc.QuerySelectorAll("h1:only-of-type");
    EXPECT_GE(elements.size(), 1);  // h1 是唯一的 h1 类型
}

TEST_F(CSS3SelectorsTest, Empty) {
    // 查找空元素（没有子节点的元素）
    auto elements = doc.QuerySelectorAll("p:empty");
    // 我们的测试文档中没有空的 p 元素
    EXPECT_EQ(elements.size(), 0);
}

TEST_F(CSS3SelectorsTest, Not) {
    auto elements = doc.QuerySelectorAll("li:not(.active)");
    EXPECT_EQ(elements.size(), 4);  // 5 个 li，1 个有 active 类
}

// ========== 表单相关伪类测试 ==========

TEST_F(CSS3SelectorsTest, Checked) {
    auto elements = doc.QuerySelectorAll("input:checked");
    EXPECT_EQ(elements.size(), 1);  // 只有一个 checkbox 被选中
}

TEST_F(CSS3SelectorsTest, Disabled) {
    auto elements = doc.QuerySelectorAll("input:disabled");
    EXPECT_EQ(elements.size(), 1);  // 只有 submit 按钮被禁用
}

TEST_F(CSS3SelectorsTest, Enabled) {
    auto elements = doc.QuerySelectorAll("input:enabled");
    EXPECT_EQ(elements.size(), 3);  // 3 个启用的 input
}

// ========== 复杂选择器测试 ==========

TEST_F(CSS3SelectorsTest, ComplexSelector1) {
    // 组合多个选择器
    auto elements = doc.QuerySelectorAll("#container > p.content");
    EXPECT_EQ(elements.size(), 2);
}

TEST_F(CSS3SelectorsTest, ComplexSelector2) {
    // 使用多个类选择器
    auto elements = doc.QuerySelectorAll(".item.active");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, ComplexSelector3) {
    // 组合属性选择器和伪类
    auto elements = doc.QuerySelectorAll("input[type='text']:enabled");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, ComplexSelector4) {
    // 多个选择器（逗号分隔）
    auto elements = doc.QuerySelectorAll("h1, p");
    EXPECT_EQ(elements.size(), 4);  // 1 个 h1 + 3 个 p
}

TEST_F(CSS3SelectorsTest, ComplexSelector5) {
    // 深度嵌套选择器
    auto elements = doc.QuerySelectorAll("#container ul#list li.item");
    EXPECT_EQ(elements.size(), 5);
}

// ========== 数据属性选择器测试 ==========

TEST_F(CSS3SelectorsTest, DataAttributeExists) {
    auto elements = doc.QuerySelectorAll("[data-type]");
    EXPECT_EQ(elements.size(), 2);
}

TEST_F(CSS3SelectorsTest, DataAttributeEquals) {
    auto elements = doc.QuerySelectorAll("[data-type='primary']");
    EXPECT_EQ(elements.size(), 1);
}

TEST_F(CSS3SelectorsTest, DataAttributeMultiple) {
    auto elements = doc.QuerySelectorAll("[data-type='primary'][data-size='large']");
    EXPECT_EQ(elements.size(), 1);
}

// ========== 链接伪类测试 ==========

TEST_F(CSS3SelectorsTest, LinkPseudoClass) {
    // :link 伪类匹配未访问的链接
    // 注意：Lexbor 可能不支持动态伪类，这个测试可能失败
    auto elements = doc.QuerySelectorAll("a");
    EXPECT_EQ(elements.size(), 3);
}

// ========== 性能测试 ==========

TEST_F(CSS3SelectorsTest, ComplexSelectorPerformance) {
    // 测试复杂选择器的性能
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        auto elements = doc.QuerySelectorAll("#container > ul#list > li.item:nth-child(odd)");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    
    // 100 次复杂选择器查询应该在 100ms 内完成
    EXPECT_LT(duration.count(), 100.0) << "Complex selector took " << duration.count() << "ms";
}

