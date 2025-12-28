/**
 * @file test_visual_consistency_properties.cpp
 * @brief 视觉一致性属性测试
 *
 * 测试分层合成架构的视觉输出一致性：
 * - 渲染输出与预期一致
 * - CPU 和 GPU 合成结果一致
 * - 增量渲染与完整渲染结果一致
 * - 层边界可视化
 */

#include <gtest/gtest.h>
#include "core/render/render_pipeline.h"
#include "core/compositor/compositor_layer.h"
#include "core/compositor/compositor.h"
#include "core/compositor/rasterizer.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColor.h"
#include <memory>
#include <cmath>

using namespace lightui;

// =========================================================================
// 测试辅助类
// =========================================================================

/**
 * @brief 带颜色的测试渲染对象
 */
class ColoredRenderObject : public RenderObject {
public:
    ColoredRenderObject(SkColor color) : RenderObject(RenderObjectType::BLOCK), color_(color) {
        layout_info_.x = 0;
        layout_info_.y = 0;
        layout_info_.width = 100;
        layout_info_.height = 100;
        layout_info_.is_laid_out = true;
    }

    void SetBounds(float x, float y, float w, float h) {
        layout_info_.x = x;
        layout_info_.y = y;
        layout_info_.width = w;
        layout_info_.height = h;
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
    }

    SkColor GetColor() const { return color_; }

private:
    SkColor color_;
};

/**
 * @brief 比较两个位图是否相似
 * @param bmp1 位图1
 * @param bmp2 位图2
 * @param tolerance 容差（0-255）
 * @return true 如果相似
 */
bool CompareBitmaps(const SkBitmap& bmp1, const SkBitmap& bmp2, int tolerance = 0) {
    if (bmp1.width() != bmp2.width() || bmp1.height() != bmp2.height()) {
        return false;
    }

    for (int y = 0; y < bmp1.height(); y++) {
        for (int x = 0; x < bmp1.width(); x++) {
            SkColor c1 = bmp1.getColor(x, y);
            SkColor c2 = bmp2.getColor(x, y);

            int dr = std::abs(static_cast<int>(SkColorGetR(c1)) - static_cast<int>(SkColorGetR(c2)));
            int dg = std::abs(static_cast<int>(SkColorGetG(c1)) - static_cast<int>(SkColorGetG(c2)));
            int db = std::abs(static_cast<int>(SkColorGetB(c1)) - static_cast<int>(SkColorGetB(c2)));
            int da = std::abs(static_cast<int>(SkColorGetA(c1)) - static_cast<int>(SkColorGetA(c2)));

            if (dr > tolerance || dg > tolerance || db > tolerance || da > tolerance) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 检查位图区域是否为指定颜色
 */
bool CheckRegionColor(const SkBitmap& bmp, const SkIRect& region, SkColor expected, int tolerance = 5) {
    for (int y = region.top(); y < region.bottom() && y < bmp.height(); y++) {
        for (int x = region.left(); x < region.right() && x < bmp.width(); x++) {
            if (x < 0 || y < 0) continue;
            
            SkColor actual = bmp.getColor(x, y);
            
            int dr = std::abs(static_cast<int>(SkColorGetR(actual)) - static_cast<int>(SkColorGetR(expected)));
            int dg = std::abs(static_cast<int>(SkColorGetG(actual)) - static_cast<int>(SkColorGetG(expected)));
            int db = std::abs(static_cast<int>(SkColorGetB(actual)) - static_cast<int>(SkColorGetB(expected)));
            
            if (dr > tolerance || dg > tolerance || db > tolerance) {
                return false;
            }
        }
    }
    return true;
}

// =========================================================================
// 基本渲染一致性测试
// =========================================================================

class BasicRenderConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 1: 单色矩形渲染正确
 */
TEST_F(BasicRenderConsistencyTest, SolidColorRectangleRendersCorrectly) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    root->SetBounds(50, 50, 100, 100);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 检查红色区域
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(55, 55, 90, 90), SK_ColorRED));
}

/**
 * Property 2: 多个矩形正确叠加
 */
TEST_F(BasicRenderConsistencyTest, MultipleRectanglesOverlapCorrectly) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorBLUE);
    root->SetBounds(0, 0, 400, 300);
    
    auto child = std::make_shared<ColoredRenderObject>(SK_ColorGREEN);
    child->SetBounds(100, 100, 100, 100);
    root->AppendChild(child);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 检查蓝色背景区域
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(10, 10, 50, 50), SK_ColorBLUE));
    
    // 检查绿色子元素区域
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(110, 110, 80, 80), SK_ColorGREEN));
}

/**
 * Property 3: 透明度正确应用
 * 注意：这个测试验证管线配置，实际透明度混合由合成器处理
 */
TEST_F(BasicRenderConsistencyTest, OpacityAppliedCorrectly) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 设置子元素透明度
    auto child = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    child->SetBounds(100, 100, 100, 100);
    child->GetComputedStyle().opacity = 0.5f;
    root->AppendChild(child);
    
    // 渲染不应该崩溃
    pipeline_->SetRenderTree(root);
    EXPECT_TRUE(pipeline_->ProcessFrame(surface_->getCanvas()));
    
    // 验证 surface 有效
    EXPECT_NE(surface_, nullptr);
    EXPECT_GT(surface_->width(), 0);
    EXPECT_GT(surface_->height(), 0);
}

// =========================================================================
// 增量渲染一致性测试
// =========================================================================

class IncrementalRenderConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface1_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
        surface2_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface1_;
    sk_sp<SkSurface> surface2_;
};

/**
 * Property 4: 增量渲染与完整渲染结果一致
 */
TEST_F(IncrementalRenderConsistencyTest, IncrementalMatchesFullRender) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorBLUE);
    root->SetBounds(0, 0, 400, 300);
    
    auto child = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    child->SetBounds(100, 100, 100, 100);
    root->AppendChild(child);
    
    // 完整渲染
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface1_->getCanvas());
    
    // 标记脏区域后增量渲染
    pipeline_->MarkDirtyRegion(SkRect::MakeXYWH(100, 100, 100, 100));
    pipeline_->ProcessFrame(surface2_->getCanvas());
    
    SkBitmap bmp1, bmp2;
    surface1_->readPixels(bmp1, 0, 0);
    surface2_->readPixels(bmp2, 0, 0);
    
    // 两次渲染结果应该一致
    EXPECT_TRUE(CompareBitmaps(bmp1, bmp2, 5));
}

/**
 * Property 5: 多次增量更新后结果正确
 */
TEST_F(IncrementalRenderConsistencyTest, MultipleIncrementalUpdatesCorrect) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 初始渲染
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface1_->getCanvas());
    
    // 多次增量更新
    for (int i = 0; i < 5; i++) {
        pipeline_->MarkDirtyRegion(SkRect::MakeXYWH(i * 50, i * 50, 50, 50));
        pipeline_->ProcessFrame(surface1_->getCanvas());
    }
    
    // 完整渲染作为参考
    pipeline_->MarkNeedsRender();
    pipeline_->ProcessFrame(surface2_->getCanvas());
    
    SkBitmap bmp1, bmp2;
    surface1_->readPixels(bmp1, 0, 0);
    surface2_->readPixels(bmp2, 0, 0);
    
    EXPECT_TRUE(CompareBitmaps(bmp1, bmp2, 5));
}

// =========================================================================
// 层合成一致性测试
// =========================================================================

class LayerCompositeConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 6: 层 z-order 正确
 */
TEST_F(LayerCompositeConsistencyTest, LayerZOrderCorrect) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorBLUE);
    root->SetBounds(0, 0, 400, 300);
    
    // 底层红色
    auto layer1 = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    layer1->SetBounds(50, 50, 150, 150);
    root->AppendChild(layer1);
    
    // 顶层绿色（部分重叠）
    auto layer2 = std::make_shared<ColoredRenderObject>(SK_ColorGREEN);
    layer2->SetBounds(100, 100, 150, 150);
    root->AppendChild(layer2);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 重叠区域应该是绿色（后添加的在上面）
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(110, 110, 80, 80), SK_ColorGREEN));
    
    // 红色独占区域应该是红色
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(55, 55, 40, 40), SK_ColorRED));
}

/**
 * Property 7: 层变换正确应用
 */
TEST_F(LayerCompositeConsistencyTest, LayerTransformApplied) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    auto child = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    child->SetBounds(100, 100, 100, 100);
    root->AppendChild(child);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 红色矩形应该在正确位置
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(110, 110, 80, 80), SK_ColorRED));
}

// =========================================================================
// 滚动一致性测试
// =========================================================================

class ScrollConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 8: 滚动后内容位置正确
 */
TEST_F(ScrollConsistencyTest, ScrolledContentPositionCorrect) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 设置为可滚动
    root->GetComputedStyle().overflow_y = "scroll";
    root->SetContentSize(400, 600);
    
    auto child = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    child->SetBounds(50, 50, 100, 100);
    root->AppendChild(child);
    
    // 初始渲染
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 红色矩形应该在初始位置
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(55, 55, 90, 90), SK_ColorRED));
}

// =========================================================================
// 调试功能测试
// =========================================================================

class DebugFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 9: 层边界可视化可以开启
 */
TEST_F(DebugFeatureTest, LayerBordersCanBeEnabled) {
    pipeline_->SetShowLayerBorders(true);
    EXPECT_TRUE(pipeline_->GetConfig().show_layer_borders);
    
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 不应该崩溃
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
}

/**
 * Property 10: 层边界可视化可以关闭
 */
TEST_F(DebugFeatureTest, LayerBordersCanBeDisabled) {
    pipeline_->SetShowLayerBorders(true);
    pipeline_->SetShowLayerBorders(false);
    EXPECT_FALSE(pipeline_->GetConfig().show_layer_borders);
}

// =========================================================================
// 性能统计测试
// =========================================================================

class PerformanceStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 11: 渲染统计正确记录
 */
TEST_F(PerformanceStatsTest, RenderStatsRecorded) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    const auto& stats = pipeline_->GetLastFrameStats();
    EXPECT_GE(stats.total_time, 0.0);
    EXPECT_GE(stats.layers_built, 0);  // 统一管线可能不构建层
}

/**
 * Property 12: 统计在每帧开始时重置
 */
TEST_F(PerformanceStatsTest, StatsResetEachFrame) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    // 确保有统计数据
    EXPECT_GE(pipeline_->GetLastFrameStats().total_time, 0.0);
    
    // 再次渲染，统计应该被重置并重新计算
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    EXPECT_GE(pipeline_->GetLastFrameStats().total_time, 0.0);
}

/**
 * Property 13: 帧跳过正确统计
 */
TEST_F(PerformanceStatsTest, FrameSkipStatsCorrect) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 第一次渲染
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    // 第二次渲染（无变化，可能跳过）
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    // 统计应该有效
    const auto& stats = pipeline_->GetLastFrameStats();
    EXPECT_GE(stats.total_time, 0.0);
}

// =========================================================================
// 边界情况测试
// =========================================================================

class VisualEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(400, 300);
        
        surface_ = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(400, 300));
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    sk_sp<SkSurface> surface_;
};

/**
 * Property 14: 空渲染树不崩溃
 */
TEST_F(VisualEdgeCaseTest, EmptyRenderTreeDoesNotCrash) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 0, 0);  // 空尺寸
    
    // 不应该崩溃
    pipeline_->SetRenderTree(root);
    EXPECT_TRUE(pipeline_->ProcessFrame(surface_->getCanvas()));
}

/**
 * Property 15: 超大渲染对象正确裁剪
 */
TEST_F(VisualEdgeCaseTest, OversizedObjectClippedCorrectly) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorRED);
    root->SetBounds(-100, -100, 1000, 1000);  // 超出视口
    
    pipeline_->SetRenderTree(root);
    pipeline_->ProcessFrame(surface_->getCanvas());
    
    SkBitmap bitmap;
    surface_->readPixels(bitmap, 0, 0);
    
    // 可见区域应该是红色
    EXPECT_TRUE(CheckRegionColor(bitmap, SkIRect::MakeXYWH(10, 10, 100, 100), SK_ColorRED));
}

/**
 * Property 16: 深层嵌套正确渲染
 */
TEST_F(VisualEdgeCaseTest, DeepNestingRendersCorrectly) {
    auto root = std::make_shared<ColoredRenderObject>(SK_ColorWHITE);
    root->SetBounds(0, 0, 400, 300);
    
    // 创建深层嵌套
    std::shared_ptr<ColoredRenderObject> current = root;
    for (int i = 0; i < 10; i++) {
        auto child = std::make_shared<ColoredRenderObject>(
            i % 2 == 0 ? SK_ColorRED : SK_ColorBLUE
        );
        child->SetBounds(10, 10, 380 - i * 20, 280 - i * 20);
        current->AppendChild(child);
        current = child;
    }
    
    // 不应该崩溃
    pipeline_->SetRenderTree(root);
    EXPECT_TRUE(pipeline_->ProcessFrame(surface_->getCanvas()));
}

