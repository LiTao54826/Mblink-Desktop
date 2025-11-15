#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "performance_utils.h"
#include <iostream>

using namespace lightui;
using namespace mbink::performance;

class DOMPerformanceTest : public ::testing::Test {
protected:
    PerformanceReporter reporter_;
    LexborDocument doc_;
    
    void SetUp() override {
        std::cout << "\n=== DOM Operations Performance Tests ===\n";
        
        // 创建一个包含 5K 元素的文档用于测试
        std::string html = HTMLGenerator::GenerateComplexHTML(5000);
        doc_.ParseHTML(html);
    }
    
    void TearDown() override {
        reporter_.PrintSummary();
        reporter_.GenerateReport("docs/dom_performance_report.md");
        reporter_.GenerateCSV("docs/dom_performance_data.csv");
    }
};

// 测试属性读取性能
TEST_F(DOMPerformanceTest, AttributeRead) {
    auto elements = doc_.QuerySelectorAll("[data-value]");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < 10000; i++) {
        for (auto* elem : elements) {
            std::string value = elem->GetAttribute("data-value");
        }
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedUs() / (10000.0 * elements.size());
    
    reporter_.AddResult("Attribute Read (avg per element)", avgTime, "ns", "Attribute Operations");
    
    std::cout << "Attribute read avg: " << avgTime << " ns per element\n";
    
    EXPECT_LT(avgTime, 1000.0) << "Attribute read should be very fast (< 1μs)";
}

// 测试属性写入性能
TEST_F(DOMPerformanceTest, AttributeWrite) {
    auto elements = doc_.QuerySelectorAll("div");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (auto* elem : elements) {
        elem->SetAttribute("data-test", "value");
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedUs() / elements.size();
    
    reporter_.AddResult("Attribute Write (avg per element)", avgTime, "μs", "Attribute Operations");
    
    std::cout << "Attribute write avg: " << avgTime << " μs per element\n";
    
    EXPECT_LT(avgTime, 10.0) << "Attribute write should be fast (< 10μs)";
}

// 测试文本内容读取性能
TEST_F(DOMPerformanceTest, TextContentRead) {
    auto elements = doc_.QuerySelectorAll("li");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < 1000; i++) {
        for (auto* elem : elements) {
            std::string text = elem->GetTextContent();
        }
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedUs() / (1000.0 * elements.size());
    
    reporter_.AddResult("Text Content Read (avg per element)", avgTime, "ns", "Content Operations");
    
    std::cout << "Text content read avg: " << avgTime << " ns per element\n";
    
    EXPECT_LT(avgTime, 1000.0) << "Text content read should be very fast (< 1μs)";
}

// 测试文本内容写入性能
TEST_F(DOMPerformanceTest, TextContentWrite) {
    auto elements = doc_.QuerySelectorAll("li");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (auto* elem : elements) {
        elem->SetTextContent("New text content");
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedUs() / elements.size();
    
    reporter_.AddResult("Text Content Write (avg per element)", avgTime, "μs", "Content Operations");
    
    std::cout << "Text content write avg: " << avgTime << " μs per element\n";
    
    EXPECT_LT(avgTime, 10.0) << "Text content write should be fast (< 10μs)";
}

// 测试 Class 操作性能
TEST_F(DOMPerformanceTest, ClassOperations) {
    auto elements = doc_.QuerySelectorAll("div");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer1, timer2, timer3;
    
    // HasClass
    timer1.Start();
    for (auto* elem : elements) {
        bool has = elem->HasClass("section");
    }
    timer1.Stop();
    
    // AddClass
    timer2.Start();
    for (auto* elem : elements) {
        elem->AddClass("new-class");
    }
    timer2.Stop();
    
    // RemoveClass
    timer3.Start();
    for (auto* elem : elements) {
        elem->RemoveClass("new-class");
    }
    timer3.Stop();
    
    double avgHas = timer1.GetElapsedUs() / elements.size();
    double avgAdd = timer2.GetElapsedUs() / elements.size();
    double avgRemove = timer3.GetElapsedUs() / elements.size();
    
    reporter_.AddResult("HasClass (avg)", avgHas, "μs", "Class Operations");
    reporter_.AddResult("AddClass (avg)", avgAdd, "μs", "Class Operations");
    reporter_.AddResult("RemoveClass (avg)", avgRemove, "μs", "Class Operations");
    
    std::cout << "HasClass avg: " << avgHas << " μs\n";
    std::cout << "AddClass avg: " << avgAdd << " μs\n";
    std::cout << "RemoveClass avg: " << avgRemove << " μs\n";
    
    EXPECT_LT(avgHas, 5.0) << "HasClass should be very fast (< 5μs)";
    EXPECT_LT(avgAdd, 10.0) << "AddClass should be fast (< 10μs)";
    EXPECT_LT(avgRemove, 10.0) << "RemoveClass should be fast (< 10μs)";
}

// 测试 DOM 树遍历性能
TEST_F(DOMPerformanceTest, DOMTraversal) {
    auto root = doc_.QuerySelector("body");
    ASSERT_NE(root, nullptr);
    
    PerformanceTimer timer;
    int elementCount = 0;
    
    timer.Start();
    
    // 递归遍历整个 DOM 树
    std::function<void(LexborElement*)> traverse = [&](LexborElement* elem) {
        if (!elem) return;
        elementCount++;
        
        auto children = elem->GetChildren();
        for (auto* child : children) {
            traverse(child);
        }
    };
    
    traverse(root);
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    double avgPerElement = timer.GetElapsedUs() / elementCount;
    
    reporter_.AddResult("DOM Tree Traversal (total)", elapsed, "ms", "Traversal");
    reporter_.AddResult("DOM Tree Traversal (avg per element)", avgPerElement, "μs", "Traversal");
    
    std::cout << "DOM traversal: " << elapsed << " ms for " << elementCount << " elements\n";
    std::cout << "Avg per element: " << avgPerElement << " μs\n";
    
    EXPECT_LT(avgPerElement, 10.0) << "DOM traversal should be fast (< 10μs per element)";
}

// 测试批量查询性能
TEST_F(DOMPerformanceTest, BatchQuery) {
    std::vector<std::string> selectors = {
        ".section", ".list", ".item", "a", "li", "ul", "section", "h2"
    };
    
    PerformanceTimer timer;
    
    timer.Start();
    for (const auto& selector : selectors) {
        auto elements = doc_.QuerySelectorAll(selector);
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedMs() / selectors.size();
    
    reporter_.AddResult("Batch Query (8 selectors, avg)", avgTime, "ms", "Batch Operations");
    
    std::cout << "Batch query avg: " << avgTime << " ms per selector\n";
    
    EXPECT_LT(avgTime, 20.0) << "Batch query should be efficient (< 20ms per selector)";
}

// 测试大量元素的属性修改
TEST_F(DOMPerformanceTest, BulkAttributeModification) {
    auto elements = doc_.QuerySelectorAll("div");
    ASSERT_GT(elements.size(), 0);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    for (size_t i = 0; i < elements.size(); i++) {
        elements[i]->SetAttribute("data-index", std::to_string(i));
        elements[i]->SetAttribute("data-modified", "true");
        elements[i]->AddClass("modified");
    }
    
    timer.Stop();
    memory.Stop();
    
    double elapsed = timer.GetElapsedMs();
    double avgPerElement = timer.GetElapsedUs() / elements.size();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Bulk Modification (total)", elapsed, "ms", "Bulk Operations");
    reporter_.AddResult("Bulk Modification (avg per element)", avgPerElement, "μs", "Bulk Operations");
    reporter_.AddResult("Bulk Modification (memory)", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Bulk modification: " << elapsed << " ms for " << elements.size() << " elements\n";
    std::cout << "Avg per element: " << avgPerElement << " μs\n";
    std::cout << "Memory increase: " << memoryMB << " MB\n";
    
    EXPECT_LT(avgPerElement, 50.0) << "Bulk modification should be efficient (< 50μs per element)";
}

// 测试 innerHTML 读取性能
TEST_F(DOMPerformanceTest, InnerHTMLRead) {
    auto sections = doc_.QuerySelectorAll("section");
    ASSERT_GT(sections.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    for (auto* section : sections) {
        std::string html = section->GetInnerHTML();
    }
    timer.Stop();
    
    double avgTime = timer.GetElapsedMs() / sections.size();
    
    reporter_.AddResult("InnerHTML Read (avg per section)", avgTime, "ms", "HTML Operations");
    
    std::cout << "InnerHTML read avg: " << avgTime << " ms per section\n";
    
    EXPECT_LT(avgTime, 5.0) << "InnerHTML read should be reasonably fast (< 5ms)";
}

// 测试元素查找性能（不同方法对比）
TEST_F(DOMPerformanceTest, ElementLookupComparison) {
    PerformanceTimer timer1, timer2, timer3;
    
    // 通过 ID 查找
    timer1.Start();
    for (int i = 0; i < 100; i++) {
        auto elem = doc_.QuerySelector("#section50");
    }
    timer1.Stop();
    
    // 通过 class 查找
    timer2.Start();
    for (int i = 0; i < 100; i++) {
        auto elements = doc_.QuerySelectorAll(".section");
    }
    timer2.Stop();
    
    // 通过复杂选择器查找
    timer3.Start();
    for (int i = 0; i < 100; i++) {
        auto elements = doc_.QuerySelectorAll("section.section > ul.list");
    }
    timer3.Stop();
    
    double avgID = timer1.GetElapsedUs() / 100.0;
    double avgClass = timer2.GetElapsedMs() / 100.0;
    double avgComplex = timer3.GetElapsedMs() / 100.0;
    
    reporter_.AddResult("Lookup by ID (avg)", avgID, "μs", "Lookup Comparison");
    reporter_.AddResult("Lookup by Class (avg)", avgClass, "ms", "Lookup Comparison");
    reporter_.AddResult("Lookup by Complex Selector (avg)", avgComplex, "ms", "Lookup Comparison");
    
    std::cout << "Lookup by ID avg: " << avgID << " μs\n";
    std::cout << "Lookup by Class avg: " << avgClass << " ms\n";
    std::cout << "Lookup by Complex Selector avg: " << avgComplex << " ms\n";
    
    EXPECT_LT(avgID, 100.0) << "ID lookup should be very fast (< 100μs)";
    EXPECT_LT(avgClass, 10.0) << "Class lookup should be fast (< 10ms)";
    EXPECT_LT(avgComplex, 20.0) << "Complex selector lookup should be reasonable (< 20ms)";
}

// 测试缓存效果
TEST_F(DOMPerformanceTest, CachingEffect) {
    const std::string selector = ".item";
    
    PerformanceTimer timer;
    std::vector<double> times;
    
    // 运行 10 次，看是否有缓存效果
    for (int i = 0; i < 10; i++) {
        timer.Start();
        auto elements = doc_.QuerySelectorAll(selector);
        timer.Stop();
        times.push_back(timer.GetElapsedMs());
    }
    
    double firstTime = times[0];
    double avgLaterTime = 0.0;
    for (size_t i = 1; i < times.size(); i++) {
        avgLaterTime += times[i];
    }
    avgLaterTime /= (times.size() - 1);
    
    reporter_.AddResult("First Query", firstTime, "ms", "Caching Effect");
    reporter_.AddResult("Average Later Queries", avgLaterTime, "ms", "Caching Effect");
    
    std::cout << "First query: " << firstTime << " ms\n";
    std::cout << "Avg later queries: " << avgLaterTime << " ms\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

