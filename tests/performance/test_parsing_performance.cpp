#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "performance_utils.h"
#include <iostream>

using namespace lightui;
using namespace mbink::performance;

class ParsingPerformanceTest : public ::testing::Test {
protected:
    PerformanceReporter reporter_;
    
    void SetUp() override {
        std::cout << "\n=== Parsing Performance Tests ===\n";
    }
    
    void TearDown() override {
        reporter_.PrintSummary();
        reporter_.GenerateReport("docs/parsing_performance_report.md");
        reporter_.GenerateCSV("docs/parsing_performance_data.csv");
    }
};

// 测试小规模 HTML 解析（1K 元素）
TEST_F(ParsingPerformanceTest, Parse1KElements) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(1000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("1K Elements - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("1K Elements - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "1K elements: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    // 目标: < 10ms
    EXPECT_LT(elapsed, 10.0) << "Parsing 1K elements should take less than 10ms";
}

// 测试中等规模 HTML 解析（5K 元素）
TEST_F(ParsingPerformanceTest, Parse5KElements) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(5000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("5K Elements - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("5K Elements - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "5K elements: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    // 目标: < 50ms
    EXPECT_LT(elapsed, 50.0) << "Parsing 5K elements should take less than 50ms";
}

// 测试大规模 HTML 解析（10K 元素）
TEST_F(ParsingPerformanceTest, Parse10KElements) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(10000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("10K Elements - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("10K Elements - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "10K elements: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    // 目标: < 100ms
    EXPECT_LT(elapsed, 100.0) << "Parsing 10K elements should take less than 100ms";
}

// 测试超大规模 HTML 解析（50K 元素）
TEST_F(ParsingPerformanceTest, Parse50KElements) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(50000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("50K Elements - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("50K Elements - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "50K elements: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    // 目标: < 500ms
    EXPECT_LT(elapsed, 500.0) << "Parsing 50K elements should take less than 500ms";
}

// 测试嵌套 HTML 解析
TEST_F(ParsingPerformanceTest, ParseNestedHTML) {
    // 深度 10，宽度 5 = 约 10K 元素
    std::string html = HTMLGenerator::GenerateNestedHTML(10, 5);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Nested HTML (depth=10) - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("Nested HTML (depth=10) - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Nested HTML: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    // 嵌套结构可能稍慢
    EXPECT_LT(elapsed, 150.0) << "Parsing nested HTML should take less than 150ms";
}

// 测试表单 HTML 解析
TEST_F(ParsingPerformanceTest, ParseFormHTML) {
    std::string html = HTMLGenerator::GenerateFormHTML(1000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Form HTML (1000 inputs) - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("Form HTML (1000 inputs) - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Form HTML: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    EXPECT_LT(elapsed, 20.0) << "Parsing form HTML should take less than 20ms";
}

// 测试表格 HTML 解析
TEST_F(ParsingPerformanceTest, ParseTableHTML) {
    std::string html = HTMLGenerator::GenerateTableHTML(100, 10); // 1000 cells
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Table HTML (100x10) - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("Table HTML (100x10) - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Table HTML: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    EXPECT_LT(elapsed, 20.0) << "Parsing table HTML should take less than 20ms";
}

// 测试复杂 HTML 解析
TEST_F(ParsingPerformanceTest, ParseComplexHTML) {
    std::string html = HTMLGenerator::GenerateComplexHTML(5000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Complex HTML (5K elements) - Parse Time", elapsed, "ms", "Parsing Performance");
    reporter_.AddResult("Complex HTML (5K elements) - Memory", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Complex HTML: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    EXPECT_LT(elapsed, 60.0) << "Parsing complex HTML should take less than 60ms";
}

// 测试重复解析（检查内存泄漏）
TEST_F(ParsingPerformanceTest, RepeatedParsing) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(1000);
    
    MemoryMonitor memory;
    memory.Start();
    
    size_t initialMemory = memory.GetCurrentMemoryUsage();
    
    // 解析 100 次
    for (int i = 0; i < 100; i++) {
        LexborDocument doc;
        doc.ParseHTML(html);
    }
    
    memory.Stop();
    
    size_t finalMemory = memory.GetCurrentMemoryUsage();
    double memoryIncreaseMB = (finalMemory - initialMemory) / (1024.0 * 1024.0);
    
    reporter_.AddResult("Repeated Parsing (100x) - Memory Increase", 
                       memoryIncreaseMB, "MB", "Memory Leak Test");
    
    std::cout << "Repeated parsing memory increase: " << memoryIncreaseMB << " MB\n";
    
    // 内存增长应该很小（< 10MB）
    EXPECT_LT(memoryIncreaseMB, 10.0) << "Repeated parsing should not leak significant memory";
}

// 测试并发解析性能
TEST_F(ParsingPerformanceTest, AverageParsingTime) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(5000);
    
    const int iterations = 10;
    double totalTime = 0.0;
    
    for (int i = 0; i < iterations; i++) {
        PerformanceTimer timer;
        timer.Start();
        
        LexborDocument doc;
        doc.ParseHTML(html);
        
        timer.Stop();
        totalTime += timer.GetElapsedMs();
    }
    
    double avgTime = totalTime / iterations;
    
    reporter_.AddResult("Average Parse Time (5K, 10 runs)", avgTime, "ms", "Parsing Performance");
    
    std::cout << "Average parsing time (10 runs): " << avgTime << " ms\n";
    
    EXPECT_LT(avgTime, 50.0) << "Average parsing time should be less than 50ms";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

