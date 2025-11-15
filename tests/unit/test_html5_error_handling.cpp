#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>
#include <fstream>
#include <cstdio>

using namespace lightui;

/**
 * @brief HTML5 错误处理和警告系统测试
 * 
 * 测试 HTML5 解析器的错误处理、警告收集和容错机制
 */
class HTML5ErrorHandlingTest : public ::testing::Test {
protected:
    LexborDocument doc;
};

// ========== 错误处理测试 ==========

TEST_F(HTML5ErrorHandlingTest, EmptyDocumentError) {
    // 空文档应该能够解析，但可能有警告
    bool result = doc.ParseHTML("");
    EXPECT_TRUE(result);  // Lexbor 应该能处理空文档
}

TEST_F(HTML5ErrorHandlingTest, NullDocumentError) {
    // 测试文档未初始化的情况
    LexborDocument null_doc;
    // 文档应该在构造时初始化，所以这个测试可能不适用
    // 但我们可以测试解析空字符串
    EXPECT_TRUE(null_doc.ParseHTML(""));
}

TEST_F(HTML5ErrorHandlingTest, MalformedHTMLWarnings) {
    // 解析格式错误的 HTML，应该有警告但仍能解析
    std::string html = R"(
        <div>
            <p>Unclosed paragraph
            <span>Unclosed span
        </div>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
    
    // 检查是否有警告
    // 注意：Lexbor 可能会自动修复这些问题，所以可能没有警告
    // 这取决于我们如何实现警告系统
}

TEST_F(HTML5ErrorHandlingTest, InvalidNestingWarnings) {
    // 测试无效的嵌套（例如 <p> 里面嵌套 <div>）
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <p>
                <div>Invalid nesting</div>
            </p>
        </body>
        </html>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
    
    // Lexbor 会自动修复无效嵌套
    LexborElement* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
}

TEST_F(HTML5ErrorHandlingTest, MismatchedTagsWarnings) {
    // 测试标签不匹配
    std::string html = R"(
        <div>
            <span>Text</div>
        </span>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
}

TEST_F(HTML5ErrorHandlingTest, ExtraClosingTagsWarnings) {
    // 测试多余的闭合标签
    std::string html = R"(
        <div>
            <p>Text</p>
        </div>
        </div>
        </div>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
}

// ========== 容错机制测试 ==========

TEST_F(HTML5ErrorHandlingTest, AutoCloseUnclosedTags) {
    // 测试自动闭合未闭合的标签
    std::string html = R"(
        <div>
            <p>Paragraph 1
            <p>Paragraph 2
        </div>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    LexborElement* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
    
    // 应该有一个 div 元素
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 1);
    
    // 应该有两个 p 元素
    auto paragraphs = doc.QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 2);
}

TEST_F(HTML5ErrorHandlingTest, HandleInvalidCharacters) {
    // 测试处理无效字符
    std::string html = R"(
        <div>
            <p>Text with \x00 null character</p>
        </div>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
}

TEST_F(HTML5ErrorHandlingTest, HandleVeryLongAttributeValue) {
    // 测试处理非常长的属性值
    std::string long_value(10000, 'a');
    std::string html = "<div data-value=\"" + long_value + "\">Text</div>";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    auto divs = doc.QuerySelectorAll("div");
    ASSERT_EQ(divs.size(), 1);
    
    std::string attr_value = divs[0]->GetAttribute("data-value");
    EXPECT_EQ(attr_value.length(), 10000);
}

TEST_F(HTML5ErrorHandlingTest, HandleDeeplyNestedElements) {
    // 测试处理深度嵌套的元素
    std::string html = "<div>";
    for (int i = 0; i < 100; i++) {
        html += "<div>";
    }
    html += "Deep content";
    for (int i = 0; i < 100; i++) {
        html += "</div>";
    }
    html += "</div>";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 101);  // 101 个 div 元素
}

// ========== 边界情况测试 ==========

TEST_F(HTML5ErrorHandlingTest, ParseOnlyWhitespace) {
    // 测试只有空白字符的文档
    std::string html = "   \n\t\r\n   ";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
}

TEST_F(HTML5ErrorHandlingTest, ParseOnlyComments) {
    // 测试只有注释的文档
    std::string html = "<!-- Comment 1 --><!-- Comment 2 -->";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
}

TEST_F(HTML5ErrorHandlingTest, ParseInvalidDoctype) {
    // 测试无效的 DOCTYPE
    std::string html = R"(
        <!DOCTYPE invalid>
        <html>
        <body>
            <p>Text</p>
        </body>
        </html>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);  // 应该能解析
    
    // 检查文档模式
    std::string mode = doc.GetDocumentMode();
    // 无效的 DOCTYPE 可能导致 quirks 模式
    EXPECT_FALSE(mode.empty());
}

TEST_F(HTML5ErrorHandlingTest, ParseMultipleDoctypes) {
    // 测试多个 DOCTYPE（只有第一个应该生效）
    std::string html = R"(
        <!DOCTYPE html>
        <!DOCTYPE html>
        <html>
        <body>
            <p>Text</p>
        </body>
        </html>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
}

TEST_F(HTML5ErrorHandlingTest, ParseHTMLWithBOM) {
    // 测试带 BOM 的 HTML
    std::string html = "\xEF\xBB\xBF<!DOCTYPE html><html><body><p>Text</p></body></html>";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    auto paragraphs = doc.QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 1);
}

// ========== 错误恢复测试 ==========

TEST_F(HTML5ErrorHandlingTest, RecoverFromInvalidHTML) {
    // 测试从严重错误的 HTML 中恢复
    std::string html = R"(
        <html>
        <body>
            <div>
                <p>Valid content</p>
                <invalid-tag>Invalid content</invalid-tag>
                <p>More valid content</p>
            </div>
        </body>
        </html>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    // 应该能找到有效的 p 元素
    auto paragraphs = doc.QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 2);
    
    // 自定义标签也应该被解析
    auto invalid_tags = doc.QuerySelectorAll("invalid-tag");
    EXPECT_EQ(invalid_tags.size(), 1);
}

TEST_F(HTML5ErrorHandlingTest, RecoverFromMissingRequiredElements) {
    // 测试缺少必需元素（如 <html>, <body>）
    std::string html = "<p>Just a paragraph</p>";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    // Lexbor 应该自动添加 <html> 和 <body>
    LexborElement* body = doc.GetBody();
    ASSERT_NE(body, nullptr);
    
    auto paragraphs = doc.QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 1);
}

TEST_F(HTML5ErrorHandlingTest, HandleScriptWithSpecialCharacters) {
    // 测试包含特殊字符的 script 标签
    std::string html = R"(
        <html>
        <body>
            <script>
                var x = "<div>Not HTML</div>";
                var y = "a < b && c > d";
            </script>
        </body>
        </html>
    )";
    
    bool result = doc.ParseHTML(html);
    EXPECT_TRUE(result);
    
    auto scripts = doc.QuerySelectorAll("script");
    EXPECT_EQ(scripts.size(), 1);
}

// ========== 文件解析错误测试 ==========

TEST_F(HTML5ErrorHandlingTest, ParseNonExistentFile) {
    // 测试解析不存在的文件
    bool result = doc.ParseHTMLFile("non_existent_file.html");
    EXPECT_FALSE(result);
    
    // 应该有错误信息
    auto errors = doc.GetErrors();
    EXPECT_FALSE(errors.empty());
}

TEST_F(HTML5ErrorHandlingTest, ParseEmptyFile) {
    // 创建一个临时空文件
    std::string temp_file = "temp_empty.html";
    std::ofstream file(temp_file);
    file.close();

    bool result = doc.ParseHTMLFile(temp_file);
    // 空文件可能解析失败，这取决于实现
    // 我们只是验证不会崩溃
    (void)result;  // 忽略结果

    // 清理临时文件
    std::remove(temp_file.c_str());
}

