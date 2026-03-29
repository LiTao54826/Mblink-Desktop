/**
 * @file test_incremental_update.cpp
 * @brief 增量更新系统验证测试
 *
 * 测试场景：模拟简单计数器应用 (test_preact_simple.js)
 * 验证内容：
 * - 文本变化只触发局部更新，不重建整个渲染树
 * - 增量样式重算正确跳过干净子树
 * - 性能统计正确记录优化比率
 *
 * Requirements: 1.1, 1.2, 1.3
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/node.h"
#include "dom/element.h"
#include "dom/text.h"
#include "dom/document.h"
#include "dom/style/incremental_style_recalc.h"
#include "dom/observers/dirty_node_tracker.h"

namespace mbink {
namespace test {

/**
 * @brief 增量更新测试类
 */
class IncrementalUpdateTest : public DOMTestBase {};

// ========== 文本变化局部性测试 ==========

/**
 * @brief 测试文本变化只标记本地节点
 * 
 * 模拟计数器场景：只有计数文本变化，其他节点应保持干净
 * Validates: Requirement 1.1 - 文本变化只影响文本节点本身
 */
TEST_F(IncrementalUpdateTest, TextChangeMarksOnlyLocalNode) {
    // 构建类似计数器的 DOM 结构
    // <div id="app">
    //   <h1>Preact 测试</h1>
    //   <div id="counter">计数: 0</div>
    //   <div id="static">静态内容</div>
    // </div>
    auto app = CreateElement("div");
    app->SetAttribute("id", "app");
    
    auto h1 = CreateElement("h1");
    auto h1_text = CreateTextNode("Preact 测试");
    h1->AppendChild(h1_text);
    
    auto counter = CreateElement("div");
    counter->SetAttribute("id", "counter");
    auto counter_text = CreateTextNode("计数: 0");
    counter->AppendChild(counter_text);
    
    auto static_div = CreateElement("div");
    static_div->SetAttribute("id", "static");
    auto static_text = CreateTextNode("静态内容");
    static_div->AppendChild(static_text);
    
    app->AppendChild(h1);
    app->AppendChild(counter);
    app->AppendChild(static_div);
    
    // 清除所有脏标记（模拟初始渲染完成）
    app->ClearNeedsStyleRecalc();
    h1->ClearNeedsStyleRecalc();
    h1_text->ClearNeedsStyleRecalc();
    counter->ClearNeedsStyleRecalc();
    counter_text->ClearNeedsStyleRecalc();
    static_div->ClearNeedsStyleRecalc();
    static_text->ClearNeedsStyleRecalc();
    
    // 验证初始状态：所有节点都是干净的
    EXPECT_FALSE(app->NeedsStyleRecalc());
    EXPECT_FALSE(h1->NeedsStyleRecalc());
    EXPECT_FALSE(counter->NeedsStyleRecalc());
    EXPECT_FALSE(static_div->NeedsStyleRecalc());
    
    // 模拟计数器更新：只修改计数文本
    counter_text->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    counter_text->SetNeedsLayout();
    
    // 验证：只有计数文本节点被标记为脏
    EXPECT_TRUE(counter_text->NeedsStyleRecalc());
    EXPECT_EQ(counter_text->GetStyleChangeType(), StyleChangeType::kLocalStyleChange);
    
    // 验证：父节点被标记为 ChildNeedsStyleRecalc（但不是自身需要重算）
    EXPECT_TRUE(counter->ChildNeedsStyleRecalc());
    EXPECT_FALSE(counter->NeedsStyleRecalc());  // 自身不需要重算
    
    // 验证：其他节点保持干净
    EXPECT_FALSE(h1->NeedsStyleRecalc());
    EXPECT_FALSE(h1->ChildNeedsStyleRecalc());
    EXPECT_FALSE(static_div->NeedsStyleRecalc());
    EXPECT_FALSE(static_div->ChildNeedsStyleRecalc());
}

/**
 * @brief 测试祖先标记正确传播
 * 
 * Validates: Requirement 2.3 - ChildNeedsStyleRecalc 正确传播到祖先
 */
TEST_F(IncrementalUpdateTest, AncestorMarkingPropagation) {
    // 构建深层嵌套结构
    // <div id="root">
    //   <div id="level1">
    //     <div id="level2">
    //       <span id="leaf">文本</span>
    //     </div>
    //   </div>
    // </div>
    auto root = CreateElement("div");
    root->SetAttribute("id", "root");
    
    auto level1 = CreateElement("div");
    level1->SetAttribute("id", "level1");
    
    auto level2 = CreateElement("div");
    level2->SetAttribute("id", "level2");
    
    auto leaf = CreateElement("span");
    leaf->SetAttribute("id", "leaf");
    auto leaf_text = CreateTextNode("文本");
    
    leaf->AppendChild(leaf_text);
    level2->AppendChild(leaf);
    level1->AppendChild(level2);
    root->AppendChild(level1);
    
    // 清除所有脏标记
    root->ClearNeedsStyleRecalc();
    level1->ClearNeedsStyleRecalc();
    level2->ClearNeedsStyleRecalc();
    leaf->ClearNeedsStyleRecalc();
    leaf_text->ClearNeedsStyleRecalc();
    
    // 标记叶子节点需要样式重算
    leaf_text->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    
    // 验证：所有祖先都被标记为 ChildNeedsStyleRecalc
    EXPECT_TRUE(leaf->ChildNeedsStyleRecalc());
    EXPECT_TRUE(level2->ChildNeedsStyleRecalc());
    EXPECT_TRUE(level1->ChildNeedsStyleRecalc());
    EXPECT_TRUE(root->ChildNeedsStyleRecalc());
    
    // 验证：祖先自身不需要样式重算
    EXPECT_FALSE(leaf->NeedsStyleRecalc());
    EXPECT_FALSE(level2->NeedsStyleRecalc());
    EXPECT_FALSE(level1->NeedsStyleRecalc());
    EXPECT_FALSE(root->NeedsStyleRecalc());
}

/**
 * @brief 测试增量样式重算跳过干净子树
 * 
 * Validates: Requirement 3.4 - 跳过干净子树
 */
TEST_F(IncrementalUpdateTest, IncrementalStyleRecalcSkipsCleanSubtrees) {
    // 使用 Document 来测试增量样式重算
    auto body = doc_->GetBody();
    
    auto dirty_branch = CreateElement("div");
    dirty_branch->SetAttribute("id", "dirty");
    auto dirty_text = CreateTextNode("脏节点");
    dirty_branch->AppendChild(dirty_text);
    
    auto clean_branch = CreateElement("div");
    clean_branch->SetAttribute("id", "clean");
    auto clean_child1 = CreateElement("span");
    auto clean_child2 = CreateElement("span");
    auto clean_child3 = CreateElement("span");
    clean_branch->AppendChild(clean_child1);
    clean_branch->AppendChild(clean_child2);
    clean_branch->AppendChild(clean_child3);
    
    body->AppendChild(dirty_branch);
    body->AppendChild(clean_branch);
    
    // 清除所有脏标记
    doc_->ClearNeedsStyleRecalc();
    body->ClearNeedsStyleRecalc();
    dirty_branch->ClearNeedsStyleRecalc();
    dirty_text->ClearNeedsStyleRecalc();
    clean_branch->ClearNeedsStyleRecalc();
    clean_child1->ClearNeedsStyleRecalc();
    clean_child2->ClearNeedsStyleRecalc();
    clean_child3->ClearNeedsStyleRecalc();
    
    // 只标记脏分支
    dirty_text->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    
    // 执行增量样式重算
    IncrementalStyleRecalc style_recalc;
    style_recalc.RecalcStyle(doc_.get());
    
    // 验证统计：应该跳过干净子树
    int visited = style_recalc.GetNodesVisited();
    int recalculated = style_recalc.GetNodesRecalculated();
    int skipped = style_recalc.GetSubtreesSkipped();
    
    // 应该跳过 clean_branch 整个子树
    EXPECT_GT(skipped, 0) << "应该跳过干净子树";
    
    // 验证：脏标记被清除
    EXPECT_FALSE(dirty_text->NeedsStyleRecalc());
    EXPECT_FALSE(dirty_branch->ChildNeedsStyleRecalc());
}

/**
 * @brief 测试布局脏标记传播
 * 
 * Validates: Requirement 4.1 - 布局脏标记正确传播
 */
TEST_F(IncrementalUpdateTest, LayoutDirtyFlagPropagation) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    auto grandchild = CreateElement("a");
    
    child->AppendChild(grandchild);
    parent->AppendChild(child);
    
    // 清除所有布局脏标记
    parent->ClearNeedsLayout();
    child->ClearNeedsLayout();
    grandchild->ClearNeedsLayout();
    
    // 验证初始状态
    EXPECT_FALSE(parent->NeedsLayoutFlag());
    EXPECT_FALSE(child->NeedsLayoutFlag());
    EXPECT_FALSE(grandchild->NeedsLayoutFlag());
    
    // 标记孙节点需要布局
    grandchild->SetNeedsLayout();
    
    // 验证：孙节点被标记
    EXPECT_TRUE(grandchild->NeedsLayoutFlag());
    
    // 验证：祖先被标记为 ChildNeedsLayout
    EXPECT_TRUE(child->ChildNeedsLayout());
    EXPECT_TRUE(parent->ChildNeedsLayout());
}

/**
 * @brief 测试 Paint-Only 属性不触发布局
 * 
 * Validates: Requirement 6.1, 6.2 - Paint-only 属性只触发重绘
 */
TEST_F(IncrementalUpdateTest, PaintOnlyPropertyDoesNotTriggerLayout) {
    auto elem = CreateElement("div");
    
    // 清除所有脏标记
    elem->ClearNeedsStyleRecalc();
    elem->ClearNeedsLayout();
    
    // 模拟 paint-only 属性变化（如 color）
    // 只标记样式变化，不标记布局
    elem->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    // 注意：不调用 SetNeedsLayout()
    
    // 验证：需要样式重算但不需要布局
    EXPECT_TRUE(elem->NeedsStyleRecalc());
    EXPECT_FALSE(elem->NeedsLayoutFlag());
}

/**
 * @brief 测试 SubtreeStyleChange 标记整个子树
 * 
 * Validates: Requirement 3.3 - SubtreeStyleChange 重算整个子树
 */
TEST_F(IncrementalUpdateTest, SubtreeStyleChangeMarksEntireSubtree) {
    auto body = doc_->GetBody();
    
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("span");
    auto grandchild = CreateElement("a");
    
    child1->AppendChild(grandchild);
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    body->AppendChild(parent);
    
    // 清除所有脏标记
    doc_->ClearNeedsStyleRecalc();
    body->ClearNeedsStyleRecalc();
    parent->ClearNeedsStyleRecalc();
    child1->ClearNeedsStyleRecalc();
    child2->ClearNeedsStyleRecalc();
    grandchild->ClearNeedsStyleRecalc();
    
    // 标记父节点为 SubtreeStyleChange
    parent->SetNeedsStyleRecalc(StyleChangeType::kSubtreeStyleChange);
    
    // 验证：父节点被标记为 SubtreeStyleChange
    EXPECT_EQ(parent->GetStyleChangeType(), StyleChangeType::kSubtreeStyleChange);
    
    // 执行增量样式重算
    IncrementalStyleRecalc style_recalc;
    style_recalc.RecalcStyle(doc_.get());
    
    // 验证：所有节点都被处理
    int recalculated = style_recalc.GetNodesRecalculated();
    EXPECT_GE(recalculated, 4) << "SubtreeStyleChange 应该重算整个子树";
}

/**
 * @brief 测试脏标记清除正确性
 * 
 * Validates: Requirement 2.5, 3.5, 4.5 - 脏标记正确清除
 */
TEST_F(IncrementalUpdateTest, DirtyFlagClearingCorrectness) {
    auto body = doc_->GetBody();
    
    auto root = CreateElement("div");
    auto child = CreateElement("span");
    auto text = CreateTextNode("文本");
    
    child->AppendChild(text);
    root->AppendChild(child);
    body->AppendChild(root);
    
    // 清除所有脏标记
    doc_->ClearNeedsStyleRecalc();
    body->ClearNeedsStyleRecalc();
    root->ClearNeedsStyleRecalc();
    child->ClearNeedsStyleRecalc();
    text->ClearNeedsStyleRecalc();
    
    // 标记脏
    text->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    text->SetNeedsLayout();
    
    // 验证：脏标记已设置
    EXPECT_TRUE(text->NeedsStyleRecalc());
    EXPECT_TRUE(text->NeedsLayoutFlag());
    EXPECT_TRUE(child->ChildNeedsStyleRecalc());
    EXPECT_TRUE(root->ChildNeedsStyleRecalc());
    
    // 执行增量样式重算
    IncrementalStyleRecalc style_recalc;
    style_recalc.RecalcStyle(doc_.get());
    
    // 验证：样式脏标记被清除
    EXPECT_FALSE(text->NeedsStyleRecalc());
    EXPECT_FALSE(child->ChildNeedsStyleRecalc());
    EXPECT_FALSE(root->ChildNeedsStyleRecalc());
    
    // 注意：布局脏标记由布局引擎清除，这里不测试
}

/**
 * @brief 模拟计数器应用的完整更新周期
 * 
 * 这是 test_preact_simple.js 场景的核心验证
 * Validates: Requirements 1.1, 1.2, 1.3
 */
TEST_F(IncrementalUpdateTest, CounterAppUpdateCycle) {
    auto body = doc_->GetBody();
    
    // 构建计数器应用的 DOM 结构
    auto app = CreateElement("div");
    app->SetAttribute("id", "app");
    
    // 标题（静态）
    auto title = CreateElement("h1");
    auto title_text = CreateTextNode("Preact 测试");
    title->AppendChild(title_text);
    
    // 静态测试区域
    auto test1 = CreateElement("div");
    test1->SetAttribute("style", "padding: 20px; background: #f0f0f0;");
    auto test1_text = CreateTextNode("静态文本测试");
    test1->AppendChild(test1_text);
    
    auto test2 = CreateElement("div");
    test2->SetAttribute("style", "padding: 20px; background: #e0e0e0;");
    auto test2_text = CreateTextNode("textContent 测试");
    test2->AppendChild(test2_text);
    
    auto test3 = CreateElement("div");
    test3->SetAttribute("style", "padding: 20px; background: #d0d0d0;");
    auto test3_span = CreateElement("span");
    auto test3_text = CreateTextNode("span 中的文本");
    test3_span->AppendChild(test3_text);
    test3->AppendChild(test3_span);
    
    // 动态计数器区域
    auto counter = CreateElement("div");
    counter->SetAttribute("id", "counter");
    counter->SetAttribute("style", "padding: 20px; background: #c0c0c0; font-size: 24px;");
    auto counter_text = CreateTextNode("计数: 0");
    counter->AppendChild(counter_text);
    
    // 组装 DOM 树
    app->AppendChild(title);
    app->AppendChild(test1);
    app->AppendChild(test2);
    app->AppendChild(test3);
    app->AppendChild(counter);
    body->AppendChild(app);
    
    // 模拟初始渲染完成：清除所有脏标记
    std::function<void(std::shared_ptr<Node>)> clearAll = [&](std::shared_ptr<Node> node) {
        node->ClearNeedsStyleRecalc();
        node->ClearNeedsLayout();
        for (auto& child : node->GetChildNodes()) {
            clearAll(child);
        }
    };
    clearAll(doc_);
    
    // 验证初始状态：所有节点都是干净的
    EXPECT_FALSE(app->NeedsStyleRecalc());
    EXPECT_FALSE(app->ChildNeedsStyleRecalc());
    
    // ========== 模拟计数器更新 ==========
    // 只有 counter_text 的内容变化
    
    // 更新文本内容
    counter_text->SetTextContent("计数: 1");
    
    // 标记脏（模拟 OnTextChanged 的行为）
    counter_text->SetNeedsStyleRecalc(StyleChangeType::kLocalStyleChange);
    counter_text->SetNeedsLayout();
    
    // 验证：只有计数器路径被标记
    EXPECT_TRUE(counter_text->NeedsStyleRecalc());
    EXPECT_TRUE(counter->ChildNeedsStyleRecalc());
    EXPECT_TRUE(app->ChildNeedsStyleRecalc());
    
    // 验证：静态区域保持干净
    EXPECT_FALSE(title->NeedsStyleRecalc());
    EXPECT_FALSE(title->ChildNeedsStyleRecalc());
    EXPECT_FALSE(test1->NeedsStyleRecalc());
    EXPECT_FALSE(test1->ChildNeedsStyleRecalc());
    EXPECT_FALSE(test2->NeedsStyleRecalc());
    EXPECT_FALSE(test2->ChildNeedsStyleRecalc());
    EXPECT_FALSE(test3->NeedsStyleRecalc());
    EXPECT_FALSE(test3->ChildNeedsStyleRecalc());
    
    // 执行增量样式重算
    IncrementalStyleRecalc style_recalc;
    style_recalc.RecalcStyle(doc_.get());
    
    // 验证统计
    int visited = style_recalc.GetNodesVisited();
    int recalculated = style_recalc.GetNodesRecalculated();
    int skipped = style_recalc.GetSubtreesSkipped();
    
    // 应该跳过大部分静态子树
    EXPECT_GT(skipped, 0) << "应该跳过静态子树";
    
    // 计算优化比率
    int total = visited + skipped;
    double optimization_ratio = (total > 0) ? static_cast<double>(skipped) / total : 0.0;
    
    // 优化比率应该大于 0（有节点被跳过）
    EXPECT_GT(optimization_ratio, 0.0) << "优化比率应该大于 0";
    
    // 验证：所有脏标记被清除
    EXPECT_FALSE(counter_text->NeedsStyleRecalc());
    EXPECT_FALSE(counter->ChildNeedsStyleRecalc());
    EXPECT_FALSE(app->ChildNeedsStyleRecalc());
}

} // namespace test
} // namespace mbink
