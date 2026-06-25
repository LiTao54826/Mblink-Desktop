/**
 * @file test_render_pipeline_properties.cpp
 * @brief 统一渲染管线属性测试
 *
 * 测试渲染管线的核心属性：
 * - 初始化和配置
 * - 渲染流程
 * - 滚动处理
 * - 动画处理
 * - 脏区域管理
 */

#include <gtest/gtest.h>
#include "core/render/pipeline/render_pipeline.h"
#include "core/compositor/compositor_layer.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include <memory>

using namespace mblink;

// =========================================================================
// 测试辅助类
// =========================================================================

/**
 * @brief 测试用渲染对象
 */
class TestRenderObject : public RenderObject {
public:
    TestRenderObject() : RenderObject(RenderObjectType::BLOCK) {
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

    void SetScrollable(bool scrollable) {
        if (scrollable) {
            computed_style_.overflow_y = "scroll";
            content_width_ = layout_info_.width * 2;
            content_height_ = layout_info_.height * 3;
        } else {
            computed_style_.overflow_y = "visible";
        }
    }

    void SetFixed(bool fixed) {
        computed_style_.position = fixed ? "fixed" : "static";
    }

    void SetPaintColor(SkColor color) {
        paint_color_ = color;
    }

    void Paint(SkCanvas* canvas) override {
        if (!canvas) {
            return;
        }

        canvas->save();
        canvas->translate(layout_info_.x, layout_info_.y);

        if (SkColorGetA(paint_color_) != 0) {
            SkPaint paint;
            paint.setColor(paint_color_);
            paint.setStyle(SkPaint::kFill_Style);
            canvas->drawRect(SkRect::MakeWH(layout_info_.width, layout_info_.height), paint);
        }

        const bool clips_scroll_content = IsScrollable();
        if (clips_scroll_content) {
            canvas->save();
            canvas->clipRect(SkRect::MakeWH(layout_info_.width, layout_info_.height));
            canvas->translate(-GetScrollX(), -GetScrollY());
        }

        for (const auto& child : children_) {
            if (!child->HasOwnCompositorLayer()) {
                child->Paint(canvas);
            }
        }

        if (clips_scroll_content) {
            canvas->restore();
        }

        canvas->restore();
        needs_paint_ = false;
    }

private:
    SkColor paint_color_ = SK_ColorTRANSPARENT;
};

// =========================================================================
// 初始化测试
// =========================================================================

class PipelineInitializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
    }

    std::unique_ptr<RenderPipeline> pipeline_;
};

/**
 * Property 1: 管线可以成功初始化
 */
TEST_F(PipelineInitializationTest, PipelineCanBeInitialized) {
    EXPECT_TRUE(pipeline_->Initialize(800, 600));
    EXPECT_TRUE(pipeline_->IsInitialized());
}

/**
 * Property 2: 重复初始化返回 true
 */
TEST_F(PipelineInitializationTest, DuplicateInitializationReturnsTrue) {
    EXPECT_TRUE(pipeline_->Initialize(800, 600));
    EXPECT_TRUE(pipeline_->Initialize(800, 600));
    EXPECT_TRUE(pipeline_->IsInitialized());
}

/**
 * Property 3: 关闭后可以重新初始化
 */
TEST_F(PipelineInitializationTest, CanReinitializeAfterShutdown) {
    pipeline_->Initialize(800, 600);
    pipeline_->Shutdown();
    EXPECT_FALSE(pipeline_->IsInitialized());
    
    EXPECT_TRUE(pipeline_->Initialize(1024, 768));
    EXPECT_TRUE(pipeline_->IsInitialized());
}

/**
 * Property 4: 配置正确应用
 */
TEST_F(PipelineInitializationTest, ConfigurationApplied) {
    UnifiedPipelineConfig config;
    config.enable_layer_promotion = false;
    config.enable_frame_skip = false;
    config.show_layer_borders = true;
    
    pipeline_->Initialize(800, 600, config);
    
    const auto& applied_config = pipeline_->GetConfig();
    EXPECT_FALSE(applied_config.enable_layer_promotion);
    EXPECT_FALSE(applied_config.enable_frame_skip);
    EXPECT_TRUE(applied_config.show_layer_borders);
}

// =========================================================================
// 渲染测试
// =========================================================================

class PipelineRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
        
        root_ = std::make_shared<TestRenderObject>();
        root_->SetBounds(0, 0, 800, 600);
        pipeline_->SetRenderTree(root_);
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    std::shared_ptr<TestRenderObject> root_;
};

/**
 * Property 5: 渲染成功返回 true
 */
TEST_F(PipelineRenderTest, RenderReturnsTrue) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    EXPECT_TRUE(pipeline_->ProcessFrame(surface->getCanvas()));
}

/**
 * Property 6: 渲染后 NeedsUpdate 为 false
 */
TEST_F(PipelineRenderTest, NeedsUpdateFalseAfterRender) {
    EXPECT_TRUE(pipeline_->NeedsUpdate());
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    EXPECT_FALSE(pipeline_->NeedsUpdate());
}

/**
 * Property 7: MarkNeedsRender 设置标志
 */
TEST_F(PipelineRenderTest, MarkNeedsRenderSetsFlag) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    EXPECT_FALSE(pipeline_->NeedsUpdate());
    
    pipeline_->MarkNeedsRender();
    EXPECT_TRUE(pipeline_->NeedsUpdate());
}

/**
 * Property 8: 渲染到 Canvas 成功
 */
TEST_F(PipelineRenderTest, RenderToCanvasSucceeds) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_NE(surface, nullptr);
    
    EXPECT_TRUE(pipeline_->ProcessFrame(surface->getCanvas()));
}

/**
 * Property 9: 渲染统计正确
 */
TEST_F(PipelineRenderTest, RenderStatsCorrect) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    
    const auto& stats = pipeline_->GetLastFrameStats();
    EXPECT_GE(stats.total_time, 0.0);
    EXPECT_GE(stats.layers_built, 1);  // 至少有根层
}

// =========================================================================
// 滚动测试
// =========================================================================

class PipelineScrollTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
        
        root_ = std::make_shared<TestRenderObject>();
        root_->SetBounds(0, 0, 800, 600);
        root_->SetScrollable(true);
        pipeline_->SetRenderTree(root_);
        
        // 先渲染一次以构建层树
        auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
        pipeline_->ProcessFrame(surface->getCanvas());
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    std::shared_ptr<TestRenderObject> root_;
};

/**
 * Property 10: 滚动处理成功
 */
TEST_F(PipelineScrollTest, HandleScrollSucceeds) {
    EXPECT_TRUE(pipeline_->HandleScroll(root_.get(), 0, 100));
}

/**
 * Property 11: ScrollTo 设置绝对位置
 */
TEST_F(PipelineScrollTest, ScrollToSetsAbsolutePosition) {
    pipeline_->HandleScroll(root_.get(), 0, 50);
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    ASSERT_FALSE(pipeline_->NeedsUpdate());

    EXPECT_TRUE(pipeline_->ScrollTo(root_.get(), 0, 200));
    
    EXPECT_FLOAT_EQ(root_->GetScrollY(), 200);
    EXPECT_TRUE(pipeline_->NeedsUpdate());
}

/**
 * Property 12: 滚动同步到 RenderObject
 */
TEST_F(PipelineScrollTest, ScrollSyncedToRenderObject) {
    pipeline_->HandleScroll(root_.get(), 50, 100);
    
    EXPECT_FLOAT_EQ(root_->GetScrollX(), 50);
    EXPECT_FLOAT_EQ(root_->GetScrollY(), 100);
}

TEST_F(PipelineScrollTest, ScrollInvalidationStatsRecordedPerFrame) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));

    EXPECT_FALSE(pipeline_->HasPendingScrollFullDirtyFallback());

    ASSERT_TRUE(pipeline_->HandleScroll(root_.get(), 0, 100));
    EXPECT_TRUE(pipeline_->HasPendingScrollFullDirtyFallback());

    ASSERT_TRUE(pipeline_->ProcessFrame(surface->getCanvas()));
    EXPECT_FALSE(pipeline_->HasPendingScrollFullDirtyFallback());

    const auto& scrolled_stats = pipeline_->GetLastFrameStats();
    EXPECT_EQ(scrolled_stats.scrolls_handled, 1);
    EXPECT_EQ(scrolled_stats.scroll_full_dirty_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_clip_layer_full_dirty_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_ancestor_layer_full_dirty_fallbacks, 0);
    EXPECT_EQ(scrolled_stats.scroll_missing_layer_target_fallbacks, 0);
    EXPECT_EQ(scrolled_stats.scroll_incremental_eligible_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_conservative_fallbacks, 0);
    EXPECT_EQ(scrolled_stats.scroll_retained_present_blocking_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::ClipLayerFullDirty);

    ASSERT_TRUE(pipeline_->ProcessFrame(surface->getCanvas()));
    const auto& idle_stats = pipeline_->GetLastFrameStats();
    EXPECT_EQ(idle_stats.scrolls_handled, 0);
    EXPECT_EQ(idle_stats.scroll_full_dirty_fallbacks, 0);
    EXPECT_EQ(idle_stats.scroll_retained_present_blocking_fallbacks, 0);
    EXPECT_EQ(idle_stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::None);
}

TEST_F(PipelineScrollTest, ScrollInvalidationStatsSurviveConfigSwitchBeforeFrame) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));

    ASSERT_TRUE(pipeline_->HandleScroll(root_.get(), 0, 100));
    EXPECT_TRUE(pipeline_->HasPendingScrollFullDirtyFallback());

    auto config = pipeline_->GetConfig();
    config.enable_incremental_layer_tree = false;
    pipeline_->SetConfig(config);
    EXPECT_TRUE(pipeline_->HasPendingScrollFullDirtyFallback());

    ASSERT_TRUE(pipeline_->ProcessFrame(surface->getCanvas()));
    EXPECT_FALSE(pipeline_->HasPendingScrollFullDirtyFallback());

    const auto& stats = pipeline_->GetLastFrameStats();
    EXPECT_EQ(stats.scrolls_handled, 1);
    EXPECT_EQ(stats.scroll_full_dirty_fallbacks, 1);
    EXPECT_EQ(stats.scroll_clip_layer_full_dirty_fallbacks, 1);
    EXPECT_EQ(stats.scroll_incremental_eligible_fallbacks, 1);
    EXPECT_EQ(stats.scroll_conservative_fallbacks, 0);
    EXPECT_EQ(stats.scroll_retained_present_blocking_fallbacks, 1);
    EXPECT_EQ(stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::ClipLayerFullDirty);
}

TEST_F(PipelineScrollTest, LegacyScrollInvalidationStatsRecordedPerFrame) {
    auto pipeline = std::make_unique<RenderPipeline>();
    UnifiedPipelineConfig config;
    config.enable_incremental_layer_tree = false;
    ASSERT_TRUE(pipeline->Initialize(800, 600, config));

    auto root = std::make_shared<TestRenderObject>();
    root->SetBounds(0, 0, 400, 300);
    root->SetScrollable(true);
    root->SetContentSize(800, 1000);
    root->ClearNeedsLayout();
    root->SetCompositorLayer(std::make_shared<CompositorLayer>(1));
    pipeline->SetRenderTree(root);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_TRUE(pipeline->ProcessFrame(surface->getCanvas()));
    EXPECT_FALSE(pipeline->HasPendingScrollFullDirtyFallback());

    ASSERT_TRUE(pipeline->HandleScroll(root.get(), 0, 100));
    EXPECT_TRUE(pipeline->HasPendingScrollFullDirtyFallback());
    ASSERT_TRUE(pipeline->ProcessFrame(surface->getCanvas()));
    EXPECT_FALSE(pipeline->HasPendingScrollFullDirtyFallback());

    const auto& scrolled_stats = pipeline->GetLastFrameStats();
    EXPECT_EQ(scrolled_stats.scrolls_handled, 1);
    EXPECT_EQ(scrolled_stats.scroll_full_dirty_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_clip_layer_full_dirty_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_incremental_eligible_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.scroll_conservative_fallbacks, 0);
    EXPECT_EQ(scrolled_stats.scroll_retained_present_blocking_fallbacks, 1);
    EXPECT_EQ(scrolled_stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::ClipLayerFullDirty);

    ASSERT_TRUE(pipeline->ProcessFrame(surface->getCanvas()));
    const auto& idle_stats = pipeline->GetLastFrameStats();
    EXPECT_EQ(idle_stats.scrolls_handled, 0);
    EXPECT_EQ(idle_stats.scroll_full_dirty_fallbacks, 0);
    EXPECT_EQ(idle_stats.scroll_retained_present_blocking_fallbacks, 0);
    EXPECT_EQ(idle_stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::None);
}

// =========================================================================
// 动画测试
// =========================================================================

TEST_F(PipelineScrollTest, MissingLayerScrollFallbackClassifiedConservative) {
    auto pipeline = std::make_unique<RenderPipeline>();
    UnifiedPipelineConfig config;
    config.enable_incremental_layer_tree = false;
    ASSERT_TRUE(pipeline->Initialize(800, 600, config));

    auto root = std::make_shared<TestRenderObject>();
    root->SetBounds(0, 0, 400, 300);
    root->SetScrollable(true);
    root->SetContentSize(800, 1000);
    root->ClearNeedsLayout();
    pipeline->SetRenderTree(root);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    ASSERT_TRUE(pipeline->ProcessFrame(surface->getCanvas()));
    root->SetCompositorLayer(nullptr);
    ASSERT_EQ(root->GetCompositorLayer(), nullptr);

    ASSERT_TRUE(pipeline->HandleScroll(root.get(), 0, 100));
    EXPECT_FALSE(pipeline->HasPendingScrollFullDirtyFallback());
    EXPECT_TRUE(pipeline->HasPendingScrollRetainedPresentBlockingFallback());
    ASSERT_TRUE(pipeline->ProcessFrame(surface->getCanvas()));

    const auto& stats = pipeline->GetLastFrameStats();
    EXPECT_EQ(stats.scrolls_handled, 1);
    EXPECT_EQ(stats.scroll_full_dirty_fallbacks, 0);
    EXPECT_EQ(stats.scroll_missing_layer_target_fallbacks, 1);
    EXPECT_EQ(stats.scroll_incremental_eligible_fallbacks, 0);
    EXPECT_EQ(stats.scroll_conservative_fallbacks, 1);
    EXPECT_EQ(stats.scroll_retained_present_blocking_fallbacks, 1);
    EXPECT_EQ(stats.last_scroll_invalidation_reason,
              ScrollInvalidationReason::MissingLayerTarget);
}

class PipelineAnimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
        
        root_ = std::make_shared<TestRenderObject>();
        root_->SetBounds(0, 0, 800, 600);
        
        animated_ = std::make_shared<TestRenderObject>();
        animated_->SetBounds(100, 100, 200, 200);
        root_->AppendChild(animated_);
        pipeline_->SetRenderTree(root_);
        
        // 先渲染一次
        auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
        pipeline_->ProcessFrame(surface->getCanvas());
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    std::shared_ptr<TestRenderObject> root_;
    std::shared_ptr<TestRenderObject> animated_;
};

/**
 * Property 13: 动画帧批处理工作
 */
TEST_F(PipelineAnimationTest, AnimationFrameBatchingWorks) {
    pipeline_->BeginAnimationFrame();
    
    // 应用多个动画属性
    auto type1 = pipeline_->UpdateAnimationProperty(animated_.get(), "opacity", "0.5");
    auto type2 = pipeline_->UpdateAnimationProperty(animated_.get(), "background-color", "red");
    
    bool has_updates = pipeline_->EndAnimationFrame();
    
    // opacity 应该返回 Paint 类型（因为没有独立层）
    EXPECT_EQ(type1, AnimationUpdateType::Paint);
    EXPECT_EQ(type2, AnimationUpdateType::Paint);
}

/**
 * Property 14: 动画开始/结束通知工作
 */
TEST_F(PipelineAnimationTest, AnimationLifecycleNotificationsWork) {
    std::vector<std::string> props = {"transform", "opacity"};
    
    // 不应该崩溃
    pipeline_->OnAnimationStart(animated_.get(), "test-anim", props);
    pipeline_->OnAnimationEnd(animated_.get(), "test-anim");
}

// =========================================================================
// 脏区域测试
// =========================================================================

class PipelineDirtyRegionTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
        
        root_ = std::make_shared<TestRenderObject>();
        root_->SetBounds(0, 0, 800, 600);
        
        child_ = std::make_shared<TestRenderObject>();
        child_->SetBounds(100, 100, 200, 200);
        root_->AppendChild(child_);
        pipeline_->SetRenderTree(root_);
        
        auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
        pipeline_->ProcessFrame(surface->getCanvas());
    }

    std::unique_ptr<RenderPipeline> pipeline_;
    std::shared_ptr<TestRenderObject> root_;
    std::shared_ptr<TestRenderObject> child_;
};

/**
 * Property 15: MarkDirty 设置 NeedsUpdate
 */
TEST_F(PipelineDirtyRegionTest, MarkDirtySetsNeedsUpdate) {
    EXPECT_FALSE(pipeline_->NeedsUpdate());
    
    pipeline_->MarkDirty(child_.get());
    
    EXPECT_TRUE(pipeline_->NeedsUpdate());
}

/**
 * Property 16: MarkDirtyRegion 设置 NeedsUpdate
 */
TEST_F(PipelineDirtyRegionTest, MarkDirtyRegionSetsNeedsUpdate) {
    EXPECT_FALSE(pipeline_->NeedsUpdate());
    
    pipeline_->MarkDirtyRegion(SkRect::MakeXYWH(0, 0, 100, 100));
    
    EXPECT_TRUE(pipeline_->NeedsUpdate());
}

// =========================================================================
// 配置测试
// =========================================================================

TEST_F(PipelineDirtyRegionTest, DirtyChildInsideScrolledLayerUsesVisibleLayerSpace) {
    auto pipeline = std::make_unique<RenderPipeline>();
    pipeline->Initialize(800, 600);

    auto root = std::make_shared<TestRenderObject>();
    root->SetBounds(0, 0, 800, 600);

    auto scroll_container = std::make_shared<TestRenderObject>();
    scroll_container->SetBounds(0, 0, 300, 200);
    scroll_container->SetScrollable(true);
    scroll_container->SetScrollY(260);

    auto scrolled_child = std::make_shared<TestRenderObject>();
    scrolled_child->SetBounds(20, 300, 80, 40);
    scrolled_child->SetPaintColor(SK_ColorRED);

    scroll_container->AppendChild(scrolled_child);
    root->AppendChild(scroll_container);
    pipeline->SetRenderTree(root);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline->ProcessFrame(surface->getCanvas());

    scrolled_child->SetPaintColor(SK_ColorGREEN);
    scrolled_child->MarkNeedsPaint();
    pipeline->MarkNeedsPaint();
    pipeline->ProcessFrame(surface->getCanvas());

    const auto& stats = pipeline->GetLastFrameStats();
    EXPECT_EQ(stats.full_rasterizations, 0);
    EXPECT_EQ(stats.incremental_rasterizations, 1);
    EXPECT_GT(stats.pixels_rasterized, 0);
    EXPECT_LT(stats.pixels_rasterized, 300 * 200);
}

class PipelineConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
    }

    std::unique_ptr<RenderPipeline> pipeline_;
};

/**
 * Property 17: SetConfig 更新配置
 */
TEST_F(PipelineConfigTest, SetConfigUpdatesConfiguration) {
    UnifiedPipelineConfig new_config;
    new_config.enable_layer_promotion = false;
    new_config.show_layer_borders = true;
    
    pipeline_->SetConfig(new_config);
    
    const auto& config = pipeline_->GetConfig();
    EXPECT_FALSE(config.enable_layer_promotion);
    EXPECT_TRUE(config.show_layer_borders);
}

/**
 * Property 18: SetShowLayerBorders 更新配置
 */
TEST_F(PipelineConfigTest, SetShowLayerBordersUpdatesConfig) {
    pipeline_->SetShowLayerBorders(true);
    EXPECT_TRUE(pipeline_->GetConfig().show_layer_borders);
    
    pipeline_->SetShowLayerBorders(false);
    EXPECT_FALSE(pipeline_->GetConfig().show_layer_borders);
}

// =========================================================================
// 组件访问测试
// =========================================================================

class PipelineComponentAccessTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
        pipeline_->Initialize(800, 600);
    }

    std::unique_ptr<RenderPipeline> pipeline_;
};

/**
 * Property 19: 组件访问器返回有效指针
 */
TEST_F(PipelineComponentAccessTest, ComponentAccessorsReturnValidPointers) {
    EXPECT_NE(pipeline_->GetLayerTreeBuilder(), nullptr);
    EXPECT_NE(pipeline_->GetRasterizer(), nullptr);
    EXPECT_NE(pipeline_->GetCompositor(), nullptr);
    EXPECT_NE(pipeline_->GetAnimationBridge(), nullptr);
    EXPECT_NE(pipeline_->GetScrollManager(), nullptr);
}

/**
 * Property 20: 渲染后有根层
 */
TEST_F(PipelineComponentAccessTest, RootLayerExistsAfterRender) {
    auto root = std::make_shared<TestRenderObject>();
    root->SetBounds(0, 0, 800, 600);
    pipeline_->SetRenderTree(root);
    
    EXPECT_EQ(pipeline_->GetRootLayer(), nullptr);
    
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    
    EXPECT_NE(pipeline_->GetRootLayer(), nullptr);
}

// =========================================================================
// 边界情况测试
// =========================================================================

class PipelineEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<RenderPipeline>();
    }

    std::unique_ptr<RenderPipeline> pipeline_;
};

/**
 * Property 21: 未初始化时渲染返回 false
 */
TEST_F(PipelineEdgeCaseTest, RenderFailsWhenNotInitialized) {
    auto root = std::make_shared<TestRenderObject>();
    pipeline_->SetRenderTree(root);
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    EXPECT_FALSE(pipeline_->ProcessFrame(surface->getCanvas()));
}

/**
 * Property 22: nullptr canvas 渲染返回 false
 */
TEST_F(PipelineEdgeCaseTest, RenderFailsWithNullCanvas) {
    pipeline_->Initialize(800, 600);
    auto root = std::make_shared<TestRenderObject>();
    pipeline_->SetRenderTree(root);
    EXPECT_FALSE(pipeline_->ProcessFrame(nullptr));
}

/**
 * Property 23: Resize 更新视口
 */
TEST_F(PipelineEdgeCaseTest, ResizeUpdatesViewport) {
    pipeline_->Initialize(800, 600);
    
    auto root = std::make_shared<TestRenderObject>();
    pipeline_->SetRenderTree(root);
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    pipeline_->ProcessFrame(surface->getCanvas());
    EXPECT_FALSE(pipeline_->NeedsUpdate());
    
    pipeline_->Resize(1024, 768);
    EXPECT_TRUE(pipeline_->NeedsUpdate());
}

