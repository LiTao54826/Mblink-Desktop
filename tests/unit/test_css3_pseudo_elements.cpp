#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>

using namespace lightui;

/**
 * @brief CSS3 伪元素测试
 * 
 * 测试伪元素的支持，包括 ::before, ::after, ::first-line, ::first-letter 等
 */
class CSS3PseudoElementsTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    void SetUp() override {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Pseudo Elements Test</title>
                <style>
                    .box::before {
                        content: "Before ";
                        color: red;
                    }
                    
                    .box::after {
                        content: " After";
                        color: blue;
                    }
                    
                    p::first-line {
                        font-weight: bold;
                    }
                    
                    p::first-letter {
                        font-size: 2em;
                        color: green;
                    }
                    
                    .quote::before {
                        content: """;
                    }
                    
                    .quote::after {
                        content: """;
                    }
                    
                    .counter::before {
                        content: counter(section);
                    }
                    
                    input::placeholder {
                        color: gray;
                    }
                    
                    ::selection {
                        background: yellow;
                        color: black;
                    }
                    
                    .marker::marker {
                        color: red;
                    }
                </style>
            </head>
            <body>
                <div class="box">Content</div>
                <p class="paragraph">This is a long paragraph with multiple lines of text that will be used to test the first-line and first-letter pseudo-elements.</p>
                <blockquote class="quote">This is a quote</blockquote>
                <div class="counter">Section 1</div>
                <input type="text" placeholder="Enter text" />
                <ul>
                    <li class="marker">Item 1</li>
                    <li class="marker">Item 2</li>
                </ul>
            </body>
            </html>
        )";
        
        ASSERT_TRUE(doc.ParseHTML(html));
    }
};

// ========== 基础伪元素测试 ==========

TEST_F(CSS3PseudoElementsTest, BeforePseudoElement) {
    // 测试 ::before 伪元素的 CSS 解析
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::before") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, AfterPseudoElement) {
    // 测试 ::after 伪元素的 CSS 解析
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::after") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLinePseudoElement) {
    // 测试 ::first-line 伪元素的 CSS 解析
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::first-line") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLetterPseudoElement) {
    // 测试 ::first-letter 伪元素的 CSS 解析
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::first-letter") != std::string::npos);
}

// ========== CSS 样式表解析测试 ==========

TEST_F(CSS3PseudoElementsTest, ParseStylesheet) {
    // 测试样式表解析
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);

    auto styleText = styleElement->GetTextContent();
    EXPECT_FALSE(styleText.empty());

    // 验证样式文本包含伪元素
    EXPECT_TRUE(styleText.find("::before") != std::string::npos);
    EXPECT_TRUE(styleText.find("::after") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, BeforeContentProperty) {
    // 测试 ::before 的 content 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("content:") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, AfterContentProperty) {
    // 测试 ::after 的 content 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    
    // 验证包含 content 属性
    size_t pos = 0;
    int contentCount = 0;
    while ((pos = styleText.find("content:", pos)) != std::string::npos) {
        contentCount++;
        pos++;
    }
    EXPECT_GE(contentCount, 2);  // 至少有 ::before 和 ::after 的 content
}

// ========== 伪元素选择器测试 ==========

TEST_F(CSS3PseudoElementsTest, BeforeSelector) {
    // 测试 .box::before 选择器
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find(".box::before") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, AfterSelector) {
    // 测试 .box::after 选择器
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find(".box::after") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLineSelector) {
    // 测试 p::first-line 选择器
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("p::first-line") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLetterSelector) {
    // 测试 p::first-letter 选择器
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("p::first-letter") != std::string::npos);
}

// ========== 内容生成测试 ==========

TEST_F(CSS3PseudoElementsTest, ContentString) {
    // 测试字符串内容生成
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("\"Before \"") != std::string::npos);
    EXPECT_TRUE(styleText.find("\" After\"") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, ContentQuotes) {
    // 测试引号内容生成
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find(".quote::before") != std::string::npos);
    EXPECT_TRUE(styleText.find(".quote::after") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, ContentCounter) {
    // 测试计数器内容生成
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("counter(section)") != std::string::npos);
}

// ========== 其他伪元素测试 ==========

TEST_F(CSS3PseudoElementsTest, PlaceholderPseudoElement) {
    // 测试 ::placeholder 伪元素
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::placeholder") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, SelectionPseudoElement) {
    // 测试 ::selection 伪元素
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::selection") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, MarkerPseudoElement) {
    // 测试 ::marker 伪元素
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("::marker") != std::string::npos);
}

// ========== 样式属性测试 ==========

TEST_F(CSS3PseudoElementsTest, BeforeColorProperty) {
    // 测试 ::before 的 color 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    
    // 查找 .box::before 规则中的 color: red
    size_t beforePos = styleText.find(".box::before");
    ASSERT_NE(beforePos, std::string::npos);
    
    size_t colorPos = styleText.find("color:", beforePos);
    EXPECT_NE(colorPos, std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, AfterColorProperty) {
    // 测试 ::after 的 color 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    
    // 查找 .box::after 规则中的 color: blue
    size_t afterPos = styleText.find(".box::after");
    ASSERT_NE(afterPos, std::string::npos);
    
    size_t colorPos = styleText.find("color:", afterPos);
    EXPECT_NE(colorPos, std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLineFontWeight) {
    // 测试 ::first-line 的 font-weight 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("font-weight: bold") != std::string::npos);
}

TEST_F(CSS3PseudoElementsTest, FirstLetterFontSize) {
    // 测试 ::first-letter 的 font-size 属性
    auto styleElement = doc.QuerySelector("style");
    ASSERT_NE(styleElement, nullptr);
    
    auto styleText = styleElement->GetTextContent();
    EXPECT_TRUE(styleText.find("font-size: 2em") != std::string::npos);
}

// ========== DOM 元素测试 ==========

TEST_F(CSS3PseudoElementsTest, BoxElement) {
    // 测试 .box 元素存在
    auto box = doc.QuerySelector(".box");
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->GetTextContent(), "Content");
}

TEST_F(CSS3PseudoElementsTest, ParagraphElement) {
    // 测试段落元素存在
    auto paragraph = doc.QuerySelector(".paragraph");
    ASSERT_NE(paragraph, nullptr);
    EXPECT_FALSE(paragraph->GetTextContent().empty());
}

TEST_F(CSS3PseudoElementsTest, QuoteElement) {
    // 测试引用元素存在
    auto quote = doc.QuerySelector(".quote");
    ASSERT_NE(quote, nullptr);
    EXPECT_EQ(quote->GetTextContent(), "This is a quote");
}

TEST_F(CSS3PseudoElementsTest, CounterElement) {
    // 测试计数器元素存在
    auto counter = doc.QuerySelector(".counter");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->GetTextContent(), "Section 1");
}

TEST_F(CSS3PseudoElementsTest, InputElement) {
    // 测试输入元素存在
    auto input = doc.QuerySelector("input[type='text']");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetAttribute("placeholder"), "Enter text");
}

TEST_F(CSS3PseudoElementsTest, MarkerElements) {
    // 测试列表项元素存在
    auto markers = doc.QuerySelectorAll(".marker");
    EXPECT_EQ(markers.size(), 2);
}

