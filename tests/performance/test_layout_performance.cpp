/**
 * @file test_layout_performance.cpp
 * @brief 布局引擎性能测试
 *
 * 测试内容：
 * 1. 大文档布局性能 (1000+ 元素)
 * 2. 深度嵌套布局性能 (50+ 层)
 * 3. 复杂 Flex/Grid 布局性能
 * 4. 布局缓存效率
 * 5. 重复布局性能
 */

#include <gtest/gtest.h>
#include "performance_utils.h"
#include "core/layout/native_layout_engine.h"
#include "core/render/render_object.h"
#include <memory>
#include <vector>

using namespace mbink::performance;
using namespace lightui;

class LayoutPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<NativeLayoutEngine>();
    }

    void TearDown() override {
        engine_->Clear();
    }

    // 创建简单的渲染对象
    std::shared_ptr<RenderObject> CreateRenderObject(
        RenderObjectType type = RenderObjectType::BLOCK) {
        auto obj = std::make_shared<RenderObject>(type);
        return obj;
    }

    // 创建 Flex 容器
    std::shared_ptr<RenderObject> CreateFlexContainer(
        const std::string& direction = "row",
        const std::string& jc = "flex-start",
        const std::string& ai = "stretch") {
        auto obj = std::make_shared<RenderObject>(RenderObjectType::FLEX);
        ComputedStyle& style = obj->GetComputedStyle();
        style.display = RenderObjectType::FLEX;
        style.flex_direction = direction;
        style.justify_content = jc;
        style.align_items = ai;
        return obj;
    }

    // 创建 Grid 容器
    std::shared_ptr<RenderObject> CreateGridContainer(
        const std::string& cols = "1fr 1fr",
        const std::string& rows = "auto") {
        auto obj = std::make_shared<RenderObject>(RenderObjectType::GRID);
        ComputedStyle& style = obj->GetComputedStyle();
        style.display = RenderObjectType::GRID;
        style.grid_template_columns = cols;
        style.grid_template_rows = rows;
        return obj;
    }

    // 创建带尺寸的子元素
    std::shared_ptr<RenderObject> CreateSizedChild(float width, float height) {
        auto obj = std::make_shared<RenderObject>(RenderObjectType::BLOCK);
        ComputedStyle& style = obj->GetComputedStyle();
        style.width = CSSLength(width, CSSUnit::PX);
        style.height = CSSLength(height, CSSUnit::PX);
        return obj;
    }

    std::unique_ptr<NativeLayoutEngine> engine_;
    PerformanceReporter reporter_;
};

// ============================================================================
// 1. 大文档布局性能测试
// ============================================================================

TEST_F(LayoutPerformanceTest, LargeDocument_1000Elements) {
    auto root = CreateRenderObject(RenderObjectType::BLOCK);

    for (int i = 0; i < 1000; i++) {
        auto child = CreateSizedChild(100, 30);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("1000 Block Elements", elapsed, "ms", "Large Document");

    std::cout << "1000 block elements layout: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 100.0) << "1000 elements layout should be < 100ms";
}

TEST_F(LayoutPerformanceTest, LargeDocument_5000Elements) {
    auto root = CreateRenderObject(RenderObjectType::BLOCK);

    for (int i = 0; i < 5000; i++) {
        auto child = CreateSizedChild(100, 30);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("5000 Block Elements", elapsed, "ms", "Large Document");

    std::cout << "5000 block elements layout: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 500.0) << "5000 elements layout should be < 500ms";
}

// ============================================================================
// 2. 深度嵌套布局性能测试
// ============================================================================

TEST_F(LayoutPerformanceTest, DeepNested_50Levels) {
    auto root = CreateRenderObject(RenderObjectType::BLOCK);
    auto current = root;

    for (int i = 0; i < 50; i++) {
        auto child = CreateRenderObject(RenderObjectType::BLOCK);
        ComputedStyle& style = child->GetComputedStyle();
        style.padding.top = CSSLength(5, CSSUnit::PX);
        style.padding.right = CSSLength(5, CSSUnit::PX);
        style.padding.bottom = CSSLength(5, CSSUnit::PX);
        style.padding.left = CSSLength(5, CSSUnit::PX);
        current->AppendChild(child);
        current = child;
    }

    auto innermost = CreateSizedChild(100, 50);
    current->AppendChild(innermost);

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("50 Nested Levels", elapsed, "ms", "Deep Nesting");

    std::cout << "50 levels deep nesting layout: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 50.0) << "50 levels nested layout should be < 50ms";
}

// ============================================================================
// 3. Flex 布局性能测试
// ============================================================================

TEST_F(LayoutPerformanceTest, FlexLayout_100Items) {
    auto root = CreateFlexContainer("row", "space-between", "stretch");
    ComputedStyle& rootStyle = root->GetComputedStyle();
    rootStyle.flex_wrap = "wrap";

    for (int i = 0; i < 100; i++) {
        auto child = CreateSizedChild(80, 40);
        ComputedStyle& style = child->GetComputedStyle();
        style.flex_grow = 1;
        style.margin.top = CSSLength(5, CSSUnit::PX);
        style.margin.right = CSSLength(5, CSSUnit::PX);
        style.margin.bottom = CSSLength(5, CSSUnit::PX);
        style.margin.left = CSSLength(5, CSSUnit::PX);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("Flex 100 Items (wrap)", elapsed, "ms", "Flex Layout");

    std::cout << "Flex 100 items with wrap: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 50.0) << "Flex 100 items layout should be < 50ms";
}

TEST_F(LayoutPerformanceTest, FlexLayout_NestedFlex) {
    auto root = CreateFlexContainer("column", "flex-start", "stretch");

    for (int i = 0; i < 10; i++) {
        auto row = CreateFlexContainer("row", "space-around", "stretch");
        for (int j = 0; j < 10; j++) {
            auto child = CreateSizedChild(60, 30);
            row->AppendChild(child);
        }
        root->AppendChild(row);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("Nested Flex (10x10)", elapsed, "ms", "Flex Layout");

    std::cout << "Nested flex 10x10: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 30.0) << "Nested flex layout should be < 30ms";
}

// ============================================================================
// 4. Grid 布局性能测试
// ============================================================================

TEST_F(LayoutPerformanceTest, GridLayout_10x10) {
    auto root = CreateGridContainer("repeat(10, 1fr)", "repeat(10, auto)");

    for (int i = 0; i < 100; i++) {
        auto child = CreateSizedChild(50, 30);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();
    engine_->ComputeLayout(800, 600);
    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    reporter_.AddResult("Grid 10x10", elapsed, "ms", "Grid Layout");

    std::cout << "Grid 10x10: " << elapsed << " ms\n";
    EXPECT_LT(elapsed, 30.0) << "Grid 10x10 layout should be < 30ms";
}

// ============================================================================
// 5. 重复布局性能测试
// ============================================================================

TEST_F(LayoutPerformanceTest, RepeatedLayout_100Times) {
    auto root = CreateFlexContainer();
    for (int i = 0; i < 50; i++) {
        auto child = CreateSizedChild(60, 30);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer;
    timer.Start();

    for (int i = 0; i < 100; i++) {
        engine_->ComputeLayout(800, 600);
    }

    timer.Stop();

    double elapsed = timer.GetElapsedMs();
    double avgTime = elapsed / 100.0;

    reporter_.AddResult("100 Repeated Layouts (total)", elapsed, "ms", "Repeated Layout");
    reporter_.AddResult("100 Repeated Layouts (avg)", avgTime, "ms", "Repeated Layout");

    std::cout << "100 repeated layouts: " << elapsed << " ms (avg: " << avgTime << " ms)\n";
    EXPECT_LT(avgTime, 5.0) << "Average layout time should be < 5ms";
}

// ============================================================================
// 6. 缓存效率测试
// ============================================================================

TEST_F(LayoutPerformanceTest, CacheEfficiency) {
    auto root = CreateFlexContainer();
    for (int i = 0; i < 100; i++) {
        auto child = CreateSizedChild(60, 30);
        root->AppendChild(child);
    }

    engine_->BuildLayoutTree(root);

    PerformanceTimer timer1;
    timer1.Start();
    engine_->ComputeLayout(800, 600);
    timer1.Stop();
    double firstLayout = timer1.GetElapsedMs();

    PerformanceTimer timer2;
    timer2.Start();
    engine_->ComputeLayout(800, 600);
    timer2.Stop();
    double secondLayout = timer2.GetElapsedMs();

    reporter_.AddResult("First Layout (cold)", firstLayout, "ms", "Cache Efficiency");
    reporter_.AddResult("Second Layout (cached)", secondLayout, "ms", "Cache Efficiency");

    std::cout << "First layout: " << firstLayout << " ms\n";
    std::cout << "Second layout: " << secondLayout << " ms\n";

    if (firstLayout > 0.1) {
        double speedup = firstLayout / secondLayout;
        std::cout << "Cache speedup: " << speedup << "x\n";
        EXPECT_GT(speedup, 1.5) << "Cached layout should be faster";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

