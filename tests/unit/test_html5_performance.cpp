#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include <string>
#include <chrono>
#include <sstream>

using namespace lightui;

/**
 * @brief HTML5 性能测试
 * 
 * 测试 HTML5 解析器的性能，包括解析速度、内存占用等
 */
class HTML5PerformanceTest : public ::testing::Test {
protected:
    LexborDocument doc;
    
    // 辅助函数：生成大型 HTML 文档
    std::string GenerateLargeHTML(int elementCount) {
        std::ostringstream oss;
        oss << "<!DOCTYPE html><html><head><title>Performance Test</title></head><body>";
        
        for (int i = 0; i < elementCount; i++) {
            oss << "<div class='item-" << i << "' id='element-" << i << "'>";
            oss << "<p>This is paragraph " << i << "</p>";
            oss << "<span>Span " << i << "</span>";
            oss << "</div>";
        }
        
        oss << "</body></html>";
        return oss.str();
    }
    
    // 辅助函数：生成深度嵌套的 HTML
    std::string GenerateDeeplyNestedHTML(int depth) {
        std::ostringstream oss;
        oss << "<!DOCTYPE html><html><body>";
        
        for (int i = 0; i < depth; i++) {
            oss << "<div id='level-" << i << "'>";
        }
        
        oss << "<p>Deep content</p>";
        
        for (int i = 0; i < depth; i++) {
            oss << "</div>";
        }
        
        oss << "</body></html>";
        return oss.str();
    }
    
    // 辅助函数：生成包含大量属性的 HTML
    std::string GenerateHTMLWithManyAttributes(int attrCount) {
        std::ostringstream oss;
        oss << "<!DOCTYPE html><html><body><div ";
        
        for (int i = 0; i < attrCount; i++) {
            oss << "data-attr-" << i << "='value-" << i << "' ";
        }
        
        oss << ">Content</div></body></html>";
        return oss.str();
    }
    
    // 辅助函数：测量解析时间（毫秒）
    double MeasureParseTime(const std::string& html) {
        auto start = std::chrono::high_resolution_clock::now();
        doc.ParseHTML(html);
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> duration = end - start;
        return duration.count();
    }
};

// ========== 解析速度测试 ==========

TEST_F(HTML5PerformanceTest, ParseSmallDocument) {
    std::string html = GenerateLargeHTML(10);
    double time = MeasureParseTime(html);
    
    // 小文档应该在 10ms 内解析完成
    EXPECT_LT(time, 10.0) << "Small document parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 10);
}

TEST_F(HTML5PerformanceTest, ParseMediumDocument) {
    std::string html = GenerateLargeHTML(100);
    double time = MeasureParseTime(html);
    
    // 中等文档应该在 50ms 内解析完成
    EXPECT_LT(time, 50.0) << "Medium document parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 100);
}

TEST_F(HTML5PerformanceTest, ParseLargeDocument) {
    std::string html = GenerateLargeHTML(1000);
    double time = MeasureParseTime(html);
    
    // 大文档应该在 500ms 内解析完成
    EXPECT_LT(time, 500.0) << "Large document parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 1000);
}

TEST_F(HTML5PerformanceTest, ParseVeryLargeDocument) {
    std::string html = GenerateLargeHTML(5000);
    double time = MeasureParseTime(html);
    
    // 超大文档应该在 2000ms 内解析完成
    EXPECT_LT(time, 2000.0) << "Very large document parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 5000);
}

// ========== 深度嵌套性能测试 ==========

TEST_F(HTML5PerformanceTest, ParseShallowNesting) {
    std::string html = GenerateDeeplyNestedHTML(10);
    double time = MeasureParseTime(html);
    
    // 浅嵌套应该在 10ms 内解析完成
    EXPECT_LT(time, 10.0) << "Shallow nesting parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 10);
}

TEST_F(HTML5PerformanceTest, ParseMediumNesting) {
    std::string html = GenerateDeeplyNestedHTML(50);
    double time = MeasureParseTime(html);
    
    // 中等嵌套应该在 50ms 内解析完成
    EXPECT_LT(time, 50.0) << "Medium nesting parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 50);
}

TEST_F(HTML5PerformanceTest, ParseDeepNesting) {
    std::string html = GenerateDeeplyNestedHTML(100);
    double time = MeasureParseTime(html);
    
    // 深度嵌套应该在 100ms 内解析完成
    EXPECT_LT(time, 100.0) << "Deep nesting parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs.size(), 100);
}

// ========== 属性处理性能测试 ==========

TEST_F(HTML5PerformanceTest, ParseFewAttributes) {
    std::string html = GenerateHTMLWithManyAttributes(10);
    double time = MeasureParseTime(html);
    
    // 少量属性应该在 10ms 内解析完成
    EXPECT_LT(time, 10.0) << "Few attributes parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    ASSERT_EQ(divs.size(), 1);
    EXPECT_TRUE(divs[0]->HasAttribute("data-attr-0"));
}

TEST_F(HTML5PerformanceTest, ParseManyAttributes) {
    std::string html = GenerateHTMLWithManyAttributes(100);
    double time = MeasureParseTime(html);
    
    // 大量属性应该在 50ms 内解析完成
    EXPECT_LT(time, 50.0) << "Many attributes parsing took " << time << "ms";
    
    // 验证解析正确
    auto divs = doc.QuerySelectorAll("div");
    ASSERT_EQ(divs.size(), 1);
    EXPECT_TRUE(divs[0]->HasAttribute("data-attr-0"));
    EXPECT_TRUE(divs[0]->HasAttribute("data-attr-99"));
}

// ========== querySelector 性能测试 ==========

TEST_F(HTML5PerformanceTest, QuerySelectorPerformance) {
    std::string html = GenerateLargeHTML(1000);
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 测试 querySelector 性能
    auto start = std::chrono::high_resolution_clock::now();
    auto element = doc.QuerySelector("#element-500");
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    
    // querySelector 应该在 10ms 内完成
    EXPECT_LT(duration.count(), 10.0) << "querySelector took " << duration.count() << "ms";
    EXPECT_NE(element, nullptr);
}

TEST_F(HTML5PerformanceTest, QuerySelectorAllPerformance) {
    std::string html = GenerateLargeHTML(1000);
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 测试 querySelectorAll 性能
    auto start = std::chrono::high_resolution_clock::now();
    auto elements = doc.QuerySelectorAll("div");
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    
    // querySelectorAll 应该在 50ms 内完成
    EXPECT_LT(duration.count(), 50.0) << "querySelectorAll took " << duration.count() << "ms";
    EXPECT_EQ(elements.size(), 1000);
}

TEST_F(HTML5PerformanceTest, ComplexSelectorPerformance) {
    std::string html = GenerateLargeHTML(1000);
    ASSERT_TRUE(doc.ParseHTML(html));
    
    // 测试复杂选择器性能
    auto start = std::chrono::high_resolution_clock::now();
    auto elements = doc.QuerySelectorAll("div.item-500 p");
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    
    // 复杂选择器应该在 50ms 内完成
    EXPECT_LT(duration.count(), 50.0) << "Complex selector took " << duration.count() << "ms";
}

// ========== 内存效率测试 ==========

TEST_F(HTML5PerformanceTest, MultipleParseMemoryTest) {
    // 多次解析同一文档，验证没有内存泄漏
    std::string html = GenerateLargeHTML(100);
    
    for (int i = 0; i < 10; i++) {
        LexborDocument tempDoc;
        ASSERT_TRUE(tempDoc.ParseHTML(html));
        auto divs = tempDoc.QuerySelectorAll("div");
        EXPECT_EQ(divs.size(), 100);
    }
    
    // 如果有内存泄漏，这个测试会导致内存持续增长
    // 在实际使用中应该配合内存分析工具（如 Valgrind）
}

TEST_F(HTML5PerformanceTest, ReparsePerformance) {
    std::string html1 = GenerateLargeHTML(100);
    std::string html2 = GenerateLargeHTML(200);
    
    // 第一次解析
    ASSERT_TRUE(doc.ParseHTML(html1));
    auto divs1 = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs1.size(), 100);
    
    // 重新解析（应该清理之前的文档）
    ASSERT_TRUE(doc.ParseHTML(html2));
    auto divs2 = doc.QuerySelectorAll("div");
    EXPECT_EQ(divs2.size(), 200);
    
    // 验证之前的查询结果不再有效
    // （这取决于实现，可能需要调整）
}

// ========== 实体解析性能测试 ==========

TEST_F(HTML5PerformanceTest, EntityParsingPerformance) {
    std::ostringstream oss;
    oss << "<!DOCTYPE html><html><body><p>";
    
    // 生成包含大量实体的文本
    for (int i = 0; i < 1000; i++) {
        oss << "&lt;div&gt; &amp; &quot;text&quot; ";
    }
    
    oss << "</p></body></html>";
    std::string html = oss.str();
    
    double time = MeasureParseTime(html);
    
    // 实体解析应该在 100ms 内完成
    EXPECT_LT(time, 100.0) << "Entity parsing took " << time << "ms";
    
    // 验证实体被正确解析
    auto paragraphs = doc.QuerySelectorAll("p");
    ASSERT_EQ(paragraphs.size(), 1);
    std::string content = paragraphs[0]->GetTextContent();
    EXPECT_TRUE(content.find("<div>") != std::string::npos);
}

// ========== 综合性能测试 ==========

TEST_F(HTML5PerformanceTest, ComplexDocumentPerformance) {
    // 生成一个复杂的真实世界文档
    std::ostringstream oss;
    oss << R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Complex Document</title>
            <style>
                body { margin: 0; padding: 0; }
                .container { max-width: 1200px; }
            </style>
        </head>
        <body>
            <header>
                <nav>
                    <ul>
    )";
    
    for (int i = 0; i < 10; i++) {
        oss << "<li><a href='#section-" << i << "'>Section " << i << "</a></li>";
    }
    
    oss << "</ul></nav></header><main>";
    
    for (int i = 0; i < 50; i++) {
        oss << R"(
            <section id='section-)" << i << R"('>
                <h2>Section )" << i << R"(</h2>
                <article>
                    <p>This is paragraph 1 with <strong>bold</strong> and <em>italic</em> text.</p>
                    <p>This is paragraph 2 with <a href='#'>a link</a>.</p>
                    <ul>
                        <li>Item 1</li>
                        <li>Item 2</li>
                        <li>Item 3</li>
                    </ul>
                </article>
            </section>
        )";
    }
    
    oss << R"(
        </main>
        <footer>
            <p>&copy; 2025 Test Company. All rights reserved.</p>
        </footer>
        <script>
            console.log("Document loaded");
        </script>
        </body>
        </html>
    )";
    
    std::string html = oss.str();
    double time = MeasureParseTime(html);
    
    // 复杂文档应该在 200ms 内解析完成
    EXPECT_LT(time, 200.0) << "Complex document parsing took " << time << "ms";
    
    // 验证解析正确
    auto sections = doc.QuerySelectorAll("section");
    EXPECT_EQ(sections.size(), 50);
    
    auto links = doc.QuerySelectorAll("a");
    EXPECT_GT(links.size(), 50);  // 至少有 50 个链接
}

