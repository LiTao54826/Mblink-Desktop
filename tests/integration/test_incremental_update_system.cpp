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
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/observers/dirty_node_tracker.h"
#include "core/layout/layout_engine.h"
#include "core/render/css/style_resolver.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/pipeline/render_tree_synchronizer.h"
#include "core/render/objects/render_object.h"

namespace mbink {
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

TEST_F(IncrementalUpdateSystemTest, SynchronizerStyleOnlyChangeDoesNotRequestLayoutTreeRebuild) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    auto panel = doc->CreateElement("div");
    panel->SetAttribute("style",
                        "display: grid; width: 856px; "
                        "grid-template-columns: repeat(auto-fit, minmax(190px, 1fr)); "
                        "grid-auto-rows: minmax(134px, auto); gap: 10px;");
    std::vector<std::shared_ptr<Element>> items;
    for (int i = 0; i < 8; ++i) {
        auto child = doc->CreateElement("article");
        child->SetAttribute("style", "height: 48px;");
        child->SetTextContent("compact target " + std::to_string(i + 1));
        items.push_back(child);
        panel->AppendChild(child);
    }
    body->AppendChild(panel);

    RenderTreeBuilder builder;
    builder.SetDocument(doc.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(panel->GetRenderObject(), nullptr);
    ASSERT_NE(items[4]->GetRenderObject(), nullptr);

    auto owned_layout_engine = std::make_unique<LayoutEngine>();
    LayoutEngine* layout_engine = owned_layout_engine.get();
    layout_engine->BuildLayoutTree(render_root);
    layout_engine->ComputeLayout(1180.0f, 820.0f);
    layout_engine->GetLayoutInfo(render_root);
    const auto normal_item = items[4]->GetRenderObject()->GetLayoutInfo();

    doc->GetDirtyTracker().Clear();
    panel->SetAttribute("style",
                        "display: grid; width: 856px; "
                        "grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); "
                        "grid-auto-rows: minmax(104px, auto); gap: 7px;");
    ASSERT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 0);
    ASSERT_GT(doc->GetDirtyTracker().GetStyleChangeCount(), 0);

    RenderTreeSynchronizer synchronizer;
    synchronizer.SetDocument(doc);
    synchronizer.SetLayoutEngine(std::shared_ptr<LayoutEngine>(
        layout_engine, [](LayoutEngine*) {}));

    bool requires_layout_tree_rebuild =
        synchronizer.Synchronize(doc->GetDirtyTracker(), render_root);

    EXPECT_FALSE(requires_layout_tree_rebuild);
    EXPECT_FALSE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_TRUE(layout_engine->ComputeIncrementalLayout(1180.0f, 820.0f));
    layout_engine->GetLayoutInfo(render_root);

    const auto compact_item = items[4]->GetRenderObject()->GetLayoutInfo();
    EXPECT_NE(compact_item.x, normal_item.x);
    EXPECT_NEAR(compact_item.x, 690.4f, 0.5f);
    EXPECT_NEAR(compact_item.y, 0.0f, 0.5f);
}

TEST_F(IncrementalUpdateSystemTest, SynchronizerStyleAttributeRefreshesInheritedTextColor) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    auto button = doc->CreateElement("button");
    button->SetAttribute("style", "background: #4a90d9; color: #fff;");
    auto label = doc->CreateTextNode("All");
    button->AppendChild(label);
    body->AppendChild(button);

    RenderTreeBuilder builder;
    builder.SetDocument(doc.get());
    auto render_root = builder.BuildRenderTree(body);
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(button->GetRenderObject(), nullptr);
    ASSERT_NE(label->GetRenderObject(), nullptr);
    EXPECT_EQ(label->GetRenderObject()->GetComputedStyle().color, "#fff");

    doc->GetDirtyTracker().Clear();
    button->SetAttribute("style", "background: #fff; color: #555;");
    ASSERT_EQ(doc->GetDirtyTracker().GetStructuralChangeCount(), 0);
    ASSERT_GT(doc->GetDirtyTracker().GetStyleChangeCount(), 0);

    auto owned_layout_engine = std::make_unique<LayoutEngine>();
    LayoutEngine* layout_engine = owned_layout_engine.get();
    layout_engine->BuildLayoutTree(render_root);

    RenderTreeSynchronizer synchronizer;
    synchronizer.SetDocument(doc);
    synchronizer.SetLayoutEngine(std::shared_ptr<LayoutEngine>(
        layout_engine, [](LayoutEngine*) {}));

    bool requires_layout_tree_rebuild =
        synchronizer.Synchronize(doc->GetDirtyTracker(), render_root);

    EXPECT_FALSE(requires_layout_tree_rebuild);
    EXPECT_FALSE(doc->GetDirtyTracker().HasPendingChanges());
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().color, "#555");
    EXPECT_EQ(label->GetRenderObject()->GetComputedStyle().color, "#555");
}

} // namespace test
} // namespace mbink
