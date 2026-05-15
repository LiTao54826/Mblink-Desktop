/**
 * @file test_compositor_properties.cpp
 * @brief 合成器属性测试
 *
 * 测试属性：
 * - Property 6: Z-order 合成正确性
 * - Property 8: 无变化时跳过帧
 */

#include <gtest/gtest.h>
#include "core/compositor/compositor.h"
#include "core/compositor/compositor_layer.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkBitmap.h"
#include <memory>

namespace mbink {
namespace testing {

// =========================================================================
// 测试辅助函数
// =========================================================================

/**
 * @brief 创建带颜色的测试层
 */
std::shared_ptr<CompositorLayer> CreateColoredLayer(SkColor color, float x, float y, float w, float h) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeXYWH(x, y, w, h));
    layer->EnsureBitmap();

    SkCanvas* canvas = layer->GetCanvas();
    if (canvas) {
        canvas->clear(color);
    }

    return layer;
}

// =========================================================================
// Property 6: Z-order 合成正确性
// =========================================================================

class ZOrderCompositeTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        // 使用 CPU 模式进行测试
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：子层在父层之上
TEST_F(ZOrderCompositeTest, ChildLayerAboveParent) {
    // 创建父层（红色）
    auto parent = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    // 创建子层（蓝色，覆盖父层中心）
    auto child = CreateColoredLayer(SK_ColorBLUE, 25, 25, 50, 50);
    parent->AddChild(child);

    // 创建目标 Canvas
    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    // 合成
    ASSERT_TRUE(compositor_->CompositeToCanvas(parent.get(), &canvas));

    // 验证：父层区域是红色
    SkColor corner_color = result.getColor(10, 10);
    EXPECT_EQ(SkColorGetR(corner_color), 255);
    EXPECT_EQ(SkColorGetG(corner_color), 0);
    EXPECT_EQ(SkColorGetB(corner_color), 0);

    // 验证：子层区域是蓝色（覆盖父层）
    SkColor center_color = result.getColor(50, 50);
    EXPECT_EQ(SkColorGetR(center_color), 0);
    EXPECT_EQ(SkColorGetG(center_color), 0);
    EXPECT_EQ(SkColorGetB(center_color), 255);
}

// 测试：多个子层按顺序合成
TEST_F(ZOrderCompositeTest, MultipleChildrenInOrder) {
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    // 第一个子层（红色）
    auto child1 = CreateColoredLayer(SK_ColorRED, 10, 10, 80, 80);
    root->AddChild(child1);

    // 第二个子层（绿色，部分覆盖第一个）
    auto child2 = CreateColoredLayer(SK_ColorGREEN, 50, 50, 80, 80);
    root->AddChild(child2);

    // 第三个子层（蓝色，部分覆盖前两个）
    auto child3 = CreateColoredLayer(SK_ColorBLUE, 90, 90, 80, 80);
    root->AddChild(child3);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 验证：只有 child1 的区域是红色
    SkColor red_area = result.getColor(20, 20);
    EXPECT_EQ(SkColorGetR(red_area), 255);

    // 验证：child2 覆盖 child1 的区域是绿色
    SkColor green_area = result.getColor(60, 60);
    EXPECT_EQ(SkColorGetG(green_area), 255);

    // 验证：child3 覆盖前两个的区域是蓝色
    SkColor blue_area = result.getColor(100, 100);
    EXPECT_EQ(SkColorGetB(blue_area), 255);
}

// 测试：嵌套层的 z-order
TEST_F(ZOrderCompositeTest, NestedLayersZOrder) {
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    auto parent1 = CreateColoredLayer(SK_ColorRED, 10, 10, 100, 100);
    root->AddChild(parent1);

    auto child_of_parent1 = CreateColoredLayer(SK_ColorYELLOW, 20, 20, 60, 60);
    parent1->AddChild(child_of_parent1);

    auto parent2 = CreateColoredLayer(SK_ColorBLUE, 80, 80, 100, 100);
    root->AddChild(parent2);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // parent2 应该在 parent1 及其子层之上
    // 检查 parent2 覆盖 child_of_parent1 的区域
    SkColor overlap_color = result.getColor(90, 90);
    EXPECT_EQ(SkColorGetB(overlap_color), 255);  // 蓝色
}

// =========================================================================
// Property 8: 无变化时跳过帧
// =========================================================================

class FrameSkipTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
        compositor_->SetFrameSkipEnabled(true);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：首帧不跳过
TEST_F(FrameSkipTest, FirstFrameNotSkipped) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);
    root->ClearDirtyRegions();

    compositor_->ResetStats();
    compositor_->BeginFrame();

    ASSERT_TRUE(compositor_->Composite(root.get()));

    auto info = compositor_->EndFrame();

    // 首帧不应该跳过
    EXPECT_FALSE(info.frame_skipped);
    EXPECT_EQ(compositor_->GetStats().frames_composited, 1);
}

// 测试：无变化时跳过帧
TEST_F(FrameSkipTest, SkipFrameWhenNoChanges) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    // 首帧
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    compositor_->EndFrame();

    // 清除脏区域
    root->ClearDirtyRegions();

    compositor_->ResetStats();

    // 第二帧（无变化）
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    auto info = compositor_->EndFrame();

    // 应该跳过
    EXPECT_TRUE(info.frame_skipped);
    EXPECT_EQ(compositor_->GetStats().frames_skipped, 1);
}

// 测试：有变化时不跳过
TEST_F(FrameSkipTest, NoSkipWhenDirty) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    // 首帧
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    compositor_->EndFrame();

    // 标记脏区域
    root->MarkDirty(SkRect::MakeXYWH(10, 10, 20, 20));

    compositor_->ResetStats();

    // 第二帧（有变化）
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    auto info = compositor_->EndFrame();

    // 不应该跳过
    EXPECT_FALSE(info.frame_skipped);
    EXPECT_EQ(compositor_->GetStats().frames_composited, 1);
}

// 测试：禁用帧跳过
TEST_F(FrameSkipTest, DisabledFrameSkip) {
    compositor_->SetFrameSkipEnabled(false);

    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    // 首帧
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    compositor_->EndFrame();

    root->ClearDirtyRegions();

    compositor_->ResetStats();

    // 第二帧（无变化，但禁用了跳过）
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    auto info = compositor_->EndFrame();

    // 不应该跳过（因为禁用了）
    EXPECT_FALSE(info.frame_skipped);
    EXPECT_EQ(compositor_->GetStats().frames_composited, 1);
}

// 测试：MarkNeedsComposite 强制合成
TEST_F(FrameSkipTest, MarkNeedsCompositeForceComposite) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    // 首帧
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    compositor_->EndFrame();

    root->ClearDirtyRegions();

    // 强制标记需要合成
    compositor_->MarkNeedsComposite();

    compositor_->ResetStats();

    // 第二帧
    compositor_->BeginFrame();
    ASSERT_TRUE(compositor_->Composite(root.get()));
    auto info = compositor_->EndFrame();

    // 不应该跳过
    EXPECT_FALSE(info.frame_skipped);
}

// =========================================================================
// 透明度合成测试
// =========================================================================

class OpacityCompositeTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：半透明层合成
TEST_F(OpacityCompositeTest, SemiTransparentLayer) {
    // 创建白色背景
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    // 创建半透明红色层
    auto child = CreateColoredLayer(SK_ColorRED, 50, 50, 100, 100);
    child->SetOpacity(0.5f);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 验证：子层区域应该是混合色（红色 + 白色）
    SkColor blended = result.getColor(100, 100);

    // 半透明红色在白色上应该产生粉红色
    // 预期：R ≈ 255, G ≈ 128, B ≈ 128
    EXPECT_GT(SkColorGetR(blended), 200);
    EXPECT_GT(SkColorGetG(blended), 100);
    EXPECT_GT(SkColorGetB(blended), 100);
}

// 测试：完全透明层不可见
TEST_F(OpacityCompositeTest, FullyTransparentLayerInvisible) {
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    auto child = CreateColoredLayer(SK_ColorRED, 50, 50, 100, 100);
    child->SetOpacity(0.0f);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 验证：子层区域应该是白色（完全透明）
    SkColor color = result.getColor(100, 100);
    EXPECT_EQ(SkColorGetR(color), 255);
    EXPECT_EQ(SkColorGetG(color), 255);
    EXPECT_EQ(SkColorGetB(color), 255);
}

// =========================================================================
// 变换合成测试
// =========================================================================

class TransformCompositeTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：平移变换
TEST_F(TransformCompositeTest, TranslateTransform) {
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    auto child = CreateColoredLayer(SK_ColorRED, 0, 0, 50, 50);

    // 平移 (50, 50)
    SkMatrix transform = SkMatrix::Translate(50, 50);
    child->SetTransform(transform);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 原位置应该是白色
    SkColor original_pos = result.getColor(10, 10);
    EXPECT_EQ(SkColorGetR(original_pos), 255);
    EXPECT_EQ(SkColorGetG(original_pos), 255);

    // 平移后位置应该是红色
    SkColor translated_pos = result.getColor(60, 60);
    EXPECT_EQ(SkColorGetR(translated_pos), 255);
    EXPECT_EQ(SkColorGetG(translated_pos), 0);
}

// 测试：缩放变换
TEST_F(TransformCompositeTest, ScaleTransform) {
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);

    auto child = CreateColoredLayer(SK_ColorRED, 0, 0, 50, 50);

    // 缩放 2x
    SkMatrix transform = SkMatrix::Scale(2.0f, 2.0f);
    child->SetTransform(transform);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 缩放后，50x50 变成 100x100
    // 检查 (75, 75) 应该是红色
    SkColor scaled_area = result.getColor(75, 75);
    EXPECT_EQ(SkColorGetR(scaled_area), 255);
    EXPECT_EQ(SkColorGetG(scaled_area), 0);
}

// =========================================================================
// 统计信息测试
// =========================================================================

class CompositorStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：统计信息正确累积
TEST_F(CompositorStatsTest, StatsAccumulate) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 100, 100);

    compositor_->ResetStats();
    compositor_->SetFrameSkipEnabled(false);

    for (int i = 0; i < 3; i++) {
        compositor_->BeginFrame();
        compositor_->Composite(root.get());
        compositor_->EndFrame();
    }

    const auto& stats = compositor_->GetStats();
    EXPECT_EQ(stats.frames_composited, 3);
}

// 测试：层数统计
TEST_F(CompositorStatsTest, LayerCountStats) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 200, 200);
    auto child1 = CreateColoredLayer(SK_ColorGREEN, 10, 10, 50, 50);
    auto child2 = CreateColoredLayer(SK_ColorBLUE, 60, 60, 50, 50);
    root->AddChild(child1);
    root->AddChild(child2);

    compositor_->ResetStats();
    compositor_->SetFrameSkipEnabled(false);

    compositor_->BeginFrame();
    compositor_->Composite(root.get());
    auto info = compositor_->EndFrame();

    // 3 个层
    EXPECT_EQ(info.layers_composited, 3);
}

TEST_F(CompositorStatsTest, CompositeToCanvasUpdatesStats) {
    auto root = CreateColoredLayer(SK_ColorRED, 0, 0, 200, 200);
    auto child = CreateColoredLayer(SK_ColorGREEN, 10, 10, 50, 50);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    compositor_->ResetStats();

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    const auto& stats = compositor_->GetStats();
    EXPECT_EQ(stats.frames_composited, 1);
    EXPECT_EQ(stats.frames_skipped, 0);
    EXPECT_EQ(stats.layers_composited, 2);
    EXPECT_GT(stats.composite_time_ms, 0.0);

    const auto& frame_info = compositor_->GetLastFrameInfo();
    EXPECT_FALSE(frame_info.frame_skipped);
    EXPECT_EQ(frame_info.layers_composited, 2);
    EXPECT_GT(frame_info.composite_time_ms, 0.0);
    EXPECT_GT(frame_info.total_time_ms, 0.0);
}

// =========================================================================
// 调试功能测试
// =========================================================================

class CompositorDebugTest : public ::testing::Test {
protected:
    void SetUp() override {
        compositor_ = std::make_unique<Compositor>();
        compositor_->Initialize(200, 200);
        compositor_->SetGPUEnabled(false);
    }

    std::unique_ptr<Compositor> compositor_;
};

// 测试：层边界显示
TEST_F(CompositorDebugTest, ShowLayerBorders) {
    compositor_->SetShowLayerBorders(true);
    EXPECT_TRUE(compositor_->IsShowingLayerBorders());

    // 创建一个有子层的根层，边界会在子层上绘制
    auto root = CreateColoredLayer(SK_ColorWHITE, 0, 0, 200, 200);
    root->SetPromotionReason(LayerPromotionReason::RootLayer);

    auto child = CreateColoredLayer(SK_ColorWHITE, 10, 10, 50, 50);
    child->SetPromotionReason(LayerPromotionReason::PositionFixed);
    root->AddChild(child);

    SkBitmap result;
    result.allocN32Pixels(200, 200);
    SkCanvas canvas(result);

    ASSERT_TRUE(compositor_->CompositeToCanvas(root.get(), &canvas));

    // 检查子层边界位置（蓝色用于 PositionFixed）
    // 边界在 (10, 10) 到 (60, 60)
    SkColor border_color = result.getColor(11, 11);

    // 边界颜色应该是蓝色（PositionFixed）
    bool has_blue_border = (SkColorGetB(border_color) > 200);
    EXPECT_TRUE(has_blue_border);
}

} // namespace testing
} // namespace mbink
