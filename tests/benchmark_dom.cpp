#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/event.h"

using namespace lightui;
using namespace std::chrono;

// 基准测试辅助类
class BenchmarkTimer {
public:
    BenchmarkTimer(const std::string& name) : name_(name) {
        start_ = high_resolution_clock::now();
    }
    
    ~BenchmarkTimer() {
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start_).count();
        std::cout << std::setw(40) << std::left << name_ 
                  << ": " << std::setw(10) << std::right << duration << " μs" << std::endl;
    }
    
private:
    std::string name_;
    high_resolution_clock::time_point start_;
};

// 基准测试 fixture
class DOMBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        doc = std::make_shared<Document>();
    }
    
    std::shared_ptr<Document> doc;
};

// ========== 节点创建基准测试 ==========

TEST_F(DOMBenchmark, CreateElements) {
    const int COUNT = 10000;
    
    {
        BenchmarkTimer timer("Create 10k elements");
        for (int i = 0; i < COUNT; ++i) {
            auto elem = doc->CreateElement("div");
        }
    }
}

TEST_F(DOMBenchmark, CreateTextNodes) {
    const int COUNT = 10000;
    
    {
        BenchmarkTimer timer("Create 10k text nodes");
        for (int i = 0; i < COUNT; ++i) {
            auto text = doc->CreateTextNode("Hello World");
        }
    }
}

// ========== DOM 树构建基准测试 ==========

TEST_F(DOMBenchmark, BuildDeepTree) {
    const int DEPTH = 100;
    
    {
        BenchmarkTimer timer("Build tree depth 100");
        auto current = doc->CreateElement("div");
        for (int i = 0; i < DEPTH; ++i) {
            auto child = doc->CreateElement("div");
            current->AppendChild(child);
            current = child;
        }
    }
}

TEST_F(DOMBenchmark, BuildWideTree) {
    const int WIDTH = 1000;
    
    {
        BenchmarkTimer timer("Build tree width 1000");
        auto root = doc->CreateElement("div");
        for (int i = 0; i < WIDTH; ++i) {
            auto child = doc->CreateElement("div");
            root->AppendChild(child);
        }
    }
}

TEST_F(DOMBenchmark, BuildComplexTree) {
    const int ROWS = 100;
    const int COLS = 10;
    
    {
        BenchmarkTimer timer("Build 100x10 grid");
        auto container = doc->CreateElement("div");
        for (int i = 0; i < ROWS; ++i) {
            auto row = doc->CreateElement("div");
            for (int j = 0; j < COLS; ++j) {
                auto cell = doc->CreateElement("div");
                row->AppendChild(cell);
            }
            container->AppendChild(row);
        }
    }
}

// ========== 属性操作基准测试 ==========

TEST_F(DOMBenchmark, SetAttributes) {
    const int COUNT = 10000;
    auto elem = doc->CreateElement("div");
    
    {
        BenchmarkTimer timer("Set 10k attributes");
        for (int i = 0; i < COUNT; ++i) {
            elem->SetAttribute("attr" + std::to_string(i), "value" + std::to_string(i));
        }
    }
}

TEST_F(DOMBenchmark, GetAttributes) {
    const int COUNT = 10000;
    auto elem = doc->CreateElement("div");
    
    // 预先设置属性
    for (int i = 0; i < COUNT; ++i) {
        elem->SetAttribute("attr" + std::to_string(i), "value" + std::to_string(i));
    }
    
    {
        BenchmarkTimer timer("Get 10k attributes");
        for (int i = 0; i < COUNT; ++i) {
            auto value = elem->GetAttribute("attr" + std::to_string(i));
        }
    }
}

// ========== ID 查询基准测试 ==========

TEST_F(DOMBenchmark, GetElementById) {
    const int COUNT = 1000;
    
    // 创建 1000 个带 ID 的元素
    for (int i = 0; i < COUNT; ++i) {
        auto elem = doc->CreateElement("div");
        elem->SetAttribute("id", "elem" + std::to_string(i));
    }
    
    {
        BenchmarkTimer timer("GetElementById 1k times");
        for (int i = 0; i < COUNT; ++i) {
            auto elem = doc->GetElementById("elem" + std::to_string(i));
        }
    }
}

// ========== 查询选择器基准测试 ==========

TEST_F(DOMBenchmark, QuerySelectorById) {
    const int COUNT = 100;
    
    // 创建树结构
    auto root = doc->CreateElement("div");
    for (int i = 0; i < COUNT; ++i) {
        auto elem = doc->CreateElement("div");
        elem->SetAttribute("id", "elem" + std::to_string(i));
        root->AppendChild(elem);
    }
    
    {
        BenchmarkTimer timer("QuerySelector by ID 100 times");
        for (int i = 0; i < COUNT; ++i) {
            auto elem = root->QuerySelector("#elem" + std::to_string(i));
        }
    }
}

TEST_F(DOMBenchmark, QuerySelectorByClass) {
    const int COUNT = 100;
    
    // 创建树结构
    auto root = doc->CreateElement("div");
    for (int i = 0; i < COUNT; ++i) {
        auto elem = doc->CreateElement("div");
        elem->SetAttribute("class", "item");
        root->AppendChild(elem);
    }
    
    {
        BenchmarkTimer timer("QuerySelector by class 100 times");
        for (int i = 0; i < COUNT; ++i) {
            auto elem = root->QuerySelector(".item");
        }
    }
}

TEST_F(DOMBenchmark, QuerySelectorAll) {
    const int COUNT = 100;
    
    // 创建树结构
    auto root = doc->CreateElement("div");
    for (int i = 0; i < COUNT; ++i) {
        auto elem = doc->CreateElement("div");
        elem->SetAttribute("class", "item");
        root->AppendChild(elem);
    }
    
    {
        BenchmarkTimer timer("QuerySelectorAll 100 items");
        auto results = root->QuerySelectorAll(".item");
        EXPECT_EQ(results.size(), COUNT);
    }
}

// ========== 事件系统基准测试 ==========

TEST_F(DOMBenchmark, AddEventListeners) {
    const int COUNT = 1000;
    auto elem = doc->CreateElement("div");
    
    {
        BenchmarkTimer timer("Add 1k event listeners");
        for (int i = 0; i < COUNT; ++i) {
            elem->AddEventListener("click", [](std::shared_ptr<Event> e) {
                // Empty handler
            });
        }
    }
}

TEST_F(DOMBenchmark, DispatchEvents) {
    const int COUNT = 1000;
    auto elem = doc->CreateElement("div");
    
    // 添加一个监听器
    elem->AddEventListener("click", [](std::shared_ptr<Event> e) {
        // Empty handler
    });
    
    {
        BenchmarkTimer timer("Dispatch 1k events");
        for (int i = 0; i < COUNT; ++i) {
            auto event = std::make_shared<Event>("click");
            elem->DispatchEvent(event);
        }
    }
}

TEST_F(DOMBenchmark, EventBubbling) {
    const int DEPTH = 10;
    const int COUNT = 100;
    
    // 创建深度为 10 的树
    auto root = doc->CreateElement("div");
    auto current = root;
    for (int i = 0; i < DEPTH; ++i) {
        auto child = doc->CreateElement("div");
        current->AppendChild(child);
        
        // 每个节点添加监听器
        current->AddEventListener("click", [](std::shared_ptr<Event> e) {
            // Empty handler
        });
        
        current = child;
    }
    
    {
        BenchmarkTimer timer("Event bubbling depth 10, 100 times");
        for (int i = 0; i < COUNT; ++i) {
            auto event = std::make_shared<Event>("click", true);
            current->DispatchEvent(event);
        }
    }
}

// ========== 克隆基准测试 ==========

TEST_F(DOMBenchmark, CloneShallowElement) {
    const int COUNT = 1000;
    auto elem = doc->CreateElement("div");
    elem->SetAttribute("id", "test");
    elem->SetAttribute("class", "item");
    
    {
        BenchmarkTimer timer("Clone shallow 1k times");
        for (int i = 0; i < COUNT; ++i) {
            auto clone = elem->CloneNode(false);
        }
    }
}

TEST_F(DOMBenchmark, CloneDeepTree) {
    const int COUNT = 100;
    
    // 创建一个复杂的树
    auto root = doc->CreateElement("div");
    for (int i = 0; i < 10; ++i) {
        auto child = doc->CreateElement("div");
        for (int j = 0; j < 10; ++j) {
            auto grandchild = doc->CreateElement("span");
            grandchild->AppendChild(doc->CreateTextNode("Text"));
            child->AppendChild(grandchild);
        }
        root->AppendChild(child);
    }
    
    {
        BenchmarkTimer timer("Clone deep tree 100 times");
        for (int i = 0; i < COUNT; ++i) {
            auto clone = root->CloneNode(true);
        }
    }
}

// ========== innerHTML 基准测试 ==========

TEST_F(DOMBenchmark, GetInnerHTML) {
    const int COUNT = 100;
    
    // 创建一个复杂的树
    auto root = doc->CreateElement("div");
    for (int i = 0; i < 10; ++i) {
        auto child = doc->CreateElement("div");
        child->SetAttribute("id", "child" + std::to_string(i));
        for (int j = 0; j < 10; ++j) {
            auto grandchild = doc->CreateElement("span");
            grandchild->AppendChild(doc->CreateTextNode("Text " + std::to_string(j)));
            child->AppendChild(grandchild);
        }
        root->AppendChild(child);
    }
    
    {
        BenchmarkTimer timer("GetInnerHTML 100 times");
        for (int i = 0; i < COUNT; ++i) {
            auto html = root->GetInnerHTML();
        }
    }
}

// ========== 主函数 ==========

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  DOM API Performance Benchmarks" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n";
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  Benchmarks Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n";
    
    return result;
}

