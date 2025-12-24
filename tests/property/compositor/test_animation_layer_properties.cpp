/**
 * @file test_animation_layer_properties.cpp
 * @brief 动画层集成属性测试
 *
 * 测试属性：
 * - Property 7: Transform/opacity 动画不需要重新光栅化
 * - 动画触发的层提升/降级
 */

#include <gtest/gtest.h>
#include "core/compositor/animation_layer_bridge.h"
#include "core/compositor/compositor_layer.h"
#include "core/compositor/layer_tree_builder.h"
#include "core/render/render_object.h"
#include "include/core/SkMatrix.h"
#include <memory>

namespace lightui {
namespace testing {

// =========================================================================
// 测试辅助类
// =========================================================================

/**
 * @brief 测试用渲染对象
 */
class TestAnimRenderObject : public RenderObject {
public:
    TestAnimRenderObject() : RenderObject(RenderObjectType::BLOCK) {
        layout_info_.width = 100;
        layout_info_.height = 100;
        layout_info_.is_laid_out = true;
    }

    void SetHasOwnLayer(bool has) {
        if (has) {
            layer_ = CreateCompositorLayer();
            layer_->SetBounds(SkRect::MakeWH(100, 100));
            layer_->SetRenderObject(this);
            layer_info_.compositor_layer = layer_;
        } else {
            layer_.reset();
            layer_info_.compositor_layer.reset();
        }
    }

    std::shared_ptr<CompositorLayer> GetTestLayer() const { return layer_; }

private:
    std::shared_ptr<CompositorLayer> layer_;
};

// =========================================================================
// Property 7: Transform/opacity 动画不需要重新光栅化
// =========================================================================

class AnimationLayerOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge_ = std::make_unique<AnimationLayerBridge>();
        layer_builder_ = std::make_unique<LayerTreeBuilder>();
        bridge_->SetLayerTreeBuilder(layer_builder_.get());
    }

    std::unique_ptr<AnimationLayerBridge> bridge_;
    std::unique_ptr<LayerTreeBuilder> layer_builder_;
};

// 测试：有独立层的 transform 动画直接更新层
TEST_F(AnimationLayerOptimizationTest, TransformAnimationUpdatesLayerDirectly) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    obj->SetHasOwnLayer(true);

    auto layer = obj->GetTestLayer();
    ASSERT_NE(layer, nullptr);

    // 初始变换是单位矩阵
    SkMatrix initial = layer->GetTransform();
    EXPECT_TRUE(initial.isIdentity());

    // 应用 transform 动画
    bridge_->BeginAnimationUpdates();
    auto update_type = bridge_->ApplyAnimationProperty(obj.get(), "transform", "translateX(50px)");
    bool has_updates = bridge_->EndAnimationUpdates();

    // 应该是 Transform 类型更新
    EXPECT_EQ(update_type, AnimationUpdateType::Transform);
    EXPECT_TRUE(has_updates);

    // 层变换应该已更新
    SkMatrix updated = layer->GetTransform();
    EXPECT_FALSE(updated.isIdentity());

    // 验证平移值
    EXPECT_NEAR(updated.getTranslateX(), 50.0f, 0.1f);
}

// 测试：有独立层的 opacity 动画直接更新层
TEST_F(AnimationLayerOptimizationTest, OpacityAnimationUpdatesLayerDirectly) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    obj->SetHasOwnLayer(true);

    auto layer = obj->GetTestLayer();
    ASSERT_NE(layer, nullptr);

    // 初始透明度是 1.0
    EXPECT_FLOAT_EQ(layer->GetOpacity(), 1.0f);

    // 应用 opacity 动画
    bridge_->BeginAnimationUpdates();
    auto update_type = bridge_->ApplyAnimationProperty(obj.get(), "opacity", "0.5");
    bool has_updates = bridge_->EndAnimationUpdates();

    // 应该是 Opacity 类型更新
    EXPECT_EQ(update_type, AnimationUpdateType::Opacity);
    EXPECT_TRUE(has_updates);

    // 层透明度应该已更新
    EXPECT_FLOAT_EQ(layer->GetOpacity(), 0.5f);
}

// 测试：没有独立层时回退到 Paint 更新
TEST_F(AnimationLayerOptimizationTest, FallbackToPaintWithoutLayer) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    // 不设置独立层

    bridge_->BeginAnimationUpdates();
    auto transform_type = bridge_->ApplyAnimationProperty(obj.get(), "transform", "rotate(45deg)");
    auto opacity_type = bridge_->ApplyAnimationProperty(obj.get(), "opacity", "0.8");
    bridge_->EndAnimationUpdates();

    // 没有独立层，应该回退到 Paint
    EXPECT_EQ(transform_type, AnimationUpdateType::Paint);
    EXPECT_EQ(opacity_type, AnimationUpdateType::Paint);
}

// 测试：布局属性返回 Layout 类型
TEST_F(AnimationLayerOptimizationTest, LayoutPropertiesReturnLayoutType) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    obj->SetHasOwnLayer(true);

    bridge_->BeginAnimationUpdates();

    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "width", "200px"),
              AnimationUpdateType::Layout);
    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "height", "150px"),
              AnimationUpdateType::Layout);
    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "margin-top", "10px"),
              AnimationUpdateType::Layout);
    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "left", "20px"),
              AnimationUpdateType::Layout);

    bridge_->EndAnimationUpdates();
}

// 测试：其他属性返回 Paint 类型
TEST_F(AnimationLayerOptimizationTest, OtherPropertiesReturnPaintType) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    obj->SetHasOwnLayer(true);

    bridge_->BeginAnimationUpdates();

    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "background-color", "#ff0000"),
              AnimationUpdateType::Paint);
    EXPECT_EQ(bridge_->ApplyAnimationProperty(obj.get(), "color", "blue"),
              AnimationUpdateType::Paint);

    bridge_->EndAnimationUpdates();
}

// =========================================================================
// 动画触发的层提升/降级
// =========================================================================

class AnimationLayerPromotionTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge_ = std::make_unique<AnimationLayerBridge>();
        layer_builder_ = std::make_unique<LayerTreeBuilder>();
        bridge_->SetLayerTreeBuilder(layer_builder_.get());
    }

    std::unique_ptr<AnimationLayerBridge> bridge_;
    std::unique_ptr<LayerTreeBuilder> layer_builder_;
};

// 测试：transform 动画开始时请求层提升
TEST_F(AnimationLayerPromotionTest, TransformAnimationRequestsPromotion) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    EXPECT_FALSE(bridge_->IsPromotedForAnimation(obj.get()));

    // 通知动画开始
    bridge_->OnAnimationStart(obj.get(), "slide", {"transform"});

    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_TRUE(bridge_->HasTransformAnimation(obj.get()));

    // 检查 LayerInfo
    const auto& layer_info = obj->GetLayerInfo();
    EXPECT_TRUE(layer_info.force_own_layer);
    EXPECT_EQ(layer_info.promotion_reason, LayerPromotionReason::TransformAnimation);
}

// 测试：opacity 动画开始时请求层提升
TEST_F(AnimationLayerPromotionTest, OpacityAnimationRequestsPromotion) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    bridge_->OnAnimationStart(obj.get(), "fade", {"opacity"});

    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_TRUE(bridge_->HasOpacityAnimation(obj.get()));

    const auto& layer_info = obj->GetLayerInfo();
    EXPECT_EQ(layer_info.promotion_reason, LayerPromotionReason::OpacityAnimation);
}

// 测试：非 transform/opacity 动画不请求层提升
TEST_F(AnimationLayerPromotionTest, OtherAnimationsNoPromotion) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    bridge_->OnAnimationStart(obj.get(), "color-change", {"background-color", "color"});

    EXPECT_FALSE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_FALSE(bridge_->HasTransformAnimation(obj.get()));
    EXPECT_FALSE(bridge_->HasOpacityAnimation(obj.get()));
}

// 测试：动画结束时请求层降级
TEST_F(AnimationLayerPromotionTest, AnimationEndRequestsDemotion) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    // 开始动画
    bridge_->OnAnimationStart(obj.get(), "slide", {"transform"});
    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));

    // 结束动画
    bridge_->OnAnimationEnd(obj.get(), "slide");
    EXPECT_FALSE(bridge_->IsPromotedForAnimation(obj.get()));

    // LayerInfo 应该被清除
    const auto& layer_info = obj->GetLayerInfo();
    EXPECT_FALSE(layer_info.force_own_layer);
    EXPECT_EQ(layer_info.promotion_reason, LayerPromotionReason::None);
}

// 测试：多个动画时只有全部结束才降级
TEST_F(AnimationLayerPromotionTest, MultipleAnimationsNoDemotionUntilAllEnd) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    // 开始两个动画
    bridge_->OnAnimationStart(obj.get(), "slide", {"transform"});
    bridge_->OnAnimationStart(obj.get(), "fade", {"opacity"});

    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_TRUE(bridge_->HasTransformAnimation(obj.get()));
    EXPECT_TRUE(bridge_->HasOpacityAnimation(obj.get()));

    // 结束第一个动画
    bridge_->OnAnimationEnd(obj.get(), "slide");
    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));  // 仍然提升
    EXPECT_FALSE(bridge_->HasTransformAnimation(obj.get()));
    EXPECT_TRUE(bridge_->HasOpacityAnimation(obj.get()));

    // 结束第二个动画
    bridge_->OnAnimationEnd(obj.get(), "fade");
    EXPECT_FALSE(bridge_->IsPromotedForAnimation(obj.get()));  // 现在降级
}

// =========================================================================
// 批处理测试
// =========================================================================

class AnimationBatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge_ = std::make_unique<AnimationLayerBridge>();
    }

    std::unique_ptr<AnimationLayerBridge> bridge_;
};

// 测试：批处理收集更新
TEST_F(AnimationBatchTest, BatchCollectsUpdates) {
    auto obj1 = std::make_shared<TestAnimRenderObject>();
    auto obj2 = std::make_shared<TestAnimRenderObject>();
    obj1->SetHasOwnLayer(true);
    obj2->SetHasOwnLayer(true);

    bridge_->BeginAnimationUpdates();

    bridge_->ApplyAnimationProperty(obj1.get(), "transform", "scale(1.5)");
    bridge_->ApplyAnimationProperty(obj2.get(), "opacity", "0.7");

    bool has_updates = bridge_->EndAnimationUpdates();

    EXPECT_TRUE(has_updates);

    const auto& updates = bridge_->GetPendingUpdates();
    EXPECT_EQ(updates.size(), 2u);
}

// 测试：无更新时返回 false
TEST_F(AnimationBatchTest, NoUpdatesReturnsFalse) {
    auto obj = std::make_shared<TestAnimRenderObject>();
    // 没有独立层

    bridge_->BeginAnimationUpdates();
    bridge_->ApplyAnimationProperty(obj.get(), "transform", "rotate(30deg)");
    bool has_updates = bridge_->EndAnimationUpdates();

    // 没有层优化，所以没有层更新
    EXPECT_FALSE(has_updates);
}

// =========================================================================
// 清理测试
// =========================================================================

class AnimationClearTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge_ = std::make_unique<AnimationLayerBridge>();
    }

    std::unique_ptr<AnimationLayerBridge> bridge_;
};

// 测试：Clear 清除所有状态
TEST_F(AnimationClearTest, ClearResetsAllState) {
    auto obj = std::make_shared<TestAnimRenderObject>();

    bridge_->OnAnimationStart(obj.get(), "test", {"transform", "opacity"});
    EXPECT_TRUE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_GT(bridge_->GetAnimationPromotedCount(), 0u);

    bridge_->Clear();

    EXPECT_FALSE(bridge_->IsPromotedForAnimation(obj.get()));
    EXPECT_EQ(bridge_->GetAnimationPromotedCount(), 0u);
    EXPECT_FALSE(bridge_->HasTransformAnimation(obj.get()));
    EXPECT_FALSE(bridge_->HasOpacityAnimation(obj.get()));
}

} // namespace testing
} // namespace lightui
