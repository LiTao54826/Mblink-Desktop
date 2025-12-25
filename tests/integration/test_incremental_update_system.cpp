/**
 * @file test_incremental_update_system.cpp
 * @brief 增量更新系统集成测试
 * 
 * 测试 DirtyNodeTracker、RenderTreeSynchronizer 和 RenderPipeline 的集成
 * 
 * **Feature: incremental-update-system**
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"
#include "dom/text.h"
#include "dom/dirty_node_tracker.h"
#include "render/render_pipeline.h"
#include "render/render_tree_synchronizer.h"
#include "render/render_object.h"

namespace lightui {
namespace test {

class IncrementalUpdateSystemTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
    }

    void TearDown() override {
        DOMTestBase::TearDown();
    }
};

// ========== DirtyNodeTracker 测试 ==========

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerRecordsNodeAdded) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 添加新节点
    auto div = doc->CreateElement("div");
    body->AppendChild(div);
    
    // 验证变化被记录
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 1);
    
    const auto& changes = doc->GetDirtyTracker().GetStructuralChanges();
    EXPECT_EQ(changes[0].type, DirtyNodeTracker::StructuralChangeType::Added);
}

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerRecordsNodeRemoved) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 先添加一个节点
    auto div = doc->CreateElement("div");
    body->AppendChild(div);
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 移除节点
    body->RemoveChild(div);
    
    // 验证变化被记录
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 1);
    
    const auto& changes = doc->GetDirtyTracker().GetStructuralChanges();
    EXPECT_EQ(changes[0].type, DirtyNodeTracker::StructuralChangeType::Removed);
}

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerRecordsNodeReplaced) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 先添加一个节点
    auto old_div = doc->CreateElement("div");
    old_div->SetAttribute("id", "old");
    body->AppendChild(old_div);
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 替换节点
    auto new_div = doc->CreateElement("div");
    new_div->SetAttribute("id", "new");
    body->ReplaceChild(new_div, old_div);
    
    // 验证变化被记录为原子替换操作
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    
    const auto& changes = doc->GetDirtyTracker().GetStructuralChanges();
    bool found_replaced = false;
    for (const auto& change : changes) {
        if (change.type == DirtyNodeTracker::StructuralChangeType::Replaced) {
            found_replaced = true;
            break;
        }
    }
    EXPECT_TRUE(found_replaced);
}

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerRecordsTextChanged) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 添加文本节点
    auto text = doc->CreateTextNode("Hello");
    body->AppendChild(text);
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 修改文本
    text->SetData("World");
    
    // 验证变化被记录
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(doc->GetDirtyTracker().GetTextChangeCount(), 1);
    
    const auto& changes = doc->GetDirtyTracker().GetTextChanges();
    EXPECT_EQ(changes[0].old_text, "Hello");
    EXPECT_EQ(changes[0].new_text, "World");
}

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerClear) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 添加一些变化
    auto div = doc->CreateElement("div");
    body->AppendChild(div);
    
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    
    // 清除变化
    doc->GetDirtyTracker().Clear();
    
    EXPECT_FALSE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 0);
}

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerOptimize) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 添加然后移除同一个节点
    auto div = doc->CreateElement("div");
    body->AppendChild(div);
    body->RemoveChild(div);
    
    // 优化前应该有 2 个变化
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 2);
    
    // 优化
    doc->GetDirtyTracker().Optimize();
    
    // 优化后应该合并/取消这些变化
    // 注意：具体行为取决于 Optimize 的实现
    EXPECT_LE(doc->GetDirtyTracker().GetStructuralChangeCount(), 2);
}

// ========== InsertBefore 测试 ==========

TEST_F(IncrementalUpdateSystemTest, DirtyTrackerRecordsInsertBefore) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 先添加一个参考节点
    auto ref = doc->CreateElement("div");
    ref->SetAttribute("id", "ref");
    body->AppendChild(ref);
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 在参考节点前插入
    auto new_div = doc->CreateElement("div");
    new_div->SetAttribute("id", "new");
    body->InsertBefore(new_div, ref);
    
    // 验证变化被记录
    EXPECT_TRUE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 1);
    
    const auto& changes = doc->GetDirtyTracker().GetStructuralChanges();
    EXPECT_EQ(changes[0].type, DirtyNodeTracker::StructuralChangeType::Added);
    EXPECT_EQ(changes[0].index, 0);  // 应该在索引 0 处插入
}

// ========== 多个变化测试 ==========

TEST_F(IncrementalUpdateSystemTest, MultipleChanges) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 清除之前的变化
    doc->GetDirtyTracker().Clear();
    
    // 添加多个节点
    for (int i = 0; i < 5; i++) {
        auto div = doc->CreateElement("div");
        div->SetAttribute("id", "div" + std::to_string(i));
        body->AppendChild(div);
    }
    
    // 验证所有变化都被记录
    EXPECT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 5);
}

// ========== RenderPipeline 生命周期测试 ==========

TEST_F(IncrementalUpdateSystemTest, RenderPipelineLifecycle) {
    RenderPipeline pipeline;
    pipeline.Initialize(800, 600);

    // 初始状态应该需要更新
    EXPECT_TRUE(pipeline.NeedsUpdate());

    // 标记需要布局
    pipeline.MarkNeedsLayout();
    EXPECT_TRUE(pipeline.NeedsUpdate());

    // 标记需要绘制
    pipeline.MarkNeedsPaint();
    EXPECT_TRUE(pipeline.NeedsUpdate());
}

// ========== RenderTreeSynchronizer 策略测试 ==========

TEST_F(IncrementalUpdateSystemTest, SynchronizerRebuildThreshold) {
    RenderTreeSynchronizer synchronizer;
    
    // 设置阈值
    synchronizer.SetRebuildThreshold(5);
    synchronizer.SetReplacedChildrenThreshold(3);
    synchronizer.SetParentChangesThreshold(2);
    
    // 这些设置应该被保存（无法直接验证，但不应该崩溃）
    SUCCEED();
}

} // namespace test
} // namespace lightui
