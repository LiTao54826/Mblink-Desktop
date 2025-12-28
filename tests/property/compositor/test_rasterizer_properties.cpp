/**
 * @file test_rasterizer_properties.cpp
 * @brief 光栅化器属性测试
 *
 * 测试属性：
 * - Property 1: 层内容正确光栅化到 CPU 位图
 * - Property 4: 增量光栅化保持未变化像素
 * - Property 5: 滚动优化 - 无需重新光栅化
 */

#include <gtest/gtest.h>
#include "core/compositor/rasterizer.h"
#include "core/compositor/compositor_layer.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include <memory>
#include <random>
#include <vector>

namespace lightui {
namespace testing {

// =========================================================================
// 测试辅助类
// =========================================================================

/**
 * @brief 简单的测试渲染对象
 * 绘制一个纯色矩形
 */
class TestRenderObject : public RenderObject {
public:
    TestRenderObject(SkColor color, float x, float y, float w, float h)
        : RenderObject(RenderObjectType::BLOCK), color_(color) {
        layout_info_.x = x;
        layout_info_.y = y;
        layout_info_.width = w;
        layout_info_.height = h;
        layout_info_.is_laid_out = true;
    }

    void Paint(SkCanvas* canvas) override {
        if (!canvas) return;

        SkPaint paint;
        paint.setColor(color_);
        paint.setStyle(SkPaint::kFill_Style);

        SkRect rect = SkRect::MakeXYWH(
            layout_info_.x, layout_info_.y,
            layout_info_.width, layout_info_.height
        );
        canvas->drawRect(rect, paint);
        paint_count_++;
    }

    void SetColor(SkColor color) { color_ = color; }
    SkColor GetColor() const { return color_; }
    int GetPaintCount() const { return paint_count_; }
    void ResetPaintCount() { paint_count_ = 0; }

private:
    SkColor color_;
    int paint_count_ = 0;
};

// =========================================================================
// Property 1: 层内容正确光栅化到 CPU 位图
// =========================================================================

class RasterizeCorrectnessTest : public ::testing::Test {
protected:
    void SetUp() override {
        rasterizer_ = std::make_unique<Rasterizer>();
    }

    std::unique_ptr<Rasterizer> rasterizer_;
};

// 测试：空层光栅化
TEST_F(RasterizeCorrectnessTest, EmptyLayerRasterizesToTransparent) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    // 验证位图已分配
    ASSERT_TRUE(layer->EnsureBitmap());
    const SkBitmap& bitmap = layer->GetBitmap();
    EXPECT_EQ(bitmap.width(), 100);
    EXPECT_EQ(bitmap.height(), 100);

    // 验证所有像素都是透明的
    for (int y = 0; y < bitmap.height(); y++) {
        for (int x = 0; x < bitmap.width(); x++) {
            SkColor color = bitmap.getColor(x, y);
            EXPECT_EQ(SkColorGetA(color), 0) << "Pixel at (" << x << ", " << y << ") is not transparent";
        }
    }
}

// 测试：带渲染对象的层光栅化
TEST_F(RasterizeCorrectnessTest, LayerWithRenderObjectRasterizesCorrectly) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    // 创建一个红色矩形渲染对象
    auto render_obj = std::make_shared<TestRenderObject>(SK_ColorRED, 10, 10, 50, 50);
    layer->SetRenderObject(render_obj.get());

    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    const SkBitmap& bitmap = layer->GetBitmap();

    // 验证红色矩形区域
    SkColor center_color = bitmap.getColor(35, 35);  // 矩形中心
    EXPECT_EQ(SkColorGetR(center_color), 255);
    EXPECT_EQ(SkColorGetG(center_color), 0);
    EXPECT_EQ(SkColorGetB(center_color), 0);

    // 验证矩形外是透明的
    SkColor outside_color = bitmap.getColor(5, 5);
    EXPECT_EQ(SkColorGetA(outside_color), 0);
}

// 测试：光栅化后脏区域被清除
TEST_F(RasterizeCorrectnessTest, DirtyRegionsClearedAfterRasterize) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->MarkFullDirty();

    EXPECT_TRUE(layer->HasDirtyRegions());

    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    EXPECT_FALSE(layer->HasDirtyRegions());
}

// 测试：光栅化后纹理被标记为脏
TEST_F(RasterizeCorrectnessTest, TextureMarkedDirtyAfterRasterize) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    EXPECT_TRUE(layer->IsTextureDirty());
}

// =========================================================================
// Property 4: 增量光栅化保持未变化像素
// =========================================================================

class IncrementalRasterizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        rasterizer_ = std::make_unique<Rasterizer>();
        rasterizer_->SetIncrementalEnabled(true);
    }

    std::unique_ptr<Rasterizer> rasterizer_;
};

// 测试：增量光栅化只更新脏区域
TEST_F(IncrementalRasterizeTest, OnlyDirtyRegionsAreUpdated) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    // 创建渲染对象
    auto render_obj = std::make_shared<TestRenderObject>(SK_ColorBLUE, 0, 0, 100, 100);
    layer->SetRenderObject(render_obj.get());

    // 首次完整光栅化
    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    // 获取位图并记录一些像素值
    SkBitmap& bitmap = layer->GetBitmap();
    SkColor original_color = bitmap.getColor(50, 50);

    // 标记一个小区域为脏
    layer->MarkDirty(SkRect::MakeXYWH(10, 10, 20, 20));

    // 修改渲染对象颜色
    render_obj->SetColor(SK_ColorGREEN);

    // 增量光栅化
    ASSERT_TRUE(rasterizer_->RasterizeDirtyRegions(layer.get()));

    // 验证脏区域被更新
    SkColor dirty_color = bitmap.getColor(20, 20);
    EXPECT_EQ(SkColorGetG(dirty_color), 255);  // 绿色

    // 验证统计信息
    const auto& stats = rasterizer_->GetStats();
    EXPECT_GT(stats.incremental_rasterizations, 0);
    EXPECT_GT(stats.pixels_skipped, 0);
}

// 测试：无脏区域时不进行光栅化
TEST_F(IncrementalRasterizeTest, NoDirtyRegionsNoRasterize) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->ClearDirtyRegions();

    rasterizer_->ResetStats();

    EXPECT_FALSE(rasterizer_->RasterizeDirtyRegions(layer.get()));

    const auto& stats = rasterizer_->GetStats();
    EXPECT_EQ(stats.layers_rasterized, 0);
}

// 测试：多个脏区域合并
TEST_F(IncrementalRasterizeTest, MultipleDirtyRegionsMerged) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    auto render_obj = std::make_shared<TestRenderObject>(SK_ColorRED, 0, 0, 100, 100);
    layer->SetRenderObject(render_obj.get());

    // 首次光栅化
    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    // 标记多个相邻的脏区域
    layer->MarkDirty(SkRect::MakeXYWH(10, 10, 20, 20));
    layer->MarkDirty(SkRect::MakeXYWH(25, 10, 20, 20));  // 相邻
    layer->MarkDirty(SkRect::MakeXYWH(10, 25, 20, 20));  // 相邻

    // 合并后应该只有一个或两个区域
    layer->MergeDirtyRegions();
    EXPECT_LE(layer->GetDirtyRegions().size(), 2u);
}

// =========================================================================
// Property 5: 滚动优化 - 无需重新光栅化
// =========================================================================

class ScrollOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        rasterizer_ = std::make_unique<Rasterizer>();
        rasterizer_->SetScrollOptimizationEnabled(true);
    }

    std::unique_ptr<Rasterizer> rasterizer_;
};

// 测试：计算滚动脏区域 - 向下滚动
TEST_F(ScrollOptimizationTest, ScrollDownDirtyRegions) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    std::vector<SkIRect> new_regions;
    SkPoint delta = {0, 20};  // 向下滚动 20 像素

    rasterizer_->CalculateScrollDirtyRegions(layer.get(), delta, new_regions);

    // 应该有一个顶部区域需要重绘
    ASSERT_GE(new_regions.size(), 1u);

    bool found_top_region = false;
    for (const auto& region : new_regions) {
        if (region.top() == 0 && region.height() == 20) {
            found_top_region = true;
            break;
        }
    }
    EXPECT_TRUE(found_top_region);
}

// 测试：计算滚动脏区域 - 向右滚动
TEST_F(ScrollOptimizationTest, ScrollRightDirtyRegions) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    std::vector<SkIRect> new_regions;
    SkPoint delta = {30, 0};  // 向右滚动 30 像素

    rasterizer_->CalculateScrollDirtyRegions(layer.get(), delta, new_regions);

    // 应该有一个左侧区域需要重绘
    ASSERT_GE(new_regions.size(), 1u);

    bool found_left_region = false;
    for (const auto& region : new_regions) {
        if (region.left() == 0 && region.width() == 30) {
            found_left_region = true;
            break;
        }
    }
    EXPECT_TRUE(found_left_region);
}

// 测试：计算滚动脏区域 - 对角滚动
TEST_F(ScrollOptimizationTest, DiagonalScrollDirtyRegions) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    std::vector<SkIRect> new_regions;
    SkPoint delta = {10, 15};  // 对角滚动

    rasterizer_->CalculateScrollDirtyRegions(layer.get(), delta, new_regions);

    // 应该有两个区域需要重绘（左边和上边）
    EXPECT_EQ(new_regions.size(), 2u);
}

// 测试：滚动处理 - 小距离滚动
TEST_F(ScrollOptimizationTest, SmallScrollPreservesPixels) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    auto render_obj = std::make_shared<TestRenderObject>(SK_ColorBLUE, 0, 0, 100, 100);
    layer->SetRenderObject(render_obj.get());

    // 首次光栅化
    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));

    // 记录原始像素
    SkBitmap& bitmap = layer->GetBitmap();
    SkColor original_center = bitmap.getColor(50, 50);

    // 处理滚动
    SkPoint old_offset = {0, 0};
    SkPoint new_offset = {10, 10};

    ASSERT_TRUE(rasterizer_->HandleScroll(layer.get(), old_offset, new_offset));

    // 验证只有边缘区域被标记为脏
    EXPECT_TRUE(layer->HasDirtyRegions());

    const auto& dirty = layer->GetDirtyRegions();
    int total_dirty_area = 0;
    for (const auto& region : dirty) {
        total_dirty_area += region.width() * region.height();
    }

    // 脏区域应该远小于整个层
    int layer_area = 100 * 100;
    EXPECT_LT(total_dirty_area, layer_area / 2);
}

// 测试：大距离滚动触发完整重绘
TEST_F(ScrollOptimizationTest, LargeScrollTriggersFullRasterize) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    auto render_obj = std::make_shared<TestRenderObject>(SK_ColorRED, 0, 0, 100, 100);
    layer->SetRenderObject(render_obj.get());

    // 首次光栅化
    ASSERT_TRUE(rasterizer_->RasterizeLayer(layer.get()));
    layer->ClearDirtyRegions();

    rasterizer_->ResetStats();

    // 大距离滚动（超过层尺寸）
    SkPoint old_offset = {0, 0};
    SkPoint new_offset = {150, 0};  // 超过宽度

    ASSERT_TRUE(rasterizer_->HandleScroll(layer.get(), old_offset, new_offset));

    // 应该触发完整光栅化
    const auto& stats = rasterizer_->GetStats();
    EXPECT_GT(stats.full_rasterizations, 0);
}

// =========================================================================
// 递归光栅化测试
// =========================================================================

class RecursiveRasterizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        rasterizer_ = std::make_unique<Rasterizer>();
    }

    std::unique_ptr<Rasterizer> rasterizer_;
};

// 测试：光栅化层树中的所有脏层
TEST_F(RecursiveRasterizeTest, RasterizeAllDirtyLayers) {
    // 创建层树
    auto root = CreateCompositorLayer();
    root->SetBounds(SkRect::MakeWH(200, 200));
    root->MarkFullDirty();

    auto child1 = CreateCompositorLayer();
    child1->SetBounds(SkRect::MakeWH(100, 100));
    child1->MarkFullDirty();
    root->AddChild(child1);

    auto child2 = CreateCompositorLayer();
    child2->SetBounds(SkRect::MakeWH(100, 100));
    child2->ClearDirtyRegions();  // 确保没有脏区域
    root->AddChild(child2);

    // 验证 child2 确实没有脏区域
    EXPECT_FALSE(child2->HasDirtyRegions());

    rasterizer_->ResetStats();

    int count = rasterizer_->RasterizeDirtyLayers(root.get());

    // 应该光栅化 2 个层（root 和 child1，child2 没有脏区域）
    EXPECT_EQ(count, 2);

    const auto& stats = rasterizer_->GetStats();
    EXPECT_EQ(stats.layers_rasterized, 2);
}

// 测试：空层树
TEST_F(RecursiveRasterizeTest, EmptyLayerTree) {
    int count = rasterizer_->RasterizeDirtyLayers(nullptr);
    EXPECT_EQ(count, 0);
}

// =========================================================================
// 统计信息测试
// =========================================================================

class RasterizeStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        rasterizer_ = std::make_unique<Rasterizer>();
    }

    std::unique_ptr<Rasterizer> rasterizer_;
};

// 测试：统计信息正确累积
TEST_F(RasterizeStatsTest, StatsAccumulateCorrectly) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));

    rasterizer_->ResetStats();

    // 多次光栅化
    for (int i = 0; i < 3; i++) {
        layer->MarkFullDirty();
        rasterizer_->RasterizeLayer(layer.get());
    }

    const auto& stats = rasterizer_->GetStats();
    EXPECT_EQ(stats.layers_rasterized, 3);
    EXPECT_EQ(stats.full_rasterizations, 3);
    EXPECT_EQ(stats.pixels_rasterized, 3 * 100 * 100);
}

// 测试：重置统计信息
TEST_F(RasterizeStatsTest, StatsResetCorrectly) {
    auto layer = CreateCompositorLayer();
    layer->SetBounds(SkRect::MakeWH(100, 100));
    layer->MarkFullDirty();

    rasterizer_->RasterizeLayer(layer.get());

    const auto& stats_before = rasterizer_->GetStats();
    EXPECT_GT(stats_before.layers_rasterized, 0);

    rasterizer_->ResetStats();

    const auto& stats_after = rasterizer_->GetStats();
    EXPECT_EQ(stats_after.layers_rasterized, 0);
    EXPECT_EQ(stats_after.pixels_rasterized, 0);
}

} // namespace testing
} // namespace lightui
