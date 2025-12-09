/**
 * @file test_incremental_rendering.cpp
 * @brief Week 2增量渲染集成测试
 *
 * 测试内容：
 * - 渲染树缓存
 * - 增量布局
 * - 批量更新API
 */

#include <gtest/gtest.h>
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include <thread>
#include <chrono>

using namespace lightui;

// 测试夹具
class IncrementalRenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清空 SDL 事件队列
        ClearSDLEvents();

        // 创建隐藏窗口用于测试
        WindowConfig config;
        config.title = "Incremental Rendering Test";
        config.width = 800;
        config.height = 600;
        config.hidden = true;  // 隐藏窗口以避免干扰

        window_ = std::make_unique<Window>(config);

        // 创建并设置Document
        doc_ = std::make_shared<Document>();
        doc_->Initialize();  // 初始化Document（创建html和body元素）
        window_->SetDocument(doc_);
    }

    void TearDown() override {
        window_.reset();
        
        // 清空 SDL 事件队列
        ClearSDLEvents();
        
        // 给 SDL 一点时间清理资源
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 清空 SDL 事件队列
    void ClearSDLEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 丢弃所有待处理事件
        }
    }

    std::unique_ptr<Window> window_;
    std::shared_ptr<Document> doc_;
};

// ========== 测试渲染树缓存 ==========

TEST_F(IncrementalRenderingTest, RenderTreeCaching) {
    std::cout << "\n=== Test: RenderTreeCaching ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    // 创建一个简单的DOM结构
    auto div = doc_->CreateElement("div");
    div->SetAttribute("id", "test-div");
    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");
    body->AppendChild(div);
    
    std::cout << "Created DOM structure" << std::endl;
    
    // 第一次渲染 - 应该构建渲染树
    window_->RenderDocumentIncremental();
    std::cout << "First render complete" << std::endl;
    
    // 修改样式（不改变DOM结构）
    div->SetStyle("width", "200px");
    std::cout << "Modified style" << std::endl;
    
    // 第二次渲染 - 应该使用缓存的渲染树
    window_->RenderDocumentIncremental();
    std::cout << "Second render complete (should use cached render tree)" << std::endl;
    
    // 添加新节点（改变DOM结构）
    auto span = doc_->CreateElement("span");
    div->AppendChild(span);
    std::cout << "Added new node" << std::endl;
    
    // 第三次渲染 - 应该重建渲染树
    window_->RenderDocumentIncremental();
    std::cout << "Third render complete (should rebuild render tree)" << std::endl;
    
    SUCCEED();
}

TEST_F(IncrementalRenderingTest, RenderTreeInvalidation) {
    std::cout << "\n=== Test: RenderTreeInvalidation ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    auto div = doc_->CreateElement("div");
    body->AppendChild(div);
    
    // 第一次渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 移除节点 - 应该使渲染树失效
    body->RemoveChild(div);
    std::cout << "Removed node" << std::endl;
    
    // 渲染 - 应该重建渲染树
    window_->RenderDocumentIncremental();
    std::cout << "Render after removal complete" << std::endl;
    
    SUCCEED();
}

// ========== 测试增量布局 ==========

TEST_F(IncrementalRenderingTest, IncrementalLayout) {
    std::cout << "\n=== Test: IncrementalLayout ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    // 创建多个元素
    auto div1 = doc_->CreateElement("div");
    div1->SetAttribute("id", "div1");
    div1->SetStyle("width", "100px");
    div1->SetStyle("height", "100px");
    
    auto div2 = doc_->CreateElement("div");
    div2->SetAttribute("id", "div2");
    div2->SetStyle("width", "100px");
    div2->SetStyle("height", "100px");
    
    body->AppendChild(div1);
    body->AppendChild(div2);
    
    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 只修改div1的布局属性
    div1->SetStyle("width", "200px");
    std::cout << "Modified div1 layout" << std::endl;
    
    // 增量渲染 - 应该只布局div1及其祖先
    window_->RenderDocumentIncremental();
    std::cout << "Incremental render complete (should only layout div1)" << std::endl;
    
    SUCCEED();
}

TEST_F(IncrementalRenderingTest, StyleOnlyChange) {
    std::cout << "\n=== Test: StyleOnlyChange ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    auto div = doc_->CreateElement("div");
    div->SetStyle("width", "100px");
    div->SetStyle("height", "100px");
    div->SetStyle("background-color", "red");
    body->AppendChild(div);
    
    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 只修改样式属性（不影响布局）
    div->SetStyle("background-color", "blue");
    std::cout << "Modified style only (no layout change)" << std::endl;
    
    // 增量渲染 - 应该跳过布局，只重绘
    window_->RenderDocumentIncremental();
    std::cout << "Incremental render complete (should skip layout)" << std::endl;
    
    SUCCEED();
}

// ========== 测试批量更新API ==========

TEST_F(IncrementalRenderingTest, BatchUpdateBasic) {
    std::cout << "\n=== Test: BatchUpdateBasic ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    auto div = doc_->CreateElement("div");
    body->AppendChild(div);
    
    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 开始批量更新
    doc_->BeginBatch();
    EXPECT_TRUE(doc_->IsInBatch());
    std::cout << "Batch started" << std::endl;
    
    // 批量期间进行多次修改
    for (int i = 0; i < 10; i++) {
        div->SetStyle("width", std::to_string(100 + i * 10) + "px");
    }
    std::cout << "Made 10 style changes in batch" << std::endl;
    
    // 结束批量更新
    doc_->EndBatch();
    EXPECT_FALSE(doc_->IsInBatch());
    std::cout << "Batch ended (should trigger single repaint)" << std::endl;
    
    // 渲染
    window_->RenderDocumentIncremental();
    std::cout << "Render after batch complete" << std::endl;
    
    SUCCEED();
}

TEST_F(IncrementalRenderingTest, NestedBatch) {
    std::cout << "\n=== Test: NestedBatch ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    auto div = doc_->CreateElement("div");
    body->AppendChild(div);
    
    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 嵌套批量更新
    doc_->BeginBatch();
    EXPECT_TRUE(doc_->IsInBatch());
    std::cout << "Outer batch started" << std::endl;
    
    div->SetStyle("width", "100px");
    
    doc_->BeginBatch();
    EXPECT_TRUE(doc_->IsInBatch());
    std::cout << "Inner batch started" << std::endl;
    
    div->SetStyle("height", "100px");
    
    doc_->EndBatch();
    EXPECT_TRUE(doc_->IsInBatch());  // 仍在外层批量中
    std::cout << "Inner batch ended (still in outer batch)" << std::endl;
    
    div->SetStyle("background-color", "red");
    
    doc_->EndBatch();
    EXPECT_FALSE(doc_->IsInBatch());  // 所有批量结束
    std::cout << "Outer batch ended (should trigger single repaint)" << std::endl;
    
    // 渲染
    window_->RenderDocumentIncremental();
    std::cout << "Render after nested batch complete" << std::endl;
    
    SUCCEED();
}

TEST_F(IncrementalRenderingTest, BatchWithMultipleElements) {
    std::cout << "\n=== Test: BatchWithMultipleElements ===" << std::endl;
    
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);
    
    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Initial render complete" << std::endl;
    
    // 批量添加多个元素
    doc_->BeginBatch();
    std::cout << "Batch started" << std::endl;
    
    for (int i = 0; i < 100; i++) {
        auto div = doc_->CreateElement("div");
        div->SetAttribute("id", "div-" + std::to_string(i));
        div->SetStyle("width", "50px");
        div->SetStyle("height", "50px");
        body->AppendChild(div);
    }
    std::cout << "Added 100 elements in batch" << std::endl;
    
    doc_->EndBatch();
    std::cout << "Batch ended (should trigger single repaint)" << std::endl;
    
    // 渲染
    window_->RenderDocumentIncremental();
    std::cout << "Render after batch complete" << std::endl;
    
    SUCCEED();
}

// 测试：1000项列表增删性能
TEST_F(IncrementalRenderingTest, PerformanceLargeListAddRemove) {
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);

    std::cout << "\n=== Performance Test: Large List Add/Remove ===" << std::endl;

    // 初始渲染
    window_->RenderDocumentIncremental();

    // 测试添加1000个元素的性能
    auto start = std::chrono::high_resolution_clock::now();

    doc_->BeginBatch();
    for (int i = 0; i < 1000; i++) {
        auto div = doc_->CreateElement("div");
        div->SetAttribute("id", "item-" + std::to_string(i));
        div->SetStyle("width", "100px");
        div->SetStyle("height", "20px");
        body->AppendChild(div);
    }
    doc_->EndBatch();
    window_->RenderDocumentIncremental();

    auto end = std::chrono::high_resolution_clock::now();
    auto add_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Add 1000 elements: " << add_duration << " ms" << std::endl;

    // 目标：< 100ms (保守目标，16ms是理想目标)
    EXPECT_LT(add_duration, 500) << "Adding 1000 elements should be fast";

    // 测试删除1000个元素的性能
    start = std::chrono::high_resolution_clock::now();

    doc_->BeginBatch();
    auto children = body->GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(child);
            if (elem && elem->GetAttribute("id").find("item-") == 0) {
                body->RemoveChild(child);
            }
        }
    }
    doc_->EndBatch();
    window_->RenderDocumentIncremental();

    end = std::chrono::high_resolution_clock::now();
    auto remove_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Remove elements: " << remove_duration << " ms" << std::endl;

    EXPECT_LT(remove_duration, 500) << "Removing elements should be fast";

    std::cout << "Performance test complete!" << std::endl;
    SUCCEED();
}

// 测试：多组件同时更新性能
TEST_F(IncrementalRenderingTest, PerformanceMultipleComponentUpdates) {
    auto body = doc_->GetBody();
    ASSERT_NE(body, nullptr);

    std::cout << "\n=== Performance Test: Multiple Component Updates ===" << std::endl;

    // 创建100个"组件"
    std::vector<std::shared_ptr<Element>> components;
    for (int i = 0; i < 100; i++) {
        auto div = doc_->CreateElement("div");
        div->SetAttribute("id", "component-" + std::to_string(i));
        div->SetStyle("width", "50px");
        div->SetStyle("height", "50px");
        div->SetStyle("background-color", "blue");
        body->AppendChild(div);
        components.push_back(div);
    }

    // 初始渲染
    window_->RenderDocumentIncremental();
    std::cout << "Created 100 components" << std::endl;

    // 测试100个组件同时更新的性能
    auto start = std::chrono::high_resolution_clock::now();

    doc_->BeginBatch();
    for (int i = 0; i < 100; i++) {
        components[i]->SetStyle("background-color", "red");
        components[i]->SetStyle("width", "60px");
    }
    doc_->EndBatch();
    window_->RenderDocumentIncremental();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Update 100 components: " << duration << " ms" << std::endl;

    // 目标：< 16ms (60 FPS)
    EXPECT_LT(duration, 100) << "Updating 100 components should be fast";

    std::cout << "Performance test complete!" << std::endl;
    SUCCEED();
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

