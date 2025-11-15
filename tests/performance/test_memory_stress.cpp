#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "performance_utils.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace lightui;
using namespace mbink::performance;

class MemoryStressTest : public ::testing::Test {
protected:
    PerformanceReporter reporter_;
    
    void SetUp() override {
        std::cout << "\n=== Memory Stress Tests ===\n";
    }
    
    void TearDown() override {
        reporter_.PrintSummary();
        reporter_.GenerateReport("docs/memory_stress_report.md");
        reporter_.GenerateCSV("docs/memory_stress_data.csv");
    }
};

// 测试重复解析的内存泄漏
TEST_F(MemoryStressTest, RepeatedParsingMemoryLeak) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(5000);
    
    MemoryMonitor memory;
    memory.Start();
    
    size_t initialMemory = memory.GetCurrentMemoryUsage();
    
    // 解析 1000 次
    for (int i = 0; i < 1000; i++) {
        LexborDocument doc;
        doc.ParseHTML(html);
        
        // 每 100 次检查一次内存
        if (i % 100 == 99) {
            size_t currentMemory = memory.GetCurrentMemoryUsage();
            double increaseMB = (currentMemory - initialMemory) / (1024.0 * 1024.0);
            std::cout << "After " << (i + 1) << " iterations: " << increaseMB << " MB increase\n";
        }
    }
    
    memory.Stop();
    
    size_t finalMemory = memory.GetCurrentMemoryUsage();
    double memoryIncreaseMB = (finalMemory - initialMemory) / (1024.0 * 1024.0);
    
    reporter_.AddResult("Repeated Parsing (1000x) - Memory Increase", 
                       memoryIncreaseMB, "MB", "Memory Leak Test");
    
    std::cout << "Total memory increase after 1000 parses: " << memoryIncreaseMB << " MB\n";
    
    // 内存增长应该很小（< 50MB）
    EXPECT_LT(memoryIncreaseMB, 50.0) 
        << "Repeated parsing should not leak significant memory";
}

// 测试大文档的内存占用
TEST_F(MemoryStressTest, LargeDocumentMemory) {
    std::vector<size_t> sizes = {1000, 5000, 10000, 50000};
    
    for (size_t size : sizes) {
        std::string html = HTMLGenerator::GenerateSimpleHTML(size);
        
        MemoryMonitor memory;
        memory.Start();
        
        size_t beforeParse = memory.GetCurrentMemoryUsage();
        
        LexborDocument doc;
        doc.ParseHTML(html);
        
        size_t afterParse = memory.GetCurrentMemoryUsage();
        memory.Stop();
        
        double memoryUsedMB = (afterParse - beforeParse) / (1024.0 * 1024.0);
        double bytesPerElement = (afterParse - beforeParse) / static_cast<double>(size);
        
        std::string testName = "Document Memory (" + std::to_string(size) + " elements)";
        reporter_.AddResult(testName, memoryUsedMB, "MB", "Document Memory");
        
        std::string perElemName = "Memory per Element (" + std::to_string(size) + " elements)";
        reporter_.AddResult(perElemName, bytesPerElement, "bytes", "Memory Efficiency");
        
        std::cout << size << " elements: " << memoryUsedMB << " MB (" 
                  << bytesPerElement << " bytes/element)\n";
    }
}

// 测试长时间运行的内存稳定性
TEST_F(MemoryStressTest, LongRunningStability) {
    std::string html = HTMLGenerator::GenerateComplexHTML(1000);
    
    MemoryMonitor memory;
    memory.Start();
    
    size_t initialMemory = memory.GetCurrentMemoryUsage();
    std::vector<double> memorySnapshots;
    
    // 运行 100 次迭代，每次都进行多种操作
    for (int i = 0; i < 100; i++) {
        LexborDocument doc;
        doc.ParseHTML(html);
        
        // 执行各种操作
        auto elements = doc.QuerySelectorAll("div");
        for (auto* elem : elements) {
            elem->GetAttribute("class");
            elem->SetAttribute("data-iter", std::to_string(i));
            elem->GetTextContent();
        }
        
        auto sections = doc.QuerySelectorAll("section");
        for (auto* section : sections) {
            auto items = section->QuerySelectorAll("li");
        }
        
        // 记录内存快照
        if (i % 10 == 9) {
            size_t currentMemory = memory.GetCurrentMemoryUsage();
            double increaseMB = (currentMemory - initialMemory) / (1024.0 * 1024.0);
            memorySnapshots.push_back(increaseMB);
            std::cout << "Iteration " << (i + 1) << ": " << increaseMB << " MB increase\n";
        }
    }
    
    memory.Stop();
    
    // 计算内存增长趋势
    double avgIncrease = 0.0;
    for (double snapshot : memorySnapshots) {
        avgIncrease += snapshot;
    }
    avgIncrease /= memorySnapshots.size();
    
    reporter_.AddResult("Long Running (100 iterations) - Avg Memory Increase", 
                       avgIncrease, "MB", "Stability Test");
    
    std::cout << "Average memory increase: " << avgIncrease << " MB\n";
    
    // 平均内存增长应该稳定且较小
    EXPECT_LT(avgIncrease, 30.0) << "Long running should maintain stable memory usage";
}

// 测试嵌套文档的内存占用
TEST_F(MemoryStressTest, NestedDocumentMemory) {
    std::vector<std::pair<size_t, size_t>> configs = {
        {5, 5},    // 深度 5，宽度 5
        {10, 3},   // 深度 10，宽度 3
        {15, 2},   // 深度 15，宽度 2
    };
    
    for (const auto& config : configs) {
        size_t depth = config.first;
        size_t width = config.second;
        
        std::string html = HTMLGenerator::GenerateNestedHTML(depth, width);
        
        MemoryMonitor memory;
        memory.Start();
        
        size_t beforeParse = memory.GetCurrentMemoryUsage();
        
        LexborDocument doc;
        doc.ParseHTML(html);
        
        size_t afterParse = memory.GetCurrentMemoryUsage();
        memory.Stop();
        
        double memoryUsedMB = (afterParse - beforeParse) / (1024.0 * 1024.0);
        
        std::string testName = "Nested Document (depth=" + std::to_string(depth) + 
                              ", width=" + std::to_string(width) + ")";
        reporter_.AddResult(testName, memoryUsedMB, "MB", "Nested Document Memory");
        
        std::cout << "Depth " << depth << ", Width " << width << ": " 
                  << memoryUsedMB << " MB\n";
    }
}

// 测试查询操作的内存影响
TEST_F(MemoryStressTest, QueryMemoryImpact) {
    std::string html = HTMLGenerator::GenerateComplexHTML(10000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    MemoryMonitor memory;
    memory.Start();
    
    size_t beforeQueries = memory.GetCurrentMemoryUsage();
    
    // 执行大量查询
    for (int i = 0; i < 1000; i++) {
        auto elements1 = doc.QuerySelectorAll(".section");
        auto elements2 = doc.QuerySelectorAll(".list");
        auto elements3 = doc.QuerySelectorAll(".item");
        auto elements4 = doc.QuerySelectorAll("a");
    }
    
    size_t afterQueries = memory.GetCurrentMemoryUsage();
    memory.Stop();
    
    double memoryIncreaseMB = (afterQueries - beforeQueries) / (1024.0 * 1024.0);
    
    reporter_.AddResult("Query Operations (4000 queries) - Memory Increase", 
                       memoryIncreaseMB, "MB", "Query Memory Impact");
    
    std::cout << "Memory increase from 4000 queries: " << memoryIncreaseMB << " MB\n";
    
    // 查询操作不应该显著增加内存
    EXPECT_LT(memoryIncreaseMB, 20.0) 
        << "Query operations should not significantly increase memory";
}

// 测试 DOM 修改的内存影响
TEST_F(MemoryStressTest, DOMModificationMemory) {
    std::string html = HTMLGenerator::GenerateSimpleHTML(5000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    MemoryMonitor memory;
    memory.Start();
    
    size_t beforeMod = memory.GetCurrentMemoryUsage();
    
    // 大量 DOM 修改
    auto elements = doc.QuerySelectorAll("div");
    for (size_t i = 0; i < elements.size(); i++) {
        elements[i]->SetAttribute("data-index", std::to_string(i));
        elements[i]->SetAttribute("data-modified", "true");
        elements[i]->SetAttribute("data-timestamp", std::to_string(i * 1000));
        elements[i]->AddClass("modified");
        elements[i]->AddClass("processed");
        elements[i]->SetTextContent("Modified content " + std::to_string(i));
    }
    
    size_t afterMod = memory.GetCurrentMemoryUsage();
    memory.Stop();
    
    double memoryIncreaseMB = (afterMod - beforeMod) / (1024.0 * 1024.0);
    double bytesPerElement = (afterMod - beforeMod) / static_cast<double>(elements.size());
    
    reporter_.AddResult("DOM Modification (5000 elements) - Memory Increase", 
                       memoryIncreaseMB, "MB", "Modification Memory");
    reporter_.AddResult("DOM Modification - Bytes per Element", 
                       bytesPerElement, "bytes", "Modification Memory");
    
    std::cout << "Memory increase from modifications: " << memoryIncreaseMB << " MB\n";
    std::cout << "Bytes per element: " << bytesPerElement << " bytes\n";
}

// 测试峰值内存使用
TEST_F(MemoryStressTest, PeakMemoryUsage) {
    MemoryMonitor memory;
    memory.Start();
    
    size_t initialMemory = memory.GetCurrentMemoryUsage();
    size_t peakMemory = initialMemory;
    
    // 创建多个大文档
    for (int i = 0; i < 10; i++) {
        std::string html = HTMLGenerator::GenerateComplexHTML(10000);
        LexborDocument doc;
        doc.ParseHTML(html);
        
        auto elements = doc.QuerySelectorAll("div");
        for (auto* elem : elements) {
            elem->GetAttribute("class");
        }
        
        size_t currentMemory = memory.GetCurrentMemoryUsage();
        if (currentMemory > peakMemory) {
            peakMemory = currentMemory;
        }
        
        std::cout << "Iteration " << (i + 1) << ": " 
                  << (currentMemory - initialMemory) / (1024.0 * 1024.0) << " MB\n";
    }
    
    memory.Stop();
    
    double peakIncreaseMB = (peakMemory - initialMemory) / (1024.0 * 1024.0);
    double finalIncreaseMB = (memory.GetCurrentMemoryUsage() - initialMemory) / (1024.0 * 1024.0);
    
    reporter_.AddResult("Peak Memory Increase", peakIncreaseMB, "MB", "Peak Memory");
    reporter_.AddResult("Final Memory Increase", finalIncreaseMB, "MB", "Peak Memory");
    
    std::cout << "Peak memory increase: " << peakIncreaseMB << " MB\n";
    std::cout << "Final memory increase: " << finalIncreaseMB << " MB\n";
    
    // 最终内存应该回到合理水平
    EXPECT_LT(finalIncreaseMB, peakIncreaseMB * 0.5) 
        << "Memory should be released after document destruction";
}

// 测试并发文档的内存使用
TEST_F(MemoryStressTest, ConcurrentDocumentsMemory) {
    MemoryMonitor memory;
    memory.Start();
    
    size_t beforeDocs = memory.GetCurrentMemoryUsage();
    
    // 创建 100 个并发文档
    std::vector<std::unique_ptr<LexborDocument>> documents;
    for (int i = 0; i < 100; i++) {
        std::string html = HTMLGenerator::GenerateSimpleHTML(1000);
        auto doc = std::make_unique<LexborDocument>();
        doc->ParseHTML(html);
        documents.push_back(std::move(doc));
    }
    
    size_t afterDocs = memory.GetCurrentMemoryUsage();
    
    double memoryUsedMB = (afterDocs - beforeDocs) / (1024.0 * 1024.0);
    double memoryPerDocMB = memoryUsedMB / 100.0;
    
    reporter_.AddResult("100 Concurrent Documents - Total Memory", 
                       memoryUsedMB, "MB", "Concurrent Documents");
    reporter_.AddResult("100 Concurrent Documents - Per Document", 
                       memoryPerDocMB, "MB", "Concurrent Documents");
    
    std::cout << "100 concurrent documents: " << memoryUsedMB << " MB total, " 
              << memoryPerDocMB << " MB per document\n";
    
    // 清理文档
    documents.clear();
    
    size_t afterCleanup = memory.GetCurrentMemoryUsage();
    memory.Stop();
    
    double memoryReleasedMB = (afterDocs - afterCleanup) / (1024.0 * 1024.0);
    
    reporter_.AddResult("Memory Released After Cleanup", 
                       memoryReleasedMB, "MB", "Concurrent Documents");
    
    std::cout << "Memory released after cleanup: " << memoryReleasedMB << " MB\n";
    
    // 大部分内存应该被释放
    EXPECT_GT(memoryReleasedMB, memoryUsedMB * 0.8) 
        << "Most memory should be released after document cleanup";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

