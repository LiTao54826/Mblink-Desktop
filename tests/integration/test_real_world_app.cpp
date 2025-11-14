/**
 * @file test_real_world_app.cpp
 * @brief 实际应用场景测试 - 简化版本，只测试DOM操作性能
 */

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <functional>
#include "core/dom/document.h"
#include "core/dom/element.h"

using namespace lightui;

class PerformanceTimer {
public:
    void Start() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    double Stop() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start_;
        return duration.count();
    }
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

class RealWorldAppTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
        doc_->Initialize();  // 必须调用Initialize()创建基本HTML结构
    }

    std::shared_ptr<Element> CreateElement(const std::string& tag,
                                          const std::string& id = "",
                                          const std::string& text = "") {
        auto elem = doc_->CreateElement(tag);
        // 注意：ID需要在元素添加到文档树后设置，否则GetOwnerDocument()返回null
        // 所以这里先不设置ID，由调用者在AppendChild后设置
        if (!text.empty()) {
            auto textNode = doc_->CreateTextNode(text);
            elem->AppendChild(textNode);
        }
        // 如果有ID，先存储到data-id属性，稍后设置
        if (!id.empty()) {
            elem->SetAttribute("data-temp-id", id);
        }
        return elem;
    }

    // 辅助函数：在元素添加到文档树后设置ID
    void SetElementId(std::shared_ptr<Element> elem) {
        std::string tempId = elem->GetAttribute("data-temp-id");
        if (!tempId.empty()) {
            elem->RemoveAttribute("data-temp-id");
            elem->SetAttribute("id", tempId);
        }
    }

    double MeasureDOMOperation(std::function<void()> operation) {
        PerformanceTimer timer;
        timer.Start();
        operation();
        return timer.Stop();
    }

    std::shared_ptr<Document> doc_;
};

TEST_F(RealWorldAppTest, TodoListApp) {
    std::cout << "\n=== 测试1: 待办事项列表应用 ===" << std::endl;
    
    auto body = doc_->GetBody();
    auto todoList = CreateElement("ul", "todo-list");
    body->AppendChild(todoList);
    
    // 批量添加20个待办事项
    double addTime = MeasureDOMOperation([&]() {
        doc_->BeginBatch();
        for (int i = 1; i <= 20; i++) {
            auto item = CreateElement("li", "todo-" + std::to_string(i), "任务" + std::to_string(i));
            todoList->AppendChild(item);
            SetElementId(item);  // 在添加到文档树后设置ID
        }
        doc_->EndBatch();
    });
    
    std::cout << "批量添加20项耗时: " << addTime << " ms" << std::endl;
    EXPECT_LT(addTime, 50.0);
    
    // 更新单个任务
    auto todo5 = doc_->GetElementById("todo-5");
    ASSERT_NE(todo5, nullptr);
    
    double updateTime = MeasureDOMOperation([&]() {
        todo5->SetAttribute("class", "completed");
    });
    
    std::cout << "更新单项耗时: " << updateTime << " ms" << std::endl;
    EXPECT_LT(updateTime, 10.0);
    
    std::cout << "✅ 待办事项列表测试通过" << std::endl;
}

TEST_F(RealWorldAppTest, DataGridApp) {
    std::cout << "\n=== 测试2: 动态表格应用 ===" << std::endl;
    
    auto body = doc_->GetBody();
    auto table = CreateElement("table");
    auto tbody = CreateElement("tbody", "tbody");
    table->AppendChild(tbody);
    body->AppendChild(table);
    
    // 批量加载50行
    double loadTime = MeasureDOMOperation([&]() {
        doc_->BeginBatch();
        for (int i = 1; i <= 50; i++) {
            auto row = CreateElement("tr", "row-" + std::to_string(i));
            row->AppendChild(CreateElement("td", "", std::to_string(i)));
            row->AppendChild(CreateElement("td", "", "用户" + std::to_string(i)));
            tbody->AppendChild(row);
        }
        doc_->EndBatch();
    });
    
    std::cout << "加载50行耗时: " << loadTime << " ms" << std::endl;
    EXPECT_LT(loadTime, 100.0);
    
    std::cout << "✅ 动态表格测试通过" << std::endl;
}

TEST_F(RealWorldAppTest, ComplexDOMStructure) {
    std::cout << "\n=== 测试3: 复杂DOM结构 ===" << std::endl;
    
    auto body = doc_->GetBody();
    
    // 创建200+节点的复杂结构
    std::vector<std::shared_ptr<Element>> sections;  // 保存section引用
    double buildTime = MeasureDOMOperation([&]() {
        doc_->BeginBatch();

        auto app = CreateElement("div", "app");

        // 导航栏
        auto nav = CreateElement("nav");
        for (int i = 0; i < 5; i++) {
            nav->AppendChild(CreateElement("a", "", "菜单" + std::to_string(i)));
        }
        app->AppendChild(nav);

        // 主内容区（20个section，每个5个card）
        auto main = CreateElement("main");
        for (int i = 0; i < 20; i++) {
            auto section = CreateElement("section", "section-" + std::to_string(i));
            sections.push_back(section);  // 保存引用
            for (int j = 0; j < 5; j++) {
                auto card = CreateElement("div", "", "内容" + std::to_string(j));
                section->AppendChild(card);
            }
            main->AppendChild(section);
        }
        app->AppendChild(main);

        body->AppendChild(app);

        // 在添加到文档树后设置所有ID
        for (auto& section : sections) {
            SetElementId(section);
        }

        doc_->EndBatch();
    });
    
    std::cout << "构建200+节点耗时: " << buildTime << " ms" << std::endl;
    EXPECT_LT(buildTime, 150.0);
    
    // 深层更新
    auto section10 = doc_->GetElementById("section-10");
    ASSERT_NE(section10, nullptr);
    
    double updateTime = MeasureDOMOperation([&]() {
        section10->SetAttribute("class", "highlighted");
    });
    
    std::cout << "深层更新耗时: " << updateTime << " ms" << std::endl;
    EXPECT_LT(updateTime, 10.0);
    
    std::cout << "✅ 复杂DOM结构测试通过" << std::endl;
    std::cout << "\n========== 所有实际应用测试通过 ==========" << std::endl;
}
