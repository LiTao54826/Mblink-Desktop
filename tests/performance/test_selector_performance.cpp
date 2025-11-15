#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "performance_utils.h"
#include <iostream>

using namespace lightui;
using namespace mbink::performance;

class SelectorPerformanceTest : public ::testing::Test {
protected:
    PerformanceReporter reporter_;
    LexborDocument doc_;
    
    void SetUp() override {
        std::cout << "\n=== Selector Performance Tests ===\n";
        
        // 创建一个包含 10K 元素的文档用于测试
        std::string html = HTMLGenerator::GenerateComplexHTML(10000);
        doc_.ParseHTML(html);
    }
    
    void TearDown() override {
        reporter_.PrintSummary();
        reporter_.GenerateReport("docs/selector_performance_report.md");
        reporter_.GenerateCSV("docs/selector_performance_data.csv");
    }
};

// 测试 ID 选择器性能
TEST_F(SelectorPerformanceTest, IDSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < 1000; i++) {
        auto elem = doc_.QuerySelector("#section500");
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedUs() / 1000.0;
    
    reporter_.AddResult("ID Selector (avg)", avgTime, "μs", "Selector Performance");
    
    std::cout << "ID selector avg: " << avgTime << " μs\n";
    
    EXPECT_LT(avgTime, 10.0) << "ID selector should be very fast (< 10μs)";
}

// 测试类选择器性能
TEST_F(SelectorPerformanceTest, ClassSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll(".section");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Class Selector (.section)", elapsed, "ms", "Selector Performance");
    
    std::cout << "Class selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 50.0) << "Class selector should be fast (< 50ms)";
}

// 测试标签选择器性能
TEST_F(SelectorPerformanceTest, TagSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("div");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Tag Selector (div)", elapsed, "ms", "Selector Performance");
    
    std::cout << "Tag selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 50.0) << "Tag selector should be fast (< 50ms)";
}

// 测试属性选择器性能
TEST_F(SelectorPerformanceTest, AttributeSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("[data-level]");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Attribute Selector ([data-level])", elapsed, "ms", "Selector Performance");
    
    std::cout << "Attribute selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 100.0) << "Attribute selector should be reasonably fast (< 100ms)";
}

// 测试后代选择器性能
TEST_F(SelectorPerformanceTest, DescendantSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("section li");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Descendant Selector (section li)", elapsed, "ms", "Selector Performance");
    
    std::cout << "Descendant selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 100.0) << "Descendant selector should be reasonably fast (< 100ms)";
}

// 测试子选择器性能
TEST_F(SelectorPerformanceTest, ChildSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("ul > li");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Child Selector (ul > li)", elapsed, "ms", "Selector Performance");
    
    std::cout << "Child selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 80.0) << "Child selector should be faster than descendant (< 80ms)";
}

// 测试复杂选择器性能
TEST_F(SelectorPerformanceTest, ComplexSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("section.section > ul.list > li.item[data-value]");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Complex Selector", elapsed, "ms", "Selector Performance");
    
    std::cout << "Complex selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 150.0) << "Complex selector should complete in reasonable time (< 150ms)";
}

// 测试伪类选择器性能
TEST_F(SelectorPerformanceTest, PseudoClassSelector) {
    PerformanceTimer timer;
    
    timer.Start();
    auto elements = doc_.QuerySelectorAll("li:first-child");
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Pseudo-class Selector (:first-child)", elapsed, "ms", "Selector Performance");
    
    std::cout << "Pseudo-class selector: " << elapsed << " ms, found " << elements.size() << " elements\n";
    
    EXPECT_LT(elapsed, 100.0) << "Pseudo-class selector should be reasonably fast (< 100ms)";
}

// 测试 querySelector vs querySelectorAll
TEST_F(SelectorPerformanceTest, QuerySelectorVsAll) {
    PerformanceTimer timer1, timer2;
    
    // querySelector (找第一个)
    timer1.Start();
    for (int i = 0; i < 100; i++) {
        auto elem = doc_.QuerySelector(".item");
    }
    timer1.Stop();
    
    // querySelectorAll (找所有)
    timer2.Start();
    for (int i = 0; i < 100; i++) {
        auto elements = doc_.QuerySelectorAll(".item");
    }
    timer2.Stop();
    
    double avgQuerySelector = timer1.GetElapsedMs() / 100.0;
    double avgQuerySelectorAll = timer2.GetElapsedMs() / 100.0;
    
    reporter_.AddResult("querySelector (avg, 100 runs)", avgQuerySelector, "ms", "Comparison");
    reporter_.AddResult("querySelectorAll (avg, 100 runs)", avgQuerySelectorAll, "ms", "Comparison");
    
    std::cout << "querySelector avg: " << avgQuerySelector << " ms\n";
    std::cout << "querySelectorAll avg: " << avgQuerySelectorAll << " ms\n";
    
    EXPECT_LT(avgQuerySelector, avgQuerySelectorAll) 
        << "querySelector should be faster than querySelectorAll";
}

// 测试不同文档规模下的选择器性能
TEST_F(SelectorPerformanceTest, ScalabilityTest) {
    std::vector<size_t> sizes = {100, 500, 1000, 5000, 10000};
    
    for (size_t size : sizes) {
        std::string html = HTMLGenerator::GenerateSimpleHTML(size);
        LexborDocument doc;
        doc.ParseHTML(html);
        
        PerformanceTimer timer;
        timer.Start();
        auto elements = doc.QuerySelectorAll(".item");
        timer.Stop();
        
        double elapsed = timer.GetElapsedMs();
        
        std::string testName = "QuerySelectorAll (" + std::to_string(size) + " elements)";
        reporter_.AddResult(testName, elapsed, "ms", "Scalability");
        
        std::cout << size << " elements: " << elapsed << " ms\n";
    }
}

// 测试选择器缓存效果
TEST_F(SelectorPerformanceTest, SelectorCaching) {
    const std::string selector = "section.section > ul.list > li.item";
    
    PerformanceTimer timer1, timer2;
    
    // 第一次查询（可能需要编译选择器）
    timer1.Start();
    auto elements1 = doc_.QuerySelectorAll(selector);
    timer1.Stop();
    
    // 第二次查询（可能使用缓存）
    timer2.Start();
    auto elements2 = doc_.QuerySelectorAll(selector);
    timer2.Stop();
    
    double firstTime = timer1.GetElapsedMs();
    double secondTime = timer2.GetElapsedMs();
    
    reporter_.AddResult("First Query (complex selector)", firstTime, "ms", "Caching Effect");
    reporter_.AddResult("Second Query (complex selector)", secondTime, "ms", "Caching Effect");
    
    std::cout << "First query: " << firstTime << " ms\n";
    std::cout << "Second query: " << secondTime << " ms\n";
    
    EXPECT_EQ(elements1.size(), elements2.size()) << "Both queries should return same results";
}

// 测试元素级别的 querySelector
TEST_F(SelectorPerformanceTest, ElementQuerySelector) {
    auto section = doc_.QuerySelector("section");
    ASSERT_NE(section, nullptr);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < 1000; i++) {
        auto items = section->QuerySelectorAll("li");
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedMs() / 1000.0;
    
    reporter_.AddResult("Element.querySelectorAll (avg)", avgTime, "ms", "Element Query");
    
    std::cout << "Element querySelectorAll avg: " << avgTime << " ms\n";
    
    EXPECT_LT(avgTime, 5.0) << "Element-level query should be very fast (< 5ms)";
}

// 测试多个选择器的性能对比
TEST_F(SelectorPerformanceTest, SelectorComparison) {
    struct SelectorTest {
        std::string name;
        std::string selector;
    };
    
    std::vector<SelectorTest> tests = {
        {"ID", "#section500"},
        {"Class", ".section"},
        {"Tag", "div"},
        {"Attribute Exists", "[data-level]"},
        {"Attribute Equals", "[data-level='0']"},
        {"Descendant", "section li"},
        {"Child", "ul > li"},
        {"Multiple Classes", ".section.item"},
        {"Pseudo-class", "li:first-child"},
        {"Complex", "section.section > ul.list > li.item"}
    };
    
    for (const auto& test : tests) {
        PerformanceTimer timer;
        
        timer.Start();
        if (test.selector.find('#') == 0) {
            // ID selector - use querySelector
            auto elem = doc_.QuerySelector(test.selector);
        } else {
            // Other selectors - use querySelectorAll
            auto elements = doc_.QuerySelectorAll(test.selector);
        }
        timer.Stop();
        
        double elapsed = timer.GetElapsedMs();
        
        reporter_.AddResult(test.name + " Selector", elapsed, "ms", "Selector Comparison");
        
        std::cout << test.name << " selector: " << elapsed << " ms\n";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

