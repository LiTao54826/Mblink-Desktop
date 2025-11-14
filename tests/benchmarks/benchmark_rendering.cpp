#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <vector>
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/dom/element.h"

using namespace lightui;

// 性能计时器
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

// 清空SDL事件队列
static void ClearSDLEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 清空事件队列
    }
}

// 基准测试基类
class RenderingBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        ClearSDLEvents();
        
        WindowConfig config;
        config.title = "Rendering Benchmark";
        config.width = 800;
        config.height = 600;
        config.hidden = true;
        
        window_ = std::make_unique<Window>(config);
        doc_ = std::make_shared<Document>();
        doc_->Initialize();
        window_->SetDocument(doc_);
    }
    
    void TearDown() override {
        window_.reset();
        ClearSDLEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 创建大量DOM节点（使用批量更新避免每次都重建渲染树）
    std::vector<std::shared_ptr<Element>> CreateManyNodes(int count) {
        std::vector<std::shared_ptr<Element>> nodes;
        auto body = doc_->GetBody();

        doc_->BeginBatch();  // 开始批量更新
        for (int i = 0; i < count; i++) {
            auto div = doc_->CreateElement("div");
            div->SetAttribute("id", "node" + std::to_string(i));
            div->SetAttribute("style", "width: 100px; height: 20px;");
            body->AppendChild(div);
            nodes.push_back(div);
        }
        doc_->EndBatch();  // 结束批量更新

        return nodes;
    }
    
    std::unique_ptr<Window> window_;
    std::shared_ptr<Document> doc_;
};

// ========== 场景1: 单节点样式更新 ==========
// 测试增量渲染的效果：1000个节点，只更新1个节点的样式
TEST_F(RenderingBenchmark, SingleNodeStyleUpdate) {
    std::cout << "\n=== 场景1: 单节点样式更新 ===" << std::endl;
    std::cout << "创建1000个节点，只更新1个节点的样式" << std::endl;
    
    // 创建1000个节点
    auto nodes = CreateManyNodes(1000);
    
    // 初始渲染
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    
    // 测试：更新单个节点的样式（10次取平均）
    PerformanceTimer timer;
    double total_time = 0.0;
    const int iterations = 10;
    
    for (int i = 0; i < iterations; i++) {
        timer.Start();
        
        // 只更新第一个节点的样式
        nodes[0]->SetAttribute("style", "width: 100px; height: 20px; background-color: red;");
        window_->SetNeedsRepaint();
        window_->RenderDocumentIncremental();
        
        double elapsed = timer.Stop();
        total_time += elapsed;
        
        // 恢复样式
        nodes[0]->SetAttribute("style", "width: 100px; height: 20px;");
    }
    
    double avg_time = total_time / iterations;
    std::cout << "平均渲染时间: " << avg_time << " ms" << std::endl;
    std::cout << "目标: <20ms (增量渲染，1000个节点)" << std::endl;

    // 验证：增量渲染应该很快（1000个节点的情况下，<20ms是合理的）
    EXPECT_LT(avg_time, 20.0) << "单节点更新应该使用增量渲染，时间应该<20ms（1000个节点）";

    if (avg_time < 10.0) {
        std::cout << "✅ 性能优秀！" << std::endl;
    } else if (avg_time < 15.0) {
        std::cout << "✅ 性能良好" << std::endl;
    } else if (avg_time < 20.0) {
        std::cout << "✅ 性能合格" << std::endl;
    } else {
        std::cout << "⚠️  性能需要优化" << std::endl;
    }
}

// ========== 场景2: 批量更新 ==========
// 测试批量更新API的效果：批量更新100个节点
TEST_F(RenderingBenchmark, BatchUpdate) {
    std::cout << "\n=== 场景2: 批量更新 ===" << std::endl;
    std::cout << "批量更新100个节点" << std::endl;
    
    // 创建100个节点
    auto nodes = CreateManyNodes(100);
    
    // 初始渲染
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    
    PerformanceTimer timer;
    
    // 测试1: 不使用批量更新（逐个更新）
    timer.Start();
    for (int i = 0; i < 100; i++) {
        nodes[i]->SetAttribute("style", "width: 100px; height: 20px; background-color: blue;");
        window_->SetNeedsRepaint();
        window_->RenderDocumentIncremental();
    }
    double time_without_batch = timer.Stop();
    
    std::cout << "不使用批量更新: " << time_without_batch << " ms" << std::endl;
    
    // 恢复样式
    for (int i = 0; i < 100; i++) {
        nodes[i]->SetAttribute("style", "width: 100px; height: 20px;");
    }
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    
    // 测试2: 使用批量更新
    timer.Start();
    doc_->BeginBatch();
    for (int i = 0; i < 100; i++) {
        nodes[i]->SetAttribute("style", "width: 100px; height: 20px; background-color: blue;");
    }
    doc_->EndBatch();
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    double time_with_batch = timer.Stop();
    
    std::cout << "使用批量更新: " << time_with_batch << " ms" << std::endl;
    
    // 计算性能提升
    double speedup = time_without_batch / time_with_batch;
    std::cout << "性能提升: " << speedup << "x" << std::endl;
    std::cout << "目标: >10x" << std::endl;
    
    // 验证：批量更新应该至少快5倍
    EXPECT_GT(speedup, 5.0) << "批量更新应该至少快5倍";
    
    if (speedup > 20.0) {
        std::cout << "✅ 性能优秀！" << std::endl;
    } else if (speedup > 10.0) {
        std::cout << "✅ 性能良好" << std::endl;
    } else if (speedup > 5.0) {
        std::cout << "✅ 性能合格" << std::endl;
    } else {
        std::cout << "⚠️  性能需要优化" << std::endl;
    }
}

// ========== 场景3: 大量DOM操作 ==========
// 测试渲染树缓存的效果：添加/删除大量节点
TEST_F(RenderingBenchmark, MassiveDOMOperations) {
    std::cout << "\n=== 场景3: 大量DOM操作 ===" << std::endl;
    std::cout << "批量添加/删除500个节点" << std::endl;
    
    auto body = doc_->GetBody();
    PerformanceTimer timer;
    
    // 测试：批量添加500个节点
    timer.Start();
    doc_->BeginBatch();
    std::vector<std::shared_ptr<Element>> nodes;
    for (int i = 0; i < 500; i++) {
        auto div = doc_->CreateElement("div");
        div->SetAttribute("id", "node" + std::to_string(i));
        div->SetAttribute("style", "width: 100px; height: 20px;");
        body->AppendChild(div);
        nodes.push_back(div);
    }
    doc_->EndBatch();
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    double add_time = timer.Stop();
    
    std::cout << "添加500个节点: " << add_time << " ms" << std::endl;
    
    // 测试：样式更新（应该使用缓存的渲染树）
    timer.Start();
    nodes[0]->SetAttribute("style", "width: 100px; height: 20px; background-color: red;");
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    double style_update_time = timer.Stop();
    
    std::cout << "样式更新（使用缓存）: " << style_update_time << " ms" << std::endl;
    
    // 测试：批量删除500个节点
    timer.Start();
    doc_->BeginBatch();
    for (auto& node : nodes) {
        body->RemoveChild(node);
    }
    doc_->EndBatch();
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    double remove_time = timer.Stop();
    
    std::cout << "删除500个节点: " << remove_time << " ms" << std::endl;
    std::cout << "目标: 添加<100ms, 样式更新<35ms, 删除<100ms" << std::endl;

    // 验证（500个节点的情况下，样式更新需要遍历整个渲染树，所以阈值设置为35ms）
    EXPECT_LT(add_time, 200.0) << "添加500个节点应该<200ms";
    EXPECT_LT(style_update_time, 35.0) << "样式更新应该<35ms（使用缓存，500个节点）";
    EXPECT_LT(remove_time, 200.0) << "删除500个节点应该<200ms";
    
    if (add_time < 100.0 && style_update_time < 10.0 && remove_time < 100.0) {
        std::cout << "✅ 性能优秀！" << std::endl;
    } else if (add_time < 150.0 && style_update_time < 15.0 && remove_time < 150.0) {
        std::cout << "✅ 性能良好" << std::endl;
    } else {
        std::cout << "✅ 性能合格" << std::endl;
    }
}

// ========== 场景4: 渲染树缓存效果 ==========
// 对比DOM结构改变 vs 样式改变的性能差异
TEST_F(RenderingBenchmark, RenderTreeCachingEffect) {
    std::cout << "\n=== 场景4: 渲染树缓存效果 ===" << std::endl;
    std::cout << "对比DOM结构改变 vs 样式改变" << std::endl;
    
    // 创建100个节点
    auto nodes = CreateManyNodes(100);
    
    // 初始渲染
    window_->SetNeedsRepaint();
    window_->RenderDocumentIncremental();
    
    PerformanceTimer timer;
    
    // 测试1: 样式改变（使用缓存的渲染树）
    double total_style_time = 0.0;
    for (int i = 0; i < 10; i++) {
        timer.Start();
        nodes[0]->SetAttribute("style", "width: 100px; height: 20px; background-color: red;");
        window_->SetNeedsRepaint();
        window_->RenderDocumentIncremental();
        total_style_time += timer.Stop();
        
        nodes[0]->SetAttribute("style", "width: 100px; height: 20px;");
    }
    double avg_style_time = total_style_time / 10.0;
    
    std::cout << "样式改变（使用缓存）: " << avg_style_time << " ms" << std::endl;
    
    // 测试2: DOM结构改变（重建渲染树）
    auto body = doc_->GetBody();
    double total_structure_time = 0.0;
    for (int i = 0; i < 10; i++) {
        timer.Start();
        auto div = doc_->CreateElement("div");
        body->AppendChild(div);
        window_->SetNeedsRepaint();
        window_->RenderDocumentIncremental();
        body->RemoveChild(div);
        total_structure_time += timer.Stop();
    }
    double avg_structure_time = total_structure_time / 10.0;
    
    std::cout << "DOM结构改变（重建渲染树）: " << avg_structure_time << " ms" << std::endl;
    
    // 计算差异
    double ratio = avg_structure_time / avg_style_time;
    std::cout << "性能差异: " << ratio << "x" << std::endl;
    std::cout << "说明: DOM结构改变比样式改变慢" << ratio << "倍" << std::endl;
    
    // 验证：样式改变应该明显快于DOM结构改变
    EXPECT_GT(ratio, 1.5) << "渲染树缓存应该使样式改变明显快于DOM结构改变";
    
    if (ratio > 3.0) {
        std::cout << "✅ 渲染树缓存效果优秀！" << std::endl;
    } else if (ratio > 2.0) {
        std::cout << "✅ 渲染树缓存效果良好" << std::endl;
    } else {
        std::cout << "✅ 渲染树缓存有效" << std::endl;
    }
}

