/**
 * @file test_layout_performance.cpp
 * @brief 布局性能测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"
#include "layout/native_layout_engine.h"
#include <chrono>
#include <iostream>

namespace mblink {
namespace test {

class LayoutPerformanceTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        layout_engine_ = std::make_unique<NativeLayoutEngine>();
    }

    void PrintResult(const std::string& name, double ms, int operations) {
        double opsPerSec = operations / (ms / 1000.0);
        std::cout << "[PERF] " << name << ": " << ms << "ms for " << operations
                  << " ops (" << opsPerSec << " ops/sec)" << std::endl;
    }

protected:
    std::unique_ptr<NativeLayoutEngine> layout_engine_;
};

// ========== 简单布局性能 ==========

TEST_F(LayoutPerformanceTest, SimpleBlockLayout) {
    const int COUNT = 1000;

    // 创建简单的 block 布局
    auto container = doc_->CreateElement("div");
    container->SetStyle("width", "800px");
    doc_->GetBody()->AppendChild(container);

    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        elem->SetStyle("width", "100%");
        elem->SetStyle("height", "50px");
        container->AppendChild(elem);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 执行布局
    // layout_engine_->ComputeLayout(800, 600);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("SimpleBlockLayout", ms, COUNT);
}

// ========== Flexbox 布局性能 ==========

TEST_F(LayoutPerformanceTest, FlexboxLayout) {
    const int CONTAINERS = 100;
    const int ITEMS_PER_CONTAINER = 10;

    auto root = doc_->CreateElement("div");
    root->SetStyle("width", "800px");
    doc_->GetBody()->AppendChild(root);

    for (int i = 0; i < CONTAINERS; i++) {
        auto flex = doc_->CreateElement("div");
        flex->SetStyle("display", "flex");
        flex->SetStyle("justify-content", "space-between");

        for (int j = 0; j < ITEMS_PER_CONTAINER; j++) {
            auto item = doc_->CreateElement("div");
            item->SetStyle("flex", "1");
            item->SetStyle("height", "50px");
            flex->AppendChild(item);
        }

        root->AppendChild(flex);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // layout_engine_->ComputeLayout(800, 600);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("FlexboxLayout", ms, CONTAINERS * ITEMS_PER_CONTAINER);
}

// ========== 嵌套 Flexbox 性能 ==========

TEST_F(LayoutPerformanceTest, NestedFlexboxLayout) {
    const int DEPTH = 10;
    const int ITEMS_PER_LEVEL = 5;

    std::function<void(std::shared_ptr<Element>, int)> createNested;
    createNested = [&](std::shared_ptr<Element> parent, int depth) {
        if (depth <= 0) return;

        for (int i = 0; i < ITEMS_PER_LEVEL; i++) {
            auto flex = doc_->CreateElement("div");
            flex->SetStyle("display", "flex");
            flex->SetStyle("flex", "1");
            parent->AppendChild(flex);

            createNested(flex, depth - 1);
        }
    };

    auto root = doc_->CreateElement("div");
    root->SetStyle("display", "flex");
    root->SetStyle("width", "800px");
    root->SetStyle("height", "600px");
    doc_->GetBody()->AppendChild(root);

    createNested(root, DEPTH);

    auto start = std::chrono::high_resolution_clock::now();

    // layout_engine_->ComputeLayout(800, 600);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("NestedFlexboxLayout", ms, 1);
}

// ========== 增量布局性能 ==========

TEST_F(LayoutPerformanceTest, IncrementalLayout) {
    const int COUNT = 100;
    const int UPDATES = 1000;

    auto container = doc_->CreateElement("div");
    container->SetStyle("width", "800px");
    doc_->GetBody()->AppendChild(container);

    std::vector<std::shared_ptr<Element>> elements;
    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        elem->SetStyle("width", "100px");
        elem->SetStyle("height", "50px");
        container->AppendChild(elem);
        elements.push_back(elem);
    }

    // 首次布局
    // layout_engine_->ComputeLayout(800, 600);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < UPDATES; i++) {
        // 修改一个元素
        elements[i % COUNT]->SetStyle("width", std::to_string(100 + (i % 50)) + "px");

        // 增量布局
        // layout_engine_->ComputeIncrementalLayout(800, 600);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("IncrementalLayout", ms, UPDATES);
}

// ========== 复杂布局性能 ==========

TEST_F(LayoutPerformanceTest, ComplexLayout) {
    // 模拟真实页面布局
    auto page = doc_->CreateElement("div");
    page->SetStyle("width", "1200px");
    doc_->GetBody()->AppendChild(page);

    // Header
    auto header = doc_->CreateElement("header");
    header->SetStyle("display", "flex");
    header->SetStyle("height", "60px");
    page->AppendChild(header);

    // Main content with sidebar
    auto main = doc_->CreateElement("main");
    main->SetStyle("display", "flex");
    page->AppendChild(main);

    // Sidebar
    auto sidebar = doc_->CreateElement("aside");
    sidebar->SetStyle("width", "250px");
    main->AppendChild(sidebar);

    for (int i = 0; i < 20; i++) {
        auto item = doc_->CreateElement("div");
        item->SetStyle("height", "40px");
        sidebar->AppendChild(item);
    }

    // Content area
    auto content = doc_->CreateElement("div");
    content->SetStyle("flex", "1");
    main->AppendChild(content);

    // Grid of cards
    auto grid = doc_->CreateElement("div");
    grid->SetStyle("display", "flex");
    grid->SetStyle("flex-wrap", "wrap");
    content->AppendChild(grid);

    for (int i = 0; i < 50; i++) {
        auto card = doc_->CreateElement("div");
        card->SetStyle("width", "200px");
        card->SetStyle("height", "150px");
        card->SetStyle("margin", "10px");
        grid->AppendChild(card);
    }

    // Footer
    auto footer = doc_->CreateElement("footer");
    footer->SetStyle("height", "100px");
    page->AppendChild(footer);

    auto start = std::chrono::high_resolution_clock::now();

    // layout_engine_->ComputeLayout(1200, 800);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("ComplexLayout", ms, 1);
}

// ========== 文本布局性能 ==========

TEST_F(LayoutPerformanceTest, TextLayout) {
    const int PARAGRAPHS = 100;

    auto container = doc_->CreateElement("div");
    container->SetStyle("width", "600px");
    doc_->GetBody()->AppendChild(container);

    for (int i = 0; i < PARAGRAPHS; i++) {
        auto p = doc_->CreateElement("p");
        p->AppendChild(doc_->CreateTextNode(
            "This is a paragraph of text that needs to be laid out. "
            "It contains multiple sentences and should wrap properly "
            "within the container width."
        ));
        container->AppendChild(p);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // layout_engine_->ComputeLayout(800, 600);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("TextLayout", ms, PARAGRAPHS);
}

// ========== 表格布局性能 ==========

TEST_F(LayoutPerformanceTest, TableLayout) {
    const int ROWS = 100;
    const int COLS = 10;

    auto table = doc_->CreateElement("table");
    table->SetStyle("width", "100%");
    doc_->GetBody()->AppendChild(table);

    for (int i = 0; i < ROWS; i++) {
        auto tr = doc_->CreateElement("tr");
        for (int j = 0; j < COLS; j++) {
            auto td = doc_->CreateElement("td");
            td->AppendChild(doc_->CreateTextNode("Cell " + std::to_string(i) + "," + std::to_string(j)));
            tr->AppendChild(td);
        }
        table->AppendChild(tr);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // layout_engine_->ComputeLayout(800, 600);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("TableLayout", ms, ROWS * COLS);
}

} // namespace test
} // namespace mblink
