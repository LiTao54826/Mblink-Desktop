/**
 * @file test_dom_performance.cpp
 * @brief DOM 性能测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"
#include <chrono>
#include <iostream>

namespace mblink {
namespace test {

class DOMPerformanceTest : public DOMTestBase {
protected:
    void PrintResult(const std::string& name, double ms, int operations) {
        double opsPerSec = operations / (ms / 1000.0);
        std::cout << "[PERF] " << name << ": " << ms << "ms for " << operations
                  << " ops (" << opsPerSec << " ops/sec)" << std::endl;
    }
};

// ========== 元素创建性能 ==========

TEST_F(DOMPerformanceTest, CreateElements) {
    const int COUNT = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("CreateElement", ms, COUNT);

    // 性能基准：应该在合理时间内完成
    EXPECT_LT(ms, 1000);  // 小于 1 秒
}

// ========== 元素添加性能 ==========

TEST_F(DOMPerformanceTest, AppendChildren) {
    const int COUNT = 10000;
    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        container->AppendChild(elem);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("AppendChild", ms, COUNT);

    EXPECT_LT(ms, 2000);
}

// ========== 属性操作性能 ==========

TEST_F(DOMPerformanceTest, SetAttributes) {
    const int COUNT = 100000;
    auto elem = doc_->CreateElement("div");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        elem->SetAttribute("data-index", std::to_string(i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("SetAttribute", ms, COUNT);

    EXPECT_LT(ms, 1000);
}

TEST_F(DOMPerformanceTest, GetAttributes) {
    const int COUNT = 100000;
    auto elem = doc_->CreateElement("div");
    elem->SetAttribute("data-value", "test");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        auto value = elem->GetAttribute("data-value");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("GetAttribute", ms, COUNT);

    EXPECT_LT(ms, 500);
}

// ========== 类操作性能 ==========

TEST_F(DOMPerformanceTest, ClassOperations) {
    const int COUNT = 100000;
    auto elem = doc_->CreateElement("div");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        elem->AddClass("class-" + std::to_string(i % 10));
        elem->HasClass("class-5");
        elem->RemoveClass("class-" + std::to_string(i % 10));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("ClassOperations", ms, COUNT * 3);

    EXPECT_LT(ms, 2000);
}

// ========== 查询选择器性能 ==========

TEST_F(DOMPerformanceTest, QuerySelectorById) {
    // 创建大量元素
    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    for (int i = 0; i < 1000; i++) {
        auto elem = doc_->CreateElement("div");
        elem->SetAttribute("id", "elem-" + std::to_string(i));
        container->AppendChild(elem);
    }

    const int QUERIES = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < QUERIES; i++) {
        auto elem = doc_->GetElementById("elem-" + std::to_string(i % 1000));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("GetElementById", ms, QUERIES);

    EXPECT_LT(ms, 1000);
}

TEST_F(DOMPerformanceTest, QuerySelectorByClass) {
    // 创建大量元素
    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    for (int i = 0; i < 1000; i++) {
        auto elem = doc_->CreateElement("div");
        elem->AddClass("item");
        elem->AddClass("item-" + std::to_string(i % 10));
        container->AppendChild(elem);
    }

    const int QUERIES = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < QUERIES; i++) {
        auto elems = container->QuerySelectorAll(".item");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("QuerySelectorAll(.item)", ms, QUERIES);

    EXPECT_LT(ms, 5000);
}

// ========== 深度嵌套性能 ==========

TEST_F(DOMPerformanceTest, DeepNesting) {
    const int DEPTH = 100;

    auto start = std::chrono::high_resolution_clock::now();

    auto current = doc_->GetBody();
    for (int i = 0; i < DEPTH; i++) {
        auto child = doc_->CreateElement("div");
        current->AppendChild(child);
        current = child;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("DeepNesting", ms, DEPTH);

    EXPECT_LT(ms, 100);
}

// ========== innerHTML 性能 ==========

TEST_F(DOMPerformanceTest, SetInnerHTML) {
    const int COUNT = 1000;
    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    std::string html = "<div><span>Item</span></div>";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        container->SetInnerHTML(html);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("SetInnerHTML", ms, COUNT);

    EXPECT_LT(ms, 2000);
}

// ========== 事件监听器性能 ==========

TEST_F(DOMPerformanceTest, AddEventListeners) {
    const int COUNT = 10000;
    auto elem = doc_->CreateElement("div");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        elem->AddEventListener("click", [](std::shared_ptr<Event> e) {});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("AddEventListener", ms, COUNT);

    EXPECT_LT(ms, 1000);
}

// ========== 批量更新性能 ==========

TEST_F(DOMPerformanceTest, BatchUpdate) {
    const int COUNT = 10000;
    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    // 不使用批量更新
    auto start1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        container->AppendChild(elem);
    }

    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    double ms1 = duration1.count() / 1000.0;

    // 清空
    container->SetInnerHTML("");

    // 使用批量更新
    auto start2 = std::chrono::high_resolution_clock::now();

    doc_->BeginBatch();
    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        container->AppendChild(elem);
    }
    doc_->EndBatch();

    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    double ms2 = duration2.count() / 1000.0;

    PrintResult("WithoutBatch", ms1, COUNT);
    PrintResult("WithBatch", ms2, COUNT);

    // 批量更新应该更快或至少相当
    EXPECT_LE(ms2, ms1 * 1.5);
}

// ========== 内存压力测试 ==========

TEST_F(DOMPerformanceTest, MemoryStress) {
    const int ITERATIONS = 100;
    const int ELEMENTS_PER_ITERATION = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        auto container = doc_->CreateElement("div");
        doc_->GetBody()->AppendChild(container);

        for (int i = 0; i < ELEMENTS_PER_ITERATION; i++) {
            auto elem = doc_->CreateElement("div");
            elem->SetAttribute("data-index", std::to_string(i));
            elem->AddClass("item");
            container->AppendChild(elem);
        }

        // 移除容器
        doc_->GetBody()->RemoveChild(container);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("MemoryStress", ms, ITERATIONS * ELEMENTS_PER_ITERATION);

    EXPECT_LT(ms, 10000);
}

} // namespace test
} // namespace mblink
